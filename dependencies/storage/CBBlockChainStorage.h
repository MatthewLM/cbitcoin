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

#define CB_MAX_VALUE_WRITES 3

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
 @brief Describes a write operation.
 */
typedef struct{
	uint8_t key[6]; /**< The key for the value */
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
	CBKeyValuePair * index; /**< Index of all key/value pairs */
	uint32_t numValues; /**< The number of key/value pairs */
	uint16_t lastFile; /**< The last file ID. */
	uint32_t lastSize; /**< Size of last file */
	CBDeletedSection * deletionIndex; /**< Index of all deleted sections */
	uint32_t numDeletions; /**< Number of deletion entries */
	CBWriteValue valueWrites[CB_MAX_VALUE_WRITES]; /**< Values to write */
	uint8_t numValueWrites; /**< The number of values to write */
	void (*logError)(char *,...);
} CBBlockChainStorage;

// Additional functions

/**
 @brief Removes all of the value write operations.
 @param self The storage object.
 */
void CBResetBlockChainStorage(CBBlockChainStorage * self);
/**
 @brief Add an append operation.
 @param self The storage object.
 @param fileID The file ID
 @param data The data to write.
 @param dataLen The length of the data to write.
 @retruns true on success and false on failure
 */
bool CBBlockChainStorageAddAppend(CBBlockChainStorage * self, uint16_t fileID, uint8_t * data, uint32_t dataLen);
/**
 @brief Add a deletion entry.
 @param self The storage object.
 @param fileID The file ID
 @param pos The position of the deleted section.
 @param len The length of the deleted section.
 @retruns true on success and false on failure
 */
bool CBBlockChainStorageAddDeletionEntry(CBBlockChainStorage * self, uint16_t fileID, uint32_t pos, uint32_t len);
/**
 @brief Add an overwrite operation.
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

#endif
