//
//  CBDatabase.h
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

/**
 @file
 @brief Implements a database for use with the storage requirements.
 */

#ifndef CBDATABASEH
#define CBDATABASEH

#include "CBDependencies.h"
#include "CBAssert.h"
#include "CBAssociativeArray.h"
#include "CBFileDependencies.h"
#include "CBValidator.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#define CB_OVERWRITE_DATA 0xFFFFFFFF
#define CB_DELETED_VALUE 0xFFFFFFFF
#define CB_DOESNT_EXIST CB_DELETED_VALUE
#define CB_DATABASE_BTREE_ELEMENTS 64
#define CB_DATABASE_BTREE_HALF_ELEMENTS (CB_DATABASE_BTREE_ELEMENTS/2)
#define CB_BRANCH_DATA_SIZE 42 // 20 for general data, 20 For branch work and 1 for branch work length
#define CB_BLOCK_CHAIN_EXTRA_SIZE (3 + CB_BRANCH_DATA_SIZE * CB_MAX_BRANCH_CACHE)
#define CB_MAX_BRANCH_SECTIONS (CB_MAX_BRANCH_CACHE*2 - 1)
#define CB_ACCOUNTER_EXTRA_SIZE 32
#define CB_ADDRESS_EXTRA_SIZE 4
#define CB_NODE_EXTRA_SIZE 8
#define CB_DATABASE_NO_LAST_FILE UINT16_MAX
#define CB_DATABASE_EXTRA_DATA_SIZE CB_BLOCK_CHAIN_EXTRA_SIZE + CB_ACCOUNTER_EXTRA_SIZE + CB_ADDRESS_EXTRA_SIZE + CB_NODE_EXTRA_SIZE
#define CBMax(a,b) (((a)>(b))?(a):(b))

typedef enum{
	CB_DATABASE_FILE_TYPE_INDEX,
	CB_DATABASE_FILE_TYPE_DELETION_INDEX,
	CB_DATABASE_FILE_TYPE_DATA,
	CB_DATABASE_FILE_TYPE_NONE,
} CBDatabaseFileType;

typedef enum{
	CB_DATABASE_INDEX_FOUND,
	CB_DATABASE_INDEX_NOT_FOUND,
	CB_DATABASE_INDEX_ERROR,
} CBIndexFindStatus;

typedef enum{
	CB_INDEX_BLOCK_HASH,
	CB_INDEX_BLOCK,
	CB_INDEX_TX,
	CB_INDEX_UTXOUT,
	CB_INDEX_ADDRS,
	CB_INDEX_OUR_TXS,
	CB_INDEX_OTHER_TXS,
	CB_INDEX_ACCOUNTER_START,
} CBIndexIDs;

typedef enum{
	CB_KEY_NONE,
	CB_KEY_CURRENT_VALUE,
	CB_KEY_CURRENT_CHANGE,
	CB_KEY_STAGED_VALUE,
	CB_KEY_STAGED_CHANGE,
	CB_KEY_DISK
} CBIteratorWhatKey;

typedef enum{
	CB_OVERWRITE_LOG_FILE_TYPE = 0,
	CB_OVERWRITE_LOG_INDEX_ID = 1,
	CB_OVERWRITE_LOG_FILE_ID = 2,
	CB_OVERWRITE_LOG_OFFSET = 4,
	CB_OVERWRITE_LOG_LENGTH = 8,
} CBOverwriteLogFileOffsets;

typedef enum{
	CB_DELETION_KEY_ACTIVE = 0,
	CB_DELETION_KEY_LENGTH = 1,
	CB_DELETION_KEY_FILE_ID = 5,
	CB_DELETION_KEY_OFFSET = 7,
} CBDeletionKeyOffsets;

/**
 @brief An index value which references the value's data position with a key.
 */
typedef struct{
	CBObject base;
	uint16_t fileID; /**< The file ID for the data */
	uint32_t pos; /**< The position of the data in the file. */
	uint32_t length; /**< The length of the data or CB_DELETED_VALUE if deleted. */
	uint8_t key[]; /**< The key for the value */
} CBIndexValue;

