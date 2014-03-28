//
//  CBBase58.h
//  cbitcoin
//
//  Created by Matthew Mitchell on 28/04/2012.
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
 */
void CBDecodeBase58(CBBigInt * bi, char * str);

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
