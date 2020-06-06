
#include <map>
#include <set>
#include <string>
#include <vector>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <iostream>
#include <string.h>
#include <algorithm>

#include "whodun_stringext.h"
#include "whodun_oshook.h"
#include "whodun_cigfq.h"
#include "gpmatlan.h"

#define CLARG_INT "-inT="
#define CLARG_IND "-inD="
#define CLARG_RFN "-rfN="
#define CLARG_DIG "-dig="
#define CLARG_ORI "-ori="
#define CLARG_OUT "-out="

#define WHITESPACE " \t\r\n"

#define FLEN_S 2
#define FLEN_I 4
#define FLEN_L 8
#define ENTRY_SIZE 8

/**
 * This will search a protein data file for a protein by name.
 * @param protMajNam The major name to search for.
 * @param protMinNam The minor name to search for.
 * @param inTabF The table file.
 * @param inDatF The data file.
 * @param numP THe number of proteins in the file.
 * @param entStore The place to put the found item.
 * @return Whether the protein was found.
 */
bool findProteinDataByName(const char* protMajNam, const char* protMinNam, FILE* inTabF, FILE* inDatF, uintptr_t numP, uint64_t* entStore){
	char entryBuff[ENTRY_SIZE*FLEN_L];
	char bufferS[FLEN_S];
	int strLength;
    #define FIND_FREAD(toBuff,entSize,entCnt,fromF) \
        if(fread(toBuff,entSize,entCnt,fromF) != (size_t)entCnt){\
            std::cerr << "Problem finding " << protMajNam << "_" << protMinNam << std::endl;\
            return false;\
        }
	//space for reading strings
		int allocLen = 64;
		char* strAlloc = (char*)malloc(allocLen); 
		#define LOAD_STRING \
            FIND_FREAD(bufferS, FLEN_S, 1, inDatF)\
			strLength = ((0x00FF&bufferS[0])<<8) + (0x00FF&bufferS[1]);\
			if((strLength+1) > allocLen){ free(strAlloc); allocLen = strLength + 1; strAlloc = (char*)malloc(allocLen); }\
			FIND_FREAD(strAlloc, 1, strLength, inDatF)\
			strAlloc[strLength] = 0;
	//binary search to find it
	uintptr_t lowBnd = 0;
	uintptr_t higBnd = numP;
	while((higBnd - lowBnd) > 0){
		uintptr_t midBnd = (higBnd + lowBnd) / 2;
		fseekPointer(inTabF, ENTRY_SIZE*FLEN_L*midBnd + FLEN_L, SEEK_SET);
		FIND_FREAD(entryBuff, FLEN_L, ENTRY_SIZE, inTabF);
		fseekPointer(inDatF, canonParseInt64(entryBuff), SEEK_SET);
			LOAD_STRING
			int majComp = strcmp(strAlloc, protMajNam);
		fseekPointer(inDatF, canonParseInt64(entryBuff + FLEN_L), SEEK_SET);
			LOAD_STRING
			int minComp = strcmp(strAlloc, protMinNam);
		if((majComp < 0) || ((majComp == 0) && (minComp < 0))){
			lowBnd = midBnd + 1;
		}
		else{
			higBnd = midBnd;
		}
	}
	//check that the name is right
		fseekPointer(inTabF, ENTRY_SIZE*FLEN_L*lowBnd + FLEN_L, SEEK_SET);
		FIND_FREAD(entryBuff, FLEN_L, ENTRY_SIZE, inTabF);
		fseekPointer(inDatF, canonParseInt64(entryBuff), SEEK_SET);
			LOAD_STRING
			bool majMatch = (strcmp(protMajNam, strAlloc) == 0);
		fseekPointer(inDatF, canonParseInt64(entryBuff + FLEN_L), SEEK_SET);
			LOAD_STRING
			bool minMatch = (strcmp(protMinNam, strAlloc) == 0);
		free(strAlloc);
		if(!majMatch || !minMatch){
			return false;
		}
	//put them in the return
		for(int i = 0; i<ENTRY_SIZE; i++){
			entStore[i] = canonParseInt64(entryBuff + (i*FLEN_L));
		}
		return true;
}