/**
 @brief Describes a deleted section of the database.
 */
typedef struct{
	uint8_t key[11]; /**< The key for this deleted section which begins with 0x01 if the deleted section is active or 0x00 if it is no longer active, then has four bytes for the length of the deleted section in big-endian, then the file ID in little-endian and finally the offset of the deleted section in little-endian. */
	uint32_t indexPos; /**< The position in the index file where this value exists */
} CBDeletedSection;

typedef struct{
	uint16_t indexFile;
	uint32_t offset;
} CBIndexDiskNodeLocation;

typedef struct CBIndexNode CBIndexNode;

/**
 @brief Information for a btree node in memory cache or on disk.
 */
typedef struct{
	bool cached; /**< True if the child is cached in memory. */
	CBIndexDiskNodeLocation diskNodeLoc; /**< The node location on disk. */
	CBIndexNode * cachedNode; /**< A pointer to the CBIndexCacheNode child in memory if cached. */
} CBIndexNodeLocation;

/**
 @brief The memory cache of the index B-Tree
 */
struct CBIndexNode{
	uint8_t numElements; /**< The number of elements in the node. */
	CBIndexValue * elements[CB_DATABASE_BTREE_ELEMENTS]; /**< The elements. */
	CBIndexNodeLocation children[CB_DATABASE_BTREE_ELEMENTS + 1]; /**< The children. */
};

/**
 @brief Stores parent nodes in a stack. The root is not stored.
 */
typedef struct{
	CBIndexNodeLocation nodes[7];
	uint8_t pos[8]; /**< Position of node in parent node. */
	uint8_t num;
} CBIndexParentStack;

typedef struct{
	uint8_t index;
	CBIndexNodeLocation nodeLoc; /**< Including temporary cached node even if it's not cached permenantly */
	CBIndexParentStack parentStack;
} CBIndexElementPos;

typedef struct{
	CBIndexFindStatus status;
	CBIndexElementPos el;
} CBIndexFindWithParentsResult;

typedef struct{
	CBIndexFindStatus status;
	union{
		CBIndexValue * el;
		CBIndexNodeLocation node;
	} data;
} CBIndexFindResult;

typedef struct{
	uint8_t * extraData; /**< The version of the extra data for this transaction. Write and read from this. During a commit, any changes will be written to disk. */
	CBAssociativeArray indexes; /**< An array of index objects with valueWrite arrays. @see CBIndexTxData */
} CBDatabaseTransactionChanges;

/**
 @brief Structure for CBDatabase objects. @see CBDatabase.h
 */
typedef struct{
	char * dataDir; /**< The data directory. */
	char * folder; /**< The folder for this database. */
	uint16_t lastFile; /**< The last file ID. */
	uint32_t lastSize; /**< Size of last file */
	CBAssociativeArray deletionIndex; /**< Index of all deleted sections. The key begins with 0x01 if the deleted section is active or 0x00 if it is no longer active, the key is then followed by the length in big endian. */
	uint32_t numDeletionValues; /**< Number of values in the deletion index */
	// Files
	CBDepObject deletionIndexFile;
	CBDepObject fileObjectCache; /**< Stores last used file object for reuse until new file is needed */
	uint16_t lastUsedFileID; /**< The last used data file number. */
	CBDepObject logFile;
	uint8_t * extraDataOnDisk;
	uint32_t extraDataSize; /**< The size of the extra data to store in the database. */
	CBDatabaseTransactionChanges staged; /**< Ready to be commited to disk. */
	CBDatabaseTransactionChanges current; /**< Changes that are not staged for commit and can be reversed. */
	uint32_t commitGap; /**< The number of miliseconds between each commit unless the cache reaches the limit. */
	uint32_t cacheLimit; /**< If the transaction holds more bytes than this, then a commit will happen. */
	uint32_t stagedSize; /**< The rough size of the staged changes. */
	uint64_t lastCommit; /**< Time of last commit */
	bool hasStaged; /**< True if there are changes to commit */
	uint8_t numIndexes; /**< The number indexes that are involved in the transaction */
	CBAssociativeArray valueWritingIndexes; /**< An array of index objects which have value writes in the staged changes. */
	// ??? Add mutexes per index
	CBDepObject diskMutex; /**< Mutex afor reading a modifying disk data. */
	CBDepObject stagedMutex; /**< Mutex for access of staged data */
	CBDepObject currentMutex;
	CBDepObject commitMutex; /**< Mutex when commits are being made. */
	CBDepObject commitThread;
	CBDepObject commitCond; 
	bool shutDownThread;
	bool doCommit;
	bool commitFail;
} CBDatabase;

