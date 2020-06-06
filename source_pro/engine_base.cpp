#include "protengine_engine.h"
#include "protengine_engine_base.h"

#include <map>
#include <string>
#include <iostream>
#include <stdlib.h>

/**
 * Cleans up basic information.
 * @param toKill The thing to remove from.
 */
void killBaseCommands(EngineState* toKill){
	uintptr_t* maxRamState = (uintptr_t*)(toKill->state[STATEID_MAXRAM]);
	free(maxRamState);
	uintptr_t* thrCntState = (uintptr_t*)(toKill->state[STATEID_THREAD]);
	free(thrCntState);
	delete(toKill->liveFunctions[COMMAND_MAXRAM]);
	delete(toKill->liveFunctions[COMMAND_PRINTCOMS]);
	delete(toKill->liveFunctions[COMMAND_THREAD]);
}

void addBaseCommands(EngineState* toAddTo){
	toAddTo->liveFunctions[COMMAND_MAXRAM] = new MaxramCommand();
	toAddTo->liveFunctions[COMMAND_PRINTCOMS] = new HelpCommand();
	toAddTo->liveFunctions[COMMAND_THREAD] = new ThreadCommand();
	toAddTo->killFunctions.push_back(killBaseCommands);
	//add a max ram function, and a target for such
	uintptr_t* maxRamState = (uintptr_t*)malloc(sizeof(uintptr_t));
	*maxRamState = 1<<14;
	toAddTo->state[STATEID_MAXRAM] = maxRamState;
	uintptr_t* thrCntState = (uintptr_t*)malloc(sizeof(uintptr_t));
	*thrCntState = 1;
	toAddTo->state[STATEID_THREAD] = thrCntState;
}

MaxramCommand::~MaxramCommand(){}
void* MaxramCommand::parseCommand(const char* toParse){
	uintptr_t* newMRStore = (uintptr_t*)malloc(sizeof(uintptr_t));
	*newMRStore = atol(toParse);
	return newMRStore;
}
int MaxramCommand::runCommand(void* toRun, EngineState* forState){
	uintptr_t* newMRStore = (uintptr_t*)toRun;
	uintptr_t newMaxRam = *newMRStore;
	uintptr_t* maxRamState = (uintptr_t*)(forState->state[STATEID_MAXRAM]);
	*maxRamState = newMaxRam;
	return 0;
}
void MaxramCommand::freeParsed(void* parseDat){
	free(parseDat);
}
const char* MaxramCommand::getCommandHelp(){
	return "MaxRam newAmount\n\tChanges the amount of RAM to use for storing intermediate results.\n\tnewAmount -- The number of bytes to use.\n";
}
HelpCommand::~HelpCommand(){}
void* HelpCommand::parseCommand(const char* toParse){
	return (void*)42;
}
int HelpCommand::runCommand(void* toRun, EngineState* forState){
	std::cout << "Available commands:" << std::endl;
	for(std::map<std::string,EngineCommand*>::iterator curEl = forState->liveFunctions.begin(); curEl != forState->liveFunctions.end(); curEl++){
		std::cout << curEl->second->getCommandHelp() << std::endl;
	}
	return 0;
}
void HelpCommand::freeParsed(void* parseDat){
}
const char* HelpCommand::getCommandHelp(){
	return "Help\n\tPrints help\n";
}
ThreadCommand::~ThreadCommand(){}
void* ThreadCommand::parseCommand(const char* toParse){
	uintptr_t* newMRStore = (uintptr_t*)malloc(sizeof(uintptr_t));
	*newMRStore = atol(toParse);
	return newMRStore;
}
int ThreadCommand::runCommand(void* toRun, EngineState* forState){
	uintptr_t* newMRStore = (uintptr_t*)toRun;
	uintptr_t newMaxRam = *newMRStore;
	uintptr_t* maxRamState = (uintptr_t*)(forState->state[STATEID_THREAD]);
	*maxRamState = newMaxRam;
	return 0;
}
void ThreadCommand::freeParsed(void* parseDat){
	free(parseDat);
}
const char* ThreadCommand::getCommandHelp(){
	return "Thread newNum\n\tChanges the number of threads to use, for options that can use multiple threads.\n\tnewNum -- The new number.\n";
}
