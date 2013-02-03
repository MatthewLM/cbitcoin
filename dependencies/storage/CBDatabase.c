//
//  CBDatabase.c
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

#include "CBDatabase.h"

CBDatabase * CBNewDatabase(char * dataDir, char * prefix){
	// Try to create the object
	CBDatabase * self = malloc(sizeof(*self));
	if (NOT self) {
		CBLogError("Could not create a database object.");
		return 0;
	}
	if (NOT CBInitDatabase(self, dataDir, prefix)) {
		free(self);
		CBLogError("Could not initialise a database object.");
		return 0;
	}
	return self;
}
bool CBInitDatabase(CBDatabase * self, char * dataDir, char * prefix){
	self->dataDir = dataDir;
	self->prefix = prefix;
	self->lastUsedFileObject = 0; // No stored file yet.
	self->numChangeKeys = 0;
	self->changeKeys = NULL;
	// Check data consistency
	if (NOT CBDatabaseEnsureConsistent(self)){
		CBLogError("The database is inconsistent and could not be recovered in CBNewDatabase");
		return false;
	}
	if (NOT CBInitAssociativeArray(&self->deleteKeys, CBKeyCompare, free)){
		CBLogError("Could not initilaise the deleteKeys array.");
		return false;
	}
	if (NOT CBInitAssociativeArray(&self->valueWrites, CBKeyCompare, free)){
		CBFreeAssociativeArray(&self->deleteKeys);
		CBLogError("Could not initilaise the valueWrites array.");
		return false;
	}
	// Load index
	if (NOT CBInitAssociativeArray(&self->index, CBKeyCompare, free)){
		CBFreeAssociativeArray(&self->deleteKeys);
		CBFreeAssociativeArray(&self->valueWrites);
		CBLogError("Could not initialise the database index.");
		return false;
	}
	char filename[strlen(dataDir) + strlen(prefix) + 7];
	sprintf(filename, "%s%s_0.dat", dataDir, prefix);
	// First check that the index exists
	if (NOT access(filename, F_OK)) {
		// Can be read
		if (NOT CBDatabaseReadAndOpenIndex(self, filename)) {
			CBFreeAssociativeArray(&self->deleteKeys);
			CBFreeAssociativeArray(&self->valueWrites);
			CBFreeAssociativeArray(&self->index);
			CBLogError("Could not load the database index.");
			return false;
		}
	}else{
		if (NOT CBDatabaseCreateIndex(self, filename)) {
			CBFreeAssociativeArray(&self->deleteKeys);
			CBFreeAssociativeArray(&self->valueWrites);
			CBFreeAssociativeArray(&self->index);
			CBLogError("Could not create the database index.");
			return false;
		}
	}
	// Load deletion index
	if (NOT CBInitAssociativeArray(&self->deletionIndex, CBKeyCompare, free)){
		CBFreeAssociativeArray(&self->deleteKeys);
		CBFreeAssociativeArray(&self->valueWrites);
		CBFreeAssociativeArray(&self->index);
		CBLogError("Could not initialise the database deletion index.");
		return false;
	}
	filename[strlen(dataDir) + strlen(prefix) + 1] = '1';
	// First check that the index exists
	if (NOT access(filename, F_OK)) {
		// Can read deletion index
		if (NOT CBDatabaseReadAndOpenDeletionIndex(self, filename)) {
			CBFreeAssociativeArray(&self->deleteKeys);
			CBFreeAssociativeArray(&self->valueWrites);
			CBFreeAssociativeArray(&self->index);
			CBFreeAssociativeArray(&self->deletionIndex);
			CBFileClose(self->indexFile);
			CBLogError("Could not load the database deletion index.");
			return false;
		}
	}else{
		// Else empty deletion index
		if (NOT CBDatabaseCreateDeletionIndex(self, filename)) {
			CBFreeAssociativeArray(&self->deleteKeys);
			CBFreeAssociativeArray(&self->valueWrites);
			CBFreeAssociativeArray(&self->index);
			CBFreeAssociativeArray(&self->deletionIndex);
			CBFileClose(self->indexFile);
			CBLogError("Could not create the database deletion index.");
			return false;
		}
	}
	return true;
}
bool CBDatabaseReadAndOpenIndex(CBDatabase * self, char * filename){
	self->indexFile = CBFileOpen(filename, false);
	if (NOT self->indexFile) {
		CBLogError("Could not open the database index.");
		return false;
	}
	uint8_t data[16];
	// Get the number of values
	if (NOT CBFileRead(self->indexFile, data, 4)){
		CBFileClose(self->indexFile);
		CBLogError("Could not read the number of values from the database index.");
		return false;
	}
	self->numValues = CBArrayToInt32(data, 0);
	// Get the last file
	if (NOT CBFileRead(self->indexFile, data, 2)) {
		CBFileClose(self->indexFile);
		CBLogError("Could not read the last file from the database index.");
		return false;
	}
	self->lastFile = CBArrayToInt16(data, 0);
	// Get the size of the last file
	if (NOT CBFileRead(self->indexFile, data, 4)) {
		CBFileClose(self->indexFile);
		CBLogError("Could not read the size of the last file from the database index.");
		return false;
	}
	self->lastSize = CBArrayToInt32(data, 0);
	self->nextIndexPos = 10;
	// Now read index
	for (uint32_t x = 0; x < self->numValues; x++) {
		// Get the key size.
		uint8_t keyLen;
		if (NOT CBFileRead(self->indexFile, &keyLen, 1)) {
			CBFileClose(self->indexFile);
			CBLogError("Could not read a key length for an index entry from the database index.");
			return false;
		}
		// Create the key-value index data
		uint8_t * keyVal = malloc(sizeof(CBIndexValue) + 1 + keyLen);
		*keyVal = keyLen;
		// Add key data to index
		if (NOT CBFileRead(self->indexFile, keyVal + 1, keyLen)) {
			CBFileClose(self->indexFile);
			CBLogError("Could not read a key for an index entry from the database index.");
			return false;
		}
		// Add value
		if (NOT CBFileRead(self->indexFile, data, 10)) {
			CBFileClose(self->indexFile);
			CBLogError("Could not read a value for an index entry from the database index.");
			return false;
		}
		CBIndexValue * value = (CBIndexValue *)(keyVal + keyLen + 1);
		value->fileID = CBArrayToInt16(data, 0);
		value->pos = CBArrayToInt32(data, 2);
		value->length = CBArrayToInt32(data, 6);
		value->indexPos = self->nextIndexPos;
		self->nextIndexPos += 11 + keyLen;
		if (NOT CBAssociativeArrayInsert(&self->index, keyVal, CBAssociativeArrayFind(&self->index, keyVal).position, NULL)){
			CBFileClose(self->indexFile);
			CBLogError("Could not insert data into the database index.");
			return false;
		}
	}
	return true;
}
bool CBDatabaseCreateIndex(CBDatabase * self, char * filename){
	// Else empty index
	self->nextIndexPos = 10; // First index starts after 10 bytes
	self->lastFile = 2;
	self->lastSize = 0;
	self->numValues = 0;
	// Open new index file.
	self->indexFile = CBFileOpen(filename, true);
	if (NOT self->indexFile) {
		CBLogError("Could not open a new index file.");
		return false;
	}
	// Write the number of values as 0
	// Write last file as 2
	// Write size of last file as 0
	if (NOT CBFileAppend(self->indexFile, (uint8_t [10]){0, 0, 0, 0, 2, 0, 0, 0, 0, 0}, 10)) {
		CBFileClose(self->indexFile);
		CBLogError("Could not initially write the size of the last data file to the database index.");
		return false;
	}
	// Synchronise.
	if (NOT CBFileSync(self->indexFile)){
		CBFileClose(self->indexFile);
		CBLogError("Could not initially synchronise the database index.");
		return false;
	}
	return true;
}
bool CBDatabaseReadAndOpenDeletionIndex(CBDatabase * self, char * filename){
	self->deletionIndexFile = CBFileOpen(filename, false);
	if (NOT self->deletionIndexFile) {
		CBLogError("Could not open the database deletion index.");
		return false;
	}
	// Read deletion entry number
	uint8_t data[11];
	// Get the number of values
	if (NOT CBFileRead(self->deletionIndexFile, data, 4)) {
		CBFileClose(self->deletionIndexFile);
		CBLogError("Could not read the number of entries from the database deletion index.");
		return false;
	}
	self->numDeletionValues = CBArrayToInt32(data, 0);
	// Now read index
	for (uint32_t x = 0; x < self->numDeletionValues; x++) {
		// Create the key-value index data
		CBDeletedSection * section = malloc(sizeof(*section));
		if (NOT section) {
			CBFileClose(self->deletionIndexFile);
			CBLogError("Could not allocate memory for deleted section value.");
			return false;
		}
		if (NOT CBFileRead(self->deletionIndexFile, data, 11)) {
			CBFileClose(self->deletionIndexFile);
			CBDatabaseClearPending(self);
			CBLogError("Could not read entry from the database deletion index.");
			return false;
		}
		// The key length is 11.
		section->key[0] = 11;
		// Add data to index
		memcpy(section->key + 1, data, 11);
		section->indexPos = x;
		if (NOT CBAssociativeArrayInsert(&self->deletionIndex, section->key, CBAssociativeArrayFind(&self->deletionIndex, section->key).position, NULL)) {
			CBFileClose(self->deletionIndexFile);
			CBLogError("Could not insert entry to the database deletion index.");
			return false;
		}
	}
	return true;
}
bool CBDatabaseCreateDeletionIndex(CBDatabase * self, char * filename){
	self->deletionIndexFile = CBFileOpen(filename, true);
	self->numDeletionValues = 0;
	if (NOT self->deletionIndexFile) {
		CBLogError("Could not create the database deletion index.");
		return true;
	}
	// Write the number of entries as 0
	if (NOT CBFileAppend(self->deletionIndexFile, (uint8_t []){0, 0, 0, 0}, 4)) {
		CBFileClose(self->deletionIndexFile);
		CBLogError("Could not initially write the number of entries to the database deletion index.");
		return true;
	}
	// Synchronise.
	if (NOT CBFileSync(self->deletionIndexFile)){
		CBFileClose(self->deletionIndexFile);
		CBLogError("Could not initially synchronise the database deletion index.");
		return true;
	}
	return true;
}
CBDatabase * CBGetDatabase(void * self){
	return self;
}
void CBFreeDatabase(CBDatabase * self){
	// Free write data
	CBFreeAssociativeArray(&self->valueWrites);
	// Free deletion data
	CBFreeAssociativeArray(&self->deleteKeys);
	// Free key change data
	for (uint32_t x = 0; x < self->numChangeKeys; x++) {
		free(self->changeKeys[x][0]);
		free(self->changeKeys[x][1]);
	}
	free(self->changeKeys);
	CBFreeAssociativeArray(&self->index);
	CBFreeAssociativeArray(&self->deletionIndex);
	// Close files
	CBFileClose(self->indexFile);
	CBFileClose(self->deletionIndexFile);
	if (self->lastUsedFileObject)
		CBFileClose(self->fileObjectCache);
	free(self);
}
bool CBDatabaseCommit(CBDatabase * self){
	char filename[strlen(self->dataDir) + strlen(self->prefix) + 9];
	sprintf(filename, "%s%s_log.dat", self->dataDir, self->prefix);
	// Open the log file
	uint64_t logFile = CBFileOpen(filename, true);
	if (NOT logFile) {
		CBLogError("The log file for overwritting could not be opened.");
		CBDatabaseClearPending(self);
		return false;
	}
	// Write the previous sizes for the indexes to the log file
	uint8_t data[15];
	data[0] = 1; // Log file active.
	uint32_t indexLen;
	uint32_t deletionIndexLen;
	if (NOT CBFileGetLength(self->indexFile, &indexLen)
		|| NOT CBFileGetLength(self->deletionIndexFile, &deletionIndexLen)) {
		CBLogError("Could not get the lengths of the index files.");
		CBDatabaseClearPending(self);
		return false;
	}
	CBInt32ToArray(data, 1, indexLen);
	CBInt32ToArray(data, 5, deletionIndexLen);
	CBInt16ToArray(data, 9, self->lastFile);
	CBInt32ToArray(data, 11, self->lastSize);
	if (NOT CBFileAppend(logFile, data, 15)) {
		CBLogError("Could not write previous size information to the log-file.");
		CBFileClose(logFile);
		CBDatabaseClearPending(self);
		return false;
	}
	// Sync log file, so that it is now active with the information of the previous file sizes.
	if (NOT CBFileSync(logFile)){
		CBLogError("Failed to sync the log file");
		CBFileClose(logFile);
		CBDatabaseClearPending(self);
		return false;
	}
	// Sync directory for log file
	if (NOT CBFileSyncDir(self->dataDir)) {
		CBLogError("Failed to synchronise the directory during a commit for the log file.");
		CBFileClose(logFile);
		CBDatabaseClearPending(self);
		return false;
	}
	uint32_t lastNumVals = self->numValues;
	uint32_t lastFileSize = self->lastSize;
	uint32_t prevDeletionEntryNum = self->numDeletionValues;
	// Go through each key
	// Get the first key with an interator
	CBPosition it;
	if (CBAssociativeArrayGetFirst(&self->valueWrites, &it)) for (;;) {
		// Get valueWrite element
		uint8_t * keyPtr = it.node->elements[it.index];
		uint32_t dataSize = *(uint32_t *)(keyPtr + *keyPtr + 1);
		uint8_t * dataPtr = keyPtr + *keyPtr + 5;
		// Check for key in index
		CBFindResult res = CBAssociativeArrayFind(&self->index, keyPtr);
		if (res.found) {
			// Exists in index so we are overwriting data.
			// See if the data can be overwritten.
			uint8_t * indexKey = res.position.node->elements[res.position.index];
			CBIndexValue * indexValue = (CBIndexValue *)(indexKey + *indexKey + 1);
			if (indexValue->length >= dataSize){
				// We are going to overwrite the previous data.
				if (NOT CBDatabaseAddOverwrite(self, indexValue->fileID, dataPtr, indexValue->pos, dataSize, logFile)){
					CBLogError("Failed to add an overwrite operation to overwrite a previous value.");
					CBFileClose(logFile);
					CBDatabaseClearPending(self);
					return false;
				}
				if (indexValue->length > dataSize) {
					// Change indexed length and mark the remaining length as deleted
					if (NOT CBDatabaseAddDeletionEntry(self, indexValue->fileID, indexValue->pos + dataSize, indexValue->length - dataSize, logFile)){
						CBLogError("Failed to add a deletion entry when overwriting a previous value with a smaller one.");
						CBFileClose(logFile);
						CBDatabaseClearPending(self);
						return false;
					}
					indexValue->length = dataSize;
					uint8_t newLength[4];
					CBInt32ToArray(newLength, 0, indexValue->length);
					if (NOT CBDatabaseAddOverwrite(self, 0, newLength, indexValue->indexPos + 7 + *indexKey, 4, logFile)) {
						CBLogError("Failed to add an overwrite operation to write the new length of a value to the database index.");
						CBFileClose(logFile);
						CBDatabaseClearPending(self);
						return false;
					}
				}
			}else{
				// We will mark the previous data as deleted and store data elsewhere, unless the data has been marked as deleted already (by having zero length in the index).
				if (indexValue->length && NOT CBDatabaseAddDeletionEntry(self, indexValue->fileID, indexValue->pos, indexValue->length, logFile)){
					CBLogError("Failed to add a deletion entry for an old value when replacing it with a larger one.");
					CBFileClose(logFile);
					CBDatabaseClearPending(self);
					return false;
				}
				if (NOT CBDatabaseAddValue(self, dataSize, dataPtr, indexValue, logFile)) {
					CBLogError("Failed to add a value to the database with a previously exiting key.");
					CBFileClose(logFile);
					CBDatabaseClearPending(self);
					return false;
				}
				// Write index
				uint8_t data[10];
				CBInt16ToArray(data, 0, indexValue->fileID);
				CBInt32ToArray(data, 2, indexValue->pos);
				CBInt32ToArray(data, 6, indexValue->length);
				if (NOT CBDatabaseAddOverwrite(self, 0, data, indexValue->indexPos + 1 + *indexKey, 10, logFile)) {
					CBLogError("Failed to add an overwrite operation for updating the index for writting data in a new location.");
					CBFileClose(logFile);
					CBDatabaseClearPending(self);
					return false;
				}
			}
		}else{
			// Does not exist in index, so we are creating new data
			// New index value data
			uint8_t * indexKey = malloc(sizeof(CBIndexValue) + 1 + *keyPtr);
			if (NOT CBDatabaseAddValue(self, dataSize, dataPtr, (CBIndexValue *)(indexKey + 1 + *keyPtr), logFile)) {
				CBLogError("Failed to add a value to the database with a new key.");
				CBFileClose(logFile);
				CBDatabaseClearPending(self);
				return false;
			}
			// Add index
			memcpy(indexKey, keyPtr, *keyPtr + 1);
			CBIndexValue * indexValue = (CBIndexValue *)(indexKey + *keyPtr + 1);
			// The index position is taken from the next index position.
			indexValue->indexPos = self->nextIndexPos;
			// Increase index position past appende index entry.
			self->nextIndexPos += 10 + 1 + *indexKey;
			// Insert index into array.
			if (NOT CBAssociativeArrayInsert(&self->index, indexKey, CBAssociativeArrayFind(&self->index, indexKey).position, NULL)) {
				CBLogError("Failed to insert new index entry.");
				CBFileClose(logFile);
				CBDatabaseClearPending(self);
				return false;
			}
			// Save to file
			if (NOT CBDatabaseAppend(self, 0, indexKey, *indexKey + 1)) {
				CBLogError("Failed to add an append operation for a new index entry key.");
				CBFileClose(logFile);
				CBDatabaseClearPending(self);
				return false;
			}
			uint8_t data[10];
			CBInt16ToArray(data, 0, indexValue->fileID);
			CBInt32ToArray(data, 2, indexValue->pos);
			CBInt32ToArray(data, 6, indexValue->length);
			if (NOT CBDatabaseAppend(self, 0, data, 10)) {
				CBLogError("Failed to add an append operation for a new index entry.");
				CBFileClose(logFile);
				CBDatabaseClearPending(self);
				return false;
			}
			self->numValues++;
		}
		// Iterate to next key-value to write.
		if (CBAssociativeArrayIterate(&self->valueWrites, &it))
			break;
	}
	if (lastNumVals != self->numValues
		|| lastFileSize != self->lastSize) {
		// Change index information.
		CBInt32ToArray(data, 0, self->numValues);
		CBInt16ToArray(data, 4, self->lastFile);
		CBInt32ToArray(data, 6, self->lastSize);
		if (NOT CBDatabaseAddOverwrite(self, 0, data, 0, 10, logFile)) {
			CBLogError("Failed to update the information for the index file.");
			CBFileClose(logFile);
			CBDatabaseClearPending(self);
			return false;
		}
	}
	// Delete values
	// Get first value to delete.
	if (CBAssociativeArrayGetFirst(&self->deleteKeys, &it)) for (;;) {
		// Find index entry
		CBFindResult res = CBAssociativeArrayFind(&self->index, it.node->elements[it.index]);
		if (NOT res.found) {
			CBLogError("Failed to find a key-value for deletion.");
			CBFileClose(logFile);
			CBDatabaseClearPending(self);
			return false;
		}
		uint8_t * indexKey = res.position.node->elements[res.position.index];
		CBIndexValue * indexVal = (CBIndexValue *)(indexKey + *indexKey + 1);
		// Create deletion entry for the data
		if (NOT CBDatabaseAddDeletionEntry(self, indexVal->fileID, indexVal->pos, indexVal->length, logFile)) {
			CBLogError("Failed to create a deletion entry for a key-value.");
			CBFileClose(logFile);
			CBDatabaseClearPending(self);
			return false;
		}
		// Make the length 0, signifying deletion of the data
		indexVal->length = 0;
		CBInt32ToArray(data, 0, 0);
		if (NOT CBDatabaseAddOverwrite(self, 0, data, indexVal->indexPos + 7 + *indexKey, 4, logFile)) {
			CBLogError("Failed to overwrite the index entry's length with 0 to signify deletion.");
			CBFileClose(logFile);
			CBDatabaseClearPending(self);
			return false;
		}
		// Iterate to next key to delete.
		if (CBAssociativeArrayIterate(&self->deleteKeys, &it))
			break;
	}
	if (prevDeletionEntryNum != self->numDeletionValues) {
		// New number of deletion entries.
		CBInt32ToArray(data, 0, self->numDeletionValues);
		if (NOT CBDatabaseAddOverwrite(self, 1, data, 0, 4, logFile)) {
			CBLogError("Failed to update the number of entries for the deletion index file.");
			CBFileClose(logFile);
			CBDatabaseClearPending(self);
			return false;
		}
	}
	// Change keys
	for (uint32_t x = 0; x < self->numChangeKeys; x++) {
		// Find key
		CBFindResult res = CBAssociativeArrayFind(&self->index, self->changeKeys[x][0]);
		if (NOT res.found) {
			CBLogError("Failed to find a key-value for changing.");
			CBFileClose(logFile);
			CBDatabaseClearPending(self);
			return false;
		}
		// Obtain index entry
		uint8_t * indexKey = res.position.node->elements[res.position.index];
		// Delete key
		CBAssociativeArrayDelete(&self->index, res.position, false);
		// Change key
		// Copy in new key.
		memcpy(indexKey + 1, self->changeKeys[x][1] + 1, *self->changeKeys[x][1]);
		// Re-insert
		if (NOT CBAssociativeArrayInsert(&self->index, indexKey, CBAssociativeArrayFind(&self->index, indexKey).position, NULL)) {
			CBLogError("Failed to insert an index entry with a changed key.");
			CBFileClose(logFile);
			CBDatabaseClearPending(self);
			return false;
		}
		// Overwrite key on disk
		if (NOT CBDatabaseAddOverwrite(self, 0, indexKey + 1, ((CBIndexValue *)(indexKey + 1 + *indexKey))->indexPos + 1, *indexKey, logFile)) {
			CBLogError("Failed to overwrite a key in the index.");
			CBFileClose(logFile);
			CBDatabaseClearPending(self);
			return false;
		}
	}
	// Sync last used file
	if ((self->lastUsedFileObject && NOT CBFileSync(self->fileObjectCache))
		|| NOT CBFileSync(self->indexFile)
		|| NOT CBFileSync(self->deletionIndexFile)) {
		CBLogError("Failed to synchronise the files during a commit.");
		CBFileClose(logFile);
		CBDatabaseClearPending(self);
		return false;
	}
	// Sync directory
	if (NOT CBFileSyncDir(self->dataDir)) {
		CBLogError("Failed to synchronise the directory during a commit.");
		CBFileClose(logFile);
		CBDatabaseClearPending(self);
		return false;
	}
	// Now we are done, make the logfile inactive. Errors do not matter here.
	if (CBFileSeek(logFile, 0)) {
		data[0] = 0;
		if (CBFileOverwrite(logFile, data, 1))
			CBFileSync(logFile);
	}
	CBFileClose(logFile);
	CBDatabaseClearPending(self);
	return true;
}
bool CBDatabaseAddDeletionEntry(CBDatabase * self, uint16_t fileID, uint32_t pos, uint32_t len, uint64_t logFile){
	// First look for inactive deletion that can be used.
	CBPosition res;
	res.node = self->deletionIndex.root;
	for (; res.node->children[0]; res.node = res.node->children[0]);
	if (res.node->numElements && NOT ((uint8_t *)res.node->elements[0])[1]) {
		// Inactive deletion entry. We will use this one.
		CBDeletedSection * section = (CBDeletedSection *)res.node->elements[0];
		section->key[1] = 1; // Re-activate.
		CBInt32ToArrayBigEndian(section->key, 2, len);
		CBInt16ToArray(section->key, 6, fileID);
		CBInt32ToArray(section->key, 8, pos);
		// Remove from index
		res.index = 0;
		CBAssociativeArrayDelete(&self->deletionIndex, res, false);
		// Re-insert into index with new key
		if (NOT CBAssociativeArrayInsert(&self->deletionIndex, section->key, CBAssociativeArrayFind(&self->deletionIndex, (uint8_t *)section).position, NULL)){
			CBLogError("Could not insert replacement deletion entry into the array.");
			return false;
		}
		// Overwrite deletion index
		uint8_t data[11];
		memcpy(data, section->key + 1, 11);
		if (NOT CBDatabaseAddOverwrite(self, 1, data, 4 + section->indexPos * 11, 11, logFile)) {
			CBLogError("There was an error when adding a deletion entry by overwritting an inactive one.");
			return false;
		}
	}else{
		// We need a new entry.
		CBDeletedSection * section = malloc(sizeof(*section));
		section->key[0] = 11;
		section->key[1] = 1;
		CBInt32ToArrayBigEndian(section->key, 2, len);
		CBInt16ToArray(section->key, 6, fileID);
		CBInt32ToArray(section->key, 8, pos);
		section->indexPos = self->numDeletionValues;
		// Insert the entry into the array
		if (NOT CBAssociativeArrayInsert(&self->deletionIndex, section->key, CBAssociativeArrayFind(&self->deletionIndex, section->key).position, NULL)){
			CBLogError("Could not insert new deletion entry into the array.");
			return false;
		}
		self->numDeletionValues++;
		// Append deletion index
		uint8_t data[11];
		memcpy(data, section->key + 1, 11);
		if (NOT CBDatabaseAppend(self, 1, data, 11)) {
			CBLogError("There was an error when adding a deletion entry by appending a new one.");
			return false;
		}
	}
	return true;
}
bool CBDatabaseAddOverwrite(CBDatabase * self, uint16_t fileID, uint8_t * data, uint32_t offset, uint32_t dataLen, uint64_t logFile){
	// Execute the overwrite and write rollback information to the log file.
	// Get the file to overwrite
	uint64_t file = CBDatabaseGetFile(self, fileID);
	if (NOT file) {
		CBLogError("Could not get the data file for overwritting.");
		return false;
	}
	// Write to the log file the rollback information for the changes being made.
	uint32_t dataTempLen = dataLen + 10;
	uint8_t * dataTemp = malloc(dataTempLen);
	if (NOT dataTemp) {
		CBLogError("Could not allocate memory for temporary memory for writting to the transaction log file.");
		return false;
	}
	// Write file ID
	CBInt16ToArray(dataTemp, 0, fileID);
	// Write offset
	CBInt32ToArray(dataTemp, 2, offset);
	// Write data size
	CBInt32ToArray(dataTemp, 6, dataLen);
	// Write old data
	if (NOT CBFileSeek(file, offset)){
		CBLogError("Could not seek to the read position for previous data in a file to be overwritten.");
		return false;
	}
	if (NOT CBFileRead(file, dataTemp + 10, dataLen)) {
		CBLogError("Could not read the previous data to be overwritten.");
		return false;
	}
	if (NOT CBFileAppend(logFile, dataTemp, dataTempLen)) {
		CBLogError("Could not write the backup information to the transaction log file.");
		return false;
	}
	free(dataTemp);
	// Sync log file
	if (NOT CBFileSync(logFile)){
		CBLogError("Failed to sync the log file");
		return false;
	}
	// Sync directory for log file
	if (NOT CBFileSyncDir(self->dataDir)) {
		CBLogError("Failed to synchronise the directory for the log file when overwritting a file.");
		return false;
	}
	// Execute overwrite
	if (NOT CBFileSeek(file, offset)){
		CBLogError("Could not seek the file to overwrite data.");
		return false;
	}
	if (NOT CBFileOverwrite(file, data, dataLen)) {
		CBLogError("Could not overwrite file %u at position %u with length %u", fileID, offset, dataLen);
		return false;
	}
	return true;
}
bool CBDatabaseAddValue(CBDatabase * self, uint32_t dataSize, uint8_t * data, CBIndexValue * indexValue, uint64_t logFile){
	// Look for a deleted section to use
	CBFindResult res = CBDatabaseGetDeletedSection(self, dataSize);
	if (res.found) {
		// ??? Is there a more effecient method for changing the key of the associative array than deleting and re-inserting with new key?
		// Found a deleted section we can use for the data
		CBDeletedSection * section = (CBDeletedSection *)res.position.node->elements[res.position.index];
		uint32_t sectionLen = CBArrayToInt32BigEndian(section->key, 2);
		uint16_t sectionFileID = CBArrayToInt16(section->key, 6);
		uint32_t sectionOffset = CBArrayToInt32(section->key, 8);
		uint32_t newSectionLen = sectionLen - dataSize;
		// Use the section
		if (NOT CBDatabaseAddOverwrite(self,  sectionFileID, data, sectionOffset + newSectionLen, dataSize, logFile)){
			CBLogError("Failed to add an overwrite operation to overwrite a previously deleted section with new data.");
			return false;
		}
		// Update index data
		indexValue->fileID = sectionFileID;
		indexValue->length = dataSize;
		indexValue->pos = sectionOffset + newSectionLen;
		// Change deletion index
		// Remove from array
		CBAssociativeArrayDelete(&self->deletionIndex, res.position, false);
		if (newSectionLen){
			// Change deletion section length
			CBInt32ToArrayBigEndian(section->key, 2, newSectionLen);
		}else
			// Make deleted section inactive.
			section->key[1] = 0; // Not active
		// Re-insert into array
		if (NOT CBAssociativeArrayInsert(&self->deletionIndex, section->key, CBAssociativeArrayFind(&self->deletionIndex, section->key).position, NULL)){
			CBLogError("Failed to change the key of a deleted section to inactive.");
			return false;
		}
		if (NOT CBDatabaseAddOverwrite(self, 1, section->key + 1, 4 + 11 * section->indexPos, 5, logFile)) {
			CBLogError("Failed to add an overwrite operation to update the deletion index for writting data to a deleted section");
			return false;
		}
	}else{
		// No suitable deleted section, therefore append the data
		indexValue->pos = self->lastSize; // Update position in index.
		if (dataSize > UINT32_MAX - self->lastSize){
			self->lastFile++;
			self->lastSize = dataSize;
		}else
			self->lastSize += dataSize;
		if (NOT CBDatabaseAppend(self, self->lastFile, data, dataSize)) {
			CBLogError("Failed to add an append operation for the value replacing an older one.");
			return false;
		}
		// Update index
		indexValue->fileID = self->lastFile;
		indexValue->length = dataSize;
	}
	return true;
}
bool CBDatabaseAddWriteValue(CBDatabase * self, uint8_t * writeValue){
	// Remove from deletion array if needed.
	CBFindResult res = CBAssociativeArrayFind(&self->deleteKeys, writeValue);
	if (res.found)
		CBAssociativeArrayDelete(&self->deleteKeys, res.position, false);
	// See if key exists to be written already
	res = CBAssociativeArrayFind(&self->valueWrites, writeValue);
	if (res.found) {
		// Found so replace the element
		free(res.position.node->elements[res.position.index]);
		res.position.node->elements[res.position.index] = writeValue;
	}else{
		// Insert into array
		if (NOT CBAssociativeArrayInsert(&self->valueWrites, writeValue, res.position, NULL)) {
			CBLogError("Failed to insert a value write element into the valueWrites array.");
			CBDatabaseClearPending(self);
			free(writeValue);
			return false;
		}
	}
	return true;
}
bool CBDatabaseAppend(CBDatabase * self, uint16_t fileID, uint8_t * data, uint32_t dataLen){
	// Execute the append operation
	uint64_t file = CBDatabaseGetFile(self, fileID);
	if (NOT file) {
		CBLogError("Could not get file %u for appending.", fileID);
		return false;
	}
	if (NOT CBFileAppend(file, data, dataLen)) {
		CBLogError("Failed to append data to file %u.", fileID);
		return false;
	}
	return true;
}
bool CBDatabaseChangeKey(CBDatabase * self, uint8_t * previousKey, uint8_t * newKey){
	self->changeKeys = realloc(self->changeKeys, sizeof(*self->changeKeys) * (self->numChangeKeys + 1));
	if (NOT self->changeKeys) {
		CBLogError("Failed to reallocate memory for the key change array.");
		CBDatabaseClearPending(self);
		return false;
	}
	self->changeKeys[self->numChangeKeys][0] = malloc(*previousKey + 1);
	if (NOT self->changeKeys[self->numChangeKeys][0]) {
		CBLogError("Failed to allocate memory for a previous key to change.");
		CBDatabaseClearPending(self);
		return false;
	}
	memcpy(self->changeKeys[self->numChangeKeys][0], previousKey, *previousKey + 1);
	self->changeKeys[self->numChangeKeys][1] = malloc(*newKey + 1);
	if (NOT self->changeKeys[self->numChangeKeys][1]) {
		CBLogError("Failed to allocate memory for a new key to replace an old one.");
		CBDatabaseClearPending(self);
		return false;
	}
	memcpy(self->changeKeys[self->numChangeKeys][1], newKey, *newKey + 1);
	self->numChangeKeys++;
	return true;
}
void CBDatabaseClearPending(CBDatabase * self){
	// Free write data
	CBFreeAssociativeArray(&self->valueWrites);
	CBInitAssociativeArray(&self->valueWrites, CBKeyCompare, free);
	// Free deletion data
	CBFreeAssociativeArray(&self->deleteKeys);
	CBInitAssociativeArray(&self->deleteKeys, CBKeyCompare, free);
	// Free key change data
	for (uint32_t x = 0; x < self->numChangeKeys; x++) {
		free(self->changeKeys[x][0]);
		free(self->changeKeys[x][1]);
	}
	free(self->changeKeys);
	self->changeKeys = NULL;
	self->numChangeKeys = 0;
}
bool CBDatabaseEnsureConsistent(CBDatabase * self){
	char filename[strlen(self->dataDir) + strlen(self->prefix) + 11];
	sprintf(filename, "%s%s_log.dat", self->dataDir, self->prefix);
	// Determine if the logfile has been created or not
	if (access(filename, F_OK))
		// Not created, so no history.
		return true;
	// Open the logfile for reading
	uint64_t logFile = CBFileOpen(filename, false);
	if (NOT logFile) {
		CBLogError("The log file for reading could not be opened.");
		return false;
	}
	// Check if the logfile is active or not.
	uint8_t data[14];
	if (NOT CBFileRead(logFile, data, 1)) {
		CBLogError("Failed reading the logfile.");
		CBFileClose(logFile);
		return false;
	}
	if (NOT data[0]){
		// Not active
		CBFileClose(logFile);
		return true;
	}
	// Active, therefore we need to attempt rollback.
	if (NOT CBFileRead(logFile, data, 14)) {
		CBLogError("Failed reading the logfile.");
		CBFileClose(logFile);
		return false;
	}
	// Truncate files
	char * filenameEnd = filename + strlen(self->dataDir) + strlen(self->prefix) + 1;
	strcpy(filenameEnd, "0.dat");
	if (NOT CBFileTruncate(filename, CBArrayToInt32(data, 0))) {
		CBLogError("Failed to truncate the index file down to the previous size.");
		CBFileClose(logFile);
		return false;
	}
	*filenameEnd = '1';
	if (NOT CBFileTruncate(filename, CBArrayToInt32(data, 4))) {
		CBLogError("Failed to truncate the deletion index file down to the previous size.");
		CBFileClose(logFile);
		return false;
	}
	uint16_t lastFile = CBArrayToInt16(data, 8);
	sprintf(filenameEnd, "%u.dat", lastFile);
	if (NOT CBFileTruncate(filename, CBArrayToInt32(data, 10))) {
		CBLogError("Failed to truncate the last file down to the previous size.");
		CBFileClose(logFile);
		return false;
	}
	// Check if a new file had been created. If it has, delete it.
	sprintf(filenameEnd, "%u.dat", lastFile + 1);
	if (NOT access(filename, F_OK))
		remove(filename);
	// Reverse overwrite operations
	uint32_t logFileLen;
	if (NOT CBFileGetLength(logFile, &logFileLen)) {
		CBLogError("Failed to get the length of the log file.");
		CBFileClose(logFile);
		return false;
	}
	for (uint32_t c = 15; c < logFileLen;) {
		// Read file ID, offset and size
		if (NOT CBFileRead(logFile, data, 10)) {
			CBLogError("Could not read from the log file.");
			CBFileClose(logFile);
			return false;
		}
		uint32_t dataLen = CBArrayToInt32(data, 6);
		// Read previous data
		uint8_t * prevData = malloc(dataLen);
		if (NOT prevData) {
			CBLogError("Could not allocate memory for previous data from the log file.");
			CBFileClose(logFile);
			return false;
		}
		if (NOT CBFileRead(logFile, prevData, dataLen)) {
			CBLogError("Could not read previous data from the log file.");
			CBFileClose(logFile);
			return false;
		}
		// Write the data to the file
		sprintf(filenameEnd, "%u.dat", CBArrayToInt16(data, 0));
		uint64_t file = CBFileOpen(filename, false);
		if (NOT file) {
			CBLogError("Could not open the file for writting the previous data.");
			CBFileClose(logFile);
			return false;
		}
		if (NOT CBFileSeek(file, CBArrayToInt32(data, 2))){
			CBLogError("Could not seek the file for writting the previous data.");
			CBFileClose(logFile);
			CBFileClose(file);
			return false;
		}
		if (NOT CBFileOverwrite(file, prevData, dataLen)) {
			CBLogError("Could not write previous data back into the file.");
			CBFileClose(logFile);
			CBFileClose(file);
			return false;
		}
		if (NOT CBFileSync(file)) {
			CBLogError("Could not synchronise a file during recovery..");
			CBFileClose(logFile);
			CBFileClose(file);
			return false;
		}
		CBFileClose(file);
		free(prevData);
		c += 14 + dataLen;
	}
	// Sync directory
	if (NOT CBFileSyncDir(self->dataDir)) {
		CBLogError("Failed to synchronise the directory when recovering database.");
		CBFileClose(logFile);
		return false;
	}
	// Now we are done, make the logfile inactive
	data[0] = 0;
	if (CBFileOverwrite(logFile, data, 1))
		CBFileSync(logFile);
	CBFileClose(logFile);
	return true;
}
CBFindResult CBDatabaseGetDeletedSection(CBDatabase * self, uint32_t length){
	CBFindResult res;
	if (CBAssociativeArrayGetLast(&self->deletionIndex, &res.position)) {
		// Found when there are elements, the deleted section is active and the deleted section length is equal or greater to "length".
		res.found = ((uint8_t *)res.position.node->elements[res.position.index])[1]
		&& CBArrayToInt32BigEndian(((uint8_t *)res.position.node->elements[res.position.index]), 2) >= length;
	}else
		res.found = false;
	return res;
}
uint64_t CBDatabaseGetFile(CBDatabase * self, uint16_t fileID){
	if (NOT fileID)
		return self->indexFile;
	else if (fileID == 1)
		return self->deletionIndexFile;
	else if (fileID == self->lastUsedFileObject)
		return self->fileObjectCache;
	else {
		// Change last used file cache
		if (self->lastUsedFileObject) {
			// First synchronise previous file
			if (NOT CBFileSync(self->fileObjectCache)) {
				CBLogError("Could not synchronise file %u when a new file is needed.", self->lastUsedFileObject);
				return false;
			}
			// Now close the file
			CBFileClose(self->fileObjectCache);
		}
		// Open the new file
		char filename[strlen(self->dataDir) + strlen(self->prefix) + 11];
		sprintf(filename, "%s%s_%i.dat", self->dataDir, self->prefix, fileID);
		self->fileObjectCache = CBFileOpen(filename, access(filename, F_OK));
		if (NOT self->fileObjectCache) {
			CBLogError("Could not open file %s.", filename);
			return false;
		}
		self->lastUsedFileObject = fileID;
		return self->fileObjectCache;
	}
}
uint32_t CBDatabaseGetLength(CBDatabase * self, uint8_t * key) {
	// Look in index for value
	CBFindResult res = CBAssociativeArrayFind(&self->index, key);
	if (NOT res.found){
		// Look for value in valueWrites array
		res = CBAssociativeArrayFind(&self->valueWrites, key);
		if (NOT res.found)
			return 0;
		return CBArrayToInt32(((uint8_t *)res.position.node->elements[res.position.index]), *key + 1);
	}
	return ((CBIndexValue *)((uint8_t *)res.position.node->elements[res.position.index] + *key + 1))->length;
}
bool CBDatabaseReadValue(CBDatabase * self, uint8_t * key, uint8_t * data, uint32_t dataSize, uint32_t offset){
	// Look in index for value
	CBFindResult res = CBAssociativeArrayFind(&self->index, key);
	if (NOT res.found){
		// Look for value in valueWrites array
		res = CBAssociativeArrayFind(&self->valueWrites, key);
		if (NOT res.found) {
			CBLogError("Could not find a value for a key.");
			return false;
		}
		// Get the data, existing, yet to be written.
		uint8_t * existingData = res.position.node->elements[res.position.index];
		// Move along key size and 5 for the key and data size information
		existingData += *existingData + 5;
		// Copy the data into the buffer supplied to the function
		memcpy(data, existingData + offset, dataSize);
		return true;
	}
	CBIndexValue * val = (CBIndexValue *)((uint8_t *)res.position.node->elements[res.position.index] + *key + 1);
	// Get file
	uint64_t file = CBDatabaseGetFile(self, val->fileID);
	if (NOT file) {
		CBLogError("Could not open file for a value.");
		return false;
	}
	if (NOT CBFileSeek(file, val->pos + offset)){
		CBLogError("Could not read seek file for value.");
		return false;
	}
	if (NOT CBFileRead(file, data, dataSize)) {
		CBLogError("Could not read from file for value.");
		return false;
	}
	return true;
}
bool CBDatabaseRemoveValue(CBDatabase * self, uint8_t * key){
	uint8_t * keyPtr = malloc(*key + 1);
	if (NOT keyPtr) {
		CBLogError("Failed to allocate memory for a key to delete from storage.");
		CBDatabaseClearPending(self);
		return false;
	}
	memcpy(keyPtr, key, *key + 1);
	// If in valueWrites array, remove it
	CBFindResult res = CBAssociativeArrayFind(&self->valueWrites, key);
	if (res.found){
		CBAssociativeArrayDelete(&self->valueWrites, res.position, false);
		// Only continue if the value is also in the index. Else we do not want to try and delete anything since it isn't there.
		if (NOT CBAssociativeArrayFind(&self->index, key).found){
			free(keyPtr);
			return true;
		}
	}
	// See if key exists to be deleted already
	res = CBAssociativeArrayFind(&self->deleteKeys, key);
	if (NOT res.found) {
		// Does already exist so insert into array
		if (NOT CBAssociativeArrayInsert(&self->deleteKeys, keyPtr, res.position, NULL)) {
			CBLogError("Failed to insert a deletion element into the deleteKeys array.");
			CBDatabaseClearPending(self);
			free(keyPtr);
			return false;
		}
	}
	return true;
}
bool CBDatabaseWriteConcatenatedValue(CBDatabase * self, uint8_t * key, uint8_t numDataParts, uint8_t ** data, uint32_t * dataSize){
	// Create element
	uint32_t size = 0;
	for (uint8_t x = 0; x < numDataParts; x++)
		size += dataSize[x];
	uint8_t * keyPtr = malloc(*key + 5 + size);
	if (NOT keyPtr) {
		CBLogError("Failed to allocate memory for a value write element.");
		CBDatabaseClearPending(self);
		return false;
	}
	memcpy(keyPtr, key, *key + 1);
	uint32_t * sizePtr = (uint32_t *)(keyPtr + *key + 1);
	*sizePtr = size;
	uint8_t * dataPtr = (uint8_t *)(sizePtr + 1);
	// Add data
	for (uint8_t x = 0; x < numDataParts; x++){
		memcpy(dataPtr, data[x], dataSize[x]);
		dataPtr += dataSize[x];
	}
	return CBDatabaseAddWriteValue(self, keyPtr);
}
bool CBDatabaseWriteValue(CBDatabase * self, uint8_t * key, uint8_t * data, uint32_t size){
	// Create element
	uint8_t * keyPtr = malloc(*key + 5 + size);
	if (NOT keyPtr) {
		CBLogError("Failed to allocate memory for a value write element.");
		CBDatabaseClearPending(self);
		return false;
	}
	memcpy(keyPtr, key, *key + 1);
	uint32_t * sizePtr = (uint32_t *)(keyPtr + *key + 1);
	*sizePtr = size;
	uint8_t * dataPtr = (uint8_t *)(sizePtr + 1);
	memcpy(dataPtr, data, size);
	return CBDatabaseAddWriteValue(self, keyPtr);
}
