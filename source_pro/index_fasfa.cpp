#include "protengine_index_fasfa.h"

#include <map>
#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "whodun_oshook.h"
#include "whodun_suffix.h"
#include "whodun_stringext.h"
#include "whodun_suffix_private.h"

/**
 * Frees the characters in a vector.
 * @param toKill The vector to kill.
 */
void freeVectorChars(std::vector<char*>* toKill){
	for(uintptr_t i = 0; i<toKill->size(); i++){
		free((*toKill)[i]);
	}
}

/**
 * This will build a combo file in memory.
 * @param allSrc THe starting files.
 * @param targetName THe name of the file to produce.
 * @param useComp The compression method to use.
 * @return Whether there was an exception.
 */
int buildInMemoryComboFile(std::vector<FailGZReader*>* allSrc, const char* targetName, CompressionMethod* useComp){
	char toDumpLong[FPSZ];
	//load all the things
	uintptr_t totSize = 0;
	std::vector<char*> allRead;
	for(uintptr_t i = 0; i<allSrc->size(); i++){
		FailGZReader* curFG = (*allSrc)[i];
		for(intptr_t j = 0; j<curFG->numEntries; j++){
			if(curFG->loadInEntry(j)){
				freeVectorChars(&allRead);
				return 1;
			}
			allRead.push_back(strdup(curFG->seq));
			totSize += curFG->seqLen;
		}
	}
	const char** onData = (const char**)malloc(allRead.size()*sizeof(const char*));
	for(uintptr_t i = 0; i<allRead.size(); i++){
		onData[i] = allRead[i];
	}
	uintptr_t* strIStore = (uintptr_t*)malloc(totSize*sizeof(uintptr_t));
	uintptr_t* sortStore = (uintptr_t*)malloc(totSize*sizeof(uintptr_t));
	buildMultistringSuffixArray(allRead.size(), onData, strIStore, sortStore);
	//dump out to file
	FILE* tgtDump = fopen(targetName, "wb");
	if(tgtDump == 0){
		std::cerr << "Problem building combo." << std::endl;
		freeVectorChars(&allRead); free(onData);
		free(strIStore); free(sortStore);
		return 1;
	}
	//write the number of strings and make space to put the locations
	uintptr_t numBlock = (totSize / FASFA_BLOCK_SIZE) + ((totSize % FASFA_BLOCK_SIZE) ? 1 : 0);
	canonPrepareInt64(numBlock, toDumpLong);
		fwrite(toDumpLong, 1, FPSZ, tgtDump);
	canonPrepareInt64(0, toDumpLong);
	for(uintptr_t i = 0; i<numBlock; i++){
		fwrite(toDumpLong, 1, FPSZ, tgtDump);
	}
	//dump out piece by piece
	char* dumpDecom = (char*)malloc(FASFA_BLOCK_SIZE*2*FPSZ);
	std::vector<intptr_t> blockLocs;
	for(uintptr_t i = 0; i<numBlock; i++){
		intptr_t blockLoc = ftellPointer(tgtDump);
			blockLocs.push_back(blockLoc);
		uintptr_t startI = i*FASFA_BLOCK_SIZE;
		uintptr_t endI = startI + FASFA_BLOCK_SIZE;
			if(endI > totSize){ endI = totSize; }
		//copy to the decom buffer
		char* curFoc = dumpDecom;
		for(uintptr_t j = startI; j<endI; j++){
			canonPrepareInt64(strIStore[j], curFoc);
			canonPrepareInt64(sortStore[j], curFoc + FPSZ);
			curFoc += (2*FPSZ);
		}
		//compress and write
		uintptr_t* compdLenp = useComp->compressData(2*FPSZ*(endI-startI), dumpDecom);
		if(!compdLenp){
			std::cerr << "Problem writing " << targetName << std::endl;
			freeVectorChars(&allRead); free(onData);
			free(strIStore); free(sortStore);
			fclose(tgtDump); killFile(targetName);
			return 1;
		}
		else{
			uintptr_t compdLen = *compdLenp;
			char* compdBuff = (char*)(compdLenp + 1);
			canonPrepareInt64(compdLen, toDumpLong);
			fwrite(toDumpLong, 1, FPSZ, tgtDump);
			fwrite(compdBuff, 1, compdLen, tgtDump);
			free(compdLenp);
		}
	}
	//dump the locations
	fseekPointer(tgtDump, FPSZ, SEEK_SET);
	for(uintptr_t i = 0; i<numBlock; i++){
		canonPrepareInt64(blockLocs[i], toDumpLong);
		fwrite(toDumpLong, 1, FPSZ, tgtDump);
	}
	fclose(tgtDump);
	//clean up
	freeVectorChars(&allRead); free(onData);
	free(strIStore); free(sortStore);
	return 0;
}

/**
 * This will build the initial structure for the combo file.
 * @param allSrc All the source files for the combo.
 * @param targetName The file to build the initial structure in.
 * @param useComp The compression method to use.
 * @param strLens The place to put the lengths of all used strings.
 * @param strFSIndMap Index from file/string pair to index in strLens.
 * @return Whether there was a problem.
 */
