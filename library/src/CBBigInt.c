//
//  CBBigInt.c
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

#include "CBBigInt.h"
#include <assert.h>

void CBBigIntAlloc(CBBigInt * bi, uint8_t allocLen){
	bi->allocLen = allocLen;
	bi->data = malloc(allocLen);
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
CBCompare CBBigIntCompareToBigInt(CBBigInt * a, CBBigInt * b){
	if (a->length > b->length)
		return CB_COMPARE_MORE_THAN;
	else if (a->length < b->length)
		return CB_COMPARE_LESS_THAN;
	for (uint8_t x = a->length; x--;) {
		if (a->data[x] < b->data[x])
			return CB_COMPARE_LESS_THAN;
		else if (a->data[x] > b->data[x])
			return CB_COMPARE_MORE_THAN;
	}
	return CB_COMPARE_EQUAL;
}
void CBBigIntEqualsAdditionByBigInt(CBBigInt * a, CBBigInt * b){
	if (a->length < b->length) {
		CBBigIntRealloc(a, b->length);
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
		overflow = ! ++a->data[x++]; // Index at x, increment x, increment data, test new value for overflow.
	if (overflow) { // Add extra byte
		a->length++;
		CBBigIntRealloc(a, a->length);
		a->data[a->length - 1] = 1;
	}
}
void CBBigIntEqualsDivisionBy58(CBBigInt * a, uint8_t * ans){
	if (a->length == 1 && ! a->data[0]) // "a" is zero
		return;
	// base-256 long division.
	uint16_t temp = 0;
	for (uint8_t x = a->length; x--;) {
		temp <<= 8;
		temp |= a->data[x];
		ans[x] = temp / 58;
		temp -= ans[x] * 58;
	}
	if (! ans[a->length-1]) // If last byte is zero, adjust length.
		a->length--;
	memmove(a->data, ans, a->length); // Done calculation. Move ans to "a".
}
void CBBigIntEqualsMultiplicationByUInt8(CBBigInt * a, uint8_t b){
	if (! b) {
		// Mutliplication by zero. "a" becomes zero
		a->length = 1;
		a->data[0] = 0;
		return;
	}
	if (a->length == 1 && ! a->data[0]) // "a" is zero
		return;
	// Multiply b by each byte and then add to answer
	uint16_t carry = 0;
	uint8_t x = 0;
	for (; x < a->length; x++) {
		carry = carry + a->data[x] * b; // Allow for overflow onto next byte.
		a->data[x] = carry;
		carry >>= 8;
	}
	if (carry) { // If last byte is not zero, adjust length.
		CBBigIntRealloc(a, a->length+1);
		a->length++;
		a->data[x] = carry;
	}
}
void CBBigIntEqualsSubtractionByBigInt(CBBigInt * a, CBBigInt * b){
	uint8_t x;
	bool carry = 0;
	// This can be made much nicer when using signed arithmetic, 
	// carry and tmp could be merged to be 0 or -1 between rounds.
	for (x = 0; x < b->length; x++) {
		uint16_t tmp = carry + b->data[x];
		carry = a->data[x] < tmp;
		a->data[x] -= tmp;
	}
	if (carry)
		a->data[x]--;
	CBBigIntNormalise(a);
}
void CBBigIntEqualsSubtractionByUInt8(CBBigInt * a, uint8_t b){
	uint8_t carry = b;
	uint8_t x = 0;
	for (; a->data[x] < carry; x++){
		a->data[x] = 255 - carry + a->data[x] + 1;
		carry = 1;
	}
	a->data[x] -= carry;
	CBBigIntNormalise(a);
}
void CBBigIntFromPowUInt8(CBBigInt * bi, uint8_t a, uint8_t b){
	bi->length = 1;
	bi->data[0] = 1;
	for (uint8_t x = 0; x < b; x++)
		CBBigIntEqualsMultiplicationByUInt8(bi, a);
}
uint8_t CBBigIntModuloWith58(CBBigInt * a){
	// Use method presented here: http://stackoverflow.com/a/10441333/238411
	uint16_t result = 0; // Prevent overflow in calculations
	for(uint8_t x = a->length - 1;; x--){
		result *= (256 % 58);
		result %= 58;
		result += a->data[x] % 58;
		result %= 58;
		if (! x)
			break;
	}
	return result;
}
void CBBigIntNormalise(CBBigInt * a){
	while (a->length > 1 && ! a->data[a->length-1])
		a->length--;
}
void CBBigIntRealloc(CBBigInt * bi, uint8_t allocLen){
	if (bi->allocLen < allocLen) {
		uint8_t * temp = realloc(bi->data, allocLen);
		bi->data = temp;
		bi->allocLen = allocLen;
	}
}
