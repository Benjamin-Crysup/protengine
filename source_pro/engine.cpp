#include "protengine_engine.h"

#include <map>
#include <time.h>
#include <vector>
#include <string>
#include <ctype.h>
#include <fstream>
#include <sstream>
#include <stdlib.h>
#include <iostream>
#include <string.h>
#include <algorithm>

#include "protengine_engine_base.h"
#include "protengine_engine_index.h"
#include "protengine_engine_index_dump.h"

/**The command line argument for command source.*/
#define CLARG_COMFILE "-com"

/**Whitespace characters.*/
#define WHITESPACE " \t\r\n"

/**
 * Runs the engine.
 * @param argc The number of arguments.
 * @param argv The arguments. -dllfl name provides a list of dll names in a file. -com name Provides a text file to run, rather than stdin.
 * @return Whether there was a problem.
 */
int main(int argc, char** argv){
	//parse the arguments
		std::vector<char*> comFiles;
		bool useStdin = false;
		int i = 1;
		while(i < argc){
			if(strcmp(argv[i], CLARG_COMFILE)==0){
				i++;
				comFiles.push_back(argv[i]);
			}
			else{
				std::cerr << "Unknown command " << argv[i] << std::endl;
				return 1;
			}
			i++;
		}
	//prepare the state
		EngineState* currentState = new EngineState();
	//note all the extra commands
		addBaseCommands(currentState);
		addIndexCommands(currentState);
		addIndexDumpCommands(currentState);
		//TODO
	//run through the required text streams, running commands
		if(!useStdin){
			comFiles.push_back(0);
		}
		for(unsigned int j = 0; j<comFiles.size(); j++){
			int wasProb;
			std::string curLine;
			char* curFileName = comFiles[j];
			std::istream* curFile;
			if(curFileName){
				curFile = new std::ifstream(curFileName);
			}
			else{
				curFile = &std::cin;
			}
			while(std::getline(*curFile, curLine)){
				std::cout << curLine << std::endl;
				clock_t startT = clock();
				wasProb = currentState->runCommand(curLine.c_str());
				if(wasProb){
					std::cerr << "problem with " << curLine << std::endl;
					break;
				}
				clock_t endT = clock();
				std::cout << "Time taken: " << ((double)(endT - startT))/CLOCKS_PER_SEC << "(s)" << std::endl;
			}
			if(curFileName){
				delete(curFile);
			}
			if(wasProb){break;}
		}
	//clean up
		delete(currentState);
	return 0;
}

EngineState::EngineState(){
	curCommand = 0;
	nextCacheID = 0;
}

EngineState::~EngineState(){
	for(unsigned int i = 0; i<killFunctions.size(); i++){
		killFunctions[i](this);
	}
	for(std::map<int,CacheResource*>::iterator curEl = cache.begin(); curEl!=cache.end(); curEl++){
		CacheResource* curCache = curEl->second;
		delete(curCache);
	}
}

/**
 * Sorts cache resources based on age.
 * @param itemA The first item to compare.
 * @param itemB The second item to compare.
 * @return Whether itemA should be before itemB.
 */
bool cacheSortMeth(CacheResource* itemA, CacheResource* itemB){
	return itemA->lastUseCommand < itemB->lastUseCommand;
}

int EngineState::runCommand(const char* comRun){
	//get the command
	const char* actStart = comRun + strspn(comRun, WHITESPACE);
	const char* actEnd = strpbrk(actStart, WHITESPACE);
	if(!actEnd){
		actEnd = actStart + strlen(actStart);
	}
	if(actEnd == actStart){
		return 0;
	}
	char* comTok = (char*)malloc((actEnd-actStart)+1);
	memcpy(comTok, actStart, (actEnd-actStart));
	comTok[actEnd-actStart] = 0;
	std::map<std::string,EngineCommand*>::iterator indInd = liveFunctions.find(comTok);
	if(indInd == liveFunctions.end()){
		std::cerr << "No command " << comTok << " found." << std::endl;
		free(comTok);
		return 1;
	}
	free(comTok);
	EngineCommand* toRun = indInd->second;
	//parse the command
	int wasProb;
	void* comParse = toRun->parseCommand(actEnd);
	if(comParse){
		//run the command
		wasProb = toRun->runCommand(comParse, this);
		toRun->freeParsed(comParse);
	}
	else{
		wasProb = 1;
	}
	//if the cache is too big, kill old things
	uintptr_t* maxRamState = (uintptr_t*)(state[STATEID_MAXRAM]);
	uintptr_t maxRam = *maxRamState;
	uintptr_t totCache = 0;
	for(std::map<int,CacheResource*>::iterator curEl = cache.begin(); curEl!=cache.end(); curEl++){
		totCache += curEl->second->infoSize;
	}
	if(totCache > maxRam){
		//TODO
	}
	curCommand++;
	return wasProb;
}

EngineCommand::~EngineCommand(){}

const char* getNextOptionalArgument(const char* inString, char** storeLoc){
	const char* startName = inString + strspn(inString, WHITESPACE);
	const char* endName = strpbrk(startName, WHITESPACE);
	if(!endName){
		endName = startName + strlen(startName);
	}
	if(endName - startName){
		char* toRet = (char*)malloc((endName-startName)+1);
		memcpy(toRet, startName, endName-startName);
		toRet[endName-startName] = 0;
		*storeLoc = toRet;
	}
	else{
		*storeLoc = 0;
	}
	return endName;
}

void fixArgumentFilename(char* toFix){
	char* curFoc = strchr(toFix, '|');
	while(curFoc){
		*curFoc = ' ';
		curFoc = strchr(curFoc+1, '|');
	}
}