int buildInitialComboFile(std::vector<FailGZReader*>* allSrc, const char* targetName, CompressionMethod* useComp, std::vector<uintptr_t>* strLens, std::map<std::pair<uintptr_t,uintptr_t>,uintptr_t>* strFSIndMap){
	char toDumpLong[4*FPSZ];
	uintptr_t numFile = allSrc->size();
	void* buildSort = useComp->comfopen(targetName, "wb");
	if(buildSort == 0){
		std::cerr << "Problem building combo." << std::endl;
		return 1;
	}
	for(uintptr_t i = 0; i<numFile; i++){
		FailGZReader* curRd = (*allSrc)[i];
		for(intptr_t j = 0; j<curRd->numEntries; j++){
			if(curRd->loadInEntry(j)){ useComp->comfclose(buildSort); return 1; }
			uintptr_t curSI = strLens->size();
			std::pair<uintptr_t,uintptr_t> curFSInd(i,j);
			(*strFSIndMap)[curFSInd] = curSI;
			strLens->push_back(curRd->seqLen);
			for(uintptr_t k = 0; k<curRd->seqLen; k++){
				canonPrepareInt64(curSI, toDumpLong);
				canonPrepareInt64(k, toDumpLong + FPSZ);
				canonPrepareInt64(0x00FF&(curRd->seq[k]), toDumpLong + 2*FPSZ);
				canonPrepareInt64(0x00FF&( ((k+1)>=(curRd->seqLen))?0:curRd->seq[k+1] ), toDumpLong + 3*FPSZ);
				useComp->comfwrite(toDumpLong, 1, 4*FPSZ, buildSort);
			}
		}
	}
	useComp->comfclose(buildSort);
	return 0;
}

#define COMBO_ENTRY_SIZE 4

/**
 * This will build new ranks post-sort, and build the initial index structure.
 * @param sortEndTgt The place to put the new-rank data (and the initial index structure).
 * @param sortStartTgt The place the old-rank data lives.
 * @param strLens The lengths of each string under consideration.
 * @param useComp The compression method to use.
 * @return Whether there was a problem.
 */
int buildComboNewRanksAndInitialIndex(const char* sortEndTgt, const char* sortStartTgt, std::vector<uintptr_t>* strLens, CompressionMethod* useComp){
	void* lastRes = useComp->comfopen(sortEndTgt, "rb");
	void* nextRun = useComp->comfopen(sortStartTgt, "wb");
	if((lastRes == 0) || (nextRun == 0)){
		std::cerr << "Problem building combo." << std::endl;
		if(lastRes){useComp->comfclose(lastRes);}
		if(nextRun){useComp->comfclose(nextRun);}
		return 1;
	}
	bool isFirst = true;
	intptr_t nextRank = 1;
	intptr_t prevRank;
	intptr_t prevComp;
	char sinBuffer[FPSZ*COMBO_ENTRY_SIZE];
	intptr_t curStrInd = 0;
	intptr_t totStrInd = 0;
	for(uintptr_t q = 0; q<strLens->size(); q++){
		totStrInd += (*strLens)[q];
	}
	intptr_t lastLoc = useComp->comftell(lastRes);
	uintptr_t numRead = useComp->comfread(sinBuffer, FPSZ*COMBO_ENTRY_SIZE, 1, lastRes);
	while(numRead != 0){
		intptr_t lastRank = canonParseInt64(sinBuffer+FPSZ*2);
		intptr_t lastComp = canonParseInt64(sinBuffer+FPSZ*3);
		//let the user know what's going on
		if(!(curStrInd & 0x00FFFFFF)){
			std::cout << "SA rerank " << curStrInd << "/" << totStrInd << std::endl;
		}
		curStrInd++;
		//update rank
		if(isFirst){
			isFirst = false;
			prevRank = lastRank;
			prevComp = lastComp;
		}
		if((lastRank!=prevRank) || (lastComp != prevComp)){
			nextRank++;
			prevRank = lastRank;
			prevComp = lastComp;
		}
		canonPrepareInt64(nextRank, sinBuffer+FPSZ*2);
		//note the file location (index calculation will want it)
		canonPrepareInt64(lastLoc, sinBuffer+FPSZ*3);
		//write
		useComp->comfwrite(sinBuffer, 1, FPSZ*COMBO_ENTRY_SIZE, nextRun);
		//and read the next thing
		lastLoc = useComp->comftell(lastRes);
		numRead = useComp->comfread(sinBuffer, FPSZ*COMBO_ENTRY_SIZE, 1, lastRes);
	}
	useComp->comfclose(lastRes); useComp->comfclose(nextRun);
	std::cout << "End Rank " << nextRank << " / " << totStrInd << std::endl;
	return 0;
}

