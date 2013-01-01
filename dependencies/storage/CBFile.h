//
//  CBFile.h
//  cbitcoin
//
//  Created by Matthew Mitchell on 28/12/2012.
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
 @brief Functions for file IO using hamming 72,64 endoding.
 */

#ifndef CBFILEH
#define CBFILEH

#include "CBHamming72.h"
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

/**
 @brief Contains a file pointer and the length of the data.
 */
typedef struct{
	FILE * rdwr; /**< The file pointer with the mode "rb+" or "wb+" if the file was opened with the new argument as true. */
	uint32_t dataLength; /**< The length of the data being held in the file and not including other data such as hamming code parity. */
	uint32_t cursor; /**< The cursor through the held data, starting at 0. */
	bool new; /**< True if the file is opened with "new" as true. */
} CBFile;

/**
 @brief Writes to the end of a file. The file should be seeked again, after using this function, as the cursor will be broken otherwise.
 @param file The file to write to.
 @param data The data to write.
 @param dataLen The data length.
 @returns true on success and false on failure.
 */
bool CBFileAppend(CBFile * file, uint8_t * data, uint32_t dataLen);
/**
 @brief Closes a file.
 @param file The file to close.
 */
void CBFileClose(CBFile * file);
/**
 @brief Opens a file, with read, write and append modes. Several modes are required for error correction.
 @param file @see CBFile. The file object will be initialised.
 @param filename The path of the file.
 @param new If true, a new file will be created and overwrite any existing files.
 @returns true on success and false on failure.
 */
bool CBFileOpen(CBFile * file, char * filename, bool new);
/**
 @brief Writes to a file, overwriting existing data. The cursor will be moved along "dataLen".
 @param file The file to write to.
 @param data The data to write.
 @param dataLen The data length.
 @returns true on success and false on failure.
 */
bool CBFileOverwrite(CBFile * file, uint8_t * data, uint32_t dataLen);
/**
 @brief Reads from a file
 @param file The file to read from.
 @param data The data to set.
 @param dataLen The length of the read.
 @returns true on success and false on failure.
 */
bool CBFileRead(CBFile * file, uint8_t * data, uint32_t dataLen);
/**
 @brief Reads the length from a file pointer.
 @param rd The file pointer to read the length from. The cursor in the file pointer should be where the length starts.
 @param length The length to set.
 @returns true on success and false on failure.
 */
bool CBFileReadLength(FILE * rd, uint32_t * length);
/**
 @brief Seeks to a position in the file.
 @param file The file to seek.
 @param pos The data position in the file.
 @returns true on success and false on failure.
 */
bool CBFileSeek(CBFile * file, uint32_t pos);
/**
 @brief Synchronises a file to disk.
 @param file The file to synchronise
 @returns true on success and false on failure.
 */
bool CBFileSync(CBFile * file);
/**
 @brief Synchronises a directory to disk.
 @param dir The path of the directory to synchronise
 @returns true on success and false on failure.
 */
bool CBFileSyncDir(char * dir);
/**
 @brief Truncates the file to a new size
 @param filename The path of the file to truncate.
 @param newSize The data position in the file.
 @returns true on success and false on failure.
 */
bool CBFileTruncate(char * filename, uint32_t newSize);
/**
 @brief Writes to a section were an overwrite or append will start midway through.
 @param rd The file pointer whcih is readable.
 @param offset The offset to the start of the write.
 @param data A pointer to the data pointer.
 @param dataLen A pointer to the data length.
 @returns The insert length on success and 0 on failure.
 */
uint8_t CBFileWriteMidway(FILE * rd, uint8_t offset, uint8_t ** data, uint32_t * dataLen);

#endif
