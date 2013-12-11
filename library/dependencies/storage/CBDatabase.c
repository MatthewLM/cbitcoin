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
#include <assert.h>

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
	if (! CBInitDatabase(self, dataDir, folder, extraDataSize, commitGap, cacheLimit)) {
		free(self);
		CBLogError("Could not initialise a database object.");
		return 0;
	}
	return self;
}
bool CBInitDatabase(CBDatabase * self, char * dataDir, char * folder, uint32_t extraDataSize, uint32_t commitGap, uint32_t cacheLimit){
	self->dataDir = dataDir;
	self->folder = folder;
	self->lastUsedFileID = CB_DATABASE_NO_LAST_FILE;
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
		CBLogError("Could not create the folder (%s) for a database.", filename);
		return false;
	}
	// Check data consistency
	if (! CBDatabaseEnsureConsistent(self)){
		CBLogError("The database is inconsistent and could not be recovered in CBNewDatabase");
		return false;
	}
	// Load deletion index
	CBInitAssociativeArray(&self->deletionIndex, CBKeyCompare, NULL, free);
	strcat(filename, "del.dat");
	// Check that the index exists
	if (! access(filename, F_OK)) {
		// Can read deletion index
		if (! CBDatabaseReadAndOpenDeletionIndex(self, filename)) {
			CBLogError("Could not load the database deletion index.");
			CBFreeAssociativeArray(&self->deletionIndex);
			return false;
		}
	}else{
		// Else empty deletion index
		if (! CBDatabaseCreateDeletionIndex(self, filename)) {
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
		if (! CBFileOpen(&file, filename, true)) {
			CBLogError("Could not open a new first data file.");
			CBFreeAssociativeArray(&self->deletionIndex);
			CBFreeDatabaseTransactionChanges(&self->staged);
			CBFreeDatabaseTransactionChanges(&self->current);
			return false;
		}
		uint8_t initData[6] = {0};
		CBInt32ToArray(initData, 2, self->lastSize);
		if (! CBFileAppend(file, initData, 6)) {
			CBFileClose(file);
			CBLogError("Could not write the initial data for the first data file.");
			CBFreeAssociativeArray(&self->deletionIndex);
			CBFreeDatabaseTransactionChanges(&self->staged);
			CBFreeDatabaseTransactionChanges(&self->current);
			return false;
		}
		if (extraDataSize != 0) {
			if (! CBFileAppendZeros(file, extraDataSize)) {
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
		if (! CBFileOpen(&file, filename, false)) {
			CBLogError("Could not open the data file.");
			CBFreeAssociativeArray(&self->deletionIndex);
			CBFreeDatabaseTransactionChanges(&self->staged);
			CBFreeDatabaseTransactionChanges(&self->current);
			return false;
		}
		if (! CBFileRead(file, data, 6)) {
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
			if (! CBFileRead(file, self->extraDataOnDisk, extraDataSize)) {
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
	// Initialise array for the index information
	CBInitAssociativeArray(&self->valueWritingIndexes, CBIndexPtrCompare, NULL, NULL);
	// Create the mutex
	CBNewMutex(&self->databaseMutex);
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
	index->lastUsedFileID = CB_DATABASE_NO_LAST_FILE;
	// Get the file
	CBDepObject indexFile;
	if (! CBDatabaseGetFile(self, &indexFile, CB_DATABASE_FILE_TYPE_INDEX, index, 0)) {
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
		if (! CBFileRead(indexFile, data, 12)){
			free(index);
			CBFileClose(indexFile);
			CBLogError("Could not read the last file and root node information from a database index.");
			return NULL;
		}
		index->lastFile = index->newLastFile = CBArrayToInt16(data, 0);
		index->lastSize = index->newLastSize = CBArrayToInt32(data, 2);
		// Load memory cache
		index->indexCache.cached = true;
		index->indexCache.diskNodeLoc.indexFile = CBArrayToInt16(data, 6);
		index->indexCache.diskNodeLoc.offset = CBArrayToInt32(data, 8);
		// Allocate memory for the root node.
		index->indexCache.cachedNode = malloc(sizeof(*index->indexCache.cachedNode));
		if (! CBDatabaseLoadIndexNode(index, index->indexCache.cachedNode, index->indexCache.diskNodeLoc.indexFile, index->indexCache.diskNodeLoc.offset)) {
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
			|| node->children[0].diskNodeLoc.indexFile != 0) for (uint32_t x = 0;;) {
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
				if (! CBDatabaseLoadIndexNode(index, node->children[y].cachedNode, indexID, offset)) {
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
				numNodes = node->numElements + 1;
				parentStack[parentStackNum++] = node;
				node = first;
				// See if this node has any children
				if (node->children[0].diskNodeLoc.offset == 0
					&& node->children[0].diskNodeLoc.indexFile == 0)
					break;
			}else
				// Move to the next node in this level
				node = (parentStackNum ? parentStack[parentStackNum - 1] : index->indexCache.cachedNode)->children[++x].cachedNode;
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
		memset(index->indexCache.cachedNode->children, 0, sizeof(index->indexCache.cachedNode->children));
		// Write to disk initial data
		CBInt32ToArray(data, 0, index->lastSize);
		if (! CBFileAppend(indexFile, (uint8_t [12]){
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
		if (! CBFileAppendZeros(indexFile, nodeLen)) {
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
	if (! CBDatabaseGetFile(index->database, &file, CB_DATABASE_FILE_TYPE_INDEX, index, nodeFile)) {
		CBLogError("Could not open an index file when loading a node.");
		return false;
	}
	if (! CBFileSeek(file, nodeOffset)){
		CBLogError("Could seek to the node position in an index file when loading a node.");
		return false;
	}
	// Get the number of elements
	if (! CBFileRead(file, &node->numElements, 1)) {
		CBLogError("Could not read the number of elements for an index node.");
		return false;
	}
	// Read the elements
	uint8_t data[10];
	for (uint8_t x = 0; x < node->numElements; x++) {
		if (! CBFileRead(file, data, 10)) {
			for (uint8_t y = 0; y < x; y++)
				CBReleaseObject(node->elements[y]);
			CBLogError("Could not read the data for an index element.");
			return false;
		}
		node->elements[x] = CBNewIndexValue(index->keySize);
		node->elements[x]->fileID = CBArrayToInt16(data, 0);
		node->elements[x]->length = CBArrayToInt32(data, 2);
		node->elements[x]->pos = CBArrayToInt32(data, 6);
		// Read the key
		if (! CBFileRead(file, node->elements[x]->key, index->keySize)) {
			for (uint8_t y = 0; y <= x; y++)
				CBReleaseObject(node->elements[y]);
			CBLogError("Could not read the key of an index element.");
			return false;
		}
	}
	// Seek past space for elements that do not exist
	if (! CBFileSeek(file, nodeOffset + 1 + (CB_DATABASE_BTREE_ELEMENTS * (index->keySize + 10)))) {
		for (uint8_t y = 0; y <= node->numElements; y++)
			CBReleaseObject(node->elements[y]);
		CBLogError("Could not seek to the children of a btree node.");
		return false;
	}
	// Read the children information
	uint8_t x = 0;
	for (; x <= node->numElements; x++) {
		// Give non-chached information to begin with.
		node->children[x].cached = false;
		if (! CBFileRead(file, data, 6)) {
			for (uint8_t y = 0; y <= node->numElements; y++)
				CBReleaseObject(node->elements[y]);
			CBLogError("Could not read the location of an index child.");
			return false;
		}
		node->children[x].diskNodeLoc.indexFile = CBArrayToInt16(data, 0);
		node->children[x].diskNodeLoc.offset = CBArrayToInt32(data, 2);
	}
	// For remaining children set with 0
	memset(node->children + x, 0, sizeof(node->children) - sizeof(*node->children) * x);
	return true;
}
bool CBDatabaseReadAndOpenDeletionIndex(CBDatabase * self, char * filename){
	if (! CBFileOpen(&self->deletionIndexFile, filename, false)) {
		CBLogError("Could not open the database deletion index.");
		return false;
	}
	// Read deletion entry number
	uint8_t data[11];
	// Get the number of values
	if (! CBFileRead(self->deletionIndexFile, data, 4)) {
		CBFileClose(self->deletionIndexFile);
		CBLogError("Could not read the number of entries from the database deletion index.");
		return false;
	}
	self->numDeletionValues = CBArrayToInt32(data, 0);
	// Now read index
	for (uint32_t x = 0; x < self->numDeletionValues; x++) {
		// Create the key-value index data
		CBDeletedSection * section = malloc(sizeof(*section));
		if (! CBFileRead(self->deletionIndexFile, data, 11)) {
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
	if (! CBFileOpen(&self->deletionIndexFile, filename, true)) {
		CBLogError("Could not open the deletion index file.");
		return true;
	}
	// Write the number of entries as 0
	if (! CBFileAppend(self->deletionIndexFile, (uint8_t []){0, 0, 0, 0}, 4)) {
		CBFileClose(self->deletionIndexFile);
		CBLogError("Could not initially write the number of entries to the database deletion index.");
		return true;
	}
	// Synchronise.
	if (! CBFileSync(self->deletionIndexFile)){
		CBFileClose(self->deletionIndexFile);
		CBLogError("Could not initially synchronise the database deletion index.");
		return true;
	}
	return true;
}
CBIndexValue * CBNewIndexValue(uint8_t keySize){
	CBIndexValue * self = malloc(sizeof(*self) + keySize);
	CBInitObject(CBGetObject(self), false);
	CBGetObject(self)->free = free;
	return self;
}

CBDatabase * CBGetDatabase(void * self){
	return self;
}

bool CBFreeDatabase(CBDatabase * self){
	// Commit staged changes if we didn't last time
	if (self->hasStaged
		&& ! CBDatabaseCommit(self)){
		CBLogError("Failed to commit when freeing a database transaction.");
		return false;
	}
	CBFreeAssociativeArray(&self->deletionIndex);
	CBFreeAssociativeArray(&self->valueWritingIndexes);
	CBFreeMutex(self->databaseMutex);
	// Close files
	CBFileClose(self->deletionIndexFile);
	if (self->lastUsedFileID != CB_DATABASE_NO_LAST_FILE)
		CBFileClose(self->fileObjectCache);
	// Free changes information
	CBFreeDatabaseTransactionChanges(&self->staged);
	CBFreeDatabaseTransactionChanges(&self->current);
	free(self->extraDataOnDisk);
	free(self);
	return true;
}
void CBFreeDatabaseTransactionChanges(CBDatabaseTransactionChanges * self){
	CBFreeAssociativeArray(&self->indexes);
	free(self->extraData);
}
void CBFreeIndexNode(CBIndexNode * node){
	for (uint8_t x = 0; x < node->numElements; x++)
		CBReleaseObject(node->elements[x]);
	for (uint8_t x = 0; x <= node->numElements; x++)
		if (node->children[x].cached)
			CBFreeIndexNode(node->children[x].cachedNode);
	free(node);
}
void CBFreeIndex(CBDatabaseIndex * index){
	CBFreeIndexNode(index->indexCache.cachedNode);
	CBFileClose(index->indexFile);
	free(index);
}
void CBFreeIndexElementPos(CBIndexElementPos pos){
	if (! pos.nodeLoc.cached)
		CBFreeIndexNode(pos.nodeLoc.cachedNode);
	for (uint8_t x = 0; x < pos.parentStack.num; x++)
		if (! pos.parentStack.nodes[x].cached)
			CBFreeIndexNode(pos.parentStack.nodes[x].cachedNode);
}
void CBFreeIndexTxData(void * vindexTxData){
	CBIndexTxData * indexTxData = vindexTxData;
	CBFreeAssociativeArray(&indexTxData->valueWrites);
	CBFreeAssociativeArray(&indexTxData->deleteKeys);
	CBFreeAssociativeArray(&indexTxData->changeKeysOld);
	CBFreeAssociativeArray(&indexTxData->changeKeysNew);
	free(indexTxData);
}
void CBFreeDatabaseRangeIterator(CBDatabaseRangeIterator * it){
	if (it->foundIndex)
		CBFreeIndexElementPos(it->indexPos);
}

bool CBDatabaseAddDeletionEntry(CBDatabase * self, uint16_t fileID, uint32_t pos, uint32_t len){
	// First look for inactive deletion that can be used.
	CBPosition res;
	if (CBAssociativeArrayGetFirst(&self->deletionIndex, &res) && ! ((uint8_t *)res.node->elements[0])[1]) {
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
		if (! CBDatabaseAddOverwrite(self, CB_DATABASE_FILE_TYPE_DELETION_INDEX, 0, 0, data, 4 + section->indexPos * 11, 11)) {
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
		if (! CBDatabaseAppend(self, CB_DATABASE_FILE_TYPE_DELETION_INDEX, 0, 0, data, 11)) {
			CBLogError("There was an error when adding a deletion entry by appending a new one.");
			return false;
		}
	}
	return true;
}
bool CBDatabaseAddOverwrite(CBDatabase * self, CBDatabaseFileType fileType, CBDatabaseIndex * index, uint16_t fileID, uint8_t * data, uint32_t offset, uint32_t dataLen){
	// Execute the overwrite and write rollback information to the log file.
	// Get the file to overwrite
	CBDepObject file;
	if (! CBDatabaseGetFile(self, &file, fileType, index, fileID)) {
		CBLogError("Could not get the data file for overwritting.");
		return false;
	}
	// Write to the log file the rollback information for the changes being made.
	uint32_t dataTempLen = dataLen + 12;
	uint8_t * dataTemp = malloc(dataTempLen);
	// Write file type
	dataTemp[0] = fileType;
	// Write index ID
	dataTemp[1] = (index == NULL) ? 0 : index->ID;
	// Write file ID
	CBInt16ToArray(dataTemp, 2, fileID);
	// Write offset
	CBInt32ToArray(dataTemp, 4, offset);
	// Write data size
	CBInt32ToArray(dataTemp, 8, dataLen);
	// Write old data ??? Change to have old data as input to function
	if (! CBFileSeek(file, offset)){
		CBLogError("Could not seek to the read position for previous data in a file to be overwritten.");
		free(dataTemp);
		return false;
	}
	if (! CBFileRead(file, dataTemp + 12, dataLen)) {
		CBLogError("Could not read the previous data to be overwritten.");
		free(dataTemp);
		return false;
	}
	if (! CBFileAppend(self->logFile, dataTemp, dataTempLen)) {
		CBLogError("Could not write the backup information to the transaction log file.");
		free(dataTemp);
		return false;
	}
	free(dataTemp);
	// Execute overwrite
	if (! CBFileSeek(file, offset)){
		CBLogError("Could not seek the file to overwrite data.");
		return false;
	}
	if (! CBFileOverwrite(file, data, dataLen)) {
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
		if (! CBDatabaseAddOverwrite(self, CB_DATABASE_FILE_TYPE_DATA, 0, sectionFileID, data, sectionOffset + newSectionLen, dataSize)){
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
		if (! CBDatabaseAddOverwrite(self, CB_DATABASE_FILE_TYPE_DELETION_INDEX, 0, 0, section->key + 1, 4 + 11 * section->indexPos, 5)) {
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
		if (! CBDatabaseAppend(self, CB_DATABASE_FILE_TYPE_DATA, 0, self->lastFile, data, dataSize)) {
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
	CBMutexLock(index->database->databaseMutex);
	CBDatabaseAddWriteValueNoMutex(writeValue, index, key, dataLen, dataPtr, offset, stage);
	CBMutexUnlock(index->database->databaseMutex);
}
void CBDatabaseAddWriteValueNoMutex(uint8_t * writeValue, CBDatabaseIndex * index, uint8_t * key, uint32_t dataLen, uint8_t * dataPtr, uint32_t offset, bool stage) {
	CBDatabase * self = index->database;
	CBDatabaseTransactionChanges * changes = stage ? &self->staged : &self->current;
	// Get the index tx data
	CBIndexTxData * indexTxData = CBDatabaseGetIndexTxData(changes, index, true);
	// Add key to write value entry if not staging
	if (! stage)
		memcpy(writeValue, key, index->keySize);
	// Remove from deletion array if needed.
	CBFindResult res = CBAssociativeArrayFind(&indexTxData->deleteKeys, writeValue);
	if (res.found)
		CBAssociativeArrayDelete(&indexTxData->deleteKeys, res.position, true);
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
				oldDataPtr = (uint8_t *)res.position.node->elements[res.position.index] + index->keySize + 4;
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
				if (! res.found)
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
			if (stage){
				self->stagedSize += dataLen + index->keySize + 8;
				// Add index to the array of value write indexes if need be
				res = CBAssociativeArrayFind(&self->valueWritingIndexes, index);
				if (! res.found) {
					// Add into the array
					CBAssociativeArrayInsert(&self->valueWritingIndexes, index, res.position, NULL);
					self->numIndexes++;
				}
			}
		}
	}
}
bool CBDatabaseAppend(CBDatabase * self, CBDatabaseFileType fileType, CBDatabaseIndex * index, uint16_t fileID, uint8_t * data, uint32_t dataLen) {
	// Execute the append operation
	CBDepObject file;
	if (! CBDatabaseGetFile(self, &file, fileType, index, fileID)) {
		CBLogError("Could not get file %u for appending.", fileID);
		return false;
	}
	if (! CBFileAppend(file, data, dataLen)) {
		CBLogError("Failed to append data to file %u.", fileID);
		return false;
	}
	return true;
}
bool CBDatabaseAppendZeros(CBDatabase * self, CBDatabaseFileType fileType, CBDatabaseIndex * index, uint16_t fileID, uint32_t amount) {
	// Execute the append operation
	CBDepObject file;
	if (! CBDatabaseGetFile(self, &file, fileType, index, fileID)) {
		CBLogError("Could not get file %u for appending.", fileID);
		return false;
	}
	if (! CBFileAppendZeros(file, amount)) {
		CBLogError("Failed to append data to file %u.", fileID);
		return false;
	}
	return true;
}
bool CBDatabaseChangeKey(CBDatabaseIndex * index, uint8_t * previousKey, uint8_t * newKey, bool stage) {
	CBMutexLock(index->database->databaseMutex);
	bool ret = CBDatabaseChangeKeyNoMutex(index, previousKey, newKey, stage);
	CBMutexUnlock(index->database->databaseMutex);
	return ret;
}
bool CBDatabaseChangeKeyNoMutex(CBDatabaseIndex * index, uint8_t * previousKey, uint8_t * newKey, bool stage) {
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
		memcpy((uint8_t *)CBFindResultToPointer(res) + index->keySize, newKey, index->keySize);
	else{
		// Insert new entry if the old key exists in the index or staged
		bool foundPrev = false;
		if (! stage) {
			// Look for the old key being staged
			CBIndexTxData * stagedIndexTxData = CBDatabaseGetIndexTxData(&self->staged, index, false);
			if (stagedIndexTxData){
				stagedIndexTxData->valueWrites.compareFunc = CBTransactionKeysCompare;
				if (CBAssociativeArrayFind(&stagedIndexTxData->valueWrites, previousKey).found
					|| CBAssociativeArrayFind(&stagedIndexTxData->changeKeysNew, previousKey).found)
					foundPrev = true;
				stagedIndexTxData->valueWrites.compareFunc = CBTransactionKeysAndOffsetCompare;
			}
		}
		if (! foundPrev) {
			CBIndexFindWithParentsResult fres = CBDatabaseIndexFindWithParents(index, previousKey);
			CBFreeIndexElementPos(fres.el);
			if (fres.status == CB_DATABASE_INDEX_ERROR) {
				CBLogError("There was an error whilst trying to find a previous key in the index to see if a change key entry should be made.");
				return false;
			}
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
				if (! res.found) {
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
		if (! res.found)
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
	return true;
}
void CBDatabaseClearCurrent(CBDatabase * self) {
	CBMutexLock(self->databaseMutex);
	// Free index tx data
	CBFreeAssociativeArray(&self->current.indexes);
	CBInitAssociativeArray(&self->current.indexes, CBIndexCompare, NULL, CBFreeIndexTxData);
	// Change extra data to match staged.
	memcpy(self->current.extraData, self->staged.extraData, self->extraDataSize);
	CBMutexUnlock(self->databaseMutex);
}
bool CBDatabaseCommit(CBDatabase * self){
	CBMutexLock(self->databaseMutex);
	bool res = CBDatabaseCommitNoMutex(self);
	CBMutexUnlock(self->databaseMutex);
	return res;
}
bool CBDatabaseCommitNoMutex(CBDatabase * self){
	char filename[strlen(self->dataDir) + strlen(self->folder) + 10];
	sprintf(filename, "%s/%s/log.dat", self->dataDir, self->folder);
	// Open the log file
	if (! CBFileOpen(&self->logFile, filename, true)) {
		CBLogError("The log file for overwritting could not be opened.");
		return false;
	}
	// Write the previous sizes for the files to the log file
	uint8_t data[12];
	data[0] = 1; // Log file active.
	uint32_t deletionIndexLen;
	if (! CBFileGetLength(self->deletionIndexFile, &deletionIndexLen)) {
		CBLogError("Could not get the length of the deletion index file.");
		CBFileClose(self->logFile);
		return false;
	}
	CBInt32ToArray(data, 1, deletionIndexLen);
	CBInt16ToArray(data, 5, self->lastFile);
	CBInt32ToArray(data, 7, self->lastSize);
	data[11] = self->numIndexes;
	if (! CBFileAppend(self->logFile, data, 12)) {
		CBLogError("Could not write previous size information to the log-file for the data and deletion index.");
		CBFileClose(self->logFile);
		return false;
	}
	// Loop through index files to write the previous lengths
	CBAssociativeArrayForEach(CBDatabaseIndex * ind, &self->valueWritingIndexes){
		data[0] = ind->ID;
		CBInt16ToArray(data, 1, ind->lastFile);
		CBInt32ToArray(data, 3, ind->lastSize);
		if (! CBFileAppend(self->logFile, data, 7)) {
			CBLogError("Could not write the previous size information for an index.");
			CBFileClose(self->logFile);
			return false;
		}
	}
	uint32_t lastFileSize = self->lastSize;
	uint32_t prevDeletionEntryNum = self->numDeletionValues;
	// Loop through indexes
	CBAssociativeArrayForEach(CBIndexTxData * indexTxData, &self->staged.indexes){
		// Change keys
		CBAssociativeArrayForEach(uint8_t * oldKeyPtr, &indexTxData->changeKeysOld){
			uint8_t * newKeyPtr = oldKeyPtr + indexTxData->index->keySize;
			// Find index entry
			CBIndexFindWithParentsResult res = CBDatabaseIndexFindWithParents(indexTxData->index, oldKeyPtr);
			if (res.status == CB_DATABASE_INDEX_ERROR) {
				CBLogError("There was an error while attempting to find an index key for changing.");
				CBFileClose(self->logFile);
				return false;
			}
			CBIndexValue * indexValue = res.el.nodeLoc.cachedNode->elements[res.el.index];
			// Allocate and assign new index entry
			CBIndexValue * newIndexValue = CBNewIndexValue(indexTxData->index->keySize);
			newIndexValue->fileID = indexValue->fileID;
			newIndexValue->length = indexValue->length;
			newIndexValue->pos = indexValue->pos;
			// Delete old key
			indexValue->length = CB_DELETED_VALUE;
			CBInt32ToArray(data, 0, CB_DELETED_VALUE);
			if (! CBDatabaseAddOverwrite(self, CB_DATABASE_FILE_TYPE_INDEX, indexTxData->index, res.el.nodeLoc.diskNodeLoc.indexFile, data, res.el.nodeLoc.diskNodeLoc.offset + 3 + (indexTxData->index->keySize + 10)*res.el.index, 4)) {
				CBLogError("Failed to overwrite the index entry's length with CB_DELETED_VALUE to signify deletion when changing a key.");
				CBFileClose(self->logFile);
				CBFreeIndexElementPos(res.el);
				CBReleaseObject(newIndexValue);
				return false;
			}
			// Add new key to index
			memcpy(newIndexValue->key, newKeyPtr, indexTxData->index->keySize);
			// Find position for index entry
			CBFreeIndexElementPos(res.el);
			res = CBDatabaseIndexFindWithParents(indexTxData->index, newKeyPtr);
			if (res.status == CB_DATABASE_INDEX_ERROR) {
				CBLogError("Failed to find the position for a new key replacing an old one.");
				CBFileClose(self->logFile);
				CBReleaseObject(newIndexValue);
				return false;
			}
			if (res.status == CB_DATABASE_INDEX_FOUND) {
				// The index entry already exists. Get the index value
				CBIndexValue * indexValue = res.el.nodeLoc.cachedNode->elements[res.el.index];
				if (indexValue->length != CB_DELETED_VALUE) {
					// Delete existing data this new key was pointing to
					if (! CBDatabaseAddDeletionEntry(self, indexValue->fileID, indexValue->pos, indexValue->length)) {
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
				if (! CBDatabaseAddOverwrite(self, CB_DATABASE_FILE_TYPE_INDEX, indexTxData->index, res.el.nodeLoc.diskNodeLoc.indexFile, data, res.el.nodeLoc.diskNodeLoc.offset + 1 + (indexTxData->index->keySize + 10)*res.el.index, 10)) {
					CBFileClose(self->logFile);
					CBFreeIndexElementPos(res.el);
					CBReleaseObject(newIndexValue);
					return false;
				}
				// Else insert index element into the index
			}else if (! CBDatabaseIndexInsert(indexTxData->index, newIndexValue, &res.el, NULL)) {
				CBLogError("Failed to insert a new index element into the index with a new key for existing data.");
				CBFileClose(self->logFile);
				CBFreeIndexElementPos(res.el);
				CBReleaseObject(newIndexValue);
				return false;
			}
			CBFreeIndexElementPos(res.el);
			CBReleaseObject(newIndexValue);
		}
		// Go through each value write key
		// Get the first key with an interator
		CBAssociativeArrayForEach(uint8_t * keyPtr, &indexTxData->valueWrites) {
			// Get the data for writing
			uint32_t offset = *(uint32_t *)(keyPtr + indexTxData->index->keySize);
			uint32_t dataSize = *(uint32_t *)(keyPtr + indexTxData->index->keySize + 4);
			uint8_t * dataPtr = keyPtr + indexTxData->index->keySize + 8;
			// Check for key in index
			CBIndexFindWithParentsResult res = CBDatabaseIndexFindWithParents(indexTxData->index, keyPtr);
			if (res.status == CB_DATABASE_INDEX_ERROR) {
				CBLogError("There was an error while searching an index for a key.");
				CBFileClose(self->logFile);
				return false;
			}
			if (res.status == CB_DATABASE_INDEX_FOUND) {
				// Exists in index so we are overwriting data.
				// Get the index data
				CBIndexValue * indexValue = res.el.nodeLoc.cachedNode->elements[res.el.index];
				// See if the data can be overwritten.
				if (indexValue->length >= dataSize && indexValue->length != CB_DELETED_VALUE){
					// We are going to overwrite the previous data.
					if (! CBDatabaseAddOverwrite(self,
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
						if (! CBDatabaseAddDeletionEntry(self, indexValue->fileID, indexValue->pos + dataSize, indexValue->length - dataSize)){
							CBLogError("Failed to add a deletion entry when overwriting a previous value with a smaller one.");
							CBFileClose(self->logFile);
							CBFreeIndexElementPos(res.el);
							return false;
						}
						// Change the length of the data in the index.
						indexValue->length = dataSize;
						uint8_t newLength[4];
						CBInt32ToArray(newLength, 0, indexValue->length);
						if (! CBDatabaseAddOverwrite(self, CB_DATABASE_FILE_TYPE_INDEX, indexTxData->index, res.el.nodeLoc.diskNodeLoc.indexFile, newLength, res.el.nodeLoc.diskNodeLoc.offset + 3 + (10+indexTxData->index->keySize)*res.el.index, 4)) {
							CBLogError("Failed to add an overwrite operation to write the new length of a value to the database index.");
							CBFileClose(self->logFile);
							CBFreeIndexElementPos(res.el);
							return false;
						}
					}
				}else{
					// We will mark the previous data as deleted and store data elsewhere, unless the data has been marked as deleted already (by CB_DELETED_VALUE length).
					if (indexValue->length != CB_DELETED_VALUE
						&& ! CBDatabaseAddDeletionEntry(self, indexValue->fileID, indexValue->pos, indexValue->length)){
						CBLogError("Failed to add a deletion entry for an old value when replacing it with a larger one.");
						CBFileClose(self->logFile);
						CBFreeIndexElementPos(res.el);
						return false;
					}
					if (! CBDatabaseAddValue(self, dataSize, dataPtr, indexValue)) {
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
					if (! CBDatabaseAddOverwrite(self, CB_DATABASE_FILE_TYPE_INDEX, indexTxData->index, res.el.nodeLoc.diskNodeLoc.indexFile, data, res.el.nodeLoc.diskNodeLoc.offset + 1 + (10+indexTxData->index->keySize)*res.el.index, 10)) {
						CBLogError("Failed to add an overwrite operation for updating the index for writting data in a new location.");
						CBFileClose(self->logFile);
						CBFreeIndexElementPos(res.el);
						return false;
					}
				}
			}else{
				// Does not exist in index, so we are creating new data
				// New index value data
				CBIndexValue * indexValue = CBNewIndexValue(indexTxData->index->keySize);
				if (! CBDatabaseAddValue(self, dataSize, dataPtr, indexValue)) {
					CBLogError("Failed to add a value to the database with a new key.");
					CBFileClose(self->logFile);
					CBFreeIndexElementPos(res.el);
					CBReleaseObject(indexValue);
					return false;
				}
				// Add key to index
				memcpy(indexValue->key, keyPtr, indexTxData->index->keySize);
				// Insert index element into the index
				if (! CBDatabaseIndexInsert(indexTxData->index, indexValue, &res.el, NULL)) {
					CBLogError("Failed to insert a new index element into the index.");
					CBFileClose(self->logFile);
					CBFreeIndexElementPos(res.el);
					CBReleaseObject(indexValue);
					return false;
				}
				CBReleaseObject(indexValue);
			}
			CBFreeIndexElementPos(res.el);
		}
		// Delete values
		CBAssociativeArrayForEach(uint8_t * keyPtr, &indexTxData->deleteKeys) {
			// Find index entry
			CBIndexFindWithParentsResult res = CBDatabaseIndexFindWithParents(indexTxData->index, keyPtr);
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
			if (! CBDatabaseIndexDelete(indexTxData->index, &res)){
				CBLogError("Failed to delete a key-value.");
				CBFileClose(self->logFile);
				CBFreeIndexElementPos(res.el);
				return false;
			}
			CBFreeIndexElementPos(res.el);
		}
		// Update the last index file information
		if (indexTxData->index->lastFile != indexTxData->index->newLastFile
			|| indexTxData->index->lastSize != indexTxData->index->newLastSize) {
			indexTxData->index->lastFile = indexTxData->index->newLastFile;
			indexTxData->index->lastSize = indexTxData->index->newLastSize;
			// Write to the index file
			CBInt16ToArray(data, 0, indexTxData->index->lastFile);
			CBInt32ToArray(data, 2, indexTxData->index->lastSize);
			if (! CBDatabaseAddOverwrite(self, CB_DATABASE_FILE_TYPE_INDEX, indexTxData->index, 0, data, 0, 6)) {
				CBLogError("Failed to write the new last file information to an index.");
				CBFileClose(self->logFile);
				return false;
			}
		}
		// Sync index
		if (! CBFileSync(indexTxData->index->indexFile)) {
			CBLogError("Failed to synchronise index file for index with ID %u", indexTxData->index->ID);
			CBFileClose(self->logFile);
			return false;
		}
	}
	// Update the data last file information
	if (lastFileSize != self->lastSize) {
		// Change index information.
		CBInt16ToArray(data, 0, self->lastFile);
		CBInt32ToArray(data, 2, self->lastSize);
		if (! CBDatabaseAddOverwrite(self, CB_DATABASE_FILE_TYPE_DATA, NULL, 0, data, 0, 6)) {
			CBLogError("Failed to update the information for the last data index file.");
			CBFileClose(self->logFile);
			return false;
		}
	}
	if (prevDeletionEntryNum != self->numDeletionValues) {
		// New number of deletion entries.
		CBInt32ToArray(data, 0, self->numDeletionValues);
		if (! CBDatabaseAddOverwrite(self, CB_DATABASE_FILE_TYPE_DELETION_INDEX, 0, 0, data, 0, 4)) {
			CBLogError("Failed to update the number of entries for the deletion index file.");
			CBFileClose(self->logFile);
			return false;
		}
	}
	// Update extra data
	uint32_t start = 0xFFFFFFFF;
	for (uint32_t x = 0; x < self->extraDataSize; x++) {
		bool same = self->extraDataOnDisk[x] == self->staged.extraData[x];
		if (! same && start == 0xFFFFFFFF)
			start = x;
		if (start != 0xFFFFFFFF
			&& (same || x == self->extraDataSize - 1)){
			// Got the section that should be written.
			uint32_t len = x - start + (x == self->extraDataSize - 1);
			memcpy(self->extraDataOnDisk + start, self->staged.extraData + start, len);
			// Now write to disk
			if (! CBDatabaseAddOverwrite(self, CB_DATABASE_FILE_TYPE_DATA, 0, 0, self->staged.extraData + start, 6 + start, len)) {
				CBLogError("Failed to write to the database's extra data.");
				CBFileClose(self->logFile);
				return false;
			}
			start = 0xFFFFFFFF;
		}
	}
	// Sync last used file and deletion index file.
	if ((! CBFileSync(self->fileObjectCache))
		|| ! CBFileSync(self->deletionIndexFile)) {
		CBLogError("Failed to synchronise the files during a commit.");
		CBFileClose(self->logFile);
		return false;
	}
	// Sync log file, so that it is now active with the information of the previous file sizes.
	if (! CBFileSync(self->logFile)){
		CBLogError("Failed to sync the log file");
		CBFileClose(self->logFile);
		return false;
	}
	// Sync directory
	if (! CBFileSyncDir(self->dataDir)) {
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
	if (! CBFileOpen(&logFile,filename, false)) {
		CBLogError("The log file for reading could not be opened.");
		return false;
	}
	// Check if the logfile is active or not.
	uint8_t data[12];
	if (! CBFileRead(logFile, data, 1)) {
		CBLogError("Failed reading the logfile.");
		CBFileClose(logFile);
		return false;
	}
	if (! data[0]){
		// Not active
		CBFileClose(logFile);
		return true;
	}
	// Active, therefore we need to attempt rollback.
	if (! CBFileRead(logFile, data, 11)) {
		CBLogError("Failed reading the logfile for the data and deletion index previous file information.");
		CBFileClose(logFile);
		return false;
	}
	// Truncate files
	char * filenameEnd = filename + strlen(self->dataDir) + strlen(self->folder) + 2;
	strcpy(filenameEnd, "del.dat");
	if (! CBFileTruncate(filename, CBArrayToInt32(data, 0))) {
		CBLogError("Failed to truncate the deletion index file down to the previous size.");
		CBFileClose(logFile);
		return false;
	}
	uint16_t lastFile = CBArrayToInt16(data, 4);
	sprintf(filenameEnd, "val_%u.dat", lastFile);
	if (! CBFileTruncate(filename, CBArrayToInt32(data, 6))) {
		CBLogError("Failed to truncate the last data file down to the previous size.");
		CBFileClose(logFile);
		return false;
	}
	// Check if a new file had been created. If it has, delete it.
	sprintf(filenameEnd, "val_%u.dat", lastFile + 1);
	if (! access(filename, F_OK))
		remove(filename);
	// Loop through indexes and truncate/delete files as necessary
	for (uint8_t x = 0; x < data[10]; x++) {
		if (! CBFileRead(logFile, data, 7)) {
			CBLogError("Failed reading the logfile for an index's previous file information.");
			CBFileClose(logFile);
			return false;
		}
		lastFile = CBArrayToInt16(data, 1);
		sprintf(filenameEnd, "idx_%u_%u.dat", data[0], lastFile);
		if (! CBFileTruncate(filename, CBArrayToInt32(data, 3))) {
			CBLogError("Failed to truncate the last file down to the previous size for an index.");
			CBFileClose(logFile);
			return false;
		}
		sprintf(filenameEnd, "idx_%u_%u.dat", data[0], lastFile + 1);
		if (! access(filename, F_OK))
			remove(filename);
	}
	// Reverse overwrite operations
	uint32_t logFileLen;
	if (! CBFileGetLength(logFile, &logFileLen)) {
		CBLogError("Failed to get the length of the log file.");
		CBFileClose(logFile);
		return false;
	}
	uint8_t * prevData = NULL;
	uint32_t prevDataSize = 0;
	for (uint32_t c = 14 + 7 * data[10]; c < logFileLen;) {
		// Read file type, index ID and file ID, offset and size
		if (! CBFileRead(logFile, data, 12)) {
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
		if (! CBFileRead(logFile, prevData, dataLen)) {
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
		if (! CBFileOpen(&file, filename, false)) {
			CBLogError("Could not open the file for writting the previous data.");
			CBFileClose(logFile);
			return false;
		}
		if (! CBFileSeek(file, CBArrayToInt32(data, 4))){
			CBLogError("Could not seek the file for writting the previous data.");
			CBFileClose(logFile);
			CBFileClose(file);
			return false;
		}
		if (! CBFileOverwrite(file, prevData, dataLen)) {
			CBLogError("Could not write previous data back into the file.");
			CBFileClose(logFile);
			CBFileClose(file);
			return false;
		}
		if (! CBFileSync(file)) {
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
	if (! CBFileSyncDir(self->dataDir)) {
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
bool CBDatabaseGetFile(CBDatabase * self, CBDepObject * file, CBDatabaseFileType type, CBDatabaseIndex * index, uint16_t fileID){
	assert(fileID == 0);
	if (type == CB_DATABASE_FILE_TYPE_DATA){
		if (fileID == self->lastUsedFileID){
			*file = self->fileObjectCache;
			return true;
		}else{
			// Change last used file cache
			if (self->lastUsedFileID != CB_DATABASE_NO_LAST_FILE) {
				// First synchronise previous file
				if (! CBFileSync(self->fileObjectCache)) {
					CBLogError("Could not synchronise a file when a new file is needed.");
					return false;
				}
				// Now close the file
				CBFileClose(self->fileObjectCache);
			}
			// Open the new file
			char filename[strlen(self->dataDir) + strlen(self->folder) + 14];
			sprintf(filename, "%s/%s/val_%u.dat", self->dataDir, self->folder, fileID);
			if (! CBFileOpen(&self->fileObjectCache, filename, access(filename, F_OK))) {
				CBLogError("Could not open file %s.", filename);
				return false;
			}
			self->lastUsedFileID = fileID;
			*file = self->fileObjectCache;
		}
	}else if (type == CB_DATABASE_FILE_TYPE_DELETION_INDEX)
		*file = self->deletionIndexFile;
	else if (fileID == index->lastUsedFileID)
		*file = index->indexFile;
	else{
		if (index->lastUsedFileID != CB_DATABASE_NO_LAST_FILE) {
			// First synchronise previous file
			if (! CBFileSync(index->indexFile)) {
				CBLogError("Could not synchronise a file when a new file is needed.");
				return false;
			}
			// Now close the file
			CBFileClose(index->indexFile);
		}
		// Open the new file
		char filename[strlen(self->dataDir) + strlen(self->folder) + 18];
		sprintf(filename, "%s/%s/idx_%u_%u.dat", self->dataDir, self->folder, index->ID, fileID);
		if (! CBFileOpen(&index->indexFile, filename, access(filename, F_OK))) {
			CBLogError("Could not open file %s.", filename);
			return false;
		}
		index->lastUsedFileID = fileID;
		*file = index->indexFile;
	}
	return true;
}
bool CBDatabaseGetLength(CBDatabaseIndex * index, uint8_t * key, uint32_t * length) {
	CBMutexLock(index->database->databaseMutex);
	bool ret = CBDatabaseGetLengthNoMutex(index, key, length);
	CBMutexUnlock(index->database->databaseMutex);
	return ret;
}
bool CBDatabaseGetLengthNoMutex(CBDatabaseIndex * index, uint8_t * key, uint32_t * length) {
	uint8_t * diskKey = key;
	CBDatabase * self = index->database;
	// Get the index tx data
	for (CBDatabaseTransactionChanges * c = &self->current;; c = &self->staged) {
		CBIndexTxData * indexTxData = CBDatabaseGetIndexTxData(c, index, false);
		if (indexTxData) {
			// Check for a deletion value
			CBFindResult res = CBAssociativeArrayFind(&indexTxData->deleteKeys, diskKey);
			if (res.found){
				*length = CB_DOESNT_EXIST;
				return true;
			}
			// First check for CB_OVERWRITE_DATA value write entry as it replaces the length on disk.
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
	CBIndexFindWithParentsResult indexRes = CBDatabaseIndexFindWithParents(index, diskKey);
	if (indexRes.status == CB_DATABASE_INDEX_ERROR) {
		CBLogError("There was an error when attempting to find a key in an index to get the data length.");
		return false;
	}
	if (indexRes.status == CB_DATABASE_INDEX_NOT_FOUND){
		CBFreeIndexElementPos(indexRes.el);
		*length = CB_DOESNT_EXIST;
		return true;
	}
	*length = indexRes.el.nodeLoc.cachedNode->elements[indexRes.el.index]->length;
	CBFreeIndexElementPos(indexRes.el);
	return true;
}
CBIndexTxData * CBDatabaseGetIndexTxData(CBDatabaseTransactionChanges * tx, CBDatabaseIndex * index, bool create){
	CBFindResult res = CBAssociativeArrayFind(&tx->indexes, &(CBIndexTxData){index});
	CBIndexTxData * indexTxData;
	if (! res.found) {
		// Can't find the index tx data so create it unless create is false.
		if (! create)
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
bool CBDatabaseIndexDelete(CBDatabaseIndex * index, CBIndexFindWithParentsResult * res){
	// Create deletion entry for the data
	CBIndexValue * indexValue = res->el.nodeLoc.cachedNode->elements[res->el.index];
	if (! CBDatabaseAddDeletionEntry(index->database, indexValue->fileID, indexValue->pos, indexValue->length)) {
		CBLogError("Failed to create a deletion entry for a key-value.");
		CBMutexUnlock(index->database->databaseMutex);
		return false;
	}
	// Make the length CB_DELETED_VALUE, signifying deletion of the data
	indexValue->length = CB_DELETED_VALUE;
	uint8_t data[4];
	CBInt32ToArray(data, 0, CB_DELETED_VALUE);
	if (! CBDatabaseAddOverwrite(index->database, CB_DATABASE_FILE_TYPE_INDEX, index, res->el.nodeLoc.diskNodeLoc.indexFile, data, res->el.nodeLoc.diskNodeLoc.offset + 3 + (index->keySize + 10)*res->el.index, 4)) {
		CBLogError("Failed to overwrite the index entry's length with CB_DELETED_VALUE to signify deletion.");
		CBMutexUnlock(index->database->databaseMutex);
		return false;
	}
	return true;
}
CBIndexFindResult CBDatabaseIndexFind(CBDatabaseIndex * index, uint8_t * key){
	CBIndexFindResult result = {.data.node = index->indexCache};
	for (;;) {
		// Do binary search on node
		if (result.data.node.cached) {
			// Do in-memory
			uint8_t pos;
			CBDatabaseIndexNodeBinarySearchMemory(index, result.data.node.cachedNode, key, &result.status, &pos);
			if (result.status == CB_DATABASE_INDEX_FOUND){
				result.data.el = result.data.node.cachedNode->elements[pos];
				CBRetainObject(result.data.el);
			}else
				result.data.node = result.data.node.cachedNode->children[pos];
		}else
			// Do from disk without loading all of the node unnecessarily.
			CBDatabaseIndexNodeSearchDisk(index, key, &result);
		if (result.status != CB_DATABASE_INDEX_NOT_FOUND)
			// Error or found
			return result;
		// Not found
		if (!result.data.node.cached
			&& result.data.node.diskNodeLoc.indexFile == 0
			&& result.data.node.diskNodeLoc.offset == 0)
			// The child doesn't exist. Return as not found.
			return result;
	}
}
CBIndexFindWithParentsResult CBDatabaseIndexFindWithParents(CBDatabaseIndex * index, uint8_t * key){
	CBIndexFindWithParentsResult result;
	CBIndexNodeLocation nodeLoc = index->indexCache;
	result.el.parentStack.num = 0;
	result.el.parentStack.cursor = 0;
	for (;;) {
		// Do binary search on node
		CBDatabaseIndexNodeBinarySearchMemory(index, nodeLoc.cachedNode, key, &result.status, &result.el.index);
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
				if (! nodeLoc.cached){
					// Allocate memory for the node
					nodeLoc.cachedNode = malloc(sizeof(*nodeLoc.cachedNode));
					if (! CBDatabaseLoadIndexNode(index, nodeLoc.cachedNode, nodeLoc.diskNodeLoc.indexFile, nodeLoc.diskNodeLoc.offset)) {
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
			if (! CBDatabaseIndexMoveElements(index, &pos->nodeLoc, &pos->nodeLoc, pos->index, pos->index + 1, pos->nodeLoc.cachedNode->numElements - pos->index, false)
				|| (! bottom && ! CBDatabaseIndexMoveChildren(index, &pos->nodeLoc, &pos->nodeLoc, pos->index + 1, pos->index + 2, pos->nodeLoc.cachedNode->numElements - pos->index, false))) {
				CBLogError("Could not move elements and children up one in a node for inserting a new element.");
				return false;
			}
		}
		// Insert element and right child
		if (! CBDatabaseIndexSetElement(index, &pos->nodeLoc, indexVal, pos->index, false)
			|| (right && ! CBDatabaseIndexSetChild(index, &pos->nodeLoc, right, pos->index + 1, false))){
			CBLogError("Could not set an element and child for adding to an unfull index node.");
			return false;
		}
		// Increase number of elements
		if (! CBDatabaseIndexSetNumElements(index, &pos->nodeLoc, pos->nodeLoc.cachedNode->numElements + 1, false)){
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
			if (bottom)
				// Set children to 0
				memset(new.cachedNode->children, 0, sizeof(new.cachedNode->children));
		}else
			new.cached = false;
		// Get position for new node
		CBDatabaseIndexSetNewNodePosition(index, &new);
		// Make both sides have half the order.
		if (! CBDatabaseIndexSetNumElements(index, &pos->nodeLoc, CB_DATABASE_BTREE_HALF_ELEMENTS, false)
			|| ! CBDatabaseIndexSetNumElements(index, &new, CB_DATABASE_BTREE_HALF_ELEMENTS, true)) {
			CBLogError("Could not set two split nodes ot have the number of elements equal to half the order.");
			return false;
		}
		CBIndexValue * middleEl;
		// Check where to insert new element
		if (pos->index > CB_DATABASE_BTREE_HALF_ELEMENTS) {
			// In the new node.
			if (
				// Copy over first part of the new child
				! CBDatabaseIndexMoveElements(index, &new, &pos->nodeLoc, CB_DATABASE_BTREE_HALF_ELEMENTS + 1, 0, pos->index - CB_DATABASE_BTREE_HALF_ELEMENTS - 1, true)
				// Copy over the element
				|| ! CBDatabaseIndexSetElement(index, &new, indexVal, pos->index - CB_DATABASE_BTREE_HALF_ELEMENTS - 1, true)
				// Copy remainder of elements
				|| ! CBDatabaseIndexMoveElements(index, &new, &pos->nodeLoc, pos->index, pos->index - CB_DATABASE_BTREE_HALF_ELEMENTS, CB_DATABASE_BTREE_ELEMENTS - pos->index, true)
				// Set remainder of elements as blank
				|| ! CBDatabaseAppendZeros(index->database, CB_DATABASE_FILE_TYPE_INDEX, index, new.diskNodeLoc.indexFile, CB_DATABASE_BTREE_HALF_ELEMENTS * (index->keySize + 10))) {
				CBLogError("Could not move elements including the new element into the new node");
				return false;
			}
			if (! bottom
				&& (! CBDatabaseIndexMoveChildren(index, &new, &pos->nodeLoc, CB_DATABASE_BTREE_HALF_ELEMENTS + 1, 0, pos->index - CB_DATABASE_BTREE_HALF_ELEMENTS, true)
					// Set the right child
					|| ! CBDatabaseIndexSetChild(index, &new, right, pos->index - CB_DATABASE_BTREE_HALF_ELEMENTS, true)
					// Set the remaining children
					|| ! CBDatabaseIndexMoveChildren(index, &new, &pos->nodeLoc, pos->index + 1, pos->index - CB_DATABASE_BTREE_HALF_ELEMENTS + 1, CB_DATABASE_BTREE_ELEMENTS - pos->index, true))){
					CBLogError("Could not insert children into new node.");
					return false;
				}
			// Set middle element
			middleEl = pos->nodeLoc.cachedNode->elements[CB_DATABASE_BTREE_HALF_ELEMENTS];
		}else if (pos->index == CB_DATABASE_BTREE_HALF_ELEMENTS) {
			// Middle
			if (
				// Copy elements
				! CBDatabaseIndexMoveElements(index, &new, &pos->nodeLoc, CB_DATABASE_BTREE_HALF_ELEMENTS, 0, CB_DATABASE_BTREE_HALF_ELEMENTS, true)
				// Set remaining elements as blank
				|| ! CBDatabaseAppendZeros(index->database, CB_DATABASE_FILE_TYPE_INDEX, index, new.diskNodeLoc.indexFile, CB_DATABASE_BTREE_HALF_ELEMENTS * (index->keySize + 10))){
				CBLogError("Could not move elements into the new node, with the new element being the middle element.");
				return false;
			}
			if (! bottom
				&& (// Set right child
					! CBDatabaseIndexSetChild(index, &new, right, 0, true)
					// Copy children
					|| ! CBDatabaseIndexMoveChildren(index, &new, &pos->nodeLoc, CB_DATABASE_BTREE_HALF_ELEMENTS + 1, 1, CB_DATABASE_BTREE_HALF_ELEMENTS, true))) {
					CBLogError("Could not move children into the new node, with the new element being the middle element.");
					return false;
				}
			// Middle value is inserted value
			middleEl = indexVal;
		}else{
			// In the first node.
			// Set middle element. Do this before moving anything as the elements array will be modified.
			middleEl = pos->nodeLoc.cachedNode->elements[CB_DATABASE_BTREE_HALF_ELEMENTS-1];
			if (// Copy elements into new node
				! CBDatabaseIndexMoveElements(index, &new, &pos->nodeLoc, CB_DATABASE_BTREE_HALF_ELEMENTS, 0, CB_DATABASE_BTREE_HALF_ELEMENTS, true)
				// Set remaining elements as blank
				|| ! CBDatabaseAppendZeros(index->database, CB_DATABASE_FILE_TYPE_INDEX, index, new.diskNodeLoc.indexFile, CB_DATABASE_BTREE_HALF_ELEMENTS * (index->keySize + 10))
				// Move elements to make room for new element
				|| ! CBDatabaseIndexMoveElements(index, &pos->nodeLoc, &pos->nodeLoc, pos->index, pos->index + 1, CB_DATABASE_BTREE_HALF_ELEMENTS - pos->index, false)
				// Move new element into old node
				|| ! CBDatabaseIndexSetElement(index, &pos->nodeLoc, indexVal, pos->index, false)) {
				CBLogError("Could not insert the new element into the old node and move elements to the new node.");
				return false;
			}
			if (! bottom
				&& (// Move children in old node to make room for the right child
					! CBDatabaseIndexMoveChildren(index, &pos->nodeLoc, &pos->nodeLoc, pos->index + 1, pos->index + 2, CB_DATABASE_BTREE_HALF_ELEMENTS - pos->index - 1, false)
					// Insert the right child
					|| ! CBDatabaseIndexSetChild(index, &pos->nodeLoc, right, pos->index + 1, false)
					// Copy children into new node
					|| ! CBDatabaseIndexMoveChildren(index, &new, &pos->nodeLoc, CB_DATABASE_BTREE_HALF_ELEMENTS, 0, CB_DATABASE_BTREE_HALF_ELEMENTS + 1, true))) {
					CBLogError("Could not insert the children to the new node when the new element is in the first node.");
					return false;
				}
		}
		if (bottom){
			// If in bottom make all children blank.
			if (! CBDatabaseIndexSetBlankChildren(index, &new, 0, CB_DATABASE_BTREE_ELEMENTS + 1)) {
				CBLogError("Could not insert blank children into the new bottom node.");
				return false;
			}
			// Else set only the last CB_DATABASE_BTREE_HALF_ELEMENTS as blank
		}else if (! CBDatabaseIndexSetBlankChildren(index, &new, CB_DATABASE_BTREE_HALF_ELEMENTS, CB_DATABASE_BTREE_HALF_ELEMENTS)){
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
			if (! CBDatabaseIndexSetNumElements(index, &index->indexCache, 1, true)) {
				CBLogError("Could not write the number of elements to a new root node.");
				return false;
			}
			// Write the middle element
			if (! CBDatabaseIndexSetElement(index, &index->indexCache, middleEl, 0, true)) {
				CBLogError("Could not set the middle element to a new root node.");
				return false;
			}
			// Write blank elements
			uint16_t elementsSize = (10+index->keySize)*(CB_DATABASE_BTREE_ELEMENTS-1);
			if (! CBDatabaseAppendZeros(index->database, CB_DATABASE_FILE_TYPE_INDEX, index, index->newLastFile, elementsSize)) {
				CBLogError("Could not write the blank elements for a new root node.");
				return false;
			}
			// Write left child
			if (! CBDatabaseIndexSetChild(index, &index->indexCache, &pos->nodeLoc, 0, true)) {
				CBLogError("Could not write the left child (old root) for a new root node.");
				return false;
			}
			// Write right child
			if (! CBDatabaseIndexSetChild(index, &index->indexCache, &new, 1, true)) {
				CBLogError("Could not write the right child (new node) for a new root node.");
				return false;
			}
			// Write blank children
			if (! CBDatabaseIndexSetBlankChildren(index, &index->indexCache, 2, CB_DATABASE_BTREE_ELEMENTS-1)) {
				CBLogError("Could not write the blank children for a new root node.");
				return false;
			}
			uint8_t data[6];
			// Overwrite root information in index
			CBInt16ToArray(data, 0, index->indexCache.diskNodeLoc.indexFile);
			CBInt32ToArray(data, 2, index->indexCache.diskNodeLoc.offset);
			if (! CBDatabaseAddOverwrite(index->database, CB_DATABASE_FILE_TYPE_INDEX, index, 0, data, 6, 6)) {
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
		return CBDatabaseIndexInsert(index, middleEl, pos, &new);
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
		if (! CBDatabaseAppend(index->database, CB_DATABASE_FILE_TYPE_INDEX, index, dest->diskNodeLoc.indexFile, data, 6 * amount)){
			CBLogError("Could not write the index children being moved by appending.");
			return false;
		}
	} else if (! CBDatabaseAddOverwrite(index->database, CB_DATABASE_FILE_TYPE_INDEX, index, dest->diskNodeLoc.indexFile, data, dest->diskNodeLoc.offset + 1 + CB_DATABASE_BTREE_ELEMENTS * (index->keySize + 10) + endPos * 6, 6 * amount)){
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
		CBInt16ToArray(dataCursor, 0, source->cachedNode->elements[startPos + x]->fileID);
		CBInt32ToArray(dataCursor, 2, source->cachedNode->elements[startPos + x]->length);
		CBInt32ToArray(dataCursor, 6, source->cachedNode->elements[startPos + x]->pos);
		// Add key to data
		memcpy(dataCursor + 10, source->cachedNode->elements[startPos + x]->key, index->keySize);
		dataCursor += elSize;
	}
	// Write the elements to the destination node
	if (append) {
		if (! CBDatabaseAppend(index->database, CB_DATABASE_FILE_TYPE_INDEX, index, dest->diskNodeLoc.indexFile, data, moveSize)){
			CBLogError("Could not write the index elements being moved by appending.");
			return false;
		}
	}else if (! CBDatabaseAddOverwrite(index->database, CB_DATABASE_FILE_TYPE_INDEX, index, dest->diskNodeLoc.indexFile, data, dest->diskNodeLoc.offset + 1 + endPos * elSize, moveSize)){
		CBLogError("Could not write the index elements being moved.");
		return false;
	}
	if (dest->cached)
		// Move elements for the cached destination node.
		memmove(dest->cachedNode->elements + endPos, source->cachedNode->elements + startPos, amount * sizeof(*source->cachedNode->elements));
	return true;
}
void CBDatabaseIndexNodeSearchDisk(CBDatabaseIndex * index, uint8_t * key, CBIndexFindResult * result){
	CBDepObject file;
	if (! CBDatabaseGetFile(index->database, &file, CB_DATABASE_FILE_TYPE_INDEX, index, result->data.node.diskNodeLoc.indexFile)) {
		CBLogError("Could not open an index file when loading a node.");
		result->status = CB_DATABASE_INDEX_ERROR;
		return;
	}
	uint32_t offset = result->data.node.diskNodeLoc.offset;
	if (! CBFileSeek(file, offset)){
		CBLogError("Could seek to the node position in an index file when loading a node.");
		result->status = CB_DATABASE_INDEX_ERROR;
		return;
	}
	//  First get the number of elements
	uint8_t elNum;
	if (! CBFileRead(file, &elNum, 1)) {
		CBLogError("Could not read the number of elements for an index node.");
		result->status = CB_DATABASE_INDEX_ERROR;
		return;
	}
	result->status = CB_DATABASE_INDEX_NOT_FOUND;
	uint8_t pos = 0;
	uint8_t data[10];
	CBIndexValue * val = CBNewIndexValue(index->keySize);
	if (elNum) {
		int cmp;
		for (;pos < elNum; pos++) {
			// Read element at position
			if (! CBFileRead(file, data, 10)) {
				CBLogError("Could not read the data for an index element.");
				result->status = CB_DATABASE_INDEX_ERROR;
				CBReleaseObject(val);
				return;
			}
			val->fileID = CBArrayToInt16(data, 0);
			val->length = CBArrayToInt32(data, 2);
			val->pos = CBArrayToInt32(data, 6);
			if (! CBFileRead(file, val->key, index->keySize)) {
				CBLogError("Could not read the key of an element in an index node.");
				result->status = CB_DATABASE_INDEX_ERROR;
				CBReleaseObject(val);
				return;
			}
			cmp = memcmp(key, val->key, index->keySize);
			if (cmp == 0) {
				result->status = CB_DATABASE_INDEX_FOUND;
				result->data.el = val;
				return;
			}else if (cmp < 0)
				break;
		}
	}
	CBReleaseObject(val);
	// Not found, read child location.
	if (! CBFileSeek(file, offset + 1 + CB_DATABASE_BTREE_ELEMENTS * (index->keySize + 10) + pos * 6)) {
		CBLogError("Could not seek to a child in an index node.");
		result->status = CB_DATABASE_INDEX_ERROR;
		return;
	}
	result->data.node.cached = false;
	if (! CBFileRead(file, data, 6)) {
		CBLogError("Could not read the location of an index child.");
		result->status = CB_DATABASE_INDEX_ERROR;
		return;
	}
	result->data.node.diskNodeLoc.indexFile = CBArrayToInt16(data, 0);
	result->data.node.diskNodeLoc.offset = CBArrayToInt32(data, 2);
}
void CBDatabaseIndexNodeBinarySearchMemory(CBDatabaseIndex * index, CBIndexNode * node, uint8_t * key, CBIndexFindStatus * status, uint8_t * pos){
	*status = CB_DATABASE_INDEX_NOT_FOUND;
	if (node->numElements) {
		uint8_t left = 0;
		uint8_t right = node->numElements - 1;
		int cmp;
		while (left <= right) {
			*pos = (right+left)/2;
			cmp = memcmp(key, node->elements[*pos]->key, index->keySize);
			if (cmp == 0) {
				*status = CB_DATABASE_INDEX_FOUND;
				break;
			}else if (cmp < 0){
				if (! *pos)
					break;
				right = *pos - 1;
			}else
				left = *pos + 1;
		}
		if (cmp > 0)
			(*pos)++;
	}else
		*pos = 0;
}
bool CBDatabaseIndexSetBlankChildren(CBDatabaseIndex * index, CBIndexNodeLocation * dest, uint8_t offset, uint8_t amount){
	if (! CBDatabaseAppendZeros(index->database, CB_DATABASE_FILE_TYPE_INDEX, index, dest->diskNodeLoc.indexFile, amount * 6)) {
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
		if (! CBDatabaseAppend(index->database, CB_DATABASE_FILE_TYPE_INDEX, index, node->diskNodeLoc.indexFile, data, 6)){
			CBLogError("Could not set a child on a node on disk by appending.");
			return false;
		}
	}else if (! CBDatabaseAddOverwrite(index->database, CB_DATABASE_FILE_TYPE_INDEX, index, node->diskNodeLoc.indexFile, data, node->diskNodeLoc.offset + 1 + CB_DATABASE_BTREE_ELEMENTS * (index->keySize + 10) + pos * 6, 6)){
		CBLogError("Could not set a child on a node on disk.");
		return false;
	}
	return true;
}
bool CBDatabaseIndexSetElement(CBDatabaseIndex * index, CBIndexNodeLocation * node, CBIndexValue * element, uint8_t pos, bool append){
	if (node->cached){
		// Add the element to the cached node.
		CBRetainObject(element);
		node->cachedNode->elements[pos] = element;
	}
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
		if (! CBDatabaseAppend(index->database, CB_DATABASE_FILE_TYPE_INDEX, index, node->diskNodeLoc.indexFile, data, elSize)){
			CBLogError("Could not write an index element being added to a node on disk by appending.");
			return false;
		}
	}else if (! CBDatabaseAddOverwrite(index->database, CB_DATABASE_FILE_TYPE_INDEX, index, node->diskNodeLoc.indexFile, data, node->diskNodeLoc.offset + 1 + pos * elSize, elSize)){
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
	assert(numElements <= CB_DATABASE_BTREE_ELEMENTS);
	if (node->cached)
		node->cachedNode->numElements = numElements;
	if (append) {
		if (! CBDatabaseAppend(index->database, CB_DATABASE_FILE_TYPE_INDEX, index, node->diskNodeLoc.indexFile, &numElements, 1)) {
			CBLogError("Could not update the number of elements in storage for an index node by appending.");
			return false;
		}
	}else if (! CBDatabaseAddOverwrite(index->database, CB_DATABASE_FILE_TYPE_INDEX, index, node->diskNodeLoc.indexFile, &numElements, node->diskNodeLoc.offset, 1)) {
		CBLogError("Could not update the number of elements in storage for an index node.");
		return false;
	}
	return true;
}
CBIndexFindStatus CBDatabaseRangeIteratorFirst(CBDatabaseRangeIterator * it){
	CBMutexLock(it->index->database->databaseMutex);
	CBIndexFindStatus ret = CBDatabaseRangeIteratorFirstNoMutex(it);
	CBMutexUnlock(it->index->database->databaseMutex);
	return ret;
}
CBIndexFindStatus CBDatabaseRangeIteratorFirstNoMutex(CBDatabaseRangeIterator * it){
	// Set the minimum and maximum elements for the iterators.
	it->currentIt.minElement = it->minElement;
	it->currentIt.maxElement = it->maxElement;
	it->stagedIt.minElement = it->minElement;
	it->stagedIt.maxElement = it->maxElement;
	it->currentChangedIt.minElement = it->minElement;
	it->currentChangedIt.maxElement = it->maxElement;
	it->stagedChangedIt.minElement = it->minElement;
	it->stagedChangedIt.maxElement = it->maxElement;
	// Get the first elements in each iterator.
	CBIndexTxData * currentIndexTxData = CBDatabaseGetIndexTxData(&it->index->database->current, it->index, false);
	if (currentIndexTxData) {
		currentIndexTxData->valueWrites.compareFunc = CBTransactionKeysCompare;
		it->gotCurrentEl = CBAssociativeArrayRangeIteratorStart(&currentIndexTxData->valueWrites, &it->currentIt);
		currentIndexTxData->valueWrites.compareFunc = CBTransactionKeysAndOffsetCompare;
		currentIndexTxData->changeKeysNew.compareFunc = CBNewKeyWithSingleKeyCompare;
		it->gotCurrentChangedEl = CBAssociativeArrayRangeIteratorStart(&currentIndexTxData->changeKeysNew, &it->currentChangedIt);
		currentIndexTxData->changeKeysNew.compareFunc = CBNewKeyCompare;
	}else
		it->gotCurrentChangedEl = it->gotCurrentEl = false;
	CBIndexTxData * stagedIndexTxData = CBDatabaseGetIndexTxData(&it->index->database->staged, it->index, false);
	if (stagedIndexTxData) {
		stagedIndexTxData->valueWrites.compareFunc = CBTransactionKeysCompare;
		it->gotStagedEl = CBAssociativeArrayRangeIteratorStart(&stagedIndexTxData->valueWrites, &it->stagedIt);
		stagedIndexTxData->valueWrites.compareFunc = CBTransactionKeysAndOffsetCompare;
		stagedIndexTxData->changeKeysNew.compareFunc = CBNewKeyWithSingleKeyCompare;
		it->gotStagedChangedEl = CBAssociativeArrayRangeIteratorStart(&stagedIndexTxData->changeKeysNew, &it->stagedChangedIt);
		stagedIndexTxData->changeKeysNew.compareFunc = CBNewKeyCompare;
	}else
		it->gotStagedChangedEl = it->gotStagedEl = false;
	// Iterate the staged elements if there is a change key entry or remove key entry for that value making it invalid
	if (it->gotStagedChangedEl || it->gotStagedEl)
		CBDatabaseRangeIteratorIterateStagedIfInvalid(it, currentIndexTxData, stagedIndexTxData, true);
	// Try to find the minimum index element
	CBIndexFindWithParentsResult res = CBDatabaseIndexFindWithParents(it->index, it->minElement);
	it->foundIndex = false;
	if (res.status == CB_DATABASE_INDEX_ERROR) {
		CBLogError("There was an error when attempting to find the first index element in a range.");
		return res.status;
	}
	bool hasNonIndex = it->gotCurrentChangedEl || it->gotCurrentEl || it->gotStagedChangedEl || it->gotStagedEl;
	if (res.status == CB_DATABASE_INDEX_NOT_FOUND) {
		if (res.el.nodeLoc.cachedNode->numElements == 0){
			// There are no elements
			it->gotIndEl = false;
			CBFreeIndexElementPos(res.el);
			return hasNonIndex ? CB_DATABASE_INDEX_FOUND : CB_DATABASE_INDEX_NOT_FOUND;
		}
		// If we are past the end of the node's elements, go back onto the previous element, and then iterate.
		if (res.el.index == res.el.nodeLoc.cachedNode->numElements) {
			res.el.index--;
			it->indexPos = res.el;
			it->foundIndex = true;
			CBIndexFindStatus ret = CBDatabaseRangeIteratorIterateIndex(it, true);
			return hasNonIndex ? CB_DATABASE_INDEX_FOUND : ret;
		}
		// Else we have landed on the element after the minimum element.
		// Check that the element is below or equal to the maximum
		if (memcmp(res.el.nodeLoc.cachedNode->elements[res.el.index]->key, it->maxElement, it->index->keySize) > 0){
			// The element is above the maximum
			it->gotIndEl = false;
			CBFreeIndexElementPos(res.el);
			return hasNonIndex ? CB_DATABASE_INDEX_FOUND : CB_DATABASE_INDEX_NOT_FOUND;
		}
	}
	it->indexPos = res.el;
	it->foundIndex = true;
	it->gotIndEl = true;
	// Iterate index if deleted
	CBIndexFindStatus stat = CBDatabaseRangeIteratorIterateIndexIfDeleted(it, true);
	return stat;
}
bool CBDatabaseRangeIteratorGetKey(CBDatabaseRangeIterator * it, uint8_t * key){
	switch (CBDatabaseRangeIteratorGetLesser(it)) {
		case CB_KEY_CURRENT_VALUE:
			memcpy(key, CBRangeIteratorGetPointer(&it->currentIt), it->index->keySize);
			return true;
		case CB_KEY_CURRENT_CHANGE:
			memcpy(key, (uint8_t *)CBRangeIteratorGetPointer(&it->currentChangedIt) + it->index->keySize, it->index->keySize);
			return true;
		case CB_KEY_STAGED_VALUE:
			memcpy(key, CBRangeIteratorGetPointer(&it->stagedIt), it->index->keySize);
			return true;
		case CB_KEY_STAGED_CHANGE:
			memcpy(key, (uint8_t *)CBRangeIteratorGetPointer(&it->stagedChangedIt) + it->index->keySize, it->index->keySize);
			return true;
		case CB_KEY_DISK:
			memcpy(key, it->indexPos.nodeLoc.cachedNode->elements[it->indexPos.index]->key, it->index->keySize);
			return true;
		case CB_KEY_NONE:
			return false;
	}
}
bool CBDatabaseRangeIteratorGetLength(CBDatabaseRangeIterator * it, uint32_t * length){
	// ??? We already have found the element in the transaction or index, no need to find twice. Should be changed
	uint8_t key[it->index->keySize];
	CBDatabaseRangeIteratorGetKey(it, key);
	return CBDatabaseGetLength(it->index, key, length);
}
CBIteratorWhatKey CBDatabaseRangeIteratorGetHigher(CBDatabaseRangeIterator * it){
	CBIteratorWhatKey what = CB_KEY_NONE;
	uint8_t * highestKey;
	if (it->gotCurrentEl){
		what = CB_KEY_CURRENT_VALUE;
		highestKey = CBRangeIteratorGetPointer(&it->currentIt);
	}else
		highestKey = NULL;
	if (it->gotCurrentChangedEl && CBDatabaseRangeIteratorKeyIfHigher(&highestKey, &it->currentChangedIt, it->index->keySize, true))
		what = CB_KEY_CURRENT_CHANGE;
	if (it->gotStagedEl && CBDatabaseRangeIteratorKeyIfHigher(&highestKey, &it->stagedIt, it->index->keySize, false))
		what = CB_KEY_STAGED_VALUE;
	if (it->gotStagedChangedEl && CBDatabaseRangeIteratorKeyIfHigher(&highestKey, &it->stagedChangedIt, it->index->keySize, true))
		what = CB_KEY_STAGED_CHANGE;
	if (it->gotIndEl && (highestKey == NULL || memcmp(highestKey, it->indexPos.nodeLoc.cachedNode->elements[it->indexPos.index]->key, it->index->keySize) < 0))
		what = CB_KEY_DISK;
	return what;
}
CBIteratorWhatKey CBDatabaseRangeIteratorGetLesser(CBDatabaseRangeIterator * it){
	CBIteratorWhatKey what = CB_KEY_NONE;
	uint8_t * lowestKey;
	if (it->gotCurrentEl){
		what = CB_KEY_CURRENT_VALUE;
		lowestKey = CBRangeIteratorGetPointer(&it->currentIt);
	}else
		lowestKey = NULL;
	if (it->gotCurrentChangedEl && CBDatabaseRangeIteratorKeyIfLesser(&lowestKey, &it->currentChangedIt, it->index->keySize, true))
		what = CB_KEY_CURRENT_CHANGE;
	if (it->gotStagedEl && CBDatabaseRangeIteratorKeyIfLesser(&lowestKey, &it->stagedIt, it->index->keySize, false))
		what = CB_KEY_STAGED_VALUE;
	if (it->gotStagedChangedEl && CBDatabaseRangeIteratorKeyIfLesser(&lowestKey, &it->stagedChangedIt, it->index->keySize, true))
		what = CB_KEY_STAGED_CHANGE;
	if (it->gotIndEl && (lowestKey == NULL || memcmp(lowestKey, it->indexPos.nodeLoc.cachedNode->elements[it->indexPos.index]->key, it->index->keySize) > 0))
		what = CB_KEY_DISK;
	return what;
}
bool CBDatabaseRangeIteratorKeyIfHigher(uint8_t ** highestKey, CBRangeIterator * it, uint8_t keySize, bool changed){
	uint8_t * key = (uint8_t *)CBRangeIteratorGetPointer(it) + changed*keySize;
	if (*highestKey == NULL || memcmp(*highestKey, key, keySize) < 0){
		*highestKey = key;
		return true;
	}
	return false;
}
bool CBDatabaseRangeIteratorKeyIfLesser(uint8_t ** lowestKey, CBRangeIterator * it, uint8_t keySize, bool changed){
	uint8_t * key = (uint8_t *)CBRangeIteratorGetPointer(it) + changed*keySize;
	if (*lowestKey == NULL || memcmp(*lowestKey, key, keySize) > 0){
		*lowestKey = key;
		return true;
	}
	return false;
}
CBIndexFindStatus CBDatabaseRangeIteratorLast(CBDatabaseRangeIterator * it){
	CBMutexLock(it->index->database->databaseMutex);
	CBIndexFindStatus ret = CBDatabaseRangeIteratorLastNoMutex(it);
	CBMutexUnlock(it->index->database->databaseMutex);
	return ret;
}
CBIndexFindStatus CBDatabaseRangeIteratorLastNoMutex(CBDatabaseRangeIterator * it){
	// Go to the end of the txKeys
	it->currentIt.minElement = it->minElement;
	it->currentIt.maxElement = it->maxElement;
	it->stagedIt.minElement = it->minElement;
	it->stagedIt.maxElement = it->maxElement;
	it->currentChangedIt.minElement = it->minElement;
	it->currentChangedIt.maxElement = it->maxElement;
	it->stagedChangedIt.minElement = it->minElement;
	it->stagedChangedIt.maxElement = it->maxElement;
	// Get the last elements in each iterator
	CBIndexTxData * currentIndexTxData = CBDatabaseGetIndexTxData(&it->index->database->current, it->index, false);
	if (currentIndexTxData) {
		currentIndexTxData->valueWrites.compareFunc = CBTransactionKeysCompare;
		it->gotCurrentEl = CBAssociativeArrayRangeIteratorLast(&currentIndexTxData->valueWrites, &it->currentIt);
		currentIndexTxData->valueWrites.compareFunc = CBTransactionKeysAndOffsetCompare;
		currentIndexTxData->changeKeysNew.compareFunc = CBNewKeyWithSingleKeyCompare;
		it->gotCurrentChangedEl = CBAssociativeArrayRangeIteratorLast(&currentIndexTxData->changeKeysNew, &it->currentChangedIt);
		currentIndexTxData->changeKeysNew.compareFunc = CBNewKeyCompare;
	}else
		it->gotCurrentChangedEl = it->gotCurrentEl = false;
	CBIndexTxData * stagedIndexTxData = CBDatabaseGetIndexTxData(&it->index->database->staged, it->index, false);
	if (stagedIndexTxData) {
		stagedIndexTxData->valueWrites.compareFunc = CBTransactionKeysCompare;
		it->gotStagedEl = CBAssociativeArrayRangeIteratorLast(&stagedIndexTxData->valueWrites, &it->stagedIt);
		stagedIndexTxData->valueWrites.compareFunc = CBTransactionKeysAndOffsetCompare;
		stagedIndexTxData->changeKeysNew.compareFunc = CBNewKeyWithSingleKeyCompare;
		it->gotStagedChangedEl = CBAssociativeArrayRangeIteratorLast(&stagedIndexTxData->changeKeysNew, &it->stagedChangedIt);
		stagedIndexTxData->changeKeysNew.compareFunc = CBNewKeyCompare;
	}else
		it->gotStagedChangedEl = it->gotStagedEl = false;
	// Iterate backwards the staged elements if there is a change key entry for that value making it invalid
	if (it->gotStagedChangedEl || it->gotStagedEl)
		CBDatabaseRangeIteratorIterateStagedIfInvalid(it, currentIndexTxData, stagedIndexTxData, false);
	// Try to find the maximum index element
	CBIndexFindWithParentsResult res = CBDatabaseIndexFindWithParents(it->index, it->maxElement);
	it->foundIndex = false;
	it->gotIndEl = true;
	if (res.status == CB_DATABASE_INDEX_ERROR) {
		CBLogError("There was an error when attempting to find the last index element in a range.");
		return res.status;
	}
	if (res.status == CB_DATABASE_INDEX_NOT_FOUND) {
		if (res.el.nodeLoc.cachedNode->numElements == 0)
			// There are no elements
			it->gotIndEl = false;
		else
		// Not found goes to the bottom.
		if (res.el.index == 0) {
			// No child, try parent
			if (res.el.nodeLoc.cachedNode == it->index->indexCache.cachedNode)
				it->gotIndEl = false;
			else{
				if (! res.el.nodeLoc.cached)
					CBFreeIndexNode(res.el.nodeLoc.cachedNode);
				if (res.el.parentStack.num)
					res.el.nodeLoc = res.el.parentStack.nodes[res.el.parentStack.num-1];
				else
					res.el.nodeLoc = it->index->indexCache;
				res.el.index = res.el.parentStack.pos[res.el.parentStack.num--];
				if (res.el.index == 0)
					it->gotIndEl = false;
				else
					res.el.index--;
			}
		}else // Else we can just go back one in this node
			res.el.index--;
		// Check that the element is above or equal to the minimum
		if (it->gotIndEl && memcmp(res.el.nodeLoc.cachedNode->elements[res.el.index]->key, it->minElement, it->index->keySize) < 0)
			// The element is below the minimum
			it->gotIndEl = false;
	}
	if (it->gotIndEl) {
		it->indexPos = res.el;
		it->foundIndex = true;
		it->gotIndEl = true;
		// If the element is to be deleted, iterate.
		CBIndexFindStatus status = CBDatabaseRangeIteratorIterateIndexIfDeleted(it, false);
		if (status == CB_DATABASE_INDEX_ERROR) {
			CBLogError("Could not reverse iterate to an element which is not marked as deleted.");
			CBFreeIndexElementPos(res.el);
			return status;
		}
	}else
		CBFreeIndexElementPos(res.el);
	// Now only use biggest key
	CBIteratorWhatKey what = CBDatabaseRangeIteratorGetHigher(it);
	it->gotCurrentEl = what == CB_KEY_CURRENT_VALUE;
	it->gotCurrentChangedEl = what == CB_KEY_CURRENT_CHANGE;
	it->gotStagedEl = what == CB_KEY_STAGED_VALUE;
	it->gotStagedChangedEl = what == CB_KEY_STAGED_CHANGE;
	it->gotIndEl = what == CB_KEY_DISK;
	return what == CB_KEY_NONE ? CB_DATABASE_INDEX_NOT_FOUND : CB_DATABASE_INDEX_FOUND;
}
CBIndexFindStatus CBDatabaseRangeIteratorNext(CBDatabaseRangeIterator * it){
	// Get the key and then iterate all keys which are equal to it
	CBMutexLock(it->index->database->databaseMutex);
	uint8_t key[it->index->keySize];
	CBDatabaseRangeIteratorGetKey(it, key);
	CBIndexTxData * currentIndexTxData = CBDatabaseGetIndexTxData(&it->index->database->current, it->index, false);
	while (it->gotCurrentEl && memcmp(key, CBRangeIteratorGetPointer(&it->currentIt), it->index->keySize) == 0)
		it->gotCurrentEl = !CBAssociativeArrayRangeIteratorNext(&currentIndexTxData->valueWrites, &it->currentIt);
	if (it->gotCurrentChangedEl && memcmp(key, (uint8_t *)CBRangeIteratorGetPointer(&it->currentChangedIt) + it->index->keySize, it->index->keySize) == 0){
		currentIndexTxData->changeKeysNew.compareFunc = CBNewKeyWithSingleKeyCompare;
		it->gotCurrentChangedEl = !CBAssociativeArrayRangeIteratorNext(&currentIndexTxData->changeKeysNew, &it->currentChangedIt);
		currentIndexTxData->changeKeysNew.compareFunc = CBNewKeyCompare;
	}
	CBIndexTxData * stagedIndexTxData = CBDatabaseGetIndexTxData(&it->index->database->staged, it->index, false);
	while (it->gotStagedEl && memcmp(key, CBRangeIteratorGetPointer(&it->stagedIt), it->index->keySize) == 0)
		it->gotStagedEl = !CBAssociativeArrayRangeIteratorNext(&stagedIndexTxData->valueWrites, &it->stagedIt);
	if (it->gotStagedChangedEl && memcmp(key, (uint8_t *)CBRangeIteratorGetPointer(&it->stagedChangedIt) + it->index->keySize, it->index->keySize) == 0){
		stagedIndexTxData->changeKeysNew.compareFunc = CBNewKeyWithSingleKeyCompare;
		it->gotStagedChangedEl = !CBAssociativeArrayRangeIteratorNext(&stagedIndexTxData->changeKeysNew, &it->stagedChangedIt);
		stagedIndexTxData->changeKeysNew.compareFunc = CBNewKeyCompare;
	}
	// Iterate if invalid staged keys
	CBDatabaseRangeIteratorIterateStagedIfInvalid(it, currentIndexTxData, stagedIndexTxData, true);
	if (it->gotIndEl && memcmp(key, it->indexPos.nodeLoc.cachedNode->elements[it->indexPos.index]->key, it->index->keySize) == 0) {
		if (CBDatabaseRangeIteratorIterateIndex(it, true) == CB_DATABASE_INDEX_ERROR) {
			CBMutexUnlock(it->index->database->databaseMutex);
			return CB_DATABASE_INDEX_ERROR;
		}
	}
	bool got = it->gotCurrentEl || it->gotCurrentChangedEl || it->gotStagedEl || it->gotStagedChangedEl || it->gotIndEl;
	CBMutexUnlock(it->index->database->databaseMutex);
	return got ? CB_DATABASE_INDEX_FOUND : CB_DATABASE_INDEX_NOT_FOUND;
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
				if (! it->indexPos.nodeLoc.cached)
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
			if (! forwards)
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
			if (! child.cached){
				// Allocate memory for the node
				child.cachedNode = malloc(sizeof(*child.cachedNode));
				if (! CBDatabaseLoadIndexNode(it->index, child.cachedNode, child.diskNodeLoc.indexFile, child.diskNodeLoc.offset)) {
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
	int cmpres = memcmp(it->indexPos.nodeLoc.cachedNode->elements[it->indexPos.index]->key, forwards ? it->maxElement : it->minElement, it->index->keySize);
	bool notFound = forwards ? (cmpres > 0) : (cmpres < 0);
	if (notFound) {
		it->gotIndEl = false;
		return CB_DATABASE_INDEX_NOT_FOUND;
	}
	// If the element is to be deleted, iterate again.
	return CBDatabaseRangeIteratorIterateIndexIfDeleted(it, true);
}
CBIndexFindStatus CBDatabaseRangeIteratorIterateIndexIfDeleted(CBDatabaseRangeIterator * it, bool forwards){
	// Get the index tx data
	uint8_t * findKey = it->indexPos.nodeLoc.cachedNode->elements[it->indexPos.index]->key;
	CBIndexTxData * currentIndexTxData = CBDatabaseGetIndexTxData(&it->index->database->current, it->index, false);
	bool deleted = currentIndexTxData && (CBAssociativeArrayFind(&currentIndexTxData->deleteKeys, findKey).found
										  // Check old keys to see if this key is changed and thus cannot be used.
										  || CBAssociativeArrayFind(&currentIndexTxData->changeKeysOld, findKey).found);
	if (! deleted) {
		// Look in staged changes for deletion with no value writes or new change keys.
		CBIndexTxData * stagedIndexTxData = CBDatabaseGetIndexTxData(&it->index->database->staged, it->index, false);
		deleted = stagedIndexTxData && (CBAssociativeArrayFind(&stagedIndexTxData->deleteKeys, findKey).found
										|| CBAssociativeArrayFind(&stagedIndexTxData->changeKeysOld, findKey).found);
	}
	if (deleted){
		CBIndexFindStatus ret = CBDatabaseRangeIteratorIterateIndex(it, forwards);
		return it->gotCurrentEl || it->gotCurrentChangedEl || it->gotStagedEl || it->gotStagedChangedEl ? CB_DATABASE_INDEX_FOUND : ret;
	}
	return CB_DATABASE_INDEX_FOUND;
}
void CBDatabaseRangeIteratorIterateStagedIfInvalid(CBDatabaseRangeIterator * it, CBIndexTxData * currentIndexTxData, CBIndexTxData * stagedIndexTxData, bool forward){
	if (currentIndexTxData == NULL || stagedIndexTxData == NULL)
		return;
	stagedIndexTxData->valueWrites.compareFunc = CBTransactionKeysCompare;
	CBDatabaseRangeIteratorIterateStagedIfInvalidParticular(currentIndexTxData, &it->stagedIt, &stagedIndexTxData->valueWrites, &it->gotStagedEl, forward);
	stagedIndexTxData->valueWrites.compareFunc = CBTransactionKeysAndOffsetCompare;
	stagedIndexTxData->changeKeysNew.compareFunc = CBNewKeyWithSingleKeyCompare;
	CBDatabaseRangeIteratorIterateStagedIfInvalidParticular(currentIndexTxData, &it->stagedChangedIt, &stagedIndexTxData->changeKeysNew, &it->gotStagedChangedEl, forward);
	stagedIndexTxData->changeKeysNew.compareFunc = CBNewKeyCompare;
}
void CBDatabaseRangeIteratorIterateStagedIfInvalidParticular(CBIndexTxData * currentIndexTxData, CBRangeIterator * it, CBAssociativeArray * arr, bool * got, bool forward){
	if (*got) while (CBAssociativeArrayFind(&currentIndexTxData->changeKeysOld, CBRangeIteratorGetPointer(it)).found
					 || CBAssociativeArrayFind(&currentIndexTxData->deleteKeys, CBRangeIteratorGetPointer(it)).found) {
		if ((forward ? CBAssociativeArrayRangeIteratorNext : CBAssociativeArrayRangeIteratorPrev)(arr, it)){
			*got = false;
			break;
		}
	}
}
void CBDatabaseRangeIteratorNextMinimum(CBDatabaseRangeIterator * it){
	CBDatabaseRangeIteratorGetKey(it, it->minElement);
	for (uint8_t x = it->index->keySize; x--;)
		if (++((uint8_t *)it->minElement)[x] != 0)
			break;
}
bool CBDatabaseRangeIteratorRead(CBDatabaseRangeIterator * it, uint8_t * data, uint32_t dataLen, uint32_t offset){
	// ??? We already have found the element in the transaction or index, no need to find twice. Should be changed!
	uint8_t key[it->index->keySize];
	CBDatabaseRangeIteratorGetKey(it, key);
	return CBDatabaseReadValue(it->index, key, data, dataLen, offset, false) == CB_DATABASE_INDEX_FOUND;
}
CBIndexFindStatus CBDatabaseReadValue(CBDatabaseIndex * index, uint8_t * key, uint8_t * data, uint32_t dataSize, uint32_t offset, bool staged){
	CBMutexLock(index->database->databaseMutex);
	CBIndexFindStatus ret = CBDatabaseReadValueNoMutex(index, key, data, dataSize, offset, staged);
	CBMutexUnlock(index->database->databaseMutex);
	return ret;
}
CBIndexFindStatus CBDatabaseReadValueNoMutex(CBDatabaseIndex * index, uint8_t * key, uint8_t * data, uint32_t dataSize, uint32_t offset, bool staged){
	uint32_t highestWritten = dataSize;
	bool readyForNext = false;
	CBIndexValue * idxVal;
	CBDepObject file;
	CBIndexTxData * indexTxData;
	uint8_t * nextKey;
	// Get the index tx data
	indexTxData = CBDatabaseGetIndexTxData(staged ? &index->database->staged : &index->database->current, index, false);
	if (indexTxData) {
		// If there is a delete entry, it cannot be found.
		if (CBAssociativeArrayFind(&indexTxData->deleteKeys, key).found)
			return CB_DATABASE_INDEX_NOT_FOUND;
		// Look to see if this key is being changed to by searching changeKeysNew so that we use the old key for staged or disk.
		indexTxData->changeKeysNew.compareFunc = CBNewKeyWithSingleKeyCompare;
		CBFindResult ckres = CBAssociativeArrayFind(&indexTxData->changeKeysNew, key);
		nextKey = ckres.found ? CBFindResultToPointer(ckres) : key;
		indexTxData->changeKeysNew.compareFunc = CBNewKeyCompare;
		// Loop through write entrys
		uint8_t compareDataMin[index->keySize + 4];
		uint8_t compareDataMax[index->keySize + 4];
		memcpy(compareDataMin, key, index->keySize);
		memcpy(compareDataMax, compareDataMin, index->keySize);
		memset(compareDataMin + index->keySize, 0xFF, 4); // Higher offsets come first
		memset(compareDataMax + index->keySize, 0, 4);
		CBRangeIterator it = {compareDataMin, compareDataMax};
		if (CBAssociativeArrayRangeIteratorStart(&indexTxData->valueWrites, &it)) for (;;){
			uint8_t * writeValue = (uint8_t *)CBRangeIteratorGetPointer(&it) + index->keySize;
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
					if (! readyForNext) {
						if (staged) {
							// Now look for disk index value
							CBIndexFindResult res = CBDatabaseIndexFind(index, nextKey);
							if (res.status == CB_DATABASE_INDEX_ERROR) {
								CBLogError("Could not find an index value for reading.");
								return CB_DATABASE_INDEX_ERROR;
							}
							idxVal = res.data.el;
							if (! CBDatabaseGetFile(index->database, &file, CB_DATABASE_FILE_TYPE_DATA, 0, idxVal->fileID)) {
								CBLogError("Could not open file for reading part of a value.");
								CBReleaseObject(idxVal);
								return CB_DATABASE_INDEX_ERROR;
							}
						}
						readyForNext = true;
					}
					uint32_t readLen = highestWritten - (startOffset + readFromLen);
					uint32_t dataOffset = startOffset + readFromLen;
					uint32_t readOffset = offset + startOffset + readFromLen;
					if (staged) {
						if (! CBFileSeek(file, idxVal->pos + readOffset)){
							CBLogError("Could not read seek file for reading part of a value.");
							return CB_DATABASE_INDEX_ERROR;
						}
						if (! CBFileRead(file, data + dataOffset, readLen)) {
							CBLogError("Could not read from file for value.");
							return CB_DATABASE_INDEX_ERROR;
						}
					}else if (CBDatabaseReadValueNoMutex(index, nextKey, data + dataOffset, readLen, readOffset, true) != CB_DATABASE_INDEX_FOUND){
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
	}else
		nextKey = key;
	// Read in the rest of the data
	if (! readyForNext) {
		if (staged) {
			CBIndexFindResult res = CBDatabaseIndexFind(index, nextKey);
			if (res.status == CB_DATABASE_INDEX_ERROR) {
				CBLogError("Could not find an index value for reading.");
				return CB_DATABASE_INDEX_ERROR;
			}
			if (res.status != CB_DATABASE_INDEX_FOUND)
				return res.status;
			idxVal = res.data.el;
			// If deleted not found
			if (idxVal->length == CB_DELETED_VALUE){
				CBReleaseObject(idxVal);
				return CB_DATABASE_INDEX_NOT_FOUND;
			}
			if (! CBDatabaseGetFile(index->database, &file, CB_DATABASE_FILE_TYPE_DATA, 0, idxVal->fileID)) {
				CBLogError("Could not open file for reading part of a value.");
				CBReleaseObject(idxVal);
				return CB_DATABASE_INDEX_ERROR;
			}
		}
	}
	if (staged) {
		if (! CBFileSeek(file, idxVal->pos + offset)) {
			CBLogError("Could not read seek file for reading part of a value.");
			return CB_DATABASE_INDEX_ERROR;
		}
		if (! CBFileRead(file, data, highestWritten)) {
			CBLogError("Could not read from file for value.");
			return CB_DATABASE_INDEX_ERROR;
		}
		CBReleaseObject(idxVal);
	}else
		return CBDatabaseReadValueNoMutex(index, nextKey, data, highestWritten, offset, true);
	return CB_DATABASE_INDEX_FOUND;
}
bool CBDatabaseRemoveValue(CBDatabaseIndex * index, uint8_t * key, bool stage) {
	CBMutexLock(index->database->databaseMutex);
	bool ret = CBDatabaseRemoveValueNoMutex(index, key, stage);
	CBMutexUnlock(index->database->databaseMutex);
	return ret;
}
bool CBDatabaseRemoveValueNoMutex(CBDatabaseIndex * index, uint8_t * key, bool stage) {
	// Get the index tx data
	CBIndexTxData * indexTxData = CBDatabaseGetIndexTxData(stage ? &index->database->staged : &index->database->current, index, true);
	// Remove all instances of the key in the values writes array
	indexTxData->valueWrites.compareFunc = CBTransactionKeysCompare;
	CBFindResult res;
	for (;;) {
		res = CBAssociativeArrayFind(&indexTxData->valueWrites, key);
		if (! res.found)
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
	if (! res.found) {
		// If there was a change key then there must be the old key in the index or staged
		bool foundKey = false;
		if (! stage) {
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
		if (! foundKey) {
			CBIndexFindWithParentsResult fres = CBDatabaseIndexFindWithParents(index, key);
			CBFreeIndexElementPos(fres.el);
			if (fres.status == CB_DATABASE_INDEX_ERROR) {
				CBLogError("There was an error whilst trying to find a previous key in the index to see if a change key entry should be made.");
				return false;
			}
			if (fres.status != CB_DATABASE_INDEX_FOUND)
				return true;
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
	CBMutexLock(self->databaseMutex);
	bool ret = CBDatabaseStageNoMutex(self);
	CBMutexUnlock(self->databaseMutex);
	return ret;
}
bool CBDatabaseStageNoMutex(CBDatabase * self){
	// Go through all indicies
	CBAssociativeArrayForEach(CBIndexTxData * indexTxData, &self->current.indexes){
		// Change keys
		CBAssociativeArrayForEach(uint8_t * changeKey, &indexTxData->changeKeysOld)
			if (! CBDatabaseChangeKeyNoMutex(indexTxData->index, changeKey, changeKey + indexTxData->index->keySize, true)) {
				CBLogError("Could not add a change key change to staged changes.");
				return false;
			}
		// Write values
		CBAssociativeArrayForEach(uint8_t * valueWrite, &indexTxData->valueWrites) {
			uint32_t * offsetPtr = (uint32_t *)(valueWrite + indexTxData->index->keySize);
			uint32_t * sizePtr = offsetPtr + 1;
			uint8_t * dataPtr = (uint8_t *)(sizePtr + 1);
			CBDatabaseAddWriteValueNoMutex(valueWrite, indexTxData->index, valueWrite, *sizePtr, dataPtr, *offsetPtr, true);
		}
		// Remove the value writes array onFree function because we have used the data for the staging
		indexTxData->valueWrites.onFree = NULL;
		// Deletions
		CBAssociativeArrayForEach(uint8_t * deleteKey, &indexTxData->deleteKeys)
			if (! CBDatabaseRemoveValueNoMutex(indexTxData->index, deleteKey, true)) {
				CBLogError("Could not add a change key change to staged changes.");
				return false;
			}
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
		if (! CBDatabaseCommitNoMutex(self)) {
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
	int res = memcmp((uint8_t *)el1 + index->keySize, (uint8_t *)el2 + index->keySize, index->keySize);
	if (res > 0)
		return CB_COMPARE_MORE_THAN;
	if (res < 0)
		return CB_COMPARE_LESS_THAN;
	return CB_COMPARE_EQUAL;
}
CBCompare CBNewKeyWithSingleKeyCompare(CBAssociativeArray * array, void * el1, void * el2){
	CBDatabaseIndex * index = array->compareObject;
	int res = memcmp(el1, (uint8_t *)el2 + index->keySize, index->keySize);
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
