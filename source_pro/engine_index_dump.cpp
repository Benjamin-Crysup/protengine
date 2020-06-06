#include "protengine_engine_index_dump.h"
#include "protengine_engine_index_private.h"

#include <map>
#include <string>
#include <fstream>
#include <iostream>
#include <string.h>
#include <stdlib.h>

#include "whodun_bamfa.h"
#include "whodun_oshook.h"
#include "protengine_compress.h"
#include "protengine_index_failgz.h"
#include "protengine_index_fassa.h"
#include "protengine_index_fasfa.h"

#define WHITESPACE " \t\r\n"

/**
 * Cleans up index information.
 * @param toKill The thing to remove from.
 */
void killIndexDumpCommands(EngineState* toKill){
	delete(toKill->liveFunctions[COMMAND_DUMPINDEX]);
	delete(toKill->liveFunctions[COMMAND_DUMPSAINDEX]);
	delete(toKill->liveFunctions[COMMAND_DUMPCOMBOINDEX]);
	delete(toKill->liveFunctions[COMMAND_DUMPCOMBONAMEINDEX]);
	delete(toKill->liveFunctions[COMMAND_DUMPSEARCHREGINDEX]);
}

void addIndexDumpCommands(EngineState* toAddTo){
	toAddTo->liveFunctions[COMMAND_DUMPINDEX] = new IndexDumpCommand();
	toAddTo->liveFunctions[COMMAND_DUMPSAINDEX] = new IndexDumpSACommand();
	toAddTo->liveFunctions[COMMAND_DUMPCOMBOINDEX] = new IndexDumpComboCommand();
	toAddTo->liveFunctions[COMMAND_DUMPCOMBONAMEINDEX] = new IndexDumpComboNamesCommand();
	toAddTo->liveFunctions[COMMAND_DUMPSEARCHREGINDEX] = new IndexDumpSearchRegionCommand();
	toAddTo->killFunctions.push_back(killIndexDumpCommands);
}

#define PARSE_REQUIRED_OPTION(optResName,errMess,errRes) \
	char* optResName;\
	curFoc = getNextOptionalArgument(curFoc, &(optResName));\
	if(!(optResName)){\
		std::cerr << errMess << std::endl;\
		errRes\
		return 0;\
	}

IndexDumpCommand::~IndexDumpCommand(){}
void* IndexDumpCommand::parseCommand(const char* toParse){
	const char* curFoc = toParse;
	PARSE_REQUIRED_OPTION(comIndName, "Problem parsing " << COMMAND_LOADINDEX, )
	PARSE_REQUIRED_OPTION(comRefName, "Problem parsing " << COMMAND_LOADINDEX, free(comIndName);)
	PARSE_REQUIRED_OPTION(comDmpName, "Problem parsing " << COMMAND_LOADINDEX, free(comIndName); free(comRefName); )
	char** toRet = (char**)malloc(3*sizeof(char*));
	toRet[0] = comIndName;
	toRet[1] = comRefName;
	toRet[2] = comDmpName;
	return toRet;
}
int IndexDumpCommand::runCommand(void* toRun, EngineState* forState){
	IndexSet* indState = (IndexSet*)(forState->state[STATEID_INDICES]);
	char** toRunC = (char**)toRun;
	if(indState->allIndices.find(toRunC[0]) == indState->allIndices.end()){
		std::cerr << "Index " << toRunC[0] << " does not exist." << std::endl;
		return 1;
	}
	IndexData* workInd = indState->allIndices[toRunC[0]];
	if(workInd->allReferences.find(toRunC[1]) == workInd->allReferences.end()){
		std::cerr << "Reference " << toRunC[1] << " does not exist." << std::endl;
		return 1;
	}
	//note the location of the file and load it
	GET_REFERENCE_FASTA_FILE(failGzFName,workInd,toRunC[1])
	GZipCompressionMethod useComp;
	FailGZReader workReader(failGzFName, &useComp);
	free(failGzFName);
	if(workReader.numEntries < 0){
		std::cerr << "Problem reading " << toRunC[1] << std::endl;
		return 1;
	}
	//open up the dump file
	FILE* dumpFile = fopen(toRunC[2], "wb");
	if(dumpFile == 0){
		std::cerr << "Problem writing " << toRunC[2] << std::endl;
		return 1;
	}
	//read and dump
	for(intptr_t i = 0; i<workReader.numEntries; i++){
		if(workReader.loadInEntry(i)){
			fclose(dumpFile); return 1;
		};
		fprintf(dumpFile, ">%s\n%s\n", workReader.name, workReader.seq);
	}
	fclose(dumpFile);
	return 0;
}
void IndexDumpCommand::freeParsed(void* parseDat){
	char** toRunC = (char**)parseDat;
	free(toRunC[0]);
	free(toRunC[1]);
	free(toRunC[2]);
	free(toRunC);
}
const char* IndexDumpCommand::getCommandHelp(){
	return "IndexDumpReference indName refName dumpName\n\tWrites out as reference in fasta format.\n\tindName -- The name of the index to add to.\n\trefName -- The name of the reference to dump\n\tdumpName -- The place to dump to.\n";
}

