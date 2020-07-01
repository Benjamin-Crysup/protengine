#include "protengine_compress.h"

#include <stdlib.h>
#include <string.h>
#include <iostream>

#define PIECE_LEN_BYTES 4
#define BUFFER_LEN 0x080000

#include <zlib.h>
//#include <zstd.h>

GZipCompressionMethod::~GZipCompressionMethod(){}
	
void* GZipCompressionMethod::comfopen(const char* fileName, const char* mode){
	gzFile* toRet = (gzFile*)malloc(sizeof(gzFile));
	*toRet = gzopen(fileName, mode);
	if(*toRet == 0){
		free(toRet);
		return 0;
	}
	return toRet;
}

size_t GZipCompressionMethod::comfwrite(const void* ptr, size_t size, size_t count, void* stream){
	gzFile* toRet = (gzFile*)stream;
	size_t numWrite = gzwrite(*toRet, ptr, size*count);
	return numWrite / size;
}

size_t GZipCompressionMethod::comfread(void* ptr, size_t size, size_t count, void* stream){
	gzFile* toRet = (gzFile*)stream;
	size_t numRead = gzread(*toRet, ptr, size*count);
	return numRead / size;
}

intptr_t GZipCompressionMethod::comftell(void* stream){
	gzFile* toRet = (gzFile*)stream;
	return gztell(*toRet);
}

int GZipCompressionMethod::comfclose(void* stream){
	gzFile* toRet = (gzFile*)stream;
	int retI = gzclose(*toRet);
	free(toRet);
	return retI;
}

uintptr_t* GZipCompressionMethod::decompressData(uintptr_t datLen, char* data){
	uintptr_t destBuffLen = datLen ? datLen : 1;
	uintptr_t* lenStore = (uintptr_t*)malloc(destBuffLen + sizeof(uintptr_t));
	unsigned char* datStore = (unsigned char*)(lenStore+1);
	unsigned long destBuffLenEnd = destBuffLen;
	int compRes = 0;
	while((compRes = uncompress(datStore, &destBuffLenEnd, (const unsigned char*)data, datLen)) != Z_OK){
		free(lenStore);
		if((compRes == Z_MEM_ERROR) || (compRes == Z_DATA_ERROR)){
			return 0;
		}
		destBuffLen = destBuffLen << 1;
		lenStore = (uintptr_t*)malloc(destBuffLen + sizeof(uintptr_t));
		datStore = (unsigned char*)(lenStore+1);
		destBuffLenEnd = destBuffLen;
	}
	*lenStore = destBuffLenEnd;
	return lenStore;
}

uintptr_t* GZipCompressionMethod::compressData(uintptr_t datLen, char* data){
	uintptr_t destBuffLen = datLen ? datLen : 1;
	uintptr_t* lenStore = (uintptr_t*)malloc(destBuffLen + sizeof(uintptr_t));
	unsigned char* datStore = (unsigned char*)(lenStore+1);
	unsigned long destBuffLenEnd = destBuffLen;
	int compRes = 0;
	while((compRes = compress(datStore, &destBuffLenEnd, (const unsigned char*)data, datLen)) != Z_OK){
		free(lenStore);
		if(compRes == Z_MEM_ERROR){
			return 0;
		}
		destBuffLen = destBuffLen << 1;
		lenStore = (uintptr_t*)malloc(destBuffLen + sizeof(uintptr_t));
		datStore = (unsigned char*)(lenStore+1);
		destBuffLenEnd = destBuffLen;
	}
	*lenStore = destBuffLenEnd;
	return lenStore;
}

