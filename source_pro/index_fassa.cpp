#include "protengine_index_fassa.h"

#include <iostream>

#include "whodun_oshook.h"
#include "whodun_suffix.h"
#include "whodun_stringext.h"
#include "protengine_index_failgz.h"

int buildFassaFromFailgz(FailGZReader* failFile, const char* fassaFileName, CompressionMethod* useComp){
	char toDumpLong[FPSZ];
	int wasProb;
	//open the output file
	FILE* fassFile = fopen(fassaFileName, "wb");
	if(fassFile == 0){
		std::cerr << "Problem writing " << fassaFileName << std::endl;
		return 1;
	}
	//write the number of entries, and zeros to come back to
	canonPrepareInt64(failFile->numEntries, toDumpLong);
		fwrite(toDumpLong, 1, FPSZ, fassFile);
	canonPrepareInt64(0, toDumpLong);
	for(intptr_t i = 0; i<failFile->numEntries; i++){
		 fwrite(toDumpLong, 1, FPSZ, fassFile);
	}
	std::vector<intptr_t> entryTargets;
	//run through the entries, writing out their info
	for(intptr_t entryI = 0; entryI < failFile->numEntries; entryI++){
		if(!(entryI & 0x00FF)){
			std::cout << (entryI+1) << "/" << failFile->numEntries << std::endl;
		}
		//read the entry itself
		wasProb = failFile->loadInEntry(entryI);
		if(wasProb){
			fclose(fassFile); killFile(fassaFileName);
			return 1;
		}
		//suffix array sort is all in memory, regardless
		uintptr_t numSI = failFile->seqLen;
		uintptr_t* sortResults = (uintptr_t*)malloc(numSI*sizeof(uintptr_t));
		buildSuffixArray(failFile->seq, sortResults);
		//copy the sort results to a buffer
		char* sortResultDecomBuff = (char*)malloc(numSI*FPSZ);
		for(uintptr_t i = 0; i<numSI; i++){
			canonPrepareInt64(sortResults[i], sortResultDecomBuff+(FPSZ*i));
		}
		free(sortResults);
		//prepare to write out the raw data, and note the location of each block
		std::vector<intptr_t> blockLocs;
		for(uintptr_t i = 0; i<numSI; i+=FASSSA_BLOCK_SIZE){
			intptr_t blockLoc = ftellPointer(fassFile);
			blockLocs.push_back(blockLoc);
			uintptr_t ni = i+FASSSA_BLOCK_SIZE;
			char* startComp = sortResultDecomBuff + (8*i);
			unsigned long plainLen;
			if(ni > numSI){
				plainLen = (numSI - i);
			}
			else{
				plainLen = FASSSA_BLOCK_SIZE;
			}
			plainLen = plainLen * 8;
			uintptr_t* compdLenp = useComp->compressData(plainLen, startComp);
			if(!compdLenp){
				std::cerr << "Problem writing " << fassaFileName << std::endl;
				free(sortResultDecomBuff);
				fclose(fassFile); killFile(fassaFileName);
				return 1;
			}
			else{
				uintptr_t compdLen = *compdLenp;
				char* compdBuff = (char*)(compdLenp + 1);
				canonPrepareInt64(compdLen, toDumpLong);
				fwrite(toDumpLong, 1, FPSZ, fassFile);
				fwrite(compdBuff, 1, compdLen, fassFile);
				free(compdLenp);
			}
		}
		free(sortResultDecomBuff);
		//add the entry data
		intptr_t returnLoc = ftellPointer(fassFile);
		entryTargets.push_back(returnLoc);
		//and make a block entry table
		uintptr_t numBlockEnts = blockLocs.size();
		canonPrepareInt64(numBlockEnts, toDumpLong);
			fwrite(toDumpLong, 1, FPSZ, fassFile);
		for(uintptr_t i = 0; i<numBlockEnts; i++){
			canonPrepareInt64(blockLocs[i], toDumpLong);
			fwrite(toDumpLong, 1, FPSZ, fassFile);
		}
	}
	//go back and write the entry data
	long int curEntryOffset = FPSZ;
	fseekPointer(fassFile, curEntryOffset, SEEK_SET);
	for(uintptr_t i = 0; i<entryTargets.size(); i++){
		canonPrepareInt64(entryTargets[i], toDumpLong);
		fwrite(toDumpLong, 1, FPSZ, fassFile);
	}
	fclose(fassFile);
	return 0;
}

