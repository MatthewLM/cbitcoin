//
//  CBBase58.c
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

#include "CBBase58.h"

bool CBDecodeBase58(CBBigInt * bi, char * str){
	// ??? Quite likely these functions can be improved
	CBBigInt bi2;
	if (NOT CBBigIntAlloc(&bi2, 1))
		return false;
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
			if (NOT CBBigIntFromPowUInt8(&bi2, 58, strlen(str) - 1 - x)){
				// Error occured.
				free(bi2.data);
				return false;
			}
			if (NOT CBBigIntEqualsMultiplicationByUInt8(&bi2, alphaIndex)){
				// Error occured.
				free(bi2.data);
				return false;
			}
			if (NOT CBBigIntEqualsAdditionByBigInt(bi, &bi2)){
				// Error occured.
				free(bi2.data);
				return false;
			}
		}
		if (NOT x)
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
		if (NOT CBBigIntRealloc(bi, bi->length))
			return false;
		memset(bi->data + bi->length - zeros, 0, zeros);
	}
	return true;
}
bool CBDecodeBase58Checked(CBBigInt * bi, char * str){
	if(NOT CBDecodeBase58(bi, str)) {
		CBLogError("Memory failure in CBDecodeBase58.");
		return false;
	}
	if (bi->length < 4){
		CBLogError("The string passed into CBDecodeBase58Checked decoded into data that was too short.");
		return false;
	}
	// Reverse bytes for checksum generation
	uint8_t * reversed = malloc(bi->length - 4);
	if (NOT reversed) {
		CBLogError("Cannot allocate %i bytes of memory in CBDecodeBase58Checked", bi->length - 4);
		return false;
	}
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
	if (NOT ok){
		CBLogError("The data passed to CBDecodeBase58Checked is invalid. Checksum does not match.");
		return false;
	}
	return true;
}
char * CBEncodeBase58(CBBigInt * bi){
	// ??? Improvements?
	uint8_t x = 0;
	char * str = malloc(bi->length);
	if (NOT str)
		return NULL;
	// Zeros
	for (uint8_t y = bi->length - 1;; y--)
		if (NOT bi->data[y]){
			str[x] = '1';
			x++;
			if (NOT y)
				break;
		}else
			break;
	uint8_t zeros = x;
	// Make temporary data store
	uint8_t * temp = malloc(bi->length);
	if (NOT temp) {
		free(str);
		return NULL;
	}
	// Encode
	uint8_t mod;
	size_t size = bi->length;
	for (;CBBigIntCompareTo58(bi) >= 0;x++) {
		mod = CBBigIntModuloWith58(bi);
		if (bi->length < x + 3) {
			size = x + 3;
			if (size > bi->length) {
				char * temp = realloc(str, size);
				if (NOT temp){
					free(str);
					return NULL;
				}
				str = temp;
			}
		}
		str[x] = base58Characters[mod];
		CBBigIntEqualsSubtractionByUInt8(bi, mod);
		memset(temp, 0, bi->length);
		CBBigIntEqualsDivisionBy58(bi, temp);
	}
	str[x] = base58Characters[bi->data[bi->length-1]];
	x++;
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