/**
 * This will split ranks and locations, and manage the lookahead for the next rank.
 * @param linkUpTgt The starting file to split.
 * @param toRankTgt The place to put the ranks.
 * @param toLocationTgt The place to put the locations.
 * @param strLens The lengths of each string under consideration.
 * @param forLen The next spread: twice the lookahead distance.
 * @param useComp The compression method to use.
 * @return whether there was a problem.
 */
int buildComboSplitIndex(char* linkUpTgt, char* toRankTgt, char* toLocationTgt, std::vector<uintptr_t>* strLens, uintptr_t forLen, CompressionMethod* useComp){
	char toDumpLong[FPSZ];
	void* lastRes = useComp->comfopen(linkUpTgt, "rb");
	void* rankFil = useComp->comfopen(toRankTgt, "wb");
	void* locFil = useComp->comfopen(toLocationTgt, "wb");
	if((lastRes == 0) || (rankFil == 0) || (locFil == 0)){
		std::cerr << "Problem building combo." << std::endl;
		if(lastRes){useComp->comfclose(lastRes);}
		if(rankFil){useComp->comfclose(rankFil);}
		if(locFil){useComp->comfclose(locFil);}
		return 1;
	}
	uintptr_t skipLen = forLen >> 1;
	intptr_t curStrInd = -1;
	bool nextReadNew = true;
	uintptr_t curReadInd = 0;
	char sinBuffer[FPSZ*COMBO_ENTRY_SIZE];
	uintptr_t numRead = useComp->comfread(sinBuffer, FPSZ*COMBO_ENTRY_SIZE, 1, lastRes);
	intptr_t numWriteLoc = 0;
	intptr_t numWriteRank = 0;
	while(numRead != 0){
		if(nextReadNew){
			nextReadNew = false;
			curStrInd++;
			curReadInd = 0;
		}
		//dump out the location
		intptr_t lastLoc = canonParseInt64(sinBuffer+FPSZ*3);
		canonPrepareInt64(lastLoc, toDumpLong);
		useComp->comfwrite(toDumpLong, FPSZ, 1, locFil);
		numWriteLoc++;
		//dump out the rank after the first skipLen
		intptr_t lastRank = canonParseInt64(sinBuffer+FPSZ*2);
		if(curReadInd >= skipLen){
			canonPrepareInt64(lastRank, toDumpLong);
			useComp->comfwrite(toDumpLong, FPSZ, 1, rankFil);
			numWriteRank++;
		}
		//update state
		curReadInd++;
		if(curReadInd >= (*strLens)[curStrInd]){
			nextReadNew = true;
			//dump out skipLen zeros
			canonPrepareInt64(0, toDumpLong);
			for(intptr_t i = numWriteRank; i<numWriteLoc; i++){
				useComp->comfwrite(toDumpLong, FPSZ, 1, rankFil);
			}
			numWriteLoc = 0;
			numWriteRank = 0;
		}
		//and read the next thing
		numRead = useComp->comfread(sinBuffer, FPSZ*COMBO_ENTRY_SIZE, 1, lastRes);
	}
	useComp->comfclose(lastRes); useComp->comfclose(locFil); useComp->comfclose(rankFil);
	return 0;
}

/**
 * This will merge a rank and location file.
 * @param toRankTgt The rank file.
 * @param toLocationTgt The location file.
 * @param dumpTgt The merge file.
 * @param useComp The compression method to use.
 * @return Whether there was a problem.
 */
int buildComboMergeRankLoc(char* toRankTgt, char* toLocationTgt, char* dumpTgt, CompressionMethod* useComp){
	void* rankFil = useComp->comfopen(toRankTgt, "rb");
	void* locFil = useComp->comfopen(toLocationTgt, "rb");
	void* dumpFile = useComp->comfopen(dumpTgt, "wb");
	if((rankFil == 0) || (locFil == 0) || (dumpFile == 0)){
		std::cerr << "Problem building combo." << std::endl;
		if(rankFil){useComp->comfclose(rankFil);}
		if(locFil){useComp->comfclose(locFil);}
		if(dumpFile){useComp->comfclose(dumpFile);}
		return 1;
	}
	char ranLocRW[FPSZ*2];
	uintptr_t numRead = useComp->comfread(ranLocRW, FPSZ, 1, rankFil);
	while(numRead != 0){
		numRead = useComp->comfread(ranLocRW+FPSZ, FPSZ, 1, locFil);
		if(numRead == 0){
			std::cerr << "Problem building combo." << std::endl;
			useComp->comfclose(rankFil); useComp->comfclose(locFil); useComp->comfclose(dumpFile);
			return 1;
		}
		useComp->comfwrite(ranLocRW, 2*FPSZ, 1, dumpFile);
		numRead = useComp->comfread(ranLocRW, FPSZ, 1, rankFil);
	}
	useComp->comfclose(rankFil); useComp->comfclose(locFil); useComp->comfclose(dumpFile);
	return 0;
}

/**
 * This will update the next ranks of a combo struct.
 * @param editTgt THe thing to edit.
 * @param rankLocTgt The file holding the ranks.
 * @param dumpTgt The palce to put the merged results.
 * @param strLens The lengths of each string under consideration.
 * @param useComp The compression method to use.
 * @return Whether there was a problem.
 */