FassaReader::FassaReader(const char* fassaFileName, CompressionMethod* compMeth){
	char toDumpLong[FPSZ];
	//set up basic variables
	useComp = compMeth;
	seqOffAlloc = 8;
	sequenceOffsets = (intptr_t*)malloc(seqOffAlloc*sizeof(intptr_t));
	decomAlloc = 64;
	decomBuffer = (char*)malloc(decomAlloc);
	currentAlloc = 64;
	currentSequence = (uintptr_t*)malloc(currentAlloc*sizeof(uintptr_t));
	filptrBuffer = (char*)malloc(currentAlloc*FPSZ);
	//open the file
	entryOffsets = 0;
	actualFile = fopen(fassaFileName, "rb");
	if(actualFile == 0){
		numEntries = -1;
		return;
	}
	//load in the offset table
	if(fread(toDumpLong, FPSZ, 1, actualFile) != 1){
		fclose(actualFile); actualFile = 0;
		numEntries = -1;
		return;
	}
	uint_least64_t numEnts = canonParseInt64(toDumpLong);
	if(numEnts & MAX_MASK(int)){
		std::cerr << "Too many entries in file for this to handle." << std::endl;
		fclose(actualFile); actualFile = 0;
		numEntries = -1;
		return;
	}
	numEntries = (uintptr_t)numEnts;
	entryOffsets = (intptr_t*)malloc(numEntries * sizeof(intptr_t));
	for(intptr_t i = 0; i<numEntries; i++){
		if(fread(toDumpLong, FPSZ, 1, actualFile) != 1){
			std::cerr << "File missing some header information." << std::endl;
			fclose(actualFile); actualFile = 0;
			numEntries = -1;
			return;
		}
		uint_least64_t curOffset = canonParseInt64(toDumpLong);
		if(curOffset & MAX_MASK(intptr_t)){
			std::cerr << "File offset too big for this program to handle." << std::endl;
			fclose(actualFile); actualFile = 0;
			numEntries = -1;
			return;
		}
		entryOffsets[i] = (intptr_t)curOffset;
	}
}

FassaReader::~FassaReader(){
	free(sequenceOffsets);
	free(currentSequence);
	free(filptrBuffer);
	free(decomBuffer);
	if(entryOffsets){ free(entryOffsets); }
	if(actualFile){ fclose(actualFile); }
}

int FassaReader::loadInEntry(uintptr_t entryID){
	char toDumpLong[FPSZ];
	if(actualFile == 0){
		std::cerr << "Problem reading fassa file." << std::endl;
		return 1;
	}
	int wasErr;
	wasErr = fseekPointer(actualFile, entryOffsets[entryID], SEEK_SET);
	if(wasErr){
		std::cerr << "Problem reading fassa file." << std::endl;
		return 1;
	}
	//read the count
	if(fread(toDumpLong, FPSZ, 1, actualFile) != 1){
		std::cerr << "Problem reading fassa file." << std::endl;
		return 1;
	}
	uint_least64_t numSeqs = canonParseInt64(toDumpLong);
	if(numSeqs & MAX_MASK(int)){
		std::cerr << "Too many sequences in file for this to handle." << std::endl;
		return 1;
	}
	numSequences = (uintptr_t)numSeqs;
	//check allocation
	if(numSequences > seqOffAlloc){
		free(sequenceOffsets);
		sequenceOffsets = (intptr_t*)malloc(numSequences*sizeof(intptr_t));
		seqOffAlloc = numSequences;
	}
	//read the offsets
	for(uintptr_t i = 0; i<numSequences; i++){
		if(fread(toDumpLong, FPSZ, 1, actualFile) != 1){
			std::cerr << "Sequence some header information." << std::endl;
			return 1;
		}
		uint_least64_t curOffset = canonParseInt64(toDumpLong);
		if(curOffset & MAX_MASK(intptr_t)){
			std::cerr << "File offset too big for this program to handle." << std::endl;
			return 1;
		}
		sequenceOffsets[i] = (intptr_t)curOffset;
	}
	return 0;
}