/**An identifier for a cigar/reference map.*/
class CigarRefIdentifier{
public:
	/**The cigar string.*/
	std::string cigar;
	/**The starting and ending locations, sorted in ascending order.*/
	std::vector< std::pair<uintptr_t,uintptr_t> > startEndLocs;
	/**The strand.*/
	std::string strand;
	/**
	 * Default constructor.
	 */
	CigarRefIdentifier(){}
	/**
	 * Copy constructor.
	 * @param toCopy The thing to copy.
	 */
	CigarRefIdentifier(const CigarRefIdentifier& toCopy){
		cigar = toCopy.cigar;
		startEndLocs = toCopy.startEndLocs;
		strand = toCopy.strand;
	}
	/**
	 * Build constructor.
	 * @param useCigar The cigar string.
	 * @param useStart The starting locations.
	 * @param useEnd The ending locations.
	 * @param useStrand The strand.
	 */
	CigarRefIdentifier(std::string& useCigar, std::vector<uintptr_t>& useStart, std::vector<uintptr_t>& useEnd, std::string& useStrand){
		cigar = useCigar;
		strand = useStrand;
		for(unsigned i = 0; i<useStart.size(); i++){
			std::pair<uintptr_t,uintptr_t> curAdd(useStart[i], useEnd[i]);
			startEndLocs.push_back(curAdd);
		}
		std::sort(startEndLocs.begin(), startEndLocs.end());
	}
};

/**The result of a cigar/reference translation.*/
class CigarRefResult{
public:
	/**The locations.*/
	std::vector<intptr_t> nextLocs;
	/**The used length.*/
	uintptr_t retLen;
	/**The number of skips at the start.*/
	uintptr_t retSkip;
	/**Whether the reference is non-codon.*/
	bool isNonCodon;
	/**Whether there is a problem with the cigar reference.*/
	bool cigRefProb;
	/**
	 * Default constructor.
	 */
	CigarRefResult(){}
	/**
	 * Copy constructor.
	 * @param toCopy The thing to copy.
	 */
	CigarRefResult(const CigarRefResult& toCopy){
		nextLocs = toCopy.nextLocs;
		retLen = toCopy.retLen;
		retSkip = toCopy.retSkip;
		isNonCodon = toCopy.isNonCodon;
		cigRefProb = toCopy.cigRefProb;
	}
};

bool compCigarRefId(const CigarRefIdentifier& op1, const CigarRefIdentifier& op2){
	//compare cigars
		if(op1.cigar < op2.cigar){
			return true;
		}
		if(op1.cigar > op2.cigar){
			return false;
		}
	//starting positions
		unsigned int minSize = op1.startEndLocs.size();
		if(op2.startEndLocs.size() < minSize){ minSize = op2.startEndLocs.size(); }
		for(unsigned int i = 0; i<minSize; i++){
			if(op1.startEndLocs[i] < op2.startEndLocs[i]){
				return true;
			}
			if(op1.startEndLocs[i] > op2.startEndLocs[i]){
				return false;
			}
		}
		if(op1.startEndLocs.size() < op2.startEndLocs.size()){
			return true;
		}
		if(op1.startEndLocs.size() > op2.startEndLocs.size()){
			return false;
		}
	//strand
		if(op1.strand < op2.strand){
			return true;
		}
		if(op1.strand > op2.strand){
			return false;
		}
	//equal
	return false;
}

template<class T1, class T2, class T3> struct triple{
	T1 first;
	T2 second;
	T3 third;
};

