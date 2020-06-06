#ifndef PROTENGINE_ENGINE_INDEX_DUMP_H
#define PROTENGINE_ENGINE_INDEX_DUMP_H 1

#include <map>
#include <vector>
#include <stdint.h>

#include "protengine_engine.h"
#include "protengine_engine_index.h"

/**The command string for dumping a reference to fastq.*/
#define COMMAND_DUMPINDEX "IndexDumpReference"

/**The command string for dumping a precomputed suffix array.*/
#define COMMAND_DUMPSAINDEX "IndexDumpSA"

/**The command string for dumping a precomputed combo.*/
#define COMMAND_DUMPCOMBOINDEX "IndexDumpCombo"

/**The command string for dumping a precomputed combo.*/
#define COMMAND_DUMPCOMBONAMEINDEX "IndexDumpComboNames"

/**The command string for dumping the neighborhood of a search.*/
#define COMMAND_DUMPSEARCHREGINDEX "IndexDumpSearchRegion"

/**
 * Adds the base commands.
 * @param toAddTo THe command state to add to.
 */
void addIndexDumpCommands(EngineState* toAddTo);

/**Dumps a reference.*/
class IndexDumpCommand : public EngineCommand{
public:
	~IndexDumpCommand();
	void* parseCommand(const char* toParse);
	int runCommand(void* toRun, EngineState* forState);
	void freeParsed(void* parseDat);
	const char* getCommandHelp();
};

/**Dumps a reference.*/
class IndexDumpSACommand : public EngineCommand{
public:
	~IndexDumpSACommand();
	void* parseCommand(const char* toParse);
	int runCommand(void* toRun, EngineState* forState);
	void freeParsed(void* parseDat);
	const char* getCommandHelp();
};

/**Dumps a reference.*/
class IndexDumpComboCommand : public EngineCommand{
public:
	~IndexDumpComboCommand();
	void* parseCommand(const char* toParse);
	int runCommand(void* toRun, EngineState* forState);
	void freeParsed(void* parseDat);
	const char* getCommandHelp();
};

/**Dumps the names of the strings in a combo..*/
class IndexDumpComboNamesCommand : public EngineCommand{
public:
	~IndexDumpComboNamesCommand();
	void* parseCommand(const char* toParse);
	int runCommand(void* toRun, EngineState* forState);
	void freeParsed(void* parseDat);
	const char* getCommandHelp();
};

/**Dump the neighboring region of a search result..*/
class IndexDumpSearchRegionCommand : public EngineCommand{
public:
	~IndexDumpSearchRegionCommand();
	void* parseCommand(const char* toParse);
	int runCommand(void* toRun, EngineState* forState);
	void freeParsed(void* parseDat);
	const char* getCommandHelp();
};

#endif