int buildComboUpdateNextRanks(char* editTgt, char* rankLocTgt, char* dumpTgt, std::vector<uintptr_t>* strLens, CompressionMethod* useComp){
	intptr_t totStrInd = 0;
	for(uintptr_t q = 0; q<strLens->size(); q++){
		totStrInd += (*strLens)[q];
	}
	//open the files
	void* editFile = useComp->comfopen(editTgt, "rb");
	void* locFile = useComp->comfopen(rankLocTgt, "rb");
	void* dumpFile = useComp->comfopen(dumpTgt, "wb");
	if((editFile == 0) || (locFile == 0) || (dumpFile == 0)){
		std::cerr << "Problem building combo." << std::endl;
		if(editFile){useComp->comfclose(editFile);}
		if(locFile){useComp->comfclose(locFile);}
		if(dumpFile){useComp->comfclose(dumpFile);}
		return 1;
	}
	//run through reading
	char sinBuffer[FPSZ*COMBO_ENTRY_SIZE];
	char rankLocBuffer[2*FPSZ];
	uintptr_t numRead = useComp->comfread(sinBuffer, FPSZ*COMBO_ENTRY_SIZE, 1, editFile);
	while(numRead != 0){
		numRead = useComp->comfread(rankLocBuffer, 2*FPSZ, 1, locFile);
		if(numRead == 0){
			std::cerr << "Problem building combo." << std::endl;
			useComp->comfclose(editFile); useComp->comfclose(locFile); useComp->comfclose(dumpFile);
			return 1;
		}
		memcpy(sinBuffer + 3*FPSZ, rankLocBuffer, FPSZ);
		useComp->comfwrite(sinBuffer, FPSZ*COMBO_ENTRY_SIZE, 1, dumpFile);
		numRead = useComp->comfread(sinBuffer, FPSZ*COMBO_ENTRY_SIZE, 1, editFile);
	}
	useComp->comfclose(editFile); useComp->comfclose(locFile); useComp->comfclose(dumpFile);
	return 0;
}

/**
 * This will dump the results into a block-compress format.
 * @param fromFlatGz The flat file (with ranks).
 * @param targetDump The place to put the results.
 * @param totSize The total size of all strings.
 * @param useComp The compression method to use.
 * @return Whether there was a problem.
 */
int buildComboDumpResults(const char* fromFlatGz, const char* targetDump, intptr_t totSize, CompressionMethod* useComp){
	char toDumpLong[FPSZ];
	void* readFrom = useComp->comfopen(fromFlatGz, "rb");
	FILE* tgtDump = fopen(targetDump, "wb");
	if((readFrom == 0) || (tgtDump == 0)){
		std::cerr << "Problem building combo." << std::endl;
		if(readFrom){useComp->comfclose(readFrom);}
		if(tgtDump){fclose(tgtDump);}
		return 1;
	}
	//make a place for the terminal link
	uintptr_t numBlock = (totSize / FASFA_BLOCK_SIZE) + ((totSize % FASFA_BLOCK_SIZE) ? 1 : 0);
	canonPrepareInt64(numBlock, toDumpLong);
		fwrite(toDumpLong, 1, FPSZ, tgtDump);
	canonPrepareInt64(0, toDumpLong);
	for(uintptr_t i = 0; i<numBlock; i++){
		fwrite(toDumpLong, 1, FPSZ, tgtDump);
	}
	uintptr_t dbgTotNumWrite = FPSZ * (numBlock + 1);
	//prepare buffers
	char* rawData = (char*)malloc(FASFA_BLOCK_SIZE*4*FPSZ);
	char* decompData = (char*)malloc(FASFA_BLOCK_SIZE*2*FPSZ);
	std::vector<uintptr_t> allFileLocs;
	//start reading and dumping
	uintptr_t numRead = useComp->comfread(rawData, 4*FPSZ, FASFA_BLOCK_SIZE, readFrom);
	while(numRead){
		//prepare the decompressed data
		char* curDecom = decompData;
		char* curRaw = rawData;
		for(uintptr_t i = 0; i<numRead; i++){
			memcpy(curDecom, curRaw, 2*FPSZ);
			curDecom = curDecom + (2*FPSZ);
			curRaw = curRaw + (4*FPSZ);
		}
		//get the current location
		allFileLocs.push_back(dbgTotNumWrite);
		//compress and write
		uintptr_t* compdLenp = useComp->compressData(2*FPSZ*numRead, decompData);
		if(!compdLenp){
			std::cerr << "Problem writing " << targetDump << std::endl;
			useComp->comfclose(readFrom); fclose(tgtDump);
			killFile(targetDump);
			free(rawData); free(decompData);
			return 1;
		}
		else{
			uintptr_t compdLen = *compdLenp;
			dbgTotNumWrite += compdLen + FPSZ;
			char* compdBuff = (char*)(compdLenp + 1);
			canonPrepareInt64(compdLen, toDumpLong);
			fwrite(toDumpLong, 1, FPSZ, tgtDump);
			fwrite(compdBuff, 1, compdLen, tgtDump);
			free(compdLenp);
		}
		//read the next
		numRead = useComp->comfread(rawData, 4*FPSZ, FASFA_BLOCK_SIZE, readFrom);
	}
	useComp->comfclose(readFrom);
	//dump out the locations
	fseekPointer(tgtDump, FPSZ, SEEK_SET);
	for(uintptr_t i = 0; i<numBlock; i++){
		canonPrepareInt64(allFileLocs[i], toDumpLong);
		fwrite(toDumpLong, 1, FPSZ, tgtDump);
	}
	fclose(tgtDump);
	//and note the location
	free(rawData); free(decompData);
	return 0;
}

