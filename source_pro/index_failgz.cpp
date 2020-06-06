#include "protengine_index_failgz.h"

#include <vector>
#include <iostream>
#include <stdlib.h>
#include <string.h>

#include "whodun_bamfa.h"
#include "whodun_oshook.h"
#include "whodun_suffix.h"
#include "whodun_stringext.h"

int buildFailgzFromFastAQ(const char* fqFileName, const char* failFileName, CompressionMethod* useComp){
	char toDumpLong[FPSZ];
	//open up the fasta file and figure out how many entries it has
	LiveFastqFileReader* readPass1 = openFastqFile(fqFileName);
	if(!readPass1){
		std::cerr << "Problem reading " << fqFileName << std::endl;
		return 1;
	}
	uintptr_t totNumSeqs = 0;
	while(readNextFastqEntry(readPass1)){
		totNumSeqs++;
	}
	closeFastqFile(readPass1);
	//open up the target file
	FILE* tgtFile = fopen(failFileName, "wb");
	if(tgtFile == 0){
		std::cerr << "Problem writing " << failFileName << std::endl;
		return 1;
	}
	//make the entry table
	canonPrepareInt64(totNumSeqs, toDumpLong);
		fwrite(toDumpLong, 1, FPSZ, tgtFile);
	canonPrepareInt64(0, toDumpLong);
	for(uintptr_t i = 0; i<totNumSeqs; i++){
		 fwrite(toDumpLong, 1, FPSZ, tgtFile);
	}
	//and run down the fasta entries
	std::vector<intptr_t> entryTargets;
	LiveFastqFileReader* readPass2 = openFastqFile(fqFileName);
	while(readNextFastqEntry(readPass2)){
		//add the entry data
		intptr_t returnLoc = ftellPointer(tgtFile);
		entryTargets.push_back(returnLoc);
		//compress the three strings (Name Seq Qual)
		uintptr_t* compdLenp;
		char* compdBuff;
		#define DUMP_ONE_BUFFER(intProp) \
			compdLenp = useComp->compressData(strlen(readPass2->intProp)+1, readPass2->intProp);\
			if(!compdLenp){\
				std::cerr << "Problem writing " << failFileName << std::endl;\
				fclose(tgtFile); closeFastqFile(readPass2);\
				killFile(failFileName);\
				return 1;\
			}\
			else{\
				uintptr_t compdLen = *compdLenp;\
				compdBuff = (char*)(compdLenp + 1);\
				canonPrepareInt64(compdLen, toDumpLong);\
				fwrite(toDumpLong, 1, FPSZ, tgtFile);\
				fwrite(compdBuff, 1, compdLen, tgtFile);\
				free(compdLenp);\
			}
		DUMP_ONE_BUFFER(lastReadName)
		DUMP_ONE_BUFFER(lastReadSeq)
		DUMP_ONE_BUFFER(lastReadQual)
		//add a zero for future proof
		canonPrepareInt64(0, toDumpLong);
		fwrite(toDumpLong, 1, FPSZ, tgtFile);
	}
	closeFastqFile(readPass2);
	//go back and write the entry data
	uintptr_t curEntryOffset = 8;
	fseekPointer(tgtFile, curEntryOffset, SEEK_SET);
	for(uintptr_t i = 0; i<entryTargets.size(); i++){
		canonPrepareInt64(entryTargets[i], toDumpLong);
		fwrite(toDumpLong, 1, FPSZ, tgtFile);
	}
	fclose(tgtFile);
	return 0;
}

int trippy(){
	std::cout << "Here" << std::endl;
	return 0;
}

int readCompressedBlockAtom(FILE* toRead, CompressionMethod* useComp, ReadCompressedAtomSpec* workSpace){
	char toDumpLong[FPSZ];
	if(fread(toDumpLong, FPSZ, 1, toRead) != 1){
		std::cerr << "Problem reading file." << std::endl;
		return 1;
	}
	uint_least64_t readLen = canonParseInt64(toDumpLong);
	if(readLen & MAX_MASK(int)){
		std::cerr << "Entry too large for this system." << std::endl;
		return 1;
	}
	if(readLen > workSpace->decomAlloc){
		free(workSpace->decomBuffer);
		workSpace->decomAlloc = readLen;
		workSpace->decomBuffer = (char*)malloc(readLen);
	}
	if(fread(workSpace->decomBuffer, 1, readLen, toRead) != readLen){
		std::cerr << "Problem reading file." << std::endl;
		return 1;
	}
	uintptr_t* decomLP = useComp->decompressData(readLen, workSpace->decomBuffer);
	if(!decomLP){
		std::cerr << "Problem reading file." << std::endl;
		return 1;
	}
	char* decomBuf = (char*)(decomLP + 1);;
	uintptr_t decomL = *decomLP;
	bool addZero = workSpace->requireNullTerm && ((decomL == 0) || (decomBuf[decomL-1]));
	uintptr_t needDecomL = decomL + (addZero ? 1 : 0);
	if(needDecomL > workSpace->storeAlloc){
		free(workSpace->storeBuffer);
		workSpace->storeAlloc = needDecomL;
		workSpace->storeBuffer = (char*)malloc(needDecomL);
	}
	memcpy(workSpace->storeBuffer, decomBuf, decomL);
	if(addZero){
		workSpace->storeBuffer[decomL] = 0;
	}
	workSpace->resultLen = needDecomL;
	free(decomLP);
	return 0;
}