IndexDumpSACommand::~IndexDumpSACommand(){}
void* IndexDumpSACommand::parseCommand(const char* toParse){
	const char* curFoc = toParse;
	PARSE_REQUIRED_OPTION(comIndName, "Problem parsing " << COMMAND_LOADINDEX, )
	PARSE_REQUIRED_OPTION(comRefName, "Problem parsing " << COMMAND_LOADINDEX, free(comIndName);)
	PARSE_REQUIRED_OPTION(comDmpName, "Problem parsing " << COMMAND_LOADINDEX, free(comIndName); free(comRefName); )
	char** toRet = (char**)malloc(3*sizeof(char*));
	toRet[0] = comIndName;
	toRet[1] = comRefName;
	toRet[2] = comDmpName;
	return toRet;
}
int IndexDumpSACommand::runCommand(void* toRun, EngineState* forState){
	IndexSet* indState = (IndexSet*)(forState->state[STATEID_INDICES]);
	char** toRunC = (char**)toRun;
	if(indState->allIndices.find(toRunC[0]) == indState->allIndices.end()){
		std::cerr << "Index " << toRunC[0] << " does not exist." << std::endl;
		return 1;
	}
	IndexData* workInd = indState->allIndices[toRunC[0]];
	if(workInd->allReferences.find(toRunC[1]) == workInd->allReferences.end()){
		std::cerr << "Reference " << toRunC[1] << " does not exist." << std::endl;
		return 1;
	}
	//note the location of the files and load them
	GET_REFERENCE_FASTA_FILE(failGzFName,workInd,toRunC[1])
	GZipCompressionMethod useComp;
	FailGZReader workReader(failGzFName, &useComp);
	free(failGzFName);
	if(workReader.numEntries < 0){
		std::cerr << "Problem reading " << toRunC[1] << std::endl;
		return 1;
	}
	GET_REFERENCE_SINGLESTRINGSA_FILE(fassaFName,workInd,toRunC[1])
	FassaReader workFassa(fassaFName, &useComp);
	free(fassaFName);
	if(workFassa.numEntries < 0){
		std::cerr << "Problem reading " << toRunC[1] << std::endl;
		return 1;
	}
	//open up the dump file
	FILE* dumpFile = fopen(toRunC[2], "wb");
	if(dumpFile == 0){
		std::cerr << "Problem writing " << toRunC[2] << std::endl;
		return 1;
	}
	//read and dump
	#define DUMPSTRLEN 5
	char writeStr[DUMPSTRLEN+1];
	writeStr[DUMPSTRLEN] = 0;
	for(intptr_t i = 0; i<workReader.numEntries; i++){
		if(workReader.loadInEntry(i)){ fclose(dumpFile); return 1; };
		if(workFassa.loadInEntry(i)){ fclose(dumpFile); return 1; };
		fprintf(dumpFile, "str%jd\n", (intmax_t)i);
		for(uintptr_t j = 0; j<workFassa.numSequences; j++){
			if(workFassa.loadInSequence(j)){ fclose(dumpFile); return 1; }
			for(uintptr_t k = 0; k<workFassa.numInCurrent; k++){
				uintmax_t curEnt = workFassa.currentSequence[k];
				for(uintptr_t l = 0; l<DUMPSTRLEN; l++){
					if(curEnt + l >= workReader.seqLen){
						writeStr[l] = 0;
						break;
					}
					writeStr[l] = workReader.seq[curEnt + l];
				}
				fprintf(dumpFile, "\t%jd %jd %s\n", (intmax_t)i, curEnt, writeStr);
			}
		}
	}
	fclose(dumpFile);
	return 0;
}
void IndexDumpSACommand::freeParsed(void* parseDat){
	char** toRunC = (char**)parseDat;
	free(toRunC[0]);
	free(toRunC[1]);
	free(toRunC[2]);
	free(toRunC);
}
const char* IndexDumpSACommand::getCommandHelp(){
	return "IndexDumpSA indName refName dumpName\n\tWrites out a single string suffix array. Used for debugging.\n\tindName -- The name of the index to add to.\n\trefName -- The name of the reference to dump\n\tdumpName -- The place to dump to.\n";
}

