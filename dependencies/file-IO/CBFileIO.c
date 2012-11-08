//
//  CBFileIO.c
//  cbitcoin
//
//  Created by Matthew Mitchell on 03/11/2012.
//  Copyright (c) 2012 Matthew Mitchell
//
//  This file is part of cbitcoin.
//
//  cbitcoin is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  cbitcoin is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with cbitcoin.  If not, see <http://www.gnu.org/licenses/>.

// Includes

#include "CBSafeOutputDependencies.h"
#include "CBConstants.h"
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/resource.h>

// Implementation

uint64_t CBFileOpen(char * filename, CBFileMode mode){
	switch (mode) {
		case CB_FILE_MODE_APPEND:
			return open(filename, O_APPEND | O_WRONLY);
		case CB_FILE_MODE_NONE:
			return open(filename, 0);
		case CB_FILE_MODE_OVERWRITE:
			return open(filename, O_WRONLY);
		case CB_FILE_MODE_READ:
			return open(filename, O_RDONLY);
		case CB_FILE_MODE_SAVE:
			return open(filename, O_WRONLY | O_TRUNC | O_CREAT);
		case CB_FILE_MODE_WRITE_AND_READ:
			return open(filename, O_RDWR | O_TRUNC | O_CREAT);
		case CB_FILE_MODE_READ_AND_UPDATE:
			return open(filename, O_RDWR);
	}
}
bool CBFileRead(uint64_t file, void * buffer, size_t size){
	return read((int) file, buffer, size) == size;
}
bool CBFileWrite(uint64_t file, void * buffer, size_t size){
	return write((int) file, buffer, size) == size;
}
bool CBFileSeek(uint64_t file, long int offset, CBFileSeekOrigin origin){
	return lseek((int) file, offset, (origin == CB_SEEK_SET) ? SEEK_SET : SEEK_CUR) != -1;
}
bool CBFileRename(void * filename, void * newname){
	return NOT rename(filename, newname);
}
bool CBFileTruncate(char * filename, size_t size){
	return NOT truncate(filename, size);
}
bool CBFileDelete(void * filename){
	return NOT remove(filename);
}
size_t CBFileGetSize(void * filename){
	struct stat stbuf;
	if (stat(filename, &stbuf))
		return -1;
	return stbuf.st_size;
}
size_t CBGetMaxFileSize(void){
	struct rlimit fileLim;
	if(getrlimit(RLIMIT_FSIZE, &fileLim))
		return -1;
	return fileLim.rlim_cur;
}
bool CBFileDiskSynchronise(uint64_t file){
	#ifdef F_FULLFSYNC
		// Use full sync to force synchronisation to the permanent storage on supported systems.
		return NOT fcntl((int) file, F_FULLFSYNC);
	#else
		return NOT fsync((int) file);
	#endif
}
bool CBFileExists(char * filename){
	return NOT access(filename, F_OK);
}
void CBFileClose(uint64_t file){
	close((int) file);
}
