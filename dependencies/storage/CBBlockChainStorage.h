//
//  CBBlockChainStorage.h
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

/**
 @file
 @brief Implements the storage dependencies.
 */

#ifndef CBSAFESTORAGEH
#define CBSAFESTORAGEH

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "CBDependencies.h"

/**
 @brief Describes a file overwrite operation that should be completed on a commit.
 */
typedef struct{
	uint64_t offset; /**< The position in the file to start the write operation. */
	uint8_t * data; /**< The data to write. */
	uint32_t size; /**< The number of bytes to write. */
} CBOverwriteOperation;

/**
 @brief Describes an append operation that should be completed on a commit.
 */
typedef struct{
	uint8_t * data; /**< The data to write. */
	uint32_t size; /**< The number of bytes to write. */
} CBAppendOperation;

/**
 @brief Append and overwrite operations to be completed on a file
 */
typedef struct{
	uint16_t fileID; /**< The file ID */
	CBOverwriteOperation * overwriteOperations; /**< A list of overwrite operations to be completed by the next commit */
	uint8_t numOverwriteOperations; /**< Number of overwrite operations */
	CBAppendOperation * appendOperations; /**< A list of append operations to be completed by the next commit */
	uint8_t numAppendOperations; /**< Number of append operations */
} CBFileOperations;

/**
 @brief A key/value pair which references the value's data position
 */
typedef struct{
	uint8_t key[6];
	uint16_t fileID; /**< The file ID for the data or 0 if deleted */
	uint32_t pos; /**< The position of the data in the file. */
	uint32_t length; /**< The length of the data */
} CBKeyValuePair;

/**
 @brief Describes a deleted section of the database.
 */
typedef struct{
	bool active; /**< If true this describes a deleted section of the database, if false this no longer applies. */
	uint16_t fileID; /**< The ID of the file with the deleted section. */
	uint32_t pos; /**< The position in the file with the deleted section. */
	uint32_t length; /**< The length of the deleted section. */
} CBDeletedSection;

/**
 @brief Structure for CBBlockChainStorage objects. @see CBBlockChainStorage.h
 */
typedef struct{
	char * dataDir; /**< The data directory. */
	uint8_t numFiles; /**< Number of files to apply operations on */
	CBFileOperations * files; /** < The files with the operations */
	CBKeyValuePair * index; /**< Index of all key/value pairs */
	uint32_t numValues; /**< The number of key/value pairs */
	CBKeyValuePair ** sortedIndex; /**< The sorted index */
	uint16_t numAvailableFiles; /**< The number of data files there are on disk. */
	uint32_t pendingAppends; /**< Bytes to be appended */
	uint32_t lastSize; /**< Size of last file */
	CBDeletedSection * deletionIndex; /**< Index of all deleted sections */
	uint32_t numDeletions; /**< Number of deletion entries */
	void (*logError)(char *,...);
} CBBlockChainStorage;

// Additional functions

/**
 @brief Add an append operation to complete on commit.
 @param self The storage object.
 @param fileID The file ID
 @param data The data to write.
 @param dataLen The length of the data to write.
 @retruns true on success and false on failure
 */
bool CBBlockChainStorageAddAppend(CBBlockChainStorage * self, uint16_t fileID, uint8_t * data, uint32_t dataLen);
/**
 @brief Add an overwrite operation to complete on commit.
 @param self The storage object.
 @param fileID The file ID
 @param data The data to write.
 @param offset The offset to begin writting.
 @param dataLen The length of the data to write.
 @retruns true on success and false on failure
 */
bool CBBlockChainStorageAddOverwrite(CBBlockChainStorage * self, uint16_t fileID, uint8_t * data, uint64_t offset, uint32_t dataLen);
/**
 @brief Find key position.
 @param self The storage object.
 @param key The key to find.
 @param found Will be set to true if found or false if not.
 @retruns The position of the key.
 */
uint32_t CBBlockChainStorageFindKey(CBBlockChainStorage * self, uint8_t key[6], bool * found);
/**
 @brief Does a quicksort on the block chain database index.
 @param self The storage object.
 @retruns true on success and false on failure.
 */
bool CBBlockChainStorageQuickSortIndex(CBBlockChainStorage * self);

#endif
