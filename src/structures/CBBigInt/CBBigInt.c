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

CBCompare CBBigIntCompareTo58(CBBigInt a){
	if(a.length > 1)
		return CB_COMPARE_MORE_THAN;
	if (a.data[0] > 58)
		return CB_COMPARE_MORE_THAN;
	else if (a.data[0] < 58)
		return CB_COMPARE_LESS_THAN;
	return CB_COMPARE_EQUAL;
}
void CBBigIntEqualsAdditionByCBBigInt(CBBigInt * a,CBBigInt * b){
	if (a->length < b->length) {
		uint8_t * temp = realloc(a->data, b->length);
		if (NOT temp) {
			free(a->data);
			a->data = NULL;
			a->length = 0;
			return;
		}
		a->data = temp;
		// Make certain data is empty
		uint8_t diff = b->length - a->length;
		memset(a->data + (b->length - diff), 0, diff);
	}
	uint8_t overflow = 0;
	for (uint8_t x = 0; x < b->length; x++) {
		a->data[x] += b->data[x] + overflow;
		overflow = (a->data[x] < (b->data[x] + overflow))? 1 : 0;
	}
	if (overflow) { // Add extra byte
		a->length = b->length + 1;
		uint8_t * new = realloc(a->data, a->length);
		if (NOT new) {
			free(a->data);
			a->data = NULL;
			a->length = 0;
			return;
		}
		a->data = new;
		a->data[a->length - 1] = 1;
	}else{
		a->length = b->length;
	}
}
void CBBigIntEqualsDivisionBy58(CBBigInt * a,uint8_t * ans){
	if (a->length == 1 && NOT a->data[0]) { // "a" is zero
		return;
	}
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
	if (NOT ans[a->length-1]) { // If last byte is zero, adjust length.
		a->length--;
		uint8_t * new = realloc(a->data, a->length);
		if (NOT new) {
			free(a->data);
			a->data = NULL;
			a->length = 0;
			return;
		}
		a->data = new;
	}
	memmove(a->data, ans, a->length); // Done calculation. Move ans to "a".
}
void CBBigIntEqualsMultiplicationByUInt8(CBBigInt * a,uint8_t b,uint8_t * ans){
	if (NOT b) {
		// Mutliplication by zero. "a" becomes zero
		a->length = 1;
		uint8_t * new = realloc(a->data, 1);
		if (NOT new) {
			free(a->data);
			a->data = NULL;
			a->length = 0;
			return;
		}
		a->data = new;
		a->data[0] = 0;
		return;
	}
	if (a->length == 1 && NOT a->data[0]) { // "a" is zero
		return;
	}
	// Multiply b by each byte and then add to answer
	for (uint8_t x = 0; x < a->length; x++) {
		uint16_t mult = ans[x] + a->data[x] * b; // Allow for overflow onto next byte.
		ans[x] = mult;
		ans[x+1] = (mult >> 8);
	}
	if (ans[a->length]) { // If last byte is not zero, adjust length.
		a->length++;
		uint8_t * new = realloc(a->data, a->length);
		if (NOT new) {
			free(a->data);
			a->data = NULL;
			a->length = 0;
			return;
		}
		a->data = new;
	}
	memmove(a->data, ans, a->length); // Done calculation. Move ans to "a".
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
uint8_t CBBigIntModuloWith58(CBBigInt a){
	// Use method presented here: http://stackoverflow.com/a/10441333/238411
	uint16_t result = 0; // Prevent overflow in calculations
	for(uint8_t x = a.length - 1;; x--){
		result *= (256 % 58);
		result %= 58;
		result += a.data[x] % 58;
		result %= 58;
		if (NOT x)
			break;
	}
	return result;
}
CBBigInt CBBigIntFromPowUInt8(uint8_t a,uint8_t b){
	CBBigInt bi;
	bi.data = malloc(1);
	if (NOT bi.data){
		bi.length = 0;
		bi.data = NULL;
		return bi;
	}
	bi.length = 1;
	bi.data[0] = 1;
	uint8_t * temp = malloc(b);
	if (NOT temp) {
		free(bi.data);
		bi.length = 0;
		bi.data = NULL;
		return bi;
	}
	for (uint8_t x = 0; x < b; x++) {
		memset(temp, 0, bi.length);
		CBBigIntEqualsMultiplicationByUInt8(&bi, a, temp);
	}
	free(temp);
	return bi;
}
void CBBigIntNormalise(CBBigInt * a){
	for (uint8_t x = a->length - 1;; x--){
		if (a->data[x]){
			if (x == a->length - 1)
				break;
			a->length = x + 1;
			break;
		}else if (NOT x){
			// Start with zero
			a->length = 1;
			break;
		}
	}
}
