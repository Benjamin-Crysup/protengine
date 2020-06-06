#ifndef PROTENGINE_ENGINE_INDEX_H
#define PROTENGINE_ENGINE_INDEX_H 1

#include <map>
#include <vector>
#include <stdint.h>

#include "protengine_engine.h"

/**The command string for initializing an index.*/
#define COMMAND_INITINDEX "IndexInit"
/**The command string for adding a fasta/q file to an index.*/
#define COMMAND_LOADINDEX "IndexReference"

/**The command to search for some sequences in a reference.*/
#define COMMAND_FINDFASTA "FindRawSearch"

/**The command string for building individual suffix arrays for the entries in a fastagzs index file.*/
#define COMMAND_SUFFARRINDEX "BuildSAPrecom"
/**The command string for finding (exact match) items in a fasta file in an index. Can also limit to a single reference.*/
#define COMMAND_FINDSINGLESTRING "FindSAPrecom"

/**The command string for building a collected suffix array for multiple failgz files.*/
#define COMMAND_COMBOSUFFARRBUILD "BuildComboPrecom"
/**The command string for searching a collected suffix array.*/
#define COMMAND_COMBOSUFFARRSEARCH "FindComboPrecom"

/**The state id for the indices.*/
#define STATEID_INDICES "prot_indices"

/**
 * Adds the base commands.
 * @param toAddTo THe command state to add to.
 */
void addIndexCommands(EngineState* toAddTo);

/**Sets up an index.*/
class InitIndexCommand : public EngineCommand{
public:
	~InitIndexCommand();
	void* parseCommand(const char* toParse);
	int runCommand(void* toRun, EngineState* forState);
	void freeParsed(void* parseDat);
	const char* getCommandHelp();
};

/**Adds a reference to an index.*/
class IndexAddReferenceCommand : public EngineCommand{
public:
	~IndexAddReferenceCommand();
	void* parseCommand(const char* toParse);
	int runCommand(void* toRun, EngineState* forState);
	void freeParsed(void* parseDat);
	const char* getCommandHelp();
};

/**Does a direct search.*/
class RawSearchCommand : public EngineCommand{
public:
	~RawSearchCommand();
	void* parseCommand(const char* toParse);
	int runCommand(void* toRun, EngineState* forState);
	void freeParsed(void* parseDat);
	const char* getCommandHelp();
};

/**Precomputes single string suffix arrays..*/
class PrecomputeSACommand : public EngineCommand{
public:
	~PrecomputeSACommand();
	void* parseCommand(const char* toParse);
	int runCommand(void* toRun, EngineState* forState);
	void freeParsed(void* parseDat);
	const char* getCommandHelp();
};

/**Does a search using precomputed suffix arrays.*/
class PrecomSASearchCommand : public EngineCommand{
public:
	~PrecomSASearchCommand();
	void* parseCommand(const char* toParse);
	int runCommand(void* toRun, EngineState* forState);
	void freeParsed(void* parseDat);
	const char* getCommandHelp();
};

/**Precomputes multi string suffix arrays..*/
class PrecomputeComboCommand : public EngineCommand{
public:
	~PrecomputeComboCommand();
	void* parseCommand(const char* toParse);
	int runCommand(void* toRun, EngineState* forState);
	void freeParsed(void* parseDat);
	const char* getCommandHelp();
};

/**Does a search using precomputed multistring suffix arrays.*/
class PrecomComboSearchCommand : public EngineCommand{
public:
	~PrecomComboSearchCommand();
	void* parseCommand(const char* toParse);
	int runCommand(void* toRun, EngineState* forState);
	void freeParsed(void* parseDat);
	const char* getCommandHelp();
};

#endif