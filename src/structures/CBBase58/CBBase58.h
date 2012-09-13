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
 @brief Functions for encoding and decoding in base 58. Avoids 0olI whch may look alike. This is for readability concerns.
 */

#ifndef CBBASE58H
#define CBBASE58H

#include <stdlib.h>
#include "CBBigInt.h"
#include "CBEvents.h"
#include "CBDependencies.h"

static const char base58Characters[58] = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";

/**
 @brief Decodes base 58 string into byte data as a CBBigInt.
 @param str Base 58 string to decode.
 @returns Pointer to the byte data as a CBBigInt. The byte data will be created in this function. Remember to free the data. On error the big int will have a NULL data pointer.
 */
CBBigInt CBDecodeBase58(char * str);
/**
 @brief Decodes base 58 string into byte data as a CBBigInt and checks a 4 byte checksum.
 @param str Base 58 string to decode.
 @returns Byte data as a CBBigInt. Is zero on failure. Checksum is included in returned data. On error the big int will have a NULL data pointer.
 */
CBBigInt CBDecodeBase58Checked(char * str,void (*onErrorReceived)(CBError error,char *,...));
/**
 @brief Encodes byte data into base 58.
 @param bytes Pointer to byte data to encode. Will almost certainly be modified. Copy data beforehand if needed.
 @param len Length of bytes to encode.
 @returns Newly allocated string with encoded data or NULL on error.
 */
char * CBEncodeBase58(uint8_t * bytes, uint8_t len);

#endif
