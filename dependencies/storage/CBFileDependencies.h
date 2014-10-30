//
//  CBFileDependencies.h
//  cbitcoin
//
//  Created by Matthew Mitchell on 28/12/2012.
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
 @brief Weak linked functions for file IO.
 */

#ifndef CBFILEDEPENDENCIESH
#define CBFILEDEPENDENCIESH

#include <stdint.h>
#include <stdbool.h>
#include "CBDependencies.h"

// Weak linking pragmas

#pragma weak CBFileAppend
#pragma weak CBFileClose
#pragma weak CBFileGetLength
#pragma weak CBFileOpen
#pragma weak CBFileOverwrite
#pragma weak CBFilePos
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
bool CBFileAppend(CBDepObject file, uint8_t * data, uint32_t dataLen);

/**
 @brief Writes to the end of a file a number of zeros. The file should be seeked again, after using this function, as the cursor will be broken otherwise.
 @param file The file to write to.
 @param amount The number of zero bytes to append.
 @returns true on success and false on failure.
 */
bool CBFileAppendZeros(CBDepObject file, uint32_t amount);

/**
 @brief Closes a file.
 @param file The file to close.
 */
void CBFileClose(CBDepObject file);

/**
 @brief Returns the length of a file.
 @param file The file to get the length for.
 @param length The length to set.
 @returns true on success and false on failure.
 */
bool CBFileGetLength(CBDepObject file, uint32_t * length);

/**
 @brief Opens a file, with read, write and append modes. Several modes are required for error correction.
 @param filename The path of the file.
 @param new If true, a new file will be created and overwrite any existing files.
 @returns true on success and false on failure.
 */
bool CBFileOpen(CBDepObject * file, char * filename, bool new);

/**
 @brief Writes to a file, overwriting existing data. The cursor will be moved along "dataLen".
 @param file The file to write to.
 @param data The data to write.
 @param dataLen The data length.
 @returns true on success and false on failure.
 */
bool CBFileOverwrite(CBDepObject file, uint8_t * data, uint32_t dataLen);
uint32_t CBFilePos(CBDepObject file);

/**
 @brief Reads from a file
 @param file The file to read from.
 @param data The data to set.
 @param dataLen The length of the read.
 @returns true on success and false on failure.
 */
bool CBFileRead(CBDepObject file, uint8_t * data, uint32_t dataLen);

/**
 @brief Seeks to a position in the file.
 @param file The file to seek.
 @param pos The data position in the file.
 @returns true on success and false on failure.
 */
bool CBFileSeek(CBDepObject file, int32_t pos, int seek);

/**
 @brief Synchronises a file to disk.
 @param file The file to synchronise
 @returns true on success and false on failure.
 */
bool CBFileSync(CBDepObject file);

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
