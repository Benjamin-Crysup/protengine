#include "protengine_engine.h"
#include "protengine_engine_index.h"
#include "protengine_engine_index_private.h"

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
 * Free the entries of a vector.
 * @param toKill The thing to kill.
 */
void killCharVectorContents(std::vector<char*>* toKill){
	for(uintptr_t i = 0; i<toKill->size(); i++){
		free((*toKill)[i]);
	}
}

IndexData::IndexData(){
	myName = 0;
	myFolder = 0;
}
IndexData::~IndexData(){
	free(myName);
	free(myFolder);
	//TODO
}
IndexSet::IndexSet(){}
IndexSet::~IndexSet(){
	for(std::map<std::string,IndexData*>::iterator it = allIndices.begin(); it!=allIndices.end(); it++){
		delete (it->second);
	}
}

/**
 * Cleans up index information.
 * @param toKill The thing to remove from.
 */
void killIndexCommands(EngineState* toKill){
	std::map<std::string,void*>::iterator toKillIt = toKill->state.find(STATEID_INDICES);
	if(toKillIt != toKill->state.end()){
		IndexSet* curKill = (IndexSet*)(toKillIt->second);
		delete(curKill);
		toKill->state.erase(toKillIt);
	}
	delete(toKill->liveFunctions[COMMAND_INITINDEX]);
	delete(toKill->liveFunctions[COMMAND_LOADINDEX]);
	delete(toKill->liveFunctions[COMMAND_FINDFASTA]);
	delete(toKill->liveFunctions[COMMAND_SUFFARRINDEX]);
	delete(toKill->liveFunctions[COMMAND_FINDSINGLESTRING]);
	delete(toKill->liveFunctions[COMMAND_COMBOSUFFARRBUILD]);
	delete(toKill->liveFunctions[COMMAND_COMBOSUFFARRSEARCH]);
	//TODO
}

void addIndexCommands(EngineState* toAddTo){
	toAddTo->liveFunctions[COMMAND_INITINDEX] = new InitIndexCommand();
	toAddTo->liveFunctions[COMMAND_LOADINDEX] = new IndexAddReferenceCommand();
	toAddTo->liveFunctions[COMMAND_FINDFASTA] = new RawSearchCommand();
	toAddTo->liveFunctions[COMMAND_SUFFARRINDEX] = new PrecomputeSACommand();
	toAddTo->liveFunctions[COMMAND_FINDSINGLESTRING] = new PrecomSASearchCommand();
	toAddTo->liveFunctions[COMMAND_COMBOSUFFARRBUILD] = new PrecomputeComboCommand();
	toAddTo->liveFunctions[COMMAND_COMBOSUFFARRSEARCH] = new PrecomComboSearchCommand();
	toAddTo->killFunctions.push_back(killIndexCommands);
	toAddTo->state[STATEID_INDICES] = new IndexSet();
}

#define PARSE_REQUIRED_OPTION(optResName,errMess,errRes) \
	char* optResName;\
	curFoc = getNextOptionalArgument(curFoc, &(optResName));\
	if(!(optResName)){\
		std::cerr << errMess << std::endl;\
		errRes\
		return 0;\
	}

/**This handles callbacks from a single source file.*/
class SingleSourceFindCallback : public SequenceFindCallback{
public:
	SingleSourceFindCallback(FILE* dumpFilePtr, const char* indexName, const char* referenceName){
		dumpFile = dumpFilePtr;
		indName = indexName;
		refName = referenceName;
	}
	virtual ~SingleSourceFindCallback(){}
	virtual int foundSequence(uintptr_t fndSeq, uintptr_t stringIndex, uintptr_t suffixIndex){
		fprintf(dumpFile, "%ju %s %s %ju %ju\n", (uintmax_t)fndSeq, indName, refName, (uintmax_t)stringIndex, (uintmax_t)suffixIndex);
		return 0;
	}
	FILE* dumpFile;
	const char* indName;
	const char* refName;
};

