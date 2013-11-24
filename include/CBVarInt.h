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

#include "CBByteArray.h"

/**
 @brief Contains decoded variable size integer information. @see CBVarInt.h
 */
typedef struct{
	uint64_t val; /**< Integer value */
	uint8_t size; /**< Size of the integer when encoded in bytes */
}CBVarInt;

/**
 @brief Decodes variable size integer from bytes into a CBVarInt structure.
 @param bytes The byte array to decode a variable size integer from.
 @param offset Offset to start decoding from.
 @returns The CBVarInt information
 */
CBVarInt CBVarIntDecode(CBByteArray * bytes, uint32_t offset);
CBVarInt CBVarIntDecodeData(uint8_t * bytes, uint32_t offset);
/**
 @brief Encodes variable size integer into bytes.
 @param bytes The byte array to encode a variable size integer into.
 @param offset Offset to start decoding from.
 @param varInt Variable integer structure.
 */
void CBVarIntEncode(CBByteArray * bytes, uint32_t offset, CBVarInt varInt);
/**
 @brief Returns a variable integer from a 64 bit integer.
 @param integer The 64 bit integer
 @returns A CBVarInt.
 */
CBVarInt CBVarIntFromUInt64(uint64_t integer);
/**
 @brief Returns the variable integer byte size of a 64 bit integer
 @param value The 64 bit integer
 @returns The size of a variable integer for this integer.
 */
uint8_t CBVarIntSizeOf(uint64_t value);

#endif
