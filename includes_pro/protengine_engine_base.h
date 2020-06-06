#ifndef PROTENGINE_ENGINE_BASE_H
#define PROTENGINE_ENGINE_BASE_H 1

#include <map>
#include <vector>
#include <stdint.h>

#include "protengine_engine.h"

/**The command string for changing maximum ram.*/
#define COMMAND_MAXRAM "MaxRam"
/**The command string for changing number of threads.*/
#define COMMAND_THREAD "Thread"
/**The command string for printing all commands.*/
#define COMMAND_PRINTCOMS "Help"

/**
 * Adds the base commands.
 * @param toAddTo THe command state to add to.
 */
void addBaseCommands(EngineState* toAddTo);

/**Changes the maxram for the cache data.*/
class MaxramCommand : public EngineCommand{
public:
	~MaxramCommand();
	void* parseCommand(const char* toParse);
	int runCommand(void* toRun, EngineState* forState);
	void freeParsed(void* parseDat);
	const char* getCommandHelp();
};

/**Prints help.*/
class HelpCommand : public EngineCommand{
public:
	~HelpCommand();
	void* parseCommand(const char* toParse);
	int runCommand(void* toRun, EngineState* forState);
	void freeParsed(void* parseDat);
	const char* getCommandHelp();
};

class ThreadCommand : public EngineCommand{
public:
	~ThreadCommand();
	void* parseCommand(const char* toParse);
	int runCommand(void* toRun, EngineState* forState);
	void freeParsed(void* parseDat);
	const char* getCommandHelp();
};

#endif