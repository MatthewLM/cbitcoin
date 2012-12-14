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
	self->dataDir = dataDir;
	self->logError = logError;
	self->numValueWrites = 0;
	self->deleteKeys = NULL;
	self->numDeleteKeys = 0;
	// Check data consistency
	if (NOT CBBlockChainStorageEnsureConsistent((uint64_t) self)){
		free(self);
		logError("The database is inconsistent and could not be recovered in CBNewBlockChainStorage");
		return 0;
	}
	// Load index
	if (NOT CBInitAssociativeArray(&self->index)){
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
		if (read(indexRead, data, 4) != 4){
			close(indexRead);
			free(self);
			logError("Could not read the number of values from the database index.");
			return 0;
		}
		self->numValues = CBArrayToInt32(data, 0);
		// Get the last file
		if (read(indexRead, data, 2) != 2) {
			close(indexRead);
			free(self);
			logError("Could not read the last file from the database index.");
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
		self->nextIndexPos = 10;
		// Now read index
		for (uint32_t x = 0; x < self->numValues; x++) {
			// Get the key size.
			uint8_t keyLen;
			if (read(indexRead, &keyLen, 1) != 1) {
				close(indexRead);
				CBFreeBlockChainStorage((uint64_t)self);
				logError("Could not read a key length for an index entry from the database index.");
				return 0;
			}
			// Create the key-value index data
			uint8_t * keyVal = malloc(sizeof(CBIndexValue) + 1 + keyLen);
			*keyVal = keyLen;
			// Add key data to index
			if (read(indexRead, keyVal + 1, keyLen) != keyLen) {
				close(indexRead);
				CBFreeBlockChainStorage((uint64_t)self);
				logError("Could not read a key for an index entry from the database index.");
				return 0;
			}
			// Add value
			if (read(indexRead, data, 10) != 10) {
				close(indexRead);
				CBFreeBlockChainStorage((uint64_t)self);
				logError("Could not read a value for an index entry from the database index.");
				return 0;
			}
			CBIndexValue * value = (CBIndexValue *)(keyVal + keyLen + 1);
			value->fileID = CBArrayToInt16(data, 0);
			value->pos = CBArrayToInt32(data, 2);
			value->length = CBArrayToInt32(data, 6);
			value->indexPos = self->nextIndexPos;
			self->nextIndexPos += 11 + keyLen;
			if (NOT CBAssociativeArrayInsert(&self->index, keyVal, CBAssociativeArrayFind(&self->index, keyVal), NULL)){
				close(indexRead);
				CBFreeBlockChainStorage((uint64_t)self);
				logError("Could not insert data into the database index.");
				return 0;
			}
		}
		// Close file
		close(indexRead);
	}else{
		// Else empty index
		self->nextIndexPos = 10; // First index starts after 10 bytes
		self->lastFile = 2;
		int indexAppend = open(filemame, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
		if (NOT indexAppend) {
			free(self);
			CBFreeBlockChainStorage((uint64_t)self);
			logError("Could not create the database index.");
			return 0;
		}
		// Write the number of values as 0
		// Write last file as 2
		// Write size of last file as 0
		if (write(indexAppend, (uint8_t [10]){0,0,0,0,2,0,0,0,0,0}, 10) != 10) {
			close(indexAppend);
			CBFreeBlockChainStorage((uint64_t)self);
			logError("Could not initially write the size of the last data file to the database index.");
			return 0;
		}
		// Synchronise.
		if(fsync(indexAppend)){
			close(indexAppend);
			CBFreeBlockChainStorage((uint64_t)self);
			logError("Could not initially synchronise the database index.");
			return 0;
		}
		close(indexAppend);
	}
	// Load deletion index
	if (NOT CBInitAssociativeArray(&self->deletionIndex)){
		free(self);
		CBFreeBlockChainStorage((uint64_t)self);
		logError("Could not initialise the database deletion index.");
		return 0;
	}
	filemame[strlen(dataDir)] = '1';
	// First check that the index exists
	if (NOT access(filemame, F_OK)) {
		// Can read deletion index
		int indexRead = open(filemame, O_RDONLY);
		if (indexRead == -1) {
			CBFreeBlockChainStorage((uint64_t)self);
			logError("Could not open the database deletion index.");
			return 0;
		}
		// Read deletion entry number
		uint8_t data[11];
		// Get the number of values
		if (read(indexRead, data, 4) != 4) {
			close(indexRead);
			CBFreeBlockChainStorage((uint64_t)self);
			logError("Could not read the number of entries from the database deletion index.");
			return 0;
		}
		self->numDeletionValues = CBArrayToInt32(data, 0);
		// Now read index
		for (uint32_t x = 0; x < self->numDeletionValues; x++) {
			// Create the key-value index data
			CBDeletedSection * section = malloc(sizeof(*section));
			if (NOT section) {
				close(indexRead);
				CBFreeBlockChainStorage((uint64_t)self);
				logError("Could not allocate memory for deleted section value.");
			}
			if (read(indexRead, data, 11) != 11) {
				close(indexRead);
				CBFreeBlockChainStorage((uint64_t)self);
				logError("Could not read entry from the database deletion index.");
				return 0;
			}
			// Add data to index
			memcpy(section->key, data, 5);
			section->fileID = CBArrayToInt16(data, 5);
			section->pos = CBArrayToInt32(data, 7);
			if (NOT CBAssociativeArrayInsert(&self->deletionIndex, (uint8_t *)section, CBAssociativeArrayFind(&self->deletionIndex, data), NULL)) {
				close(indexRead);
				CBFreeBlockChainStorage((uint64_t)self);
				logError("Could not insert entry to the database deletion index.");
				return 0;
			}
		}
	}else{
		// Else empty deletion index
		int indexAppend = open(filemame, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
		if (NOT indexAppend) {
			free(self);
			CBFreeBlockChainStorage((uint64_t)self);
			logError("Could not create the database deletion index.");
			return 0;
		}
		// Write the number of entries as 0
		if (write(indexAppend, (uint8_t []){0,0,0,0}, 4) != 4) {
			close(indexAppend);
			CBFreeBlockChainStorage((uint64_t)self);
			logError("Could not initially write the number of entries to the database deletion index.");
			return 0;
		}
		// Synchronise.
		if(fsync(indexAppend)){
			close(indexAppend);
			CBFreeBlockChainStorage((uint64_t)self);
			logError("Could not initially synchronise the database deletion index.");
			return 0;
		}
		close(indexAppend);
	}
	return (uint64_t)self;
}
void CBFreeBlockChainStorage(uint64_t iself){
	CBBlockChainStorage * self = (CBBlockChainStorage *)iself;
	CBResetBlockChainStorage(iself);
	CBFreeAssociativeArray(&self->index,true);
	CBFreeAssociativeArray(&self->deletionIndex,true);
	free(self);
}
void CBResetBlockChainStorage(uint64_t iself){
	CBBlockChainStorage * self = (CBBlockChainStorage *)iself;
	self->numValueWrites = 0;
	free(self->deleteKeys);
	self->deleteKeys = NULL;
	self->numDeleteKeys = 0;
	free(self->changeKeys);
	self->changeKeys = NULL;
	self->numChangeKeys = 0;
}
bool CBBlockChainStorageChangeKey(uint64_t iself, uint8_t * previousKey, uint8_t * newKey){
	CBBlockChainStorage * self = (CBBlockChainStorage *)iself;
	self->changeKeys = realloc(self->changeKeys, sizeof(*self->changeKeys) * ++self->numChangeKeys);
	if (NOT self->changeKeys) {
		self->logError("Failed to reaalocate memory for the key change array");
		CBResetBlockChainStorage(iself);
		return false;
	}
	self->changeKeys[self->numChangeKeys - 1][0] = previousKey;
	self->changeKeys[self->numChangeKeys - 1][1] = newKey;
	return true;
}
bool CBBlockChainStorageWriteValue(uint64_t iself, uint8_t * key, uint8_t * data, uint32_t dataSize, uint32_t offset, uint32_t totalSize){
	CBBlockChainStorage * self = (CBBlockChainStorage *) iself;
	if (self->numValueWrites == CB_MAX_VALUE_WRITES) {
		self->logError("Too many writes to the database at once.");
		CBResetBlockChainStorage(iself);
		return false;
	}
	self->valueWrites[self->numValueWrites].data = data;
	self->valueWrites[self->numValueWrites].key = key;
	self->valueWrites[self->numValueWrites].offset = offset;
	self->valueWrites[self->numValueWrites].totalLen = totalSize;
	self->valueWrites[self->numValueWrites].dataLen = dataSize;
	self->valueWrites[self->numValueWrites].allocLen = dataSize;
	self->numValueWrites++;
	return true;
}
bool CBBlockChainStorageReadValue(uint64_t iself, uint8_t * key, uint8_t * data, uint32_t dataSize, uint32_t offset){
	CBBlockChainStorage * self = (CBBlockChainStorage *)iself;
	// Look in index for value
	CBFindResult res = CBAssociativeArrayFind(&self->index, key);
	if (NOT res.found){
		self->logError("Could not find a value for a key.");
		return false;
	}
	CBIndexValue * val = (CBIndexValue *)(res.node->elements[res.pos] + *res.node->elements[res.pos] + 1);
	// Try opening file
	char filename[strlen(self->dataDir) + 10];
	sprintf(filename,"%s%i.dat",self->dataDir,val->fileID);
	int file = open(filename, O_RDONLY);
	if (NOT file) {
		self->logError("Could not open file for a value.");
		return false;
	}
	if (lseek(file, val->pos + offset, SEEK_SET) == -1){
		self->logError("Could not read seek file for value.");
		return false;
	}
	if (read(file, data, dataSize) != dataSize) {
		self->logError("Could not read from file for value.");
		return false; 
	}
	return true;
}
bool CBBlockChainStorageCommitData(uint64_t iself){
	CBBlockChainStorage * self = (CBBlockChainStorage *)iself;
	char filename[strlen(self->dataDir) + 8];
	memcpy(filename,self->dataDir,strlen(self->dataDir));
	// Open the log file
	strcpy(filename + strlen(self->dataDir),"log.dat");
	int logFile = open(filename, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
	if (NOT logFile) {
		self->logError("The log file for overwritting could not be opened.");
		CBResetBlockChainStorage(iself);
		return false;
	}
	// Write the previous sizes for the indexes to the log file
	struct stat inf;
	uint8_t data[15];
	data[0] = 1; // Log file active.
	strcpy(filename + strlen(self->dataDir),"0.dat");
	if (stat(filename, &inf)) {
		self->logError("Information for the index file could not be obtained.");
		close(logFile);
		CBResetBlockChainStorage(iself);
		return false;
	}
	CBInt32ToArray(data, 1, inf.st_size);
	strcpy(filename + strlen(self->dataDir),"1.dat");
	if (stat(filename, &inf)) {
		self->logError("Information for the deletion index file could not be obtained.");
		close(logFile);
		CBResetBlockChainStorage(iself);
		return false;
	}
	CBInt32ToArray(data, 5, inf.st_size);
	CBInt16ToArray(data, 9, self->lastFile);
	CBInt32ToArray(data, 11, self->lastSize);
	if (write(logFile, data, 15) != 15) {
		self->logError("Could not write previous size information to the log-file.");
		close(logFile);
		CBResetBlockChainStorage(iself);
		return false;
	}
	// Sync log file, so that it is now active with the information of the previous file sizes.
	if (fsync(logFile)){
		self->logError("Failed to sync the log file");
		CBResetBlockChainStorage(iself);
		return false;
	}
	// Sync directory for log file
	filename[strlen(self->dataDir)] = '\0';
	int dir = open(filename, 0);
	if (fsync(dir)) {
		self->logError("Failed to synchronise the directory during a commit.");
		CBResetBlockChainStorage(iself);
		return false;
	}
	uint32_t lastNumVals = self->numValues;
	uint32_t lastFileSize = self->lastSize;
	uint32_t prevDeletionEntryNum = self->numDeletionValues;
	// Go through each key
	for (uint8_t x = 0; x < self->numValueWrites; x++) {
		// Check for key in index
		CBFindResult res = CBAssociativeArrayFind(&self->index, self->valueWrites[x].key);
		if (res.found) {
			// Exists in index so we are overwriting data.
			// See if the data can be overwritten.
			uint8_t * indexKey = res.node->elements[res.pos];
			CBIndexValue * indexValue = (CBIndexValue *)(indexKey + *indexKey + 1);
			if (indexValue->length >= self->valueWrites[x].totalLen){
				// We are going to overwrite the previous data.
				if (NOT CBBlockChainStorageAddOverwrite(self, indexValue->fileID, self->valueWrites[x].data, indexValue->pos + self->valueWrites[x].offset, self->valueWrites[x].dataLen, logFile, dir)){
					self->logError("Failed to add an overwrite operation to overwrite a previous value.");
					close(logFile);
					CBResetBlockChainStorage(iself);
					return false;
				}
				if (indexValue->length > self->valueWrites[x].totalLen) {
					// Change indexed length and mark the remaining length as deleted
					if (NOT CBBlockChainStorageAddDeletionEntry(self, indexValue->fileID, indexValue->pos + self->valueWrites[x].totalLen, indexValue->length - self->valueWrites[x].totalLen, logFile, dir)){
						self->logError("Failed to add a deletion entry when overwriting a previous value with a smaller one.");
						close(logFile);
						CBResetBlockChainStorage(iself);
						return false;
					}
					indexValue->length = self->valueWrites[x].totalLen;
					uint8_t newLength[4];
					CBInt32ToArray(newLength, 0, indexValue->length);
					if (NOT CBBlockChainStorageAddOverwrite(self, 0, newLength, indexValue->indexPos + 7 + *indexKey, 4, logFile, dir)) {
						self->logError("Failed to add an overwrite operation to write the new length of a value to the database index.");
						close(logFile);
						CBResetBlockChainStorage(iself);
						return false;
					}
				}
			}else{
				// We will mark the previous data as deleted and store data elsewhere, unless the data has been marked as deleted already (by having zero length in the index).
				// THIS COMPLETELY IGNORES PREVIOUS DATA. THE PREVIOUS DATA IS LOST IF THE NEW VALUE IS LARGER.
				if (indexValue->length && NOT CBBlockChainStorageAddDeletionEntry(self, indexValue->fileID, indexValue->pos, indexValue->length, logFile, dir)){
					self->logError("Failed to add a deletion entry for an old value when replacing it with a larger one.");
					close(logFile);
					CBResetBlockChainStorage(iself);
					return false;
				}
				if (NOT CBBlockChainStorageAddValue(self, self->valueWrites[x], indexValue, logFile, dir)) {
					self->logError("Failed to add a value to the database with a previously exiting key.");
					close(logFile);
					CBResetBlockChainStorage(iself);
					return false;
				}
				// Write index
				uint8_t data[10];
				CBInt16ToArray(data, 0, indexValue->fileID);
				CBInt32ToArray(data, 2, indexValue->pos);
				CBInt32ToArray(data, 6, indexValue->length);
				if (NOT CBBlockChainStorageAddOverwrite(self, 0, data, indexValue->indexPos + 1 + *indexKey, 10, logFile, dir)) {
					self->logError("Failed to add an overwrite operation for updating the index for writting data in a new location.");
					close(logFile);
					CBResetBlockChainStorage(iself);
					return false;
				}
			}
		}else{
			// Does not exist in index, so we are creating new data
			// New index value data
			uint8_t * indexKey = malloc(sizeof(CBIndexValue) + 1 + *self->valueWrites[x].key);
			if (NOT CBBlockChainStorageAddValue(self, self->valueWrites[x], (CBIndexValue *)(indexKey + 1 + *self->valueWrites[x].key), logFile, dir)) {
				self->logError("Failed to add a value to the database with a new key.");
				close(logFile);
				CBResetBlockChainStorage(iself);
				return false;
			}
			// Add index
			memcpy(indexKey, self->valueWrites[x].key, *self->valueWrites[x].key + 1);
			CBIndexValue * indexValue = (CBIndexValue *)(indexKey + *self->valueWrites[x].key + 1);
			// The index position is taken from the next index position.
			indexValue->indexPos = self->nextIndexPos;
			// Increase index position past appende index entry.
			self->nextIndexPos += 10 + 1 + *indexKey;
			// Insert index into array.
			if (NOT CBAssociativeArrayInsert(&self->index, indexKey, CBAssociativeArrayFind(&self->index, indexKey), NULL)) {
				self->logError("Failed to insert new index entry.");
				close(logFile);
				CBResetBlockChainStorage(iself);
				return false;
			}
			// Save to file
			if (NOT CBBlockChainStorageAppend(self, 0, indexKey, *indexKey + 1)) {
				self->logError("Failed to add an append operation for a new index entry key.");
				close(logFile);
				CBResetBlockChainStorage(iself);
				return false;
			}
			uint8_t data[10];
			CBInt16ToArray(data, 0, indexValue->fileID);
			CBInt32ToArray(data, 2, indexValue->pos);
			CBInt32ToArray(data, 6, indexValue->length);
			if (NOT CBBlockChainStorageAppend(self, 0, data, 10)) {
				self->logError("Failed to add an append operation for a new index entry.");
				close(logFile);
				CBResetBlockChainStorage(iself);
				return false;
			}
			self->numValues++;
		}
	}
	if (lastNumVals != self->numValues
		|| lastFileSize != self->lastSize) {
		// Change index information.
		CBInt32ToArray(data, 0, self->numValues);
		CBInt16ToArray(data, 4, self->lastFile);
		CBInt32ToArray(data, 6, self->lastSize);
		if (NOT CBBlockChainStorageAddOverwrite(self, 0, data, 0, 10, logFile, dir)) {
			self->logError("Failed to update the information for the index file.");
			close(logFile);
			CBResetBlockChainStorage(iself);
			return false;
		}
	}
	// Delete values
	for (uint32_t x = 0; x < self->numDeleteKeys; x++) {
		// Find index entry
		CBFindResult res = CBAssociativeArrayFind(&self->index, self->deleteKeys[x]);
		uint8_t * indexKey = res.node->elements[res.pos];
		CBIndexValue * indexVal = (CBIndexValue *)(indexKey + *indexKey + 1);
		// Create deletion entry for the data
		if (NOT CBBlockChainStorageAddDeletionEntry(self, indexVal->fileID, indexVal->pos, indexVal->length, logFile, dir)) {
			self->logError("Failed to create a deletion entry for a key-value.");
			close(logFile);
			CBResetBlockChainStorage(iself);
			return false;
		}
		// Make the length 0, signifying deletion of the data
		indexVal->length = 0;
		CBInt32ToArray(data, 0, 0);
		if (NOT CBBlockChainStorageAddOverwrite(self, 0, data, indexVal->indexPos + 7 + *indexKey, 4, logFile, dir)) {
			self->logError("Failed to overwrite the index entry's length with 0 to signify deletion.");
			close(logFile);
			CBResetBlockChainStorage(iself);
			return false;
		}
	}
	if (prevDeletionEntryNum != self->numDeletionValues) {
		// New number of deletion entries.
		CBInt32ToArray(data, 0, self->numDeletionValues);
		if (NOT CBBlockChainStorageAddOverwrite(self, 1, data, 0, 4, logFile, dir)) {
			self->logError("Failed to update the number of entries for the deletion index file.");
			close(logFile);
			CBResetBlockChainStorage(iself);
			return false;
		}
	}
	// Change keys
	for (uint32_t x = 0; x < self->numChangeKeys; x++) {
		// Find key
		CBFindResult res = CBAssociativeArrayFind(&self->index, self->changeKeys[x][0]);
		// Obtain index entry
		uint8_t * indexKey = res.node->elements[res.pos];
		// Delete key
		CBAssociativeArrayDelete(&self->index, res);
		// Change key
		// Copy in new key.
		memcpy(indexKey + 1,self->changeKeys[x][1] + 1,*self->changeKeys[x][1]);
		// Re-insert
		if (NOT CBAssociativeArrayInsert(&self->index, indexKey, CBAssociativeArrayFind(&self->index, indexKey), NULL)) {
			self->logError("Failed to insert an index entry with a changed key.");
			close(logFile);
			CBResetBlockChainStorage(iself);
			return false;
		}
		// Overwrite key on disk
		if (NOT CBBlockChainStorageAddOverwrite(self, 0, indexKey + 1, ((CBIndexValue *)(indexKey + 1 + *indexKey))->indexPos + 1, *indexKey, logFile, dir)) {
			self->logError("Failed to overwrite a key in the index.");
			close(logFile);
			CBResetBlockChainStorage(iself);
			return false;
		}
	}
	// Sync directory
	if (fsync(dir)) {
		self->logError("Failed to synchronise the directory during a commit.");
		close(logFile);
		CBResetBlockChainStorage(iself);
		return false;
	}
	close(dir);
	// Now we are done, make the logfile inactive. Errors do not matter here.
	data[0] = 0;
	lseek(logFile, 0, SEEK_SET);
	write(logFile, data, 1);
	fsync(logFile);
	close(logFile);
	CBResetBlockChainStorage(iself);
	return true;
}
bool CBBlockChainStorageEnsureConsistent(uint64_t iself){
	CBBlockChainStorage * self = (CBBlockChainStorage *)iself;
	char filename[strlen(self->dataDir) + 10];
	memcpy(filename,self->dataDir,strlen(self->dataDir));
	strcpy(filename + strlen(self->dataDir),"log.dat");
	// Determine if the logfile has been created or not
	if (access(filename, F_OK))
		// Not created, so no history.
		return true;
	// Open the logfile for reading
	int logFile = open(filename, O_RDWR);
	if (NOT logFile) {
		self->logError("The log file for reading could not be opened.");
		return false;
	}
	// Check if the logfile is active or not.
	uint8_t data[14];
	if (read(logFile, data, 1) != 1) {
		self->logError("Failed reading the logfile.");
		return false;
	}
	if (NOT data[0])
		// Not active
		return true;
	// Active, therefore we need to attempt rollback.
	if (read(logFile, data, 14) != 14) {
		self->logError("Failed reading the logfile.");
		return false;
	}
	// Truncate files
	strcpy(filename + strlen(self->dataDir), "0.dat");
	if (truncate(filename, CBArrayToInt32(data, 0))) {
		self->logError("Failed to truncate the index file down to the previous size.");
		return false;
	}
	filename[strlen(self->dataDir)] = '1';
	if (truncate(filename, CBArrayToInt32(data, 4))) {
		self->logError("Failed to truncate the deletion index file down to the previous size.");
		return false;
	}
	uint16_t lastFile = CBArrayToInt16(data, 8);
	sprintf(filename + strlen(self->dataDir), "%u.dat", lastFile);
	if (truncate(filename, CBArrayToInt32(data, 10))) {
		self->logError("Failed to truncate the last file down to the previous size.");
		return false;
	}
	// Check if a new file had been created. If it has, delete it.
	sprintf(filename + strlen(self->dataDir), "%u.dat", lastFile + 1);
	if (NOT access(filename, F_OK))
		remove(filename);
	// Reverse overwrite operations
	struct stat inf;
	if (fstat(logFile, &inf)) {
		self->logError("Could not obtain information for the log file.");
		return false;
	}
	for (uint32_t c = 15; c < inf.st_size;) {
		// Read file ID, offset and size
		uint8_t data[14];
		if (read(logFile, data, 14) != 14) {
			self->logError("Could not read from the log file.");
			return false;
		}
		uint32_t dataLen = CBArrayToInt32(data, 10);
		// Read previous data
		uint8_t * prevData = malloc(dataLen);
		if (NOT prevData) {
			self->logError("Could not allocate memory for previous data from the log file.");
			return false;
		}
		if (read(logFile, prevData, dataLen) != dataLen) {
			self->logError("Could not read previous data from the log file.");
			return false;
		}
		// Write the data to the file
		sprintf(filename + strlen(self->dataDir), "%u.dat", CBArrayToInt16(data, 0));
		int file = open(filename, O_WRONLY);
		if (lseek(file, CBArrayToInt64(data, 2), SEEK_SET) == -1){
			self->logError("Could not seek the file for writting the previous data.");
			return false;
		}
		if (write(file, prevData, dataLen) != dataLen) {
			self->logError("Could not write previous data back into the file.");
			return false;
		}
		free(prevData);
		c += 14 + dataLen;
	}
	// Sync directory
	filename[strlen(self->dataDir)] = '\0';
	int dir = open(filename, 0);
	if (fsync(dir)) {
		self->logError("Failed to synchronise the directory when recovering database.");
		return false;
	}
	// Now we are done, make the logfile inactive
	data[0] = 0;
	write(logFile, data, 1);
	fsync(logFile);
	close(logFile);
	return true;
}
uint32_t CBBlockChainStorageGetLength(uint64_t iself,uint8_t key[6]){
	CBBlockChainStorage * self = (CBBlockChainStorage *)iself;
	// Look in index for value
	CBFindResult res = CBAssociativeArrayFind(&self->index, key);
	if (NOT res.found)
		return 0;
	return ((CBIndexValue *)res.node->elements[res.pos])->length;
}
bool CBBlockChainStorageAddDeletionEntry(CBBlockChainStorage * self, uint16_t fileID, uint32_t pos, uint32_t len, int logFile, int dir){
	// First look for inactive deletion that can be used.
	CBFindResult res;
	res.node = self->deletionIndex.root;
	for (; res.node->children[0]; res.node = res.node->children[0]);
	if (res.node->numElements && NOT res.node->elements[0][0]) {
		// Inactive deletion entry. We will use this one.
		CBDeletedSection * section = (CBDeletedSection *)res.node->elements[0];
		section->fileID = fileID;
		section->pos = pos;
		section->key[0] = 1; // Re-activate.
		CBInt32ToArrayBigEndian(section->key, 1, len);
		// Remove from index
		res.pos = 0;
		CBAssociativeArrayDelete(&self->deletionIndex, res);
		// Re-insert into index with new key
		if (NOT CBAssociativeArrayInsert(&self->deletionIndex, section->key, CBAssociativeArrayFind(&self->deletionIndex, (uint8_t *)section), NULL)){
			self->logError("Could not insert replacement deletion entry into the array.");
			return false;
		}
		// Overwrite deletion index
		uint8_t data[11];
		memcpy(data, section->key, 5);
		CBInt16ToArray(data, 5, section->fileID);
		CBInt32ToArray(data, 7, section->pos);
		if (NOT CBBlockChainStorageAddOverwrite(self, 1, data, 4 + section->indexPos * 11, 11, logFile, dir)) {
			self->logError("There was an error when adding a deletion entry by overwritting an inactive one.");
			return false;
		}
	}else{
		// We need a new entry.
		CBDeletedSection * section = malloc(sizeof(*section));
		section->fileID = fileID;
		section->pos = pos;
		section->key[0] = 1;
		CBInt32ToArrayBigEndian(section->key, 1, len);
		section->indexPos = self->numDeletionValues;
		// Insert the entry into the array
		if (NOT CBAssociativeArrayInsert(&self->deletionIndex, section->key, CBAssociativeArrayFind(&self->deletionIndex, section->key), NULL)){
			self->logError("Could not insert new deletion entry into the array.");
			return false;
		}
		self->numDeletionValues++;
		// Append deletion index
		uint8_t data[11];
		memcpy(data, section->key, 5);
		CBInt16ToArray(data, 5, section->fileID);
		CBInt32ToArray(data, 7, section->pos);
		if (NOT CBBlockChainStorageAppend(self, 1, data, 11)) {
			self->logError("There was an error when adding a deletion entry by appending a new one.");
			return false;
		}
	}
	return true;
}
bool CBBlockChainStorageAddOverwrite(CBBlockChainStorage * self, uint16_t fileID, uint8_t * data, uint64_t offset, uint32_t dataLen, int logFile, int dir){
	// Execute the overwrite and write rollback information to the log file.
	// Open the file to overwrite
	char filename[strlen(self->dataDir) + 10];
	sprintf(filename, "%s%i.dat",self->dataDir,fileID);
	int file = open(filename, O_RDWR);
	if (NOT file) {
		self->logError("The data file for overwritting could not be opened.");
		return false;
	}
	// Write to the log file the rollback information for the changes being made.
	uint32_t dataTempLen = dataLen + 14;
	uint8_t * dataTemp = malloc(dataTempLen);
	if (NOT dataTemp) {
		self->logError("Could not allocate memory for temporary memory for writting to the transaction log file.");
		return false;
	}
	// Write file ID
	CBInt16ToArray(dataTemp, 0, fileID);
	// Write offset
	CBInt64ToArray(dataTemp, 2, offset);
	// Write data size
	CBInt32ToArray(dataTemp, 10, dataLen);
	// Write old data
	if (lseek(file, offset, SEEK_SET) == -1){
		self->logError("Could not seek to the read position for previous data in a file to be overwritten.");
		return false;
	}
	if (read(file, dataTemp + 14, dataLen) != dataLen) {
		self->logError("Could not read the previous data to be overwritten.");
		return false;
	}
	if (write(logFile, dataTemp, dataTempLen) != dataTempLen) {
		self->logError("Could not write the backup information to the transaction log file.");
		return false;
	}
	free(dataTemp);
	// Sync log file
	if (fsync(logFile)){
		self->logError("Failed to sync the log file");
		return false;
	}
	// Sync directory for log file
	if (fsync(dir)) {
		self->logError("Failed to synchronise the directory for the log file when overwritting a file.");
		return false;
	}
	// Execute overwrite
	if (lseek(file, offset, SEEK_SET) == -1){
		self->logError("Could not seek the file to overwrite data.");
		return false;
	}
	if (write(file, data, dataLen) != dataLen) {
		self->logError("Could not overwrite file %u at position %u with length %u",fileID,offset,dataLen);
		return false;
	}
	// Synchonise file.
	if (fsync(file)) {
		self->logError("Could not synchornise file %u during an overwrite at position %u with length %u",fileID,offset,dataLen);
		return false;
	}
	// Close file
	close(file);
	return true;
}
bool CBBlockChainStorageAddValue(CBBlockChainStorage * self, CBWriteValue writeValue, CBIndexValue * indexValue, int logFile, int dir){
	// Look for a deleted section to use
	CBFindResult res = CBBlockChainStorageGetDeletedSection(self, writeValue.totalLen);
	if (res.found) {
		// ??? Is there a more effecient method for changing the key of the associative array than deleting and re-inserting with new key?
		// Found a deleted section we can use for the data
		CBDeletedSection * section = (CBDeletedSection *)res.node->elements[res.pos];
		uint32_t sectionLen = CBArrayToInt32BigEndian(section->key,1);
		uint32_t newSectionLen = sectionLen - writeValue.totalLen;
		// Use the section
		if (NOT CBBlockChainStorageAddOverwrite(self, section->fileID, writeValue.data, section->pos + newSectionLen + writeValue.offset, writeValue.dataLen, logFile, dir)){
			self->logError("Failed to add an overwrite operation to overwrite a previously deleted section with new data.");
			return false;
		}
		// Update index data
		indexValue->fileID = section->fileID;
		indexValue->length = writeValue.totalLen;
		indexValue->pos = section->pos + newSectionLen;
		// Change deletion index
		// Remove from array
		CBAssociativeArrayDelete(&self->deletionIndex, res);
		if (newSectionLen) {
			// Change deletion section length
			CBInt32ToArrayBigEndian(section->key, 1, newSectionLen);
		}else{
			// Make deleted section inactive.
			section->key[0] = 0; // Not active
		}
		// Re-insert into array
		if (NOT CBAssociativeArrayInsert(&self->deletionIndex, (uint8_t *)section, CBAssociativeArrayFind(&self->deletionIndex, (uint8_t *)section), NULL)){
			self->logError("Failed to change the key of a deleted section to inactive.");
			return false;
		}
		if (NOT CBBlockChainStorageAddOverwrite(self, 1, (uint8_t *)section, 4 + 11 * section->indexPos, 5, logFile, dir)) {
			self->logError("Failed to add an overwrite operation to update the deletion index for writting data to a deleted section");
			return false;
		}
	}else{
		// No suitable deleted section, therefore append the data
		indexValue->pos = self->lastSize; // Update position in index.
		if (writeValue.totalLen > UINT32_MAX - self->lastSize){
			self->lastFile++;
			self->lastSize = writeValue.totalLen;
		}else
			self->lastSize += writeValue.totalLen;
		if (writeValue.offset) {
			// Append upto offset
			if (NOT CBBlockChainStorageAppend(self, self->lastFile, NULL, writeValue.offset)) {
				self->logError("Failed to add an append operation for the offset to a value replacing an older one.");
				return false;
			}
		}
		if (NOT CBBlockChainStorageAppend(self, self->lastFile, writeValue.data, writeValue.dataLen)) {
			self->logError("Failed to add an append operation for the value replacing an older one.");
			return false;
		}
		uint32_t ending = writeValue.totalLen - writeValue.offset - writeValue.dataLen;
		if (ending) {
			// Append ending
			if (NOT CBBlockChainStorageAppend(self, self->lastFile, NULL, ending)) {
				self->logError("Failed to add an append operation for the ending to a value replacing an older one.");
				return false;
			}
		}
		// Update index
		indexValue->fileID = self->lastFile;
		indexValue->length = writeValue.totalLen;
	}
	return true;
}
bool CBBlockChainStorageAppend(CBBlockChainStorage * self, uint16_t fileID, uint8_t * data, uint32_t dataLen){
	// Execute the append operation
	char filename[strlen(self->dataDir) + 10];
	sprintf(filename,"%s%i.dat",self->dataDir,fileID);
	int file = open(filename, O_WRONLY | O_APPEND | O_CREAT, S_IRUSR | S_IWUSR);
	if (NOT file) {
		self->logError("Could not open file %u for appending.",fileID);
		close(file);
		return false;
	}
	if (NOT data) {
		for (uint32_t x = 0; x < dataLen; x++) {
			if (write(file, &data, 1) != 1) {
				self->logError("Failed to append data to file %u.",fileID);
				close(file);
				return false;
			}
		}
	}else if (write(file, data, dataLen) != dataLen) {
		self->logError("Failed to append data to file %u.",fileID);
		close(file);
		return false;
	}
	// Synchronise file
	if (fsync(file)) {
		self->logError("Failed to synchronise when appending data to file %u.",fileID);
		close(file);
		return false;
	}
	// Close file
	close(file);
	return true;
}
CBFindResult CBBlockChainStorageGetDeletedSection(CBBlockChainStorage * self, uint32_t length){
	CBFindResult res;
	res.node = self->deletionIndex.root;
	for (; res.node->children[res.node->numElements]; res.node = res.node->children[res.node->numElements]);
	res.pos = res.node->numElements - 1;
	res.found = res.node->numElements && CBArrayToInt32BigEndian(res.node->elements[res.pos], 1) >= length;
	return res;
}
bool CBBlockChainStorageRemoveValue(uint64_t iself, uint8_t * key){
	CBBlockChainStorage * self = (CBBlockChainStorage *)iself;
	self->deleteKeys = realloc(self->deleteKeys, sizeof(*self->deleteKeys) * ++self->numDeleteKeys);
	if (NOT self->deleteKeys) {
		self->logError("Failed to reaalocate memory for the deleted keys array");
		CBResetBlockChainStorage(iself);
		return false;
	}
	self->deleteKeys[self->numDeleteKeys - 1] = key;
	return true;
}