#define TEMP_NAME "combo_tmp"

int buildComboFileFromFailgz(std::vector<FailGZReader*>* allSrc, const char* targetName, const char* temporaryFolder, int numThread, uintptr_t maxByteLoad, CompressionMethod* useComp){
	//run through the files, figure out string lengths
	bool canFitMem = true;
	uintptr_t totSize = 0;
	uintptr_t curFSInd = 0;
	for(uintptr_t i = 0; i<allSrc->size(); i++){
		FailGZReader* curFG = (*allSrc)[i];
		for(intptr_t j = 0; j<curFG->numEntries; j++){
			if(curFG->loadInEntry(j)){
				return 1;
			}
			totSize += curFG->seqLen;
			if(totSize > maxByteLoad){
				canFitMem = false;
			}
			curFSInd++;
		}
	}
	//if it can fit in memory, do it in memory
	//otherwise, do the out of memory thing
	if(canFitMem){
		return buildInMemoryComboFile(allSrc, targetName, useComp);
	}
	else{
		MultiStringSuffixRankSortOption comboSorts;
		MultiStringSuffixIndexSortOption indexSorts;
		MultiStringSuffixRLPairSortOption rankSorts;
		comboSorts.numThread = numThread;	indexSorts.numThread = numThread;	rankSorts.numThread = numThread;
		comboSorts.maxLoad = maxByteLoad;	indexSorts.maxLoad = maxByteLoad;	rankSorts.maxLoad = maxByteLoad;
		comboSorts.inCom = useComp;			indexSorts.inCom = useComp; 		rankSorts.inCom = useComp;
		comboSorts.workCom = useComp;		indexSorts.workCom = useComp; 		rankSorts.workCom = useComp;
		comboSorts.outCom = useComp;		indexSorts.outCom = useComp; 		rankSorts.outCom = useComp;
		//make four temporary files
		uintptr_t foldLen = strlen(temporaryFolder);
		uintptr_t sepLen = strlen(pathElementSep);
		uintptr_t tmpLen = strlen(TEMP_NAME);
		uintptr_t numLen = 4*sizeof(uintptr_t);
		uintptr_t totLen = foldLen + sepLen + tmpLen + numLen;
		char* sortStartTgt = (char*)malloc(totLen); sprintf(sortStartTgt, "%s%s%s%x", temporaryFolder, pathElementSep, TEMP_NAME, 1);
		char* sortEndTgt = (char*)malloc(totLen); sprintf(sortEndTgt, "%s%s%s%x", temporaryFolder, pathElementSep, TEMP_NAME, 2);
		char* linkUpTgt = (char*)malloc(totLen); sprintf(linkUpTgt, "%s%s%s%x", temporaryFolder, pathElementSep, TEMP_NAME, 3);
		char* sortUpdateTgt = (char*)malloc(totLen); sprintf(sortUpdateTgt, "%s%s%s%x", temporaryFolder, pathElementSep, TEMP_NAME, 4);
		char* tmpRankFile = (char*)malloc(totLen); sprintf(tmpRankFile, "%s%s%s%x", temporaryFolder, pathElementSep, TEMP_NAME, 5);
		char* tmpLocFile = (char*)malloc(totLen); sprintf(tmpLocFile, "%s%s%s%x", temporaryFolder, pathElementSep, TEMP_NAME, 6);
		#define CHECK_RESPOND_ERROR(errVar) \
			if(errVar){\
				std::cerr << "Problem building combo." << std::endl;\
				KILL_TEMPS\
				return 1;\
			}
		#define KILL_TEMPS killFile(sortStartTgt); free(sortStartTgt); \
			killFile(sortEndTgt); free(sortEndTgt); \
			killFile(linkUpTgt); free(linkUpTgt); \
			killFile(sortUpdateTgt); free(sortUpdateTgt); \
			killFile(tmpRankFile); free(tmpRankFile); \
			killFile(tmpLocFile); free(tmpLocFile);
		//build the initial file
		std::vector<uintptr_t> strLens;
		std::map< std::pair<uintptr_t,uintptr_t>,uintptr_t > strFSIndMap;
		int buildErr = buildInitialComboFile(allSrc, sortStartTgt, useComp, &strLens, &strFSIndMap);
		CHECK_RESPOND_ERROR(buildErr)
		//note the maximum length
		uintptr_t totSize = 0;
		uintptr_t maxStrLen = 0;
		for(uintptr_t i = 0; i<strLens.size(); i++){
			totSize += strLens[i];
			if(strLens[i]>maxStrLen){
				maxStrLen = strLens[i];
			}
		}
		//sort the current items
		int sortProb = outOfMemoryMultithreadMergesort(sortStartTgt, temporaryFolder, sortEndTgt, &comboSorts);
		CHECK_RESPOND_ERROR(sortProb)
		//sort on further
		for(uintptr_t forLen = 4; forLen < 2*maxStrLen; forLen = forLen << 1){
			std::cout << "SA " << forLen << "/" << (2*maxStrLen) << " bytes" << std::endl;
			//figure out where everything wound up
			//generate new current ranks (and fill in the index file)
			std::cout << "New ranks" << std::endl;
			int rankErr = buildComboNewRanksAndInitialIndex(sortEndTgt, sortStartTgt, &strLens, useComp);
			CHECK_RESPOND_ERROR(rankErr)
			//sort the index file on fsi
			sortProb = outOfMemoryMultithreadMergesort(sortStartTgt, temporaryFolder, linkUpTgt, &indexSorts);
			CHECK_RESPOND_ERROR(sortProb)
			//split into location and r files
			std::cout << "Split rankloc" << std::endl;
			int splitErr = buildComboSplitIndex(linkUpTgt, tmpRankFile, tmpLocFile, &strLens, forLen, useComp);
			CHECK_RESPOND_ERROR(splitErr)
			//merge the two (into sortOnEnd)
			std::cout << "Merge rankloc" << std::endl;
			int mergeErr = buildComboMergeRankLoc(tmpRankFile, tmpLocFile, sortEndTgt, useComp);
			CHECK_RESPOND_ERROR(mergeErr)
			//sort back into index (into linkUpTgt)
			std::cout << "Sort rankloc" << std::endl;
			sortProb = outOfMemoryMultithreadMergesort(sortEndTgt, temporaryFolder, linkUpTgt, &rankSorts);
			CHECK_RESPOND_ERROR(sortProb)
			//figure out the next ranks
			std::cout << "Next ranks" << std::endl;
			int nextErr = buildComboUpdateNextRanks(sortStartTgt, linkUpTgt, sortUpdateTgt, &strLens, useComp);
			CHECK_RESPOND_ERROR(nextErr)
			//sort
			std::cout << "Sorting on rank/nextrank" << std::endl;
			sortProb = outOfMemoryMultithreadMergesort(sortUpdateTgt, temporaryFolder, sortEndTgt, &comboSorts);
			CHECK_RESPOND_ERROR(sortProb)
		}
		//dump to target
		std::cout << "Dumping to target" << std::endl;
		int dumpErr = buildComboDumpResults(sortEndTgt, targetName, totSize, useComp);
		CHECK_RESPOND_ERROR(dumpErr)
		//kill temps
		KILL_TEMPS
		std::cout << "Dumped" << std::endl;
		return 0;
	}
}

