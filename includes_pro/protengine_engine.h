#ifndef PROTENGINE_ENGINE_H
#define PROTENGINE_ENGINE_H 1

#include <map>
#include <vector>
#include <string>
#include <stdint.h>

/**The state id for the maximum ram.*/
#define STATEID_MAXRAM "engine_maxram"
/**The state id for the number of threads*/
#define STATEID_THREAD "engine_thread"

/**A cached resource.*/
class CacheResource{
public:
	/**Default constructor.*/
	CacheResource();
	/**Kills this resource.*/
	virtual ~CacheResource();
	
	/**The ID in the cache.*/
	int cacheID;
	/**The command this was last used for.*/
	int lastUseCommand;
	/**The size of said info.*/
	uintptr_t infoSize;
};

class EngineState;

/**A command to the engine.*/
class EngineCommand{
public:
	/**Clean tear down.*/
	virtual ~EngineCommand();
	/**
	 * This will parse the command for future use.
	 * @param toParse The command to parse, minus the name of this command.
	 * @return The parsed command, ready to run.
	 */
	virtual void* parseCommand(const char* toParse) = 0;
	/**
	 * This will run a parsed command.
	 * @param toRun The command to run. Will be consumed.
	 * @param forState The state of the engine.
	 * @return Whether there was a problem.
	 */
	virtual int runCommand(void* toRun, EngineState* forState) = 0;
	/**
	 * This will free previously parsed data.
	 * @param parseDat The parsed data.
	 */
	virtual void freeParsed(void* parseDat) = 0;
	/**
	 * Gets the help string for this command.
	 * @return The documentation.
	 */
	virtual const char* getCommandHelp() = 0;
};

/**The current state of the engine.*/
class EngineState{
public:
	/**Sets up an empty state.*/
	EngineState();
	/**Kills any remaining cache, and the starting state.*/
	~EngineState();
	/**
	 * This will parse and run a command.
	 * @param comRun The command to run.
	 * @return Whether there was a problem running the command.
	 */
	int runCommand(const char* comRun);
	
	/**The functions this thing can do.*/
	std::map<std::string,EngineCommand*> liveFunctions;
	/**Functions to call before dying.*/
	std::vector<void (*)(EngineState*)> killFunctions;
	/**The cached resources, by id.*/
	std::map<int, CacheResource*> cache;
	/**Current state.*/
	std::map<std::string,void*> state;
	/**The current command.*/
	int curCommand;
	/**The next cache id.*/
	int nextCacheID;
};

/**
 * This will get the next argument.
 * @param inString The string to search through.
 * @param storeLoc The palce to put the allocated argument.
 * @return The character after the next argument.
 */
const char* getNextOptionalArgument(const char* inString, char** storeLoc);

/**
 * Replaces all pipes in a filename with spaces.
 * @param toFix The filename to fix.
 */
 void fixArgumentFilename(char* toFix);

#endif