#ifndef PROTENGINE_COMPRESS_H
#define PROTENGINE_COMPRESS_H 1

#include "whodun_compress.h"

/**This will use gzip compression.*/
class GZipCompressionMethod : public CompressionMethod{
public:
	/**Deconstruct*/
	~GZipCompressionMethod();
	
	void* comfopen(const char* fileName, const char* mode);
	size_t comfwrite(const void* ptr, size_t size, size_t count, void* stream);
	size_t comfread(void* ptr, size_t size, size_t count, void* stream);
	intptr_t comftell(void* stream);
	int comfclose(void* stream);
	uintptr_t* decompressData(uintptr_t datLen, char* data);
	uintptr_t* compressData(uintptr_t datLen, char* data);
};

#endif