FasfaReader::FasfaReader(const char* fasfaFileName, CompressionMethod* compMeth){
	char toDumpLong[FPSZ];
	//set up basic variables
	useComp = compMeth;
	decomAlloc = 64;
	decomBuffer = (char*)malloc(decomAlloc);
	currentAlloc = 64;
	currentSequence = (uintptr_t*)malloc(currentAlloc*sizeof(uintptr_t));
	filptrBuffer = (char*)malloc(currentAlloc*FPSZ);
	seqOffLowInd = -1;
	seqOffHigInd = -1;
	//open the file
	actualFile = fopen(fasfaFileName, "rb");
	if(actualFile == 0){
		numSequences = -1;
		return;
	}
	//load in the number of sequences
	if(fread(toDumpLong, FPSZ, 1, actualFile) != 1){
		fclose(actualFile); actualFile = 0;
		numSequences = -1;
		return;
	}
	uint_least64_t numEnts = canonParseInt64(toDumpLong);
	if(numEnts & MAX_MASK(int)){
		std::cerr << "Too many entries in file for this to handle." << std::endl;
		fclose(actualFile); actualFile = 0;
		numSequences = -1;
		return;
	}
	numSequences = (int)numEnts;
}


FasfaReader::~FasfaReader(){
	free(currentSequence);
	free(filptrBuffer);
	free(decomBuffer);
	if(actualFile){ fclose(actualFile); }
}

