//
//  CBBase58.c
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

#include "CBBase58.h"

void CBDecodeBase58(CBBigInt * bi, char * str){
	// ??? Quite likely these functions can be improved
	CBBigInt bi2;
	CBBigIntAlloc(&bi2, 1);
	bi->data[0] = 0;
	bi->length = 1;
	for (uint8_t x = strlen(str) - 1;; x--){ // Working backwards
		// Get index in alphabet array
		uint8_t alphaIndex = str[x];
		if (alphaIndex != 49){ // If not 1
			if (str[x] < 58){ // Numbers
				alphaIndex -= 49;
			}else if (str[x] < 73){ // A-H
				alphaIndex -= 56;
			}else if (str[x] < 79){ // J-N
				alphaIndex -= 57;
			}else if (str[x] < 91){ // P-Z
				alphaIndex -= 58;
			}else if (str[x] < 108){ // a-k
				alphaIndex -= 64;
			}else{ // m-z
				alphaIndex -= 65;
			}
			CBBigIntFromPowUInt8(&bi2, 58, strlen(str) - 1 - x);
			CBBigIntEqualsMultiplicationByUInt8(&bi2, alphaIndex);
			CBBigIntEqualsAdditionByBigInt(bi, &bi2);
		}
		if (! x)
			break;
	}
	free(bi2.data);
	// Got CBBigInt from base-58 string. Add zeros on end.
	uint8_t zeros = 0;
	for (uint8_t x = 0; x < strlen(str); x++)
		if (str[x] == '1')
			zeros++;
		else
			break;
	if (zeros) {
		bi->length += zeros;
		CBBigIntRealloc(bi, bi->length);
		memset(bi->data + bi->length - zeros, 0, zeros);
	}else
		// Reallocate to actual length
		CBBigIntRealloc(bi, bi->length);
}
bool CBDecodeBase58Checked(CBBigInt * bi, char * str){
	CBDecodeBase58(bi, str);
	if (bi->length < 4){
		CBLogError("The string passed into CBDecodeBase58Checked decoded into data that was too short.");
		return false;
	}
	// Reverse bytes for checksum generation
	uint8_t * reversed = malloc(bi->length - 4);
	for (uint8_t x = 4; x < bi->length; x++)
		reversed[bi->length - 1 - x] = bi->data[x];
	// The checksum uses SHA-256, twice, for some reason unknown to man.
	uint8_t checksum[32];
	uint8_t checksum2[32];
	CBSha256(reversed, bi->length - 4, checksum);
	free(reversed);
	CBSha256(checksum, 32, checksum2);
	bool ok = true;
	for (uint8_t x = 0; x < 4; x++)
		if (checksum2[x] != bi->data[3-x])
			ok = false;
	if (! ok){
		CBLogError("The data passed to CBDecodeBase58Checked is invalid. Checksum does not match.");
		return false;
	}
	return true;
}
char * CBEncodeBase58(CBBigInt * bi){
	// ??? Improvements?
	uint8_t x = 0;
	char * str = malloc(bi->length);
	// Zeros
	for (uint8_t y = bi->length - 1;; y--)
		if (! bi->data[y]){
			str[x] = '1';
			x++;
			if (! y)
				break;
		}else
			break;
	uint8_t zeros = x;
	// Make temporary data store
	uint8_t * temp = malloc(bi->length);
	// Encode
	uint8_t mod;
	size_t size = bi->length;
	for (;CBBigIntCompareTo58(bi) >= 0;x++) {
		mod = CBBigIntModuloWith58(bi);
		if (bi->length < x + 3) {
			size = x + 3;
			if (size > bi->length) {
				char * temp = realloc(str, size);
				str = temp;
			}
		}
		str[x] = base58Characters[mod];
		CBBigIntEqualsSubtractionByUInt8(bi, mod);
		memset(temp, 0, bi->length);
		CBBigIntEqualsDivisionBy58(bi, temp);
	}
	str[x++] = base58Characters[bi->data[bi->length-1]];
	// Reversal
	for (uint8_t y = 0; y < (x-zeros) / 2; y++) {
		char temp = str[y+zeros];
		str[y+zeros] = str[x-y-1];
		str[x-y-1] = temp;
	}
	str[x] = '\0';
	// Cleanup
	free(temp);
	return str;
}
