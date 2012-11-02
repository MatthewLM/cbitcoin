//
//  CBSafeOutput.h
//  cbitcoin
//
//  Created by Matthew Mitchell on 20/10/2012.
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
 @brief Writes to the disk in a manner which is recoverable on failure. It writes a data backup to the disk before applying changes so that if the change fails (Such as sudden shutdown), recovery can be done back to the previous state.
 */

#ifndef CBSAFEOUTPUTH
#define CBSAFEOUTPUTH

#include "CBConstants.h"
#include "CBObject.h"
#include <string.h>

/**
 @brief Describes an overwrite operation that should be completed on a commit.
 */
typedef struct{
	long int offset; /**< The position in the file to start the write operation. */
	const void * data; /**< The data to write. */
	uint32_t size; /**< The number of bytes to write. */
} CBOverwriteOperation;

/**
 @brief Describes an append or save (Overwrite entire file) operation that should be completed on a commit.
 */
typedef struct{
	const void * data; /**< The data to write. */
	uint32_t size; /**< The number of bytes to write. */
} CBAppendOrSaveOperation;

/**
 @brief Append and overwrite operations
 */
typedef struct{
	CBOverwriteOperation * overwriteOperations; /**< A list of overwrite operations to be completed by the next commit */
	uint8_t numOverwriteOperations; /**< Number of overwrite operations */
	CBAppendOrSaveOperation * appendOperations; /**< A list of append operations to be completed by the next commit */
	uint8_t numAppendOperations; /**< Number of append operations */
} CBAppendAndOverwrite;

/**
 @brief A union containing the data for one type of output operation.
 */
union CBSafeOutputOperationData{
	CBAppendAndOverwrite appendAndOverwriteOps; /**< A list of overwrites and appendages to apply to this file. */
	CBAppendOrSaveOperation saveOp; /**< A write operation which wipes the entire file. */
	char * newName; /**< The new name of the file when renaming it. */
	size_t newSize; /**< The new size of the file when truncating it */
};

/**
 @brief A list of CBOutputOperation objects for a particular file.
 */
typedef struct{
	char * fileName; /**< The filename */
	union CBSafeOutputOperationData opData; /**< The data for the operation or operations. */
	CBSafeOutputOperation type; /**< Type of the operation or operations to apply to this file */
} CBFileOutputOperations;

/**
 @brief Structure for CBSafeOutput objects. @see CBSafeOutput.h
 */
typedef struct{
	CBObject base;
	CBFileOutputOperations * files; /**< List of files with the operations */
	uint8_t numFiles; /**< Number of files */
} CBSafeOutput;

/**
 @brief Creates a new CBSafeOutput object.
 @param directory The directory where the files exist.
 @returns A new CBSafeOutput object.
 */
CBSafeOutput * CBNewSafeOutput(void (*logError)(char *,...));

/**
 @brief Gets a CBSafeOutput from another object. Use this to avoid casts.
 @param self The object to obtain the CBSafeOutput from.
 @returns The CBSafeOutput object.
 */
CBSafeOutput * CBGetSafeOutput(void * self);

/**
 @brief Initialises a CBSafeOutput object
 @param self The CBSafeOutput object to initialise
 @returns true on success, false on failure.
 */
bool CBInitSafeOutput(CBSafeOutput * self, void (*logError)(char *,...));

/**
 @brief Frees a CBSafeOutput object.
 @param vself The CBSafeOutput object to free.
 */
void CBFreeSafeOutput(void * vself);

/**
 @brief Frees everything inside a CBSafeOutput object but not the object itself.
 @param vself The CBSafeOutput object.
 */
void CBFreeSafeOutputProcess(CBSafeOutput * self);

// Functions

/**
 @brief Adds an output append operation for the last file added. This appends data within a file using "ab"
 @param self The CBSafeOutput object.
 @param data The data to write.
 @param size The number of bytes to write.
 */
void CBSafeOutputAddAppendOperation(CBSafeOutput * self, const void * data, uint32_t size);
/**
 @brief Adds a delete operation for a file.
 @param self The CBSafeOutput object.
 @param fileName The filename
 */
void CBSafeOutputAddDeleteFileOperation(CBSafeOutput * self, char * fileName);
/**
 @brief Adds a new file to apply overwrite and append output operations on.
 @param self The CBSafeOutput object.
 @param fileName The filename
 @param overwriteOperations How many overwrite operations will be added for this file.
 @param appendOperations How many append operations will be added for this file.
 @returns true on success and false on failure.
 */
bool CBSafeOutputAddFile(CBSafeOutput * self, char * fileName, uint8_t overwriteOperations, uint8_t appendOperations);
/**
 @brief Adds an output overwrite operation for the last file added. This overwrites data within a file using "rb+"
 @param self The CBSafeOutput object.
 @param offset The position in the file to start the write operation.
 @param data The data to write.
 @param size The number of bytes to write.
 */
void CBSafeOutputAddOverwriteOperation(CBSafeOutput * self, long int offset, const void * data, uint32_t size);
/**
 @brief Adds a delete operation for a file.
 @param self The CBSafeOutput object.
 @param fileName The filename
 @param fileName The new filename
 */
void CBSafeOutputAddRenameFileOperation(CBSafeOutput * self, char * fileName, char * newName);
/**
 @brief Adds an output save operation for a new file. This wipes previous data and replaces it with new data within a file using "wb". This should not be combined with overwrite or append operations.
 @param self The CBSafeOutput object.
 @param fileName The filename
 @param data The data to write.
 @param size The number of bytes to write.
 */
void CBSafeOutputAddSaveFileOperation(CBSafeOutput * self, char * fileName, const void * data, uint32_t size);
/**
 @brief Allocates data for adding files.
 @param numFiles Number of files to apply operations to. This includes rename and delete operations.
 @returns true on success and false on failure.
 */
bool CBSafeOutputAlloc(CBSafeOutput * self,uint8_t numFiles);
/**
 @brief Commits the write operations to disk. This function will attempt to recover to the previous state if any errors have occured. If it does not recover the files the backup data will be available for future recovery with CBSafeOutputRecover. The CBOutputOperation objects are cleared after this function is complete.
 @param self The CBSafeOutput object.
 @param backup The backup.bak file object which should be opened with CB_FILE_MODE_WRITE_AND_READ.
 @returns @see CBSafeOutputResult
 */
CBSafeOutputResult CBSafeOutputCommit(CBSafeOutput * self, void * backup);
/**
 @brief Recovers files if a data backup can be found. This should be called before using any files from a previous program execution.
 @param backup The backup.bak file object which should be opened with CB_FILE_MODE_READ or CB_FILE_MODE_WRITE_AND_READ. 
 @returns true if the files are in an OK state or false if recovery failed.
 */
bool CBSafeOutputRecover(void * backup);

#endif
