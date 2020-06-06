
#include <map>
#include <vector>
#include <fstream>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <iostream>

#define CLARG_INU "-inU="
#define CLARG_TSV "-tsv="
#define CLARG_OUT "-out="
#define CLARG_HAP "-hap="

#define WHITESPACE " \t\r\n"

/**Info on a major/minor pair.*/
class MinorClassInfo{
public:
	/**Whether reference info is present.*/
	bool haveInfo;
	/**Whether the reference info has an odd number of bases.*/
	bool codonProb;
	/**Whether the cigar string has a problem.*/
	bool cigarProb;
	/**The number of haploids with this variant.*/
	uintptr_t indivCount;
	/**The locations the search string was found at.*/
	std::vector<uintptr_t> foundAt;
	/**The genomic locations of those findings.*/
	std::vector<std::string> foundLoc;
	/**Set up.*/
	MinorClassInfo(){
		haveInfo = true;
		codonProb = false;
		cigarProb = false;
		indivCount = -1;
	}
	/**Tear down.*/
	~MinorClassInfo(){}
};

/**Info on a major protein.*/
class MajorClassInfo{
public:
	/**The variants.*/
	std::map<std::string, MinorClassInfo*> foundMinors;
	/**Set up.*/
	MajorClassInfo(){}
	/**Tear down.*/
	~MajorClassInfo(){
		for(std::map<std::string, MinorClassInfo*>::iterator mit = foundMinors.begin(); mit != foundMinors.end(); mit++){
			delete(mit->second);
		}
	}
};

