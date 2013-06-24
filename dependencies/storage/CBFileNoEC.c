//
//  CBFileNoEC.c
//  cbitcoin
//
//  Created by Matthew Mitchell on 06/01/2013.
//  Copyright (c) 2012 Matthew Mitchell
//
//  This file is part of cbitcoin. It is subject to the license terms
//  in the LICENSE file found in the top-level directory of this
//  distribution and at http://www.cbitcoin.com/license.html. No part of
//  cbitcoin, including this file, may be copied, modified, propagated,
//  or distributed except according to the terms contained in the
//  LICENSE file.

//  SEE HEADER FILE FOR DOCUMENTATION

#include "CBFileEC.h"

bool CBFileAppend(CBDepObject file, uint8_t * data, uint32_t dataLen){
	FILE * fileObj = file.ptr;
	long pos = ftell(fileObj);
	if (fseek(fileObj, 0, SEEK_END)
		|| fwrite(data, 1, dataLen, fileObj) != dataLen
		|| fseek(fileObj, pos, SEEK_SET))
		return false;
	return true;
}
bool CBFileAppendZeros(CBDepObject file, uint32_t amount){
	FILE * fileObj = file.ptr;
	long pos = ftell(fileObj);
	if (fseek(fileObj, 0, SEEK_END))
		return false;
	uint8_t zero = 0;
	for (uint32_t x = 0; x < amount; x++)
		if (fwrite(&zero, 1, 1, fileObj) != 1)
			return false;
	if (fseek(fileObj, pos, SEEK_SET))
		return false;
	return true;
}
void CBFileClose(uint64_t file){
	fclose((FILE *)file);
}
bool CBFileGetLength(uint64_t file, uint32_t * length){
	FILE * fileObj = (FILE *)file;
	long pos = ftell(fileObj);
	if (fseek(fileObj, 0, SEEK_END))
		return false;
	*length = (uint32_t)ftell(fileObj);
	if (fseek(fileObj, pos, SEEK_SET))
		return false;
	return true;
}
uint64_t CBFileOpen(char * filename, bool new){
	FILE * fileObj = fopen(filename, new ? "wb+" : "rb+");
	// Set F_FULLFSYNC
	// F_FULLFSYNC will ensure writes are stored on disk in-order. It is not necessarily important that writes are written immediately but they must be written in order to avoid data corruption. Unfortunately this makes IO operations extremely slow. ??? How to ensure in-order disk writes on other systems?
#ifdef F_FULLFSYNC
	if (fcntl(fileno(fileObj), F_FULLFSYNC)){
		CBFileClose((uint64_t)fileObj);
		return false;
	}
#endif
	return (uint64_t)fileObj;
}
bool CBFileOverwrite(uint64_t file, uint8_t * data, uint32_t dataLen){
	return fwrite(data, 1, dataLen, (FILE *)file) == dataLen;
}
bool CBFileRead(uint64_t file, uint8_t * data, uint32_t dataLen){
	return fread(data, 1, dataLen, (FILE *)file) == dataLen;
}
bool CBFileSeek(uint64_t file, uint32_t pos){
	return NOT fseek((FILE *)file, pos, SEEK_SET);
}
bool CBFileSync(uint64_t file){
	FILE * fileObj = (FILE *)file;
	if (fflush(fileObj))
		return false;
	if (fsync(fileno(fileObj)))
		return false;
	return true;
}
bool CBFileSyncDir(char * dir){
	int dird = open(dir, 0);
	if (dird == -1)
		return false;
	if (fsync(dird)
		// If errno is EINVAL then it is likely the filesystem doesn't support directory synchronisation or does not need it. ???
		&& errno != EINVAL){
		close(dird);
		return false;
	}
	close(dird);
	return true;
}
bool CBFileTruncate(char * filename, uint32_t newSize){
	return NOT truncate(filename, newSize);
}