/**This handles callbacks from multiple source files.*/
class MultipleSourceFindCallback : public SequenceFindCallback{
public:
	MultipleSourceFindCallback(FILE* dumpFilePtr, const char* indexName, std::vector<std::string>* referenceNames, std::vector<uintptr_t>* referenceSizes){
		dumpFile = dumpFilePtr;
		indName = indexName;
		allRefNames = *referenceNames;
		uintptr_t totalOff = 0;
		for(uintptr_t i = 0; i<referenceSizes->size(); i++){
			allRefOffs.push_back(totalOff);
			uintptr_t curSize = (*referenceSizes)[i];
			for(uintptr_t j = 0; j<curSize; j++){
				allRefInds.push_back(i);
			}
			totalOff += curSize;
		}
	}
	virtual ~MultipleSourceFindCallback(){}
	virtual int foundSequence(uintptr_t fndSeq, uintptr_t stringIndex, uintptr_t suffixIndex){
		uintptr_t windex = allRefInds[stringIndex];
		fprintf(dumpFile, "%ju %s %s %ju %ju\n", (uintmax_t)fndSeq, indName, allRefNames[windex].c_str(), stringIndex - allRefOffs[windex], (uintmax_t)suffixIndex);
		return 0;
	}
	FILE* dumpFile;
	const char* indName;
	/**The names of each reference.*/
	std::vector<std::string> allRefNames;
	/**The offsets of each reference, in terms of linear indices.*/
	std::vector<uintptr_t> allRefOffs;
	/**The indices of the reference name and offset for linear string indices.*/
	std::vector<uintptr_t> allRefInds;
};

////////////////////////////////////////////////////////////////
//InitIndex

InitIndexCommand::~InitIndexCommand(){}
void* InitIndexCommand::parseCommand(const char* toParse){
	const char* curFoc = toParse;
	PARSE_REQUIRED_OPTION(comIndName, "Problem parsing " << COMMAND_INITINDEX, )
	PARSE_REQUIRED_OPTION(comFoldName, "Problem parsing " << COMMAND_INITINDEX, free(comIndName);)
	char** toRet = (char**)malloc(2*sizeof(char*));
	toRet[0] = comIndName;
	toRet[1] = comFoldName;
	return toRet;
}
int InitIndexCommand::runCommand(void* toRun, EngineState* forState){
	IndexSet* indState = (IndexSet*)(forState->state[STATEID_INDICES]);
	char** toRunC = (char**)toRun;
	if(indState->allIndices.find(toRunC[0]) != indState->allIndices.end()){
		std::cerr << "Index " << toRunC[0] << " already exists" << std::endl;
		return 1;
	}
	IndexData* newInd = new IndexData();
	newInd->myName = strdup(toRunC[0]);
	newInd->myFolder = strdup(toRunC[1]);
	fixArgumentFilename(newInd->myFolder);
	indState->allIndices[newInd->myName] = newInd;
	return 0;
}
void InitIndexCommand::freeParsed(void* parseDat){
	char** toRunC = (char**)parseDat;
	free(toRunC[0]);
	free(toRunC[1]);
	free(toRunC);
}
const char* InitIndexCommand::getCommandHelp(){
	return "IndexInit indName foldName\n\tAdds a new index to consider.\n\tindName -- The name used for this index.\n\tfoldName -- The name of the folder this index uses.\n";
}

////////////////////////////////////////////////////////////////
//IndexReference