CigarRefResult* cigarToRefLocations(CigarRefIdentifier* toFind, std::ofstream* outF, std::string* repID, std::map<CigarRefIdentifier,CigarRefResult,bool(*)(const CigarRefIdentifier&,const CigarRefIdentifier&)>* cigMemo){
	bool isForwardStrand = toFind->strand == "+";
	//handle memoization
	if(cigMemo->find(*toFind) != cigMemo->end()){
		CigarRefResult* toRet = &((*cigMemo)[*toFind]);
		if(toRet->isNonCodon){
			*outF << "W " << *repID << " Non-codon" << std::endl;
		}
		if(toRet->cigRefProb){
			*outF << "W " << *repID << " Cigar-Reference Number Mismatch" << std::endl;
		}
		return toRet;
	}
	if(cigMemo->size() > 256){
		cigMemo->clear();
	}
	//build a map from amino acid number to start position
	bool isNonC = false;
	std::vector< triple<uintptr_t,uintptr_t,uintptr_t> > aaNtoSP;
	{
		std::vector<uintptr_t> curTri;
		for(unsigned i = 0; i<toFind->startEndLocs.size(); i++){
			std::pair<uintptr_t,uintptr_t> curSE = toFind->startEndLocs[i];
			for(unsigned j = curSE.first; j<curSE.second; j++){
				curTri.push_back(j);
			}
		}
		unsigned triCods = curTri.size() / 3;
		aaNtoSP.reserve(triCods);
		unsigned triCOver = curTri.size() % 3;
		if(triCOver){
			isNonC = true;
			if(!isForwardStrand){
				//erase the first overrun
				curTri.erase(curTri.begin(), curTri.begin() + triCOver);
			}
		}
		triple<uintptr_t,uintptr_t,uintptr_t> curAdd;
		unsigned curInd = 0;
		for(unsigned i = 0; i<triCods;  i++){
			curAdd.first = curTri[curInd];
			curAdd.second = curTri[curInd + 1];
			curAdd.third = curTri[curInd + 2];
			aaNtoSP.push_back(curAdd);
			curInd += 3;
		}
	}
	if(isNonC){
		*outF << "W " << *repID << " Non-codon" << std::endl;
	}
	//run the cigar string to get indices
	const char* cigErr;
	GPML_ByteArray cigAsBA = allocateByteArray(toFind->cigar.size());
		memcpy(cigAsBA->arrConts, toFind->cigar.c_str(), cigAsBA->arrLen);
	GPML_IntArray cigLocRet = cigarStringToReferenceLocations(cigAsBA, 0, &cigErr);
	free(cigAsBA->arrConts); free(cigAsBA);
	uintptr_t retLen = cigLocRet->arrConts[cigLocRet->arrLen - 1];
	uintptr_t retSkp = cigLocRet->arrConts[cigLocRet->arrLen - 2];
	if(retSkp > 0){
		//TODO
	}
	int clrRealLen = cigLocRet->arrLen - 2;
	//run down the locations
	int maxRef = aaNtoSP.size() - 1;
	bool cigProb = false;
	std::vector<intptr_t> nextLocs;
	for(int i = 0; i<clrRealLen; i++){
		if(cigLocRet->arrConts[i] < 0){
			nextLocs.push_back(-1);
			nextLocs.push_back(-1);
			nextLocs.push_back(-1);
		}
		else{
			if(cigLocRet->arrConts[i] > maxRef){
				cigProb = true;
				break;
			}
			if(isForwardStrand){
				triple<uintptr_t,uintptr_t,uintptr_t>& curCodL = aaNtoSP[cigLocRet->arrConts[i]];
				nextLocs.push_back(curCodL.first);
				nextLocs.push_back(curCodL.second);
				nextLocs.push_back(curCodL.third);
			}
			else{
				triple<uintptr_t,uintptr_t,uintptr_t>& curCodL = aaNtoSP[maxRef - cigLocRet->arrConts[i]];
				nextLocs.push_back(curCodL.third);
				nextLocs.push_back(curCodL.second);
				nextLocs.push_back(curCodL.first);
			}
		}
	}
	if(cigProb){
		*outF << "W " << *repID << " Cigar-Reference Number Mismatch" << std::endl;
	}
	free(cigLocRet->arrConts);
	free(cigLocRet);
	//add to memo and return
	CigarRefResult* curAddMemo = &((*cigMemo)[*toFind]);
	curAddMemo->nextLocs = nextLocs;
	curAddMemo->retLen = retLen;
	curAddMemo->retSkip = retSkp;
	curAddMemo->isNonCodon = isNonC;
	curAddMemo->cigRefProb = cigProb;
	return curAddMemo;
}

