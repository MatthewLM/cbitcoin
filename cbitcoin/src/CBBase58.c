//
//  CBBase58.c
//  cbitcoin
//
//  Created by Matthew Mitchell on 28/04/2012.
//  Last modified by Matthew Mitchell on 10/05/2012.
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

#include "CBBase58.h"

void CBDecodeBase58(u_int8_t * bytes,char * str, u_int8_t len){
	
}
void CBDecodeBase58Checked(u_int8_t * bytes,char * str, u_int8_t len){
	
}
void CBEncodeBase58(char * str, u_int8_t * bytes, u_int8_t len){
	// ??? Improvement?
	u_int8_t x = 0;
	// Make CBBigInt
	CBBigInt bi;
	bi.data = bytes;
	bi.length = len;
	CBBigIntNormalise(&bi);
	// Make temporary data store
	u_int8_t * temp = malloc(len);
	// Encode
	u_int8_t mod;
	for (;CBBigIntCompareToUInt8(bi, 58) >= 0;x++) {
		mod = CBBigIntModuloWithUInt8(bi, 58);
		str[x] = base58Characters[mod];
		CBBigIntEqualsSubtractionByUInt8(&bi, mod);
		memset(temp, 0, len);
		CBBigIntEqualsDivisionByUInt8(&bi, 58,temp);
	}
	str[x] = base58Characters[bi.data[bi.length-1]];
	x++;
	// Leading zeros
	for (int y = 0; y < len; y++, x++)
		if (!bytes[y])
			str[x] = '1';
		else
			break;
	// Reversal
	for (int y = 0; y < x / 2; y++) {
		char temp = str[y];
		str[y] = str[x-y-1];
		str[x-y-1] = temp;
	}
	str[x] = '\0';
}