IndexAddReferenceCommand::~IndexAddReferenceCommand(){}
void* IndexAddReferenceCommand::parseCommand(const char* toParse){
	const char* curFoc = toParse;
	PARSE_REQUIRED_OPTION(comIndName, "Problem parsing " << COMMAND_LOADINDEX, )
	PARSE_REQUIRED_OPTION(comRefName, "Problem parsing " << COMMAND_LOADINDEX, free(comIndName);)
	char* comFileName;
	curFoc = getNextOptionalArgument(curFoc, &comFileName);
	char** toRet = (char**)malloc(3*sizeof(char*));
	toRet[0] = comIndName;
	toRet[1] = comRefName;
	toRet[2] = comFileName;
	return toRet;
}
int IndexAddReferenceCommand::runCommand(void* toRun, EngineState* forState){
	IndexSet* indState = (IndexSet*)(forState->state[STATEID_INDICES]);
	char** toRunC = (char**)toRun;
	if(indState->allIndices.find(toRunC[0]) == indState->allIndices.end()){
		std::cerr << "Index " << toRunC[0] << " does not exist." << std::endl;
		return 1;
	}
	IndexData* workInd = indState->allIndices[toRunC[0]];
	if(workInd->allReferences.find(toRunC[1]) != workInd->allReferences.end()){
		std::cerr << "Reference " << toRunC[1] << " already exists." << std::endl;
		return 1;
	}
	//build it if need-be
	GZipCompressionMethod useComp;
	//RawOutputCompressionMethod useComp;
	GET_REFERENCE_FASTA_FILE(failGzFName,workInd,toRunC[1])
	if(!fileExists(failGzFName)){
		//try to build it
		if(toRunC[2]){
			//build it
			int fileWriteProb = buildFailgzFromFastAQ(toRunC[2], failGzFName, &useComp);
			if(fileWriteProb){
				free(failGzFName);
				return fileWriteProb;
			}
		}
		else{
			std::cerr << "No filename to load provided." << std::endl;
			free(failGzFName);
			return 1;
		}
	}
	//open it, read the first 8 bytes and save the number
	FailGZReader workReader(failGzFName, &useComp);
	uintptr_t numSeqInRef = workReader.numEntries;
	if(numSeqInRef < 0){
		std::cerr << "Problem reading reference." << std::endl;
		return 1;
	}
	//add info
	ReferenceData newInfo;
	newInfo.numSeq = numSeqInRef;
	newInfo.haveSuffArr = false;
	newInfo.haveMergeSuffArr = false;
	workInd->allReferences[toRunC[1]] = newInfo;
	return 0;
}
void IndexAddReferenceCommand::freeParsed(void* parseDat){
	char** toRunC = (char**)parseDat;
	free(toRunC[0]);
	free(toRunC[1]);
	free(toRunC[2]);
	free(toRunC);
}
const char* IndexAddReferenceCommand::getCommandHelp(){
	return "IndexReference indName refName filename.fa\n\tAdds a reference sequence to an index.\n\tindName -- The name of the index to add to.\n\trefName -- The reporting name of the reference sequence(s).\n\tfilename.fa -- The fasta file containing the references to add. Leave out if you have previously built a reference.\n";
}

////////////////////////////////////////////////////////////////
//FindRawSearch