int FasfaReader::loadInSequence(uintptr_t seqID){
	//figure its offset
	//load in offset table, if necessary
	if(((intptr_t)seqID < seqOffLowInd) || ((intptr_t)seqID >= seqOffHigInd)){
		//figure out what to load
		seqOffLowInd = seqID - (FASFA_BLOCK_SIZE/2);
		if(seqOffLowInd < 0){
			seqOffLowInd = 0;
		}
		seqOffHigInd = seqOffLowInd + FASFA_BLOCK_SIZE;
		if(seqOffHigInd > numSequences){
			seqOffHigInd = numSequences;
		}
		//allocate space in the filptrBuffer
		uintptr_t numLoad = seqOffHigInd - seqOffLowInd;
		if(currentAlloc < numLoad){
			free(currentSequence);
			free(filptrBuffer);
			currentAlloc = numLoad;
			currentSequence = (uintptr_t*)malloc(currentAlloc*sizeof(uintptr_t));
			filptrBuffer = (char*)malloc(currentAlloc*FPSZ);
		}
		//seek to start
		int seekPro = fseekPointer(actualFile, FPSZ*((intptr_t)seqOffLowInd)+FPSZ, SEEK_SET);
		if(seekPro){ return 1; }
		uintptr_t numRead = fread(filptrBuffer, FPSZ, numLoad, actualFile);
		if(numRead != numLoad){ return 1; }
		//convert to long int
		for(uintptr_t i = 0; i < numLoad; i++){
			sequenceOffsets[i] = canonParseInt64(filptrBuffer + i*FPSZ);
		}
	}
	//load in the data
	int seekPro = fseekPointer(actualFile, sequenceOffsets[seqID - seqOffLowInd], SEEK_SET);
	if(seekPro){ return 1; }
	ReadCompressedAtomSpec workingSpec;
	workingSpec.requireNullTerm = false;
	workingSpec.decomBuffer = decomBuffer;
	workingSpec.decomAlloc = decomAlloc;
	workingSpec.storeBuffer = filptrBuffer;
	workingSpec.storeAlloc = FPSZ*currentAlloc;
	int wasErr = readCompressedBlockAtom(actualFile, useComp, &workingSpec);
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
	if(numInCurrent % 2){
		std::cerr << "Expected a pair, read an odd number of values." << std::endl;
		return 1;
	}
	numInCurrent = numInCurrent / 2;
	return 0;
}

/**Bulk information for finding a single sequence.*/
typedef struct{
	/**The files containing the sequences.*/
	std::vector<FailGZReader*>* failFile;
	/**The combo data.*/
	FasfaReader* fassaFile;
	/**The sequence to look for.*/
	const char* findSeq;
	/**The index of said sequence.*/
	uintptr_t findSeqInd;
	/**The place to write results.*/
	SequenceFindCallback* toReport;
	/**Linear string index to file index.*/
	std::vector<uintptr_t>* sindToFF;
	/**Linear string index to in-file string index.*/
	std::vector<uintptr_t>* sindToFI;
	/**On call, the lowest sequence that needs to be searched. On return, the lowest sequence that was used.*/
	uintptr_t lowSeqInd;
	/**On call, the highest sequence that needs to be searched. On return, the highest sequence that was used. Exclusive for both.*/
	uintptr_t higSeqInd;
} FindSingleInformation;

/**
 * This will search for a single sequence in a fasfa.
 * @param infoArg The arguments to the function.
 * @return whether there was a problem.
 */