int FassaReader::loadInSequence(uintptr_t seqID){
	if(actualFile == 0){
		std::cerr << "Problem reading fassa file." << std::endl;
		return 1;
	}
	int wasErr;
	wasErr = fseekPointer(actualFile, sequenceOffsets[seqID], SEEK_SET);
	if(wasErr){
		std::cerr << "Problem reading fassa file." << std::endl;
		return 1;
	}
	//read the block, raw
	ReadCompressedAtomSpec workingSpec;
	workingSpec.requireNullTerm = false;
	workingSpec.decomBuffer = decomBuffer;
	workingSpec.decomAlloc = decomAlloc;
	workingSpec.storeBuffer = filptrBuffer;
	workingSpec.storeAlloc = FPSZ*currentAlloc;
	wasErr = readCompressedBlockAtom(actualFile, useComp, &workingSpec);
	filptrBuffer = workingSpec.storeBuffer;
	uintptr_t numRead = workingSpec.resultLen / FPSZ;
	decomBuffer = workingSpec.decomBuffer;
	decomAlloc = workingSpec.decomAlloc;
	uintptr_t updateAlloc = workingSpec.storeAlloc / FPSZ;
	if(updateAlloc != currentAlloc){
		free(currentSequence);
		currentAlloc = updateAlloc;
		currentSequence = (uintptr_t*)malloc(currentAlloc*sizeof(uintptr_t));
	}
	if(workingSpec.resultLen % FPSZ){ return 1; }
	if(wasErr){ return wasErr; }
	numInCurrent = numRead;
	for(uintptr_t i = 0; i<numInCurrent; i++){
		uint_least64_t curValue = canonParseInt64(filptrBuffer + FPSZ*i);
		if(curValue & MAX_MASK(uintptr_t)){
			std::cerr << "Value too big for this program to handle." << std::endl;
			return 1;
		}
		currentSequence[i] = (uintptr_t)curValue;
	}
	return 0;
}

int findSequencesInFassa(FailGZReader* failFile, FassaReader* fassaFile, std::vector<const char*>* findSeqs, uintptr_t findSeqInd, SequenceFindCallback* toReport){
	for(intptr_t i = 0; i<failFile->numEntries; i++){
		int readPro = failFile->loadInEntry(i);
		if(readPro){
			return 1;
		}
		readPro = fassaFile->loadInEntry(i);
		if(readPro){
			return 1;
		}
		//run through all the sequences, and search
		for(uintptr_t j = 0; j<findSeqs->size(); j++){
			const char* curLookFor = (*findSeqs)[j];
			uintptr_t findLen = strlen(curLookFor);
			//figure out which block it starts at (last block where str > block[-1])+1
			uintptr_t lowBlock = 0;
			uintptr_t higBlock = fassaFile->numSequences;
			while((higBlock - lowBlock) > 0){
				uintptr_t midBlock = lowBlock + ((higBlock - lowBlock)>>1);
				//load in the block
				readPro = fassaFile->loadInSequence(midBlock);
				if(readPro){
					return 1;
				}
				//figure out what the last string is
				const char* lastStr = failFile->seq + fassaFile->currentSequence[fassaFile->numInCurrent-1];
				//compare and move
				int compVal = strncmp(lastStr, curLookFor, findLen);
				if(compVal < 0){
					lowBlock = midBlock + 1;
				}
				else{
					higBlock = midBlock;
				}
			}
			uintptr_t startBlock = lowBlock;
			//figure out which block it ends at (first block where str < block[0])-1
			lowBlock = 0;
			higBlock = fassaFile->numSequences;
			while((higBlock - lowBlock) > 0){
				uintptr_t midBlock = lowBlock + ((higBlock - lowBlock)>>1);
				//load in the block
				readPro = fassaFile->loadInSequence(midBlock);
				if(readPro){
					return 1;
				}
				//figure out what the last string is
				const char* firstStr = failFile->seq + fassaFile->currentSequence[0];
				//compare and move
				int compVal = strncmp(firstStr, curLookFor, findLen);
				if(compVal <= 0){
					lowBlock = midBlock + 1;
				}
				else{
					higBlock = midBlock;
				}
			}
			uintptr_t endBlock = lowBlock;
			//run through all of them
			for(uintptr_t bi = startBlock; bi<endBlock; bi++){
				readPro = fassaFile->loadInSequence(bi);
				if(readPro){
					return 1;
				}
				//find the bounding elements
				uintptr_t lowerBound = suffixArrayLowerBound(curLookFor, failFile->seq, fassaFile->currentSequence, fassaFile->numInCurrent);
				uintptr_t upperBound = suffixArrayUpperBound(curLookFor, failFile->seq, fassaFile->currentSequence, fassaFile->numInCurrent);
				//run down
				readPro = 0;
				for(uintptr_t k = lowerBound; k<upperBound; k++){
					readPro = readPro | toReport->foundSequence(findSeqInd+j, i, fassaFile->currentSequence[k]);
				}
				//clean up
				if(readPro){
					return 1;
				}
			}
		}
	}
	return 0;
}