RawSearchCommand::~RawSearchCommand(){}
void* RawSearchCommand::parseCommand(const char* toParse){
	const char* curFoc = toParse;
	PARSE_REQUIRED_OPTION(comIndName, "Problem parsing " << COMMAND_FINDFASTA, )
	PARSE_REQUIRED_OPTION(comRefName, "Problem parsing " << COMMAND_FINDFASTA, free(comIndName);)
	PARSE_REQUIRED_OPTION(comFastNam, "Problem parsing " << COMMAND_FINDFASTA, free(comIndName);free(comRefName);)
	PARSE_REQUIRED_OPTION(comDumpNam, "Problem parsing " << COMMAND_FINDFASTA, free(comIndName);free(comRefName);free(comFastNam);)
	char** toRet = (char**)malloc(4*sizeof(char*));
	toRet[0] = comIndName;
	toRet[1] = comRefName;
	toRet[2] = comFastNam;
	toRet[3] = comDumpNam;
	return toRet;
}
int RawSearchCommand::runCommand(void* toRun, EngineState* forState){
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
	//note the maximum ram
	uintptr_t maxRam = ((uintptr_t*)(forState->state[STATEID_MAXRAM]))[0];
	//open the target
	FILE* dumpFile = fopen(toRunC[3], "wb");
	if(dumpFile == 0){
		std::cerr << "Cannot open " << toRunC[3] << std::endl;
		return 1;
	}
	SingleSourceFindCallback dumpCall(dumpFile, toRunC[0], toRunC[1]);
	//note the location of the file and load it
	GET_REFERENCE_FASTA_FILE(failGzFName,workInd,toRunC[1])
	GZipCompressionMethod useComp;
	//RawOutputCompressionMethod useComp;
	FailGZReader workReader(failGzFName, &useComp);
	free(failGzFName);
	if(workReader.numEntries < 0){
		std::cerr << "Problem reading " << toRunC[1] << std::endl;
		return 1;
	}
	//open up the search file, chunk by max ram
	LiveFastqFileReader* readPass1 = openFastqFile(toRunC[2]);
	if(!readPass1){
		std::cerr << "Problem reading " << toRunC[2] << std::endl;
		fclose(dumpFile); free(failGzFName);
		return 1;
	}
	std::vector<char*> allSeqs;
	std::vector<const char*> allPassSeqs;
	uintptr_t curSeq = 0;
	uintptr_t totLoaded = 0;
	#define RAWSEARCH_PERFORM_DUMP \
		int findProb = findSequencesInFailGz(&workReader, &allPassSeqs, curSeq, &dumpCall);\
		curSeq += allSeqs.size();\
		for(uintptr_t i = 0; i<allSeqs.size(); i++){\
			free(allSeqs[i]);\
		}\
		allSeqs.clear();\
		allPassSeqs.clear();\
		totLoaded = 0;\
		if(findProb){\
			closeFastqFile(readPass1);\
			fclose(dumpFile);\
			return 1;\
		}
	while(readNextFastqEntry(readPass1)){
		allSeqs.push_back(strdup(readPass1->lastReadSeq));
		allPassSeqs.push_back(allSeqs[allSeqs.size()-1]);
		totLoaded += (strlen(readPass1->lastReadSeq)+1);
		if(totLoaded > maxRam){
			RAWSEARCH_PERFORM_DUMP
		}
	}
	if(totLoaded > 0){
		RAWSEARCH_PERFORM_DUMP
	}
	//open it up
	closeFastqFile(readPass1);
	fclose(dumpFile); 
	return 0;
}
void RawSearchCommand::freeParsed(void* parseDat){
	char** toRunC = (char**)parseDat;
	free(toRunC[0]);
	free(toRunC[1]);
	free(toRunC[2]);
	free(toRunC[3]);
	free(toRunC);
}
const char* RawSearchCommand::getCommandHelp(){
	return "FindRawSearch indName refName search.fa dump.txt\n\tSearches for sequences from a raw sequence.\n\tindName -- The name of the index to search through.\n\trefName -- The name of the reference to search through.\n\tsearch.fa -- The fasta file containing the sequences to search for.\n\tdump.txt -- THe place to write the results.\n";
}

////////////////////////////////////////////////////////////////
//BuildSAPrecom