IndexDumpComboCommand::~IndexDumpComboCommand(){}
void* IndexDumpComboCommand::parseCommand(const char* toParse){
	const char* curFoc = toParse;
	PARSE_REQUIRED_OPTION(comIndName, "Problem parsing " << COMMAND_LOADINDEX, )
	PARSE_REQUIRED_OPTION(comRefName, "Problem parsing " << COMMAND_LOADINDEX, free(comIndName);)
	PARSE_REQUIRED_OPTION(comDmpName, "Problem parsing " << COMMAND_LOADINDEX, free(comIndName); free(comRefName); )
	char** toRet = (char**)malloc(3*sizeof(char*));
	toRet[0] = comIndName;
	toRet[1] = comRefName;
	toRet[2] = comDmpName;
	return toRet;
}
int IndexDumpComboCommand::runCommand(void* toRun, EngineState* forState){
	IndexSet* indState = (IndexSet*)(forState->state[STATEID_INDICES]);
	char** toRunC = (char**)toRun;
	if(indState->allIndices.find(toRunC[0]) == indState->allIndices.end()){
		std::cerr << "Index " << toRunC[0] << " does not exist." << std::endl;
		return 1;
	}
	IndexData* workInd = indState->allIndices[toRunC[0]];
	if(workInd->allComboIn.find(toRunC[1]) == workInd->allComboIn.end()){
		std::cerr << "Combo " << toRunC[1] << " does not exist." << std::endl;
		return 1;
	}
	GZipCompressionMethod useComp;
	//get the reference names and the sizes
	std::vector<std::string>* refNames = &(workInd->allComboIn[toRunC[1]]);
	//note the conversion from linear index to file/piece
	std::vector<uintptr_t> sindToFF;
	std::vector<uintptr_t> sindToFI;
	for(uintptr_t i = 0; i<refNames->size(); i++){
		GET_REFERENCE_FASTA_FILE(failGzFName,workInd,(*refNames)[i].c_str())
		FailGZReader tmpFGReader(failGzFName, &useComp);
		free(failGzFName);
		if(tmpFGReader.numEntries < 0){ return 1; }
		for(intptr_t j = 0; j<tmpFGReader.numEntries; j++){
			sindToFF.push_back(i);
			sindToFI.push_back(j);
		}
	}
	//open up the fasfa file
	GET_COMBO_SUFFARR_FILE(fasfaFName,workInd,toRunC[1]);
	FasfaReader readFasf(fasfaFName, &useComp);
	free(fasfaFName);
	if(readFasf.numSequences < 0){
		std::cerr << "Problem reading " << toRunC[1] << std::endl;
		return 1;
	}
	//open up the dump file
	FILE* dumpFile = fopen(toRunC[2], "wb");
	if(dumpFile == 0){
		std::cerr << "Problem writing " << toRunC[2] << std::endl;
		return 1;
	}
	char writeStr[DUMPSTRLEN+1];
	writeStr[DUMPSTRLEN] = 0;
	//read and dump
	for(intptr_t i = 0; i<readFasf.numSequences; i++){
		if(readFasf.loadInSequence(i)){fclose(dumpFile); return 1;}
		for(uintptr_t j = 0; j<readFasf.numInCurrent; j++){
			uintptr_t slind = readFasf.currentSequence[2*j];
			uintptr_t suind = readFasf.currentSequence[2*j+1];
			uintptr_t sFind = sindToFF[slind];
			uintptr_t sIind = sindToFI[slind];
			//open the file
			GET_REFERENCE_FASTA_FILE(failGzFName,workInd,(*refNames)[sFind].c_str())
			FailGZReader tmpFGReader(failGzFName, &useComp);
			free(failGzFName);
			if(tmpFGReader.numEntries < 0){ fclose(dumpFile); return 1; }
			if(tmpFGReader.loadInEntry(sIind)){ fclose(dumpFile); return 1; }
			//dump
			for(uintptr_t l = 0; l<DUMPSTRLEN; l++){
				if(suind + l >= tmpFGReader.seqLen){
					writeStr[l] = 0;
					break;
				}
				writeStr[l] = tmpFGReader.seq[suind + l];
			}
			fprintf(dumpFile, "%jd %jd %jd %s\n", (intmax_t)sFind, (intmax_t)sIind, (intmax_t)suind, writeStr);
		}
	}
	//clean up
	fclose(dumpFile);
	return 0;
}
void IndexDumpComboCommand::freeParsed(void* parseDat){
	char** toRunC = (char**)parseDat;
	free(toRunC[0]);
	free(toRunC[1]);
	free(toRunC[2]);
	free(toRunC);
}
const char* IndexDumpComboCommand::getCommandHelp(){
	return "IndexDumpCombo indName comName dumpName\n\tWrites out a combo file for debugging.\n\tindName -- The name of the index to add to.\n\tcomName -- The name of the combo to dump\n\tdumpName -- The place to dump to.\n";
}

