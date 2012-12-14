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
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "CBDependencies.h"
#include "CBAssociativeArray.h"

#define CB_MAX_VALUE_WRITES 7

/**
 @brief An index value which references the value's data position with a key. This should occur in memory after a key. A key is one byte for the length and then the key bytes.
 */
typedef struct{
	uint32_t indexPos; /**< The position in the index file where this value exists */
	uint16_t fileID; /**< The file ID for the data */
	uint32_t pos; /**< The position of the data in the file. */
	uint32_t length; /**< The length of the data or 0 if deleted. */
} CBIndexValue;

/**
 @brief Describes a deleted section of the database. The key for this deleted section which begins with 0x01 if the deleted section is active or 0x00 if it is no longer active and ends with four bytes for the length of the deleted section in big-endian. The key becomes before this structure in memory.
 */
typedef struct{
	uint8_t key[5];
	uint32_t indexPos; /**< The position in the index file where this value exists */
	uint16_t fileID; /**< The ID of the file with the deleted section. */
	uint32_t pos; /**< The position in the file with the deleted section. */
} CBDeletedSection;

/**
 @brief Describes a write operation.
 */
typedef struct{
	uint8_t * key; /**< The key with the length being the first byte */
	uint32_t offset; /**< Offset to begin writting */
	uint8_t * data; /**< The data to write */
	uint32_t dataLen; /**< The length of the data to write */
	uint32_t allocLen; /**< The length allocated for this value */
	uint32_t totalLen; /**< The length of the total value entry */
} CBWriteValue;

/**
 @brief Structure for CBBlockChainStorage objects. @see CBBlockChainStorage.h
 */
typedef struct{
	char * dataDir; /**< The data directory. */
	CBAssociativeArray index; /**< Index of all key/value pairs */
	uint32_t numValues; /**< Number of values in the index */
	uint16_t lastFile; /**< The last file ID. */
	uint32_t lastSize; /**< Size of last file */
	CBAssociativeArray deletionIndex; /**< Index of all deleted sections. The key begins with 0x01 if the deleted section is active or 0x00 if it is no longer active, the key is then followed by the length in big endian. */
	uint32_t numDeletionValues; /**< Number of values in the deletion index */
	CBWriteValue valueWrites[CB_MAX_VALUE_WRITES]; /**< Values to write */
	uint8_t numValueWrites; /**< The number of values to write */
	uint8_t ** deleteKeys; /**< A list of keys to delete with the first byte being the length and the remaining bytes being the key data. */
	uint32_t numDeleteKeys; /**< Number of keys to delete Similar structure to "deleteKeys" but with the old key followed by the new key.  */
	uint8_t *(* changeKeys)[2]; /**< A list of keys to be changed with the last bytes being the new key. */
	uint32_t numChangeKeys;
	uint32_t nextIndexPos; /**< The next position for an index */
	void (*logError)(char *,...);
} CBBlockChainStorage;

// Additional functions

/**
 @brief Add a deletion entry.
 @param self The storage object.
 @param fileID The file ID
 @param pos The position of the deleted section.
 @param len The length of the deleted section.
 @param logFile The file descriptor for the log file.
 @param dir The file descriptor for the data directory.
 @retruns true on success and false on failure
 */
bool CBBlockChainStorageAddDeletionEntry(CBBlockChainStorage * self, uint16_t fileID, uint32_t pos, uint32_t len, int logFile, int dir);
/**
 @brief Add an overwrite operation.
 @param self The storage object.
 @param fileID The file ID
 @param data The data to write.
 @param offset The offset to begin writting.
 @param dataLen The length of the data to write.
 @param logFile The file descriptor for the log file.
 @param dir The file descriptor for the data directory.
 @retruns true on success and false on failure
 */
bool CBBlockChainStorageAddOverwrite(CBBlockChainStorage * self, uint16_t fileID, uint8_t * data, uint64_t offset, uint32_t dataLen, int logFile, int dir);
/**
 @brief Adds a value to the database without overwriting previous indexed data.
 @param self The storage object.
 @param writeValue Information for writting the value.
 @param indexValue The index data to write.
 @param logFile The file descriptor for the log file.
 @param dir The file descriptor for the data directory.
 @retruns true on success and false on failure
 */
bool CBBlockChainStorageAddValue(CBBlockChainStorage * self, CBWriteValue writeValue, CBIndexValue * indexValue, int logFile, int dir);
/**
 @brief Add an append operation.
 @param self The storage object.
 @param fileID The file ID
 @param data The data to write.
 @param dataLen The length of the data to write.
 @retruns true on success and false on failure
 */
bool CBBlockChainStorageAppend(CBBlockChainStorage * self, uint16_t fileID, uint8_t * data, uint32_t dataLen);
/**
 @brief Returns a CBFindResult for largest active deleted section and "found" will be true if the largest active deleted section is above a length.
 @param self The storage object.
 @param length The minimum length required.
 @returns The largest active deleted section as a CBFindResult.
 */
CBFindResult CBBlockChainStorageGetDeletedSection(CBBlockChainStorage * self, uint32_t length);

#endif