typedef struct{
	CBDatabase * database;
	uint8_t ID; /**< The ID for the index */
	CBIndexNodeLocation indexCache; /**< The memory cache of the btree nodes for the index. This is the root index. */
	uint64_t numCached; /**< The number of cached nodes */
	uint8_t keySize; /**< The size in bytes of the keys. */
	uint32_t cacheLimit; /**< The number of bytes that can be cached */
	uint16_t lastFile; /**< The last index file ID. */
	uint32_t lastSize; /**< Size of last index file */
	uint16_t newLastFile; /** Used to store the new last file number during a commit. */
	uint32_t newLastSize; /** Used to store the new last file size during a commit. */
	CBDepObject indexFile;
	uint16_t lastUsedFileID;
} CBDatabaseIndex;

typedef struct{
	CBDatabaseIndex * index;
	CBAssociativeArray valueWrites; /**< Values to write. The key followed by a 32 bit integer for the data length and then the data, followed by a 32bit integer for the offset. If CB_OVERWRITE_DATA is used then the value in the database is replaced by the new data. */
	CBAssociativeArray deleteKeys; /**< An array of keys to delete with the key data. */
	CBAssociativeArray changeKeysOld; /**< An array of keys to change with the old key followed imediately by the new key, ordered by the old key. */
	CBAssociativeArray changeKeysNew; /**< An array of keys to change with the old key followed imediately by the new key, ordered by the new key. */
} CBIndexTxData;

/**
 @brief Allows iteration between a min element and a max element in an index. ??? Do NULL for no minimum or maximum?
 */
typedef struct{
	uint8_t * minElement;
	uint8_t * maxElement;
	CBDatabaseIndex * index;
	CBRangeIterator currentIt; /**< Iterator of current value writes */
	CBRangeIterator stagedIt; /**< Iterator of staged value writes. Those which are to be changed are ignored. */
	CBRangeIterator currentChangedIt; /**< Iterator over change key entrys to find staged/commited values that are to be changed by current data. */
	CBRangeIterator stagedChangedIt; /**< Iterator over change key entrys to find committed values that are to be changed by staged data. */
	CBIndexElementPos indexPos; /**< Position in the index */
	bool gotCurrentEl;
	bool gotStagedEl;
	bool gotCurrentChangedEl;
	bool gotStagedChangedEl;
	bool gotIndEl;
	bool foundIndex;
} CBDatabaseRangeIterator;

// Initialisation

/**
 @brief Returns a new database object.
 @param dataDir The directory where the data files should be stored.
 @param folder A folder for the data files to prevent conflicts. Will be created if it doesn't exist.
 @param extraDataSize The size of bytes to put aside for extra data in the database.
 @returns The database object or NULL on failure.
 */
CBDatabase * CBNewDatabase(char * dataDir, char * folder, uint32_t extraDataSize, uint32_t commitGap, uint32_t cacheLimit);

/**
 @brief Initialises a database object.
 @param self The CBDatabase object to initialise.
 @param dataDir The directory where the data files should be stored.
 @param prefix A prefix for the data files to prevent conflicts.
 @param extraDataSize The size of bytes to put aside for extra data in the database.
 @returns true on success and false on failure.
 */
bool CBInitDatabase(CBDatabase * self, char * dataDir, char * folder, uint32_t extraDataSize, uint32_t commitGap, uint32_t cacheLimit);

/**
 @brief Initialises a database transaction changes object.
 @param self The CBDatabaseTransactionChanges object to initialise.
 @param database The database object.
 */
