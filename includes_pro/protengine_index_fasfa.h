#ifndef PROTENGINE_INDEX_FASFA_H
#define PROTENGINE_INDEX_FASFA_H 1

#include <vector>

#include "whodun_compress.h"
#include "protengine_index_failgz.h"

/**The number of characters to take before splitting up into blocks.*/
#define FASFA_BLOCK_SIZE 1000

/**
 * This will build a fasfa file from multiple source files..
 * @param allSrc THe starting files.
 * @param targetName THe name of the file to produce.
 * @param temporaryFolder The folder to put temporary files in.
 * @param numThread THe number of threads to use.
 * @param maxByteLoad The number of bytes to load.
 * @param useComp The compression method to use.
 * @return Whether there was an exception.
 */
int buildComboFileFromFailgz(std::vector<FailGZReader*>* allSrc, const char* targetName, const char* temporaryFolder, int numThread, uintptr_t maxByteLoad, CompressionMethod* useComp);

/**Will read a fasfa file.*/
class FasfaReader{
public:
	/**
	 * Opens up a fasfa file.
	 * @param fasfaFileName The name of the fasfa file.
	 * @param compMeth The compression method to use.
	 */
	FasfaReader(const char* fasfaFileName, CompressionMethod* compMeth);
	/**
	 * Cleans up.
	 */
	~FasfaReader();
	/**
	 * Loads in the data for a sequence.
	 * @param seqID The sequence index.
	 * @return Whether there was a problem loading.
	 */
	int loadInSequence(uintptr_t seqID);
	/**The number of sequences in the current entry.*/
	intptr_t numSequences;
	/**The number of items in the current sequence (each item is two entries).*/
	uintptr_t numInCurrent;
	/**The current sequence.*/
	uintptr_t* currentSequence;
private:
	/**The file to read through.*/
	FILE* actualFile;
	/**The compression/decompression algorithm.*/
	CompressionMethod* useComp;
	/**THe currently loaded sequence offsets in the file.*/
	intptr_t sequenceOffsets[FASFA_BLOCK_SIZE];
	/**The currently loaded sequence low ind.*/
	intptr_t seqOffLowInd;
	/**The currently loaded sequence high ind.*/
	intptr_t seqOffHigInd;
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
 * This will find a sequence in a fasfa file.
 * @param failFile The failgz files used to build the fassa.
 * @param fasfaFile The fasfa file in question.
 * @param findSeqs The sequences to look for.
 * @param findSeqInd The index of the first sequence.
 * @param toReport The place to put all the found locations.
 * @return Whether there was a problem.
 */
int findSequencesInFasfa(std::vector<FailGZReader*>* failFile, FasfaReader* fassaFile, std::vector<const char*>* findSeqs, uintptr_t findSeqInd, SequenceFindCallback* toReport);

#endif