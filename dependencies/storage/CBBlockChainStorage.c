//
//  CBBlockChainStorage.c
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

#include "CBBlockChainStorage.h"

uint64_t CBNewBlockChainStorage(char * dataDir, void (*logError)(char *,...)){
	// Try to create the object
	CBBlockChainStorage * self = malloc(sizeof(*self));
	if (NOT self) {
		logError("Could not create the block chain storage object.");
		return 0;
	}
	// Check data consistency
	if (NOT CBBlockChainStorageEnsureConsistent((uint64_t) self)){
		logError("The database is inconsistent and could not be recovered in CBNewBlockChainStorage");
		return 0;
	}
	// Load index
	char filemame[strlen(dataDir) + 6];
	memcpy(filemame, dataDir, strlen(dataDir));
	strcpy(filemame + strlen(dataDir), "0.dat");
	// First check that the index exists
	if (NOT access(filemame, F_OK)) {
		// Can be read
		int indexRead = open(filemame, O_RDONLY);
		if (indexRead == -1) {
			free(self);
			logError("Could not open the database index.");
			return 0;
		}
		uint8_t data[16];
		// Get the number of values
		if (read(indexRead, data, 4) != 4) {
			close(indexRead);
			free(self);
			logError("Could not read the number of values from the database index.");
			return 0;
		}
		self->numValues = CBArrayToInt32(data, 0);
		// Get the number of files on disk
		if (read(indexRead, data, 2) != 2) {
			close(indexRead);
			free(self);
			logError("Could not read the number of files from the database index.");
			return 0;
		}
		self->numAvailableFiles = CBArrayToInt16(data, 0);
		// Get the size of the last file
		if (read(indexRead, data, 4) != 4) {
			close(indexRead);
			free(self);
			logError("Could not read the size of the last file from the database index.");
			return 0;
		}
		self->lastSize = CBArrayToInt32(data, 0);
		// First allocate index data
		self->index = malloc(sizeof(*self->index) * self->numValues);
		if (NOT self->index) {
			close(indexRead);
			free(self);
			logError("Could not allocate data for the database index.");
			return 0;
		}
		// Now read index
		for (uint32_t x = 0; x < self->numValues; x++) {
			if (read(indexRead, data, 16) != 16) {
				close(indexRead);
				free(self);
				free(self->index);
				logError("Could not read from the database index.");
				return 0;
			}
			// Add data to index
			memcpy(self->index[x].key, data, 6);
			self->index[x].fileID = CBArrayToInt16(data, 6);
			self->index[x].pos = CBArrayToInt32(data, 8);
			self->index[x].length = CBArrayToInt32(data, 12);
		}
		// Now sort the index
		CBBlockChainStorageQuickSortIndex(self);
		// Close file
		close(indexRead);
	}else{
		// Else empty index
		self->numValues = 0;
		self->index = NULL;
		self->numAvailableFiles = 1;
		int indexAppend = open(filemame, O_WRONLY | O_CREAT);
		if (NOT indexAppend) {
			free(self);
			logError("Could not create the database index.");
			return 0;
		}
		// Write the number of values as 0
		if (write(indexAppend, (uint8_t []){0,0,0,0}, 4) != 4) {
			close(indexAppend);
			free(self);
			logError("Could not initially write the number of values to the database index.");
			return 0;
		}
		// Write available files as 1
		if (write(indexAppend, (uint8_t []){1,0}, 2) != 2) {
			close(indexAppend);
			free(self);
			logError("Could not initially write the number of files to the database index.");
			return 0;
		}
		// Write size of last file as 0
		if (write(indexAppend, (uint8_t []){0,0,0,0}, 4) != 4) {
			close(indexAppend);
			free(self);
			logError("Could not initially write the size of the last data file to the database index.");
			return 0;
		}
		// Synchronise.
		if(fsync(indexAppend)){
			close(indexAppend);
			free(self);
			logError("Could not initially synchronise the database index.");
			return 0;
		}
		close(indexAppend);
	}
	// Open deletion index
	filemame[strlen(dataDir)] = '1';
	// First check that the index exists
	if (NOT access(filemame, F_OK)) {
		// Can read deletion index
		int indexRead = open(filemame, O_RDONLY);
		if (indexRead == -1) {
			free(self);
			free(self->index);
			logError("Could not open the database deletion index.");
			return 0;
		}
		// Read deletion entry number
		uint8_t data[11];
		// Get the number of values
		if (read(indexRead, data, 4) != 4) {
			close(indexRead);
			free(self);
			free(self->index);
			logError("Could not read the number of entries from the database deletion index.");
			return 0;
		}
		self->numDeletions = CBArrayToInt32(data, 0);
		// Make deletion index
		self->deletionIndex = malloc(sizeof(*self->deletionIndex) * self->numDeletions);
		if (NOT self->deletionIndex) {
			close(indexRead);
			free(self);
			free(self->index);
			logError("Could not allocate data for the database deletion index.");
			return 0;
		}
		// Now read index
		for (uint32_t x = 0; x < self->numDeletions; x++) {
			if (read(indexRead, data, 11) != 11) {
				close(indexRead);
				free(self);
				free(self->index);
				free(self->deletionIndex);
				logError("Could not read entry from the database deletion index.");
				return 0;
			}
			// Add data to index
			self->deletionIndex[x].active = data[0];
			self->deletionIndex[x].fileID = CBArrayToInt16(data, 1);
			self->deletionIndex[x].pos = CBArrayToInt32(data, 3);
			self->deletionIndex[x].length = CBArrayToInt32(data, 7);
		}
	}else{
		// Else empty deletion index
		self->deletionIndex = NULL;
		self->numDeletions = 0;
		int indexAppend = open(filemame, O_WRONLY | O_CREAT);
		if (NOT indexAppend) {
			free(self);
			logError("Could not create the database deletion index.");
			return 0;
		}
		// Write the number of entries as 0
		if (write(indexAppend, (uint8_t []){0,0,0,0}, 4) != 4) {
			close(indexAppend);
			free(self);
			logError("Could not initially write the number of entries to the database deletion index.");
			return 0;
		}
		// Synchronise.
		if(fsync(indexAppend)){
			close(indexAppend);
			free(self);
			logError("Could not initially synchronise the database deletion index.");
			return 0;
		}
		close(indexAppend);
	}
	self->dataDir = dataDir;
	self->logError = logError;
	self->pendingAppends = 0;
	return (uint64_t)self;
}
void CBFreeBlockChainStorage(uint64_t iself){
	CBBlockChainStorage * self = (CBBlockChainStorage *)iself;
	free(self);
}
bool CBBlockChainStorageWriteValue(uint64_t iself, uint8_t key[6], uint8_t * data, uint32_t dataSize, uint32_t offset, uint32_t totalSize){
	CBBlockChainStorage * self = (CBBlockChainStorage *)iself;
	// Check for key in index
	bool found;
	uint32_t pos = CBBlockChainStorageFindKey(self, key, &found);
	if (found) {
		// Exists in index so we are overwriting data.
		// See if the data can be overwritten.
		if (self->sortedIndex[pos]->length >= totalSize){
			// We are going to overwrite the previous data.
			if (NOT CBBlockChainStorageAddOverwrite(self, self->sortedIndex[pos]->fileID, data, self->sortedIndex[pos]->pos + offset, dataSize)){
				self->logError("Failed to add an overwrite operation to overwrite a previous value.");
				return false;
			}
		}else{
			// We will mark the previous data as deleted and store data elsewhere
			
		}
	}else{
		// Does not exist in index, so we are creating new data
		// Look for deleted data
		
		// Make index data
		uint8_t indexData[20];
		memcpy(indexData, key, 6);
		// The position is all the values, plus the 6 bytes at the begining and the previous pending appends.
		CBInt32ToArray(indexData, 8, self->numValues * 16 + 6 + self->pendingAppends);
		self->pendingAppends += dataSize;
		// Add one to the file ID if the data stretches past the 32 bit integer limit.
		uint16_t fileID = self->numAvailableFiles + (self->lastSize + self->pendingAppends > UINT32_MAX);
		CBInt16ToArray(indexData, 6, fileID);
		CBInt32ToArray(indexData, 12, totalSize);
		// Add append operation for the index
		if (NOT CBBlockChainStorageAddAppend(self, 0, indexData, 20)){
			self->logError("Failed to add an append operation to the database index.");
			return false;
		}
		// Now add append operation for the data
		if (NOT CBBlockChainStorageAddAppend(self, fileID, indexData, totalSize)){
			self->logError("Failed to add an append operation for a new value.");
			return false;
		}
	}
	return true;
}
uint8_t * CBBlockChainStorageReadValue(uint64_t iself, uint8_t key[6], uint32_t dataSize, uint32_t offset){
	CBBlockChainStorage * self = (CBBlockChainStorage *)iself;
	
}
bool CBBlockChainStorageCommitData(uint64_t iself){
	CBBlockChainStorage * self = (CBBlockChainStorage *)iself;
	
	return true;
}
bool CBBlockChainStorageEnsureConsistent(uint64_t iself){
	
}
bool CBBlockChainStorageAddAppend(CBBlockChainStorage * self, uint16_t fileID, uint8_t * data, uint32_t dataLen){
	// Find file or add it
	uint8_t x = 0;
	for (; x < self->numFiles; x++) {
		if (self->files[x].fileID == fileID) {
			break;
		}
	}
	if (x == self->numFiles) {
		CBFileOperations * temp = realloc(self->files, sizeof(*temp) * ++self->numFiles);
		if (NOT temp) {
			self->logError("Failed to reallocate file operation data.");
			return false;
		}
		self->files = temp;
	}
	// Add overwrite operation.
	CBAppendOperation * temp = realloc(self->files[x].appendOperations, sizeof(*temp) * ++self->files[x].numAppendOperations);
	if (NOT temp) {
		self->logError("Failed to reallocate file append operation data.");
		return false;
	}
	self->files[x].appendOperations[self->files[x].numAppendOperations - 1].data = data;
	self->files[x].appendOperations[self->files[x].numAppendOperations - 1].size = dataLen;
	return true;
}
bool CBBlockChainStorageAddOverwrite(CBBlockChainStorage * self, uint16_t fileID, uint8_t * data, uint64_t offset, uint32_t dataLen){
	// Find file or add it
	uint8_t x = 0;
	for (; x < self->numFiles; x++) {
		if (self->files[x].fileID == fileID) {
			break;
		}
	}
	if (x == self->numFiles) {
		CBFileOperations * temp = realloc(self->files, sizeof(*temp) * ++self->numFiles);
		if (NOT temp) {
			self->logError("Failed to reallocate file operation data.");
			return false;
		}
		self->files = temp;
	}
	// Add overwrite operation.
	CBOverwriteOperation * temp = realloc(self->files[x].overwriteOperations, sizeof(*temp) * ++self->files[x].numOverwriteOperations);
	if (NOT temp) {
		self->logError("Failed to reallocate file overwrite operation data.");
		return false;
	}
	self->files[x].overwriteOperations[self->files[x].numOverwriteOperations - 1].data = data;
	self->files[x].overwriteOperations[self->files[x].numOverwriteOperations - 1].offset = offset;
	self->files[x].overwriteOperations[self->files[x].numOverwriteOperations - 1].size = dataLen;
	return true;
}
uint32_t CBBlockChainStorageFindKey(CBBlockChainStorage * self, uint8_t key[6], bool * found){
	// Binary search for key
	if (NOT self->numValues){
		// No keys yet so add at index 0
		*found = false;
		return 0;
	}
	uint32_t left = 0;
	uint32_t right = self->numValues - 1;
	uint32_t mid;
	while (left <= right) {
		mid = (right+left)/2;
		int res = memcmp(key, self->sortedIndex[mid]->key, 6);
		if (res == 0) {
			*found = true;
			return mid;
		}else if (res < 0){
			if (NOT mid)
				break;
			right = mid - 1;
		}else{
			left = mid + 1;
		}
	}
	*found = false;
	return mid;
}
bool CBBlockChainStorageQuickSortIndex(CBBlockChainStorage * self){
	// Allocate sorted index
	self->sortedIndex = malloc(sizeof(*self->sortedIndex) * self->numValues * 2);
	if (NOT self->sortedIndex) {
		self->logError("Memory cannot be allocated for quicksort.");
		return false;
	}
	bool usingSecond = false;
	uint32_t widthStack[32]; // Recorded right lengths
	uint32_t width = self->numValues;
	uint8_t depth = 0;
	uint32_t i = 0;
	// Make pointers for second memory block
	for (uint64_t x = 0; x < self->numValues; x++)
		self->sortedIndex[x + self->numValues] = self->index + x;
	// Algorithm loop
	for (;;) {
		CBKeyValuePair ** into, ** outof;
		if (usingSecond) {
			into = self->sortedIndex + self->numValues;
			outof = self->sortedIndex;
		}else{
			into = self->sortedIndex;
			outof = self->sortedIndex + self->numValues;
		}
		uint32_t leftNum = 0;
		uint32_t rightNum = 0;
		for (uint32_t x = 1; x < width; x++) {
			if (memcmp(outof[x]->key, outof[i]->key, 6) < 0) {
				into[leftNum] = outof[x];
				leftNum++;
			}else{
				into[width - rightNum - 1] = outof[x];
				rightNum++;
			}
		}
		// Add pivot
		into[leftNum] = outof[i];
		if (leftNum < 2) {
			// No more going to left, try right
			if (rightNum < 2) {
				// No more going right, move up
				for (;depth;) {
					// Coming out of depth, copy over data
					memcpy(outof + i, into + i, width);
					i += width;
					width = widthStack[--depth];
					if (width > 1)
						// Found right width wide enough to sort.
						break;
				}
				if (NOT depth)
					// Came out of list
					break;
			}else{
				// Go right
				i += leftNum + 1;
				width = rightNum;
			}
		}else{
			// Go left
			widthStack[depth++] = rightNum;
			width = leftNum;
			usingSecond = NOT usingSecond;
		}
	}
	// Reallocate memory
	CBKeyValuePair ** temp = realloc(self->sortedIndex, sizeof(*temp) * self->numValues);
	if (temp)
		self->sortedIndex = temp;
	return true;
}
