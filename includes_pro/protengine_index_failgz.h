#ifndef PROTENGINE_INDEX_FAILGZ_H
#define PROTENGINE_INDEX_FAILGZ_H 1

#include <vector>

#include "whodun_compress.h"

/**The size of file pointers.*/
#define FPSZ 8

/**
 * A mask for integers too large to fit in a type.
 * @param watchTp The name of a type to consider.
 * @return A mask: bitwise and to check against overflow.
 */
#define MAX_MASK(watchTp) (((uintmax_t)-1) << (8*sizeof(watchTp)-1))

/**
 * Builds an indexible fasta file from a fastq file.
 * @param fqFileName The name of the fastq file.
 * @param failFileName The name of the indexible fastq file.
 * @param useComp The compression method to use.
 * @return Whether there was a problem.
 */
int buildFailgzFromFastAQ(const char* fqFileName, const char* failFileName, CompressionMethod* useComp);

/**Arguments to a function/*/
typedef struct{
	/**The buffer to store to (or compress and write). Is both in and out.*/
	char* storeBuffer;
	/**The number of bytes allocated for storeBuffer. Is both in and out.*/
	uintptr_t storeAlloc;
	/**The result length. Is out only.*/
	uintptr_t resultLen;
	/**A temporary decompression buffer. Is both in and out.*/
	char* decomBuffer;
	/**The number of bytes allocated for decomBuffer. Is both in and out.*/
	uintptr_t decomAlloc;
	/**Whether the read thing should be required to be null terminated.*/
	bool requireNullTerm;
} ReadCompressedAtomSpec;

/**
 * This will read a compressed block from a file.
 * @param toRead The file to read.
 * @param useComp The compression method to use.
 * @param workSpace The buffers to work with (and possibly replace).
 * @return Whether there was a problem.
 */
int readCompressedBlockAtom(FILE* toRead, CompressionMethod* useComp, ReadCompressedAtomSpec* workSpace);

/**Reads a failgz file.*/
class FailGZReader{
public:
	/**
	 * Opens a failgz file for parsing.
	 * @param failFileName The filename to open.
	 * @param compMeth The compression method to use.
	 */
	FailGZReader(const char* failFileName, CompressionMethod* compMeth);
	/**
	 * Cleans up.
	 */
	~FailGZReader();
	/**
	 * Loads an entry from the file.
	 * @param entryID The index of the entry.
	 * @return Whether there was a problem loading.
	 */
	int loadInEntry(uintptr_t entryID);
	/**The number of things in the file. Negative one if error.*/
	intptr_t numEntries;
	/**The length of the last read name, including the null.*/
	uintptr_t nameLen;
	/**The last read name.*/
	char* name;
	/**The length of the last read sequence, including the null.*/
	uintptr_t seqLen;
	/**The last read sequence.*/
	char* seq;
	/**The length of the last read quality, including the null.*/
	uintptr_t qualLen;
	/**The last read quality.*/
	char* qual;
private:
	/**The file to read through.*/
	FILE* actualFile;
	/**The compression/decompression algorithm.*/
	CompressionMethod* useComp;
	/**THe things in the file.*/
	intptr_t* allOffsets;
	/**The space presently allocated for the docompression buffer.*/
	uintptr_t decomAlloc;
	/**The decompression buffer.*/
	char* decomBuffer;
	/**The number of characters allocated for the name.*/
	uintptr_t nameAlloc;
	/**The number of characters allocated for the sequence.*/
	uintptr_t seqAlloc;
	/**The number of characters allocated for the quality.*/
	uintptr_t qualAlloc;
};

/**Used to alert when a sequence is found.*/
class SequenceFindCallback{
public:
	/**
	 * Called when a sequence is found.
	 * @param fndSeq The found sequence index.
	 * @param stringIndex The index of the string.
	 * @param suffixIndex The location in that string.
	 */
	virtual int foundSequence(uintptr_t fndSeq, uintptr_t stringIndex, uintptr_t suffixIndex) = 0;
	/**Let subclasses destruct properly.*/
	virtual ~SequenceFindCallback();
};

/**
 * This will find a sequence in a failgz file.
 * @param failFile The failgz file.
 * @param findSeqs The sequences to look for.
 * @param findSeqInd The index of the first sequence (assumed sequential).
 * @param toReport The place to put all the found locations.
 * @return Whether there was a problem.
 */
int findSequencesInFailGz(FailGZReader* failFile, std::vector<const char*>* findSeqs, uintptr_t findSeqInd, SequenceFindCallback* toReport);

#endif