void CBInitDatabaseTransactionChanges(CBDatabaseTransactionChanges * self, CBDatabase * database);
void CBInitDatabaseRangeIterator(CBDatabaseRangeIterator * it, uint8_t * minEl, uint8_t * maxEl, CBDatabaseIndex * index);

/**
 @brief Loads an index or creates it if it doesn't exist.
 @param self The database object.
 @param indexID The identifier for this index.
 @param keySize The key size for this index.
 @param cacheLimit The number of bytes that can be cached for this index.
 @returns The index or NULL on failure.
 */
CBDatabaseIndex * CBLoadIndex(CBDatabase * self, uint8_t indexID, uint8_t keySize, uint32_t cacheLimit);

/**
 @brief Loads a node from the disk into memory.
 @param index The index object.
 @param node The allocated node.
 @param nodeFile The index of the index files where the node exists.
 @param nodeOffset The offset from the begining of the file where the node exists.
 */
bool CBDatabaseLoadIndexNode(CBDatabaseIndex * index, CBIndexNode * node, uint16_t nodeFile, uint32_t nodeOffset);

/**
 @brief Reads and opens the deletion index during initialisation
 @param self The storage object.
 @param filename The index filename.
 @returns true on success or false on failure.
 */
bool CBDatabaseReadAndOpenDeletionIndex(CBDatabase * self, char * filename);

/**
 @brief Creates a deletion index during the first initialisation
 @param self The storage object.
 @param filename The index filename.
 @returns true on success or false on failure.
 */
bool CBDatabaseCreateDeletionIndex(CBDatabase * self, char * filename);
CBIndexValue * CBNewIndexValue(uint8_t keySize);

/**
 @brief Gets a CBDatabase from another object. Use this to avoid casts.
 @param self The object to obtain the CBDatabase from.
 @returns The CBDatabase object.
 */
CBDatabase * CBGetDatabase(void * self);

// Destructor

/**
 @brief Frees the database object.
 @param self The database object.
 @param true on success or false on commit error.
 */
bool CBFreeDatabase(CBDatabase * self);

/**
 @brief Frees the database transaction changes object.
 @param self The database transaction changes object.
 */
void CBFreeDatabaseTransactionChanges(CBDatabaseTransactionChanges * self);

/**
 @brief Frees the node of an index.
 @param node The node to free.
 */
void CBFreeIndexNode(CBIndexNode * node);

/**
 @brief Frees an index.
 @param index The index to free.
 */
void CBFreeIndex(CBDatabaseIndex * index);
void CBFreeIndexElementPos(CBIndexElementPos pos);
void CBFreeIndexTxData(void * indexTxData);

/**
 @brief Free a database range iterator. Only use after CBDatabaseRangeIteratorFirst or CBDatabaseRangeIteratorLast returns found.
 @param it The iterator.
 */
void CBFreeDatabaseRangeIterator(CBDatabaseRangeIterator * it);

// Additional functions

/**
 @brief Add a deletion entry.
 @param self The storage object.
 @param fileID The file ID
 @param pos The position of the deleted section.
 @param len The length of the deleted section.
 @retruns true on success and false on failure
 */
bool CBDatabaseAddDeletionEntry(CBDatabase * self, uint16_t fileID, uint32_t pos, uint32_t len);

/**
 @brief Add an overwrite operation.
 @param self The storage object.
 @param fileType The type of the file.
 @param index The index used if the type is an index.
 @param fileID The file ID
 @param data The data to write.
 @param offset The offset to begin writting.
 @param dataLen The length of the data to write.
 @retruns true on success and false on failure
 */
bool CBDatabaseAddOverwrite(CBDatabase * self, CBDatabaseFileType fileType, CBDatabaseIndex * index, uint16_t fileID, uint8_t * data, uint32_t offset, uint32_t dataLen);

/**
 @brief Adds a value to the database without overwriting previous indexed data.
 @param self The storage object.
 @param dataSize The size of the data to write.
 @param data The data to write.
 @param indexValue The index data to write.
 @retruns true on success and false on failure
 */
bool CBDatabaseAddValue(CBDatabase * self, uint32_t dataSize, uint8_t * data, CBIndexValue * indexValue);

