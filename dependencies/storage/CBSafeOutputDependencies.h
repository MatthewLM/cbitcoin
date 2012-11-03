//
//  CBSafeOutputDependencies.h
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
 @brief These are the file IO dependencies for CBSafeOutput. It is seperated to allow developers to plugin alternative implementations.
 */

#ifndef CBSAFEOUTPUTDEPENDENCIESH
#define CBSAFEOUTPUTDEPENDENCIESH

#include "CBSafeOutputConstants.h"
#include <string.h>
#include <stdbool.h>

// Weak linking for file-IO functions

#pragma weak CBFileOpen
#pragma weak CBFileRead
#pragma weak CBFileWrite
#pragma weak CBFileSeek
#pragma weak CBFileRename
#pragma weak CBFileTruncate
#pragma weak CBFileDelete
#pragma weak CBFileGetSize
#pragma weak CBGetMaxFileSize
#pragma weak CBFileDiskSynchronise
#pragma weak CBFileExists
#pragma weak CBEOF
#pragma weak CBFileClose

// FILE-IO DEPENENCIES

/**
 @brief Opens a file for use with the other IO functions, or opens a directory with CB_FILE_MODE_NONE.
 @param filename The path of the file to open.
 @param mode The mode required for this file.
 @returns A void pointer as a reference for this file or NULL on failure.
 */
void * CBFileOpen(char * filename, CBFileMode mode);
/**
 @brief Reads data from a file which has been opened with CB_FILE_MODE_READ.
 @param file The file pointer.
 @param buffer A buffer to read the data into.
 @param size The size of the data to read in bytes.
 @returns true if the read succeeded or false if the read failed.
 */
bool CBFileRead(void * file, void * buffer, size_t size);
/**
 @brief Writes to a file. If the file was opened with CB_FILE_MODE_APPEND, the data is written to the end of the file.
 @param file The file pointer.
 @param buffer The data to write.
 @param size The size of the data to write in bytes.
 @returns true if the write succeeded or false if the write failed.
 */
bool CBFileWrite(void * file, void * buffer, size_t size);
/**
 @brief Seeks the position of the file cursor. This moves the cursor an offset from the origin.
 @param file The file pointer.
 @param offset The offset to move the file cursor.
 @param orgin The orgin of the offset. If CB_SEEK_SET, the origin is 0. If CB_SEEK_CUR, the origin is the current cursor position.
 @returns true if the seel succeeded or false if the seek failed.
 */
bool CBFileSeek(void * file, long int offset, CBFileSeekOrigin origin);
/**
 @brief Renames a file.
 @param filename The path of the file to rename.
 @param newname The new path.
 @returns true if the rename succeeded or false if the rename failed.
 */
bool CBFileRename(void * filename, void * newname);
/**
 @brief Truncates a file opened with CB_FILE_MODE_TRUNCATE.
 @param filename The file name of the file to truncate.
 @param size The new size of the file.
 @returns true if the truncate succeeded or false if the truncate failed.
 */
bool CBFileTruncate(char * filename, size_t size);
/**
 @brief Deletes a file.
 @param filename The path of the file to delete.
 @returns true if the delete succeeded or false if the delete failed.
 */
bool CBFileDelete(void * filename);
/**
 @brief Gets the size, in bytes, of a file.
 @param filename The path of the file to obtain the size for.
 @returns The size of the file or -1 on failure.
 */
size_t CBFileGetSize(void * filename);
/**
 @brief Gets the maximum allowed size of a file on the filesystem.
 @returns The maximum allowed size of a file or -1 on failure.
 */
size_t CBGetMaxFileSize(void);
/**
 @brief Synchronises a file or directory to permenant storage as much as possibly can be guarenteed. When synchronising a directory, changes to filenames and file deletions should be synchronised.
 @param ptr The pointer of the file or directory opened with CBFileOpen.
 @returns true if the synchronisation succeeded or false if the synchronisation failed.
 */
bool CBFileDiskSynchronise(void * ptr);
/**
 @brief Deteremines if a file exists
 @param filename The path to the file.
 @returns true if the file exists and false otherwise.
 */
bool CBFileExists(char * filename);
/**
 @brief Deteremines if the end of a file has been reached.
 @param file The pointer of the file or directory opened with CBFileOpen.
 @returns true if the end of the file has been reached, else false.
 */
bool CBEOF(void * file);
/**
 @brief Closes a file or directory opened with CBFileOpen and frees any relevant data.
 @param ptr The pointer of the file or directory opened with CBFileOpen.
 */
void CBFileClose(void * file);


#endif
