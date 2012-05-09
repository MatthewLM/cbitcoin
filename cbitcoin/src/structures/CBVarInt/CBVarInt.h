//
//  CBVarInt.h
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
 @brief Simple structure for variable size integers information.
 */

#ifndef CBVARINTH
#define CBVARINTH

#include "CBByteArray.h"

/**
 @brief Contains decoded variable size integer information. @see CBVarInt.h
 */
typedef struct{
	u_int64_t val; //*< Integer value */
	u_int8_t size; //*< Size of the integer when encoded in bytes */
}CBVarInt;

/**
 @brief Decodes variable size integer from bytes into a CBVarInt structure.
 @param bytes The byte array to decode a variable size integer from.
 @param offset Offset to start decoding from.
 @returns The CBVarInt information
 */
CBVarInt CBVarIntDecode(CBByteArray * bytes,u_int32_t offset);
/**
 @brief Returns the variable integer byte size of an integer
 @param value The integer
 @returns The size of a variable integer for this integer.
 */
u_int8_t CBVarIntSizeOf(u_int32_t value);

#endif