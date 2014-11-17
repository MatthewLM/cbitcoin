//
//  CBFileNoEC.c
//  cbitcoin
//
//  Created by Matthew Mitchell on 06/01/2013.
//  Copyright (c) 2013 Matthew Mitchell
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
	long pos = ftell(file.ptr);
	fseek(file.ptr, 0, SEEK_END);
	if (fwrite(data, 1, dataLen, file.ptr) != dataLen
		|| fseek(file.ptr, pos, SEEK_SET))
		return false;
	return true;
}
bool CBFileAppendZeros(CBDepObject file, uint32_t amount){
	long pos = ftell(file.ptr);
	if (fseek(file.ptr, 0, SEEK_END))
		return false;
	uint8_t zero = 0;
	for (uint32_t x = 0; x < amount; x++)
		if (fwrite(&zero, 1, 1, file.ptr) != 1)
			return false;
	if (fseek(file.ptr, pos, SEEK_SET))
		return false;
	return true;
}
void CBFileClose(CBDepObject file){
	fclose(file.ptr);
}
bool CBFileGetLength(CBDepObject file, uint32_t * length){
	long pos = ftell(file.ptr);
	if (fseek(file.ptr, 0, SEEK_END))
		return false;
	*length = (uint32_t)ftell(file.ptr);
	if (fseek(file.ptr, pos, SEEK_SET))
		return false;
	return true;
}
bool CBFileOpen(CBDepObject * file, char * filename, bool new){
	file->ptr = fopen(filename, new ? "wb+" : "rb+");
	return file->ptr != NULL;
}
bool CBFileOverwrite(CBDepObject file, uint8_t * data, uint32_t dataLen){
	return fwrite(data, 1, dataLen, file.ptr) == dataLen;
}
uint32_t CBFilePos(CBDepObject file){
	return (uint32_t)ftell(file.ptr);
}
bool CBFileRead(CBDepObject file, uint8_t * data, uint32_t dataLen){
	return fread(data, 1, dataLen, file.ptr) == dataLen;
}
bool CBFileSeek(CBDepObject file, int32_t pos, int seek){
	return ! fseek(file.ptr, pos, seek);
}
bool CBFileSync(CBDepObject file){
	if (fflush(file.ptr))
		return false;
	if (fsync(fileno(file.ptr)))
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
	return ! truncate(filename, newSize);
}
