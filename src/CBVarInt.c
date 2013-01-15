//
//  CBVarInt.h
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

//  SEE HEADER FILE FOR DOCUMENTATION

#include "CBVarInt.h"

CBVarInt CBVarIntDecode(CBByteArray * bytes, uint32_t offset){
	uint8_t first = CBByteArrayGetByte(bytes, offset);
	CBVarInt result;
	if (first < 253) {
		// 8 bits.
		result.val = first;
		result.size = 1;
	} else if (first == 253) { 
		// 16 bits.
		result.val = CBByteArrayReadInt16(bytes, offset + 1);
		result.size = 3;
	} else if (first == 254) {
		// 32 bits.
		result.val = CBByteArrayReadInt32(bytes, offset + 1);
		result.size = 5;
	} else {
		// 64 bits.
		result.val = CBByteArrayReadInt64(bytes, offset + 1);
		result.size = 9;
	}
	return result;
}
void CBVarIntEncode(CBByteArray * bytes, uint32_t offset, CBVarInt varInt){
	switch (varInt.size) {
		case 1:
			CBByteArraySetByte(bytes, offset, (uint8_t)varInt.val);
			break;
		case 3:
			CBByteArraySetByte(bytes, offset, 253);
			CBByteArraySetInt16(bytes, offset + 1, (uint16_t)varInt.val);
			break;
		case 5:
			CBByteArraySetByte(bytes, offset, 254);
			CBByteArraySetInt32(bytes, offset + 1, (uint32_t)varInt.val);
			break;
		case 9:
			CBByteArraySetByte(bytes, offset, 255);
			CBByteArraySetInt64(bytes, offset + 1, varInt.val);
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
	else if (value < 65536)
		return 3;  // 1 marker + 2 data bytes
	else if (value < 4294967296)
		return 5;  // 1 marker + 4 data bytes
	return 9; // 1 marker + 4 data bytes
}