FailGZReader::FailGZReader(const char* failFileName, CompressionMethod* compMeth){
	char toDumpLong[FPSZ];
	//set up basic variables
	useComp = compMeth;
	nameAlloc = 64;
	name = (char*)malloc(nameAlloc);
	seqAlloc = 64;
	seq = (char*)malloc(seqAlloc);
	qualAlloc = 64;
	qual = (char*)malloc(qualAlloc);
	decomAlloc = 64;
	decomBuffer = (char*)malloc(decomAlloc);
	//open the file
	allOffsets = 0;
	actualFile = fopen(failFileName, "rb");
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
	allOffsets = (intptr_t*)malloc(numEntries * sizeof(intptr_t));
	for(intptr_t i = 0; i<numEntries; i++){
		if(fread(toDumpLong, FPSZ, 1, actualFile) != 1){
			std::cerr << "File missing some header information." << std::endl;
			fclose(actualFile); actualFile = 0;
			numEntries = -1;
			return;
		}
		uint_least64_t curOffset = canonParseInt64(toDumpLong);
		if(curOffset & MAX_MASK(uintptr_t)){
			std::cerr << "File offset too big for this program to handle." << std::endl;
			fclose(actualFile); actualFile = 0;
			numEntries = -1;
			return;
		}
		allOffsets[i] = (intptr_t)curOffset;
	}
}

FailGZReader::~FailGZReader(){
	free(name);
	free(seq);
	free(qual);
	free(decomBuffer);
	if(allOffsets){ free(allOffsets); }
	if(actualFile){ fclose(actualFile); }
}

int FailGZReader::loadInEntry(uintptr_t entryID){
	if(actualFile == 0){
		return 1;
	}
	int wasErr;
	wasErr = fseekPointer(actualFile, allOffsets[entryID], SEEK_SET);
	if(wasErr){
		std::cerr << "Problem reading failgz file." << std::endl;
		return 1;
	}
	ReadCompressedAtomSpec workingSpec;
	workingSpec.requireNullTerm = true;
	workingSpec.decomBuffer = decomBuffer;
	workingSpec.decomAlloc = decomAlloc;
	#define READ_ONE_VALUE(valName,valLen,valAlloc) \
		workingSpec.storeBuffer = valName;\
		workingSpec.storeAlloc = valAlloc;\
		wasErr = readCompressedBlockAtom(actualFile, useComp, &workingSpec);\
		valName = workingSpec.storeBuffer;\
		valAlloc = workingSpec.storeAlloc;\
		valLen = workingSpec.resultLen;\
		decomBuffer = workingSpec.decomBuffer;\
		decomAlloc = workingSpec.decomAlloc;\
		if(wasErr){ return wasErr; }
	READ_ONE_VALUE(name,nameLen,nameAlloc)
	READ_ONE_VALUE(seq,seqLen,seqAlloc)
	READ_ONE_VALUE(qual,qualLen,qualAlloc)
	return 0;
}

int findSequencesInFailGz(FailGZReader* failFile, std::vector<const char*>* findSeqs, uintptr_t findSeqInd, SequenceFindCallback* toReport){
	for(intptr_t i = 0; i<failFile->numEntries; i++){
		int readPro = failFile->loadInEntry(i);
		if(readPro){
			return 1;
		}
		//build a suffix array
		uintptr_t* sortResults = (uintptr_t*)malloc((failFile->seqLen)*sizeof(uintptr_t));
		buildSuffixArray(failFile->seq, sortResults);
		//run through all the sequences, and search
		for(uintptr_t j = 0; j<findSeqs->size(); j++){
			const char* curLookFor = (*findSeqs)[j];
			//find the bounding elements
			uintptr_t lowerBound = suffixArrayLowerBound(curLookFor, failFile->seq, sortResults, failFile->seqLen);
			uintptr_t upperBound = suffixArrayUpperBound(curLookFor, failFile->seq, sortResults, failFile->seqLen);
			//run down
			for(uintptr_t k = lowerBound; k<upperBound; k++){
				toReport->foundSequence(findSeqInd + j, i, sortResults[k]);
			}
		}
		free(sortResults);
	}
	return 0;
}

SequenceFindCallback::~SequenceFindCallback(){}
