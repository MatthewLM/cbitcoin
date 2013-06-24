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
	try{
		if (access(filename, F_OK)
			&& (mkdir(filename, S_IRWXU | S_IRWXG)
				|| chmod(filename, S_IRWXU | S_IRWXG))){
			CBLogError("Could not create the folder for a database.");
			throw;
		}
		// Check data consistency
		if (NOT CBDatabaseEnsureConsistent(self)){
			CBLogError("The database is inconsistent and could not be recovered in CBNewDatabase");
			throw;
		}
		// Load deletion index
		if (NOT CBInitAssociativeArray(&self->deletionIndex, CBKeyCompare, free)){
			CBLogError("Could not initialise the database deletion index.");
			throw;
		}
		try{
			strcat(filename, "del.dat");
			// Check that the index exists
			if (NOT access(filename, F_OK)) {
				// Can read deletion index
				if (NOT CBDatabaseReadAndOpenDeletionIndex(self, filename)) {
					CBLogError("Could not load the database deletion index.");
					throw;
				}
			}else{
				// Else empty deletion index
				if (NOT CBDatabaseCreateDeletionIndex(self, filename)) {
					CBLogError("Could not create the database deletion index.");
					throw;
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
				CBDepObject file;
				if (NOT CBFileOpen(&file, filename, true)) {
					CBLogError("Could not open a new first data file.");
					throw;
				}
				if (NOT CBFileAppend(file, (uint8_t []){0,0,6,0,0,0}, 6)) {
					CBFileClose(file);
					CBLogError("Could not write the initial data for the first data file.");
					throw;
				}
				CBFileClose(file);
			}else{
				// The data file does exist.
				CBDepObject file;
				if (NOT CBFileOpen(&file, filename, false)) {
					CBLogError("Could not open the data file.");
					throw;
				}
				if (NOT CBFileRead(file, data, 6)) {
					CBFileClose(file);
					CBLogError("Could not read the last file and last file size from the first data file.");
					throw;
				}
				CBFileClose(file);
				self->lastFile = CBArrayToInt16(data, 0);
				self->lastSize = CBArrayToInt32(data, 2);
			}
		}catch
			CBFreeAssociativeArray(&self->deletionIndex);
		pass;
	}catch
		return false;
	stop;
	return true;
}
bool CBInitDatabaseTransaction(CBDatabaseTransaction * self){
	self->numChangeKeys = 0;
	self->changeKeys = NULL;
	self->numIndexes = 0;
	if (CBInitAssociativeArray(&self->deleteKeys, CBDeleteKeysCompare, free)){
		if (CBInitAssociativeArray(&self->valueWrites, CBValueWriteCompare, free)){
			if (CBInitAssociativeArray(&self->indexes, CBIndexCompare, NULL))
				return true;
			CBLogError("Could not initilaise the indexes array.");
			CBFreeAssociativeArray(&self->valueWrites);
		}else
			CBLogError("Could not initilaise the valueWrites array.");
		CBFreeAssociativeArray(&self->deleteKeys);
	}else
		CBLogError("Could not initilaise the deleteKeys array.");
	return false;
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
		if (NOT index->indexCache.cachedNode) {
			free(index);
			CBFileClose(indexFile);
			CBLogError("Could not allocate memory for the root index node.");
			return NULL;
		}
		if (NOT CBDatabaseLoadIndexNode(self, index, index->indexCache.cachedNode, index->indexCache.diskNodeLoc.indexFile, index->indexCache.diskNodeLoc.offset)) {
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
				if (NOT node->children[y].cachedNode) {
					CBFreeIndex(index);
					CBFileClose(indexFile);
					CBLogError("Could not allocate memory for a cached index node.");
					return NULL;
				}
				// Load the node.
				if (NOT CBDatabaseLoadIndexNode(self, index, node->children[y].cachedNode, indexID, offset)) {
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
bool CBDatabaseLoadIndexNode(CBDatabase * self, CBDatabaseIndex * index, CBIndexNode * node, uint16_t nodeFile, uint32_t nodeOffset){
	// Read the node from the disk.
	CBDepObject file;
	if (NOT CBDatabaseGetFile(self, &file, CB_DATABASE_FILE_TYPE_INDEX, index->ID, nodeFile)) {
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
		if (NOT section) {
			CBFileClose(self->deletionIndexFile);
			CBLogError("Could not allocate memory for deleted section value.");
			return false;
		}
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
		if (NOT CBAssociativeArrayInsert(&self->deletionIndex, section->key, CBAssociativeArrayFind(&self->deletionIndex, section->key).position, NULL)) {
			CBFileClose(self->deletionIndexFile);
			free(section);
			CBLogError("Could not insert entry to the database deletion index.");
			return false;
		}
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
void CBFreeDatabase(CBDatabase * self){
	CBFreeAssociativeArray(&self->deletionIndex);
	// Close files
	CBFileClose(self->deletionIndexFile);
	if (self->lastUsedFileType != CB_DATABASE_FILE_TYPE_NONE)
		CBFileClose(self->fileObjectCache);
	free(self);
}
void CBFreeDatabaseTransaction(CBDatabaseTransaction * self){
	// Free indexes array
	CBFreeAssociativeArray(&self->indexes);
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
			CBFreeIndexNode(node->children[x].cachedNode);
	free(node);
}
void CBFreeIndex(CBDatabaseIndex * index){
	CBFreeIndexNode(index->indexCache.cachedNode);
	free(index);
}
void CBFreeIndexFindResult(CBIndexFindResult res){
	if (NOT res.el.nodeLoc.cached) 
		CBFreeIndexNode(res.el.nodeLoc.cachedNode);
	for (uint8_t x = 0; x < res.el.parentStack.num; x++)
		if (NOT res.el.parentStack.nodes[x].cached)
			CBFreeIndexNode(res.el.parentStack.nodes[x].cachedNode);
}
CBCompare CBCompareIndexPtrAndData(void * el1, void * el2, uint8_t size1, uint8_t size2){
	if (size1 > size2)
		return CB_COMPARE_MORE_THAN;
	else if (size2 > size1)
		return CB_COMPARE_LESS_THAN;
	int res = memcmp(el1, el2, sizeof(CBDatabaseIndex *) + size1);
	if (res > 0)
		return CB_COMPARE_MORE_THAN;
	else if (res < 0)
		return CB_COMPARE_LESS_THAN;
	return CB_COMPARE_EQUAL;
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
	CBDepObject file;
	if (NOT CBDatabaseGetFile(self, &file, fileType, indexID, fileID)) {
		CBLogError("Could not get the data file for overwritting.");
		return false;
	}
	// Write to the log file the rollback information for the changes being made.
	uint32_t dataTempLen = dataLen + 12;
	uint8_t * dataTemp = malloc(dataTempLen);
	if (NOT dataTemp) {
		CBLogError("Could not allocate memory for temporary memory for writting to the transaction log file.");
		return false;
	}
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
bool CBDatabaseAddWriteValue(CBDatabaseTransaction * self, void * writeValue) {
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
		// Add index to the array of indexes if need be
		res = CBAssociativeArrayFind(&self->indexes, *(CBDatabaseIndex **)writeValue);
		if (NOT res.found) {
			// Add into the array
			if (NOT CBAssociativeArrayInsert(&self->indexes, *(CBDatabaseIndex **)writeValue, res.position, NULL)) {
				CBLogError("Failed to insert an index into the indices array.");
				CBDatabaseClearPending(self);
				return false;
			}
			self->numIndexes++;
		}
	}
	return true;
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
bool CBDatabaseChangeKey(CBDatabaseTransaction * self, CBDatabaseIndex * index, uint8_t * previousKey, uint8_t * newKey) {
	try{
		self->changeKeys = realloc(self->changeKeys, sizeof(*self->changeKeys) * (self->numChangeKeys + 1));
		if (NOT self->changeKeys) {
			CBLogError("Failed to reallocate memory for the key change array.");
			throw;
		}
		self->changeKeys[self->numChangeKeys] = malloc(sizeof(index) + index->keySize * 2);
		if (NOT self->changeKeys[self->numChangeKeys]) {
			CBLogError("Failed to allocate memory for the information for changing a key.");
			throw;
		}
		*(CBDatabaseIndex **)self->changeKeys[self->numChangeKeys] = index;
		memcpy(self->changeKeys[self->numChangeKeys] + sizeof(index), previousKey, index->keySize);
		memcpy(self->changeKeys[self->numChangeKeys] + sizeof(index) + index->keySize, newKey, index->keySize);
		self->numChangeKeys++;
		// Add index to the array of indexes if need be
		CBFindResult res = CBAssociativeArrayFind(&self->indexes, index);
		if (NOT res.found) {
			// Add into the array
			if (NOT CBAssociativeArrayInsert(&self->indexes, index, res.position, NULL)) {
				CBLogError("Failed to insert an index into the indices array.");
				throw;
			}
			self->numIndexes++;
		}
	}catch{
		CBDatabaseClearPending(self);
		return false;
	}
	stop;
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
	try{
		if (NOT CBFileOpen(&self->logFile, filename, true)) {
			CBLogError("The log file for overwritting could not be opened.");
			throw;
		}
		try{
			// Write the previous sizes for the files to the log file
			uint8_t data[12];
			data[0] = 1; // Log file active.
			uint32_t deletionIndexLen;
			if (NOT CBFileGetLength(self->deletionIndexFile, &deletionIndexLen)) {
				CBLogError("Could not get the length of the deletion index file.");
				throw;
			}
			CBInt32ToArray(data, 1, deletionIndexLen);
			CBInt16ToArray(data, 5, self->lastFile);
			CBInt32ToArray(data, 7, self->lastSize);
			data[11] = tx->numIndexes;
			if (NOT CBFileAppend(self->logFile, data, 12)) {
				CBLogError("Could not write previous size information to the log-file for the data and deletion index.");
				throw;
			}
			// Loop through index files to write the previous lengths
			CBPosition pos;
			if (CBAssociativeArrayGetFirst(&tx->indexes, &pos)) for (;;) {
				CBDatabaseIndex * ind = pos.node->elements[pos.index];
				data[0] = ind->ID;
				CBInt16ToArray(data, 1, ind->lastFile);
				CBInt32ToArray(data, 3, ind->lastSize);
				if (NOT CBFileAppend(self->logFile, data, 7)) {
					CBLogError("Could not write the previous size information for an index.");
					throw;
				}
				if (CBAssociativeArrayIterate(&tx->indexes, &pos))
					break;
			}
			// Sync log file, so that it is now active with the information of the previous file sizes.
			if (NOT CBFileSync(self->logFile)){
				CBLogError("Failed to sync the log file");
				throw;
			}
			// Sync directory for log file
			if (NOT CBFileSyncDir(self->dataDir)) {
				CBLogError("Failed to synchronise the directory during a commit for the log file.");
				throw;
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
					throw;
				}
				try{
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
								throw;
							}
							if (indexValue->length > dataSize && offset == CB_OVERWRITE_DATA) {
								// Change indexed length and mark the remaining length as deleted
								if (NOT CBDatabaseAddDeletionEntry(self, indexValue->fileID, indexValue->pos + dataSize, indexValue->length - dataSize)){
									CBLogError("Failed to add a deletion entry when overwriting a previous value with a smaller one.");
									throw;
								}
								// Change the length of the data in the index.
								indexValue->length = dataSize;
								uint8_t newLength[4];
								CBInt32ToArray(newLength, 0, indexValue->length);
								if (NOT CBDatabaseAddOverwrite(self, CB_DATABASE_FILE_TYPE_INDEX, index->ID, res.el.nodeLoc.diskNodeLoc.indexFile, newLength, res.el.nodeLoc.diskNodeLoc.offset + 3 + (10+index->keySize)*res.el.index, 4)) {
									CBLogError("Failed to add an overwrite operation to write the new length of a value to the database index.");
									throw;
								}
							}
						}else{
							// We will mark the previous data as deleted and store data elsewhere, unless the data has been marked as deleted already (by CB_DELETED_VALUE length).
							if (indexValue->length != CB_DELETED_VALUE
								&& NOT CBDatabaseAddDeletionEntry(self, indexValue->fileID, indexValue->pos, indexValue->length)){
								CBLogError("Failed to add a deletion entry for an old value when replacing it with a larger one.");
								throw;
							}
							if (NOT CBDatabaseAddValue(self, dataSize, dataPtr, indexValue)) {
								CBLogError("Failed to add a value to the database with a previously exiting key.");
								throw;
							}
							// Write index
							uint8_t data[10];
							CBInt16ToArray(data, 0, indexValue->fileID);
							CBInt32ToArray(data, 2, indexValue->length);
							CBInt32ToArray(data, 6, indexValue->pos);
							if (NOT CBDatabaseAddOverwrite(self, CB_DATABASE_FILE_TYPE_INDEX, index->ID, res.el.nodeLoc.diskNodeLoc.indexFile, data, res.el.nodeLoc.diskNodeLoc.offset + 1 + (10+index->keySize)*res.el.index, 10)) {
								CBLogError("Failed to add an overwrite operation for updating the index for writting data in a new location.");
								throw;
							}
						}
					}else{
						// Does not exist in index, so we are creating new data
						// New index value data
						CBIndexValue * indexValue = malloc(sizeof(*indexValue));
						if (NOT indexValue) {
							CBLogError("Could not allocate memory for an index value.");
							throw;
						}
						try{
							if (NOT CBDatabaseAddValue(self, dataSize, dataPtr, indexValue)) {
								CBLogError("Failed to add a value to the database with a new key.");
								throw;
							}
							// Add key to index
							indexValue->key = malloc(index->keySize);
							if (NOT indexValue->key) {
								CBLogError("Failed to allocate memory for an index value key.");
								throw;
							}
							memcpy(indexValue->key, keyPtr, index->keySize);
							// Insert index element into the index
							if (NOT CBDatabaseIndexInsert(self, index, indexValue, &res.el, NULL)) {
								CBLogError("Failed to insert a new index element into the index.");
								throw;
							}
						}catch{
							free(indexValue);
						}
						pass;
					}
				}
				CBFreeIndexFindResult(res);
				pass;
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
						throw;
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
					throw;
				}
			}
			// Delete values
			if (CBAssociativeArrayGetFirst(&tx->deleteKeys, &it)) for (;;) {
				CBDatabaseIndex * index = *(CBDatabaseIndex **)it.node->elements[it.index];
				uint8_t * keyPtr = ((uint8_t *)it.node->elements[it.index]) + sizeof(index);
				// Find index entry
				CBIndexFindResult res = CBDatabaseIndexFind(self, index, keyPtr);
				if (res.status == CB_DATABASE_INDEX_ERROR) {
					CBLogError("There was an error attempting to find an index key-value for deletion.");
					throw;
				}
				try{
					if (res.status == CB_DATABASE_INDEX_NOT_FOUND) {
						CBLogError("Could not find an index key-value for deletion.");
						throw;
					}
					if (NOT CBDatabaseIndexDelete(self, index, &res)){
						CBLogError("Failed to delete a key-value.");
						throw;
					}
				}
				CBFreeIndexFindResult(res);
				pass;
				// Iterate to next key to delete.
				if (CBAssociativeArrayIterate(&tx->deleteKeys, &it))
					break;
			}
			// Change keys
			for (uint32_t x = 0; x < tx->numChangeKeys; x++) {
				CBDatabaseIndex * index = *(CBDatabaseIndex **)tx->changeKeys[x];
				uint8_t * oldKeyPtr = ((uint8_t *)tx->changeKeys[x]) + sizeof(index);
				uint8_t * newKeyPtr = oldKeyPtr + index->keySize;
				// Find index entry
				CBIndexFindResult res = CBDatabaseIndexFind(self, index, oldKeyPtr);
				if (res.status == CB_DATABASE_INDEX_ERROR) {
					CBLogError("There was an error while attempting to find an index key for changing.");
					throw;
				}
				try{
					CBIndexValue * indexValue = &res.el.nodeLoc.cachedNode->elements[res.el.index];
					// Allocate and assign new index entry
					CBIndexValue * newIndexValue = malloc(sizeof(*indexValue));
					if (NOT newIndexValue) {
						CBLogError("Could not allocate memory for an index value for the new key when changing a key.");
						throw;
					}
					try{
						newIndexValue->fileID = indexValue->fileID;
						newIndexValue->length = indexValue->length;
						newIndexValue->pos = indexValue->pos;
						// Delete old key
						indexValue->length = CB_DELETED_VALUE;
						CBInt32ToArray(data, 0, CB_DELETED_VALUE);
						if (NOT CBDatabaseAddOverwrite(self, CB_DATABASE_FILE_TYPE_INDEX, index->ID, res.el.nodeLoc.diskNodeLoc.indexFile, data, res.el.nodeLoc.diskNodeLoc.offset + 3 + (index->keySize + 10)*res.el.index, 4)) {
							CBLogError("Failed to overwrite the index entry's length with CB_DELETED_VALUE to signify deletion when changing a key.");
							throw;
						}
						// Add new key to index
						newIndexValue->key = malloc(index->keySize);
						if (NOT newIndexValue->key) {
							CBLogError("Failed to allocate memory for a new index value key when changing the key.");
							throw;
						}
						try{
							memcpy(newIndexValue->key, newKeyPtr, index->keySize);
							// Find position for index entry
							CBFreeIndexFindResult(res);
							res = CBDatabaseIndexFind(self, index, newKeyPtr);
							if (res.status == CB_DATABASE_INDEX_ERROR) {
								CBLogError("Failed to find the position for a new key replacing an old one.");
								throw;
							}
							if (res.status == CB_DATABASE_INDEX_FOUND) {
								// The index entry already exists. Get the index value
								CBIndexValue * indexValue = &res.el.nodeLoc.cachedNode->elements[res.el.index];
								if (indexValue->length != CB_DELETED_VALUE) {
									// Delete existing data this new key was pointing to
									if (NOT CBDatabaseAddDeletionEntry(self, indexValue->fileID, indexValue->pos, indexValue->length)) {
										CBLogError("Failed to create a deletion entry for a value, the key thereof was changed to point to other data.");
										throw;
									}
								}
								// Replace the index value to point to the new data
								indexValue->fileID = newIndexValue->fileID;
								indexValue->length = newIndexValue->length;
								indexValue->pos = newIndexValue->pos;
								CBInt16ToArray(data, 0, indexValue->fileID);
								CBInt32ToArray(data, 2, indexValue->length);
								CBInt32ToArray(data, 6, indexValue->pos);
								if (NOT CBDatabaseAddOverwrite(self, CB_DATABASE_FILE_TYPE_INDEX, index->ID, res.el.nodeLoc.diskNodeLoc.indexFile, data, res.el.nodeLoc.diskNodeLoc.offset + 1 + (index->keySize + 10)*res.el.index, 10)) {
									throw;
								}
							// Else insert index element into the index
							}else if (NOT CBDatabaseIndexInsert(self, index, newIndexValue, &res.el, NULL)) {
								CBLogError("Failed to insert a new index element into the index with a new key for existing data.");
								throw;
							}
						}catch
							free(newIndexValue->key);
						pass;
					}catch
						free(newIndexValue);
					pass;
				}
				CBFreeIndexFindResult(res);
				pass;
			}
			if (prevDeletionEntryNum != self->numDeletionValues) {
				// New number of deletion entries.
				CBInt32ToArray(data, 0, self->numDeletionValues);
				if (NOT CBDatabaseAddOverwrite(self, CB_DATABASE_FILE_TYPE_DELETION_INDEX, 0, 0, data, 0, 4)) {
					CBLogError("Failed to update the number of entries for the deletion index file.");
					throw;
				}
			}
			// Sync last used file and deletion index file.
			if ((NOT CBFileSync(self->fileObjectCache))
				|| NOT CBFileSync(self->deletionIndexFile)) {
				CBLogError("Failed to synchronise the files during a commit.");
				throw;
			}
			// Sync directory
			if (NOT CBFileSyncDir(self->dataDir)) {
				CBLogError("Failed to synchronise the directory during a commit.");
				throw;
			}
			// Now we are done, make the logfile inactive. Errors do not matter here.
			if (CBFileSeek(self->logFile, 0)) {
				data[0] = 0;
				if (CBFileOverwrite(self->logFile, data, 1))
					CBFileSync(self->logFile);
			}
		}
		CBFileClose(self->logFile);
		pass;
	}
	CBDatabaseClearPending(tx);
	catch
		return false;
	stop;
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
		return true;;
	}
}
uint32_t CBDatabaseGetLength(CBDatabase * self, CBDatabaseTransaction * tx, CBDatabaseIndex * index, uint8_t * key) {
	// Look in index for value
	CBIndexFindResult indexRes = CBDatabaseIndexFind(self, index, key);
	if (indexRes.status == CB_DATABASE_INDEX_ERROR) {
		CBLogError("There was an error when attempting to find a key in an index to get the data length.");
		return false;
	}
	if (indexRes.status == CB_DATABASE_INDEX_NOT_FOUND){
		CBFreeIndexFindResult(indexRes);
		// Look for value in valueWrites array
		CBFindResult res;
		res = CBAssociativeArrayFind(&tx->valueWrites, key);
		if (NOT res.found)
			return CB_DOESNT_EXIST;
		return CBArrayToInt32(((uint8_t *)CBFindResultToPointer(res)), *key + 1);
	}
	uint32_t ret = indexRes.el.nodeLoc.cachedNode->elements[indexRes.el.index].length;
	CBFreeIndexFindResult(indexRes);
	return ret;
}
bool CBDatabaseIndexDelete(CBDatabase * self, CBDatabaseIndex * index, CBIndexFindResult * res){
	// Create deletion entry for the data
	CBIndexValue * indexValue = &res->el.nodeLoc.cachedNode->elements[res->el.index];
	if (NOT CBDatabaseAddDeletionEntry(self, indexValue->fileID, indexValue->pos, indexValue->length)) {
		CBLogError("Failed to create a deletion entry for a key-value.");
		return false;
	}
	// Make the length CB_DELETED_VALUE, signifying deletion of the data
	indexValue->length = CB_DELETED_VALUE;
	uint8_t data[4];
	CBInt32ToArray(data, 0, CB_DELETED_VALUE);
	if (NOT CBDatabaseAddOverwrite(self, CB_DATABASE_FILE_TYPE_INDEX, index->ID, res->el.nodeLoc.diskNodeLoc.indexFile, data, res->el.nodeLoc.diskNodeLoc.offset + 3 + (index->keySize + 10)*res->el.index, 4)) {
		CBLogError("Failed to overwrite the index entry's length with CB_DELETED_VALUE to signify deletion.");
		return false;
	}
	return true;
}
CBIndexFindResult CBDatabaseIndexFind(CBDatabase * self, CBDatabaseIndex * index, uint8_t * key){
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
					if (NOT nodeLoc.cachedNode
						|| NOT CBDatabaseLoadIndexNode(self, index, nodeLoc.cachedNode, nodeLoc.diskNodeLoc.indexFile, nodeLoc.diskNodeLoc.offset)) {
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
bool CBDatabaseIndexInsert(CBDatabase * self, CBDatabaseIndex * index, CBIndexValue * indexVal, CBIndexElementPos * pos, CBIndexNodeLocation * right){
	bool bottom = pos->nodeLoc.cachedNode->children[0].diskNodeLoc.indexFile == 0 && pos->nodeLoc.cachedNode->children[0].diskNodeLoc.offset == 0;
	// See if we can insert data in this node
	if (pos->nodeLoc.cachedNode->numElements < CB_DATABASE_BTREE_ELEMENTS) {
		if (pos->nodeLoc.cachedNode->numElements > pos->index){
			// Move up the elements and children
			if (NOT CBDatabaseIndexMoveElements(self, index, &pos->nodeLoc, &pos->nodeLoc, pos->index, pos->index + 1, pos->nodeLoc.cachedNode->numElements - pos->index, false)
				|| (NOT bottom && NOT CBDatabaseIndexMoveChildren(self, index, &pos->nodeLoc, &pos->nodeLoc, pos->index + 1, pos->index + 2, pos->nodeLoc.cachedNode->numElements - pos->index, false))) {
				CBLogError("Could not move elements and children up one in a node for inserting a new element.");
				return false;
			}
		}
		// Insert element and right child
		if (NOT CBDatabaseIndexSetElement(self, index, &pos->nodeLoc, indexVal, pos->index, false)
			|| (right && NOT CBDatabaseIndexSetChild(self, index, &pos->nodeLoc, right, pos->index + 1, false))){
			CBLogError("Could not set an element and child for adding to an unfull index node.");
			return false;
		}
		// Increase number of elements
		if (NOT CBDatabaseIndexSetNumElements(self, index, &pos->nodeLoc, pos->nodeLoc.cachedNode->numElements + 1, false)){
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
			if (NOT new.cachedNode) {
				CBLogError("Could not allocate memory for a new cached node.");
				return false;
			}
			index->numCached++;
		}else
			new.cached = false;
		// Get position for new node
		CBDatabaseIndexSetNewNodePosition(index, &new);
		// Make both sides have half the order.
		if (NOT CBDatabaseIndexSetNumElements(self, index, &pos->nodeLoc, CB_DATABASE_BTREE_HALF_ELEMENTS, false)
			|| NOT CBDatabaseIndexSetNumElements(self, index, &new, CB_DATABASE_BTREE_HALF_ELEMENTS, true)) {
			CBLogError("Could not set two split nodes ot have the number of elements equal to half the order.");
			return false;
		}
		CBIndexValue middleEl;
		// Check where to insert new element
		if (pos->index > CB_DATABASE_BTREE_HALF_ELEMENTS) {
			// In the new node.
			if (
				// Copy over first part of the new child
				NOT CBDatabaseIndexMoveElements(self, index, &new, &pos->nodeLoc, CB_DATABASE_BTREE_HALF_ELEMENTS + 1, 0, pos->index - CB_DATABASE_BTREE_HALF_ELEMENTS - 1, true)
				// Copy over the element
				|| NOT CBDatabaseIndexSetElement(self, index, &new, indexVal, pos->index - CB_DATABASE_BTREE_HALF_ELEMENTS - 1, true)
				// Copy remainder of elements
				|| NOT CBDatabaseIndexMoveElements(self, index, &new, &pos->nodeLoc, pos->index, pos->index - CB_DATABASE_BTREE_HALF_ELEMENTS, CB_DATABASE_BTREE_ELEMENTS - pos->index, true)
				// Set remainder of elements as blank
				|| NOT CBDatabaseAppendZeros(self, CB_DATABASE_FILE_TYPE_INDEX, index->ID, new.diskNodeLoc.indexFile, CB_DATABASE_BTREE_HALF_ELEMENTS * (index->keySize + 10))) {
				CBLogError("Could not move elements including the new element into the new node");
				return false;
			}
			if (NOT bottom
				&& (NOT CBDatabaseIndexMoveChildren(self, index, &new, &pos->nodeLoc, CB_DATABASE_BTREE_HALF_ELEMENTS + 1, 0, pos->index - CB_DATABASE_BTREE_HALF_ELEMENTS, true)
					// Set the right child
					|| NOT CBDatabaseIndexSetChild(self, index, &new, right, pos->index - CB_DATABASE_BTREE_HALF_ELEMENTS, true)
					// Set the remaining children
					|| NOT CBDatabaseIndexMoveChildren(self, index, &new, &pos->nodeLoc, pos->index + 1, pos->index - CB_DATABASE_BTREE_HALF_ELEMENTS + 1, CB_DATABASE_BTREE_ELEMENTS - pos->index, true))){
					CBLogError("Could not insert children into new node.");
					return false;
				}
			// Set middle element
			middleEl = pos->nodeLoc.cachedNode->elements[CB_DATABASE_BTREE_HALF_ELEMENTS];
		}else if (pos->index == CB_DATABASE_BTREE_HALF_ELEMENTS) {
			// Middle
			if (
				// Copy elements
				NOT CBDatabaseIndexMoveElements(self, index, &new, &pos->nodeLoc, CB_DATABASE_BTREE_HALF_ELEMENTS, 0, CB_DATABASE_BTREE_HALF_ELEMENTS, true)
				// Set remaining elements as blank
				|| NOT CBDatabaseAppendZeros(self, CB_DATABASE_FILE_TYPE_INDEX, index->ID, new.diskNodeLoc.indexFile, CB_DATABASE_BTREE_HALF_ELEMENTS * (index->keySize + 10))){
				CBLogError("Could not move elements into the new node, with the new element being the middle element.");
				return false;
			}
			if (NOT bottom
				&& (// Set right child
					NOT CBDatabaseIndexSetChild(self, index, &new, right, 0, true)
					// Copy children
					|| NOT CBDatabaseIndexMoveChildren(self, index, &new, &pos->nodeLoc, CB_DATABASE_BTREE_HALF_ELEMENTS + 1, 1, CB_DATABASE_BTREE_HALF_ELEMENTS, true))) {
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
				NOT CBDatabaseIndexMoveElements(self, index, &new, &pos->nodeLoc, CB_DATABASE_BTREE_HALF_ELEMENTS, 0, CB_DATABASE_BTREE_HALF_ELEMENTS, true)
				// Set remaining elements as blank
				|| NOT CBDatabaseAppendZeros(self, CB_DATABASE_FILE_TYPE_INDEX, index->ID, new.diskNodeLoc.indexFile, CB_DATABASE_BTREE_HALF_ELEMENTS * (index->keySize + 10))
				// Move elements to make room for new element
				|| NOT CBDatabaseIndexMoveElements(self, index, &pos->nodeLoc, &pos->nodeLoc, pos->index, pos->index + 1, CB_DATABASE_BTREE_HALF_ELEMENTS - pos->index, false)
				// Move new element into old node
				|| NOT CBDatabaseIndexSetElement(self, index, &pos->nodeLoc, indexVal, pos->index, false)) {
				CBLogError("Could not insert the new element into the old node and move elements to the new node.");
				return false;
			}
			if (NOT bottom
				&& (// Move children in old node to make room for the right child
					NOT CBDatabaseIndexMoveChildren(self, index, &pos->nodeLoc, &pos->nodeLoc, pos->index + 1, pos->index + 2, CB_DATABASE_BTREE_HALF_ELEMENTS - pos->index - 1, false)
					// Insert the right child
					|| NOT CBDatabaseIndexSetChild(self, index, &pos->nodeLoc, right, pos->index + 1, false)
					// Copy children into new node
					|| NOT CBDatabaseIndexMoveChildren(self, index, &new, &pos->nodeLoc, CB_DATABASE_BTREE_HALF_ELEMENTS, 0, CB_DATABASE_BTREE_HALF_ELEMENTS + 1, true))) {
					CBLogError("Could not insert the children to the new node when the new element is in the first node.");
					return false;
				}
		}
		if (bottom){
			// If in bottom make all children blank.
			if (NOT CBDatabaseIndexSetBlankChildren(self, index, &new, 0, CB_DATABASE_BTREE_ELEMENTS + 1)) {
				CBLogError("Could not insert blank children into the new bottom node.");
				return false;
			}
			// Else set only the last CB_DATABASE_BTREE_HALF_ELEMENTS as blank
		}else if (NOT CBDatabaseIndexSetBlankChildren(self, index, &new, CB_DATABASE_BTREE_HALF_ELEMENTS, CB_DATABASE_BTREE_HALF_ELEMENTS)){
			CBLogError("Could not insert blank children into the new not bottom node.");
			return false;
		}
		// If the root was split, create a new root
		if (pos->nodeLoc.cachedNode == index->indexCache.cachedNode) {
			index->numCached++;
			CBIndexNode * newRootCache = malloc(sizeof(*newRootCache));
			if (NOT newRootCache) {
				CBLogError("Could not create a new root node");
				return false;
			}
			index->indexCache.cachedNode = newRootCache;
			// Write the new root node to disk
			CBDatabaseIndexSetNewNodePosition(index, &index->indexCache);
			// Write number of elements which is one
			if (NOT CBDatabaseIndexSetNumElements(self, index, &index->indexCache, 1, true)) {
				CBLogError("Could not write the number of elements to a new root node.");
				return false;
			}
			// Write the middle element
			if (NOT CBDatabaseIndexSetElement(self, index, &index->indexCache, &middleEl, 0, true)) {
				CBLogError("Could not set the middle element to a new root node.");
				return false;
			}
			// Write blank elements
			uint16_t elementsSize = (10+index->keySize)*(CB_DATABASE_BTREE_ELEMENTS-1);
			if (NOT CBDatabaseAppendZeros(self, CB_DATABASE_FILE_TYPE_INDEX, index->ID, index->newLastFile, elementsSize)) {
				CBLogError("Could not write the blank elements for a new root node.");
				return false;
			}
			// Write left child
			if (NOT CBDatabaseIndexSetChild(self, index, &index->indexCache, &pos->nodeLoc, 0, true)) {
				CBLogError("Could not write the left child (old root) for a new root node.");
				return false;
			}
			// Write right child
			if (NOT CBDatabaseIndexSetChild(self, index, &index->indexCache, &new, 1, true)) {
				CBLogError("Could not write the right child (new node) for a new root node.");
				return false;
			}
			// Write blank children
			if (NOT CBDatabaseIndexSetBlankChildren(self, index,&index->indexCache, 2, CB_DATABASE_BTREE_ELEMENTS-1)) {
				CBLogError("Could not write the blank children for a new root node.");
				return false;
			}
			uint8_t data[6];
			// Overwrite root information in index
			CBInt16ToArray(data, 0, index->indexCache.diskNodeLoc.indexFile);
			CBInt32ToArray(data, 2, index->indexCache.diskNodeLoc.offset);
			if (NOT CBDatabaseAddOverwrite(self, CB_DATABASE_FILE_TYPE_INDEX, index->ID, 0, data, 6, 6)) {
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
		return CBDatabaseIndexInsert(self, index, &middleEl, pos, &new);
	}
}
bool CBDatabaseIndexMoveChildren(CBDatabase * self, CBDatabaseIndex * index, CBIndexNodeLocation * dest, CBIndexNodeLocation * source, uint8_t startPos, uint8_t endPos, uint8_t amount, bool append){
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
		if (NOT CBDatabaseAppend(self, CB_DATABASE_FILE_TYPE_INDEX, index->ID, dest->diskNodeLoc.indexFile, data, 6 * amount)){
			CBLogError("Could not write the index children being moved by appending.");
			return false;
		}
	} else if (NOT CBDatabaseAddOverwrite(self, CB_DATABASE_FILE_TYPE_INDEX, index->ID, dest->diskNodeLoc.indexFile, data, dest->diskNodeLoc.offset + 1 + CB_DATABASE_BTREE_ELEMENTS * (index->keySize + 10) + endPos * 6, 6 * amount)){
		CBLogError("Could not write the index children being moved.");
		return false;
	}
	if (dest->cached)
		// Move children for the cached destination node.
		memmove(dest->cachedNode->children + endPos, source->cachedNode->children + startPos, amount * sizeof(*source->cachedNode->children));
	return true;
}
bool CBDatabaseIndexMoveElements(CBDatabase * self, CBDatabaseIndex * index, CBIndexNodeLocation * dest, CBIndexNodeLocation * source, uint8_t startPos, uint8_t endPos, uint8_t amount, bool append){
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
		if (NOT CBDatabaseAppend(self, CB_DATABASE_FILE_TYPE_INDEX, index->ID, dest->diskNodeLoc.indexFile, data, moveSize)){
			CBLogError("Could not write the index elements being moved by appending.");
			return false;
		}
	}else if (NOT CBDatabaseAddOverwrite(self, CB_DATABASE_FILE_TYPE_INDEX, index->ID, dest->diskNodeLoc.indexFile, data, dest->diskNodeLoc.offset + 1 + endPos * elSize, moveSize)){
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
bool CBDatabaseIndexSetBlankChildren(CBDatabase * self, CBDatabaseIndex * index, CBIndexNodeLocation * dest, uint8_t offset, uint8_t amount){
	if (NOT CBDatabaseAppendZeros(self, CB_DATABASE_FILE_TYPE_INDEX, index->ID, dest->diskNodeLoc.indexFile, amount * 6)) {
		CBLogError("Could not append blank children to the new node.");
		return false;
	}
	if (dest->cached && offset == 0)
		memset(dest->cachedNode->children, 0, sizeof(*dest->cachedNode->children));
	return true;
}
bool CBDatabaseIndexSetChild(CBDatabase * self, CBDatabaseIndex * index, CBIndexNodeLocation * node, CBIndexNodeLocation * child, uint8_t pos, bool append){
	if (node->cached)
		// Set the child for the cached node.
		node->cachedNode->children[pos] = *child;
	// Write the child to the node on disk.
	uint8_t data[6];
	CBInt16ToArray(data, 0, child->diskNodeLoc.indexFile);
	CBInt32ToArray(data, 2, child->diskNodeLoc.offset);
	if (append) {
		if (NOT CBDatabaseAppend(self, CB_DATABASE_FILE_TYPE_INDEX, index->ID, node->diskNodeLoc.indexFile, data, 6)){
			CBLogError("Could not set a child on a node on disk by appending.");
			return false;
		}
	}else if (NOT CBDatabaseAddOverwrite(self, CB_DATABASE_FILE_TYPE_INDEX, index->ID, node->diskNodeLoc.indexFile, data, node->diskNodeLoc.offset + 1 + CB_DATABASE_BTREE_ELEMENTS * (index->keySize + 10) + pos * 6, 6)){
		CBLogError("Could not set a child on a node on disk.");
		return false;
	}
	return true;
}
bool CBDatabaseIndexSetElement(CBDatabase * self, CBDatabaseIndex * index, CBIndexNodeLocation * node, CBIndexValue * element, uint8_t pos, bool append){
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
		if (NOT CBDatabaseAppend(self, CB_DATABASE_FILE_TYPE_INDEX, index->ID, node->diskNodeLoc.indexFile, data, elSize)){
			CBLogError("Could not write an index element being added to a node on disk by appending.");
			return false;
		}
	}else if (NOT CBDatabaseAddOverwrite(self, CB_DATABASE_FILE_TYPE_INDEX, index->ID, node->diskNodeLoc.indexFile, data, node->diskNodeLoc.offset + 1 + pos * elSize, elSize)){
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
bool CBDatabaseIndexSetNumElements(CBDatabase * self, CBDatabaseIndex * index, CBIndexNodeLocation * node, uint8_t numElements, bool append){
	if (node->cached)
		node->cachedNode->numElements = numElements;
	if (append) {
		if (NOT CBDatabaseAppend(self, CB_DATABASE_FILE_TYPE_INDEX, index->ID, node->diskNodeLoc.indexFile, &numElements, 1)) {
			CBLogError("Could not update the number of elements in storage for an index node by appending.");
			return false;
		}
	}else if (NOT CBDatabaseAddOverwrite(self, CB_DATABASE_FILE_TYPE_INDEX, index->ID, node->diskNodeLoc.indexFile, &numElements, node->diskNodeLoc.offset, 1)) {
		CBLogError("Could not update the number of elements in storage for an index node.");
		return false;
	}
	return true;
}
CBIndexFindStatus CBDatabaseReadValue(CBDatabase * self, CBDatabaseTransaction * transaction, CBDatabaseIndex * index, uint8_t * key, uint8_t * data, uint32_t dataSize, uint32_t offset){
	CBFindResult res;
	if (transaction != NULL){
		// First check for value writes
		transaction->valueWrites.compareFunc = CBDeleteKeysCompare;
		uint8_t compareData[index->keySize + sizeof(index)];
		*(CBDatabaseIndex **)compareData = index;
		memcpy(compareData + sizeof(index), key, index->keySize);
		res = CBAssociativeArrayFind(&transaction->valueWrites, compareData);
		transaction->valueWrites.compareFunc = CBValueWriteCompare;
	}else
		res.found = false;
	if (NOT res.found) {
		// Look in index for value
		CBIndexFindResult res = CBDatabaseIndexFind(self, index, key);
		if (res.status != CB_DATABASE_INDEX_FOUND){
			CBLogError("Could not find a value for a key or an error occured.");
			return res.status;
		}
		// Get the value
		CBIndexValue val = res.el.nodeLoc.cachedNode->elements[res.el.index];
		// Free CBIndexFindResult
		CBFreeIndexFindResult(res);
		// If deleted not found
		if (val.length == CB_DELETED_VALUE)
			return CB_DATABASE_INDEX_NOT_FOUND;
		// Get file
		CBDepObject file;
		if (NOT CBDatabaseGetFile(self, &file, CB_DATABASE_FILE_TYPE_DATA, 0, val.fileID)) {
			CBLogError("Could not open file for a value.");
			return CB_DATABASE_INDEX_ERROR;
		}
		if (NOT CBFileSeek(file, val.pos + offset)){
			CBLogError("Could not read seek file for value.");
			return CB_DATABASE_INDEX_ERROR;
		}
		if (NOT CBFileRead(file, data, dataSize)) {
			CBLogError("Could not read from file for value.");
			return CB_DATABASE_INDEX_ERROR;
		}
		return res.status;
	}
	// Copy the data into the buffer supplied to the function
	memcpy(data, CBFindResultToPointer(res) + sizeof(index) + index->keySize + 4 + offset, dataSize);
	return CB_DATABASE_INDEX_FOUND;
}
bool CBDatabaseRemoveValue(CBDatabase * self, CBDatabaseTransaction * transaction, CBDatabaseIndex * index, uint8_t * key){
	CBDatabaseIndex ** indexPtr = malloc(sizeof(index) + index->keySize);
	if (NOT indexPtr) {
		CBLogError("Failed to allocate memory for a deletion information to delete from storage.");
		CBDatabaseClearPending(transaction);
		return false;
	}
	// Add index pointer
	*indexPtr = index;
	// Get key pointer
	uint8_t * keyPtr = (uint8_t *)(indexPtr + 1);
	// Copy key
	memcpy(keyPtr, key, index->keySize);
	// If in valueWrites array, remove it
	CBFindResult res = CBAssociativeArrayFind(&transaction->valueWrites, indexPtr);
	if (res.found)
		CBAssociativeArrayDelete(&transaction->valueWrites, res.position, false);
	// Only continue if the value is also in the index. Else we do not want to try and delete anything since it isn't there.
	CBIndexFindResult ires = CBDatabaseIndexFind(self, index, keyPtr);
	CBFreeIndexFindResult(ires);
	if (ires.status != CB_DATABASE_INDEX_FOUND){
		free(indexPtr);
		if (ires.status == CB_DATABASE_INDEX_ERROR) {
			CBDatabaseClearPending(transaction);
			CBLogError("There was an error while attempting to find a key in an index.");
			return false;
		}
		return true;
	}
	// See if key exists to be deleted already
	res = CBAssociativeArrayFind(&transaction->deleteKeys, indexPtr);
	if (NOT res.found) {
		// Does not already exist so insert into array
		if (NOT CBAssociativeArrayInsert(&transaction->deleteKeys, indexPtr, res.position, NULL)) {
			CBLogError("Failed to insert a deletion element into the deleteKeys array.");
			CBDatabaseClearPending(transaction);
			free(keyPtr);
			return false;
		}
	}
	return true;
}
bool CBDatabaseWriteConcatenatedValue(CBDatabase * self, CBDatabaseTransaction * transaction, CBDatabaseIndex * index, uint8_t * key, uint8_t numDataParts, uint8_t ** data, uint32_t * dataSize){
	// Create element
	uint32_t size = 0;
	for (uint8_t x = 0; x < numDataParts; x++)
		size += dataSize[x];
	CBDatabaseIndex ** indexPtr = malloc(sizeof(index) + index->keySize + 8 + size);
	if (NOT indexPtr) {
		CBLogError("Failed to allocate memory for a value write element.");
		CBDatabaseClearPending(transaction);
		return false;
	}
	*indexPtr = index;
	uint8_t * keyPtr = (uint8_t *)(index + 1);
	memcpy(keyPtr, key, index->keySize);
	uint32_t * intPtr = (uint32_t *)(keyPtr + index->keySize);
	*intPtr = size;
	uint8_t * dataPtr = (uint8_t *)(intPtr + 1);
	// Add data
	for (uint8_t x = 0; x < numDataParts; x++){
		memcpy(dataPtr, data[x], dataSize[x]);
		dataPtr += dataSize[x];
	}
	intPtr = (uint32_t *)dataPtr;
	*intPtr = CB_OVERWRITE_DATA;
	return CBDatabaseAddWriteValue(transaction, indexPtr);
}
bool CBDatabaseWriteValue(CBDatabase * self, CBDatabaseTransaction * transaction, CBDatabaseIndex * index, uint8_t * key, uint8_t * data, uint32_t size){
	// Write element by overwriting previous data
	return CBDatabaseWriteValueSubSection(self, transaction, index, key, data, size, CB_OVERWRITE_DATA);
}
bool CBDatabaseWriteValueSubSection(CBDatabase * self, CBDatabaseTransaction * transaction, CBDatabaseIndex * index, uint8_t * key, uint8_t * data, uint32_t size, uint32_t offset){
	// Create element
	CBDatabaseIndex ** indexPtr = malloc(sizeof(index) + index->keySize + 8 + size);
	*indexPtr = index;
	uint8_t * keyPtr = (uint8_t *)(indexPtr + 1);
	if (NOT keyPtr) {
		CBLogError("Failed to allocate memory for a value write element.");
		CBDatabaseClearPending(transaction);
		return false;
	}
	memcpy(keyPtr, key, index->keySize);
	uint32_t * intPtr = (uint32_t *)(keyPtr + index->keySize);
	*intPtr = size;
	uint8_t * dataPtr = (uint8_t *)(intPtr + 1);
	memcpy(dataPtr, data, size);
	intPtr = (uint32_t *)(dataPtr + size);
	*intPtr = offset;
	return CBDatabaseAddWriteValue(transaction, indexPtr);
}
CBCompare CBDeleteKeysCompare(void * el1, void * el2){
	uint8_t size1 = (*(CBDatabaseIndex **)el1)->keySize;
	uint8_t size2 = (*(CBDatabaseIndex **)el2)->keySize;
	return CBCompareIndexPtrAndData(el1, el2, size1, size2);
}
CBCompare CBIndexCompare(void * el1, void * el2){
	if (el1 > el2)
		return CB_COMPARE_MORE_THAN;
	else if (el2 > el1)
		return CB_COMPARE_LESS_THAN;
	return CB_COMPARE_EQUAL;
}
CBCompare CBValueWriteCompare(void * el1, void * el2){
	uint8_t size1 = (*(CBDatabaseIndex **)el1)->keySize;
	size1 += *(uint32_t *)(el1 + size1 + sizeof(CBDatabaseIndex *)) + 8;
	uint8_t size2 = (*(CBDatabaseIndex **)el2)->keySize;
	size2 += *(uint32_t *)(el2 + size2 + sizeof(CBDatabaseIndex *)) + 8;
	return CBCompareIndexPtrAndData(el1, el2, size1, size2);
}
