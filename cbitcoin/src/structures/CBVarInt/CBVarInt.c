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
		result.val = CBGetByteArrayVT(bytes)->getByte(bytes,offset+1) | (CBGetByteArrayVT(bytes)->getByte(bytes,offset+2) << 8);
		result.size = 2;
	} else if (first == 254) {
		// 32 bits.
		result.val = CBGetByteArrayVT(bytes)->readUInt32(bytes,offset+1);
		result.size = 4;
	} else {
		// 64 bits.
		result.val = CBGetByteArrayVT(bytes)->readUInt64(bytes,offset+1);
		result.size = 8;
	}
	return result;
}

u_int8_t CBVarIntSizeOf(u_int32_t value){
	if (value < 253)
		return 1;
	else if (value < 65536)
		return 3;  // 1 marker + 2 data bytes
	return 5;  // 1 marker + 4 data bytes
}

