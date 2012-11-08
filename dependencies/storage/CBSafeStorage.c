//
//  CBSafeStorage.c
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

//  SEE HEADER FILE FOR DOCUMENTATION

#include "CBSafeStorage.h"

uint64_t CBNewBlockChainStorage(char * dataDir, void (*logError)(char *,...)){
	// Try to create the object
	uint64_t self = malloc(sizeof(*self));
	if (NOT self){
		logError("Could not allocate memory for the CBSafeStorage object.");
		return 0;
	}
	// Get maximum file size
	struct rlimit fileLim;
	if(getrlimit(RLIMIT_FSIZE,&fileLim)){
		logError("Could not get RLIMIT_FSIZE limits.");
		return 0;
	}
	self->fileSizeLimit = fileLim.rlim_cur;
	self->dataDir = dataDir;
	// Open backup file for reading
	char backupFile[strlen(self->dataDir) + 11];
	sprintf(backupFile, "%sbackup.bak", self->dataDir);
	// Check to see if the backup file exists already
	if (NOT access(backupFile, F_OK)) {
		// Exists. Open file and check recovery if needed.
		FILE * file = CBFileOpen(backupFile, CB_FILE_MODE_READ_AND_UPDATE);
		if (NOT file) {
			logError("Could not open backup file for reading and writting in CBInitFullValidator.");
			return 0;
		}
		if (NOT CBSafeOutputRecover(file)) {
			fclose(file);
			logError("Data recovery failure in CBInitFullValidator.");
			return 0;
		}
		fclose(file);
	}
	// Now open backup file in wb+ mode.
	self->backup = CBFileOpen(backupFile, CB_FILE_MODE_WRITE_AND_READ);
	if (NOT self->backup) {
		logError("Could not open backup file for writting and reading in CBInitFullValidator.");
		return 0;
	}
	return self;
}

void CBFreeBlockChainStorage(uint64_t self){
	CBFileClose(self->backup);
}

bool CBBlockChainStorageWriteBlock(uint64_t self, CBBlock * block, uint32_t blockID, uint8_t branchID){
	// Save block. First find first block file with space.
	uint16_t fileIndex = 0;
	char blockFile[strlen(self->dataDir) + 22];
	// Use stat.h for fast information
	sprintf(blockFile, "%sblocks%u-%u.dat",self->dataDir, branch, self->branches[branch].numBlockFiles - 1);
	struct stat st;
	if(stat(blockFile, &st))
		return CB_ADD_BLOCK_FAIL_PREVIOUS;
	uint64_t size = st.st_size;
	bool newFile = false;
	if (CBGetMessage(block)->bytes->length + 4 > self->fileSizeLimit - size){
		// Not enough room in this file, so use a new file
		sprintf(blockFile, "%sblocks%u-%u.dat",self->dataDir, branch, self->branches[branch].numBlockFiles);
		newFile = true;
	}
	// Prepare IO
	if (NOT CBSafeOutputAlloc(self->output, 2))
		return CB_ADD_BLOCK_FAIL_PREVIOUS;
	if (NOT CBSafeOutputAddFile(self->output, blockFile, 0, 2))
		return CB_ADD_BLOCK_FAIL_PREVIOUS;
}
