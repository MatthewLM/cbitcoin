//
//  CBSafeOutputConstants.h
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
 @brief Constants for CBSafeOutput
 */

#ifndef CBSAFEOUTPUTCONSTANTSH
#define CBSAFEOUTPUTSONSTANTSH

/**
 @brief The type of output operations to apply to a file.
 */
typedef enum{
	CB_SAFE_OUTPUT_OP_SAVE, /**< A save operation */
	CB_SAFE_OUTPUT_OP_OVERWRITE_APPEND, /**< Overwrite and append operations */
	CB_SAFE_OUTPUT_OP_DELETE, /**< A delete operation. */
	CB_SAFE_OUTPUT_OP_RENAME, /**< A rename operation. */
	CB_SAFE_OUTPUT_OP_TRUNCATE, /**< A truncate operation. */
} CBSafeOutputOperation;

/**
 @brief The file mode required for a file, passed to CBFileOpen
 */
typedef enum{
	CB_FILE_MODE_READ, /**< Open to read binary data from the file (ie. rb). */
	CB_FILE_MODE_OVERWRITE, /**< Open to overwrite (ie. update) binary contents of the file. (ie. rb+) */
	CB_FILE_MODE_SAVE, /**< Open to save new binary contents to the file (ie. wb) */
	CB_FILE_MODE_APPEND, /**< Open to append binary data to the file (ie. ab) */
	CB_FILE_MODE_WRITE_AND_READ, /**< Open to write and read binary data to the file. When opened the file should be cleared. If it does not exist it should be created (ie. wb+). */
	CB_FILE_MODE_NONE, /**< Open file or directory with no read/write mode specified but which can be used to synchronise to disk. */
} CBFileMode;

typedef enum{
	CB_SEEK_SET,
	CB_SEEK_CUR,
} CBFileSeekOrigin;

#endif
