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
		free(self);
		logError("The database is inconsistent and could not be recovered in CBNewBlockChainStorage");
		return 0;
	}
	// Load index
	if (NOT CBInitAssociativeArray(self->index, 6, sizeof(CBIndexValue))){
		free(self);
		logError("Could not initialise the database index.");
		return 0;
	}
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
		// Now read index
		for (uint32_t x = 0; x < numValues; x++) {
			if (read(indexRead, data, 16) != 16) {
				close(indexRead);
				free(self);
				free(self->index);
				logError("Could not read from the database index.");
				return 0;
			}
			// Add data to index
			CBIndexValue val;
			val.fileID = CBArrayToInt16(data, 6);
			val.pos = CBArrayToInt32(data, 8);
			val.length = CBArrayToInt32(data, 12);
			val.indexPos = x;
			if (NOT CBAssociativeArrayInsert(self->index, data, &val, CBAssociativeArrayFind(self->index, data), NULL)){
				close(indexRead);
				free(self);
				logError("Could not insert data into the database index.");
				return 0;
			}
		}
		// Close file
		close(indexRead);
	}else{
		// Else empty index
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
	// Load deletion index
	if (NOT CBInitAssociativeArray(self->deletionIndex, 5, sizeof(CBDeletedSection))){
		free(self);
		free(self->index);
		logError("Could not initialise the database deletion index.");
		return 0;
	}
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
		uint32_t numDeletions = CBArrayToInt32(data, 0);
		// Now read index
		for (uint32_t x = 0; x < numDeletions; x++) {
			if (read(indexRead, data, 11) != 11) {
				close(indexRead);
				free(self);
				free(self->index);
				free(self->deletionIndex);
				logError("Could not read entry from the database deletion index.");
				return 0;
			}
			// Add data to index
			CBDeletedSection section;
			section.fileID = CBArrayToInt16(data, 0);
			section.pos = CBArrayToInt32(data, 2);
			if (NOT CBAssociativeArrayInsert(self->deletionIndex, data + 6, &section, CBAssociativeArrayFind(self->deletionIndex, data + 7), NULL)) {
				close(indexRead);
				free(self);
				free(self->index);
				free(self->deletionIndex);
				logError("Could not insert entry to the database deletion index.");
				return 0;
			}
		}
	}else{
		// Else empty deletion index
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
		CBFindResult res = CBAssociativeArrayFind(self->index, self->valueWrites[x].key);
		if (res.found) {
			// Exists in index so we are overwriting data.
			// See if the data can be overwritten.
			CBIndexValue * indexValue = CBAssociativeArrayGetData(self->index, res);
			if (indexValue->length >= self->valueWrites[x].totalLen){
				// We are going to overwrite the previous data.
				if (NOT CBBlockChainStorageAddOverwrite(self, indexValue->fileID, self->valueWrites[x].data, indexValue->pos + self->valueWrites[x].offset, self->valueWrites[x].dataLen)){
					self->logError("Failed to add an overwrite operation to overwrite a previous value.");
					return false;
				}
				if (indexValue->length > self->valueWrites[x].totalLen) {
					// Change indexed length and mark the remaining length as deleted
					if (NOT CBBlockChainStorageAddDeletionEntry(self, indexValue->fileID, indexValue->pos + indexValue->length, indexValue->length - self->valueWrites[x].totalLen)){
						self->logError("Failed to add a deletion entry when overwriting a previous value with a smaller one.");
						return false;
					}
					indexValue->length = self->valueWrites[x].totalLen;
					uint8_t newLength[4];
					CBInt32ToArray(newLength, 0, indexValue->length);
					if (NOT CBBlockChainStorageAddOverwrite(self, 0, newLength, 28 + 16 * indexValue->indexPos, 4)) {
						self->logError("Failed to add an overwrite operation to write the new length of a value to the database index.");
						return false;
					}
				}
			}else{
				// We will mark the previous data as deleted and store data elsewhere
				if (NOT CBBlockChainStorageAddDeletionEntry(self, indexValue->fileID, indexValue->pos, indexValue->length)){
					self->logError("Failed to add a deletion entry for an old value when replacing it with a larger one.");
					return false;
				}
				// Look for a deleted section to use
				CBFindResult res = CBBlockChainStorageGetDeletedSection(self, self->valueWrites[x].totalLen);
				if (res.found) {
					// Found a deleted section we can use for the data
					HERE
				}else{
					// No suitable deleted section, therefore append the data
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
						// Append the previous data upto the total length.
						if (NOT CBBlockChainStorageAddAppend(self, self->lastFile, NULL, self->valueWrites[x].totalLen - self->valueWrites[x].dataLen - self->valueWrites[x].offset)) {
							self->logError("Failed to add an append operation for the remaining length to a value replacing an older one.");
							return false;
						}
					}

				}
				// Update index
				indexValue->fileID = self->lastFile;
				indexValue->pos = self->lastSize - self->valueWrites[x].totalLen;
				indexValue->length = self->valueWrites[x].totalLen;
				uint8_t data[10];
				CBInt16ToArray(data, 0, indexValue->fileID);
				CBInt32ToArray(data, 2, indexValue->pos);
				CBInt32ToArray(data, 6, indexValue->length);
				if (NOT CBBlockChainStorageAddOverwrite(self, 0, data, 22 + 16 * indexValue->indexPos, 10)) {
					self->logError("Failed to add an overwrite operation for updating the index for moving a value.");
					return false;
				}
			}
		}else{
			// Does not exist in index, so we are creating new data
			// Look for deleted data large enough
			
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
CBFindResult CBBlockChainStorageGetDeletedSection(CBBlockChainStorage * self, uint32_t length){
	CBFindResult res;
	res.node = self->deletionIndex->root;
	for (; res.node->children[res.node->numElements]; res.node = res.node->children[res.node->numElements]);
	uint8_t * key = (uint8_t *)(res.node + 1);
	uint8_t data[5];
	data[0] = 1;
	CBInt32ToArrayBigEndian(data, 1, length);
	res.pos = res.node->numElements - 1;
	res.found = res.node->numElements && memcmp(key, data, 5) > 0;
	return res;
}