/**
 @brief Adds a write value to the valueWrites array for the transaction.
 @param writeValue The write value element data.
 @param stage If true stage change.
 */
void CBDatabaseAddWriteValue(uint8_t * writeValue, CBDatabaseIndex * index, uint8_t * key, uint32_t dataLen, uint8_t * dataPtr, uint32_t offset, bool stage);

/**
 @brief Add an append operation.
 @param self The database object.
 @param fileType The type of the file to append.
 @param indexID The index ID used if appending an index.
 @param fileID The file ID
 @param data The data to write.
 @param dataLen The length of the data to write.
 @retruns true on success and false on failure
 */
bool CBDatabaseAppend(CBDatabase * self, CBDatabaseFileType fileType, CBDatabaseIndex * index, uint16_t fileID, uint8_t * data, uint32_t dataLen);

/**
 @brief Add an append operation of zeros.
 @param self The database object.
 @param fileType The type of the file to append.
 @param indexID The index ID used if appending an index.
 @param fileID The file ID
 @param amount Number of zero bytes to append.
 @retruns true on success and false on failure
 */
bool CBDatabaseAppendZeros(CBDatabase * self, CBDatabaseFileType fileType, CBDatabaseIndex * index, uint16_t fileID, uint32_t amount);

/**
 @brief Replaces a key for a value with a key of the same length.
 @param self The database object.
 @param index The database index to change the key for.
 @param previousKey The current key to be replaced. The first byte is the length.
 @param newKey The new key for this value. The first byte is the length and should be the same as the first key.
 @param If true, stage this change.
 @returns true on success and false on failure.
 */
bool CBDatabaseChangeKey(CBDatabaseIndex * index, uint8_t * previousKey, uint8_t * newKey, bool stage);

/**
 @brief Removes all of the current value write, delete and change key operations.
 @param self The database object.
 */
void CBDatabaseClearCurrent(CBDatabase * self);

/**
 @brief The data is written to the disk.
 @param tx The database object with the staged changes.
 @returns true on success and false on failure, and thus the database needs to be recovered with CBDatabaseEnsureConsistent.
 */
bool CBDatabaseCommit(CBDatabase * database);
bool CBDatabaseCommitProcess(CBDatabase * database);
void CBDatabaseCommitThread(void * database);

/**
 @brief Ensure the database is consistent and recover the database if it is not.
 @param self The database object.
 @returns true if the database is consistent and false on failure.
 */
bool CBDatabaseEnsureConsistent(CBDatabase * self);

/**
 @brief Returns a CBFindResult for largest active deleted section and "found" will be true if the largest active deleted section is above a length.
 @param self The database object.
 @param length The minimum length required.
 @returns The largest active deleted section as a CBFindResult.
 */
CBFindResult CBDatabaseGetDeletedSection(CBDatabase * self, uint32_t length);

/**
 @brief Gets a file object for a file number (eg. 0 for index, 1 for deleted index and 2 for first data file)
 @param self The database object.
 @param type The type of file to obtain.
 @param indexID The index ID, only applicable for CB_DATABASE_FILE_TYPE_INDEX.
 @param fileID The id of the file to get an object for. Not needed for CB_DATABASE_FILE_TYPE_DELETION_INDEX.
 @returns true on success and false on failure.
 */
bool CBDatabaseGetFile(CBDatabase * self, CBDepObject * file, CBDatabaseFileType type, CBDatabaseIndex * index, uint16_t fileID);

/**
 @brief Gets the length of a value in the database or CB_DOESNT_EXIST if it does not exist.
 @param index The database index.
 @param key The key. The first byte is the length.
 @param length Set to the total length of the value or CB_DOESNT_EXIST if the value does not exist in the database.
 @returns true on success and false on failure.
 */
bool CBDatabaseGetLength(CBDatabaseIndex * index, uint8_t * key, uint32_t * length);

/**
 @brief Gets a pointer to the CBIndexTxData for a database index.
 @param changes The database transaction changes object.
 @param index The index object.
 @param create If true, create if not found, else return NULL if not found.
 @returns @see CBIndexTxData or NULL if read is true and no index data is available.
 */
