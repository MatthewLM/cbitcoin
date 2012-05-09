//
//  CBBase58.h
//  cbitcoin
//
//  Created by Matthew Mitchell on 28/04/2012.
//  Last modified by Matthew Mitchell on 29/04/2012.
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

static const char base58Characters[58] = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";

#include <stdlib.h>
#include "CBBigInt.h"

/**
 @brief Decodes base 58 string into byte data.
 @param bytes Pointer to byte data. Must point to memory large enough.
 @param str Base 58 string to decode.
 */
void CBDecodeBase58(u_int8_t * bytes,char * str);
/**
 @brief Decodes base 58 string into byte data. Checks a 4 byte checksum.
 @param bytes Pointer to byte data. Must point to memory large enough.
 @param str Base 58 string to decode.
 */
void CBDecodeBase58Checked(u_int8_t * bytes,char * str);
/**
 @brief Encodes byte data into base 58.
 @param str Pointer to a string to be filled with base 58 data.
 @param bytes Pointer to byte data to encode. Will almost certainly be modified. Copy data beforehand if needed.
 @param len Length of bytes to encode.
 */
void CBEncodeBase58(char * str, u_int8_t * bytes, u_int8_t len);

#endif