PrecomputeSACommand::~PrecomputeSACommand(){}
void* PrecomputeSACommand::parseCommand(const char* toParse){
	const char* curFoc = toParse;
	PARSE_REQUIRED_OPTION(comIndName, "Problem parsing " << COMMAND_SUFFARRINDEX, )
	PARSE_REQUIRED_OPTION(comRefName, "Problem parsing " << COMMAND_SUFFARRINDEX, free(comIndName);)
	char** toRet = (char**)malloc(2*sizeof(char*));
	toRet[0] = comIndName;
	toRet[1] = comRefName;
	return toRet;
}
int PrecomputeSACommand::runCommand(void* toRun, EngineState* forState){
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
	ReferenceData workRef = workInd->allReferences[toRunC[1]];
	//build it if need-be
	GZipCompressionMethod useComp;
	//RawOutputCompressionMethod useComp;
	GET_REFERENCE_SINGLESTRINGSA_FILE(fassaFName,workInd,toRunC[1])
	if(!fileExists(fassaFName)){
		GET_REFERENCE_FASTA_FILE(failGzFName,workInd,toRunC[1])
		FailGZReader workReader(failGzFName, &useComp);
		free(failGzFName);
		if(workReader.numEntries < 0){
			free(fassaFName);
			return 1;
		}
		int buildProb = buildFassaFromFailgz(&workReader, fassaFName, &useComp);
		free(fassaFName);
		if(buildProb){
			return 1;
		}
	}
	//note the suffix array
	workRef.haveSuffArr = true;
	workInd->allReferences[toRunC[1]] = workRef;
	return 0;
}
void PrecomputeSACommand::freeParsed(void* parseDat){
	char** toRunC = (char**)parseDat;
	free(toRunC[0]);
	free(toRunC[1]);
	free(toRunC);
}
const char* PrecomputeSACommand::getCommandHelp(){
	return "BuildSAPrecom indName refName\n\tPrecomputes single string suffix arrays for a reference.\n\tindName -- THe name of the relevant index.\n\trefName -- The name of the reference to precompute for.\n";
}

////////////////////////////////////////////////////////////////
//FindSAPrecom

PrecomSASearchCommand::~PrecomSASearchCommand(){}
void* PrecomSASearchCommand::parseCommand(const char* toParse){
	const char* curFoc = toParse;
	PARSE_REQUIRED_OPTION(comIndName, "Problem parsing " << COMMAND_FINDSINGLESTRING, )
	PARSE_REQUIRED_OPTION(comRefName, "Problem parsing " << COMMAND_FINDSINGLESTRING, free(comIndName);)
	PARSE_REQUIRED_OPTION(comFastNam, "Problem parsing " << COMMAND_FINDSINGLESTRING, free(comIndName);free(comRefName);)
	PARSE_REQUIRED_OPTION(comDumpNam, "Problem parsing " << COMMAND_FINDSINGLESTRING, free(comIndName);free(comRefName);free(comFastNam);)
	char** toRet = (char**)malloc(4*sizeof(char*));
	toRet[0] = comIndName;
	toRet[1] = comRefName;
	toRet[2] = comFastNam;
	toRet[3] = comDumpNam;
	return toRet;
}
int PrecomSASearchCommand::runCommand(void* toRun, EngineState* forState){
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
	//note the maximum ram
	uintptr_t maxRam = ((uintptr_t*)(forState->state[STATEID_MAXRAM]))[0];
	//open the target
	FILE* dumpFile = fopen(toRunC[3], "wb");
	if(dumpFile == 0){
		std::cerr << "Cannot open " << toRunC[3] << std::endl;
		return 1;
	}
	SingleSourceFindCallback dumpCall(dumpFile, toRunC[0], toRunC[1]);
	//note the location of the files and load them
	GET_REFERENCE_FASTA_FILE(failGzFName,workInd,toRunC[1])
	GZipCompressionMethod useComp;
	//RawOutputCompressionMethod useComp;
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
	//open up the search file, chunk by max ram
	LiveFastqFileReader* readPass1 = openFastqFile(toRunC[2]);
	if(!readPass1){
		std::cerr << "Problem reading " << toRunC[2] << std::endl;
		fclose(dumpFile); free(failGzFName);
		return 1;
	}
	std::vector<char*> allSeqs;
	std::vector<const char*> allPassSeqs;
	uintptr_t curSeq = 0;
	uintptr_t totLoaded = 0;
	#define SASEARCH_PERFORM_DUMP \
		int findProb = findSequencesInFassa(&workReader, &workFassa, &allPassSeqs, curSeq, &dumpCall);\
		curSeq += allSeqs.size();\
		for(uintptr_t i = 0; i<allSeqs.size(); i++){\
			free(allSeqs[i]);\
		}\
		allSeqs.clear();\
		allPassSeqs.clear();\
		totLoaded = 0;\
		if(findProb){\
			closeFastqFile(readPass1);\
			fclose(dumpFile);\
			return 1;\
		}
	while(readNextFastqEntry(readPass1)){
		allSeqs.push_back(strdup(readPass1->lastReadSeq));
		allPassSeqs.push_back(allSeqs[allSeqs.size()-1]);
		totLoaded += (strlen(readPass1->lastReadSeq)+1);
		if(totLoaded > maxRam){
			SASEARCH_PERFORM_DUMP
		}
	}
	if(totLoaded > 0){
		SASEARCH_PERFORM_DUMP
	}
	//open it up
	closeFastqFile(readPass1);
	fclose(dumpFile); 
	return 0;
}
void PrecomSASearchCommand::freeParsed(void* parseDat){
	char** toRunC = (char**)parseDat;
	free(toRunC[0]);
	free(toRunC[1]);
	free(toRunC[2]);
	free(toRunC[3]);
	free(toRunC);
}
const char* PrecomSASearchCommand::getCommandHelp(){
	return "FindSAPrecom indName refName search.fa dump.txt\n\tSearches through a reference using prebuilt single string suffix arrays.\n\tindName -- The name of the index to search through.\n\trefName -- The name of the reference to search through.\n\tsearch.fa -- The fasta file containing the sequences to search for.\n\tdump.txt -- The place to write the results.\n";
}