CBIndexTxData * CBDatabaseGetIndexTxData(CBDatabaseTransactionChanges * changes, CBDatabaseIndex * index, bool create);

/**
 @brief Deletes an index element.
 @param index The index object.
 @param res The information of the node we are inserting to.
 @returns true on success and false on failure.
 */
bool CBDatabaseIndexDelete(CBDatabaseIndex * index, CBIndexFindWithParentsResult * res);
CBIndexFindResult CBDatabaseIndexFind(CBDatabaseIndex * index, uint8_t * key);

/**
 @brief Finds an index entry, or where the index should be inserted.
 @param index The index object.
 @param key The key to find.
 @returns @see CBIndexFindResult
 */
CBIndexFindWithParentsResult CBDatabaseIndexFindWithParents(CBDatabaseIndex * index, uint8_t * key);

/**
 @brief Inserts an index value into an index.
 @param index The index object.
 @param indexVal The index value to insert.
 @param pos The information of the node we are inserting to.
 @param right The child to the right of the inserted element.
 @returns true on success and false on failure.
 */
bool CBDatabaseIndexInsert(CBDatabaseIndex * index, CBIndexValue * indexVal, CBIndexElementPos * pos, CBIndexNodeLocation * right);

/**
 @brief Copies children in an index node.
 @param index The index object.
 @param dest The destination node.
 @param source The source node.
 @param startPos The starting position to take children from the source node.
 @param endPos The starting position to place children in the destination node.
 @param amount The number of children to copy.
 @param append If true, append to the destination node.
 @param true on success or false on failure.
 */
bool CBDatabaseIndexMoveChildren(CBDatabaseIndex * index, CBIndexNodeLocation * dest, CBIndexNodeLocation * source, uint8_t startPos, uint8_t endPos, uint8_t amount, bool append);

/**
 @brief Copies elements in an index node.
 @param index The index object.
 @param dest The destination node.
 @param source The source node.
 @param startPos The starting position to take elements from the source node.
 @param endPos The starting position to place elements in the destination node.
 @param amount The number of elements to copy.
 @param append If true, append to the destination node.
 @param true on success or false on failure.
 */
bool CBDatabaseIndexMoveElements(CBDatabaseIndex * index, CBIndexNodeLocation * dest, CBIndexNodeLocation * source, uint8_t startPos, uint8_t endPos, uint8_t amount, bool append);
void CBDatabaseIndexNodeSearchDisk(CBDatabaseIndex * index, uint8_t * key, CBIndexFindResult * result);

/**
 @brief Does a binary search on a node in the cache and returns the location and index data.
 @param index The index object.
 @param node The node to search.
 @param key The key to search for.
 */
void CBDatabaseIndexNodeBinarySearchMemory(CBDatabaseIndex * index, CBIndexNode * node, uint8_t * key, CBIndexFindStatus * status, uint8_t * pos);

/**
 @brief Sets blank children
 @param index The index object.
 @param dest The destination node.
 @param offset The offset to the first child to make blank.
 @param amount The number of children to make blank.
 @param true on success or false on failure.
 */
bool CBDatabaseIndexSetBlankChildren(CBDatabaseIndex * index, CBIndexNodeLocation * dest, uint8_t offset, uint8_t amount);

/**
 @brief Sets a child in an index node.
 @param index The index object.
 @param node The destination node.
 @param child The child to set.
 @param pos The position to set the child.
 @param append If true, append to the destination node.
 @returns true on success or false on failure.
 */
bool CBDatabaseIndexSetChild(CBDatabaseIndex * index, CBIndexNodeLocation * node, CBIndexNodeLocation * child, uint8_t pos, bool append);

/**
 @brief Sets an element in an index node.
 @param index The index object.
 @param node The destination node.
 @param element The element to set.
 @param pos The position to set the element.
 @param append If true, append to the destination node.
 @returns true on success or false on failure.
 */
bool CBDatabaseIndexSetElement(CBDatabaseIndex * index, CBIndexNodeLocation * node, CBIndexValue * element, uint8_t pos, bool append);