IndexDumpComboNamesCommand::~IndexDumpComboNamesCommand(){}
void* IndexDumpComboNamesCommand::parseCommand(const char* toParse){
	const char* curFoc = toParse;
	PARSE_REQUIRED_OPTION(comIndName, "Problem parsing " << COMMAND_LOADINDEX, )
	PARSE_REQUIRED_OPTION(comRefName, "Problem parsing " << COMMAND_LOADINDEX, free(comIndName);)
	PARSE_REQUIRED_OPTION(comDmpName, "Problem parsing " << COMMAND_LOADINDEX, free(comIndName); free(comRefName); )
	char** toRet = (char**)malloc(3*sizeof(char*));
	toRet[0] = comIndName;
	toRet[1] = comRefName;
	toRet[2] = comDmpName;
	return toRet;
}
int IndexDumpComboNamesCommand::runCommand(void* toRun, EngineState* forState){
	IndexSet* indState = (IndexSet*)(forState->state[STATEID_INDICES]);
	char** toRunC = (char**)toRun;
	if(indState->allIndices.find(toRunC[0]) == indState->allIndices.end()){
		std::cerr << "Index " << toRunC[0] << " does not exist." << std::endl;
		return 1;
	}
	IndexData* workInd = indState->allIndices[toRunC[0]];
	if(workInd->allComboIn.find(toRunC[1]) == workInd->allComboIn.end()){
		std::cerr << "Combo " << toRunC[1] << " does not exist." << std::endl;
		return 1;
	}
	GZipCompressionMethod useComp;
	//get the reference names
	std::vector<std::string>* refNames = &(workInd->allComboIn[toRunC[1]]);
	//open up the dump file
	FILE* dumpFile = fopen(toRunC[2], "wb");
	if(dumpFile == 0){
		std::cerr << "Problem writing " << toRunC[2] << std::endl;
		return 1;
	}
	//run them down
	for(uintptr_t i = 0; i<refNames->size(); i++){
		GET_REFERENCE_FASTA_FILE(failGzFName,workInd,(*refNames)[i].c_str())
		FailGZReader tmpFGReader(failGzFName, &useComp);
		free(failGzFName);
		if(tmpFGReader.numEntries < 0){ fclose(dumpFile); return 1; }
		for(intptr_t j = 0; j<tmpFGReader.numEntries; j++){
			if(tmpFGReader.loadInEntry(j)){ fclose(dumpFile); return 1; }
			fprintf(dumpFile, "%s\n", tmpFGReader.name);
		}
	}
	//clean up
	fclose(dumpFile);
	return 0;
}
void IndexDumpComboNamesCommand::freeParsed(void* parseDat){
	char** toRunC = (char**)parseDat;
	free(toRunC[0]);
	free(toRunC[1]);
	free(toRunC[2]);
	free(toRunC);
}
const char* IndexDumpComboNamesCommand::getCommandHelp(){
	return "IndexDumpComboNames indName comName dumpName\n\tWrites out the strings used in a combo file, in order.\n\tindName -- The name of the index to add to.\n\tcomName -- The name of the combo to dump\n\tdumpName -- The place to dump to.\n";
}

