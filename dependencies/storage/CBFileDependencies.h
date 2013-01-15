//
//  CBFileDependencies.h
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
 @brief Weak linked functions for file IO.
 */

#ifndef CBFILEDEPENDENCIESH
#define CBFILEDEPENDENCIESH

#include <stdint.h>
#include <stdbool.h>

// Weak linking pragmas

#pragma weak CBFileAppend
#pragma weak CBFileClose
#pragma weak CBFileGetLength
#pragma weak CBFileOpen
#pragma weak CBFileOverwrite
#pragma weak CBFileRead
#pragma weak CBFileSeek
#pragma weak CBFileSync
#pragma weak CBFileSyncDir
#pragma weak CBFileTruncate

/**
 @brief Writes to the end of a file. The file should be seeked again, after using this function, as the cursor will be broken otherwise.
 @param file The file to write to.
 @param data The data to write.
 @param dataLen The data length.
 @returns true on success and false on failure.
 */
bool CBFileAppend(uint64_t file, uint8_t * data, uint32_t dataLen);
/**
 @brief Closes a file.
 @param file The file to close.
 */
void CBFileClose(uint64_t file);
/**
 @brief Returns the length of a file.
 @param file The file to get the length for.
 @param length The length to set.
 @returns true on success and false on failure.
 */
bool CBFileGetLength(uint64_t file, uint32_t * length);
/**
 @brief Opens a file, with read, write and append modes. Several modes are required for error correction.
 @param filename The path of the file.
 @param new If true, a new file will be created and overwrite any existing files.
 @returns The file object as an integer on success and false on failure.
 */
uint64_t CBFileOpen(char * filename, bool new);
/**
 @brief Writes to a file, overwriting existing data. The cursor will be moved along "dataLen".
 @param file The file to write to.
 @param data The data to write.
 @param dataLen The data length.
 @returns true on success and false on failure.
 */
bool CBFileOverwrite(uint64_t file, uint8_t * data, uint32_t dataLen);
/**
 @brief Reads from a file
 @param file The file to read from.
 @param data The data to set.
 @param dataLen The length of the read.
 @returns true on success and false on failure.
 */
bool CBFileRead(uint64_t file, uint8_t * data, uint32_t dataLen);
/**
 @brief Seeks to a position in the file.
 @param file The file to seek.
 @param pos The data position in the file.
 @returns true on success and false on failure.
 */
bool CBFileSeek(uint64_t file, uint32_t pos);
/**
 @brief Synchronises a file to disk.
 @param file The file to synchronise
 @returns true on success and false on failure.
 */
bool CBFileSync(uint64_t file);
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

#endif