/**
 @brief Sets a new nodes position in the index files.
 @param index The index.
 @param new The new node.
 */
void CBDatabaseIndexSetNewNodePosition(CBDatabaseIndex * index, CBIndexNodeLocation * new);

/**
 @brief Sets the number of elements for an index node.
 @param index The index object.
 @param node The node.
 @param numElements The number of elements.
 @param append If true, append to the destination node.
 @returns true on success or false on failure.
 */
bool CBDatabaseIndexSetNumElements(CBDatabaseIndex * index, CBIndexNodeLocation * node, uint8_t numElements, bool append);

/**
 @brief Goes to the first elment of the iterator.
 @param it The iterator.
 @returns @see CBIndexFindStatus
 */
CBIndexFindStatus CBDatabaseRangeIteratorFirst(CBDatabaseRangeIterator * it);

/**
 @brief Copies the key at the current position into "key" and does not iterate.
 @param it The iterator.
 @returns true if a key exists, or false
 */
bool CBDatabaseRangeIteratorGetKey(CBDatabaseRangeIterator * it, uint8_t * key);
bool CBDatabaseRangeIteratorGetLength(CBDatabaseRangeIterator * it, uint32_t * length);

/**
 @param it The iterator.
 @returns the highest key
 */
CBIteratorWhatKey CBDatabaseRangeIteratorGetHigher(CBDatabaseRangeIterator * it);

/**
 @param it The iterator.
 @returns the lowest key
 */
CBIteratorWhatKey CBDatabaseRangeIteratorGetLesser(CBDatabaseRangeIterator * it);
bool CBDatabaseRangeIteratorKeyIfHigher(uint8_t ** highestKey, CBRangeIterator * it, uint8_t keySize, bool changed);
bool CBDatabaseRangeIteratorKeyIfLesser(uint8_t ** lowestKey, CBRangeIterator * it, uint8_t keySize, bool changed);

/**
 @brief Goes to the last element of the iterator.
 @param it The iterator.
 @returns @see CBIndexFindStatus
 */
CBIndexFindStatus CBDatabaseRangeIteratorLast(CBDatabaseRangeIterator * it);

/**
 @brief Goes to the next element of the iterator.
 @param it The iterator.
 @returns @see CBIndexFindStatus
 */
CBIndexFindStatus CBDatabaseRangeIteratorNext(CBDatabaseRangeIterator * it);

/**
 @brief Goes to the next or previous index element of the iterator. Does not iterate the transaction iterator.
 @param it The iterator.
 @returns @see CBIndexFindStatus
 */
CBIndexFindStatus CBDatabaseRangeIteratorIterateIndex(CBDatabaseRangeIterator * it, bool forwards);

/**
 @brief Goes to the next element of the iterator if the current key is deleted.
 @param it The iterator.
 @param If true go forwards, else backwards.
 @returns @see CBIndexFindStatus
 */
CBIndexFindStatus CBDatabaseRangeIteratorIterateIndexIfDeleted(CBDatabaseRangeIterator * it, bool forwards);
void CBDatabaseRangeIteratorIterateStagedIfInvalid(CBDatabaseRangeIterator * it, CBIndexTxData * currentIndexTxData, CBIndexTxData * stagedIndexTxData, bool forward);
void CBDatabaseRangeIteratorIterateStagedIfInvalidParticular(CBIndexTxData * currentIndexTxData, CBRangeIterator * it, CBAssociativeArray * arr, bool * got, bool forward);
void CBDatabaseRangeIteratorNextMinimum(CBDatabaseRangeIterator * it);

/**
 @brief Reads from the iterator at the current position and does not iterate.
 @param it The iterator.
 @param data The data to set.
 @param dataLen The length of the data to read.
 @param offset The offset to begin reading.
 @returns true on success and false on failure.
 */
bool CBDatabaseRangeIteratorRead(CBDatabaseRangeIterator * it, uint8_t * data, uint32_t dataLen, uint32_t offset);