////////////////////////////////////////////////////////////////
//BuildComboPrecom

PrecomputeComboCommand::~PrecomputeComboCommand(){}
void* PrecomputeComboCommand::parseCommand(const char* toParse){
	std::vector<char*> parsedOpts;
	const char* curFoc = toParse;
	PARSE_REQUIRED_OPTION(comIndName, "Problem parsing " << COMMAND_COMBOSUFFARRBUILD, killCharVectorContents(&parsedOpts);)
		parsedOpts.push_back(comIndName);
	PARSE_REQUIRED_OPTION(comComName, "Problem parsing " << COMMAND_COMBOSUFFARRBUILD, killCharVectorContents(&parsedOpts);)
		parsedOpts.push_back(comComName);
	char* comRefName;
	do{
		curFoc = getNextOptionalArgument(curFoc, &comRefName);
		if(comRefName){
			parsedOpts.push_back(comRefName);
		}
	}while(comRefName);
	uintptr_t* retStore = (uintptr_t*)malloc(sizeof(uintptr_t) + parsedOpts.size()*sizeof(char*));
	char** strStore = (char**)(retStore+1);
	*retStore = parsedOpts.size();
	for(unsigned int i = 0; i<parsedOpts.size(); i++){
		strStore[i] = parsedOpts[i];
	}
	return retStore;
}
int PrecomputeComboCommand::runCommand(void* toRun, EngineState* forState){
	IndexSet* indState = (IndexSet*)(forState->state[STATEID_INDICES]);
	uintptr_t* toRunLP = (uintptr_t*)toRun;
	uintptr_t toRunL = *toRunLP;
	char** toRunC = (char**)(toRunLP + 1);
	if(indState->allIndices.find(toRunC[0]) == indState->allIndices.end()){
		std::cerr << "Index " << toRunC[0] << " does not exist." << std::endl;
		return 1;
	}
	IndexData* workInd = indState->allIndices[toRunC[0]];
	if(workInd->allComboIn.find(toRunC[1]) != workInd->allComboIn.end()){
		std::cerr << "Combo " << toRunC[1] << " already exists." << std::endl;
		return 1;
	}
	//ram and thread count
	uintptr_t maxRam = ((uintptr_t*)(forState->state[STATEID_MAXRAM]))[0];
	uintptr_t useThr = ((uintptr_t*)(forState->state[STATEID_THREAD]))[0];
	//open the files, note their size
	GZipCompressionMethod useComp;
	//RawOutputCompressionMethod useComp;
	std::vector<uintptr_t> allSrcLen;
	std::vector<FailGZReader*> allSrc;
	for(uintptr_t i = 2; i<toRunL; i++){
		GET_REFERENCE_FASTA_FILE(failGzFName,workInd,toRunC[i])
		FailGZReader* workReader = new FailGZReader(failGzFName, &useComp);
		free(failGzFName);
		if(workReader->numEntries < 0){
			for(uintptr_t j = 0; j<allSrc.size(); j++){
				delete(allSrc[j]);
			}
			return 1;
		}
		allSrc.push_back(workReader);
		allSrcLen.push_back(workReader->numEntries);
	}
	//build it if need-be
	int buildProb = 0;
	GET_COMBO_SUFFARR_FILE(fasfaFName,workInd,toRunC[1])
	if(!fileExists(fasfaFName)){
		//open up the fasta files
		buildProb = buildComboFileFromFailgz(&allSrc, fasfaFName, workInd->myFolder, useThr, maxRam, &useComp);
	}
	//kill
	free(fasfaFName);
	for(uintptr_t j = 0; j<allSrc.size(); j++){
		delete(allSrc[j]);
	}
	//note the suffix array
	std::vector<std::string> allRefed;
	for(uintptr_t i = 2; i<toRunL; i++){
		allRefed.push_back(toRunC[i]);
	}
	workInd->allComboIn[toRunC[1]] = allRefed;
	workInd->allComboSS[toRunC[1]] = allSrcLen;
	return buildProb;
}
void PrecomputeComboCommand::freeParsed(void* parseDat){
	uintptr_t* retStore = (uintptr_t*)parseDat;
	char** strStore = (char**)(retStore+1);
	uintptr_t numStr = *retStore;
	for(uintptr_t i = 0; i<numStr; i++){
		free(strStore[i]);
	}
	free(retStore);
}
const char* PrecomputeComboCommand::getCommandHelp(){
	return "BuildComboPrecom indName comboName refName+\n\tPrecomput a multistring suffix array for several reference.\n\tindName -- THe index to build for.\n\tcomboName -- The reporting name of the combo to build.\n\trefName -- The reference(s) to build for\n\t";
}