int findSingleSequenceInFasfa(FindSingleInformation* infoArg){
	//std::cout << "Looking for " << infoArg->findSeq << std::endl;
	std::vector<FailGZReader*>* failFile = infoArg->failFile;
	FasfaReader* fassaFile = infoArg->fassaFile;
	std::vector<uintptr_t>* sindToFF = infoArg->sindToFF;
	std::vector<uintptr_t>* sindToFI = infoArg->sindToFI;
	SequenceFindCallback* toReport = infoArg->toReport;
	uintptr_t findSeqInd = infoArg->findSeqInd;
	const char* findSeq = infoArg->findSeq;
	int findLen = strlen(findSeq);
	//figure out which block it starts at (last block where str > block[-1])+1
	uintptr_t lowBlock = infoArg->lowSeqInd;
	uintptr_t higBlock = infoArg->higSeqInd;
	while((higBlock - lowBlock) > 0){
		uintptr_t midBlock = lowBlock + ((higBlock - lowBlock)>>1);
		//load in the block
		if(fassaFile->loadInSequence(midBlock)){ return 1; }
		//get the last string info
		uintptr_t strLin = fassaFile->currentSequence[2*fassaFile->numInCurrent-2];
		uintptr_t subInd = fassaFile->currentSequence[2*fassaFile->numInCurrent-1];
		uintptr_t fileInd = (*sindToFF)[strLin];
		uintptr_t strInd = (*sindToFI)[strLin];
		FailGZReader* curFGZ = (*failFile)[fileInd];
		if(curFGZ->loadInEntry(strInd)){ return 1; }
		const char* lastStr = curFGZ->seq + subInd;
		//compare and move
		int compVal = strncmp(lastStr, findSeq, findLen);
		if(compVal < 0){
			lowBlock = midBlock + 1;
		}
		else{
			higBlock = midBlock;
		}
	}
	uintptr_t startBlock = lowBlock;
	//figure out which block it ends at (first block where str < block[0])-1
	lowBlock = infoArg->lowSeqInd;
	higBlock = infoArg->higSeqInd;
	while((higBlock - lowBlock) > 0){
		uintptr_t midBlock = lowBlock + ((higBlock - lowBlock)>>1);
		//load in the block
		if(fassaFile->loadInSequence(midBlock)){ return 1; }
		//get the last string info
		uintptr_t strLin = fassaFile->currentSequence[0];
		uintptr_t subInd = fassaFile->currentSequence[1];
		uintptr_t fileInd = (*sindToFF)[strLin];
		uintptr_t strInd = (*sindToFI)[strLin];
		FailGZReader* curFGZ = (*failFile)[fileInd];
		if(curFGZ->loadInEntry(strInd)){ return 1; }
		const char* lastStr = curFGZ->seq + subInd;
		//compare and move
		int compVal = strncmp(lastStr, findSeq, findLen);
		if(compVal <= 0){
			lowBlock = midBlock + 1;
		}
		else{
			higBlock = midBlock;
		}
	}
	uintptr_t endBlock = lowBlock;
	//std::cout << "\tblocks[" << startBlock << ":" << endBlock << "]" << std::endl;
	//run through all of them
	for(uintptr_t bi = startBlock; bi<endBlock; bi++){
		if(fassaFile->loadInSequence(bi)){ return 1; }
		//find the bounding elements
		uintptr_t lowInd = 0;
		uintptr_t higInd = fassaFile->numInCurrent;
		while(higInd-lowInd > 0){
			uintptr_t midInd = (higInd + lowInd) >> 1;
			uintptr_t strLin = fassaFile->currentSequence[2*midInd];
			uintptr_t subInd = fassaFile->currentSequence[2*midInd+1];
			uintptr_t fileInd = (*sindToFF)[strLin];
			uintptr_t strInd = (*sindToFI)[strLin];
			FailGZReader* curFGZ = (*failFile)[fileInd];
			if(curFGZ->loadInEntry(strInd)){ return 1; }
			const char* lastStr = curFGZ->seq + subInd;
			int compVal = strncmp(lastStr, findSeq, findLen);
			if(compVal < 0){
				lowInd = midInd + 1;
			}
			else{
				higInd = midInd;
			}
		}
		uintptr_t lowerBound = lowInd;
		lowInd = 0;
		higInd = fassaFile->numInCurrent;
		while(higInd-lowInd > 0){
			uintptr_t midInd = (higInd + lowInd) >> 1;
			uintptr_t strLin = fassaFile->currentSequence[2*midInd];
			uintptr_t subInd = fassaFile->currentSequence[2*midInd+1];
			uintptr_t fileInd = (*sindToFF)[strLin];
			uintptr_t strInd = (*sindToFI)[strLin];
			FailGZReader* curFGZ = (*failFile)[fileInd];
			if(curFGZ->loadInEntry(strInd)){ return 1; }
			const char* lastStr = curFGZ->seq + subInd;
			int compVal = strncmp(lastStr, findSeq, findLen);
			if(compVal <= 0){
				lowInd = midInd + 1;
			}
			else{
				higInd = midInd;
			}
		}
		uintptr_t upperBound = lowInd;
		//std::cout << "\tblock[" << bi << "][" << lowerBound << ":" << upperBound << "]" << std::endl;
		//run down
		for(uintptr_t i = lowerBound; i<upperBound; i++){
			uintptr_t strLin = fassaFile->currentSequence[2*i];
			uintptr_t subInd = fassaFile->currentSequence[2*i+1];
			int readErr = toReport->foundSequence(findSeqInd, strLin, subInd);
			if(readErr){ return 1; }
		}
	}
	return 0;
}

int findSequencesInFasfa(std::vector<FailGZReader*>* failFile, FasfaReader* fassaFile, std::vector<const char*>* findSeqs, uintptr_t findSeqInd, SequenceFindCallback* toReport){
	//figure the map from string index to fail file
	std::vector<uintptr_t> sindToFF;
	std::vector<uintptr_t> sindToFI;
	for(uintptr_t i = 0; i<failFile->size(); i++){
		uintptr_t numEnt = (*failFile)[i]->numEntries;
		for(uintptr_t j = 0; j<numEnt; j++){
			sindToFF.push_back(i);
			sindToFI.push_back(j);
		}
	}
	//linear for now, sort and work later
	FindSingleInformation infoArg;
	infoArg.failFile = failFile;
	infoArg.fassaFile = fassaFile;
	infoArg.toReport = toReport;
	infoArg.sindToFF = &sindToFF;
	infoArg.sindToFI = &sindToFI;
	for(uintptr_t i = 0; i<findSeqs->size(); i++){
		infoArg.findSeq = (*findSeqs)[i];
		infoArg.findSeqInd = findSeqInd + i;
		infoArg.lowSeqInd = 0;
		infoArg.higSeqInd = fassaFile->numSequences;
		if(findSingleSequenceInFasfa(&infoArg)){ return 1; }
	}
	return 0;
}