/**
 @brief Queues a key-value read operation.
 @param index The index where the value is to be read.
 @param key The key for this data. The first byte is the length.
 @param data A pointer to memory with enough space to hold the data. The data will be set to this.
 @param dataSize The size to read.
 @param offset The offset to begin reading.
 @param staged Read from staged
 @returns The found status of the value.
 */
CBIndexFindStatus CBDatabaseReadValue(CBDatabaseIndex * index, uint8_t * key, uint8_t * data, uint32_t dataSize, uint32_t offset, bool staged);

/**
 @brief Queues a key-value delete operation.
 @param index The index object.
 @param key The key for this data. The first byte is the length.
 @param If true, stage this change.
 @returns true on success and false on failure.
 */
bool CBDatabaseRemoveValue(CBDatabaseIndex * index, uint8_t * key, bool stage);

/**
 @brief Stages current changes for commiting to disk such that they are no longer to be reversed. Will commit if commit gap reached or cache limit reached.
 @param self The database object.
 @returns true on success and false on failure.
 */
bool CBDatabaseStage(CBDatabase * self);

/**
 @brief Queues a key-value write operation from a many data parts. The data is concatenated.
 @param index The inex object.
 @param key The key for this data. The first byte is the length.
 @param numDataParts The number of data parts.
 @param data A list of the data.
 @param dataSize A list of the data sizes
 */
void CBDatabaseWriteConcatenatedValue(CBDatabaseIndex * index, uint8_t * key, uint8_t numDataParts, uint8_t ** data, uint32_t * dataSize);

/**
 @brief Queues a key-value write operation.
 @param index The index object.
 @param key The key for this data. The first byte is the length.
 @param data The data to store.
 @param size The size of the data to store. A new value replaces the old one.
 */
void CBDatabaseWriteValue(CBDatabaseIndex * index, uint8_t * key, uint8_t * data, uint32_t size);

/**
 @brief Queues a key-value write operation for a sub-section of existing data.
 @param index The index object
 @param key The key for this data. The first byte is the length.
 @param data The data to store.
 @param size The size of the data to write.
 @param offset The offset to start writing. CB_OVERWRITE_DATA to remove the old data and write anew.
 */
void CBDatabaseWriteValueSubSection(CBDatabaseIndex * index, uint8_t * key, uint8_t * data, uint32_t size, uint32_t offset);
/*
 @brief Compares two CBIndexTxData objects by the indexes
 @param array The associative array object.
 @param el1 Index transaction data object one
 @param el2 Index transaction data object two
 @returns @see CBCompare
 */
CBCompare CBIndexCompare(CBAssociativeArray * array, void * el1, void * el2);
/*
 @brief Compares two CBDatabaseIndex pointers.
 @param array The associative array object.
 @param el1 Pointer one to compare.
 @param el2 Pointer two to compare.
 @returns @see CBCompare
 */
CBCompare CBIndexPtrCompare(CBAssociativeArray * array, void * el1, void * el2);
/*
 @brief Compares two new keys in the changeKeysNew array.
 @param array The associative array object.
 @param el1 The first to compare.
 @param el2 The second to compare.
 @returns @see CBCompare
 */
CBCompare CBNewKeyCompare(CBAssociativeArray * array, void * el1, void * el2);
/*
 @brief Compares new keys in the changeKeysNew array with a single key.
 @param array The associative array object.
 @param el1 The single key
 @param el2 The key from the array.
 @returns@see CBCompare
 */
CBCompare CBNewKeyWithSingleKeyCompare(CBAssociativeArray * array, void * el1, void * el2);
/*
 @brief Compares two deletion or value write entries in a database transaction.
 @param array The associative array object.
 @param el1 The first to compare.
 @param el2 The second to compare.
 @returns @see CBCompare
 */
CBCompare CBTransactionKeysCompare(CBAssociativeArray * array, void * el1, void * el2);
/*
 @brief Compares two value write entries in a database transaction including the offset, such that high offsets come first.
 @param array The associative array object.
 @param el1 The first to compare.
 @param el2 The second to compare.
 @returns @see CBCompare
 */
CBCompare CBTransactionKeysAndOffsetCompare(CBAssociativeArray * array, void * el1, void * el2);

#endif
