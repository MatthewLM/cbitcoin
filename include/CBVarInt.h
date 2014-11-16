//
//  CBVarInt.h
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
 @brief Simple structure for variable size integers information. This is an annoying structure used in the bitcoin protocol. One has to wonder why it was ever used.
 */

#ifndef CBVARINTH
#define CBVARINTH

#include <stdbool.h>
#include <stdint.h>
#include "CBConstants.h"

/**
 @brief Contains decoded variable size integer information. @see CBVarInt.h
 */
typedef struct{
	long long int val; /**< Integer value */
	int size; /**< Size of the integer when encoded in bytes */
}CBVarInt;

CBVarInt CBVarIntDecodeData(unsigned char * bytes, int offset);
int CBVarIntDecodeSize(unsigned char * bytes, int offset);

/**
 @brief Encodes variable size integer into bytes.
 @param bytes The bytes to encode a variable size integer into.
 @param offset Offset to start encoding to.
 @param varInt Variable integer structure.
 */
void CBByteArraySetVarIntData(unsigned char * bytes, int offset, CBVarInt varInt);

/**
 @brief Returns a variable integer from a 64 bit integer.
 @param integer The 64 bit integer
 @returns A CBVarInt.
 */
CBVarInt CBVarIntFromUInt64(long long int integer);

/**
 @brief Returns the variable integer byte size of a 64 bit integer
 @param value The 64 bit integer
 @returns The size of a variable integer for this integer.
 */
int CBVarIntSizeOf(long long int value);

#endif
