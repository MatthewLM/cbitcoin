//
//  CBDatabase.c
//  cbitcoin
//
//  Created by Matthew Mitchell on 03/11/2012.
//  Copyright (c) 2012 Matthew Mitchell
//
//  This file is part of cbitcoin. It is subject to the license terms
//  in the LICENSE file found in the top-level directory of this
//  distribution and at http://www.cbitcoin.com/license.html. No part of
//  cbitcoin, including this file, may be copied, modified, propagated,
//  or distributed except according to the terms contained in the
//  LICENSE file.

//  SEE HEADER FILE FOR DOCUMENTATION

// ??? And range deletion? Delete a range of key values?

#include "CBDatabase.h"

bool CBNewStorageDatabase(CBDepObject * database, char * dataDir, uint32_t commitGap, uint32_t cacheLimit){
	database->ptr = CBNewDatabase(dataDir, "cbitcoin", CB_DATABASE_EXTRA_DATA_SIZE, commitGap, cacheLimit);
	return database->ptr != NULL;
}
bool CBStorageDatabaseStage(CBDepObject database){
	return CBDatabaseStage(database.ptr);
}
bool CBStorageDatabaseRevert(CBDepObject database){
	CBDatabaseClearCurrent(database.ptr);
	return true;
}
void CBFreeStorageDatabase(CBDepObject database){
	CBFreeDatabase(database.ptr);
}
CBDatabase * CBNewDatabase(char * dataDir, char * folder, uint32_t extraDataSize, uint32_t commitGap, uint32_t cacheLimit){
	// Try to create the object
	CBDatabase * self = malloc(sizeof(*self));
	if (NOT CBInitDatabase(self, dataDir, folder, extraDataSize, commitGap, cacheLimit)) {
		free(self);
		CBLogError("Could not initialise a database object.");
		return 0;
	}
	return self;
}
bool CBInitDatabase(CBDatabase * self, char * dataDir, char * folder, uint32_t extraDataSize, uint32_t commitGap, uint32_t cacheLimit){
	self->dataDir = dataDir;
	self->folder = folder;
	self->lastUsedFileType = CB_DATABASE_FILE_TYPE_NONE; // No stored file yet.
	self->commitGap = commitGap;
	self->cacheLimit = cacheLimit;
	self->stagedSize = 0;
	self->lastCommit = CBGetMilliseconds();
	self->hasStaged = false;
	char filename[strlen(self->dataDir) + strlen(self->folder) + 12];
	// Create folder if it doesn't exist
	sprintf(filename, "%s/%s/", self->dataDir, self->folder);
	if (access(filename, F_OK)
		&& (mkdir(filename, S_IRWXU | S_IRWXG)
			|| chmod(filename, S_IRWXU | S_IRWXG))){
		CBLogError("Could not create the folder for a database.");
		return false;
	}
	// Check data consistency
	if (NOT CBDatabaseEnsureConsistent(self)){
		CBLogError("The database is inconsistent and could not be recovered in CBNewDatabase");
		return false;
	}
	// Load deletion index
	CBInitAssociativeArray(&self->deletionIndex, CBKeyCompare, NULL, free);
	strcat(filename, "del.dat");
	// Check that the index exists
	if (NOT access(filename, F_OK)) {
		// Can read deletion index
		if (NOT CBDatabaseReadAndOpenDeletionIndex(self, filename)) {
			CBLogError("Could not load the database deletion index.");
			CBFreeAssociativeArray(&self->deletionIndex);
			return false;
		}
	}else{
		// Else empty deletion index
		if (NOT CBDatabaseCreateDeletionIndex(self, filename)) {
			CBLogError("Could not create the database deletion index.");
			CBFreeAssociativeArray(&self->deletionIndex);
			return false;
		}
	}
	self->extraDataSize = extraDataSize;
	self->extraDataOnDisk = malloc(extraDataSize);
	// Init transaction changes
	CBInitDatabaseTransactionChanges(&self->staged, self);
	CBInitDatabaseTransactionChanges(&self->current, self);
	// Get the last file and last file size
	uint8_t data[6];
	strcpy(filename + strlen(self->dataDir) + strlen(self->folder) + 2, "val_0.dat");
	// Check that the data file exists.
	if (access(filename, F_OK)) {
		// The data file does not exist.
		self->lastFile = 0;
		self->lastSize = extraDataSize + 6;
		CBDepObject file;
		if (NOT CBFileOpen(&file, filename, true)) {
			CBLogError("Could not open a new first data file.");
			CBFreeAssociativeArray(&self->deletionIndex);
			CBFreeDatabaseTransactionChanges(&self->staged);
			CBFreeDatabaseTransactionChanges(&self->current);
			return false;
		}
		uint8_t initData[6] = {0};
		CBInt32ToArray(initData, 2, self->lastSize);
		if (NOT CBFileAppend(file, initData, 6)) {
			CBFileClose(file);
			CBLogError("Could not write the initial data for the first data file.");
			CBFreeAssociativeArray(&self->deletionIndex);
			CBFreeDatabaseTransactionChanges(&self->staged);
			CBFreeDatabaseTransactionChanges(&self->current);
			return false;
		}
		if (extraDataSize != 0) {
			if (NOT CBFileAppendZeros(file, extraDataSize)) {
				CBFileClose(file);
				CBLogError("Could not append space for extra data.");
				CBFreeAssociativeArray(&self->deletionIndex);
				CBFreeDatabaseTransactionChanges(&self->staged);
				CBFreeDatabaseTransactionChanges(&self->current);
				return false;
			}
			memset(self->staged.extraData, 0, extraDataSize);
			memset(self->current.extraData, 0, extraDataSize);
			memset(self->extraDataOnDisk, 0, extraDataSize);
		}
		CBFileClose(file);
	}else{
		// The data file does exist.
		CBDepObject file;
		if (NOT CBFileOpen(&file, filename, false)) {
			CBLogError("Could not open the data file.");
			CBFreeAssociativeArray(&self->deletionIndex);
			CBFreeDatabaseTransactionChanges(&self->staged);
			CBFreeDatabaseTransactionChanges(&self->current);
			return false;
		}
		if (NOT CBFileRead(file, data, 6)) {
			CBFileClose(file);
			CBLogError("Could not read the last file and last file size from the first data file.");
			CBFreeAssociativeArray(&self->deletionIndex);
			CBFreeDatabaseTransactionChanges(&self->staged);
			CBFreeDatabaseTransactionChanges(&self->current);
			return false;
		}
		self->lastFile = CBArrayToInt16(data, 0);
		self->lastSize = CBArrayToInt32(data, 2);
		if (extraDataSize != 0){
			if (NOT CBFileRead(file, self->extraDataOnDisk, extraDataSize)) {
				CBFileClose(file);
				CBLogError("Could not read the extra data from a database.");
				CBFreeAssociativeArray(&self->deletionIndex);
				CBFreeDatabaseTransactionChanges(&self->staged);
				CBFreeDatabaseTransactionChanges(&self->current);
				return false;
			}
			memcpy(self->current.extraData, self->extraDataOnDisk, extraDataSize);
			memcpy(self->staged.extraData, self->extraDataOnDisk, extraDataSize);
		}
		CBFileClose(file);
	}
	// Init arrays
	CBInitAssociativeArray(&self->itData, CBIndexCompare, NULL, CBFreeIndexItData);
	CBInitAssociativeArray(&self->valueWritingIndexes, CBIndexPtrCompare, NULL, NULL);
	return true;
}
void CBInitDatabaseTransactionChanges(CBDatabaseTransactionChanges * self, CBDatabase * database){
	CBInitAssociativeArray(&self->indexes, CBIndexCompare, NULL, CBFreeIndexTxData);
	self->extraData = malloc(database->extraDataSize);
}
CBDatabaseIndex * CBLoadIndex(CBDatabase * self, uint8_t indexID, uint8_t keySize, uint32_t cacheLimit){
	// Load index
	CBDatabaseIndex * index = malloc(sizeof(*index));
	index->database = self;
	index->ID = indexID;
	index->keySize = keySize;
	index->cacheLimit = cacheLimit;
	// Get the file
	CBDepObject indexFile;
	if (NOT CBDatabaseGetFile(self, &indexFile, CB_DATABASE_FILE_TYPE_INDEX, indexID, 0)) {
		free(index);
		CBLogError("Could not open a database index.");
		return NULL;
	}
	uint32_t len;
	CBFileGetLength(indexFile, &len);
	uint8_t data[12];
	// First check that the index exists
	if (len != 0) {
		// Can be read
		// Get the last file and root information
		if (NOT CBFileRead(indexFile, data, 12)){
			free(index);
			CBFileClose(indexFile);
			CBLogError("Could not read the last file and root node information from a database index.");
			return NULL;
		}
		index->lastFile = CBArrayToInt16(data, 0);
		index->lastSize = CBArrayToInt32(data, 2);
		// Load memory cache
		index->indexCache.cached = true;
		index->indexCache.diskNodeLoc.indexFile = CBArrayToInt16(data, 6);
		index->indexCache.diskNodeLoc.offset = CBArrayToInt16(data, 8);
		// Allocate memory for the root node.
		index->indexCache.cachedNode = malloc(sizeof(*index->indexCache.cachedNode));
		if (NOT CBDatabaseLoadIndexNode(index, index->indexCache.cachedNode, index->indexCache.diskNodeLoc.indexFile, index->indexCache.diskNodeLoc.offset)) {
			free(index->indexCache.cachedNode);
			free(index);
			CBFileClose(indexFile);
			CBLogError("Could not load the root memory cache for a database index.");
			return NULL;
		}
		index->numCached = 1;
		CBIndexNode * first;
		uint32_t numNodes = 1;
		CBIndexNode * node = index->indexCache.cachedNode;
		CBIndexNode * parentStack[7];
		uint8_t parentStackNum = 0;
		if (node->children[0].diskNodeLoc.offset != 0
			|| node->children[0].diskNodeLoc.indexFile != 0) for (uint32_t x = 0;; x++) {
			// For each child in the node, create a cache upto the cache limit.
			bool reached = false;
			for (uint8_t y = 0; y <= node->numElements; y++) {
				// See if the cached data passes limit. Assumes nodes are full for key data.
				if ((index->numCached + 1) * (index->keySize * CB_DATABASE_BTREE_ELEMENTS + sizeof(CBIndexNode)) > index->cacheLimit) {
					reached = true;
					break;
				}
				// Get the child position.
				uint16_t indexID = node->children[y].diskNodeLoc.indexFile;
				uint32_t offset = node->children[y].diskNodeLoc.offset;
				// Allocate memory for node.
				node->children[y].cached = true;
				node->children[y].cachedNode = malloc(sizeof(*node->children[y].cachedNode));
				// Load the node.
				if (NOT CBDatabaseLoadIndexNode(index, node->children[y].cachedNode, indexID, offset)) {
					CBFreeIndex(index);
					CBFileClose(indexFile);
					CBLogError("Could not load memory cache for a database index.");
					return NULL;
				}
			}
			if (x == 0)
				// This node has the first child.
				first = node->children[0].cachedNode;
			if (reached)
				break;
			if (x == numNodes - 1) {
				// Move to next level
				x = 0;
				numNodes = node->numElements;
				parentStack[parentStackNum++] = node;
				node = first;
				// See if this node has any children
				if (node->children[0].diskNodeLoc.offset == 0
					&& node->children[0].diskNodeLoc.indexFile == 0)
					break;
			}else
				// Move to the next node in this level
				node = (parentStackNum ? parentStack[parentStackNum - 1] : index->indexCache.cachedNode)->children[x].cachedNode;
		}
	}else{
		// Cannot be read. Make initial data
		uint16_t nodeLen = ((16 + keySize) * CB_DATABASE_BTREE_ELEMENTS) + 7;
		// Make initial memory data
		index->lastFile = 0;
		index->lastSize = 12 + nodeLen;
		index->newLastFile = 0;
		index->newLastSize = index->lastSize;
		index->numCached = 1;
		index->indexCache.cachedNode = malloc(sizeof(*index->indexCache.cachedNode));
		if (index->indexCache.cachedNode == NULL) {
			free(index);
			CBFileClose(indexFile);
			CBLogError("Could not allocate memory for the initial root node of a new index.");
			return NULL;
		}
		index->indexCache.cachedNode->numElements = 0;
		index->indexCache.diskNodeLoc.indexFile = 0;
		index->indexCache.diskNodeLoc.offset = 12;
		index->indexCache.cached = true;
		// Write to disk initial data
		CBInt16ToArray(data, 0, index->lastSize);
		if (NOT CBFileAppend(indexFile, (uint8_t [12]){
			0,0, // First file is zero
			data[0],data[1],0,0, // Size of file is 12 plus the node length
			0,0, // Root exists in file 0
			12,0,0,0, // Root has an offset of 12,
		}, 12)) {
			free(index->indexCache.cachedNode);
			free(index);
			CBFileClose(indexFile);
			CBLogError("Could not write inital data to the index.");
			return NULL;
		}
		if (NOT CBFileAppendZeros(indexFile, nodeLen)) {
			free(index->indexCache.cachedNode);
			free(index);
			CBFileClose(indexFile);
			CBLogError("Could not write inital data to the index with zeros.");
			return NULL;
		}
	}
	index->newLastSize = index->lastFile;
	index->newLastSize = index->lastSize;
	return index;
}
bool CBDatabaseLoadIndexNode(CBDatabaseIndex * index, CBIndexNode * node, uint16_t nodeFile, uint32_t nodeOffset){
	// Read the node from the disk.
	CBDepObject file;
	if (NOT CBDatabaseGetFile(index->database, &file, CB_DATABASE_FILE_TYPE_INDEX, index->ID, nodeFile)) {
		CBLogError("Could not open an index file when loading a node.");
		return false;
	}
	if (NOT CBFileSeek(file, nodeOffset)){
		CBLogError("Could seek to the node position in an index file when loading a node.");
		return false;
	}
	// Get the number of elements
	if (NOT CBFileRead(file, &node->numElements, 1)) {
		CBLogError("Could not read the number of elements for an index node.");
		return false;
	}
	// Read the elements
	uint8_t data[10];
	for (uint8_t x = 0; x < node->numElements; x++) {
		if (NOT CBFileRead(file, data, 10)) {
			for (uint8_t y = 0; y < x; y++)
				free(node->elements[y].key);
			CBLogError("Could not read the data for an index element.");
			return false;
		}
		node->elements[x].fileID = CBArrayToInt16(data, 0);
		node->elements[x].length = CBArrayToInt32(data, 2);
		node->elements[x].pos = CBArrayToInt32(data, 6);
		// Read the key
		node->elements[x].key = malloc(index->keySize);
		if (NOT CBFileRead(file, node->elements[x].key, index->keySize)) {
			for (uint8_t y = 0; y <= x; y++)
				free(node->elements[y].key);
			CBLogError("Could not read the key of an index element.");
			return false;
		}
	}
	// Seek past space for elements that do not exist
	if (NOT CBFileSeek(file, nodeOffset + 1 + (CB_DATABASE_BTREE_ELEMENTS * (index->keySize + 10)))) {
		for (uint8_t y = 0; y <= node->numElements; y++)
			free(node->elements[y].key);
		CBLogError("Could not seek to the children of a btree node.");
		return false;
	}
	// Read the children information
	for (uint8_t x = 0; x <= node->numElements; x++) {
		// Give non-chached information to begin with.
		node->children[x].cached = false;
		if (NOT CBFileRead(file, data, 6)) {
			for (uint8_t y = 0; y <= node->numElements; y++)
				free(node->elements[y].key);
			CBLogError("Could not read the location of an index child.");
			return false;
		}
		node->children[x].diskNodeLoc.indexFile = CBArrayToInt16(data, 0);
		node->children[x].diskNodeLoc.offset = CBArrayToInt32(data, 2);
	}
	return true;
}
bool CBDatabaseReadAndOpenDeletionIndex(CBDatabase * self, char * filename){
	if (NOT CBFileOpen(&self->deletionIndexFile, filename, false)) {
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
		if (NOT CBFileRead(self->deletionIndexFile, data, 11)) {
			CBFileClose(self->deletionIndexFile);
			free(section);
			CBLogError("Could not read entry from the database deletion index.");
			return false;
		}
		// The key length is 11.
		section->key[0] = 11;
		// Add data to index
		memcpy(section->key + 1, data, 11);
		section->indexPos = x;
		CBAssociativeArrayInsert(&self->deletionIndex, section->key, CBAssociativeArrayFind(&self->deletionIndex, section->key).position, NULL);
	}
	return true;
}
bool CBDatabaseCreateDeletionIndex(CBDatabase * self, char * filename){
	self->numDeletionValues = 0;
	if (NOT CBFileOpen(&self->deletionIndexFile, filename, true)) {
		CBLogError("Could not open the deletion index file.");
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
bool CBFreeDatabase(CBDatabase * self){
	// Commit staged changes if we didn't last time
	if (self->hasStaged
		&& NOT CBDatabaseCommit(self)){
		CBLogError("Failed to commit when freeing a database transaction.");
		return false;
	}
	CBFreeAssociativeArray(&self->deletionIndex);
	CBFreeAssociativeArray(&self->itData);
	CBFreeAssociativeArray(&self->valueWritingIndexes);
	// Close files
	CBFileClose(self->deletionIndexFile);
	if (self->lastUsedFileType != CB_DATABASE_FILE_TYPE_NONE)
		CBFileClose(self->fileObjectCache);
	// Free changes information
	CBFreeDatabaseTransactionChanges(&self->staged);
	CBFreeDatabaseTransactionChanges(&self->current);
	free(self);
	return true;
}
void CBFreeDatabaseTransactionChanges(CBDatabaseTransactionChanges * self){
	CBFreeAssociativeArray(&self->indexes);
	free(self->extraData);
}
void CBFreeIndexNode(CBIndexNode * node){
	for (uint8_t x = 0; x < node->numElements; x++)
		free(node->elements[x].key);
	for (uint8_t x = 0; x <= node->numElements; x++)
		if (node->children[x].cached)
			CBFreeIndexNode(node->children[x].cachedNode);
	free(node);
}
void CBFreeIndex(CBDatabaseIndex * index){
	CBFreeIndexNode(index->indexCache.cachedNode);
	free(index);
}
void CBFreeIndexElementPos(CBIndexElementPos pos){
	if (NOT pos.nodeLoc.cached)
		CBFreeIndexNode(pos.nodeLoc.cachedNode);
	for (uint8_t x = 0; x < pos.parentStack.num; x++)
		if (NOT pos.parentStack.nodes[x].cached)
			CBFreeIndexNode(pos.parentStack.nodes[x].cachedNode);
}
void CBFreeIndexItData(void * vindexItData){
	CBIndexItData * indexItData = vindexItData;
	CBFreeAssociativeArray(&indexItData->currentAddedKeys);
	CBFreeAssociativeArray(&indexItData->txKeys);
}
void CBFreeIndexTxData(void * vindexTxData){
	CBIndexTxData * indexTxData = vindexTxData;
	CBFreeAssociativeArray(&indexTxData->valueWrites);
	CBFreeAssociativeArray(&indexTxData->deleteKeys);
	CBFreeAssociativeArray(&indexTxData->changeKeysOld);
	CBFreeAssociativeArray(&indexTxData->changeKeysNew);
}
void CBFreeDatabaseRangeIterator(CBDatabaseRangeIterator * it){
	if (it->foundIndex)
		CBFreeIndexElementPos(it->indexPos);
}
bool CBDatabaseAddDeletionEntry(CBDatabase * self, uint16_t fileID, uint32_t pos, uint32_t len){
	// First look for inactive deletion that can be used.
	CBPosition res;
	if (CBAssociativeArrayGetFirst(&self->deletionIndex, &res) && NOT ((uint8_t *)res.node->elements[0])[1]) {
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
		CBAssociativeArrayInsert(&self->deletionIndex, section->key, CBAssociativeArrayFind(&self->deletionIndex, (uint8_t *)section).position, NULL);
		// Overwrite deletion index
		uint8_t data[11];
		memcpy(data, section->key + 1, 11);
		if (NOT CBDatabaseAddOverwrite(self, CB_DATABASE_FILE_TYPE_DELETION_INDEX, 0, 0, data, 4 + section->indexPos * 11, 11)) {
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
		CBAssociativeArrayInsert(&self->deletionIndex, section->key, CBAssociativeArrayFind(&self->deletionIndex, section->key).position, NULL);
		self->numDeletionValues++;
		// Append deletion index
		uint8_t data[11];
		memcpy(data, section->key + 1, 11);
		if (NOT CBDatabaseAppend(self, CB_DATABASE_FILE_TYPE_DELETION_INDEX, 0, 0, data, 11)) {
			CBLogError("There was an error when adding a deletion entry by appending a new one.");
			return false;
		}
	}
	return true;
}
bool CBDatabaseAddOverwrite(CBDatabase * self, CBDatabaseFileType fileType, uint8_t indexID, uint16_t fileID, uint8_t * data, uint32_t offset, uint32_t dataLen){
	// Execute the overwrite and write rollback information to the log file.
	// Get the file to overwrite
	CBDepObject file;
	if (NOT CBDatabaseGetFile(self, &file, fileType, indexID, fileID)) {
		CBLogError("Could not get the data file for overwritting.");
		return false;
	}
	// Write to the log file the rollback information for the changes being made.
	uint32_t dataTempLen = dataLen + 12;
	uint8_t * dataTemp = malloc(dataTempLen);
	// Write file type
	dataTemp[0] = fileType;
	// Write index ID
	dataTemp[1] = indexID;
	// Write file ID
	CBInt16ToArray(dataTemp, 2, fileID);
	// Write offset
	CBInt32ToArray(dataTemp, 4, offset);
	// Write data size
	CBInt32ToArray(dataTemp, 8, dataLen);
	// Write old data
	if (NOT CBFileSeek(file, offset)){
		CBLogError("Could not seek to the read position for previous data in a file to be overwritten.");
		free(dataTemp);
		return false;
	}
	if (NOT CBFileRead(file, dataTemp + 12, dataLen)) {
		CBLogError("Could not read the previous data to be overwritten.");
		free(dataTemp);
		return false;
	}
	if (NOT CBFileAppend(self->logFile, dataTemp, dataTempLen)) {
		CBLogError("Could not write the backup information to the transaction log file.");
		free(dataTemp);
		return false;
	}
	free(dataTemp);
	// Sync log file
	if (NOT CBFileSync(self->logFile)){
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
bool CBDatabaseAddValue(CBDatabase * self, uint32_t dataSize, uint8_t * data, CBIndexValue * indexValue){
	// Look for a deleted section to use
	if (dataSize == 0)
		// Null value
		return true;
	CBFindResult res = CBDatabaseGetDeletedSection(self, dataSize);
	if (res.found) {
		// ??? Is there a more effecient method for changing the key of the associative array than deleting and re-inserting with new key?
		// Found a deleted section we can use for the data
		CBDeletedSection * section = (CBDeletedSection *)CBFindResultToPointer(res);
		uint32_t sectionLen = CBArrayToInt32BigEndian(section->key, 2);
		uint16_t sectionFileID = CBArrayToInt16(section->key, 6);
		uint32_t sectionOffset = CBArrayToInt32(section->key, 8);
		uint32_t newSectionLen = sectionLen - dataSize;
		// Use the section
		if (NOT CBDatabaseAddOverwrite(self, CB_DATABASE_FILE_TYPE_DATA, 0, sectionFileID, data, sectionOffset + newSectionLen, dataSize)){
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
		CBAssociativeArrayInsert(&self->deletionIndex, section->key, CBAssociativeArrayFind(&self->deletionIndex, section->key).position, NULL);
		if (NOT CBDatabaseAddOverwrite(self, CB_DATABASE_FILE_TYPE_DELETION_INDEX, 0, 0, section->key + 1, 4 + 11 * section->indexPos, 5)) {
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
		if (NOT CBDatabaseAppend(self, CB_DATABASE_FILE_TYPE_DATA, 0, self->lastFile, data, dataSize)) {
			CBLogError("Failed to add an append operation for the value replacing an older one.");
			return false;
		}
		// Update index
		indexValue->fileID = self->lastFile;
		indexValue->length = dataSize;
	}
	return true;
}
void CBDatabaseAddWriteValue(uint8_t * writeValue, CBDatabaseIndex * index, uint8_t * key, uint32_t dataLen, uint8_t * dataPtr, uint32_t offset, bool stage) {
	CBDatabase * self = index->database;
	CBDatabaseTransactionChanges * changes = stage ? &self->staged : &self->current;
	// Get the index tx data
	CBIndexTxData * indexTxData = CBDatabaseGetIndexTxData(changes, index, true);
	// Add key to write value entry if not staging
	if (NOT stage)
		memcpy(writeValue, key, index->keySize);
	// Remove from deletion array if needed.
	CBFindResult res = CBAssociativeArrayFind(&indexTxData->deleteKeys, writeValue);
	if (res.found)
		CBAssociativeArrayDelete(&indexTxData->deleteKeys, res.position, false);
	// See if entry exists to be written already by CB_OVERWRITE_DATA
	uint32_t * intPtr = (uint32_t *)(writeValue + index->keySize);
	*intPtr = CB_OVERWRITE_DATA;
	res = CBAssociativeArrayFind(&indexTxData->valueWrites, writeValue);
	if (res.found) {
		// Found, now replace data
		uint8_t * keyPtr = CBFindResultToPointer(res);
		uint8_t * oldDataPtr = keyPtr + index->keySize + 4;
		// Just replace the old data
		if (offset == CB_OVERWRITE_DATA){
			// Reallocate memory if the new data length is higher
			if (dataLen > *(uint32_t *)oldDataPtr){
				res.position.node->elements[res.position.index] = realloc(keyPtr, index->keySize + 8 + dataLen);
				oldDataPtr = res.position.node->elements[res.position.index] + index->keySize + 4;
				if (stage) 
					self->stagedSize += dataLen - *(uint32_t *)oldDataPtr;
			}
			// Ensure the data length is the new one
			*(uint32_t *)oldDataPtr = dataLen;
			// Overwrite all
			offset = 0;
		}
		memcpy(oldDataPtr + 4 + offset, dataPtr, dataLen);
		free(writeValue);
	}else{
		// No overwrite Look for entry with the same offset.
		*intPtr = offset;
		if (offset == CB_OVERWRITE_DATA) {
			// As this is overwriting all previous data, delete any instances of this key with any offset
			indexTxData->valueWrites.compareFunc = CBTransactionKeysCompare;
			for(;;) {
				res = CBAssociativeArrayFind(&indexTxData->valueWrites, writeValue);
				if (NOT res.found)
					break;
				CBAssociativeArrayDelete(&indexTxData->valueWrites, res.position, true);
			}
			indexTxData->valueWrites.compareFunc = CBTransactionKeysAndOffsetCompare;
		}else
			// Look for previous value for this offset
			res = CBAssociativeArrayFind(&indexTxData->valueWrites, writeValue);
		if (res.found) {
			// Replace the entry which has this offset
			free(CBFindResultToPointer(res));
			res.position.node->elements[res.position.index] = writeValue;
		}else{
			// Insert into array
			CBAssociativeArrayInsert(&indexTxData->valueWrites, writeValue, res.position, NULL);
			// Update sizes
			if (stage)
				self->stagedSize += dataLen + index->keySize + 8;
			// Change iteration data
			if (stage) {
				// Add index to the array of value write indexes if need be
				res = CBAssociativeArrayFind(&self->valueWritingIndexes, index);
				if (NOT res.found) {
					// Add into the array
					CBAssociativeArrayInsert(&self->valueWritingIndexes, index, res.position, NULL);
					self->numIndexes++;
				}
				// Remove from currentAddedKeys if it exists
				CBIndexItData * itData = CBDatabaseGetIndexItData(self, index, false);
				if (itData) {
					res = CBAssociativeArrayFind(&itData->currentAddedKeys, key);
					if (res.found)
						CBAssociativeArrayDelete(&itData->currentAddedKeys, res.position, false);
				}
			}else{
				CBIndexItData * itData = CBDatabaseGetIndexItData(self, index, true);
				if (itData){
					// Add to currentAddedKeys and txKeys if not existant
					res = CBAssociativeArrayFind(&itData->txKeys, key);
					if (NOT res.found) {
						uint8_t * insertKey = malloc(index->keySize);
						memcpy(insertKey, key, index->keySize);
						CBAssociativeArrayInsert(&itData->txKeys, insertKey, res.position, NULL);
						CBAssociativeArrayInsert(&itData->currentAddedKeys, insertKey, CBAssociativeArrayFind(&itData->currentAddedKeys, insertKey).position, NULL);
					}
				}
			}
		}
	}
}
bool CBDatabaseAppend(CBDatabase * self, CBDatabaseFileType fileType, uint8_t indexID, uint16_t fileID, uint8_t * data, uint32_t dataLen) {
	// Execute the append operation
	CBDepObject file;
	if (NOT CBDatabaseGetFile(self, &file, fileType, indexID, fileID)) {
		CBLogError("Could not get file %u for appending.", fileID);
		return false;
	}
	if (NOT CBFileAppend(file, data, dataLen)) {
		CBLogError("Failed to append data to file %u.", fileID);
		return false;
	}
	return true;
}
bool CBDatabaseAppendZeros(CBDatabase * self, CBDatabaseFileType fileType, uint8_t indexID, uint16_t fileID, uint32_t amount) {
	// Execute the append operation
	CBDepObject file;
	if (NOT CBDatabaseGetFile(self, &file, fileType, indexID, fileID)) {
		CBLogError("Could not get file %u for appending.", fileID);
		return false;
	}
	if (NOT CBFileAppendZeros(file, amount)) {
		CBLogError("Failed to append data to file %u.", fileID);
		return false;
	}
	return true;
}
bool CBDatabaseChangeKey(CBDatabaseIndex * index, uint8_t * previousKey, uint8_t * newKey, bool stage) {
	CBDatabase * self = index->database;
	CBDatabaseTransactionChanges * changes = stage ? &self->staged : &self->current;
	// Get the index tx data
	CBIndexTxData * indexTxData = CBDatabaseGetIndexTxData(changes, index, true);
	// See if the previous key was a new key in another change key entry.
	indexTxData->changeKeysNew.compareFunc = CBNewKeyWithSingleKeyCompare;
	CBFindResult res = CBAssociativeArrayFind(&indexTxData->changeKeysNew, previousKey);
	indexTxData->changeKeysNew.compareFunc = CBNewKeyCompare;
	if (res.found)
		// Now change the old entry
		memcpy(CBFindResultToPointer(res) + index->keySize, newKey, index->keySize);
	else{
		// Insert new entry if the old key exists in the index or staged
		bool foundPrev = false;
		if (NOT stage) {
			// Look for the old key being staged
			CBIndexTxData * stagedIndexTxData = CBDatabaseGetIndexTxData(&self->staged, index, false);
			if (stagedIndexTxData
				&& (CBAssociativeArrayFind(&stagedIndexTxData->valueWrites, previousKey).found
					|| CBAssociativeArrayFind(&stagedIndexTxData->changeKeysNew, previousKey).found))
				foundPrev = true;
		}
		if (NOT foundPrev) {
			CBIndexFindResult fres = CBDatabaseIndexFind(index, previousKey);
			if (fres.status == CB_DATABASE_INDEX_ERROR) {
				CBLogError("There was an error whilst trying to find a previous key in the index to see if a change key entry should be made.");
				return false;
			}
			CBFreeIndexElementPos(fres.el);
			foundPrev = fres.status == CB_DATABASE_INDEX_FOUND;
		}
		if (foundPrev) {
			uint8_t * keys = malloc(index->keySize * 2);
			memcpy(keys, previousKey, index->keySize);
			memcpy(keys + index->keySize, newKey, index->keySize);
			// Add to the change keys arrays
			CBAssociativeArrayInsert(&indexTxData->changeKeysOld, keys, CBAssociativeArrayFind(&indexTxData->changeKeysOld, keys).position, NULL);
			CBAssociativeArrayInsert(&indexTxData->changeKeysNew, keys, CBAssociativeArrayFind(&indexTxData->changeKeysNew, keys).position, NULL);
			// Update sizes
			if (stage)
				self->stagedSize += index->keySize*2;
			if (stage) {
				// Add index to the array of value write indexes if need be
				CBFindResult res = CBAssociativeArrayFind(&self->valueWritingIndexes, index);
				if (NOT res.found) {
					// Add into the array
					CBAssociativeArrayInsert(&self->valueWritingIndexes, index, res.position, NULL);
					self->numIndexes++;
				}
			}
		}
	}
	// Change value writes to the new key, do not use iterator as the array is being changed.
	for (;;) {
		indexTxData->valueWrites.compareFunc = CBTransactionKeysCompare;
		CBFindResult res = CBAssociativeArrayFind(&indexTxData->valueWrites, previousKey);
		indexTxData->valueWrites.compareFunc = CBTransactionKeysAndOffsetCompare;
		if (NOT res.found)
			break;
		uint8_t * val = CBFindResultToPointer(res);
		CBAssociativeArrayDelete(&indexTxData->valueWrites, res.position, false);
		memcpy(val, newKey, index->keySize);
		CBAssociativeArrayInsert(&indexTxData->valueWrites, val, CBAssociativeArrayFind(&indexTxData->valueWrites, val).position, NULL);
	}
	// Remove remove value for new key
	res = CBAssociativeArrayFind(&indexTxData->deleteKeys, newKey);
	if (res.found){
		CBAssociativeArrayDelete(&indexTxData->deleteKeys, res.position, true);
		if (stage)
			self->stagedSize -= index->keySize;
	}
	if (NOT stage) {
		// Change txKeys and/or currentAddedKeys
		CBIndexItData * indexItData = CBDatabaseGetIndexItData(self, index, true);
		res = CBAssociativeArrayFind(&indexItData->txKeys, previousKey);
		if (res.found) {
			// Remove from txKeys and replace with new key. Also look for currentAddedKeys
			uint8_t * key = CBFindResultToPointer(res);
			CBAssociativeArrayDelete(&indexItData->txKeys, res.position, false);
			CBFindResult res2 = CBAssociativeArrayFind(&indexItData->currentAddedKeys, previousKey);
			if (res2.found)
				CBAssociativeArrayDelete(&indexItData->currentAddedKeys, res2.position, false);
			memcpy(key, newKey, index->keySize);
			CBAssociativeArrayInsert(&indexItData->txKeys, key, CBAssociativeArrayFind(&indexItData->txKeys, key).position, NULL);
			if (res2.found)
				CBAssociativeArrayInsert(&indexItData->currentAddedKeys, key, CBAssociativeArrayFind(&indexItData->currentAddedKeys, key).position, NULL);
			return true;
		}
		// Must be changing key in disk. Add new key to txKeys
		uint8_t * key = malloc(index->keySize);
		memcpy(key, newKey, index->keySize);
		CBAssociativeArrayInsert(&indexItData->txKeys, key, CBAssociativeArrayFind(&indexItData->txKeys, key).position, NULL);
		CBAssociativeArrayInsert(&indexItData->currentAddedKeys, key, CBAssociativeArrayFind(&indexItData->currentAddedKeys, key).position, NULL);
	}
	return true;
}
void CBDatabaseClearCurrent(CBDatabase * self) {
	CBPosition pos;
	CBPosition pos2;
	// Go through index tx data
	if (CBAssociativeArrayGetFirst(&self->current.indexes, &pos)) for (;;){
		CBIndexTxData * indexTxData = pos.node->elements[pos.index];
		CBIndexItData * indexItData = CBDatabaseGetIndexItData(self, indexTxData->index, false);
		if (indexItData) {
			// Change txKeys back to old keys in changeKeys
			if (CBAssociativeArrayGetFirst(&indexTxData->changeKeysOld, &pos2)) for (;;){
				uint8_t * changeKey = pos2.node->elements[pos2.index];
				uint8_t cmpKey[indexTxData->index->keySize + 2];
				cmpKey[0] = indexTxData->index->keySize + 1;
				cmpKey[1] = indexTxData->index->ID;
				memcpy(cmpKey + 2, changeKey + indexTxData->index->keySize, indexTxData->index->keySize);
				CBFindResult res = CBAssociativeArrayFind(&indexItData->txKeys, cmpKey);
				if (res.found) {
					uint8_t * txKey = CBFindResultToPointer(res);
					CBAssociativeArrayDelete(&indexItData->txKeys, res.position, false);
					memcpy(txKey + 2, changeKey, indexTxData->index->keySize);
					CBAssociativeArrayInsert(&indexItData->txKeys, txKey, CBAssociativeArrayFind(&indexItData->txKeys, txKey).position, NULL);
				}
				if (CBAssociativeArrayIterate(&indexTxData->changeKeysOld, &pos2))
					break;
			}
		}
		if (CBAssociativeArrayIterate(&self->current.indexes, &pos))
			break;
	}
	// Remove currentAddedKeys from txKeys
	if (CBAssociativeArrayGetFirst(&self->itData, &pos)) for (;;) {
		CBIndexItData * indexItData = pos.node->elements[pos.index];
		if (CBAssociativeArrayGetFirst(&indexItData->currentAddedKeys, &pos2)) for (;;){
			CBAssociativeArrayDelete(&indexItData->txKeys, CBAssociativeArrayFind(&indexItData->txKeys, pos.node->elements[pos.index]).position, true);
			if (CBAssociativeArrayIterate(&indexItData->currentAddedKeys, &pos2))
				break;
		}
		// Reset currentAddedKeys
		CBFreeAssociativeArray(&indexItData->currentAddedKeys);
		CBInitAssociativeArray(&indexItData->currentAddedKeys, CBTransactionKeysCompare, indexItData->index, NULL);
		if (CBAssociativeArrayIterate(&self->itData, &pos))
			break;
	}
	// Free index tx data
	CBFreeAssociativeArray(&self->current.indexes);
	CBInitAssociativeArray(&self->current.indexes, CBIndexCompare, NULL, CBFreeIndexTxData);
	// Change extra data to match staged.
	memcpy(self->current.extraData, self->staged.extraData, self->extraDataSize);
}
bool CBDatabaseCommit(CBDatabase * self){
	char filename[strlen(self->dataDir) + strlen(self->folder) + 10];
	sprintf(filename, "%s/%s/log.dat", self->dataDir, self->folder);
	// Open the log file
	if (NOT CBFileOpen(&self->logFile, filename, true)) {
		CBLogError("The log file for overwritting could not be opened.");
		return false;
	}
	// Write the previous sizes for the files to the log file
	uint8_t data[12];
	data[0] = 1; // Log file active.
	uint32_t deletionIndexLen;
	if (NOT CBFileGetLength(self->deletionIndexFile, &deletionIndexLen)) {
		CBLogError("Could not get the length of the deletion index file.");
		CBFileClose(self->logFile);
		return false;
	}
	CBInt32ToArray(data, 1, deletionIndexLen);
	CBInt16ToArray(data, 5, self->lastFile);
	CBInt32ToArray(data, 7, self->lastSize);
	data[11] = self->numIndexes;
	if (NOT CBFileAppend(self->logFile, data, 12)) {
		CBLogError("Could not write previous size information to the log-file for the data and deletion index.");
		CBFileClose(self->logFile);
		return false;
	}
	// Loop through index files to write the previous lengths
	CBPosition pos;
	if (CBAssociativeArrayGetFirst(&self->valueWritingIndexes, &pos)) for (;;) {
		CBDatabaseIndex * ind = pos.node->elements[pos.index];
		data[0] = ind->ID;
		CBInt16ToArray(data, 1, ind->lastFile);
		CBInt32ToArray(data, 3, ind->lastSize);
		if (NOT CBFileAppend(self->logFile, data, 7)) {
			CBLogError("Could not write the previous size information for an index.");
			CBFileClose(self->logFile);
			return false;
		}
		if (CBAssociativeArrayIterate(&self->valueWritingIndexes, &pos))
			break;
	}
	// Sync log file, so that it is now active with the information of the previous file sizes.
	if (NOT CBFileSync(self->logFile)){
		CBLogError("Failed to sync the log file");
		CBFileClose(self->logFile);
		return false;
	}
	// Sync directory for log file
	if (NOT CBFileSyncDir(self->dataDir)) {
		CBLogError("Failed to synchronise the directory during a commit for the log file.");
		CBFileClose(self->logFile);
		return false;
	}
	uint32_t lastFileSize = self->lastSize;
	uint32_t prevDeletionEntryNum = self->numDeletionValues;
	// Loop through indexes
	CBPosition indIt;
	if (CBAssociativeArrayGetFirst(&self->staged.indexes, &indIt)) for (;;) {
		CBPosition it;
		// Get index tx data
		CBIndexTxData * indexTxData = indIt.node->elements[indIt.index];
		// Change keys
		if (CBAssociativeArrayGetFirst(&indexTxData->changeKeysOld, &it)) for (;;) {
			uint8_t * oldKeyPtr = it.node->elements[it.index];
			uint8_t * newKeyPtr = oldKeyPtr + indexTxData->index->keySize;
			// Find index entry
			CBIndexFindResult res = CBDatabaseIndexFind(indexTxData->index, oldKeyPtr);
			if (res.status == CB_DATABASE_INDEX_ERROR) {
				CBLogError("There was an error while attempting to find an index key for changing.");
				CBFileClose(self->logFile);
				return false;
			}
			CBIndexValue * indexValue = &res.el.nodeLoc.cachedNode->elements[res.el.index];
			// Allocate and assign new index entry
			CBIndexValue * newIndexValue = malloc(sizeof(*indexValue));
			newIndexValue->fileID = indexValue->fileID;
			newIndexValue->length = indexValue->length;
			newIndexValue->pos = indexValue->pos;
			// Delete old key
			indexValue->length = CB_DELETED_VALUE;
			CBInt32ToArray(data, 0, CB_DELETED_VALUE);
			if (NOT CBDatabaseAddOverwrite(self, CB_DATABASE_FILE_TYPE_INDEX, indexTxData->index->ID, res.el.nodeLoc.diskNodeLoc.indexFile, data, res.el.nodeLoc.diskNodeLoc.offset + 3 + (indexTxData->index->keySize + 10)*res.el.index, 4)) {
				CBLogError("Failed to overwrite the index entry's length with CB_DELETED_VALUE to signify deletion when changing a key.");
				CBFileClose(self->logFile);
				CBFreeIndexElementPos(res.el);
				free(newIndexValue);
				return false;
			}
			// Add new key to index
			newIndexValue->key = malloc(indexTxData->index->keySize);
			memcpy(newIndexValue->key, newKeyPtr, indexTxData->index->keySize);
			// Find position for index entry
			CBFreeIndexElementPos(res.el);
			res = CBDatabaseIndexFind(indexTxData->index, newKeyPtr);
			if (res.status == CB_DATABASE_INDEX_ERROR) {
				CBLogError("Failed to find the position for a new key replacing an old one.");
				CBFileClose(self->logFile);
				CBFreeIndexElementPos(res.el);
				free(newIndexValue->key);
				free(newIndexValue);
				return false;
			}
			if (res.status == CB_DATABASE_INDEX_FOUND) {
				// The index entry already exists. Get the index value
				CBIndexValue * indexValue = &res.el.nodeLoc.cachedNode->elements[res.el.index];
				if (indexValue->length != CB_DELETED_VALUE) {
					// Delete existing data this new key was pointing to
					if (NOT CBDatabaseAddDeletionEntry(self, indexValue->fileID, indexValue->pos, indexValue->length)) {
						CBLogError("Failed to create a deletion entry for a value, the key thereof was changed to point to other data.");
						CBFileClose(self->logFile);
						CBFreeIndexElementPos(res.el);
						free(newIndexValue->key);
						free(newIndexValue);
						return false;
					}
				}
				// Replace the index value to point to the new data
				indexValue->fileID = newIndexValue->fileID;
				indexValue->length = newIndexValue->length;
				indexValue->pos = newIndexValue->pos;
				CBInt16ToArray(data, 0, indexValue->fileID);
				CBInt32ToArray(data, 2, indexValue->length);
				CBInt32ToArray(data, 6, indexValue->pos);
				if (NOT CBDatabaseAddOverwrite(self, CB_DATABASE_FILE_TYPE_INDEX, indexTxData->index->ID, res.el.nodeLoc.diskNodeLoc.indexFile, data, res.el.nodeLoc.diskNodeLoc.offset + 1 + (indexTxData->index->keySize + 10)*res.el.index, 10)) {
					CBFileClose(self->logFile);
					CBFreeIndexElementPos(res.el);
					free(newIndexValue->key);
					free(newIndexValue);
					return false;
				}
				// Else insert index element into the index
			}else if (NOT CBDatabaseIndexInsert(indexTxData->index, newIndexValue, &res.el, NULL)) {
				CBLogError("Failed to insert a new index element into the index with a new key for existing data.");
				CBFileClose(self->logFile);
				CBFreeIndexElementPos(res.el);
				free(newIndexValue->key);
				free(newIndexValue);
				return false;
			}
			CBFreeIndexElementPos(res.el);
			// Iterate to next key to change.
			if (CBAssociativeArrayIterate(&indexTxData->changeKeysOld, &it))
				break;
		}
		// Go through each value write key
		// Get the first key with an interator
		if (CBAssociativeArrayGetFirst(&indexTxData->valueWrites, &it)) for (;;) {
			// Get the data for writing
			uint8_t * keyPtr = it.node->elements[it.index];
			uint32_t offset = *(uint32_t *)(keyPtr + indexTxData->index->keySize);
			uint32_t dataSize = *(uint32_t *)(keyPtr + indexTxData->index->keySize + 4);
			uint8_t * dataPtr = keyPtr + indexTxData->index->keySize + 8;
			// Check for key in index
			CBIndexFindResult res = CBDatabaseIndexFind(indexTxData->index, keyPtr);
			if (res.status == CB_DATABASE_INDEX_ERROR) {
				CBLogError("There was an error while searching an index for a key.");
				CBFileClose(self->logFile);
				return false;
			}
			if (res.status == CB_DATABASE_INDEX_FOUND) {
				// Exists in index so we are overwriting data.
				// Get the index data
				CBIndexValue * indexValue = &res.el.nodeLoc.cachedNode->elements[res.el.index];
				// See if the data can be overwritten.
				if (indexValue->length >= dataSize && indexValue->length != CB_DELETED_VALUE){
					// We are going to overwrite the previous data.
					if (NOT CBDatabaseAddOverwrite(self,
												   CB_DATABASE_FILE_TYPE_DATA,
												   0,
												   indexValue->fileID,
												   dataPtr,
												   // The offset also tracks when the previous data should be overwritten and discarded.
												   indexValue->pos + ((offset == CB_OVERWRITE_DATA) ? 0 : offset),
												   dataSize)){
						CBLogError("Failed to add an overwrite operation to overwrite a previous value.");
						CBFileClose(self->logFile);
						CBFreeIndexElementPos(res.el);
						return false;
					}
					if (indexValue->length > dataSize && offset == CB_OVERWRITE_DATA) {
						// Change indexed length and mark the remaining length as deleted
						if (NOT CBDatabaseAddDeletionEntry(self, indexValue->fileID, indexValue->pos + dataSize, indexValue->length - dataSize)){
							CBLogError("Failed to add a deletion entry when overwriting a previous value with a smaller one.");
							CBFileClose(self->logFile);
							CBFreeIndexElementPos(res.el);
							return false;
						}
						// Change the length of the data in the index.
						indexValue->length = dataSize;
						uint8_t newLength[4];
						CBInt32ToArray(newLength, 0, indexValue->length);
						if (NOT CBDatabaseAddOverwrite(self, CB_DATABASE_FILE_TYPE_INDEX, indexTxData->index->ID, res.el.nodeLoc.diskNodeLoc.indexFile, newLength, res.el.nodeLoc.diskNodeLoc.offset + 3 + (10+indexTxData->index->keySize)*res.el.index, 4)) {
							CBLogError("Failed to add an overwrite operation to write the new length of a value to the database index.");
							CBFileClose(self->logFile);
							CBFreeIndexElementPos(res.el);
							return false;
						}
					}
				}else{
					// We will mark the previous data as deleted and store data elsewhere, unless the data has been marked as deleted already (by CB_DELETED_VALUE length).
					if (indexValue->length != CB_DELETED_VALUE
						&& NOT CBDatabaseAddDeletionEntry(self, indexValue->fileID, indexValue->pos, indexValue->length)){
						CBLogError("Failed to add a deletion entry for an old value when replacing it with a larger one.");
						CBFileClose(self->logFile);
						CBFreeIndexElementPos(res.el);
						return false;
					}
					if (NOT CBDatabaseAddValue(self, dataSize, dataPtr, indexValue)) {
						CBLogError("Failed to add a value to the database with a previously exiting key.");
						CBFileClose(self->logFile);
						CBFreeIndexElementPos(res.el);
						return false;
					}
					// Write index
					uint8_t data[10];
					CBInt16ToArray(data, 0, indexValue->fileID);
					CBInt32ToArray(data, 2, indexValue->length);
					CBInt32ToArray(data, 6, indexValue->pos);
					if (NOT CBDatabaseAddOverwrite(self, CB_DATABASE_FILE_TYPE_INDEX, indexTxData->index->ID, res.el.nodeLoc.diskNodeLoc.indexFile, data, res.el.nodeLoc.diskNodeLoc.offset + 1 + (10+indexTxData->index->keySize)*res.el.index, 10)) {
						CBLogError("Failed to add an overwrite operation for updating the index for writting data in a new location.");
						CBFileClose(self->logFile);
						CBFreeIndexElementPos(res.el);
						return false;
					}
				}
			}else{
				// Does not exist in index, so we are creating new data
				// New index value data
				CBIndexValue * indexValue = malloc(sizeof(*indexValue));
				if (NOT CBDatabaseAddValue(self, dataSize, dataPtr, indexValue)) {
					CBLogError("Failed to add a value to the database with a new key.");
					CBFileClose(self->logFile);
					CBFreeIndexElementPos(res.el);
					free(indexValue);
					return false;
				}
				// Add key to index
				indexValue->key = malloc(indexTxData->index->keySize);
				memcpy(indexValue->key, keyPtr, indexTxData->index->keySize);
				// Insert index element into the index
				if (NOT CBDatabaseIndexInsert(indexTxData->index, indexValue, &res.el, NULL)) {
					CBLogError("Failed to insert a new index element into the index.");
					CBFileClose(self->logFile);
					CBFreeIndexElementPos(res.el);
					free(indexValue->key);
					free(indexValue);
					return false;
				}
			}
			// Iterate to next key-value to write.
			if (CBAssociativeArrayIterate(&indexTxData->valueWrites, &it))
				break;
		}
		// Delete values
		if (CBAssociativeArrayGetFirst(&indexTxData->deleteKeys, &it)) for (;;) {
			uint8_t * keyPtr = it.node->elements[it.index];
			// Find index entry
			CBIndexFindResult res = CBDatabaseIndexFind(indexTxData->index, keyPtr);
			if (res.status == CB_DATABASE_INDEX_ERROR) {
				CBLogError("There was an error attempting to find an index key-value for deletion.");
				CBFileClose(self->logFile);
				return false;
			}
			if (res.status == CB_DATABASE_INDEX_NOT_FOUND) {
				CBLogError("Could not find an index key-value for deletion.");
				CBFileClose(self->logFile);
				CBFreeIndexElementPos(res.el);
				return false;
			}
			if (NOT CBDatabaseIndexDelete(indexTxData->index, &res)){
				CBLogError("Failed to delete a key-value.");
				CBFileClose(self->logFile);
				CBFreeIndexElementPos(res.el);
				return false;
			}
			// Iterate to next key to delete.
			if (CBAssociativeArrayIterate(&indexTxData->deleteKeys, &it))
				break;
		}
		// Update the last index file information
		if (indexTxData->index->lastFile != indexTxData->index->newLastFile
			|| indexTxData->index->lastSize != indexTxData->index->newLastSize) {
			indexTxData->index->lastFile = indexTxData->index->newLastFile;
			indexTxData->index->lastSize = indexTxData->index->newLastSize;
			// Write to the index file
			CBInt16ToArray(data, 0, indexTxData->index->lastFile);
			CBInt32ToArray(data, 2, indexTxData->index->lastSize);
			if (NOT CBDatabaseAddOverwrite(self, CB_DATABASE_FILE_TYPE_INDEX, indexTxData->index->ID, 0, data, 0, 6)) {
				CBLogError("Failed to write the new last file information to an index.");
				CBFileClose(self->logFile);
				return false;
			}
		}
		// Iterate to this next index if there is one
		if (CBAssociativeArrayIterate(&self->staged.indexes, &indIt))
			break;
	}
	// Update the data last file information
	if (lastFileSize != self->lastSize) {
		// Change index information.
		CBInt16ToArray(data, 0, self->lastFile);
		CBInt32ToArray(data, 2, self->lastSize);
		if (NOT CBDatabaseAddOverwrite(self, CB_DATABASE_FILE_TYPE_DATA, 0, 0, data, 0, 6)) {
			CBLogError("Failed to update the information for the last data index file.");
			CBFileClose(self->logFile);
			return false;
		}
	}
	if (prevDeletionEntryNum != self->numDeletionValues) {
		// New number of deletion entries.
		CBInt32ToArray(data, 0, self->numDeletionValues);
		if (NOT CBDatabaseAddOverwrite(self, CB_DATABASE_FILE_TYPE_DELETION_INDEX, 0, 0, data, 0, 4)) {
			CBLogError("Failed to update the number of entries for the deletion index file.");
			CBFileClose(self->logFile);
			return false;
		}
	}
	// Update extra data
	uint32_t start = 0xFFFFFFFF;
	for (uint32_t x = 0; x < self->extraDataSize; x++) {
		bool same = self->extraDataOnDisk[x] == self->staged.extraData[x];
		if (NOT same && start == 0xFFFFFFFF)
			start = x;
		if (start != 0xFFFFFFFF
			&& (same || x == self->extraDataSize - 1)){
			// Got the section that should be written.
			uint32_t len = x - start + (x == self->extraDataSize - 1);
			memcpy(self->extraDataOnDisk + start, self->staged.extraData + start, len);
			// Now write to disk
			if (NOT CBDatabaseAddOverwrite(self, CB_DATABASE_FILE_TYPE_DATA, 0, 0, self->staged.extraData + start, 6 + start, len)) {
				CBLogError("Failed to write to the database's extra data.");
				CBFileClose(self->logFile);
				return false;
			}
			start = 0xFFFFFFFF;
		}
	}
	// Sync last used file and deletion index file.
	if ((NOT CBFileSync(self->fileObjectCache))
		|| NOT CBFileSync(self->deletionIndexFile)) {
		CBLogError("Failed to synchronise the files during a commit.");
		CBFileClose(self->logFile);
		return false;
	}
	// Sync directory
	if (NOT CBFileSyncDir(self->dataDir)) {
		CBLogError("Failed to synchronise the directory during a commit.");
		CBFileClose(self->logFile);
		return false;
	}
	// Now we are done, make the logfile inactive. Errors do not matter here.
	if (CBFileSeek(self->logFile, 0)) {
		data[0] = 0;
		if (CBFileOverwrite(self->logFile, data, 1))
			CBFileSync(self->logFile);
	}
	CBFileClose(self->logFile);
	// Reset txKeys
	if (CBAssociativeArrayGetFirst(&self->itData, &pos)) for (;;) {
		CBIndexItData * indexItData = pos.node->elements[pos.index];
		// Reset currentAddedKeys
		CBFreeAssociativeArray(&indexItData->txKeys);
		CBInitAssociativeArray(&indexItData->txKeys, CBTransactionKeysCompare, indexItData->index, free);
		if (CBAssociativeArrayIterate(&self->itData, &pos))
			break;
	}
	// Free index tx data
	CBFreeAssociativeArray(&self->staged.indexes);
	CBInitAssociativeArray(&self->staged.indexes, CBIndexCompare, NULL, CBFreeIndexTxData);
	// Reset valueWritingIndexes
	CBFreeAssociativeArray(&self->valueWritingIndexes);
	CBInitAssociativeArray(&self->valueWritingIndexes, CBIndexPtrCompare, NULL, NULL);
	self->numIndexes = 0;
	// Update commit time
	self->lastCommit = CBGetMilliseconds();
	// Make staged size zero
	self->stagedSize = 0;
	self->hasStaged = false;
	return true;
}
bool CBDatabaseEnsureConsistent(CBDatabase * self){
	char filename[strlen(self->dataDir) + strlen(self->folder) + 12];
	sprintf(filename, "%s/%s/log.dat", self->dataDir, self->folder);
	// Determine if the logfile has been created or not
	if (access(filename, F_OK))
		// Not created, so no history.
		return true;
	// Open the logfile for reading
	CBDepObject logFile;
	if (NOT CBFileOpen(&logFile,filename, false)) {
		CBLogError("The log file for reading could not be opened.");
		return false;
	}
	// Check if the logfile is active or not.
	uint8_t data[12];
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
	if (NOT CBFileRead(logFile, data, 11)) {
		CBLogError("Failed reading the logfile for the data and deletion index previous file information.");
		CBFileClose(logFile);
		return false;
	}
	// Truncate files
	char * filenameEnd = filename + strlen(self->dataDir) + strlen(self->folder) + 2;
	strcpy(filenameEnd, "del.dat");
	if (NOT CBFileTruncate(filename, CBArrayToInt32(data, 0))) {
		CBLogError("Failed to truncate the deletion index file down to the previous size.");
		CBFileClose(logFile);
		return false;
	}
	uint16_t lastFile = CBArrayToInt16(data, 4);
	sprintf(filenameEnd, "val_%u.dat", lastFile);
	if (NOT CBFileTruncate(filename, CBArrayToInt32(data, 6))) {
		CBLogError("Failed to truncate the last data file down to the previous size.");
		CBFileClose(logFile);
		return false;
	}
	// Check if a new file had been created. If it has, delete it.
	sprintf(filenameEnd, "val_%u.dat", lastFile + 1);
	if (NOT access(filename, F_OK))
		remove(filename);
	// Loop through indexes and truncate/delete files as necessary
	for (uint8_t x = 0; x < data[10]; x++) {
		if (NOT CBFileRead(logFile, data, 7)) {
			CBLogError("Failed reading the logfile for an index's previous file information.");
			CBFileClose(logFile);
			return false;
		}
		lastFile = CBArrayToInt16(data, 1);
		sprintf(filenameEnd, "idx_%u_%u.dat", data[0], lastFile);
		if (NOT CBFileTruncate(filename, CBArrayToInt32(data, 3))) {
			CBLogError("Failed to truncate the last file down to the previous size for an index.");
			CBFileClose(logFile);
			return false;
		}
		sprintf(filenameEnd, "idx_%u_%u.dat", data[0], lastFile + 1);
		if (NOT access(filename, F_OK))
			remove(filename);
	}
	// Reverse overwrite operations
	uint32_t logFileLen;
	if (NOT CBFileGetLength(logFile, &logFileLen)) {
		CBLogError("Failed to get the length of the log file.");
		CBFileClose(logFile);
		return false;
	}
	uint8_t * prevData = NULL;
	uint32_t prevDataSize = 0;
	for (uint32_t c = 14 + 7 * data[10]; c < logFileLen;) {
		// Read file type, index ID and file ID, offset and size
		if (NOT CBFileRead(logFile, data, 12)) {
			CBLogError("Could not read from the log file.");
			CBFileClose(logFile);
			return false;
		}
		uint32_t dataLen = CBArrayToInt32(data, 8);
		// Read previous data
		if (prevDataSize < dataLen) {
			prevData = realloc(prevData, dataLen);
			prevDataSize = dataLen;
		}
		if (NOT CBFileRead(logFile, prevData, dataLen)) {
			CBLogError("Could not read previous data from the log file.");
			CBFileClose(logFile);
			return false;
		}
		// Write the data to the file
		switch (data[0]) {
			case CB_DATABASE_FILE_TYPE_DATA:
				sprintf(filenameEnd, "val_%u.dat", CBArrayToInt16(data, 2));
				break;
			case CB_DATABASE_FILE_TYPE_DELETION_INDEX:
				strcpy(filenameEnd, "del.dat");
				break;
			case CB_DATABASE_FILE_TYPE_INDEX:
				sprintf(filenameEnd, "idx_%u_%u.dat", data[1], CBArrayToInt16(data, 2));
				break;
			default:
				break;
		}
		CBDepObject file;
		if (NOT CBFileOpen(&file, filename, false)) {
			CBLogError("Could not open the file for writting the previous data.");
			CBFileClose(logFile);
			return false;
		}
		if (NOT CBFileSeek(file, CBArrayToInt32(data, 4))){
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
		c += 12 + dataLen;
	}
	free(prevData);
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
		res.found = ((uint8_t *)CBFindResultToPointer(res))[1]
		&& CBArrayToInt32BigEndian(((uint8_t *)CBFindResultToPointer(res)), 2) >= length;
	}else
		res.found = false;
	return res;
}
bool CBDatabaseGetFile(CBDatabase * self, CBDepObject * file, CBDatabaseFileType type, uint8_t indexID, uint16_t fileID){
	if (fileID == self->lastUsedFileID
		&& type == self->lastUsedFileType
		&& indexID == self->lastUsedFileIndexID){
		*file = self->fileObjectCache;
		return true;
	}else if (type == CB_DATABASE_FILE_TYPE_DELETION_INDEX){
		*file = self->deletionIndexFile;
		return true;
	}else{
		// Change last used file cache
		if (self->lastUsedFileType != CB_DATABASE_FILE_TYPE_NONE) {
			// First synchronise previous file
			if (NOT CBFileSync(self->fileObjectCache)) {
				CBLogError("Could not synchronise a file when a new file is needed.");
				return false;
			}
			// Now close the file
			CBFileClose(self->fileObjectCache);
		}
		// Open the new file
		char filename[strlen(self->dataDir) + strlen(self->folder) + 18];
		switch (type) {
			case CB_DATABASE_FILE_TYPE_DATA:
				sprintf(filename, "%s/%s/val_%u.dat", self->dataDir, self->folder, fileID);
				break;
			case CB_DATABASE_FILE_TYPE_INDEX:
				sprintf(filename, "%s/%s/idx_%u_%u.dat", self->dataDir, self->folder, indexID, fileID);
				break;
			default:
				break;
		}
		if (NOT CBFileOpen(&self->fileObjectCache, filename, access(filename, F_OK))) {
			CBLogError("Could not open file %s.", filename);
			return 0;
		}
		self->lastUsedFileID = fileID;
		self->lastUsedFileIndexID = indexID;
		self->lastUsedFileType = type;
		*file = self->fileObjectCache;
		return true;
	}
}
bool CBDatabaseGetLength(CBDatabaseIndex * index, uint8_t * key, uint32_t * length) {
	uint8_t * diskKey = key;
	CBDatabase * self = index->database;
	// Get the index tx data
	for (CBDatabaseTransactionChanges * c = &self->current;; c = &self->staged) {
		CBIndexTxData * indexTxData = CBDatabaseGetIndexTxData(c, index, false);
		if (indexTxData) {
			// First check for CB_OVERWRITE_DATA value write entry as it replaces the length on disk.
			CBFindResult res;
			uint8_t compareData[index->keySize + 4];
			memcpy(compareData, diskKey, index->keySize);
			*(uint32_t *)(compareData + index->keySize) = CB_OVERWRITE_DATA;
			res = CBAssociativeArrayFind(&indexTxData->valueWrites, compareData);
			if (res.found){
				*length = CBArrayToInt32(((uint8_t *)CBFindResultToPointer(res)), index->keySize + 4);
				return true;
			}
			// Get the key as it should exist in the staged changes or in the disk
			indexTxData->changeKeysNew.compareFunc = CBNewKeyWithSingleKeyCompare;
			CBFindResult ckres = CBAssociativeArrayFind(&indexTxData->changeKeysNew, diskKey);
			if (ckres.found)
				diskKey = CBFindResultToPointer(ckres);
			indexTxData->changeKeysNew.compareFunc = CBNewKeyCompare;
		}
		if (c == &self->staged)
			break;
	}
	// Look in index for value
	CBIndexFindResult indexRes = CBDatabaseIndexFind(index, diskKey);
	if (indexRes.status == CB_DATABASE_INDEX_ERROR) {
		CBLogError("There was an error when attempting to find a key in an index to get the data length.");
		return false;
	}
	if (indexRes.status == CB_DATABASE_INDEX_NOT_FOUND){
		CBFreeIndexElementPos(indexRes.el);
		*length = CB_DOESNT_EXIST;
		return true;
	}
	*length = indexRes.el.nodeLoc.cachedNode->elements[indexRes.el.index].length;
	CBFreeIndexElementPos(indexRes.el);
	return true;
}
CBIndexItData * CBDatabaseGetIndexItData(CBDatabase * database, CBDatabaseIndex * index, bool create){
	CBFindResult res = CBAssociativeArrayFind(&database->itData, &(CBIndexItData){index});
	CBIndexItData * indexItData;
	if (NOT res.found) {
		// Can't find the index it data so create it unless create is false.
		if (NOT create)
			return NULL;
		indexItData = malloc(sizeof(*indexItData));
		indexItData->index = index;
		// Initialise arrays
		CBInitAssociativeArray(&indexItData->txKeys, CBTransactionKeysCompare, index, free);
		CBInitAssociativeArray(&indexItData->currentAddedKeys, CBTransactionKeysCompare, index, NULL);
		// Insert into the array.
		CBAssociativeArrayInsert(&database->itData, indexItData, res.position, NULL);
	}else
		indexItData = CBFindResultToPointer(res);
	return indexItData;
}
CBIndexTxData * CBDatabaseGetIndexTxData(CBDatabaseTransactionChanges * tx, CBDatabaseIndex * index, bool create){
	CBFindResult res = CBAssociativeArrayFind(&tx->indexes, &(CBIndexTxData){index});
	CBIndexTxData * indexTxData;
	if (NOT res.found) {
		// Can't find the index tx data so create it unless create is false.
		if (NOT create)
			return NULL;
		indexTxData = malloc(sizeof(*indexTxData));
		indexTxData->index = index;
		// Initialise arrays
		CBInitAssociativeArray(&indexTxData->changeKeysOld, CBTransactionKeysCompare, index, free);
		CBInitAssociativeArray(&indexTxData->changeKeysNew, CBNewKeyCompare, index, NULL);
		CBInitAssociativeArray(&indexTxData->deleteKeys, CBTransactionKeysCompare, index, free);
		CBInitAssociativeArray(&indexTxData->valueWrites, CBTransactionKeysAndOffsetCompare, index, free);
		// Insert into the array.
		CBAssociativeArrayInsert(&tx->indexes, indexTxData, res.position, NULL);
	}else
		indexTxData = CBFindResultToPointer(res);
	return indexTxData;
}
bool CBDatabaseIndexDelete(CBDatabaseIndex * index, CBIndexFindResult * res){
	// Create deletion entry for the data
	CBIndexValue * indexValue = &res->el.nodeLoc.cachedNode->elements[res->el.index];
	if (NOT CBDatabaseAddDeletionEntry(index->database, indexValue->fileID, indexValue->pos, indexValue->length)) {
		CBLogError("Failed to create a deletion entry for a key-value.");
		return false;
	}
	// Make the length CB_DELETED_VALUE, signifying deletion of the data
	indexValue->length = CB_DELETED_VALUE;
	uint8_t data[4];
	CBInt32ToArray(data, 0, CB_DELETED_VALUE);
	if (NOT CBDatabaseAddOverwrite(index->database, CB_DATABASE_FILE_TYPE_INDEX, index->ID, res->el.nodeLoc.diskNodeLoc.indexFile, data, res->el.nodeLoc.diskNodeLoc.offset + 3 + (index->keySize + 10)*res->el.index, 4)) {
		CBLogError("Failed to overwrite the index entry's length with CB_DELETED_VALUE to signify deletion.");
		return false;
	}
	return true;
}
CBIndexFindResult CBDatabaseIndexFind(CBDatabaseIndex * index, uint8_t * key){
	CBIndexFindResult result;
	CBIndexNodeLocation nodeLoc;
	nodeLoc = index->indexCache;
	result.el.parentStack.num = 0;
	result.el.parentStack.cursor = 0;
	for (;;) {
		// Do binary search on node
		CBDatabaseIndexNodeBinarySearch(index, nodeLoc.cachedNode, key, &result);
		if (result.status == CB_DATABASE_INDEX_FOUND){
			// Found the data on this node.
			result.el.nodeLoc = nodeLoc;
			return result;
		}else{
			// We have not found the data on this node, if there is a child go to the child.
			CBIndexNodeLocation * childLoc = &nodeLoc.cachedNode->children[result.el.index];
			if (childLoc->cached
				|| childLoc->diskNodeLoc.indexFile != 0
				|| childLoc->diskNodeLoc.offset != 0) {
				// Set parent information
				if (nodeLoc.cachedNode != index->indexCache.cachedNode){
					result.el.parentStack.nodes[result.el.parentStack.num++] = nodeLoc;
					result.el.parentStack.cursor++;
				}
				result.el.parentStack.pos[result.el.parentStack.num] = result.el.index;
				// Child exists, move to it
				nodeLoc = *childLoc;
				// Load node from disk if necessary
				if (NOT nodeLoc.cached){
					// Allocate memory for the node
					nodeLoc.cachedNode = malloc(sizeof(*nodeLoc.cachedNode));
					if (NOT CBDatabaseLoadIndexNode(index, nodeLoc.cachedNode, nodeLoc.diskNodeLoc.indexFile, nodeLoc.diskNodeLoc.offset)) {
						CBLogError("Could not load an index node from the disk.");
						result.status = CB_DATABASE_INDEX_ERROR;
						return result;
					}
				}
			}else{
				// The child doesn't exist. Return as not found.
				result.el.nodeLoc = nodeLoc;
				return result;
			}
		}
	}
}
bool CBDatabaseIndexInsert(CBDatabaseIndex * index, CBIndexValue * indexVal, CBIndexElementPos * pos, CBIndexNodeLocation * right){
	bool bottom = pos->nodeLoc.cachedNode->children[0].diskNodeLoc.indexFile == 0 && pos->nodeLoc.cachedNode->children[0].diskNodeLoc.offset == 0;
	// See if we can insert data in this node
	if (pos->nodeLoc.cachedNode->numElements < CB_DATABASE_BTREE_ELEMENTS) {
		if (pos->nodeLoc.cachedNode->numElements > pos->index){
			// Move up the elements and children
			if (NOT CBDatabaseIndexMoveElements(index, &pos->nodeLoc, &pos->nodeLoc, pos->index, pos->index + 1, pos->nodeLoc.cachedNode->numElements - pos->index, false)
				|| (NOT bottom && NOT CBDatabaseIndexMoveChildren(index, &pos->nodeLoc, &pos->nodeLoc, pos->index + 1, pos->index + 2, pos->nodeLoc.cachedNode->numElements - pos->index, false))) {
				CBLogError("Could not move elements and children up one in a node for inserting a new element.");
				return false;
			}
		}
		// Insert element and right child
		if (NOT CBDatabaseIndexSetElement(index, &pos->nodeLoc, indexVal, pos->index, false)
			|| (right && NOT CBDatabaseIndexSetChild(index, &pos->nodeLoc, right, pos->index + 1, false))){
			CBLogError("Could not set an element and child for adding to an unfull index node.");
			return false;
		}
		// Increase number of elements
		if (NOT CBDatabaseIndexSetNumElements(index, &pos->nodeLoc, pos->nodeLoc.cachedNode->numElements + 1, false)){
			CBLogError("Could not increment the number of elements for an index node when inserting a node.");
			return false;
		}
		// The element has been added, we can stop here.
		return true;
	}else{
		// We need to split this node into two. Create cached node if we can.
		CBIndexNodeLocation new;
		if ((index->numCached + 1) * (index->keySize * CB_DATABASE_BTREE_ELEMENTS + sizeof(CBIndexNode)) <= index->cacheLimit){
			new.cached = true;
			new.cachedNode = malloc(sizeof(*new.cachedNode));
			index->numCached++;
		}else
			new.cached = false;
		// Get position for new node
		CBDatabaseIndexSetNewNodePosition(index, &new);
		// Make both sides have half the order.
		if (NOT CBDatabaseIndexSetNumElements(index, &pos->nodeLoc, CB_DATABASE_BTREE_HALF_ELEMENTS, false)
			|| NOT CBDatabaseIndexSetNumElements(index, &new, CB_DATABASE_BTREE_HALF_ELEMENTS, true)) {
			CBLogError("Could not set two split nodes ot have the number of elements equal to half the order.");
			return false;
		}
		CBIndexValue middleEl;
		// Check where to insert new element
		if (pos->index > CB_DATABASE_BTREE_HALF_ELEMENTS) {
			// In the new node.
			if (
				// Copy over first part of the new child
				NOT CBDatabaseIndexMoveElements(index, &new, &pos->nodeLoc, CB_DATABASE_BTREE_HALF_ELEMENTS + 1, 0, pos->index - CB_DATABASE_BTREE_HALF_ELEMENTS - 1, true)
				// Copy over the element
				|| NOT CBDatabaseIndexSetElement(index, &new, indexVal, pos->index - CB_DATABASE_BTREE_HALF_ELEMENTS - 1, true)
				// Copy remainder of elements
				|| NOT CBDatabaseIndexMoveElements(index, &new, &pos->nodeLoc, pos->index, pos->index - CB_DATABASE_BTREE_HALF_ELEMENTS, CB_DATABASE_BTREE_ELEMENTS - pos->index, true)
				// Set remainder of elements as blank
				|| NOT CBDatabaseAppendZeros(index->database, CB_DATABASE_FILE_TYPE_INDEX, index->ID, new.diskNodeLoc.indexFile, CB_DATABASE_BTREE_HALF_ELEMENTS * (index->keySize + 10))) {
				CBLogError("Could not move elements including the new element into the new node");
				return false;
			}
			if (NOT bottom
				&& (NOT CBDatabaseIndexMoveChildren(index, &new, &pos->nodeLoc, CB_DATABASE_BTREE_HALF_ELEMENTS + 1, 0, pos->index - CB_DATABASE_BTREE_HALF_ELEMENTS, true)
					// Set the right child
					|| NOT CBDatabaseIndexSetChild(index, &new, right, pos->index - CB_DATABASE_BTREE_HALF_ELEMENTS, true)
					// Set the remaining children
					|| NOT CBDatabaseIndexMoveChildren(index, &new, &pos->nodeLoc, pos->index + 1, pos->index - CB_DATABASE_BTREE_HALF_ELEMENTS + 1, CB_DATABASE_BTREE_ELEMENTS - pos->index, true))){
					CBLogError("Could not insert children into new node.");
					return false;
				}
			// Set middle element
			middleEl = pos->nodeLoc.cachedNode->elements[CB_DATABASE_BTREE_HALF_ELEMENTS];
		}else if (pos->index == CB_DATABASE_BTREE_HALF_ELEMENTS) {
			// Middle
			if (
				// Copy elements
				NOT CBDatabaseIndexMoveElements(index, &new, &pos->nodeLoc, CB_DATABASE_BTREE_HALF_ELEMENTS, 0, CB_DATABASE_BTREE_HALF_ELEMENTS, true)
				// Set remaining elements as blank
				|| NOT CBDatabaseAppendZeros(index->database, CB_DATABASE_FILE_TYPE_INDEX, index->ID, new.diskNodeLoc.indexFile, CB_DATABASE_BTREE_HALF_ELEMENTS * (index->keySize + 10))){
				CBLogError("Could not move elements into the new node, with the new element being the middle element.");
				return false;
			}
			if (NOT bottom
				&& (// Set right child
					NOT CBDatabaseIndexSetChild(index, &new, right, 0, true)
					// Copy children
					|| NOT CBDatabaseIndexMoveChildren(index, &new, &pos->nodeLoc, CB_DATABASE_BTREE_HALF_ELEMENTS + 1, 1, CB_DATABASE_BTREE_HALF_ELEMENTS, true))) {
					CBLogError("Could not move children into the new node, with the new element being the middle element.");
					return false;
				}
			// Middle value is inserted value
			middleEl = *indexVal;
		}else{
			// In the first node.
			// Set middle element. Do this before moving anything as the elements array will be modified.
			middleEl = pos->nodeLoc.cachedNode->elements[CB_DATABASE_BTREE_HALF_ELEMENTS-1];
			if (// Copy elements into new node
				NOT CBDatabaseIndexMoveElements(index, &new, &pos->nodeLoc, CB_DATABASE_BTREE_HALF_ELEMENTS, 0, CB_DATABASE_BTREE_HALF_ELEMENTS, true)
				// Set remaining elements as blank
				|| NOT CBDatabaseAppendZeros(index->database, CB_DATABASE_FILE_TYPE_INDEX, index->ID, new.diskNodeLoc.indexFile, CB_DATABASE_BTREE_HALF_ELEMENTS * (index->keySize + 10))
				// Move elements to make room for new element
				|| NOT CBDatabaseIndexMoveElements(index, &pos->nodeLoc, &pos->nodeLoc, pos->index, pos->index + 1, CB_DATABASE_BTREE_HALF_ELEMENTS - pos->index, false)
				// Move new element into old node
				|| NOT CBDatabaseIndexSetElement(index, &pos->nodeLoc, indexVal, pos->index, false)) {
				CBLogError("Could not insert the new element into the old node and move elements to the new node.");
				return false;
			}
			if (NOT bottom
				&& (// Move children in old node to make room for the right child
					NOT CBDatabaseIndexMoveChildren(index, &pos->nodeLoc, &pos->nodeLoc, pos->index + 1, pos->index + 2, CB_DATABASE_BTREE_HALF_ELEMENTS - pos->index - 1, false)
					// Insert the right child
					|| NOT CBDatabaseIndexSetChild(index, &pos->nodeLoc, right, pos->index + 1, false)
					// Copy children into new node
					|| NOT CBDatabaseIndexMoveChildren(index, &new, &pos->nodeLoc, CB_DATABASE_BTREE_HALF_ELEMENTS, 0, CB_DATABASE_BTREE_HALF_ELEMENTS + 1, true))) {
					CBLogError("Could not insert the children to the new node when the new element is in the first node.");
					return false;
				}
		}
		if (bottom){
			// If in bottom make all children blank.
			if (NOT CBDatabaseIndexSetBlankChildren(index, &new, 0, CB_DATABASE_BTREE_ELEMENTS + 1)) {
				CBLogError("Could not insert blank children into the new bottom node.");
				return false;
			}
			// Else set only the last CB_DATABASE_BTREE_HALF_ELEMENTS as blank
		}else if (NOT CBDatabaseIndexSetBlankChildren(index, &new, CB_DATABASE_BTREE_HALF_ELEMENTS, CB_DATABASE_BTREE_HALF_ELEMENTS)){
			CBLogError("Could not insert blank children into the new not bottom node.");
			return false;
		}
		// If the root was split, create a new root
		if (pos->nodeLoc.cachedNode == index->indexCache.cachedNode) {
			index->numCached++;
			CBIndexNode * newRootCache = malloc(sizeof(*newRootCache));
			index->indexCache.cachedNode = newRootCache;
			// Write the new root node to disk
			CBDatabaseIndexSetNewNodePosition(index, &index->indexCache);
			// Write number of elements which is one
			if (NOT CBDatabaseIndexSetNumElements(index, &index->indexCache, 1, true)) {
				CBLogError("Could not write the number of elements to a new root node.");
				return false;
			}
			// Write the middle element
			if (NOT CBDatabaseIndexSetElement(index, &index->indexCache, &middleEl, 0, true)) {
				CBLogError("Could not set the middle element to a new root node.");
				return false;
			}
			// Write blank elements
			uint16_t elementsSize = (10+index->keySize)*(CB_DATABASE_BTREE_ELEMENTS-1);
			if (NOT CBDatabaseAppendZeros(index->database, CB_DATABASE_FILE_TYPE_INDEX, index->ID, index->newLastFile, elementsSize)) {
				CBLogError("Could not write the blank elements for a new root node.");
				return false;
			}
			// Write left child
			if (NOT CBDatabaseIndexSetChild(index, &index->indexCache, &pos->nodeLoc, 0, true)) {
				CBLogError("Could not write the left child (old root) for a new root node.");
				return false;
			}
			// Write right child
			if (NOT CBDatabaseIndexSetChild(index, &index->indexCache, &new, 1, true)) {
				CBLogError("Could not write the right child (new node) for a new root node.");
				return false;
			}
			// Write blank children
			if (NOT CBDatabaseIndexSetBlankChildren(index, &index->indexCache, 2, CB_DATABASE_BTREE_ELEMENTS-1)) {
				CBLogError("Could not write the blank children for a new root node.");
				return false;
			}
			uint8_t data[6];
			// Overwrite root information in index
			CBInt16ToArray(data, 0, index->indexCache.diskNodeLoc.indexFile);
			CBInt32ToArray(data, 2, index->indexCache.diskNodeLoc.offset);
			if (NOT CBDatabaseAddOverwrite(index->database, CB_DATABASE_FILE_TYPE_INDEX, index->ID, 0, data, 6, 6)) {
				CBLogError("Could not add an overwrite operation for the root node information in the index on disk.");
				return false;
			}
			// All done
			return true;
		}
		// Get the new pos for inserting the middle value and right node
		if (pos->parentStack.cursor)
			pos->nodeLoc = pos->parentStack.nodes[pos->parentStack.cursor-1];
		else
			pos->nodeLoc = index->indexCache;
		pos->index = pos->parentStack.pos[pos->parentStack.cursor--];
		// Insert the middle value into the parent with the right node.
		return CBDatabaseIndexInsert(index, &middleEl, pos, &new);
	}
}
bool CBDatabaseIndexMoveChildren(CBDatabaseIndex * index, CBIndexNodeLocation * dest, CBIndexNodeLocation * source, uint8_t startPos, uint8_t endPos, uint8_t amount, bool append){
	if (amount == 0)
		return true;
	// Create the data to write the moved children
	uint8_t data[6 * amount];
	uint8_t * dataCursor = data;
	for (uint8_t x = 0; x < amount; x++) {
		CBInt16ToArray(dataCursor, 0, source->cachedNode->children[startPos + x].diskNodeLoc.indexFile);
		CBInt32ToArray(dataCursor, 2, source->cachedNode->children[startPos + x].diskNodeLoc.offset);
		dataCursor += 6;
	}
	// Write the children to the destination node
	if (append) {
		if (NOT CBDatabaseAppend(index->database, CB_DATABASE_FILE_TYPE_INDEX, index->ID, dest->diskNodeLoc.indexFile, data, 6 * amount)){
			CBLogError("Could not write the index children being moved by appending.");
			return false;
		}
	} else if (NOT CBDatabaseAddOverwrite(index->database, CB_DATABASE_FILE_TYPE_INDEX, index->ID, dest->diskNodeLoc.indexFile, data, dest->diskNodeLoc.offset + 1 + CB_DATABASE_BTREE_ELEMENTS * (index->keySize + 10) + endPos * 6, 6 * amount)){
		CBLogError("Could not write the index children being moved.");
		return false;
	}
	if (dest->cached)
		// Move children for the cached destination node.
		memmove(dest->cachedNode->children + endPos, source->cachedNode->children + startPos, amount * sizeof(*source->cachedNode->children));
	return true;
}
bool CBDatabaseIndexMoveElements(CBDatabaseIndex * index, CBIndexNodeLocation * dest, CBIndexNodeLocation * source, uint8_t startPos, uint8_t endPos, uint8_t amount, bool append){
	if (amount == 0)
		return true;
	// Create the data to write the moved elements.
	uint8_t elSize = index->keySize + 10;
	uint16_t moveSize = amount * elSize;
	uint8_t data[moveSize];
	uint8_t * dataCursor = data;
	for (uint8_t x = 0; x < amount; x++) {
		CBInt16ToArray(dataCursor, 0, source->cachedNode->elements[startPos + x].fileID);
		CBInt32ToArray(dataCursor, 2, source->cachedNode->elements[startPos + x].length);
		CBInt32ToArray(dataCursor, 6, source->cachedNode->elements[startPos + x].pos);
		// Add key to data
		memcpy(dataCursor + 10, source->cachedNode->elements[startPos + x].key, index->keySize);
		dataCursor += elSize;
	}
	// Write the elements to the destination node
	if (append) {
		if (NOT CBDatabaseAppend(index->database, CB_DATABASE_FILE_TYPE_INDEX, index->ID, dest->diskNodeLoc.indexFile, data, moveSize)){
			CBLogError("Could not write the index elements being moved by appending.");
			return false;
		}
	}else if (NOT CBDatabaseAddOverwrite(index->database, CB_DATABASE_FILE_TYPE_INDEX, index->ID, dest->diskNodeLoc.indexFile, data, dest->diskNodeLoc.offset + 1 + endPos * elSize, moveSize)){
		CBLogError("Could not write the index elements being moved.");
		return false;
	}
	if (dest->cached)
		// Move elements for the cached destination node.
		memmove(dest->cachedNode->elements + endPos, source->cachedNode->elements + startPos, amount * sizeof(*source->cachedNode->elements));
	return true;
}
void CBDatabaseIndexNodeBinarySearch(CBDatabaseIndex * index, CBIndexNode * node, uint8_t * key, CBIndexFindResult * result){
	result->status = CB_DATABASE_INDEX_NOT_FOUND;
	if (node->numElements) {
		uint8_t left = 0;
		uint8_t right = node->numElements - 1;
		int cmp;
		while (left <= right) {
			result->el.index = (right+left)/2;
			cmp = memcmp(key, node->elements[result->el.index].key, index->keySize);
			if (cmp == 0) {
				result->status = CB_DATABASE_INDEX_FOUND;
				break;
			}else if (cmp < 0){
				if (NOT result->el.index)
					break;
				right = result->el.index - 1;
			}else
				left = result->el.index + 1;
		}
		if (cmp > 0)
			result->el.index++;
	}else
		result->el.index = 0;
}
bool CBDatabaseIndexSetBlankChildren(CBDatabaseIndex * index, CBIndexNodeLocation * dest, uint8_t offset, uint8_t amount){
	if (NOT CBDatabaseAppendZeros(index->database, CB_DATABASE_FILE_TYPE_INDEX, index->ID, dest->diskNodeLoc.indexFile, amount * 6)) {
		CBLogError("Could not append blank children to the new node.");
		return false;
	}
	if (dest->cached && offset == 0)
		memset(dest->cachedNode->children, 0, sizeof(*dest->cachedNode->children));
	return true;
}
bool CBDatabaseIndexSetChild(CBDatabaseIndex * index, CBIndexNodeLocation * node, CBIndexNodeLocation * child, uint8_t pos, bool append){
	if (node->cached)
		// Set the child for the cached node.
		node->cachedNode->children[pos] = *child;
	// Write the child to the node on disk.
	uint8_t data[6];
	CBInt16ToArray(data, 0, child->diskNodeLoc.indexFile);
	CBInt32ToArray(data, 2, child->diskNodeLoc.offset);
	if (append) {
		if (NOT CBDatabaseAppend(index->database, CB_DATABASE_FILE_TYPE_INDEX, index->ID, node->diskNodeLoc.indexFile, data, 6)){
			CBLogError("Could not set a child on a node on disk by appending.");
			return false;
		}
	}else if (NOT CBDatabaseAddOverwrite(index->database, CB_DATABASE_FILE_TYPE_INDEX, index->ID, node->diskNodeLoc.indexFile, data, node->diskNodeLoc.offset + 1 + CB_DATABASE_BTREE_ELEMENTS * (index->keySize + 10) + pos * 6, 6)){
		CBLogError("Could not set a child on a node on disk.");
		return false;
	}
	return true;
}
bool CBDatabaseIndexSetElement(CBDatabaseIndex * index, CBIndexNodeLocation * node, CBIndexValue * element, uint8_t pos, bool append){
	if (node->cached)
		// Add the element to the cached node.
		node->cachedNode->elements[pos] = *element;
	// Write the element to the node on disk
	// First create data
	uint8_t elSize = index->keySize + 10;
	uint8_t data[elSize];
	CBInt16ToArray(data, 0, element->fileID);
	CBInt32ToArray(data, 2, element->length);
	CBInt32ToArray(data, 6, element->pos);
	memcpy(data + 10, element->key, index->keySize);
	// Now write data
	if (append) {
		if (NOT CBDatabaseAppend(index->database, CB_DATABASE_FILE_TYPE_INDEX, index->ID, node->diskNodeLoc.indexFile, data, elSize)){
			CBLogError("Could not write an index element being added to a node on disk by appending.");
			return false;
		}
	}else if (NOT CBDatabaseAddOverwrite(index->database, CB_DATABASE_FILE_TYPE_INDEX, index->ID, node->diskNodeLoc.indexFile, data, node->diskNodeLoc.offset + 1 + pos * elSize, elSize)){
		CBLogError("Could not write an index element being added to a node on disk.");
		return false;
	}
	return true;
}
void CBDatabaseIndexSetNewNodePosition(CBDatabaseIndex * index, CBIndexNodeLocation * new){
	uint32_t remaining = INT32_MAX - index->newLastSize;
	uint16_t nodeSize = (index->keySize + 16) * CB_DATABASE_BTREE_ELEMENTS + 7;
	if (nodeSize > remaining) {
		// Use new file
		new->diskNodeLoc.indexFile = ++index->newLastFile;
		new->diskNodeLoc.offset = 0;
		index->newLastSize = nodeSize;
	}else{
		// Append to existing file
		new->diskNodeLoc.indexFile = index->newLastFile;
		new->diskNodeLoc.offset = index->newLastSize;
		index->newLastSize += nodeSize;
	}
}
bool CBDatabaseIndexSetNumElements(CBDatabaseIndex * index, CBIndexNodeLocation * node, uint8_t numElements, bool append){
	if (node->cached)
		node->cachedNode->numElements = numElements;
	if (append) {
		if (NOT CBDatabaseAppend(index->database, CB_DATABASE_FILE_TYPE_INDEX, index->ID, node->diskNodeLoc.indexFile, &numElements, 1)) {
			CBLogError("Could not update the number of elements in storage for an index node by appending.");
			return false;
		}
	}else if (NOT CBDatabaseAddOverwrite(index->database, CB_DATABASE_FILE_TYPE_INDEX, index->ID, node->diskNodeLoc.indexFile, &numElements, node->diskNodeLoc.offset, 1)) {
		CBLogError("Could not update the number of elements in storage for an index node.");
		return false;
	}
	return true;
}
CBIndexFindStatus CBDatabaseRangeIteratorFirst(CBDatabaseRangeIterator * it){
	// Go to the begining of the txKeys
	it->txIt.minElement = it->minElement;
	it->txIt.maxElement = it->maxElement;
	CBIndexItData * indexItData = CBDatabaseGetIndexItData(it->index->database, it->index, false);
	it->gotTxEl = indexItData ? CBAssociativeArrayRangeIteratorStart(&indexItData->txKeys, &it->txIt) : false;
	// Try to find the minimum index element
	CBIndexFindResult res = CBDatabaseIndexFind(it->index, it->minElement);
	it->foundIndex = false;
	if (res.status == CB_DATABASE_INDEX_ERROR) {
		CBLogError("There was an error when attempting to find the first index element in a range.");
		return res.status;
	}
	if (res.status == CB_DATABASE_INDEX_NOT_FOUND) {
		if (res.el.nodeLoc.cachedNode->numElements == 0){
			// There are no elements
			it->gotIndEl = false;
			return it->gotTxEl ? CB_DATABASE_INDEX_FOUND : CB_DATABASE_INDEX_NOT_FOUND;
		}
		// If we are past the end of the node's elements, go back onto the previous element, and then iterate.
		if (res.el.index == res.el.nodeLoc.cachedNode->numElements) {
			res.el.index--;
			it->indexPos = res.el;
			it->foundIndex = true;
			CBIndexFindStatus ret = CBDatabaseRangeIteratorIterateIndex(it, true);
			return it->gotTxEl ? CB_DATABASE_INDEX_FOUND : ret;
		}
		// Else we have landed on the element after the minimum element.
		// Check that the element is below or equal to the maximum
		if (memcmp(res.el.nodeLoc.cachedNode->elements[res.el.index].key, it->maxElement, it->index->keySize) > 0){
			// The element is above the maximum
			it->gotIndEl = false;
			return it->gotTxEl ? CB_DATABASE_INDEX_FOUND : CB_DATABASE_INDEX_NOT_FOUND;
		}
	}
	it->indexPos = res.el;
	it->foundIndex = true;
	it->gotIndEl = true;
	// If the element is to be deleted, iterate.
	return CBDatabaseRangeIteratorIterateIfDeletion(it, true);
}
uint8_t * CBDatabaseRangeIteratorGetKey(CBDatabaseRangeIterator * it){
	return (CBDatabaseRangeIteratorGetLesser(it) < 0) ?
	// Transaction key is the least
	((uint8_t *)CBRangeIteratorGetPointer(&it->txIt))
	// The index key is the least or both are the same.
	: it->indexPos.nodeLoc.cachedNode->elements[it->indexPos.index].key;
}
int CBDatabaseRangeIteratorGetLesser(CBDatabaseRangeIterator * it){
	if (it->gotTxEl && it->gotIndEl)
		return memcmp(((uint8_t *)CBRangeIteratorGetPointer(&it->txIt)),
						it->indexPos.nodeLoc.cachedNode->elements[it->indexPos.index].key,
						it->index->keySize);
	return it->gotIndEl ? 1 : -1;
}
CBIndexFindStatus CBDatabaseRangeIteratorLast(CBDatabaseRangeIterator * it){
	// Go to the end of the txKeys
	it->txIt.minElement = it->minElement;
	it->txIt.maxElement = it->maxElement;
	CBIndexItData * indexItData = CBDatabaseGetIndexItData(it->index->database, it->index, false);
	it->gotTxEl = indexItData ? CBAssociativeArrayRangeIteratorLast(&indexItData->txKeys, &it->txIt) : false;
	// Try to find the maximum index element
	CBIndexFindResult res = CBDatabaseIndexFind(it->index, it->maxElement);
	it->foundIndex = false;
	if (res.status == CB_DATABASE_INDEX_ERROR) {
		CBLogError("There was an error when attempting to find the first index element in a range.");
		return res.status;
	}
	if (res.status == CB_DATABASE_INDEX_NOT_FOUND) {
		if (res.el.nodeLoc.cachedNode->numElements == 0) {
			// There are no elements
			it->gotIndEl = false;
			return it->gotTxEl ? CB_DATABASE_INDEX_FOUND : CB_DATABASE_INDEX_NOT_FOUND;
		}
		// Not found goes to the bottom.
		if (res.el.index == 0) {
			// No child, try parent
			if (res.el.nodeLoc.cachedNode == it->index->indexCache.cachedNode) {
				it->gotIndEl = false;
				return it->gotTxEl ? CB_DATABASE_INDEX_FOUND : CB_DATABASE_INDEX_NOT_FOUND;
			}
			if (NOT res.el.nodeLoc.cached)
				CBFreeIndexNode(res.el.nodeLoc.cachedNode);
			if (res.el.parentStack.num)
				res.el.nodeLoc = res.el.parentStack.nodes[res.el.parentStack.num-1];
			else
				res.el.nodeLoc = it->index->indexCache;
			res.el.index = res.el.parentStack.pos[res.el.parentStack.num--];
			if (res.el.index == 0) {
				it->gotIndEl = false;
				return it->gotTxEl ? CB_DATABASE_INDEX_FOUND : CB_DATABASE_INDEX_NOT_FOUND;
			}
			res.el.index--;
		}else // Else we can just go back one in this node
			res.el.index--;
		// Check that the element is above or equal to the minimum
		if (memcmp(res.el.nodeLoc.cachedNode->elements[res.el.index].key, it->minElement, it->index->keySize) < 0){
			// The element is below the minimum
			it->gotIndEl = false;
			return it->gotTxEl ? CB_DATABASE_INDEX_FOUND : CB_DATABASE_INDEX_NOT_FOUND;
		}
	}
	it->indexPos = res.el;
	it->foundIndex = true;
	// If the element is to be deleted, iterate.
	it->gotIndEl = true;
	CBIndexFindStatus status = CBDatabaseRangeIteratorIterateIfDeletion(it, false);
	if (status == CB_DATABASE_INDEX_ERROR) {
		CBLogError("Could not reverse iterate to an element which is not marked as deleted.");
		return status;
	}
	if (status == CB_DATABASE_INDEX_NOT_FOUND)
		return it->gotTxEl ? CB_DATABASE_INDEX_FOUND : CB_DATABASE_INDEX_NOT_FOUND;
	if (NOT it->gotTxEl)
		return CB_DATABASE_INDEX_FOUND;
	// Now only use biggest key
	if (CBDatabaseRangeIteratorGetLesser(it) < 0)
		// Transaction is least.
		it->gotTxEl = false;
	else
		// Index is least or both are the same
		it->gotIndEl = false;
	return CB_DATABASE_INDEX_FOUND;
}
CBIndexFindStatus CBDatabaseRangeIteratorNext(CBDatabaseRangeIterator * it){
	// Iterate lowest key, or both if they are the same.
	int cmpres = CBDatabaseRangeIteratorGetLesser(it);
	if (cmpres <= 0) {
		// Iterate transaction iterator to new key, and continue whilst we land on removed value.
		CBIndexTxData * currentTxData = CBDatabaseGetIndexTxData(&it->index->database->current, it->index, false);
		CBIndexTxData * stagedTxData = CBDatabaseGetIndexTxData(&it->index->database->staged, it->index, false);
		for (;;) {
			if (CBAssociativeArrayRangeIteratorNext(&CBDatabaseGetIndexItData(it->index->database, it->index, false)->txKeys, &it->txIt)){
				it->gotTxEl = false;
				break;
			}
			// Check to see if removed, if not we can end here
			uint8_t * key = CBRangeIteratorGetPointer(&it->txIt);
			if ((NOT currentTxData || NOT CBAssociativeArrayFind(&currentTxData->deleteKeys, key).found)
				&& (NOT stagedTxData || NOT CBAssociativeArrayFind(&stagedTxData->deleteKeys, key).found))
				break;
		}
	}
	if (cmpres >= 0)
		// The transaction key was not less so iterate the index iterator
		if (CBDatabaseRangeIteratorIterateIndex(it, true) == CB_DATABASE_INDEX_ERROR)
			return CB_DATABASE_INDEX_ERROR;
	return (it->gotTxEl || it->gotIndEl) ? CB_DATABASE_INDEX_FOUND : CB_DATABASE_INDEX_NOT_FOUND;
}
CBIndexFindStatus CBDatabaseRangeIteratorIterateIfDeletion(CBDatabaseRangeIterator * it, bool forwards){
	// Get the index tx data
	uint8_t * findKey = it->indexPos.nodeLoc.cachedNode->elements[it->indexPos.index].key;
	CBIndexTxData * currentIndexTxData = CBDatabaseGetIndexTxData(&it->index->database->current, it->index, false);
	bool deleted = currentIndexTxData && (CBAssociativeArrayFind(&currentIndexTxData->deleteKeys, findKey).found
										  // Check old keys to see if this key is changed and thus cannot be used.
										  || CBAssociativeArrayFind(&currentIndexTxData->changeKeysOld, findKey).found);
	if (NOT deleted) {
		// Look in staged changes for deletion with no value writes or new change keys in current changes.
		CBIndexTxData * stagedIndexTxData = CBDatabaseGetIndexTxData(&it->index->database->staged, it->index, false);
		deleted = stagedIndexTxData && (CBAssociativeArrayFind(&stagedIndexTxData->deleteKeys, findKey).found
											  || CBAssociativeArrayFind(&stagedIndexTxData->changeKeysOld, findKey).found);
	}
	if (deleted){
		CBIndexFindStatus ret = CBDatabaseRangeIteratorIterateIndex(it, forwards);
		return it->gotTxEl ? CB_DATABASE_INDEX_FOUND : ret;
	}
	return CB_DATABASE_INDEX_FOUND;
}
CBIndexFindStatus CBDatabaseRangeIteratorIterateIndex(CBDatabaseRangeIterator * it, bool forwards){
	// First look for child
	CBIndexNodeLocation child = it->indexPos.nodeLoc.cachedNode->children[forwards ? (it->indexPos.index + 1) : 0];
	if (child.diskNodeLoc.indexFile == 0 && child.diskNodeLoc.offset == 0) {
		// No child, move along node if possible.
		if (it->indexPos.index == (forwards ? (it->indexPos.nodeLoc.cachedNode->numElements - 1) : 0)) {
			// Not possible, free this node and then move to parent unless root, and loop until we find the next element
			do{
				if (it->indexPos.nodeLoc.cachedNode == it->index->indexCache.cachedNode){
					// We are at the root, there are no more elements
					it->gotIndEl = false;
					return CB_DATABASE_INDEX_NOT_FOUND;
				}
				if (NOT it->indexPos.nodeLoc.cached)
					// This is not permenently cached so free
					CBFreeIndexNode(it->indexPos.nodeLoc.cachedNode);
				if (it->indexPos.parentStack.num)
					it->indexPos.nodeLoc = it->indexPos.parentStack.nodes[it->indexPos.parentStack.num-1];
				else
					it->indexPos.nodeLoc = it->index->indexCache;
				it->indexPos.index = it->indexPos.parentStack.pos[it->indexPos.parentStack.num];
				if (it->indexPos.parentStack.num)
					it->indexPos.parentStack.num--;
			} while(it->indexPos.index == (forwards ? it->indexPos.nodeLoc.cachedNode->numElements : 0));
			if (NOT forwards)
				it->indexPos.index--;
		}else
			forwards ? it->indexPos.index++ : it->indexPos.index--;
	}else{
		// There is a child, move to it and keep moving until leaf is reached.
		bool first = true;
		for (;;) {
			// First set the parent information
			if (it->indexPos.nodeLoc.cachedNode != it->index->indexCache.cachedNode){
				it->indexPos.parentStack.nodes[it->indexPos.parentStack.num++] = it->indexPos.nodeLoc;
				it->indexPos.parentStack.cursor++;
			}
			it->indexPos.parentStack.pos[it->indexPos.parentStack.num] = it->indexPos.index + first*(forwards ? 1 : -1);
			first = false;
			// Load node from disk if necessary
			if (NOT child.cached){
				// Allocate memory for the node
				child.cachedNode = malloc(sizeof(*child.cachedNode));
				if (NOT CBDatabaseLoadIndexNode(it->index, child.cachedNode, child.diskNodeLoc.indexFile, child.diskNodeLoc.offset)) {
					CBLogError("Could not load an index node from the disk for iteration.");
					return CB_DATABASE_INDEX_ERROR;
				}
			}
			it->indexPos.nodeLoc = child;
			// See if there is another child
			child = it->indexPos.nodeLoc.cachedNode->children[forwards ? 0 : child.cachedNode->numElements];
			if (child.diskNodeLoc.indexFile == 0 && child.diskNodeLoc.offset == 0)
				break;
		}
		it->indexPos.index = forwards ? 0 : (it->indexPos.nodeLoc.cachedNode->numElements-1);
	}
	// Check the iterated element is below or equal to the maximum element if going forwards or above or equal to the minimum element if going backwards
	int cmpres = memcmp(it->indexPos.nodeLoc.cachedNode->elements[it->indexPos.index].key, forwards ? it->maxElement : it->minElement, it->index->keySize);
	bool notFound = forwards ? (cmpres > 0) : (cmpres < 0);
	if (notFound) {
		it->gotIndEl = false;
		return CB_DATABASE_INDEX_NOT_FOUND;
	}
	// If the element is to be deleted, iterate again.
	return CBDatabaseRangeIteratorIterateIfDeletion(it, true);
}
bool CBDatabaseRangeIteratorRead(CBDatabaseRangeIterator * it, uint8_t * data, uint32_t dataLen, uint32_t offset){
	// ??? We already have found the element in the transaction or index, no need to find twice. Should be changed
	return CBDatabaseReadValue(it->index, CBDatabaseRangeIteratorGetKey(it), data, dataLen, offset, false) == CB_DATABASE_INDEX_FOUND;
}
CBIndexFindStatus CBDatabaseReadValue(CBDatabaseIndex * index, uint8_t * key, uint8_t * data, uint32_t dataSize, uint32_t offset, bool staged){
	uint32_t highestWritten = dataSize;
	bool readyForNext = false;
	CBIndexValue idxVal;
	CBDepObject file;
	CBIndexTxData * indexTxData;
	uint8_t * nextKey;
	// Get the index tx data
	indexTxData = CBDatabaseGetIndexTxData(staged ? &index->database->staged : &index->database->current, index, false);
	if (indexTxData) {
		// Loop through write entrys
		uint8_t compareDataMin[index->keySize + 4];
		uint8_t compareDataMax[index->keySize + 4];
		memcpy(compareDataMin, key, index->keySize);
		memcpy(compareDataMax, compareDataMin, index->keySize);
		memset(compareDataMin + index->keySize, 0xFF, 4); // Higher offsets come first
		memset(compareDataMax + index->keySize, 0, 4);
		CBRangeIterator it = {compareDataMin, compareDataMax};
		if (CBAssociativeArrayRangeIteratorStart(&indexTxData->valueWrites, &it)) for (;;){
			uint8_t * writeValue = CBRangeIteratorGetPointer(&it) + index->keySize;
			uint32_t valOffset = *(uint32_t *)writeValue;
			uint32_t valLen = *(uint32_t *)(writeValue + 4);
			writeValue += 8;
			if (valOffset == CB_OVERWRITE_DATA) {
				// Take data from this
				memcpy(data, writeValue + offset, dataSize);
				return CB_DATABASE_INDEX_FOUND;
			}
			// Read from this subsection write if possible
			uint32_t readFromOffset = 0;
			uint32_t readFromLen = 0;
			if (offset > valOffset){
				if (valOffset + valLen > offset){
					readFromOffset = offset - valOffset;
					readFromLen = (valOffset + valLen) - offset;
					if (readFromLen > dataSize)
						readFromLen = dataSize;
				}
			}else if (offset + dataSize > valOffset) {
				readFromLen = (offset + dataSize) - valOffset;
				if (readFromLen > valLen)
					readFromLen = valLen;
			}
			if (readFromLen){
				uint32_t startOffset = readFromOffset - offset + valOffset;
				memcpy(data + startOffset, writeValue + readFromOffset, readFromLen);
				// Read from next data source above read data to last written offset.
				if (startOffset + readFromLen != highestWritten) {
					if (NOT readyForNext) {
						// Look to see if this key is being changed to by searching changeKeysNew so that we use the old key
						indexTxData->changeKeysNew.compareFunc = CBNewKeyWithSingleKeyCompare;
						CBFindResult ckres = CBAssociativeArrayFind(&indexTxData->changeKeysNew, key);
						nextKey = ckres.found ? CBFindResultToPointer(ckres) : key;
						indexTxData->changeKeysNew.compareFunc = CBNewKeyCompare;
						if (staged) {
							// Now look for disk index value
							CBIndexFindResult res = CBDatabaseIndexFind(index, nextKey);
							if (res.status == CB_DATABASE_INDEX_ERROR) {
								CBLogError("Could not find an index value for reading.");
								return CB_DATABASE_INDEX_ERROR;
							}
							idxVal = res.el.nodeLoc.cachedNode->elements[res.el.index];
							CBFreeIndexElementPos(res.el);
							if (NOT CBDatabaseGetFile(index->database, &file, CB_DATABASE_FILE_TYPE_DATA, 0, idxVal.fileID)) {
								CBLogError("Could not open file for reading part of a value.");
								return CB_DATABASE_INDEX_ERROR;
							}
						}
						readyForNext = true;
					}
					uint32_t readLen = highestWritten - (startOffset + readFromLen);
					uint32_t dataOffset = startOffset + readFromLen;
					uint32_t readOffset = offset + startOffset + readFromLen;
					if (staged) {
						if (NOT CBFileSeek(file, idxVal.pos + readOffset)){
							CBLogError("Could not read seek file for reading part of a value.");
							return CB_DATABASE_INDEX_ERROR;
						}
						if (NOT CBFileRead(file, data + dataOffset, readLen)) {
							CBLogError("Could not read from file for value.");
							return CB_DATABASE_INDEX_ERROR;
						}
					}else if (CBDatabaseReadValue(index, nextKey, data + dataOffset, readLen, readOffset, true) != CB_DATABASE_INDEX_FOUND){
						CBLogError("Could not read from staged changes for reading part of a value.");
						return CB_DATABASE_INDEX_ERROR;
					}
				}
				highestWritten = startOffset;
			}
			// Iterate to next value
			if (CBAssociativeArrayRangeIteratorNext(&indexTxData->valueWrites, &it))
				break;
		}
		if (highestWritten == 0)
			return CB_DATABASE_INDEX_FOUND;
	}
	// Read in the rest of the data
	if (NOT readyForNext) {
		// Look to see if this key is being changed to by searching changeKeysNew so that we use the old key
		if (indexTxData) {
			indexTxData->changeKeysNew.compareFunc = CBNewKeyWithSingleKeyCompare;
			CBFindResult ckres = CBAssociativeArrayFind(&indexTxData->changeKeysNew, key);
			nextKey = ckres.found ? CBFindResultToPointer(ckres) : key;
			indexTxData->changeKeysNew.compareFunc = CBNewKeyCompare;
		}else
			nextKey = key;
		if (staged) {
			CBIndexFindResult res = CBDatabaseIndexFind(index, nextKey);
			if (res.status == CB_DATABASE_INDEX_ERROR) {
				CBLogError("Could not find an index value for reading.");
				return CB_DATABASE_INDEX_ERROR;
			}
			if (res.status != CB_DATABASE_INDEX_FOUND){
				CBFreeIndexElementPos(res.el);
				return res.status;
			}
			idxVal = res.el.nodeLoc.cachedNode->elements[res.el.index];
			CBFreeIndexElementPos(res.el);
			// If deleted not found
			if (idxVal.length == CB_DELETED_VALUE)
				return CB_DATABASE_INDEX_NOT_FOUND;
			if (NOT CBDatabaseGetFile(index->database, &file, CB_DATABASE_FILE_TYPE_DATA, 0, idxVal.fileID)) {
				CBLogError("Could not open file for reading part of a value.");
				return CB_DATABASE_INDEX_ERROR;
			}
		}
	}
	if (staged) {
		if (NOT CBFileSeek(file, idxVal.pos + offset)){
			CBLogError("Could not read seek file for reading part of a value.");
			return CB_DATABASE_INDEX_ERROR;
		}
		if (NOT CBFileRead(file, data, highestWritten)) {
			CBLogError("Could not read from file for value.");
			return CB_DATABASE_INDEX_ERROR;
		}
	}else return CBDatabaseReadValue(index, nextKey, data, highestWritten, offset, true);
	return CB_DATABASE_INDEX_FOUND;
}
bool CBDatabaseRemoveValue(CBDatabaseIndex * index, uint8_t * key, bool stage) {
	// Get the index tx data
	CBIndexTxData * indexTxData = CBDatabaseGetIndexTxData(stage ? &index->database->staged : &index->database->current, index, true);
	// Remove all instances of the key in the values writes array
	indexTxData->valueWrites.compareFunc = CBTransactionKeysCompare;
	CBFindResult res;
	for (;;) {
		res = CBAssociativeArrayFind(&indexTxData->valueWrites, key);
		if (NOT res.found)
			break;
		uint32_t * dataLen = (uint32_t *)((uint8_t *)CBFindResultToPointer(res) + index->keySize + 4);
		if (stage)
			index->database->stagedSize -= indexTxData->index->keySize + *dataLen + 8;
		CBAssociativeArrayDelete(&indexTxData->valueWrites, res.position, true);
	}
	indexTxData->valueWrites.compareFunc = CBTransactionKeysAndOffsetCompare;
	// See if there is a change keys entry, in which case we can delete the change keys entry and remove for the previous key.
	indexTxData->changeKeysNew.compareFunc = CBNewKeyWithSingleKeyCompare;
	res = CBAssociativeArrayFind(&indexTxData->changeKeysNew, key);
	indexTxData->changeKeysNew.compareFunc = CBNewKeyCompare;
	// Only continue if the value is also in the index or staged. Else we do not want to try and delete anything since it isn't there.
	if (NOT res.found) {
		// If there was a change key then there must be the old key in the index or staged
		bool foundKey = false;
		if (NOT stage) {
			// Look for the old key being staged
			CBIndexTxData * stagedIndexTxData = CBDatabaseGetIndexTxData(&index->database->staged, index, false);
			if (stagedIndexTxData){
				stagedIndexTxData->valueWrites.compareFunc = CBTransactionKeysCompare;
				if (CBAssociativeArrayFind(&stagedIndexTxData->valueWrites, key).found
					|| CBAssociativeArrayFind(&stagedIndexTxData->changeKeysNew, key).found)
					foundKey = true;
				stagedIndexTxData->valueWrites.compareFunc = CBTransactionKeysAndOffsetCompare;
			}
		}
		if (NOT foundKey) {
			CBIndexFindResult fres = CBDatabaseIndexFind(index, key);
			if (fres.status == CB_DATABASE_INDEX_ERROR) {
				CBLogError("There was an error whilst trying to find a previous key in the index to see if a change key entry should be made.");
				return false;
			}
			CBFreeIndexElementPos(fres.el);
			foundKey = fres.status == CB_DATABASE_INDEX_FOUND;
		}
		if (NOT foundKey){
			// We have not found the key in the index or staged so remove from txKeys and then exit.
			CBIndexItData * indexItData = CBDatabaseGetIndexItData(index->database, index, false);
			if (NOT stage)
				// Removing from current only so remove from currentAddedKeys
				CBAssociativeArrayDelete(&indexItData->currentAddedKeys, CBAssociativeArrayFind(&indexItData->currentAddedKeys, key).position, false);
			CBAssociativeArrayDelete(&indexItData->txKeys, CBAssociativeArrayFind(&indexItData->txKeys, key).position, true);
			return true;
		}
	}else if (NOT stage){
		// Found key as new key, remove from txKeys if exists
		CBIndexItData * indexItData = CBDatabaseGetIndexItData(index->database, index, false);
		if (indexItData) {
			CBFindResult res = CBAssociativeArrayFind(&indexItData->currentAddedKeys, key);
			if (res.found) {
				CBAssociativeArrayDelete(&indexItData->currentAddedKeys, res.position, false);
				CBAssociativeArrayDelete(&indexItData->txKeys, CBAssociativeArrayFind(&indexItData->txKeys, key).position, true);
			}
		}
	}
	// See if key exists to be deleted already
	uint8_t * delkey = res.found ? CBFindResultToPointer(res) : key;
	CBFindResult resd = CBAssociativeArrayFind(&indexTxData->deleteKeys, delkey);
	if (resd.found)
		return true;
	// Insert remove value
	uint8_t * keyPtr = malloc(index->keySize);
	memcpy(keyPtr, delkey, index->keySize);
	CBAssociativeArrayInsert(&indexTxData->deleteKeys, keyPtr, resd.position, NULL);
	if (stage)
		index->database->stagedSize += index->keySize;
	if (res.found) {
		// Remove the key change.
		uint8_t * keys = CBFindResultToPointer(res);
		CBAssociativeArrayDelete(&indexTxData->changeKeysNew, res.position, false);
		CBAssociativeArrayDelete(&indexTxData->changeKeysOld, CBAssociativeArrayFind(&indexTxData->changeKeysOld, keys).position, true);
		if (stage)
			index->database->stagedSize -= index->keySize*2;
	}
	return true;
}
bool CBDatabaseStage(CBDatabase * self){
	// Go through all indicies
	CBPosition pos;
	if (CBAssociativeArrayGetFirst(&self->current.indexes, &pos)) for (;;){
		CBIndexTxData * indexTxData = pos.node->elements[pos.index];
		// Change keys
		CBPosition pos2;
		if (CBAssociativeArrayGetFirst(&indexTxData->changeKeysOld, &pos2)) for (;;) {
			uint8_t * changeKey = pos2.node->elements[pos2.index];
			if (NOT CBDatabaseChangeKey(indexTxData->index, changeKey, changeKey + indexTxData->index->keySize, true)) {
				CBLogError("Could not add a change key change to staged changes.");
				return false;
			}
			if (CBAssociativeArrayIterate(&indexTxData->changeKeysOld, &pos2))
				break;
		}
		// Write values
		if (CBAssociativeArrayGetFirst(&indexTxData->valueWrites, &pos2)) for (;;) {
			uint8_t * valueWrite = pos2.node->elements[pos2.index];
			uint32_t * offsetPtr = (uint32_t *)(valueWrite + indexTxData->index->keySize);
			uint32_t * sizePtr = offsetPtr + 1;
			uint8_t * dataPtr = (uint8_t *)(sizePtr + 1);
			CBDatabaseAddWriteValue(valueWrite, indexTxData->index, valueWrite, *sizePtr, dataPtr, *offsetPtr, true);
			if (CBAssociativeArrayIterate(&indexTxData->valueWrites, &pos2))
				break;
		}
		// Remove the value writes array onFree function because we have used the data for the staging
		indexTxData->valueWrites.onFree = NULL;
		// Deletions
		if (CBAssociativeArrayGetFirst(&indexTxData->deleteKeys, &pos2)) for (;;) {
			uint8_t * deleteKey = pos2.node->elements[pos2.index];
			if (NOT CBDatabaseRemoveValue(indexTxData->index, deleteKey, true)) {
				CBLogError("Could not add a change key change to staged changes.");
				return false;
			}
			if (CBAssociativeArrayIterate(&indexTxData->deleteKeys, &pos2))
				break;
		}
		if (CBAssociativeArrayIterate(&self->current.indexes, &pos))
			break;
	}
	// Reset currentAddedKeys
	if (CBAssociativeArrayGetFirst(&self->itData, &pos)) for (;;) {
		CBIndexItData * indexItData = pos.node->elements[pos.index];
		// Reset currentAddedKeys
		CBFreeAssociativeArray(&indexItData->currentAddedKeys);
		CBInitAssociativeArray(&indexItData->currentAddedKeys, CBTransactionKeysCompare, indexItData->index, NULL);
		if (CBAssociativeArrayIterate(&self->itData, &pos))
			break;
	}
	// Free index tx data
	CBFreeAssociativeArray(&self->current.indexes);
	CBInitAssociativeArray(&self->current.indexes, CBIndexCompare, NULL, CBFreeIndexTxData);
	// Copy over extra data
	memcpy(self->staged.extraData, self->current.extraData, self->extraDataSize);
	// If the staged changes size is more than the cache limit or if the last commit was longer than the commit gap ago, then commit.
	uint64_t now = CBGetMilliseconds();
	if (self->stagedSize > self->cacheLimit
		|| self->lastCommit + self->commitGap < now) {
		if (NOT CBDatabaseCommit(self)) {
			CBLogError("There was a problem commiting to disk.");
			return false;
		}
	}else
		self->hasStaged = true;
	return true;
}
void CBDatabaseWriteConcatenatedValue(CBDatabaseIndex * index, uint8_t * key, uint8_t numDataParts, uint8_t ** data, uint32_t * dataSize){
	// Create element
	uint32_t size = 0;
	for (uint8_t x = 0; x < numDataParts; x++)
		size += dataSize[x];
	uint8_t * valueWrite = malloc(index->keySize + 8 + size);
	uint32_t * intPtr = (uint32_t *)(valueWrite + index->keySize + 4);
	*intPtr = size;
	uint8_t * dataPtr = (uint8_t *)(intPtr + 1);
	// Add data
	for (uint8_t x = 0; x < numDataParts; x++){
		memcpy(dataPtr, data[x], dataSize[x]);
		dataPtr += dataSize[x];
	}
	CBDatabaseAddWriteValue(valueWrite, index, key, size, dataPtr, CB_OVERWRITE_DATA, false);
}
void CBDatabaseWriteValue(CBDatabaseIndex * index, uint8_t * key, uint8_t * data, uint32_t size){
	// Write element by overwriting previous data
	CBDatabaseWriteValueSubSection(index, key, data, size, CB_OVERWRITE_DATA);
}
void CBDatabaseWriteValueSubSection(CBDatabaseIndex * index, uint8_t * key, uint8_t * data, uint32_t size, uint32_t offset){
	// Create element
	uint8_t * valueWrite = malloc(index->keySize + 8 + size);
	uint32_t * intPtr = (uint32_t *)(valueWrite + index->keySize + 4);
	*intPtr = size;
	uint8_t * dataPtr = (uint8_t *)(intPtr + 1);
	memcpy(dataPtr, data, size);
	CBDatabaseAddWriteValue(valueWrite, index, key, size, dataPtr, offset, false);
}
CBCompare CBIndexCompare(CBAssociativeArray * array, void * el1, void * el2){
	CBIndexTxData * indexTxData1 = el1;
	CBIndexTxData * indexTxData2 = el2;
	if (indexTxData1->index > indexTxData2->index)
		return CB_COMPARE_MORE_THAN;
	else if (indexTxData2->index > indexTxData1->index)
		return CB_COMPARE_LESS_THAN;
	return CB_COMPARE_EQUAL;
}
CBCompare CBIndexPtrCompare(CBAssociativeArray * array, void * el1, void * el2){
	if (el1 > el2)
		return CB_COMPARE_MORE_THAN;
	if (el2 > el1)
		return CB_COMPARE_LESS_THAN;
	return CB_COMPARE_EQUAL;
}
CBCompare CBNewKeyCompare(CBAssociativeArray * array, void * el1, void * el2){
	CBDatabaseIndex * index = array->compareObject;
	int res = memcmp(el1 + index->keySize, el2 + index->keySize, index->keySize);
	if (res > 0)
		return CB_COMPARE_MORE_THAN;
	if (res < 0)
		return CB_COMPARE_LESS_THAN;
	return CB_COMPARE_EQUAL;
}
CBCompare CBNewKeyWithSingleKeyCompare(CBAssociativeArray * array, void * el1, void * el2){
	CBDatabaseIndex * index = array->compareObject;
	int res = memcmp(el1, el2 + index->keySize, index->keySize);
	if (res > 0)
		return CB_COMPARE_MORE_THAN;
	if (res < 0)
		return CB_COMPARE_LESS_THAN;
	return CB_COMPARE_EQUAL;
}
CBCompare CBTransactionKeysCompare(CBAssociativeArray * array, void * el1, void * el2){
	CBDatabaseIndex * index = array->compareObject;
	int res = memcmp(el1, el2, index->keySize);
	if (res > 0)
		return CB_COMPARE_MORE_THAN;
	if (res < 0)
		return CB_COMPARE_LESS_THAN;
	return CB_COMPARE_EQUAL;
}
CBCompare CBTransactionKeysAndOffsetCompare(CBAssociativeArray * array, void * el1, void * el2){
	CBDatabaseIndex * index = array->compareObject;
	int res = memcmp(el1, el2, index->keySize);
	if (res > 0)
		return CB_COMPARE_MORE_THAN;
	if (res < 0)
		return CB_COMPARE_LESS_THAN;
	// Now compare offset in reverse, so higher offsets come first.
	uint32_t offset1 = *(uint32_t *)((uint8_t *)el1 + index->keySize);
	uint32_t offset2 = *(uint32_t *)((uint8_t *)el2 + index->keySize);
	if (offset1 > offset2)
		return CB_COMPARE_LESS_THAN;
	if (offset2 > offset1)
		return CB_COMPARE_MORE_THAN;
	return CB_COMPARE_EQUAL;
}
