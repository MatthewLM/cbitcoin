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

#include "CBDatabase.h"

CBDatabase * CBNewDatabase(char * dataDir, char * folder){
	// Try to create the object
	CBDatabase * self = malloc(sizeof(*self));
	if (NOT self) {
		CBLogError("Could not create a database object.");
		return 0;
	}
	if (NOT CBInitDatabase(self, dataDir, folder)) {
		free(self);
		CBLogError("Could not initialise a database object.");
		return 0;
	}
	return self;
}
CBDatabaseTransaction * CBNewDatabaseTransaction(){
	CBDatabaseTransaction * self = malloc(sizeof(*self));
	if (NOT self) {
		CBLogError("Could not create a database transaction object.");
		return 0;
	}
	if (NOT CBInitDatabaseTransaction(self)) {
		free(self);
		CBLogError("Could not initialise a database transaction object.");
		return 0;
	}
	return self;
}
bool CBInitDatabase(CBDatabase * self, char * dataDir, char * folder){
	self->dataDir = dataDir;
	self->folder = folder;
	self->lastUsedFileType = CB_DATABASE_FILE_TYPE_NONE; // No stored file yet.
	char filename[strlen(self->dataDir) + strlen(self->folder) + 12];
	// Create folder if it doesn't exist
	sprintf(filename, "%s/%s/", self->dataDir, self->folder);
	if (access(filename, F_OK)
		&& mkdir(filename, 770)){
		CBLogError("Could not create the folder for a database.");
		return false;
	}
	// Check data consistency
	if (NOT CBDatabaseEnsureConsistent(self)){
		CBLogError("The database is inconsistent and could not be recovered in CBNewDatabase");
		return false;
	}
	// Load deletion index
	if (NOT CBInitAssociativeArray(&self->deletionIndex, CBKeyCompare, free)){
		CBLogError("Could not initialise the database deletion index.");
		return false;
	}
	strcat(filename, "del.dat");
	// First check that the index exists
	if (NOT access(filename, F_OK)) {
		// Can read deletion index
		if (NOT CBDatabaseReadAndOpenDeletionIndex(self, filename)) {
			CBFreeAssociativeArray(&self->deletionIndex);
			CBLogError("Could not load the database deletion index.");
			return false;
		}
	}else{
		// Else empty deletion index
		if (NOT CBDatabaseCreateDeletionIndex(self, filename)) {
			CBFreeAssociativeArray(&self->deletionIndex);
			CBLogError("Could not create the database deletion index.");
			return false;
		}
	}
	// Get the last file and last file size
	uint8_t data[6];
	strcpy(filename + strlen(self->dataDir) + strlen(self->folder) + 2, "val_0.dat");
	// Check that the data file exists.
	if (access(filename, F_OK)) {
		// The data file does not exist.
		self->lastFile = 0;
		self->lastSize = 6;
		uint64_t file = CBFileOpen(filename, true);
		if (NOT file) {
			CBFreeAssociativeArray(&self->deletionIndex);
			CBLogError("Could not open a new first data file.");
			return false;
		}
		if (NOT CBFileAppend(file, (uint8_t []){0,0,6,0,0,0}, 6)) {
			CBFileClose(file);
			CBFreeAssociativeArray(&self->deletionIndex);
			CBLogError("Could not write the initial data for the first data file.");
			return false;
		}
	}else{
		// The data file does exist.
		uint64_t file = CBFileOpen(filename, false);
		if (NOT file) {
			CBFreeAssociativeArray(&self->deletionIndex);
			CBLogError("Could not open the data file.");
			return false;
		}
		if (NOT CBFileRead(file, data, 6)) {
			CBFileClose(file);
			CBFreeAssociativeArray(&self->deletionIndex);
			CBLogError("Could not read the last file and last file size from the first data file.");
			return false;
		}
		self->lastFile = CBArrayToInt16(data, 0);
		self->lastSize = CBArrayToInt32(data, 2);
	}
	return true;
}
bool CBInitDatabaseTransaction(CBDatabaseTransaction * self){
	self->numChangeKeys = 0;
	self->changeKeys = NULL;
	if (NOT CBInitAssociativeArray(&self->deleteKeys, CBKeyCompare, free)){
		CBLogError("Could not initilaise the deleteKeys array.");
		return false;
	}
	if (NOT CBInitAssociativeArray(&self->valueWrites, CBKeyCompare, free)){
		CBFreeAssociativeArray(&self->deleteKeys);
		CBLogError("Could not initilaise the valueWrites array.");
		return false;
	}
	return true;
}
CBDatabaseIndex * CBLoadIndex(CBDatabase * self, uint8_t indexID, uint8_t keySize, uint32_t cacheLimit){
	// Load index
	CBDatabaseIndex * index = malloc(sizeof(*index));
	if (NOT index) {
		CBLogError("Could not allocate memory for a database index.");
		return NULL;
	}
	index->ID = indexID;
	index->keySize = keySize;
	index->cacheLimit = cacheLimit;
	// Get filename
	char filename[strlen(self->dataDir) + strlen(self->folder) + 18];
	sprintf(filename, "%s/%s/idx_%u_0.dat", self->dataDir, self->folder, indexID);
	// Get the file
	uint64_t indexFile = CBDatabaseGetFile(self, CB_DATABASE_FILE_TYPE_INDEX, indexID, 0);
	if (NOT indexFile) {
		free(index);
		CBLogError("Could not open a database index.");
		return NULL;
	}
	// First check that the index exists
	if (NOT access(filename, F_OK)) {
		// Can be read
		uint8_t data[8];
		// Get the last file information
		if (NOT CBFileRead(indexFile, data, 6)){
			free(index);
			CBFileClose(indexFile);
			CBLogError("Could not read the number of nodes from a database index.");
			return NULL;
		}
		index->lastFile = CBArrayToInt16(data, 0);
		index->lastFile = CBArrayToInt32(data, 2);
		// Get the root position
		if (NOT CBFileRead(indexFile, data, 6)){
			free(index);
			CBFileClose(indexFile);
			CBLogError("Could not read the root location from a database index.");
			return NULL;
		}
		// Load memory cache
		index->indexCache.parent = NULL;
		if (NOT CBDatabaseLoadIndexNode(self, index, &index->indexCache, CBArrayToInt16(data, 0), CBArrayToInt32(data, 2))) {
			free(index);
			CBFileClose(indexFile);
			CBLogError("Could not load the root memory cache for a database index.");
			return NULL;
		}
		index->numCached = 1;
		CBIndexNode * first;
		uint32_t numNodes = 1;
		CBIndexNode * node = &index->indexCache;
		for (uint32_t x = 0;; x++) {
			// For each child in the node, create a cache upto the cache limit.
			bool reached = false;
			for (uint8_t y = 0; y <= node->numElements; y++) {
				// See if the cached data passes limit. Assumes nodes are full for key data.
				if ((index->numCached + 1) * (index->keySize * CB_DATABASE_BTREE_ELEMENTS + sizeof(CBIndexNode)) > index->cacheLimit) {
					reached = true;
					break;
				}
				// Get the child position.
				uint16_t indexID = node->children[y].location.diskNodeLoc.indexFile;
				uint32_t offset = node->children[y].location.diskNodeLoc.offset;
				// Allocate memory for node.
				node->children[y].cached = false;
				node->children[y].location.cachedNode = malloc(sizeof(*node->children[y].location.cachedNode));
				if (NOT node->children[y].location.cachedNode) {
					CBFreeIndex(index);
					CBFileClose(indexFile);
					CBLogError("Could not allocate memory for a cached index node.");
					return NULL;
				}
				// Load the node.
				if (NOT CBDatabaseLoadIndexNode(self, index, node->children[y].location.cachedNode, indexID, offset)) {
					CBFreeIndex(index);
					CBFileClose(indexFile);
					CBLogError("Could not load memory cache for a database index.");
					return NULL;
				}
				((CBIndexNode *)node->children[y].location.cachedNode)->parent = node;
				((CBIndexNode *)node->children[y].location.cachedNode)->parentChildIndex = y;
			}
			if (x == 0)
				// This node has the first child.
				first = node->children[0].location.cachedNode;
			if (reached)
				break;
			if (x == numNodes - 1) {
				// Move to next level
				x = 0;
				node = first;
				// See if this node has any children
				if (node->children[0].location.diskNodeLoc.offset == 0
					&& node->children[0].location.diskNodeLoc.indexFile == 0)
					break;
			}else
				// Move to the next node in this level
				node = ((CBIndexNode *)node->parent)->children[node->parentChildIndex + 1].location.cachedNode;
		}
	}else{
		// Cannot be read. Make initial data
		uint16_t nodeLen = ((16 + keySize) * CB_DATABASE_BTREE_ELEMENTS) + 7;
		// Make initial memory data
		index->lastFile = 0;
		index->lastSize = 16 + nodeLen;
		index->newLastFile = 0;
		index->newLastSize = index->lastSize;
		index->numCached = 1;
		index->indexCache.parent = NULL;
		index->indexCache.numElements = 0;
		// Write to disk initial data
		if (NOT CBFileAppend(indexFile, (uint8_t [16]){
			0,0, // First file is zero
			index->lastSize,0,0,0, // Size of file is 16 plus the node length
			0,0, // Root exists in file 0
			16,0,0,0, // Root has an offset of 16,
		}, 16)) {
			free(index);
			CBFileClose(indexFile);
			CBLogError("Could not write inital data to the index.");
			return NULL;
		}
		uint8_t zero = 0;
		for (uint16_t x = 0; x < nodeLen; x++) {
			if (NOT CBFileAppend(indexFile, &zero, 1)) {
				free(index);
				CBFileClose(indexFile);
				CBLogError("Could not write inital data to the index with zeros.");
				return NULL;
			}
		}
	}
	return index;
}
bool CBDatabaseLoadIndexNode(CBDatabase * self, CBDatabaseIndex * index, CBIndexNode * node, uint16_t nodeFile, uint32_t nodeOffset){
	// Assign the file ID and offset to the node.
	node->indexFile = nodeFile;
	node->offset = nodeOffset;
	// Read the node from the disk.
	uint64_t file = CBDatabaseGetFile(self, CB_DATABASE_FILE_TYPE_INDEX, index->ID, nodeFile);
	if (NOT file) {
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
		if (NOT node->elements[x].key
			|| NOT CBFileRead(file, node->elements[x].key, index->keySize)) {
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
		node->children[x].location.diskNodeLoc.indexFile = CBArrayToInt16(data, 0);
		node->children[x].location.diskNodeLoc.offset = CBArrayToInt32(data, 2);
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
	CBFreeAssociativeArray(&self->deletionIndex);
	// Close files
	CBFileClose(self->deletionIndexFile);
	if (self->lastUsedFileType != CB_DATABASE_FILE_TYPE_NONE)
		CBFileClose(self->fileObjectCache);
	free(self);
}
void CBFreeDatabaseTransaction(CBDatabaseTransaction * self){
	// Free write data
	CBFreeAssociativeArray(&self->valueWrites);
	// Free deletion data
	CBFreeAssociativeArray(&self->deleteKeys);
	// Free key change data
	for (uint32_t x = 0; x < self->numChangeKeys; x++)
		free(self->changeKeys[x]);
	free(self->changeKeys);
}
void CBFreeIndexNode(CBIndexNode * node){
	for (uint8_t x = 0; x < node->numElements; x++)
		free(node->elements[x].key);
	for (uint8_t x = 0; x <= node->numElements; x++)
		if (node->children[x].cached)
			CBFreeIndexNode(node->children[x].location.cachedNode);
	free(node);
}
void CBFreeIndex(CBDatabaseIndex * index){
	CBFreeIndexNode(&index->indexCache);
	free(index);
}
bool CBDatabaseAddDeletionEntry(CBDatabase * self, uint16_t fileID, uint32_t pos, uint32_t len){
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
		if (NOT CBAssociativeArrayInsert(&self->deletionIndex, section->key, CBAssociativeArrayFind(&self->deletionIndex, section->key).position, NULL)){
			CBLogError("Could not insert new deletion entry into the array.");
			return false;
		}
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
	uint64_t file = CBDatabaseGetFile(self, fileType, indexID, fileID);
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
	if (NOT CBFileAppend(self->logFile, dataTemp, dataTempLen)) {
		CBLogError("Could not write the backup information to the transaction log file.");
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
		if (NOT CBAssociativeArrayInsert(&self->deletionIndex, section->key, CBAssociativeArrayFind(&self->deletionIndex, section->key).position, NULL)){
			CBLogError("Failed to change the key of a deleted section to inactive.");
			return false;
		}
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
bool CBDatabaseAddWriteValue(CBDatabaseTransaction * self, uint8_t * writeValue) {
	// Remove from deletion array if needed.
	CBFindResult res = CBAssociativeArrayFind(&self->deleteKeys, writeValue);
	if (res.found)
		CBAssociativeArrayDelete(&self->deleteKeys, res.position, false);
	// See if key exists to be written already
	res = CBAssociativeArrayFind(&self->valueWrites, writeValue);
	if (res.found) {
		// Found so replace the element
		free(CBFindResultToPointer(res));
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
bool CBDatabaseAppend(CBDatabase * self, CBDatabaseFileType fileType, uint8_t indexID, uint16_t fileID, uint8_t * data, uint32_t dataLen) {
	// Execute the append operation
	uint64_t file = CBDatabaseGetFile(self, fileType, indexID, fileID);
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
bool CBDatabaseChangeKey(CBDatabaseTransaction * self, CBDatabaseIndex * index, uint8_t * previousKey, uint8_t * newKey) {
	self->changeKeys = realloc(self->changeKeys, sizeof(*self->changeKeys) * (self->numChangeKeys + 1));
	if (NOT self->changeKeys) {
		CBLogError("Failed to reallocate memory for the key change array.");
		CBDatabaseClearPending(self);
		return false;
	}
	self->changeKeys[self->numChangeKeys] = malloc(sizeof(index) + index->keySize * 2);
	if (NOT self->changeKeys[self->numChangeKeys]) {
		CBLogError("Failed to allocate memory for the information for changing a key.");
		CBDatabaseClearPending(self);
		return false;
	}
	*(CBDatabaseIndex **)self->changeKeys[self->numChangeKeys] = index;
	self->changeKeys[self->numChangeKeys] += sizeof(index);
	memcpy(self->changeKeys[self->numChangeKeys], previousKey, index->keySize);
	memcpy(self->changeKeys[self->numChangeKeys] + index->keySize, newKey, index->keySize);
	self->numChangeKeys++;
	return true;
}
void CBDatabaseClearPending(CBDatabaseTransaction * self) {
	// Free write data
	CBFreeAssociativeArray(&self->valueWrites);
	CBInitAssociativeArray(&self->valueWrites, CBKeyCompare, free);
	// Free deletion data
	CBFreeAssociativeArray(&self->deleteKeys);
	CBInitAssociativeArray(&self->deleteKeys, CBKeyCompare, free);
	// Free key change data
	for (uint32_t x = 0; x < self->numChangeKeys; x++)
		free(self->changeKeys[x]);
	free(self->changeKeys);
	self->changeKeys = NULL;
	self->numChangeKeys = 0;
}
bool CBDatabaseCommit(CBDatabase * self, CBDatabaseTransaction * tx){
	char filename[strlen(self->dataDir) + strlen(self->folder) + 10];
	sprintf(filename, "%s/%s/log.dat", self->dataDir, self->folder);
	// Open the log file
	self->logFile = CBFileOpen(filename, true);
	if (NOT self->logFile) {
		CBLogError("The log file for overwritting could not be opened.");
		CBDatabaseClearPending(tx);
		return false;
	}
	// Write the previous sizes for the files to the log file
	uint8_t data[12];
	data[0] = 1; // Log file active.
	uint32_t deletionIndexLen;
	if (NOT CBFileGetLength(self->deletionIndexFile, &deletionIndexLen)) {
		CBLogError("Could not get the length of the deletion index file.");
		CBDatabaseClearPending(tx);
		return false;
	}
	CBInt32ToArray(data, 1, deletionIndexLen);
	CBInt16ToArray(data, 5, self->lastFile);
	CBInt32ToArray(data, 7, self->lastSize);
	data[11] = tx->numIndexes;
	if (NOT CBFileAppend(self->logFile, data, 12)) {
		CBLogError("Could not write previous size information to the log-file for the data and deletion index.");
		CBFileClose(self->logFile);
		CBDatabaseClearPending(tx);
		return false;
	}
	// Loop through index files to write the previous lengths
	CBPosition pos;
	CBAssociativeArrayGetFirst(&tx->indexes, &pos);
	for (;;) {
		CBDatabaseIndex * ind = pos.node->elements[pos.index];
		data[0] = ind->ID;
		CBInt16ToArray(data, 1, ind->lastFile);
		CBInt32ToArray(data, 3, ind->lastSize);
		if (NOT CBFileAppend(self->logFile, data, 7)) {
			CBLogError("Could not write the previous size information for an index.");
			CBFileClose(self->logFile);
			CBDatabaseClearPending(tx);
			return false;
		}
		if (CBAssociativeArrayIterate(&tx->indexes, &pos))
			break;
	}
	// Sync log file, so that it is now active with the information of the previous file sizes.
	if (NOT CBFileSync(self->logFile)){
		CBLogError("Failed to sync the log file");
		CBFileClose(self->logFile);
		CBDatabaseClearPending(tx);
		return false;
	}
	// Sync directory for log file
	if (NOT CBFileSyncDir(self->dataDir)) {
		CBLogError("Failed to synchronise the directory during a commit for the log file.");
		CBFileClose(self->logFile);
		CBDatabaseClearPending(tx);
		return false;
	}
	uint32_t lastFileSize = self->lastSize;
	uint32_t prevDeletionEntryNum = self->numDeletionValues;
	// Go through each key
	// Get the first key with an interator
	CBPosition it;
	if (CBAssociativeArrayGetFirst(&tx->valueWrites, &it)) for (;;) {
		// Get the data for writing
		CBDatabaseIndex * index = *(CBDatabaseIndex **)it.node->elements[it.index];
		uint8_t * keyPtr = it.node->elements[it.index] + sizeof(index);
		uint32_t dataSize = *(uint32_t *)(keyPtr + index->keySize);
		uint8_t * dataPtr = keyPtr + index->keySize + 4;
		uint32_t offset = *(uint32_t *)(dataPtr + dataSize);
		// Check for key in index
		CBIndexFindResult res = CBDatabaseIndexFind(self, index, keyPtr);
		if (res.status == CB_DATABASE_INDEX_ERROR) {
			CBLogError("There was an error while searching an index for a key.");
			CBFileClose(self->logFile);
			CBDatabaseClearPending(tx);
			return false;
		}
		if (res.status == CB_DATABASE_INDEX_FOUND) {
			// Exists in index so we are overwriting data.
			// Get the index data
			CBIndexValue * indexValue = &res.nodeIfCached->node->elements[res.index];
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
					CBDatabaseClearPending(tx);
					return false;
				}
				if (indexValue->length > dataSize && offset == CB_OVERWRITE_DATA) {
					// Change indexed length and mark the remaining length as deleted
					if (NOT CBDatabaseAddDeletionEntry(self, indexValue->fileID, indexValue->pos + dataSize, indexValue->length - dataSize)){
						CBLogError("Failed to add a deletion entry when overwriting a previous value with a smaller one.");
						CBFileClose(self->logFile);
						CBDatabaseClearPending(tx);
						return false;
					}
					// Change the length of the data in the index.
					indexValue->length = dataSize;
					uint8_t newLength[4];
					CBInt32ToArray(newLength, 0, indexValue->length);
					if (NOT CBDatabaseAddOverwrite(self, CB_DATABASE_FILE_TYPE_INDEX, index->ID, res.location.indexFile, newLength, res.location.offset + 6 + index->keySize, 4)) {
						CBLogError("Failed to add an overwrite operation to write the new length of a value to the database index.");
						CBFileClose(self->logFile);
						CBDatabaseClearPending(tx);
						return false;
					}
				}
			}else{
				// We will mark the previous data as deleted and store data elsewhere, unless the data has been marked as deleted already (by CB_DELETED_VALUE length).
				if (indexValue->length != CB_DELETED_VALUE
					&& NOT CBDatabaseAddDeletionEntry(self, indexValue->fileID, indexValue->pos, indexValue->length)){
					CBLogError("Failed to add a deletion entry for an old value when replacing it with a larger one.");
					CBFileClose(self->logFile);
					CBDatabaseClearPending(tx);
					return false;
				}
				if (NOT CBDatabaseAddValue(self, dataSize, dataPtr, indexValue)) {
					CBLogError("Failed to add a value to the database with a previously exiting key.");
					CBFileClose(self->logFile);
					CBDatabaseClearPending(tx);
					return false;
				}
				// Write index
				uint8_t data[10];
				CBInt16ToArray(data, 0, indexValue->fileID);
				CBInt32ToArray(data, 2, indexValue->pos);
				CBInt32ToArray(data, 6, indexValue->length);
				if (NOT CBDatabaseAddOverwrite(self, CB_DATABASE_FILE_TYPE_INDEX, index->ID, res.location.indexFile, data, res.location.offset + index->keySize, 10)) {
					CBLogError("Failed to add an overwrite operation for updating the index for writting data in a new location.");
					CBFileClose(self->logFile);
					CBDatabaseClearPending(tx);
					return false;
				}
			}
		}else{
			// Does not exist in index, so we are creating new data
			// New index value data
			CBIndexValue * indexValue = malloc(sizeof(*indexValue));
			if (NOT indexValue) {
				CBLogError("Could not allocate memory for an index value.");
				CBFileClose(self->logFile);
				CBDatabaseClearPending(tx);
				return false;
			}
			if (NOT CBDatabaseAddValue(self, dataSize, dataPtr, indexValue)) {
				CBLogError("Failed to add a value to the database with a new key.");
				CBFileClose(self->logFile);
				CBDatabaseClearPending(tx);
				return false;
			}
			// Add key to index
			indexValue->key = malloc(index->keySize);
			if (NOT indexValue->key) {
				CBLogError("Failed to allocate memory for an index value key.");
				CBFileClose(self->logFile);
				CBDatabaseClearPending(tx);
				return false;
			}
			memcpy(indexValue->key, keyPtr, index->keySize);
			// Insert index element into the index
			if (NOT CBDatabaseIndexInsert(self, index, indexValue, &res)) {
				CBLogError("Failed to insert a new index element into the index.");
				CBFileClose(self->logFile);
				CBDatabaseClearPending(tx);
				return false;
			}
		}
		// Iterate to next key-value to write.
		if (CBAssociativeArrayIterate(&tx->valueWrites, &it))
			break;
	}
	// Loop through indexes to update the last index file information
	if (CBAssociativeArrayGetFirst(&tx->indexes, &it)) for (;;) {
		CBDatabaseIndex * index = it.node->elements[it.index];
		if (index->lastFile != index->newLastFile
			|| index->lastSize != index->newLastSize) {
			index->lastFile = index->newLastFile;
			index->lastSize = index->newLastSize;
			// Write to the index file
			CBInt16ToArray(data, 0, index->lastFile);
			CBInt32ToArray(data, 2, index->lastSize);
			if (NOT CBDatabaseAddOverwrite(self, CB_DATABASE_FILE_TYPE_INDEX, index->ID, 0, data, 0, 6)) {
				CBLogError("Failed to write the new last file information to an index.");
				CBFileClose(self->logFile);
				CBDatabaseClearPending(tx);
				return false;
			}
		}
		if (CBAssociativeArrayIterate(&tx->indexes, &it))
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
			CBDatabaseClearPending(tx);
			return false;
		}
	}
	// Delete values
	// Get first value to delete.
	if (CBAssociativeArrayGetFirst(&tx->deleteKeys, &it)) for (;;) {
		CBDatabaseIndex * index = *(CBDatabaseIndex **)it.node->elements[it.index];
		uint8_t * keyPtr = ((uint8_t *)it.node->elements[it.index]) + sizeof(index);
		// Find index entry
		CBIndexFindResult res = CBDatabaseIndexFind(self, index, keyPtr);
		if (res.status != CB_DATABASE_INDEX_FOUND) {
			CBLogError("There was an problem finding an index key-value for deletion.");
			CBFileClose(self->logFile);
			CBDatabaseClearPending(tx);
			return false;
		}
		if (NOT CBDatabaseIndexDelete(self, index, &res)){
			CBLogError("Failed to delete a key-value.");
			CBFileClose(self->logFile);
			CBDatabaseClearPending(tx);
			return false;
		}
		// Iterate to next key to delete.
		if (CBAssociativeArrayIterate(&tx->deleteKeys, &it))
			break;
	}
	if (prevDeletionEntryNum != self->numDeletionValues) {
		// New number of deletion entries.
		CBInt32ToArray(data, 0, self->numDeletionValues);
		if (NOT CBDatabaseAddOverwrite(self, CB_DATABASE_FILE_TYPE_DELETION_INDEX, 0, 0, data, 0, 4)) {
			CBLogError("Failed to update the number of entries for the deletion index file.");
			CBFileClose(self->logFile);
			CBDatabaseClearPending(tx);
			return false;
		}
	}
	// Change keys
	for (uint32_t x = 0; x < tx->numChangeKeys; x++) {
		CBDatabaseIndex * index = *(CBDatabaseIndex **)it.node->elements[it.index];
		uint8_t * oldKeyPtr = ((uint8_t *)it.node->elements[it.index]) + sizeof(index);
		uint8_t * newKeyPtr = oldKeyPtr + sizeof(index);
		// Find index entry
		CBIndexFindResult res = CBDatabaseIndexFind(self, index, oldKeyPtr);
		if (res.status != CB_DATABASE_INDEX_FOUND) {
			CBLogError("There was an problem finding an index key for changing.");
			CBFileClose(self->logFile);
			CBDatabaseClearPending(tx);
			return false;
		}
		CBIndexValue * indexValue = &res.nodeIfCached->node->elements[res.index];
		// Delete old key
		if (NOT CBDatabaseIndexDelete(self, index, &res)){
			CBLogError("Failed to delete a key-value when changing the key.");
			CBFileClose(self->logFile);
			CBDatabaseClearPending(tx);
			return false;
		}
		// Insert new key
		CBIndexValue * newIndexValue = malloc(sizeof(*indexValue));
		if (NOT newIndexValue) {
			CBLogError("Could not allocate memory for an index value for the new key when changing a key.");
			CBFileClose(self->logFile);
			CBDatabaseClearPending(tx);
			return false;
		}
		// Add key to index
		newIndexValue->key = malloc(index->keySize);
		if (NOT newIndexValue->key) {
			CBLogError("Failed to allocate memory for a new index value key when changing the key.");
			CBFileClose(self->logFile);
			CBDatabaseClearPending(tx);
			return false;
		}
		memcpy(newIndexValue->key, newKeyPtr, index->keySize);
		// Insert index element into the index
		if (NOT CBDatabaseIndexInsert(self, index, indexValue, &res)) {
			CBLogError("Failed to insert a new index element into the index with a new key for existing data.");
			CBFileClose(self->logFile);
			CBDatabaseClearPending(tx);
			return false;
		}
	}
	// Sync last used file and deletion index file.
	if ((NOT CBFileSync(self->fileObjectCache))
		|| NOT CBFileSync(self->deletionIndexFile)) {
		CBLogError("Failed to synchronise the files during a commit.");
		CBFileClose(self->logFile);
		CBDatabaseClearPending(tx);
		return false;
	}
	// Sync directory
	if (NOT CBFileSyncDir(self->dataDir)) {
		CBLogError("Failed to synchronise the directory during a commit.");
		CBFileClose(self->logFile);
		CBDatabaseClearPending(tx);
		return false;
	}
	// Now we are done, make the logfile inactive. Errors do not matter here.
	if (CBFileSeek(self->logFile, 0)) {
		data[0] = 0;
		if (CBFileOverwrite(self->logFile, data, 1))
			CBFileSync(self->logFile);
	}
	CBFileClose(self->logFile);
	CBDatabaseClearPending(tx);
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
	uint64_t logFile = CBFileOpen(filename, false);
	if (NOT logFile) {
		CBLogError("The log file for reading could not be opened.");
		return false;
	}
	// Check if the logfile is active or not.
	uint8_t data[13];
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
	if (NOT CBFileRead(logFile, data, 13)) {
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
	for (uint8_t x = 0; x < data[12]; x++) {
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
	for (uint32_t c = 14 + 7 * data[12]; c < logFileLen;) {
		// Read file type, index ID and file ID, offset and size
		if (NOT CBFileRead(logFile, data, 12)) {
			CBLogError("Could not read from the log file.");
			CBFileClose(logFile);
			return false;
		}
		uint32_t dataLen = CBArrayToInt32(data, 8);
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
		uint64_t file = CBFileOpen(filename, false);
		if (NOT file) {
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
		free(prevData);
		c += 12 + dataLen;
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
		res.found = ((uint8_t *)CBFindResultToPointer(res))[1]
		&& CBArrayToInt32BigEndian(((uint8_t *)CBFindResultToPointer(res)), 2) >= length;
	}else
		res.found = false;
	return res;
}
uint64_t CBDatabaseGetFile(CBDatabase * self, CBDatabaseFileType type, uint8_t indexID, uint16_t fileID){
	if (fileID == self->lastUsedFileID
		&& type == self->lastUsedFileType
		&& indexID == self->lastUsedFileIndexID)
		return self->fileObjectCache;
	else if (type == CB_DATABASE_FILE_TYPE_DELETION_INDEX)
		return self->deletionIndexFile;
	else {
		// Change last used file cache
		if (self->lastUsedFileType == CB_DATABASE_FILE_TYPE_NONE) {
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
		self->fileObjectCache = CBFileOpen(filename, access(filename, F_OK));
		if (NOT self->fileObjectCache) {
			CBLogError("Could not open file %s.", filename);
			return 0;
		}
		self->lastUsedFileID = fileID;
		self->lastUsedFileIndexID = indexID;
		self->lastUsedFileType = type;
		return self->fileObjectCache;
	}
}
uint32_t CBDatabaseGetLength(CBDatabase * self, CBDatabaseIndex * index, CBDatabaseTransaction * tx, uint8_t * key) {
	// Look in index for value
	CBIndexFindResult indexRes = CBDatabaseIndexFind(self, index, key);
	if (indexRes.status == CB_DATABASE_INDEX_ERROR) {
		CBLogError("There was an error when attempting to find a key in an index to get the data length.");
		return false;
	}
	if (indexRes.status == CB_DATABASE_INDEX_NOT_FOUND){
		// Look for value in valueWrites array
		CBFindResult res;
		res = CBAssociativeArrayFind(&tx->valueWrites, key);
		if (NOT res.found)
			return CB_DOESNT_EXIST;
		return CBArrayToInt32(((uint8_t *)CBFindResultToPointer(res)), *key + 1);
	}
	return indexRes.nodeIfCached->node->elements[indexRes.index].length;
}
bool CBDatabaseIndexDelete(CBDatabase * self, CBDatabaseIndex * index, CBIndexFindResult * res){
	// Create deletion entry for the data
	CBIndexValue * indexValue = &res->nodeIfCached->node->elements[res->index];
	if (NOT CBDatabaseAddDeletionEntry(self, indexValue->fileID, indexValue->pos, indexValue->length)) {
		CBLogError("Failed to create a deletion entry for a key-value.");
		return false;
	}
	// Make the length CB_DELETED_VALUE, signifying deletion of the data
	indexValue->length = CB_DELETED_VALUE;
	uint8_t data[4];
	CBInt32ToArray(data, 0, CB_DELETED_VALUE);
	if (NOT CBDatabaseAddOverwrite(self, CB_DATABASE_FILE_TYPE_INDEX, index->ID, res->location.indexFile, data, res->location.offset + index->keySize + 6, 4)) {
		CBLogError("Failed to overwrite the index entry's length with CB_DELETED_VALUE to signify deletion.");
		return false;
	}
	return true;
}
CBIndexFindResult CBDatabaseIndexFind(CBDatabase * self, CBDatabaseIndex * index, uint8_t * key){
	CBIndexFindResult result;
	CBIndexNodeLocation nodeLoc;
	CBIndexNode * nodePtr;
	CBIndexNode * diskNode = malloc(sizeof(*diskNode));
	if (NOT diskNode) {
		CBLogError("Could not allocate memory for a node loaded from disk");
		result.status = CB_DATABASE_INDEX_ERROR;
		return result;
	}
	nodeLoc.cached = true;
	nodeLoc.location.cachedNode = &index->indexCache;
	for (;;) {
		// Get the node
		if (nodeLoc.cached)
			nodePtr = nodeLoc.location.cachedNode;
		else{
			// Load the node from disk
			if (NOT CBDatabaseLoadIndexNode(self, index, diskNode, nodeLoc.location.diskNodeLoc.indexFile, nodeLoc.location.diskNodeLoc.offset)) {
				free(diskNode);
				CBLogError("Could not load an index node from the disk.");
				result.status = CB_DATABASE_INDEX_ERROR;
				return result;
			}
			nodePtr = diskNode;
		}
		// Do binary search on node
		result = CBDatabaseIndexNodeBinarySearch(index, nodePtr, key);
		if (result.status == CB_DATABASE_INDEX_FOUND){
			// Found the data on this node.
			if (nodeLoc.cached)
				free(diskNode);
			result.nodeIfCached->cached = nodeLoc.cached;
			return result;
		}else{
			// We have not found the data on this node, if there is a child go to the child.
			CBIndexNodeLocation * childLoc = &nodePtr->children[result.index];
			if (childLoc->cached
				|| (childLoc->location.diskNodeLoc.indexFile != 0
				    && childLoc->location.diskNodeLoc.offset != 0)) {
				if (NOT nodeLoc.cached)
					// The node was on disk, free the allocated keys
					for (uint8_t x = 0; x < diskNode->numElements; x++)
						free(diskNode->elements[x].key);
				// Child exists, move to it
				nodeLoc = *childLoc;
			}else{
				// The child doesn't exist. Return as not found.
				free(diskNode);
				return result;
			}
		}
	}
}
bool CBDatabaseIndexMoveElements(CBDatabase * self, CBDatabaseIndex * index, CBIndexNodeAndIfCached * dest, CBIndexNodeAndIfCached * source, uint8_t startPos, uint8_t endPos, uint8_t amount){
	uint8_t elSize = index->keySize + 10;
	uint16_t moveSize = amount * elSize;
	uint8_t data[moveSize];
	if (dest->cached){
		// Move elements for the cached destination node.
		memmove(dest->node->elements + endPos, source->node->elements + startPos, amount);
		// Create the data to write the moved elements.
		uint8_t * dataCursor = data;
		for (uint8_t x = 0; x < amount; x++) {
			CBInt16ToArray(dataCursor, 0, source->node->elements[x].fileID);
			CBInt32ToArray(dataCursor, 2, source->node->elements[x].length);
			CBInt32ToArray(dataCursor, 6, source->node->elements[x].pos);
			dataCursor += elSize;
		}
	}else{
		// Get the file for the source node.
		uint64_t file = CBDatabaseGetFile(self, CB_DATABASE_FILE_TYPE_INDEX, index->ID, source->node->indexFile);
		// Read the elements to move from the source node.
		if (NOT CBFileSeek(file, source->node->offset + 1 + startPos * elSize)
			|| NOT CBFileRead(file, data, moveSize)) {
			CBLogError("Could not read the index elements to be moved.");
			return false;
		}
	}
	// Write the elements to the destination node
	if (NOT CBDatabaseAddOverwrite(self, CB_DATABASE_FILE_TYPE_INDEX, index->ID, dest->node->indexFile, data, dest->node->offset + 1 + endPos * elSize, moveSize)){
		CBLogError("Could not write the index elements being moved.");
		return false;
	}
	return true;
}
bool CBDatabaseIndexInsert(CBDatabase * self, CBDatabaseIndex * index, CBIndexValue * indexVal, CBIndexFindResult * res){
	// See if we can insert data in this node
	if (res->nodeIfCached->node->numElements < CB_DATABASE_BTREE_ELEMENTS) {
		if (res->nodeIfCached->node->numElements > res->index){
			// Move up the elements and children
			if (NOT CBDatabaseIndexMoveElements(self, index, res->nodeIfCached, res->nodeIfCached, res->index, res->index + 1, res->nodeIfCached->numElements - res->index)) {
				CBLogError("Could not move elements up one in a node for inserting a new element.");
				return false;
			}
			memmove(pos.node->children + pos.index + 2, pos.node->children + pos.index + 1, (pos.node->numElements - pos.index) * sizeof(*pos.node->children));
		}
		// Copy over new key, data and children
		pos.node->elements[pos.index] = element;
		pos.node->children[pos.index + 1] = right;
		// Increase number of elements
		pos.node->numElements++;
		// The element has been added, we can stop here.
		return true;
	}else{
		// Nope, we need to split this node into two.
		CBBTreeNode * new = malloc(sizeof(*new));
		if (NOT new)
			return false;
		// Make both sides have half the order.
		new->numElements = CB_BTREE_HALF_ELEMENTS;
		pos.node->numElements = CB_BTREE_HALF_ELEMENTS;
		uint8_t * midKeyValue;
		if (pos.index >= CB_BTREE_HALF_ELEMENTS) {
			// Not in first child.
			if (pos.index == CB_BTREE_HALF_ELEMENTS) {
				// New key is median.
				memcpy(new->elements, pos.node->elements + CB_BTREE_HALF_ELEMENTS, CB_BTREE_HALF_ELEMENTS * sizeof(*new->elements));
				// Copy children
				memcpy(new->children + 1, pos.node->children + CB_BTREE_HALF_ELEMENTS + 1, CB_BTREE_HALF_ELEMENTS * sizeof(*new->children));
				// First child in right of the split becomes the right input node
				new->children[0] = right;
				// Middle value is the inserted value
				midKeyValue = element;
			}else{
				// In new child.
				// Copy over first part of the new child
				if (pos.index > CB_BTREE_HALF_ELEMENTS + 1)
					memcpy(new->elements, pos.node->elements + CB_BTREE_HALF_ELEMENTS + 1, (pos.index - CB_BTREE_HALF_ELEMENTS - 1) * sizeof(*new->elements));
				// Copy in data
				new->elements[pos.index - CB_BTREE_HALF_ELEMENTS - 1] = element;
				// Copy over the last part of the new child. It seems so simple to visualise but coding this is very hard...
				memcpy(new->elements + pos.index - CB_BTREE_HALF_ELEMENTS, pos.node->elements + pos.index, (CB_BTREE_ELEMENTS - pos.index) * sizeof(*new->elements));
				// Copy children (Can be the confusing part) o 0 i 1 ii 3 iii 4 iv
				memcpy(new->children, pos.node->children + CB_BTREE_HALF_ELEMENTS + 1, (pos.index - CB_BTREE_HALF_ELEMENTS) * sizeof(*new->children)); // Includes left as previously
				new->children[pos.index - CB_BTREE_HALF_ELEMENTS] = right;
				// Rest of the children.
				if (CB_BTREE_ELEMENTS > pos.index)
					memcpy(new->children + pos.index - CB_BTREE_HALF_ELEMENTS + 1, pos.node->children + pos.index + 1, (CB_BTREE_ELEMENTS - pos.index) * sizeof(*new->children));
				// Middle value
				midKeyValue = pos.node->elements[CB_BTREE_HALF_ELEMENTS];
			}
		}else{
			// In first child. This is the (supposedly) easy one.
			// Copy data to new child
			memcpy(new->elements, pos.node->elements + CB_BTREE_HALF_ELEMENTS, CB_BTREE_HALF_ELEMENTS * sizeof(*new->elements));
			// Insert key and data
			memmove(pos.node->elements + pos.index + 1, pos.node->elements + pos.index, (CB_BTREE_HALF_ELEMENTS - pos.index) * sizeof(*pos.node->elements));
			pos.node->elements[pos.index] = element;
			// Children...
			memcpy(new->children, pos.node->children + CB_BTREE_HALF_ELEMENTS, (CB_BTREE_HALF_ELEMENTS + 1) * sizeof(*new->children)); // OK
			if (CB_BTREE_HALF_ELEMENTS > 1 + pos.index)
				memmove(pos.node->children + pos.index + 2, pos.node->children + pos.index + 1, (CB_BTREE_HALF_ELEMENTS - pos.index - 1) * sizeof(*new->children));
			// Insert right to inserted area
			pos.node->children[pos.index + 1] = right; // OK
			// Middle value
			midKeyValue = pos.node->elements[CB_BTREE_HALF_ELEMENTS];
		}
		// Make the new node's children's parents reflect this new node
		if (new->children[0])
			for (uint8_t x = 0; x < CB_BTREE_HALF_ELEMENTS + 1; x++) {
				CBBTreeNode * child = new->children[x];
				child->parent = new;
			}
		// Move middle value to parent, if parent does not exist (ie. root) create new root.
		if (NOT pos.node->parent) {
			// Create new root
			self->root = malloc(sizeof(*self->root));
			if (NOT self)
				return false;
			self->root->numElements = 0;
			self->root->parent = NULL;
			pos.node->parent = self->root;
			self->root->children[0] = pos.node; // Make left the left split node.
		}
		// New node requires that the parent be set
		new->parent = pos.node->parent;
		// Find position of value in parent and then insert.
		CBFindResult res = CBBTreeNodeBinarySearch(pos.node->parent, midKeyValue, self->compareFunc);
		res.position.node = pos.node->parent;
		return CBAssociativeArrayInsert(self, midKeyValue, res.position, new);
	}
}
CBIndexFindResult CBDatabaseIndexNodeBinarySearch(CBDatabaseIndex * index, CBIndexCacheNode * node, uint8_t * key){
	CBIndexFindResult result;
	result.status = CB_DATABASE_INDEX_NOT_FOUND;
	if (NOT node->numElements){
		result.index = 0;
		result.location = *nodeLoc;
		return res;
	}
	uint8_t left = 0;
	uint8_t right = node->numElements - 1;
	int cmp;
	while (left <= right) {
		result.index = (right+left)/2;
		cmp = memcmp(key, node->elements[result.index].key, index->keySize);
		if (cmp == 0) {
			result.status = CB_DATABASE_INDEX_FOUND;
			result.node = *node;
			break;
		}else if (cmp > 0){
			if (NOT result.index)
				break;
			right = result.index - 1;
		}else
			left = result.index + 1;
	}
	if (cmp > 0)
		result.index++;
	return res;
}
bool CBDatabaseReadValue(CBDatabase * self, uint8_t * key, uint8_t * data, uint32_t dataSize, uint32_t offset){
	// First check for value writes
	CBFindResult res = CBAssociativeArrayFind(&self->valueWrites, key);
	if (NOT res.found) {
		// Look in index for value
		CBFindResult res = CBAssociativeArrayFind(&self->index, key);
		if (NOT res.found){
			CBLogError("Could not find a value for a key.");
			return false;
		}
		CBIndexValue * val = (CBIndexValue *)((uint8_t *)CBFindResultToPointer(res) + *key + 1);
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
	// Get the data, existing, yet to be written.
	uint8_t * existingData = CBFindResultToPointer(res);
	// Move along key size and 5 for the key and data size information
	existingData += *existingData + 5;
	// Copy the data into the buffer supplied to the function
	memcpy(data, existingData + offset, dataSize);
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
	uint8_t * keyPtr = malloc(*key + 9 + size);
	if (NOT keyPtr) {
		CBLogError("Failed to allocate memory for a value write element.");
		CBDatabaseClearPending(self);
		return false;
	}
	memcpy(keyPtr, key, *key + 1);
	uint32_t * intPtr = (uint32_t *)(keyPtr + *key + 1);
	*intPtr = size;
	uint8_t * dataPtr = (uint8_t *)(intPtr + 1);
	// Add data
	for (uint8_t x = 0; x < numDataParts; x++){
		memcpy(dataPtr, data[x], dataSize[x]);
		dataPtr += dataSize[x];
	}
	intPtr = (uint32_t *)dataPtr;
	*intPtr = CB_OVERWRITE_DATA;
	return CBDatabaseAddWriteValue(self, keyPtr);
}
bool CBDatabaseWriteValue(CBDatabase * self, uint8_t * key, uint8_t * data, uint32_t size){
	// Write element by overwriting previous data
	return CBDatabaseWriteValueSubSection(self, key, data, size, CB_OVERWRITE_DATA);
}
bool CBDatabaseWriteValueSubSection(CBDatabase * self, uint8_t * key, uint8_t * data, uint32_t size, uint32_t offset){
	// Create element
	uint8_t * keyPtr = malloc(*key + 9 + size);
	if (NOT keyPtr) {
		CBLogError("Failed to allocate memory for a value write element.");
		CBDatabaseClearPending(self);
		return false;
	}
	memcpy(keyPtr, key, *key + 1);
	uint32_t * intPtr = (uint32_t *)(keyPtr + *key + 1);
	*intPtr = size;
	uint8_t * dataPtr = (uint8_t *)(intPtr + 1);
	memcpy(dataPtr, data, size);
	intPtr = (uint32_t *)(dataPtr + size);
	*intPtr = offset;
	return CBDatabaseAddWriteValue(self, keyPtr);
}