int main(int argc, char** argv){
	//parse the arguments
		char* fullN = 0;
		char* add2N = 0;
		char* outN = 0;
		char* hapStr = 0;
		#define CHECK_ARG(wantArg, endFoc) \
			if(strncmp(argv[i], wantArg, strlen(wantArg)) == 0){\
				endFoc = argv[i] + strlen(wantArg);\
				continue;\
			}
		for(int i = 1; i<argc; i++){
			CHECK_ARG(CLARG_INU, fullN)
			CHECK_ARG(CLARG_TSV, add2N)
			CHECK_ARG(CLARG_OUT, outN)
			CHECK_ARG(CLARG_HAP, hapStr)
		}
		if(!(fullN && add2N && outN && hapStr)){
			std::cerr << "Missing required argument" << std::endl;
			return 1;
		}
		double allNum = atof(hapStr);
	//open the tsv and its annotation
		std::ifstream tsvF(add2N);
		std::ifstream annF(fullN);
		std::ofstream outF(outN);
		std::string lineT;
		std::string lineA;
		bool haveLA = false;
		bool endedLA = false;
	//pass the first line of the tsv with some added categories
		if(!std::getline(tsvF, lineT)){
			std::cerr << "Problem with tsv file" << std::endl;
			return 1;
		}
		outF << lineT << "	found	found_known	unique	count_low	count_high	allele_freq	found_in	found_at	found_count	found_refloc	info_missing	ref_non_codon	ref_cigar_mismatch" << std::endl;
	//run down the lines of the tsv
		uintptr_t curTSVLine = 0;
		while(std::getline(tsvF, lineT)){
			//read until the annotation line has a higher index
				std::vector<std::string> allAnnotate;
				while(true){
					if(haveLA){
						haveLA = false;
						//skip to the index
						const char* curFoc = lineA.c_str();
						curFoc += strspn(curFoc, WHITESPACE);
						if(*curFoc == 0){ continue; }
						curFoc += strcspn(curFoc, WHITESPACE);
						curFoc += strspn(curFoc, WHITESPACE);
						if(*curFoc == 0){ continue; }
						uintptr_t lineInd = atol(curFoc);
						if(lineInd < curTSVLine){ continue; }
						else if(lineInd > curTSVLine){ haveLA = true; break; }
						else{ allAnnotate.push_back(lineA); }
					}
					else if(endedLA){
						break;
					}
					else{
						if(std::getline(annF, lineA)){
							haveLA = true;
							continue;
						}
						else{
							endedLA = true;
							break;
						}
					}
				}
			//figure out what the annotation lines were saying
				bool markSkip = false;
				bool thinkUnique = false;
				std::map<std::string, MajorClassInfo*> annotDat;
				for(unsigned i = 0; i<allAnnotate.size(); i++){
					const char* curAnn = allAnnotate[i].c_str();
					curAnn += strspn(curAnn, WHITESPACE);
					if(*curAnn == 'S'){
						markSkip = true;
						continue;
					}
					else if(*curAnn == 'D'){
						thinkUnique = false;
						continue;
					}
					else if(*curAnn == 'A'){
						thinkUnique = true;
						continue;
					}
					//find the major and minor
						const char* tmpScan = curAnn + 1;
						tmpScan += strspn(tmpScan, WHITESPACE);
						tmpScan += strcspn(tmpScan, WHITESPACE);
						const char* majStart = tmpScan + strspn(tmpScan, WHITESPACE);
						const char* majEnd = majStart + strcspn(majStart, WHITESPACE);
						const char* minStart = majEnd + strspn(majEnd, WHITESPACE);
						const char* minEnd = minStart + strcspn(minStart, WHITESPACE);
						std::string majNam(majStart, majEnd);
						std::string minNam(minStart, minEnd);
						if(annotDat.find(majNam) == annotDat.end()){
							annotDat[majNam] = new MajorClassInfo();
						}
						MajorClassInfo* majInfo = annotDat[majNam];
						if(majInfo->foundMinors.find(minNam) == majInfo->foundMinors.end()){
							majInfo->foundMinors[minNam] = new MinorClassInfo();
						}
						MinorClassInfo* minInfo = majInfo->foundMinors[minNam];
					//run through the possibilities
					switch(*curAnn){
						case 'L':
							{
								uintptr_t newVal = atol(minEnd + strspn(minEnd, WHITESPACE));
								minInfo->foundAt.push_back(newVal);
							}
							break;
						case 'C':
							{
								uintptr_t newVal = atol(minEnd + strspn(minEnd, WHITESPACE));
								minInfo->indivCount = newVal;
							}
							break;
						case 'X':
							{
								const char* locStart = minEnd + strspn(minEnd, WHITESPACE);
								const char* locEnd = locStart + strcspn(locStart, WHITESPACE);
								std::string loc(locStart, locEnd);
								minInfo->foundLoc.push_back(loc);
							}
							break;
						case 'W':
							{
								if(strstr(minEnd, "Missing Info") != 0){
									minInfo->haveInfo = false;
								}
								else if(strstr(minEnd, "Non-codon") != 0){
									minInfo->codonProb = true;
								}
								else if(strstr(minEnd, "Cigar-Reference Number Mismatch") != 0){
									minInfo->cigarProb = true;
								}
							}
							break;
						default:
							//do nothing
							continue;
					}
				}
			//write out the starting line with the extra crap
				outF << lineT;
				if(markSkip || (annotDat.size()==0)){
					outF << "	False	False	\"\"	\"\"	\"\"	\"\"	\"\"	\"\"	\"\"	\"\"	\"\"	\"\"	\"\"";
				}
				else{
					char intAscBuffer[4*sizeof(uintptr_t)+4];
					outF << "	True";
					std::map<std::string, MajorClassInfo*>::iterator mat;
					std::map<std::string, MinorClassInfo*>::iterator mit;
					//see if anything has data
					bool firstRound = true;
					bool seeData = false;
					std::string missingDat;
					std::string nonCodon;
					std::string cigProblem;
					std::string foundIn;
					std::string foundAt;
					std::string foundCount;
					std::string foundGenRef;
					std::map< std::string, uintptr_t > majCountMap;
					for(mat = annotDat.begin(); mat != annotDat.end(); mat++){
						const std::string* curMajN = &(mat->first);
						MajorClassInfo* curMajC = mat->second;
						for(mit = curMajC->foundMinors.begin(); mit != curMajC->foundMinors.end(); mit++){
							const std::string* curMinN = &(mit->first);
							MinorClassInfo* curMinC = mit->second;
							//the first round
								if(!firstRound){
									foundIn.push_back(',');
									foundAt.push_back(',');
									foundCount.push_back(',');
									foundGenRef.push_back(',');
								}
								firstRound = false;
							//add found in and at
								foundIn.append(*curMajN);
								foundIn.push_back('_');
								foundIn.append(*curMinN);
								for(unsigned i = 0; i<curMinC->foundAt.size(); i++){
									sprintf(intAscBuffer, "%ju", (uintmax_t)(curMinC->foundAt[i]));
									if(i){ foundAt.push_back('/'); }
									foundAt.append(intAscBuffer);
								}
							//if have info, note it
								if(curMinC->haveInfo){
									seeData = true;
									if(majCountMap.find(*curMajN) == majCountMap.end()){
										majCountMap[*curMajN] = 0;
									}
									majCountMap[*curMajN] += curMinC->indivCount;
									//add to count and genRef
									sprintf(intAscBuffer, "%ju", (uintmax_t)(curMinC->indivCount));
									foundCount.append(intAscBuffer);
									for(unsigned i = 0; i<curMinC->foundLoc.size(); i++){
										if(i){ foundGenRef.push_back('/'); }
										foundGenRef.append(curMinC->foundLoc[i]);
									}
								}
								else{
									if(missingDat.size()){ missingDat.push_back(','); }
									missingDat.append(*curMajN);
									missingDat.push_back('_');
									missingDat.append(*curMinN);
									//count and genRef are empty
								}
							//note codon problem and cigar problem
								if(curMinC->codonProb){
									if(nonCodon.size()){ nonCodon.push_back(','); }
									nonCodon.append(*curMajN);
									nonCodon.push_back('_');
									nonCodon.append(*curMinN);
								}
								if(curMinC->cigarProb){
									if(cigProblem.size()){ cigProblem.push_back(','); }
									cigProblem.append(*curMajN);
									cigProblem.push_back('_');
									cigProblem.append(*curMinN);
								}
						}
					}
					//dump it
						if(seeData){
							outF << "	True	" << (thinkUnique ? "True" : "False");
							//counts
							uintptr_t maxCount = 0;
							uintptr_t minCount = UINTPTR_MAX;
							std::map< std::string, uintptr_t >::iterator mci;
							for(mci = majCountMap.begin(); mci != majCountMap.end(); mci++){
								uintptr_t curCount = mci->second;
								if(curCount > maxCount){ maxCount = curCount; }
								if(curCount < minCount){ minCount = curCount; }
							}
							outF << "	" << minCount << "	" << maxCount << "	" << (maxCount / allNum);
						}
						else{
							outF << "	False	\"\"	\"\"	\"\"	\"\"";
						}
						outF << "	" << (foundIn.size() ? foundIn : "\"\"");
						outF << "	" << (foundAt.size() ? foundAt : "\"\"");
						outF << "	" << (foundCount.size() ? foundCount : "\"\"");
						outF << "	" << (foundGenRef.size() ? foundGenRef : "\"\"");
						outF << "	" << (missingDat.size() ? missingDat : "\"\"");
						outF << "	" << (nonCodon.size() ? nonCodon : "\"\"");
						outF << "	" << (cigProblem.size() ? cigProblem : "\"\"");
				}
				outF << std::endl;
			//delete the things
				for(std::map<std::string, MajorClassInfo*>::iterator mit = annotDat.begin(); mit != annotDat.end(); mit++){
					delete(mit->second);
				}
			curTSVLine++;
		}
	tsvF.close();
	annF.close();
	outF.close();
	return 0;
}
