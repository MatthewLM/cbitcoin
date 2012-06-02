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

CBVarInt CBVarIntDecode(CBByteArray * bytes,u_int32_t offset){
	char first = CBGetByteArrayVT(bytes)->getByte(bytes,offset);
	CBVarInt result;
	if (first < 253) {
		// 8 bits.
		result.val = first;
		result.size = 1;
	} else if (first == 253) {
		// 16 bits.
		result.val = CBGetByteArrayVT(bytes)->readUInt16(bytes,offset+1);
		result.size = 3;
	} else if (first == 254) {
		// 32 bits.
		result.val = CBGetByteArrayVT(bytes)->readUInt32(bytes,offset+1);
		result.size = 5;
	} else {
		// 64 bits.
		result.val = CBGetByteArrayVT(bytes)->readUInt64(bytes,offset+1);
		result.size = 9;
	}
	return result;
}
void CBVarIntEncode(CBByteArray * bytes,u_int32_t offset,CBVarInt varInt){
	switch (varInt.size) {
		case 1:
			CBGetByteArrayVT(bytes)->setByte(bytes,offset,(u_int8_t)varInt.val);
			break;
		case 3:
			CBGetByteArrayVT(bytes)->setByte(bytes,offset,253);
			CBGetByteArrayVT(bytes)->setUInt16(bytes,offset+1,(u_int16_t)varInt.val);
			break;
		case 5:
			CBGetByteArrayVT(bytes)->setByte(bytes,offset,254);
			CBGetByteArrayVT(bytes)->setUInt32(bytes,offset+1,(u_int32_t)varInt.val);
			break;
		case 9:
			CBGetByteArrayVT(bytes)->setByte(bytes,offset,255);
			CBGetByteArrayVT(bytes)->setUInt64(bytes,offset+1,varInt.val);
			break;
	}
}
CBVarInt CBVarIntFromUInt64(u_int64_t integer){
	CBVarInt varInt;
	varInt.val = integer;
	varInt.size = CBVarIntSizeOf(integer);
	return varInt;
}
u_int8_t CBVarIntSizeOf(u_int64_t value){
	if (value < 253)
		return 1;
	else if (value < 65536)
		return 3;  // 1 marker + 2 data bytes
	else if (value < 4294967296)
		return 5;  // 1 marker + 4 data bytes
	return 9; // 1 marker + 4 data bytes
}

