//
//  CBBigInt.c
//  cbitcoin
//
//  Created by Matthew Mitchell on 28/04/2012.
//  Last modified by Matthew Mitchell on 11/05/2012.
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
//  Contains BIGDIGITS multiple-precision arithmetic code originally
//  written by David Ireland, copyright (c) 2001-11 by D.I. Management
//  Services Pty Limited <www.di-mgt.com.au>, and is used with
//  permission.

#include "CBBigInt.h"

u_int8_t CBPowerOf2Log2(u_int8_t a){
	switch (a) {
		case 1:
			return 0;
		case 2:
			return 1;
		case 4:
			return 2;
		case 8:
			return 3;
		case 16:
			return 4;
		case 32:
			return 5;
		case 64:
			return 6;
	}
	return 7;
}
u_int8_t CBFloorLog2(u_int8_t a){
	if (a < 16){
		if (a < 4) {
			if (a == 1){
				return 0;
			}
			return 1;
		}
		if (a < 8){
			return 2;
		}
		return 3;
	}
	if (a < 64){
		if (a < 32) {
			return 4;
		}
		return 5;
	}
	if (a < 128){
		return 6;
	}
	return 7;
}
CBCompare CBBigIntCompare(CBBigInt a,CBBigInt b){
	if (a.length > b.length)
		return CB_COMPARE_MORE_THAN;
	else if (a.length < b.length)
		return CB_COMPARE_LESS_THAN;
	for (u_int8_t x = a.length - 1;; x--) {
		if (a.data[x] < b.data[x])
			return CB_COMPARE_LESS_THAN;
		else if (a.data[x] > b.data[x])
			return CB_COMPARE_MORE_THAN;
		if (!x)
			break;
	}
	return CB_COMPARE_EQUAL;
}
CBCompare CBBigIntCompareToUInt8(CBBigInt a,u_int8_t b){
	for (u_int8_t x = a.length - 1;x > 0; x--)
		if (a.data[x]) 
			return CB_COMPARE_MORE_THAN;
	if (a.data[0] > b)
		return CB_COMPARE_MORE_THAN;
	else if (a.data[0] < b)
		return CB_COMPARE_LESS_THAN;
	return CB_COMPARE_EQUAL;
}
void CBBigIntEqualsDivisionByUInt8(CBBigInt * a,u_int8_t b,u_int8_t * ans){
	// base-256 long division.
	u_int16_t temp = 0;
	for (u_int8_t x = a->length-1;; x--) {
		temp <<= 8;
		temp |= a->data[x];
		ans[x] = temp / b;
		temp -= ans[x] * b;
		if (!x)
			break;
	}
	if (!ans[a->length-1]) { // If last byte is zero, adjust length.
		a->length--;
		a->data = realloc(a->data, a->length);
	}
	memmove(a->data, ans, a->length); // Done calculation. Move ans to "a".
}
void CBBigIntEqualsMultiplicationByUInt8(CBBigInt * a,u_int8_t b,u_int8_t * ans){
	// Multiply b by each byte and then add to answer
	for (u_int8_t x = 0; x < a->length; x++) {
		u_int16_t mult = ans[x] + a->data[x] * b; // Allow for overflow onto next byte.
		ans[x] = mult;
		ans[x+1] = (mult >> 8);
	}
	if (ans[a->length-1]) { // If last byte is not zero, adjust length.
		a->length++;
		a->data = realloc(a->data, a->length);
	}
	memmove(a->data, ans, a->length); // Done calculation. Move ans to "a".
}
void CBBigIntEqualsRightShiftByUInt8(CBBigInt * a,u_int8_t b){
	u_int8_t deadBytes = b / 8; // These bytes fall off the side.
	a->length -= deadBytes; // Reduce length of bignum by the removed bytes
	u_int8_t remainderShift = b % 8;
	if (!remainderShift) { // No more work
		return;
	}
	u_int16_t splitter;
	u_int8_t toRight = 0; // Bits taken from the left to the next byte.
	for (u_int8_t x = a->length-1;; x--) {
		splitter = a->data[x] << 8 - remainderShift; // Splits data in splitters between first and second byte.
		a->data[x] = splitter >> 8; // First byte in splitter is the new data.
		a->data[x] |= toRight; // Take the bits from the left
		toRight = splitter; // Second byte is the data going to the right from this byte.
		if (!x)
			break;
	}
}
void CBBigIntEqualsSubtractionByUInt8(CBBigInt * a,u_int8_t b){
	u_int16_t end = a->data[0] + (a->data[1] << 8);
	end -= b;
	a->data[0] = end;
	a->data[1] = end >> 8;
}
u_int8_t CBBigIntModuloWithUInt8(CBBigInt a,u_int8_t b){
	if (!(b & (b - 1)))
		return a.data[0] & b - 1; // For powers of two this can be done
	// Wasn't a power of two. Use method presented here: http://stackoverflow.com/a/10441333/238411
	u_int16_t result = 0; // Prevents overflow in calculations
	for(u_int8_t x = a.length - 1;; x--){
		result *= (256 % b);
		result %= b;
		result += a.data[x] % b;
		result %= b;
		if (!x)
			break;
	}
	return result;
}
CBBigInt CBBigIntFromPowUInt8(u_int8_t a,u_int8_t b){
	CBBigInt bi;
	bi.data = malloc(1);
	bi.length = 1;
	bi.data[0] = 1;
	u_int8_t * temp = malloc(b);
	for (u_int8_t x = 0; x < b; x++) {
		CBBigIntEqualsMultiplicationByUInt8(&bi, a, temp);
	}
	free(temp);
	return bi;
}
void CBBigIntNormalise(CBBigInt * a){
	for (u_int8_t x = a->length - 1;; x--){
		if (a->data[x]){
			if (x == a->length - 1)
				break;
			a->length = x + 1;
			break;
		}else if (!x){
			// Start with zero
			a->length = 1;
			break;
		}
	}
}