////////////////////////////////////////////////////////////////
//FindComboPrecom

PrecomComboSearchCommand::~PrecomComboSearchCommand(){}
void* PrecomComboSearchCommand::parseCommand(const char* toParse){
	const char* curFoc = toParse;
	PARSE_REQUIRED_OPTION(comIndName, "Problem parsing " << COMMAND_COMBOSUFFARRSEARCH, )
	PARSE_REQUIRED_OPTION(comComName, "Problem parsing " << COMMAND_COMBOSUFFARRSEARCH, free(comIndName);)
	PARSE_REQUIRED_OPTION(comFastNam, "Problem parsing " << COMMAND_COMBOSUFFARRSEARCH, free(comIndName);free(comComName);)
	PARSE_REQUIRED_OPTION(comDumpNam, "Problem parsing " << COMMAND_COMBOSUFFARRSEARCH, free(comIndName);free(comComName);free(comFastNam);)
	char** toRet = (char**)malloc(4*sizeof(char*));
	toRet[0] = comIndName;
	toRet[1] = comComName;
	toRet[2] = comFastNam;
	toRet[3] = comDumpNam;
	return toRet;
}
int PrecomComboSearchCommand::runCommand(void* toRun, EngineState* forState){
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
	//note the maximum ram
	uintptr_t maxRam = ((uintptr_t*)(forState->state[STATEID_MAXRAM]))[0];
	//get the reference names and the sizes
	std::vector<std::string>* refNames = &(workInd->allComboIn[toRunC[1]]);
	std::vector<uintptr_t>* refSizes = &(workInd->allComboSS[toRunC[1]]);
	//open up the fasfa file
	GZipCompressionMethod useComp;
	//RawOutputCompressionMethod useComp;
	GET_COMBO_SUFFARR_FILE(fasfaFName,workInd,toRunC[1]);
	FasfaReader readFasf(fasfaFName, &useComp);
	free(fasfaFName);
	if(readFasf.numSequences < 0){
		std::cerr << "Problem reading " << toRunC[1] << std::endl;
		return 1;
	}
	//open the target
	FILE* dumpFile = fopen(toRunC[3], "wb");
	if(dumpFile == 0){
		std::cerr << "Cannot open " << toRunC[3] << std::endl;
		return 1;
	}
	MultipleSourceFindCallback dumpCall(dumpFile, toRunC[0], refNames, refSizes);
	//open up the reference fail files
	std::vector<FailGZReader*> allFails;
	for(uintptr_t i = 0; i<refNames->size(); i++){
		GET_REFERENCE_FASTA_FILE(failGzFName,workInd,(*refNames)[i].c_str())
		FailGZReader* curRead = new FailGZReader(failGzFName, &useComp);
		allFails.push_back(curRead);
		free(failGzFName);
		if(curRead->numEntries < 0){
			std::cerr << "Problem reading " << (*refNames)[i] << std::endl;
			fclose(dumpFile);
			for(uintptr_t j = 0; j<allFails.size(); j++){
				delete (allFails[j]);
			}
			return 1;
		}
	}
	//open up the search file, chunk by max ram
	LiveFastqFileReader* readPass1 = openFastqFile(toRunC[2]);
	if(!readPass1){
		std::cerr << "Problem reading " << toRunC[2] << std::endl;
		fclose(dumpFile);
		return 1;
	}
	std::vector<char*> allSeqs;
	std::vector<const char*> allPassSeqs;
	uintptr_t curSeq = 0;
	uintptr_t totLoaded = 0;
	#define COMBOSEARCH_PERFORM_DUMP \
		int findProb = findSequencesInFasfa(&allFails, &readFasf, &allPassSeqs, curSeq, &dumpCall);\
		curSeq += allSeqs.size();\
		for(uintptr_t i = 0; i<allSeqs.size(); i++){\
			free(allSeqs[i]);\
		}\
		allSeqs.clear();\
		allPassSeqs.clear();\
		totLoaded = 0;\
		if(findProb){\
			closeFastqFile(readPass1);\
			fclose(dumpFile);\
			for(uintptr_t j = 0; j<allFails.size(); j++){ delete (allFails[j]); }\
			return 1;\
		}
	while(readNextFastqEntry(readPass1)){
		allSeqs.push_back(strdup(readPass1->lastReadSeq));
		allPassSeqs.push_back(allSeqs[allSeqs.size()-1]);
		totLoaded += (strlen(readPass1->lastReadSeq)+1);
		if(totLoaded > maxRam){
			COMBOSEARCH_PERFORM_DUMP
		}
	}
	if(totLoaded > 0){
		COMBOSEARCH_PERFORM_DUMP
	}
	//open it up
	closeFastqFile(readPass1);
	fclose(dumpFile); 
	for(uintptr_t j = 0; j<allFails.size(); j++){ delete (allFails[j]); }
	return 0;
}
void PrecomComboSearchCommand::freeParsed(void* parseDat){
	char** toRunC = (char**)parseDat;
	free(toRunC[0]);
	free(toRunC[1]);
	free(toRunC[2]);
	free(toRunC[3]);
	free(toRunC);
}
const char* PrecomComboSearchCommand::getCommandHelp(){
	return "FindComboPrecom indName comboName search.fa dump.txt\n\tSearches through multiple references using a prebuild multi-string suffix array.\n\tindName -- The name of the index to search through.\n\tcomboName -- The name of the combo to search through.\n\tsearch.fa -- The fasta file containing the sequences to search for.\n\tdump.txt -- The place to write the results.\n"; //TODO
}