/**Parameters for a string match.*/
class StringMatchParams{
public:
	/**The seen string match info.*/
	std::vector< std::vector< std::string > > allSMatch;
	/**The names of the reference indices.*/
	std::vector<std::string> snameForI;
	/**The lengths of the search strings.*/
	std::vector<uintptr_t> searchLens;
	/**The table file.*/
	FILE* inTab;
	/**The data file.*/
	FILE* inDat;
	/**The number of proteins.*/
	uintptr_t numP;
	/**The output file.*/
	std::ofstream* resF;
	/**Cigar memoization*/
	std::map<CigarRefIdentifier,CigarRefResult,bool(*)(const CigarRefIdentifier&,const CigarRefIdentifier&)>* cigMemo;
};

#define DEF_DUMP *curFSInd << " " << majNam << " " << minNam
#define SHORT_RECON(fromBuff) (((0x00FF&(fromBuff)[0])<<8) + (0x00FF&(fromBuff)[1]))
#define INT_RECON(fromBuff) (SHORT_RECON(fromBuff) << 16) + (SHORT_RECON(fromBuff + 2))

int runThroughStringMatches(StringMatchParams* wo){
	std::string* curFSInd = &(wo->allSMatch[0][0]);
	intptr_t curFSLen = wo->searchLens[atoi(curFSInd->c_str())];
	uint64_t endStore[ENTRY_SIZE];
	char bufferL[FLEN_L];
	char bufferI[FLEN_I];
	char bufferS[FLEN_S];
    #define RUNSM_FREAD(toBuff,entSize,entCnt,fromF) \
        if(fread(toBuff,entSize,entCnt,fromF) != (size_t)entCnt){\
            std::cerr << "Problem handling item " << *curFSInd << std::endl;\
            return 1;\
        }
	//split major and minor names
	// and load the starts, ends, chromosomes, cigars and strands for each
		std::vector<std::string> majnames;
		std::vector<std::string> minnames;
		std::vector<int> liveInds;
		std::vector<CigarRefIdentifier> cigSEStr;
		std::vector<std::string> chroms;
		for(unsigned i = 0; i<wo->allSMatch.size(); i++){
			//split up the name
				std::string* curNam = &(wo->snameForI[atoi(wo->allSMatch[i][3].c_str())]);
				const char* majS = curNam->c_str();
				const char* majE = majS + strcspn(majS, "_");
				const char* minS = majE + 1;
				std::string majNam(majS, majE); majnames.push_back(majNam);
				std::string minNam(minS); minnames.push_back(minNam);
				*(wo->resF) << "L " << DEF_DUMP << " " << wo->allSMatch[i][4] << std::endl;
			//get the protein data from the table
				if(!findProteinDataByName(majNam.c_str(), minNam.c_str(), wo->inTab, wo->inDat, wo->numP, endStore)){
					*(wo->resF) << "W " << DEF_DUMP << " Missing Info" << std::endl;
					continue;
				}
			//number of individuals
				fseekPointer(wo->inDat, endStore[2], SEEK_SET);
				RUNSM_FREAD(bufferI, FLEN_I, 1, wo->inDat);
				*(wo->resF) << "C " << DEF_DUMP << " " << (INT_RECON(bufferI)) << std::endl;
			//start positions
				std::vector<uintptr_t> startP;
				fseekPointer(wo->inDat, endStore[3], SEEK_SET);
				RUNSM_FREAD(bufferS, FLEN_S, 1, wo->inDat);
				int numSP = SHORT_RECON(bufferS);
				for(int j = 0; j<numSP; j++){
					RUNSM_FREAD(bufferL, FLEN_L, 1, wo->inDat);
					startP.push_back(canonParseInt64(bufferL));
				}
			//end positions
				std::vector<uintptr_t> endP;
				fseekPointer(wo->inDat, endStore[4], SEEK_SET);
				RUNSM_FREAD(bufferS, FLEN_S, 1, wo->inDat);
				int numEP = SHORT_RECON(bufferS);
				for(int j = 0; j<numEP; j++){
					RUNSM_FREAD(bufferL, FLEN_L, 1, wo->inDat);
					endP.push_back(canonParseInt64(bufferL));
				}
			//chromosome
				fseekPointer(wo->inDat, endStore[5] + 2, SEEK_SET);
				RUNSM_FREAD(bufferS, FLEN_S, 1, wo->inDat);
				int chrLen = SHORT_RECON(bufferS);
				char* curChrom = (char*)malloc(chrLen);
				RUNSM_FREAD(curChrom, 1, chrLen, wo->inDat);
				std::string curChromS(curChrom, curChrom + chrLen);
				free(curChrom);
			//cigar
				fseekPointer(wo->inDat, endStore[6], SEEK_SET);
				RUNSM_FREAD(bufferI, FLEN_I, 1, wo->inDat);
				int cigLen = INT_RECON(bufferI);
				char* curCig = (char*)malloc(cigLen);
				RUNSM_FREAD(curCig, 1, cigLen, wo->inDat);
				std::string curCigS(curCig, curCig + cigLen);
				free(curCig);
				*(wo->resF) << "V " << DEF_DUMP << " " << curCigS << std::endl;
			//strand
				fseekPointer(wo->inDat, endStore[7], SEEK_SET);
				RUNSM_FREAD(bufferS, FLEN_S, 1, wo->inDat);
				int sideLen = SHORT_RECON(bufferS);
				char* curSide = (char*)malloc(sideLen);
				RUNSM_FREAD(curSide, 1, sideLen, wo->inDat);
				std::string curSideS(curSide, curSide + sideLen);
				free(curSide);
			//add to vectors
				CigarRefIdentifier curSEVal(curCigS, startP, endP, curSideS);
				cigSEStr.push_back(curSEVal);
				chroms.push_back(curChromS);
				liveInds.push_back(i);
		}
	//calculate locations of the variants
		std::vector< std::vector<intptr_t> > cigPepLocs;
		for(unsigned i = 0; i<liveInds.size(); i++){
			int fi = liveInds[i];
			uintptr_t curMatSI = 3*atoi(wo->allSMatch[fi][4].c_str());
			uintptr_t curMatEI = curMatSI + 3*curFSLen;
			std::string* majNam = &(majnames[fi]);
			std::string* minNam = &(minnames[fi]);
			std::string nonCID = *curFSInd + " " + *majNam + " " + *minNam;
			CigarRefResult* cigRet = cigarToRefLocations(&(cigSEStr[i]), wo->resF, &nonCID, wo->cigMemo);
			if(curMatSI > cigRet->nextLocs.size()){
				curMatSI = cigRet->nextLocs.size();
			}
			if(curMatEI > cigRet->nextLocs.size()){
				curMatEI = cigRet->nextLocs.size();
			}
			std::vector<intptr_t> curPepLocs;
			curPepLocs.insert(curPepLocs.begin(), cigRet->nextLocs.begin() + curMatSI, cigRet->nextLocs.begin() + curMatEI);
			cigPepLocs.push_back(curPepLocs);
		}
	//dump out the address ranges
		#define ADD_CHROM_LOC_STR \
			if(curRanS >= 0){\
				if(needPipe){ curDump.push_back('|'); } needPipe = true;\
				curDump.append(*curChrom);\
				curDump.push_back(':');\
				sprintf(strdumpbuff, "%ju", (uintmax_t)curRanS);\
				curDump.append(strdumpbuff);\
				if(curRanE >= 0){\
					curDump.push_back('-');\
					sprintf(strdumpbuff, "%ju", (uintmax_t)curRanE);\
					curDump.append(strdumpbuff);\
				}\
			}
		for(unsigned i = 0; i<liveInds.size(); i++){
			char strdumpbuff[4*sizeof(uintmax_t)+2];
			int fi = liveInds[i];
			std::vector<intptr_t>* curLocs = &(cigPepLocs[i]);
			std::string* curChrom = &(chroms[i]);
			bool needPipe = false;
			std::string curDump;
			bool negRun = false;
			intptr_t curRanS = -1;
			intptr_t curRanE = -1;
			intptr_t seqDir;
			for(unsigned j = 0; j<curLocs->size(); j++){
				intptr_t curLoc = (*curLocs)[j];
				if(negRun){
					if(curLoc < 0){ continue; }
					negRun = false;
				}
				else if(curLoc < 0){
					negRun = true;
					ADD_CHROM_LOC_STR
					curDump.append("|-");
					curRanS = -1; curRanE = -1;
					continue;
				}
				if(curRanS < 0){ curRanS = curLoc; }
				else if(curRanE < 0){
					if(curLoc == (curRanS + 1)){ curRanE = curLoc; seqDir = 1; }
					else if(curLoc == (curRanS - 1)){ curRanE = curLoc; seqDir = -1; }
					else{
						ADD_CHROM_LOC_STR
						curRanS = -1;
					}
				}
				else if(curLoc == (curRanE + seqDir)){ curRanE = curLoc; }
				else{
					ADD_CHROM_LOC_STR
					curRanS = curLoc;
					curRanE = -1;
				}
			}
			ADD_CHROM_LOC_STR
			if(curDump.size() == 0){ curDump.push_back('?'); }
			std::string* majNam = &(majnames[fi]);
			std::string* minNam = &(minnames[fi]);
			*(wo->resF) << "X " << *curFSInd << " " << *majNam << " " << *minNam << " " << curDump << std::endl;
		}
	//split things into different majors
		std::map<std::string, std::vector<uintptr_t> > allMajGroup;
		for(unsigned i = 0; i<liveInds.size(); i++){
			int fi = liveInds[i];
			std::string* majNam = &(majnames[fi]);
			allMajGroup[*majNam].push_back(i);
		}
	//note whether any minor shows up twice
		bool anyTwice = false;
		for(std::map<std::string, std::vector<uintptr_t> >::iterator mit = allMajGroup.begin(); mit != allMajGroup.end(); mit++){
			std::set<std::string> allSeenMin;
			std::vector<uintptr_t>* curMinInd = &(mit->second);
			for(unsigned si = 0; si<curMinInd->size(); si++){
				unsigned i = (*curMinInd)[si];
				int fi = liveInds[i];
				std::string* curMinN = &(minnames[fi]);
				if(allSeenMin.find(*curMinN) != allSeenMin.end()){
					anyTwice = true;
					break;
				}
				allSeenMin.insert(*curMinN);
			}
			if(anyTwice){ break; }
		}
	//merge within majors (note whether any minor shows up twice)
		std::vector<intptr_t> reverseTmp;
		for(std::map<std::string, std::vector<uintptr_t> >::iterator mit = allMajGroup.begin(); mit != allMajGroup.end(); mit++){
			std::vector<uintptr_t>* curMinInd = &(mit->second);
			unsigned si = 0;
			while(si < curMinInd->size()){
				unsigned i = (*curMinInd)[si];
				int fi = liveInds[i];
				//get and reverse the current vector
				std::vector<intptr_t>* curILocs = &(cigPepLocs[i]);
				reverseTmp.clear();
				reverseTmp.insert(reverseTmp.begin(), curILocs->begin(), curILocs->end());
				std::reverse(reverseTmp.begin(), reverseTmp.end());
				//run through the rest
				unsigned sj = si + 1;
				while(sj < curMinInd->size()){
					unsigned j = (*curMinInd)[sj];
					uintptr_t fj = liveInds[j];
					std::vector<intptr_t>* curJLocs = &(cigPepLocs[j]);
					if((*curILocs == *curJLocs) || (reverseTmp == *curJLocs)){
						*(wo->resF) << "M " << *curFSInd << " " << majnames[fi] << " " << minnames[fi] << " " << majnames[fj] << " " << minnames[fj] << std::endl;
						curMinInd->erase(curMinInd->begin() + sj);
					}
					else{ sj++; }
				}
				si++;
			}
		}
	//merge between majors
		for(std::map<std::string, std::vector<uintptr_t> >::iterator mita = allMajGroup.begin(); mita != allMajGroup.end(); mita++){
			std::vector<uintptr_t>* curMinIndA = &(mita->second);
			for(unsigned si = 0; si<curMinIndA->size(); si++){
				unsigned i = (*curMinIndA)[si];
				int fi = liveInds[i];
				//get and reverse the current vector
				std::vector<intptr_t>* curILocs = &(cigPepLocs[i]);
				reverseTmp.clear();
				reverseTmp.insert(reverseTmp.begin(), curILocs->begin(), curILocs->end());
				std::reverse(reverseTmp.begin(), reverseTmp.end());
				for(std::map<std::string, std::vector<uintptr_t> >::iterator mitb = mita; mitb != allMajGroup.end(); mitb++){
					if(mitb == mita){ continue; }
					std::vector<uintptr_t>* curMinIndB = &(mitb->second);
					unsigned sj = 0;
					while(sj < curMinIndB->size()){
						unsigned j = (*curMinIndB)[sj];
						uintptr_t fj = liveInds[j];
						std::vector<intptr_t>* curJLocs = &(cigPepLocs[j]);
						if((*curILocs == *curJLocs) || (reverseTmp == *curJLocs)){
							*(wo->resF) << "M " << *curFSInd << " " << majnames[fi] << " " << minnames[fi] << " " << majnames[fj] << " " << minnames[fj] << std::endl;
							curMinIndB->erase(curMinIndB->begin() + sj);
						}
						else{ sj++; }
					}
				}
			}
		}
	//remove all major entries that have zero items, note whether any minor variant shows up twice
		int mitInd = 0;
		std::map<std::string, std::vector<uintptr_t> >::iterator mit = allMajGroup.begin();
		while(mit != allMajGroup.end()){
			std::vector<uintptr_t>* curMinInd = &(mit->second);
			if(curMinInd->size()){
				mitInd++;
				mit++;
			}
			else{
				allMajGroup.erase(mit);
				mit = allMajGroup.begin(); for(int j = 0; j<mitInd; j++){ mit++; }
			}
		}
	//success if only one major left, failure if more
		if((allMajGroup.size() > 1) || anyTwice){
			*(wo->resF) << "D ";
		}
		else{
			*(wo->resF) << "A ";
		}
	//and dump out the equivalencie classes
		*(wo->resF) << *curFSInd;
		for(std::map<std::string, std::vector<uintptr_t> >::iterator mita = allMajGroup.begin(); mita != allMajGroup.end(); mita++){
			std::vector<uintptr_t>* curMinIndA = &(mita->second);
			for(unsigned si = 0; si<curMinIndA->size(); si++){
				unsigned i = (*curMinIndA)[si];
				int fi = liveInds[i];
				*(wo->resF) << " " << majnames[fi] << " " << minnames[fi];
			}
		}
		*(wo->resF) << std::endl;
	return 0;
}

