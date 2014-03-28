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

//  SEE HEADER FILE FOR DOCUMENTATION

#include "CBVarInt.h"

CBVarInt CBVarIntDecodeData(uint8_t * bytes, uint32_t offset){
	CBVarInt result;
	result.size = CBVarIntDecodeSize(bytes, offset);
	if (result.size == 1) result.val = bytes[offset];
	else if (result.size == 3) result.val = CBArrayToInt16(bytes, offset + 1);
	else if (result.size == 5) result.val = CBArrayToInt32(bytes, offset + 1);
	else result.val = CBArrayToInt64(bytes, offset + 1);
	return result;
}
uint8_t CBVarIntDecodeSize(uint8_t * bytes, uint32_t offset) {
	if (bytes[offset] < 253)
		// 8 bits.
		return 1;
	if (bytes[offset] == 253)
		// 16 bits.
		return 3;
	if (bytes[offset] == 254)
		// 32 bits.
		return 5;
	// 64 bits.
	return 9;
}
void CBByteArraySetVarIntData(uint8_t * bytes, uint32_t offset, CBVarInt varInt) {
	switch (varInt.size) {
		case 1:
			bytes[offset] = (uint8_t)varInt.val;
			break;
		case 3:
			bytes[offset] = 253;
			CBInt16ToArray(bytes, offset + 1, varInt.val);
			break;
		case 5:
			bytes[offset] = 254;
			CBInt32ToArray(bytes, offset + 1, varInt.val);
			break;
		case 9:
			bytes[offset] = 255;
			CBInt64ToArray(bytes, offset + 1, varInt.val);
			break;
	}
}
CBVarInt CBVarIntFromUInt64(uint64_t integer){
	CBVarInt varInt;
	varInt.val = integer;
	varInt.size = CBVarIntSizeOf(integer);
	return varInt;
}
uint8_t CBVarIntSizeOf(uint64_t value){
	if (value < 253)
		return 1;
	else if (value <= UINT16_MAX)
		return 3;  // 1 marker + 2 data bytes
	else if (value <= UINT32_MAX)
		return 5;  // 1 marker + 4 data bytes
	return 9; // 1 marker + 4 data bytes
} 

