//
//  CBFileNoEC.h
//  cbitcoin
//
//  Created by Matthew Mitchell on 06/01/2013.
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
 @brief Functions for file IO with no error correction.
 */

#ifndef CBFILEH
#define CBFILEH

#include "CBHamming72.h"
#include "CBFileDependencies.h"
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

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
 @brief Reads the length from a file pointer.
 @param rd The file pointer to read the length from. The cursor in the file pointer should be where the length starts.
 @param length The length to set.
 @returns true on success and false on failure.
 */
bool CBFileReadLength(FILE * rd, uint32_t * length);
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
