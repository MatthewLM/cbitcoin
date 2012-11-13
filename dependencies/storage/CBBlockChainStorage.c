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
		uint32_t numValues = CBArrayToInt32(data, 0);
		// Get the number of files on disk
		if (read(indexRead, data, 2) != 2) {
			close(indexRead);
			free(self);
			logError("Could not read the number of files from the database index.");
			return 0;
		}
		self->lastFile = CBArrayToInt16(data, 0);
		// Get the size of the last file
		if (read(indexRead, data, 4) != 4) {
			close(indexRead);
			free(self);
			logError("Could not read the size of the last file from the database index.");
			return 0;
		}
		self->lastSize = CBArrayToInt32(data, 0);
		// First allocate index data
		if (self->numValues) {
			self->index = malloc(sizeof(*self->index) * numValues);
			if (NOT self->index) {
				close(indexRead);
				free(self);
				logError("Could not allocate data for the database index.");
				return 0;
			}
		}else
			self->index = NULL;
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
			bool found;
			uint32_t x = CBBlockChainStorageFindKey(self, data + 6, &found);
			if (self->numValues > x)
				memmove(self->index + x + 1, self->index + x, self->numValues - x);
			memcpy(self->index[x].key, data, 6);
			self->index[x].fileID = CBArrayToInt16(data, 6);
			self->index[x].pos = CBArrayToInt32(data, 8);
			self->index[x].length = CBArrayToInt32(data, 12);
			self->numValues++;
		}
		// Close file
		close(indexRead);
	}else{
		// Else empty index
		self->numValues = 0;
		self->index = NULL;
		self->lastFile = 1;
		int indexAppend = open(filemame, O_WRONLY | O_CREAT);
		if (NOT indexAppend) {
			free(self);
			logError("Could not create the database index.");
			return 0;
		}
		// Write the number of values as 0
		// Write last file as 2
		// Write size of last file as 0
		if (write(indexAppend, (uint8_t [10]){0,0,0,0,2,0,0,0,0,0}, 10) != 10) {
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
		if (self->numDeletions) {
			self->deletionIndex = malloc(sizeof(*self->deletionIndex) * self->numDeletions);
			if (NOT self->deletionIndex) {
				close(indexRead);
				free(self);
				free(self->index);
				logError("Could not allocate data for the database deletion index.");
				return 0;
			}
		}else
			self->deletionIndex = NULL;
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
	return (uint64_t)self;
}
void CBFreeBlockChainStorage(uint64_t iself){
	CBBlockChainStorage * self = (CBBlockChainStorage *)iself;
	CBResetBlockChainStorage(self);
	free(self->index);
	free(self->deletionIndex);
	free(self);
}
void CBResetBlockChainStorage(CBBlockChainStorage * self){
	for (uint8_t x = 0; x < self->numValueWrites; x++)
		free(self->valueWrites[x].data);
	self->numValueWrites = 0;
}
bool CBBlockChainStorageWriteValue(uint64_t iself, uint8_t key[6], uint8_t * data, uint32_t dataSize, uint32_t offset, uint32_t totalSize){
	CBBlockChainStorage * self = (CBBlockChainStorage *)iself;
	// Look for previous write for the key
	uint8_t x = 0;
	for (; x < self->numValueWrites; x++)
		if (NOT memcmp(self->valueWrites[x].key, key, 6))
			break;
	if (x == self->numValueWrites) {
		if (x == CB_MAX_VALUE_WRITES) {
			self->logError("Too many writes to the database at once.");
			CBResetBlockChainStorage(self);
			return false;
		}
		self->valueWrites[x].data = malloc(dataSize);
		if (NOT self->valueWrites[x].data) {
			self->logError("Could not allocate memory for a value write operation.");
			CBResetBlockChainStorage(self);
			return false;
		}
		self->numValueWrites++;
		memcpy(self->valueWrites[x].key, key, 6);
		self->valueWrites[x].offset = offset;
		self->valueWrites[x].totalLen = totalSize;
		self->valueWrites[x].dataLen = dataSize;
		self->valueWrites[x].allocLen = dataSize;
	}else{
		// Modifying previous data
		if (self->valueWrites[x].totalLen != totalSize) {
			// Replacement
			if (self->valueWrites[x].allocLen < dataSize) {
				// We need more memory
				uint8_t * temp = realloc(self->valueWrites[x].data, dataSize);
				if (NOT temp) {
					self->logError("Could not reallocate memory for a replacement value write operation.");
					CBResetBlockChainStorage(self);
					return false;
				}
				self->valueWrites[x].data = temp;
				self->valueWrites[x].allocLen = dataSize;
			}
			self->valueWrites[x].offset = offset;
			self->valueWrites[x].totalLen = totalSize;
			self->valueWrites[x].dataLen = dataSize;
		}else{
			// Replace some part of previous data or adds additional data
			uint32_t additionalData = 0;
			uint32_t prevEnd = self->valueWrites[x].dataLen + self->valueWrites[x].offset;
			uint32_t end = dataSize + offset;
			if (end > prevEnd)
				additionalData += end - prevEnd;
			if (offset < self->valueWrites[x].offset)
				// Adding before previous data
				additionalData += self->valueWrites[x].offset - offset;
			if (self->valueWrites[x].dataLen > self->valueWrites[x].allocLen) {
				// We need more memory
				uint8_t * temp = realloc(self->valueWrites[x].data, self->valueWrites[x].dataLen + additionalData);
				if (NOT temp) {
					self->logError("Could not reallocate memory for a value write operation when adding new data.");
					CBResetBlockChainStorage(self);
					return false;
				}
				self->valueWrites[x].data = temp;
				self->valueWrites[x].allocLen = self->valueWrites[x].dataLen + additionalData;
			}
			uint8_t * write = self->valueWrites[x].data;
			if (offset < self->valueWrites[x].offset){
				// Move previous data for new offset.
				memmove(self->valueWrites[x].data + self->valueWrites[x].offset - offset, self->valueWrites[x].data, self->valueWrites[x].dataLen);
				// New lower offset
				self->valueWrites[x].offset = offset;
			}else if (offset > self->valueWrites[x].offset)
				// Begin writing at the offset from the previous offset
				write += offset - self->valueWrites[x].offset;
			// Write new data
			memcpy(write, data, dataSize);
			// Update data length
			self->valueWrites[x].dataLen += additionalData;
		}
	}
	return true;
}
bool CBBlockChainStorageReadValue(uint64_t iself, uint8_t key[6], uint8_t * data, uint32_t dataSize, uint32_t offset){
	CBBlockChainStorage * self = (CBBlockChainStorage *)iself;
	
}
bool CBBlockChainStorageCommitData(uint64_t iself){
	CBBlockChainStorage * self = (CBBlockChainStorage *)iself;
	// Go through each key
	for (uint8_t x = 0; x < self->numValueWrites; x++) {
		// Check for key in index
		bool found;
		uint32_t pos = CBBlockChainStorageFindKey(self, self->valueWrites[x].key, &found);
		if (found) {
			// Exists in index so we are overwriting data.
			// See if the data can be overwritten.
			if (self->index[pos].length >= self->valueWrites[x].totalLen){
				// We are going to overwrite the previous data.
				if (NOT CBBlockChainStorageAddOverwrite(self, self->index[pos].fileID, self->valueWrites[x].data, self->index[pos].pos + self->valueWrites[x].offset, self->valueWrites[x].dataLen)){
					self->logError("Failed to add an overwrite operation to overwrite a previous value.");
					return false;
				}
				if (self->index[pos].length > self->valueWrites[x].totalLen) {
					// Change indexed length and mark the remaining length as deleted
					if (NOT CBBlockChainStorageAddDeletionEntry(self, self->index[pos].fileID, self->index[pos].pos + self->index[pos].length, self->index[pos].length - self->valueWrites[x].totalLen)){
						self->logError("Failed to add a deletion entry when overwriting a previous value with a smaller one.");
						return false;
					}
					self->index[pos].length = self->valueWrites[x].totalLen;
					uint8_t newLength[4];
					CBInt32ToArray(newLength, 0, self->index[pos].length);
					if (NOT CBBlockChainStorageAddOverwrite(self, 0, newLength, 22 + 16 * pos, 4)) {
						self->logError("Failed to add an overwrite operation to write the new length of a value to the database index.");
						return false;
					}
				}
			}else{
				// We will mark the previous data as deleted and store data elsewhere
				if (NOT CBBlockChainStorageAddDeletionEntry(self, self->index[pos].fileID, self->index[pos].pos, self->index[pos].length)){
					self->logError("Failed to add a deletion entry for an old value when replacing it with a larger one.");
					return false;
				}
				if (self->valueWrites[x].totalLen > UINT32_MAX - self->lastSize){
					self->lastFile++;
					self->lastSize = self->valueWrites[x].totalLen;
				}else
					self->lastSize += self->valueWrites[x].totalLen;
				if (self->valueWrites[x].offset) {
					// Append upto offset
					if (NOT CBBlockChainStorageAddAppend(self, self->lastFile, NULL, self->valueWrites[x].dataLen)) {
						self->logError("Failed to add an append operation for the offset to a value replacing an older one.");
						return false;
					}
				}
				if (NOT CBBlockChainStorageAddAppend(self, self->lastFile, self->valueWrites[x].data, self->valueWrites[x].dataLen)) {
					self->logError("Failed to add an append operation for the value replacing an older one.");
					return false;
				}
				if (self->valueWrites[x].offset + self->valueWrites[x].dataLen < self->valueWrites[x].totalLen) {
					// Append upto the total length, the previous data
					if (NOT CBBlockChainStorageAddAppend(self, self->lastFile, NULL, self->valueWrites[x].totalLen - self->valueWrites[x].dataLen - self->valueWrites[x].offset)) {
						self->logError("Failed to add an append operation for the remaining length to a value replacing an older one.");
						return false;
					}
				}
				// Update index
				self->index[pos].fileID = self->lastFile;
				self->index[pos].pos = self->lastSize - self->valueWrites[x].totalLen;
				self->index[pos].length = self->valueWrites[x].totalLen;
				uint8_t data[10];
				CBInt16ToArray(data, 0, self->index[pos].fileID);
				if (NOT CBBlockChainStorageAddOverwrite(self, 0, data, 16 + 16 * pos, 10)) {
					self->logError("Failed to add an overwrite operation for updating the index for moving a value.");
					return false;
				}
			}
		}else{
			// Does not exist in index, so we are creating new data
			CBKeyValuePair * temp = 
			if (self->numValues > x)
				memmove(self->index + x + 1, self->index + x, self->numValues - x);
			self->numValues++;
			// Look for deleted data
			uint32_t x = 0;
			for (; x < self->numDeletions; x++)
				if (self->deletionIndex[x].active && self->deletionIndex[x].length >= self->valueWrites[x].totalLen)
					break;
			if (x == self->numDeletions) {
				// No deleted regions to use
				if (self->valueWrites[x].totalLen > UINT32_MAX - self->lastSize){
					self->lastFile++;
					self->lastSize = self->valueWrites[x].totalLen;
				}else
					self->lastSize += self->valueWrites[x].totalLen;
				
			}
			// Make index data
			uint8_t indexData[20];
			memcpy(indexData, self->valueWrites[x].key, 6);
			// The position is all the values, plus the 6 bytes at the begining and the previous pending appends.
			CBInt32ToArray(indexData, 8, self->numValues * 16 + 6 + self->pendingAppends);
			self->pendingAppends += dataSize;
			// Add one to the file ID if the data stretches past the 32 bit integer limit.
			uint16_t fileID = self->lastFile + (self->lastSize + self->pendingAppends > UINT32_MAX);
			CBInt16ToArray(indexData, 6, fileID);
			CBInt32ToArray(indexData, 12, totalSize);
			// Add append operation for the index
			if (NOT CBBlockChainStorageAddAppend(self, 0, indexData, 20)){
				self->logError("Failed to add an append operation to the database index.");
				return false;
			}
			// Now add append operation for the data
			if (NOT CBBlockChainStorageAddAppend(self, fileID, , totalSize)){
				self->logError("Failed to add an append operation for a new value.");
				return false;
			}
		}
	}
	return true;
}
bool CBBlockChainStorageEnsureConsistent(uint64_t iself){
	return true;
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
bool CBBlockChainStorageAddDeletionEntry(CBBlockChainStorage * self, uint16_t fileID, uint32_t pos, uint32_t len){
	// Find if the entry can be inserted or not
	uint32_t x = 0;
	for (; x < self->numDeletions; x++)
		if (NOT self->deletionIndex[x].active)
			break;
	// Make data
	uint8_t data[11];
	data[0] = true;
	CBInt16ToArray(data, 1, fileID);
	CBInt32ToArray(data, 3, pos);
	CBInt32ToArray(data, 7, len);
	if (x == self->numDeletions) {
		self->numDeletions++;
		CBDeletedSection * temp = realloc(self->deletionIndex, sizeof(*temp) * self->numDeletions);
		if (NOT temp) {
			self->logError("Could not reallocate memory for the deletion index.");
			return false;
		}
		self->deletionIndex = temp;
		// Append
		if (NOT CBBlockChainStorageAddAppend(self, 1, data, 11)) {
			self->logError("Could not add appendage for the deletion index.");
			return false;
		}
	}else{
		// Overwrite
		if (NOT CBBlockChainStorageAddOverwrite(self, 1, data, 4 + x * 11, 11)) {
			self->logError("Could not add overwrite for the deletion index.");
			return false;
		}
	}
	self->deletionIndex[x].active = true;
	self->deletionIndex[x].fileID = fileID;
	self->deletionIndex[x].length = len;
	self->deletionIndex[x].pos = pos;
	return true;
}
bool CBBlockChainStorageAddOverwrite(CBBlockChainStorage * self, uint16_t fileID, uint8_t * data, uint64_t offset, uint32_t dataLen){
	// Find file or add it
	uint8_t x = 0;
	for (; x < self->numFiles; x++)
		if (self->files[x].fileID == fileID)
			break;
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
		int res = memcmp(key, self->index[mid].key, 6);
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
