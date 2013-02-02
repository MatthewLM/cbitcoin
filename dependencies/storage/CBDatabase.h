//
//  CBDatabase.h
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
 @brief Implements a database for use with the storage requirements.
 */

#ifndef CBDATABASEH
#define CBDATABASEH

#include "CBDependencies.h"
#include "CBAssociativeArray.h"
#include "CBFileDependencies.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

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
 @brief Describes a deleted section of the database.
 */
typedef struct{
	uint8_t key[12]; /**< The key for this deleted section which begins with 0x01 if the deleted section is active or 0x00 if it is no longer active, then has four bytes for the length of the deleted section in big-endian, then the file ID in little-endian and finally the offset of the deleted section in little-endian. */
	uint32_t indexPos; /**< The position in the index file where this value exists */
} CBDeletedSection;

/**
 @brief Structure for CBDatabase objects. @see CBDatabase.h
 */
typedef struct{
	char * dataDir; /**< The data directory. */
	char * prefix; /**< The file prefix. */
	CBAssociativeArray index; /**< Index of all key/value pairs */
	uint32_t numValues; /**< Number of values in the index */
	uint16_t lastFile; /**< The last file ID. */
	uint32_t lastSize; /**< Size of last file */
	CBAssociativeArray deletionIndex; /**< Index of all deleted sections. The key begins with 0x01 if the deleted section is active or 0x00 if it is no longer active, the key is then followed by the length in big endian. */
	uint32_t numDeletionValues; /**< Number of values in the deletion index */
	CBAssociativeArray valueWrites; /**< Values to write, the key followed by a 32 bit integer for the data length and then the data. */
	CBAssociativeArray deleteKeys; /**< An array of keys to delete with the first byte being the length and the remaining bytes being the key data. */
	uint8_t *(* changeKeys)[2]; /**< A list of keys to be changed with the last bytes being the new key. */
	uint32_t numChangeKeys;
	uint32_t nextIndexPos; /**< The next position for an index */
	// Files
	uint64_t indexFile;
	uint64_t deletionIndexFile;
	uint64_t fileObjectCache; /**< Stores last used file object for reuse until new file is needed */
	uint16_t lastUsedFileObject; /**< The last used file number or 0 if none. */
} CBDatabase;

// Initialisation

/**
 @brief Returns a new database object.
 @param dataDir The directory where the data files should be stored.
 @param prefix A prefix for the data files to prevent conflicts.
 @returns The database object or 0 on failure.
 */
CBDatabase * CBNewDatabase(char * dataDir, char * prefix);
/**
 @brief Initialises a database object.
 @param self The CBDatabase object to initialise.
 @param dataDir The directory where the data files should be stored.
 @param prefix A prefix for the data files to prevent conflicts.
 @returns true on success and false on failure.
 */
bool CBInitDatabase(CBDatabase * self, char * dataDir, char * prefix);
/**
 @brief Reads and opens the index during initialisation
 @param self The storage object.
 @param filename The index filename.
 @returns true on success or false on failure.
 */
bool CBDatabaseReadAndOpenIndex(CBDatabase * self, char * filename);
/**
 @brief Creates an index during the first initialisation
 @param self The storage object.
 @param filename The index filename.
 @returns true on success or false on failure.
 */
bool CBDatabaseCreateIndex(CBDatabase * self, char * filename);
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
 */
void CBFreeDatabase(CBDatabase * self);

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
bool CBDatabaseAddDeletionEntry(CBDatabase * self, uint16_t fileID, uint32_t pos, uint32_t len, uint64_t logFile);
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
bool CBDatabaseAddOverwrite(CBDatabase * self, uint16_t fileID, uint8_t * data, uint32_t offset, uint32_t dataLen, uint64_t logFile);
/**
 @brief Adds a value to the database without overwriting previous indexed data.
 @param self The storage object.
 @param dataSize The size of the data to write.
 @param data The data to write.
 @param indexValue The index data to write.
 @param logFile The file descriptor for the log file.
 @param dir The file descriptor for the data directory.
 @retruns true on success and false on failure
 */
bool CBDatabaseAddValue(CBDatabase * self, uint32_t dataSize, uint8_t * data, CBIndexValue * indexValue, uint64_t logFile);
/**
 @brief Adds a write value to the valueWrites array.
 @param self The storage object.
 @param writeValue The write value element data.
 @retruns true on success and false on failure
 */
bool CBDatabaseAddWriteValue(CBDatabase * self, uint8_t * writeValue);
/**
 @brief Add an append operation.
 @param self The database object.
 @param fileID The file ID
 @param data The data to write.
 @param dataLen The length of the data to write.
 @retruns true on success and false on failure
 */
bool CBDatabaseAppend(CBDatabase * self, uint16_t fileID, uint8_t * data, uint32_t dataLen);
/**
 @brief Replaces a key for a value with a key of the same length.
 @param self The database object.
 @param previousKey The current key to be replaced. The first byte is the length.
 @param newKey The new key for this value. The first byte is the length and should be the same as the first key.
 @returns true on success and false on failure.
 */
bool CBDatabaseChangeKey(CBDatabase * self, uint8_t * previousKey, uint8_t * newKey);
/**
 @brief Removes all of the pending value write, delete and change key operations.
 @param self The database object.
 */
void CBDatabaseClearPending(CBDatabase * self);
/**
 @brief The data is written to the disk.
 @param self The database object.
 @returns true on success and false on failure, and thus the database needs to be recovered with CBDatabaseEnsureConsistent.
 */
bool CBDatabaseCommit(CBDatabase * self);
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
 @param fileID The id of the file to get an object for.
 @returns A pointer to the file object on success and NULL on failure.
 */
uint64_t CBDatabaseGetFile(CBDatabase * self, uint16_t fileID);
/**
 @brief Gets the length of a value in the database or zero if it does not exist.
 @param self The database object.
 @param key The key. The first byte is the length.
 @returns The total length of the value or 0 if the value does not exist in the database.
 */
uint32_t CBDatabaseGetLength(CBDatabase * self, uint8_t * key);
/**
 @brief Queues a key-value read operation.
 @param self The database object.
 @param key The key for this data. The first byte is the length.
 @param data A pointer to memory with enough space to hold the data. The data will be set to this.
 @param dataSize The size to read.
 @param offset The offset to begin reading.
 @returns true on success and false on failure.
 */
bool CBDatabaseReadValue(CBDatabase * self, uint8_t * key, uint8_t * data, uint32_t dataSize, uint32_t offset);
/**
 @brief Queues a key-value delete operation.
 @param self The database object.
 @param key The key for this data. The first byte is the length.
 @returns true on success and false on failure.
 */
bool CBDatabaseRemoveValue(CBDatabase * self, uint8_t * key);
/**
 @brief Queues a key-value write operation from a many data parts. The data is concatenated.
 @param self The database object.
 @param key The key for this data. The first byte is the length.
 @param numDataParts The number of data parts.
 @param data A list of the data.
 @param dataSize A list of the data sizes
 @returns true on success and false on failure.
 */
bool CBDatabaseWriteConcatenatedValue(CBDatabase * self, uint8_t * key, uint8_t numDataParts, uint8_t ** data, uint32_t * dataSize);
/**
 @brief Queues a key-value write operation.
 @param self The database object.
 @param key The key for this data. The first byte is the length.
 @param data The data to store.
 @param size The size of the data to store. A new value replaces the old one.
 @returns true on success and false on failure.
 */
bool CBDatabaseWriteValue(CBDatabase * self, uint8_t * key, uint8_t * data, uint32_t size);

#endif