IndexDumpSearchRegionCommand::~IndexDumpSearchRegionCommand(){}
void* IndexDumpSearchRegionCommand::parseCommand(const char* toParse){
	char** toRet = (char**)malloc(5*sizeof(char*));
	const char* curFoc = toParse;
	{ PARSE_REQUIRED_OPTION(comFastNam, "Problem parsing " << COMMAND_COMBOSUFFARRSEARCH, goto errHandleA;) toRet[0] = comFastNam; }
	{ PARSE_REQUIRED_OPTION(comResName, "Problem parsing " << COMMAND_COMBOSUFFARRSEARCH, goto errHandleB;) toRet[1] = comResName; }
	{ PARSE_REQUIRED_OPTION(comBefName, "Problem parsing " << COMMAND_COMBOSUFFARRSEARCH, goto errHandleC;) toRet[2] = comBefName; }
	{ PARSE_REQUIRED_OPTION(comAftName, "Problem parsing " << COMMAND_COMBOSUFFARRSEARCH, goto errHandleD;) toRet[3] = comAftName; }
	{ PARSE_REQUIRED_OPTION(comDmpName, "Problem parsing " << COMMAND_COMBOSUFFARRSEARCH, goto errHandleE;) toRet[4] = comDmpName; }
	return toRet;
	errHandleE:
		free(toRet[3]);
	errHandleD:
		free(toRet[2]);
	errHandleC:
		free(toRet[1]);
	errHandleB:
		free(toRet[0]);
	errHandleA:
		free(toRet);
		return 0;
}
int IndexDumpSearchRegionCommand::runCommand(void* toRun, EngineState* forState){
	IndexSet* indState = (IndexSet*)(forState->state[STATEID_INDICES]);
	char** toRunC = (char**)toRun;
	GZipCompressionMethod useComp;
	//not the number before and after
	int numBef = atoi(toRunC[2]);
	int numAft = atoi(toRunC[3]);
	//open up the fasta, get the lengths of each entry
	std::vector<uintptr_t> allSearchLens;
	LiveFastqFileReader* readPass1 = openFastqFile(toRunC[0]);
	if(!readPass1){
		std::cerr << "Problem reading " << toRunC[0] << std::endl;
		return 1;
	}
	while(readNextFastqEntry(readPass1)){
		allSearchLens.push_back(strlen(readPass1->lastReadSeq));
	}
	closeFastqFile(readPass1);
	//open the dump and search result
	std::ifstream rawSearch(toRunC[1]);
	std::ofstream seqDump(toRunC[4]);
	//maintain a list of open failgz files
	std::map<std::string, std::map<std::string,FailGZReader*> > openRefs;
	//run down the search result
	std::string line;
	while(std::getline(rawSearch, line)){
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
		if(lineSpl.size() == 5){
			uintptr_t fStrInd = atoi(lineSpl[0].c_str());
			std::string& indNam = lineSpl[1];
			std::string& refNam = lineSpl[2]; 
			uintptr_t rStrInd = atoi(lineSpl[3].c_str());
			uintptr_t rStrLoc = atoi(lineSpl[4].c_str());
			FailGZReader* curRead;
				std::map<std::string, std::map<std::string,FailGZReader*> >::iterator it = openRefs.find(indNam);
				if((it != openRefs.end()) && (it->second.find(refNam) != it->second.end())){
					curRead = openRefs[indNam][refNam];
				}
				else{
					if(indState->allIndices.find(indNam) == indState->allIndices.end()){
						std::cerr << "Index " << indNam << " does not exist." << std::endl; goto errHandle;
					}
					IndexData* workInd = indState->allIndices[indNam];
					if(workInd->allReferences.find(refNam) == workInd->allReferences.end()){
						std::cerr << "Reference " << refNam << " does not exist." << std::endl; goto errHandle;
					}
					GET_REFERENCE_FASTA_FILE(failGzFName,workInd,refNam.c_str())
					curRead = new FailGZReader(failGzFName, &useComp);
					free(failGzFName);
					openRefs[indNam][refNam] = curRead;
					if(curRead->numEntries < 0){
						std::cerr << "Problem reading " << refNam << std::endl;
						goto errHandle;
					}
				}
			if(fStrInd >= allSearchLens.size()){
				std::cerr << "Problem reading " << refNam << std::endl; goto errHandle;
			}
			uintptr_t curFSLen = allSearchLens[fStrInd];
			if(rStrInd >= (uintptr_t)(curRead->numEntries)){
				std::cerr << "Problem reading " << refNam << std::endl; goto errHandle;
			}
			if(curRead->loadInEntry(rStrInd)){
				std::cerr << "Problem reading " << refNam << std::endl; goto errHandle;
			}
			if((rStrLoc + curFSLen) >= curRead->seqLen){
				std::cerr << "Problem reading " << refNam << std::endl; goto errHandle;
			}
			const char* seqStart = curRead->seq;
			const char* seqEnd = curRead->seq + curRead->seqLen - 1;
			const char* fsStart = curRead->seq + rStrLoc;
			const char* fsEnd = fsStart + curFSLen;
			const char* prefStart = fsStart - numBef;
				if(prefStart < seqStart){ prefStart = seqStart; }
			const char* suffEnd = fsEnd + numAft;
				if(suffEnd > seqEnd){ suffEnd = seqEnd; }
			const char* hexChars = "0123456789ABCDEF";
			seqDump << "(";
			for(const char* curF = prefStart; curF < fsStart; curF++){ seqDump << hexChars[0x0F & (*curF)>>4] << hexChars[0x0F & *curF]; }
			seqDump << ")(";
			for(const char* curF = fsStart; curF < fsEnd; curF++){ seqDump << hexChars[0x0F & (*curF)>>4] << hexChars[0x0F & *curF]; }
			seqDump << ")(";
			for(const char* curF = fsEnd; curF < suffEnd; curF++){ seqDump << hexChars[0x0F & (*curF)>>4] << hexChars[0x0F & *curF]; }
			seqDump << ")";
		}
		seqDump << std::endl;
	}
	//clean up
	for(std::map<std::string, std::map<std::string,FailGZReader*> >::iterator it1 = openRefs.begin(); it1 != openRefs.end(); it1++){
		for(std::map<std::string,FailGZReader*>::iterator it2 = it1->second.begin(); it2 != it1->second.end(); it2++){
			delete(it2->second);
		}
	}
	rawSearch.close();
	seqDump.close();
	return 0;
	errHandle:
		for(std::map<std::string, std::map<std::string,FailGZReader*> >::iterator it1 = openRefs.begin(); it1 != openRefs.end(); it1++){
			for(std::map<std::string,FailGZReader*>::iterator it2 = it1->second.begin(); it2 != it1->second.end(); it2++){
				delete(it2->second);
			}
		}
		rawSearch.close();
		seqDump.close();
		return 1;
}
void IndexDumpSearchRegionCommand::freeParsed(void* parseDat){
	char** toRunC = (char**)parseDat;
	for(int i = 0; i<5; i++){
		free(toRunC[i]);
	}
	free(toRunC);
}
const char* IndexDumpSearchRegionCommand::getCommandHelp(){
	return "IndexDumpSearchRegion search.fa searchFile numBefore numAfter dumpName\n\t\
	Writes out the strings used in a combo file, in order.\n\
	search.fa -- The sequences that were searched for.\n\
	searchFile -- The search result file.\n\
	numBefore -- The number of characters before the match to dump.\n\
	numAfter -- The number of characters after the match to dump.\n\
	dumpName -- The place to dump to.\n";
}
