//
//  CBBigInt.c
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
//

//  SEE HEADER FILE FOR DOCUMENTATION

#include "CBBigInt.h"
#include <assert.h>

bool CBBigIntAlloc(CBBigInt * bi, uint8_t allocLen){
	bi->allocLen = allocLen;
	bi->data = malloc(allocLen);
	return bi->data;
}
CBCompare CBBigIntCompareTo58(CBBigInt * a){
	if(a->length > 1)
		return CB_COMPARE_MORE_THAN;
	if (a->data[0] > 58)
		return CB_COMPARE_MORE_THAN;
	else if (a->data[0] < 58)
		return CB_COMPARE_LESS_THAN;
	return CB_COMPARE_EQUAL;
}
CBCompare CBBigIntCompareToBigInt(CBBigInt * a,CBBigInt * b){
	if (a->length > b->length)
		return CB_COMPARE_MORE_THAN;
	else if (a->length < b->length)
		return CB_COMPARE_LESS_THAN;
	for (uint8_t x = a->length - 1;; x--) {
		if (a->data[x] < b->data[x])
			return CB_COMPARE_LESS_THAN;
		else if (a->data[x] > b->data[x])
			return CB_COMPARE_MORE_THAN;
		if (!x)
			break;
	}
	return CB_COMPARE_EQUAL;
}
bool CBBigIntEqualsAdditionByBigInt(CBBigInt * a,CBBigInt * b){
	if (a->length < b->length) {
		if (NOT CBBigIntRealloc(a, b->length))
			return false;
		// Make certain expansion of data is empty
		memset(a->data + a->length, 0, b->length - a->length);
		a->length = b->length;
	}
	// a->length >= b->length
	bool overflow = 0;
	uint8_t x = 0;
	for (; x < b->length; x++) {
		a->data[x] += b->data[x] + overflow;
		// a->data[x] now equals the result of the addition.
		// The overflow will never go beyond 1. Imagine a->data[x] == 0xff, b->data[x] == 0xff and the overflow is 1, the new overflow is still 1 and a->data[x] is 0xff. Therefore it does work.
		overflow = (a->data[x] < (b->data[x] + overflow))? 1 : 0;
	}
	// Propagate overflow up the whole length of a if necessary
	while (overflow && x < a->length)
		overflow = NOT ++a->data[x++]; // Index at x, increment x, increment data, test new value for overflow.
	if (overflow) { // Add extra byte
		a->length++;
		if (NOT CBBigIntRealloc(a, a->length))
			return false;
		a->data[a->length - 1] = 1;
	}
	return true;
}
void CBBigIntEqualsDivisionBy58(CBBigInt * a,uint8_t * ans){
	if (a->length == 1 && NOT a->data[0]) // "a" is zero
		return;
	// base-256 long division.
	uint16_t temp = 0;
	for (uint8_t x = a->length-1;; x--) {
		temp <<= 8;
		temp |= a->data[x];
		ans[x] = temp / 58;
		temp -= ans[x] * 58;
		if (NOT x)
			break;
	}
	if (NOT ans[a->length-1]) // If last byte is zero, adjust length.
		a->length--;
	memmove(a->data, ans, a->length); // Done calculation. Move ans to "a".
}
bool CBBigIntEqualsMultiplicationByUInt8(CBBigInt * a,uint8_t b,uint8_t * ans){
	if (NOT b) {
		// Mutliplication by zero. "a" becomes zero
		a->length = 1;
		a->data[0] = 0;
		return true;
	}
	if (a->length == 1 && NOT a->data[0]) // "a" is zero
		return true;
	// Multiply b by each byte and then add to answer
	for (uint8_t x = 0; x < a->length; x++) {
		uint16_t mult = ans[x] + a->data[x] * b; // Allow for overflow onto next byte.
		ans[x] = mult;
		ans[x+1] = (mult >> 8);
	}
	if (ans[a->length]) { // If last byte is not zero, adjust length.
		a->length++;
		if (NOT CBBigIntRealloc(a, a->length))
			return false;
	}
	memmove(a->data, ans, a->length); // Done calculation. Move ans to "a".
	return true;
}
void CBBigIntEqualsSubtractionByBigInt(CBBigInt * a,CBBigInt * b){
	// ??? Needs a test for this function.
	for (uint8_t x = 0; x < b->length; x++) {
		uint8_t sub = b->data[x];
		for (uint8_t y = x; y < a->length; y++) {
			if (a->data[y] >= sub) {
				a->data[y] -= sub;
				break;
			}else{
				a->data[y] = 255 - (sub - a->data[y] - 1);
				sub = 1;
			}
		}
	}
	CBBigIntNormalise(a);
}
void CBBigIntEqualsSubtractionByUInt8(CBBigInt * a,uint8_t b){
	uint8_t sub = b;
	for (uint8_t x = 0;x < a->length; x++) {
		if (a->data[x] >= sub) {
			a->data[x] -= sub;
			break;
		}else{
			a->data[x] = 255 - (sub - a->data[x] - 1);
			sub = 1;
		}
	}
	CBBigIntNormalise(a);
}
bool CBBigIntFromPowUInt8(CBBigInt * bi,uint8_t a,uint8_t b){
	bi->length = 1;
	bi->data[0] = 1;
	uint8_t * temp = malloc(b);
	if (NOT temp)
		// ERROR
		return false;
	for (uint8_t x = 0; x < b; x++) {
		memset(temp, 0, bi->length);
		if (NOT CBBigIntEqualsMultiplicationByUInt8(bi, a, temp))
			// ERROR
			return false;
	}
	free(temp);
	return true;
}
uint8_t CBBigIntModuloWith58(CBBigInt * a){
	// Use method presented here: http://stackoverflow.com/a/10441333/238411
	uint16_t result = 0; // Prevent overflow in calculations
	for(uint8_t x = a->length - 1;; x--){
		result *= (256 % 58);
		result %= 58;
		result += a->data[x] % 58;
		result %= 58;
		if (NOT x)
			break;
	}
	return result;
}
void CBBigIntNormalise(CBBigInt * a){
	while (a->length > 1 && NOT a->data[a->length-1])
		a->length--;
}
bool CBBigIntRealloc(CBBigInt * bi, uint8_t allocLen){
	if (bi->allocLen < allocLen) {
		uint8_t * temp = realloc(bi->data, allocLen);
		if (NOT temp)
			return false;
		bi->data = temp;
		bi->allocLen = allocLen;
	}
	return true;
}
