//
//  CBSafeOutput.c
//  cbitcoin-SafeOutput
//
//  Created by Matthew Mitchell on 20/10/2012.
//  Copyright (c) 2012 Matthew Mitchell
//
//  This file is part of cbitcoin-SafeOutput.
//
//  cbitcoin-SafeOutput is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  cbitcoin-SafeOutput is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with cbitcoin-SafeOutput.  If not, see <http://www.gnu.org/licenses/>.

//  SEE HEADER FILE FOR DOCUMENTATION

#include "CBSafeOutput.h"

void CBFreeSafeOutputProcess(CBSafeOutput * self){
	for (uint8_t x = 0; x < self->numFiles; x++)
		if (self->files[x].type == CB_SAFE_OUTPUT_OP_OVERWRITE_APPEND) {
			free(self->files[x].opData.appendAndOverwriteOps.appendOperations);
			free(self->files[x].opData.appendAndOverwriteOps.overwriteOperations);
		}
	free(self->files);
	self->files = NULL;
	self->numFiles = 0;
}

void CBSafeOutputAddAppendOperation(CBSafeOutput * self, void * data, uint32_t size){
	CBAppendAndOverwrite * ops = &self->files[self->numFiles - 1].opData.appendAndOverwriteOps;
	ops->appendOperations[ops->numAppendOperations].data = data;
	ops->appendOperations[ops->numAppendOperations].size = size;
	ops->numAppendOperations++;
}
void CBSafeOutputAddDeleteFileOperation(CBSafeOutput * self, char * fileName){
	self->files[self->numFiles].fileName = fileName;
	self->files[self->numFiles].type = CB_SAFE_OUTPUT_OP_DELETE;
	self->numFiles++;
}
bool CBSafeOutputAddFile(CBSafeOutput * self, char * fileName, uint8_t overwriteOperations, uint8_t appendOperations){
	self->files[self->numFiles].fileName = fileName;
	self->files[self->numFiles].type = CB_SAFE_OUTPUT_OP_OVERWRITE_APPEND;
	// Overwrite operations
	CBAppendAndOverwrite * ops = &self->files[self->numFiles].opData.appendAndOverwriteOps;
	ops->overwriteOperations = malloc(sizeof(*ops->overwriteOperations) * overwriteOperations);
	if (NOT ops->overwriteOperations){
		CBFreeSafeOutputProcess(self);
		return false;
	}
	ops->numOverwriteOperations = 0;
	// Append operations
	ops->appendOperations = malloc(sizeof(*ops->appendOperations) * appendOperations);
	if (NOT ops->appendOperations) {
		CBFreeSafeOutputProcess(self);
		return false;
	}
	ops->numAppendOperations = 0;
	self->numFiles++;
	return true;
}
void CBSafeOutputAddOverwriteOperation(CBSafeOutput * self, long int offset, void * data, uint32_t size){
	CBAppendAndOverwrite * ops = &self->files[self->numFiles - 1].opData.appendAndOverwriteOps;
	ops->overwriteOperations[ops->numOverwriteOperations].offset = offset;
	ops->overwriteOperations[ops->numOverwriteOperations].data = data;
	ops->overwriteOperations[ops->numOverwriteOperations].size = size;
	ops->numOverwriteOperations++;
}
void CBSafeOutputAddRenameFileOperation(CBSafeOutput * self, char * fileName, char * newName){
	self->files[self->numFiles].fileName = fileName;
	self->files[self->numFiles].type = CB_SAFE_OUTPUT_OP_RENAME;
	self->files[self->numFiles].opData.newName = newName;
	self->numFiles++;
}
void CBSafeOutputAddSaveFileOperation(CBSafeOutput * self, char * fileName, void * data, uint32_t size){
	self->files[self->numFiles].fileName = fileName;
	self->files[self->numFiles].type = CB_SAFE_OUTPUT_OP_SAVE;
	self->files[self->numFiles].opData.saveOp.data = data;
	self->files[self->numFiles].opData.saveOp.size = size;
	self->numFiles++;
}
void CBSafeOutputAddTruncateFileOperation(CBSafeOutput * self, char * fileName, uint32_t newsize){
	self->files[self->numFiles].fileName = fileName;
	self->files[self->numFiles].type = CB_SAFE_OUTPUT_OP_TRUNCATE;
	self->files[self->numFiles].opData.newSize = newsize;
	self->numFiles++;
}
bool CBSafeOutputAlloc(CBSafeOutput * self,uint8_t numFiles){
	// Remove any previous data
	CBFreeSafeOutputProcess(self);
	// Allocate new memory ??? Would be more efficient to not reallocate.
	self->files = malloc(sizeof(*self->files) * numFiles);
	return self->files;
}
CBSafeOutputResult CBSafeOutputCommit(CBSafeOutput * self, u_int64_t backup){
	// Write backup information for the files
	if (NOT CBFileSeek(backup, 0, CB_SEEK_SET)){
		CBFreeSafeOutputProcess(self);
		return CB_SAFE_OUTPUT_FAIL_PREVIOUS;
	}
	uint8_t * readData = NULL;
	uint32_t readDataSize = 0;
	uint8_t byte;
	// Ensure backup is not active while making it
	byte = 0;
	if(NOT CBFileWrite(backup, &byte, 1)){
		CBFreeSafeOutputProcess(self);
		return CB_SAFE_OUTPUT_FAIL_PREVIOUS;
	}
	// Write the number of files
	if (NOT CBFileWrite(backup, &self->numFiles, 1)) {
		CBFreeSafeOutputProcess(self);
		return CB_SAFE_OUTPUT_FAIL_PREVIOUS;
	}
	// Go through operations adding transaction log information. The operations are logged in reverse for obvious reasons.
	for (uint8_t x = self->numFiles; x--;) {
		// Write file name size
		byte = strlen(self->files[x].fileName);
		if (NOT CBFileWrite(backup, &byte, 1)) {
			free(readData);
			CBFreeSafeOutputProcess(self);
			return CB_SAFE_OUTPUT_FAIL_PREVIOUS;
		}
		// Write file name
		if (NOT CBFileWrite(backup, self->files[x].fileName, byte)){
			free(readData);
			CBFreeSafeOutputProcess(self);
			return CB_SAFE_OUTPUT_FAIL_PREVIOUS;
		}
		// Write type of operation or operations
		byte = self->files[x].type;
		if (NOT CBFileWrite(backup, &byte, 1)) {
			free(readData);
			CBFreeSafeOutputProcess(self);
			return CB_SAFE_OUTPUT_FAIL_PREVIOUS;
		}
		uint64_t read = 0;
		size_t size;
		if (self->files[x].type == CB_SAFE_OUTPUT_OP_OVERWRITE_APPEND
			|| (self->files[x].type == CB_SAFE_OUTPUT_OP_SAVE
				&& NOT CBFileExists(self->files[x].fileName))) {
			read = CBFileOpen(self->files[x].fileName, CB_FILE_MODE_READ);
			if (NOT read) {
				free(readData);
				CBFreeSafeOutputProcess(self);
				return CB_SAFE_OUTPUT_FAIL_PREVIOUS;
			}
			// Write previous file size (Truncation removes appended data on recovery)
			size = CBFileGetSize(self->files[x].fileName);
			if (size == -1) {
				free(readData);
				CBFileClose(read);
				CBFreeSafeOutputProcess(self);
				return CB_SAFE_OUTPUT_FAIL_PREVIOUS;
			}
			if(NOT CBFileWrite(backup, (uint8_t []){
				size,
				size >> 8,
				size >> 16,
				size >> 24,
			}, 4)){
				free(readData);
				CBFileClose(read);
				CBFreeSafeOutputProcess(self);
				return CB_SAFE_OUTPUT_FAIL_PREVIOUS;
			}
		}
		switch (self->files[x].type) {
			case CB_SAFE_OUTPUT_OP_DELETE:
				// Do nothing more at this stage
				break;
			case CB_SAFE_OUTPUT_OP_TRUNCATE:
				// Write the data being deleted from the end of the file
				// Get the file size to determine the size of the data being removed thorugh truncation
				size = CBFileGetSize(self->files[x].fileName);
				if (size == -1) {
					free(readData);
					CBFileClose(read);
					CBFreeSafeOutputProcess(self);
					return CB_SAFE_OUTPUT_FAIL_PREVIOUS;
				}
				// Take the new size away from the previous size to get the size of the data being removed.
				size -= self->files[x].opData.newSize;
				// Write the size of the data being removed
				if(NOT CBFileWrite(backup, (uint8_t []){
					size,
					size >> 8,
					size >> 16,
					size >> 24,
				}, 4)){
					free(readData);
					CBFileClose(read);
					CBFreeSafeOutputProcess(self);
					return CB_SAFE_OUTPUT_FAIL_PREVIOUS;
				}
				// Read then write the removed data
				read = CBFileOpen(self->files[x].fileName, CB_FILE_MODE_READ);
				if (NOT read) {
					free(readData);
					CBFreeSafeOutputProcess(self);
					return CB_SAFE_OUTPUT_FAIL_PREVIOUS;
				}
				if (size > readDataSize) {
					uint8_t * temp = realloc(readData, size);
					if (NOT temp) {
						free(readData);
						CBFileClose(read);
						CBFreeSafeOutputProcess(self);
						return CB_SAFE_OUTPUT_FAIL_PREVIOUS;
					}
					readData = temp;
					readDataSize = (uint32_t) size;
				}
				if (NOT CBFileRead(read, readData, size)){
					free(readData);
					CBFileClose(read);
					CBFreeSafeOutputProcess(self);
					return CB_SAFE_OUTPUT_FAIL_PREVIOUS;
				}
				if (NOT CBFileWrite(backup, readData, size)){
					free(readData);
					CBFileClose(read);
					CBFreeSafeOutputProcess(self);
					return CB_SAFE_OUTPUT_FAIL_PREVIOUS;
				}
				break;
			case CB_SAFE_OUTPUT_OP_RENAME:
				// Write new filename
				byte = strlen(self->files[x].opData.newName);
				if (NOT CBFileWrite(backup, &byte, 1)) {
					free(readData);
					CBFreeSafeOutputProcess(self);
					return CB_SAFE_OUTPUT_FAIL_PREVIOUS;
				}
				if (NOT CBFileWrite(backup, self->files[x].opData.newName, byte)){
					free(readData);
					CBFreeSafeOutputProcess(self);
					return CB_SAFE_OUTPUT_FAIL_PREVIOUS;
				}
				break;
			case CB_SAFE_OUTPUT_OP_OVERWRITE_APPEND:{
				// Overwrites and appendages
				// Write number of overwrite operations for this file
				CBAppendAndOverwrite * ops = &self->files[x].opData.appendAndOverwriteOps;
				byte = ops->numOverwriteOperations;
				if (NOT CBFileWrite(backup, &byte, 1)) {
					free(readData);
					CBFreeSafeOutputProcess(self);
					return CB_SAFE_OUTPUT_FAIL_PREVIOUS;
				}
				// Go through and make backups for all of the operations.
				for (uint8_t y = 0; y < ops->numOverwriteOperations; y++) {
					// Write file offset
					if(NOT CBFileWrite(backup, (uint8_t []){
						ops->overwriteOperations[y].offset,
						ops->overwriteOperations[y].offset >> 8,
						ops->overwriteOperations[y].offset >> 16,
						ops->overwriteOperations[y].offset >> 24,
						ops->overwriteOperations[y].offset >> 32,
						ops->overwriteOperations[y].offset >> 40,
						ops->overwriteOperations[y].offset >> 48,
						ops->overwriteOperations[y].offset >> 56,
					}, 8)) {
						free(readData);
						CBFileClose(read);
						CBFreeSafeOutputProcess(self);
						return CB_SAFE_OUTPUT_FAIL_PREVIOUS;
					}
					// Write data size
					if (NOT CBFileWrite(backup, (uint8_t []){
						ops->overwriteOperations[y].size,
						ops->overwriteOperations[y].size >> 8,
						ops->overwriteOperations[y].size >> 16,
						ops->overwriteOperations[y].size >> 24,
					}, 4)) {
						free(readData);
						CBFileClose(read);
						CBFreeSafeOutputProcess(self);
						return CB_SAFE_OUTPUT_FAIL_PREVIOUS;
					}
					// Read and write old data
					if (ops->overwriteOperations[y].size > readDataSize) {
						uint8_t * temp = realloc(readData, ops->overwriteOperations[y].size);
						if (NOT temp) {
							free(readData);
							CBFileClose(read);
							CBFreeSafeOutputProcess(self);
							return CB_SAFE_OUTPUT_FAIL_PREVIOUS;
						}
						readData = temp;
						readDataSize = ops->overwriteOperations[y].size;
					}
					CBFileSeek(read, ops->overwriteOperations[y].offset, CB_SEEK_SET);
					if (NOT CBFileRead(read, readData, ops->overwriteOperations[y].size)) {
						free(readData);
						CBFileClose(read);
						CBFreeSafeOutputProcess(self);
						return CB_SAFE_OUTPUT_FAIL_PREVIOUS;
					}
					if (NOT CBFileRead(backup, readData, ops->overwriteOperations[y].size)) {
						free(readData);
						CBFileClose(read);
						CBFreeSafeOutputProcess(self);
						return CB_SAFE_OUTPUT_FAIL_PREVIOUS;
					}
				}
				break;
			}
			case CB_SAFE_OUTPUT_OP_SAVE:
				// Save operation
				if (read) {
					// Write entire previous file.
					if (size > readDataSize) {
						uint8_t * temp = realloc(readData, size);
						if (NOT temp) {
							free(readData);
							CBFileClose(read);
							CBFreeSafeOutputProcess(self);
							return CB_SAFE_OUTPUT_FAIL_PREVIOUS;
						}
						readData = temp;
						readDataSize = (uint32_t)size;
					}
					if (NOT CBFileRead(read, readData, size)) {
						free(readData);
						CBFileClose(read);
						CBFreeSafeOutputProcess(self);
						return CB_SAFE_OUTPUT_FAIL_PREVIOUS;
					}
					if (NOT CBFileWrite(backup, readData, size)) {
						free(readData);
						CBFileClose(read);
						CBFreeSafeOutputProcess(self);
						return CB_SAFE_OUTPUT_FAIL_PREVIOUS;
					}
				}else{
					// Write 0 to signify no previous file
					if (NOT CBFileWrite(backup, (uint8_t []){0,0,0,0,}, 4)){
						free(readData);
						CBFileClose(read);
						CBFreeSafeOutputProcess(self);
						return CB_SAFE_OUTPUT_FAIL_PREVIOUS;
					}
				}
				break;
		}
		if (read) 
			// No longer need the file for reading.
			CBFileClose(read);
	}
	free(readData);
	// Transaction log is complete and active. Signify this with the indicator at the begining of the file.
	CBFileSeek(backup, 0, CB_SEEK_SET);
	byte = 1;
	if (NOT CBFileWrite(backup, &byte, 1)) {
		CBFreeSafeOutputProcess(self);
		return CB_SAFE_OUTPUT_FAIL_PREVIOUS;
	}
	// Syncronise to disk
	if (NOT CBFileDiskSynchronise(backup)){
		CBFreeSafeOutputProcess(self);
		return CB_SAFE_OUTPUT_FAIL_PREVIOUS;
	}
	// Now implement the changes
	bool err = false;
	for (uint8_t x = 0; x < self->numFiles; x++) {
		uint64_t file;
		switch (self->files[x].type) {
			case CB_SAFE_OUTPUT_OP_OVERWRITE_APPEND:
				// First complete the overwrite operations
				// Open file for update
				file = CBFileOpen(self->files[x].fileName, CB_FILE_MODE_OVERWRITE);
				if (NOT file){
					err = true;
					break;
				}
				CBAppendAndOverwrite * ops = &self->files[x].opData.appendAndOverwriteOps;
				for (uint8_t y = 0; y < ops->numOverwriteOperations; y++) {
					CBFileSeek(file, ops->overwriteOperations[y].offset, CB_SEEK_SET);
					if (NOT CBFileWrite(file, ops->overwriteOperations[y].data, ops->overwriteOperations[y].size)) {
						err = true;
						break;
					}
				}
				// Re-open for appendages
				CBFileClose(file);
				file = CBFileOpen(self->files[x].fileName, CB_FILE_MODE_APPEND);
				if (NOT file){
					err = true;
					break;
				}
				for (uint8_t y = 0; y < ops->numAppendOperations; y++) {
					if (NOT CBFileWrite(file, ops->appendOperations[y].data, ops->appendOperations[y].size)) {
						err = true;
						break;
					}
				}
				break;
			case CB_SAFE_OUTPUT_OP_SAVE:
				// Open file for writting
				file = CBFileOpen(self->files[x].fileName, CB_FILE_MODE_SAVE);
				if (NOT file) {
					err = true;
					break;
				}
				if (NOT CBFileWrite(file, self->files[x].opData.saveOp.data, self->files[x].opData.saveOp.size))
					err = true;
				break;
			case CB_SAFE_OUTPUT_OP_DELETE:{
				char backupFile[strlen(self->files[x].fileName) + 5];
				memcpy(backupFile, self->files[x].fileName, strlen(self->files[x].fileName));
				strcpy(backupFile, ".bak");
				if (NOT CBFileDelete(backupFile)) // Force the rename
					err = true;
				else if (NOT CBFileRename(self->files[x].fileName, backupFile))
					err = true;
				if (err)
					break;
				// Synchronise the directory.
				char * pos = strrchr(backupFile, '/');
				pos[0] = '\0';
				file = CBFileOpen(backupFile, CB_FILE_MODE_NONE);
				if (NOT file)
					err = true;
				break;
			}
			case CB_SAFE_OUTPUT_OP_RENAME:
				if (NOT CBFileDelete(self->files[x].opData.newName)) // Force the rename
					err = true;
				else if (NOT CBFileRename(self->files[x].fileName, self->files[x].opData.newName))
					err = true;
				if (err)
					break;
				// Synchronise the directory.
				char * pos = strrchr(self->files[x].fileName, '/');
				char temp = pos[0];
				pos[0] = '\0';
				file = CBFileOpen(self->files[x].fileName, CB_FILE_MODE_NONE);
				if (NOT file){
					err = true;
					break;
				}
				pos[0] = temp;
				break;
			case CB_SAFE_OUTPUT_OP_TRUNCATE:
				// Truncate the file
				if (NOT CBFileTruncate(self->files[x].fileName, self->files[x].opData.newSize)) {
					err = true;
					break;
				}
				// Synchronise the file
				file = CBFileOpen(self->files[x].fileName, CB_FILE_MODE_NONE);
				if (NOT file)
					err = true;
				break;
		}
		if (NOT err)
			// Now try synchronising changes
			if (NOT CBFileDiskSynchronise(file))
				err = true;
		// Finnished with this file.
		CBFileClose(file);
		if (err)
			break;
	}
	// Check to see if an error has occured.
	if (err) {
		// Write operation failure. Try recovery.
		if (CBSafeOutputRecover(backup)){
			// Successful Recovery
			CBFreeSafeOutputProcess(self);
			return CB_SAFE_OUTPUT_FAIL_PREVIOUS;
		}else{
			// Recovery failed. In a bad state.
			CBFreeSafeOutputProcess(self);
			return CB_SAFE_OUTPUT_FAIL_BAD_STATE;
		}
	}
	// Indicate that backup is not active. If this fails then it is not an issue for data integrity it just might lead to a transaction reversal.
	CBFileSeek(backup, 0, CB_SEEK_SET);
	byte = 0;
	CBFileWrite(backup, &byte, 1);
	// Done this commit, free data
	CBFreeSafeOutputProcess(self);
	return CB_SAFE_OUTPUT_OK;
}
bool CBSafeOutputRecover(uint64_t backup){
	// Check backup is completed and active
	uint8_t byte;
	CBFileSeek(backup , 0, CB_SEEK_SET);
	if (NOT CBFileRead(backup, &byte, 1))
		return false;
	if (NOT byte)
		// With no active backup, no recovery is needed.
		return true;
	// Get the number of files
	uint8_t numFiles;
	if (NOT CBFileRead(backup, &numFiles, 1))
		return false;
	// Go through each file and restore the backups.
	uint8_t * readData = malloc(8);
	if (NOT readData)
		return false;
	uint32_t readDataSize = 8;
	for (uint8_t x = 0; x < numFiles; x++) {
		// Read filename length
		if (NOT CBFileRead(backup, &byte, 1)){
			free(readData);
			return false;
		}
		// Read filename
		char fileName[byte + 1];
		if (NOT CBFileRead(backup, &fileName, byte)) {
			free(readData);
			return false; 
		}
		fileName[byte] = '\0';
		// Read the type of operation
		if (NOT CBFileRead(backup, &byte, 1)){
			free(readData);
			return false;
		}
		uint32_t filesize;
		if (byte == CB_SAFE_OUTPUT_OP_OVERWRITE_APPEND
			|| byte == CB_SAFE_OUTPUT_OP_SAVE) {
			// Read previous file size.
			if (NOT CBFileRead(backup, readData, 4)){
				free(readData);
				return false;
			}
			filesize = readData[0]
			| (uint32_t)readData[1] << 8
			| (uint32_t)readData[2] << 16
			| (uint32_t)readData[3] << 24;
		}
		uint64_t file;
		switch (byte) {
			case CB_SAFE_OUTPUT_OP_SAVE:{
				// Save operation
				if (filesize) {
					// Read length of old file
					if (NOT CBFileRead(backup, readData, 4)){
						free(readData);
						return false;
					}
					uint32_t size = readData[0]
					| (uint32_t)readData[1] << 8
					| (uint32_t)readData[2] << 16
					| (uint32_t)readData[3] << 24;
					// Read data
					if (size > readDataSize) {
						uint8_t * temp = realloc(readData, size);
						if (NOT temp) {
							free(readData);
							return false;
						}
						readData = temp;
						readDataSize = size;
					}
					if (CBFileRead(backup, readData, size)) {
						free(readData);
						return false;
					}
					// Write data back into file.
					file = CBFileOpen(fileName, CB_FILE_MODE_SAVE);
					if (NOT file) {
						free(readData);
						return false;
					}
					if (NOT CBFileWrite(file, readData, size)) {
						free(readData);
						CBFileClose(file);
						return false;
					}
				}else{
					// No previous file. Delete this file
					if (NOT CBFileDelete(fileName)) {
						free(readData);
						return false;
					}
				}
				break;
			}
			case CB_SAFE_OUTPUT_OP_OVERWRITE_APPEND:{
				// Overwrites and appendages
				// Truncate to old size
				if (CBFileTruncate(fileName, filesize)) {
					free(readData);
					return false;
				}
				// Read number of operations
				if (NOT CBFileRead(backup, &byte, 1)){
					free(readData);
					return false;
				}
				file = CBFileOpen(fileName, CB_FILE_MODE_OVERWRITE);
				// Reverse all overwrite operations
				for (uint8_t x = 0; x < byte; x++) {
					// Read file offset
					if (CBFileRead(backup, readData, 8)){
						free(readData);
						CBFileClose(file);
						return false;
					}
					uint64_t offset = readData[0]
					| (uint64_t)readData[1] << 8
					| (uint64_t)readData[2] << 16
					| (uint64_t)readData[3] << 24
					| (uint64_t)readData[4] << 32
					| (uint64_t)readData[5] << 40
					| (uint64_t)readData[6] << 48
					| (uint64_t)readData[7] << 56;
					// Move to offset
					CBFileSeek(file, offset, CB_SEEK_SET);
					// Read data size
					if (CBFileRead(backup, readData, 4)){
						free(readData);
						CBFileClose(file);
						return false;
					}
					uint32_t size = readData[0]
					| (uint32_t)readData[1] << 8
					| (uint32_t)readData[2] << 16
					| (uint32_t)readData[3] << 24;
					// Read and then write the backup
					if (size > readDataSize) {
						uint8_t * temp = realloc(readData, size);
						if (NOT temp) {
							free(readData);
							CBFileClose(file);
							return false;
						}
						readData = temp;
						readDataSize = size;
					}
					if (NOT CBFileRead(backup, readData, size)) {
						free(readData);
						CBFileClose(file);
						return false;
					}
					if (CBFileWrite(file, readData, size)) {
						free(readData);
						CBFileClose(file);
						return false;
					}
				}
				break;
			}
			case CB_SAFE_OUTPUT_OP_DELETE:{
				// Check backup file exists. It may have been renamed already
				char backupFile[strlen(fileName) + 5];
				memcpy(backupFile, fileName, strlen(fileName));
				strcpy(backupFile, ".bak");
				if (CBFileExists(backupFile)) {
					// Rename backup file to original
					if (NOT CBFileRename(backupFile, fileName)) {
						free(readData);
						return false;
					}
				}
				// Sync directory
				char * pos = strrchr(backupFile, '/');
				pos[0] = '\0';
				file = CBFileOpen(backupFile, CB_FILE_MODE_NONE);
				if (NOT file) {
					free(readData);
					return false;
				}
				break;
			}
			case CB_SAFE_OUTPUT_OP_RENAME:{
				// Reverse rename to the original name
				if (NOT CBFileRead(backup, &byte, 1)){
					free(readData);
					return false;
				}
				// Read filename
				char newFileName[byte + 1];
				if (CBFileRead(backup, newFileName, byte)) {
					free(readData);
					return false;
				}
				newFileName[byte] = '\0';
				// Check the new file name exists. If may have been renamed already
				if (CBFileExists(newFileName)) {
					// Now rename
					if (CBFileRename(newFileName, fileName)) {
						free(readData);
						return false;
					}
				}
				// Sync directory
				char * pos = strrchr(newFileName, '/');
				pos[0] = '\0';
				file = CBFileOpen(newFileName, CB_FILE_MODE_NONE);
				if (NOT file) {
					free(readData);
					return false;
				}
				break;
			}
			case CB_SAFE_OUTPUT_OP_TRUNCATE:{
				// Append what was truncated.
				// Read length of data to append
				if (NOT CBFileRead(backup, readData, 4)){
					free(readData);
					return false;
				}
				uint32_t size = readData[0]
				| (uint32_t)readData[1] << 8
				| (uint32_t)readData[2] << 16
				| (uint32_t)readData[3] << 24;
				// Read data
				if (size > readDataSize) {
					uint8_t * temp = realloc(readData, size);
					if (NOT temp) {
						free(readData);
						return false;
					}
					readData = temp;
					readDataSize = size;
				}
				if (CBFileRead(backup, readData, size)) {
					free(readData);
					return false;
				}
				// Write data back into file.
				file = CBFileOpen(fileName, CB_FILE_MODE_APPEND);
				if (NOT file) {
					free(readData);
					return false;
				}
				if (NOT CBFileWrite(file, readData, size)) {
					free(readData);
					CBFileClose(file);
					return false;
				}
				break;
			}
		}
		// Synchronise to the disk
		if (NOT CBFileDiskSynchronise(file)) {
			free(readData);
			CBFileClose(file);
			return false;
		}
		// No longer need file
		CBFileClose(file);
	}
	free(readData);
	// Indicate that backup is not active. If this fails then it is not an issue for data integrity it just might lead to unecessary recovery.
	CBFileSeek(backup, 0, CB_SEEK_SET);
	byte = 0;
	CBFileWrite(backup, &byte, 1);
	return true;
}
