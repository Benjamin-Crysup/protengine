#ifndef PROTENGINE_INDEX_FASSA_H
#define PROTENGINE_INDEX_FASSA_H 1

#include <vector>

#include "whodun_compress.h"
#include "protengine_index_failgz.h"

/**The number of characters to take before splitting up into blocks.*/
#define FASSSA_BLOCK_SIZE 1000

/**
 * This will build a fasssa file from a failgz.
 * @param failFile THe fail file.
 * @param fassaFileName The name of the fasssa file.
 * @param useComp The compression method to use.
 * @return Whether there was a problem.
 */
int buildFassaFromFailgz(FailGZReader* failFile, const char* fassaFileName, CompressionMethod* useComp);

/**Reads a fassa file.*/
class FassaReader{
public:
	/**
	 * Opens a fassa file for parsing.
	 * @param fassaFileName The filename to open.
	 * @param compMeth The compression method to use.
	 */
	FassaReader(const char* fassaFileName, CompressionMethod* compMeth);
	/**
	 * Cleans up.
	 */
	~FassaReader();
	/**
	 * Loads in the data for an entry. An entry is a collection of sequences.
	 * @param entryID The entry index.
	 * @return Whether there was a problem loading.
	 */
	int loadInEntry(uintptr_t entryID);
	/**
	 * Loads in the data for a sequence.
	 * @param seqID The sequence index.
	 * @return Whether there was a problem loading.
	 */
	int loadInSequence(uintptr_t seqID);
	/**The number of things in the file. Negative one if error.*/
	intptr_t numEntries;
	/**The number of sequences in the current entry.*/
	uintptr_t numSequences;
	/**The number of items in the current sequence.*/
	uintptr_t numInCurrent;
	/**The current sequence.*/
	uintptr_t* currentSequence;
private:
	/**The file to read through.*/
	FILE* actualFile;
	/**The compression/decompression algorithm.*/
	CompressionMethod* useComp;
	/**THe entry offsets in the file.*/
	intptr_t* entryOffsets;
	/**THe sequence offsets in the file.*/
	intptr_t* sequenceOffsets;
	/**The number of bytes allocated for sequenceOffsets.*/
	uintptr_t seqOffAlloc;
	/**The space presently allocated for the docompression buffer.*/
	uintptr_t decomAlloc;
	/**The decompression buffer.*/
	char* decomBuffer;
	/**The file pointer buffer.*/
	char* filptrBuffer;
	/**The number of items allocated for the current sequence.*/
	uintptr_t currentAlloc;
};

/**
 * This will find a sequence in a fassa file.
 * @param failFile The failgz file used to build the fassa.
 * @param fassaFile The fassa file in question.
 * @param findSeqs The sequences to look for.
 * @param findSeqInd The index of the first sequence.
 * @param toReport The place to put all the found locations.
 * @return Whether there was a problem.
 */
int findSequencesInFassa(FailGZReader* failFile, FassaReader* fassaFile, std::vector<const char*>* findSeqs, uintptr_t findSeqInd, SequenceFindCallback* toReport);

#endif