int main(int argc, char** argv){
	std::string line;
	char bufferL[FLEN_L];
	//parse the arguments
		char* inTabN = 0;
		char* inDatN = 0;
		char* snamN = 0;
		char* digSN = 0;
		char* origN = 0;
		char* outFN = 0;
		#define CHECK_ARG(wantArg, endFoc) \
			if(strncmp(argv[i], wantArg, strlen(wantArg)) == 0){\
				endFoc = argv[i] + strlen(wantArg);\
				continue;\
			}
		for(int i = 1; i<argc; i++){
			CHECK_ARG(CLARG_INT, inTabN)
			CHECK_ARG(CLARG_IND, inDatN)
			CHECK_ARG(CLARG_RFN, snamN)
			CHECK_ARG(CLARG_DIG, digSN)
			CHECK_ARG(CLARG_ORI, origN)
			CHECK_ARG(CLARG_OUT, outFN)
		}
		if(!(inTabN && inDatN && snamN && digSN && origN && outFN)){
			std::cerr << "Missing required argument" << std::endl;
			return 1;
		}
		StringMatchParams allSMPar;
	//load in the string names
		std::vector<std::string>& allSNames = allSMPar.snameForI;
		std::ifstream refNames(snamN);
		while(std::getline(refNames, line)){
			const char* lineACS = line.c_str();
			const char* lineS = lineACS + strspn(lineACS, WHITESPACE);
			const char* lineE = lineS + strcspn(lineS, WHITESPACE);
			if(lineS == lineE){ continue; }
			std::string curVal(lineS, lineE);
			allSNames.push_back(curVal);
		}
		refNames.close();
	//get the lengths of the strings in the fasta file
		std::vector<uintptr_t>* allLens = &(allSMPar.searchLens);
		std::ifstream origF(origN);
		while(std::getline(origF, line)){
			if(line[0] == '>'){
				allLens->push_back(0);
				continue;
			}
			if(allLens->size()){
				const char* curLin = line.c_str();
				while(*curLin){
					int curWsSpan = strspn(curLin, WHITESPACE);
					if(curWsSpan){
						curLin += curWsSpan;
						continue;
					}
					int curCharSpan = strcspn(curLin, WHITESPACE);
					(*allLens)[allLens->size()-1] += curCharSpan;
					curLin += curCharSpan;
				}
			}
		}
		origF.close();
	//open the files of interest
		FILE* inTab = fopen(inTabN, "rb");
		if(inTab == 0){ std::cerr << "Problem opening table " << inTabN << std::endl; return 1; }
		FILE* inDat = fopen(inDatN, "rb");
		if(inDat == 0){ std::cerr << "Problem opening data " << inDatN << std::endl; fclose(inTab); return 1; }
		std::ofstream outF(outFN);
		allSMPar.inTab = inTab;
		allSMPar.inDat = inDat;
		allSMPar.resF = &outF;
		#define ERR_CLEAN_UP fclose(inTab); fclose(inDat); outF.close();
	//get the number of unique sequences
		if(fread(bufferL, 1, FLEN_L, inTab) != FLEN_L){std::cerr << "Malformed table file." << std::endl; return 1;}
		uintptr_t numProt = canonParseInt64(bufferL);
		allSMPar.numP = numProt;
	//make the memoization map
		std::map<CigarRefIdentifier,CigarRefResult,bool(*)(const CigarRefIdentifier&,const CigarRefIdentifier&)> cigMemo(compCigarRefId);
		allSMPar.cigMemo = &cigMemo;
	//scan through the digest file
		intptr_t curSInd = -1;
		std::vector< std::vector< std::string > >* curSMatch = &(allSMPar.allSMatch);
		std::ifstream digSF(digSN);
		while(std::getline(digSF, line)){
			std::vector<std::string> lineSpl;
			const char* curLin = line.c_str();
			while(*curLin){
				int curWsSpan = strspn(curLin, WHITESPACE);
				if(curWsSpan){
					curLin += curWsSpan;
					continue;
				}
				int curCharSpan = strcspn(curLin, WHITESPACE);
				std::string nextStr(curLin, curLin + curCharSpan);
				lineSpl.push_back(nextStr);
				curLin += curCharSpan;
			}
			if(lineSpl.size()){
				int curLineInd = atoi(lineSpl[0].c_str());
				if(curLineInd < curSInd){
					continue;
				}
				if(curLineInd != curSInd){
					if(curSInd >= 0){
						runThroughStringMatches(&allSMPar);
					}
					curSInd++;
					while(curSInd < curLineInd){
						outF << "S " << curSInd << std::endl;
						curSInd++;
					}
					curSMatch->clear();
				}
				curSMatch->push_back(lineSpl);
			}
		}
		if(curSMatch->size()){
			runThroughStringMatches(&allSMPar);
		}
	//clean up
		fclose(inTab);
		fclose(inDat);
		outF.close();
		digSF.close();
}
