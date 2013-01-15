//
//  CBBase58.h
//  cbitcoin
//
//  Created by Matthew Mitchell on 28/04/2012.
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
 @brief Functions for encoding and decoding in base 58. Avoids "0", "o", "l", "I", which may look alike. This is due to readability concerns.
 */

#ifndef CBBASE58H
#define CBBASE58H

#include <stdlib.h>
#include "CBBigInt.h"
#include "CBDependencies.h"

static const char base58Characters[58] = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";

/**
 @brief Decodes base 58 string into byte data as a CBBigInt.
 @param bi The CBBigInt which should be preallocated with at least one byte.
 @param str Base 58 string to decode.
 @returns true on success, false on failure.
 */
bool CBDecodeBase58(CBBigInt * bi, char * str);
/**
 @brief Decodes base 58 string into byte data as a CBBigInt and checks a 4 byte checksum.
 @param bi The CBBigInt which should be preallocated with at least one byte.
 @param str Base 58 string to decode.
 @returns true on success, false on failure.
 */
bool CBDecodeBase58Checked(CBBigInt * bi, char * str);
/**
 @brief Encodes byte data into base 58.
 @param bytes Pointer to a normalised CBBigInt containing the byte data to encode. Will almost certainly be modified. Copy data beforehand if needed.
 @returns Newly allocated string with encoded data or NULL on error.
 */
char * CBEncodeBase58(CBBigInt * bi);

#endif
