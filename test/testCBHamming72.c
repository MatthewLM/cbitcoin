//
//  testCBHamming72.c
//  cbitcoin
//
//  Created by Matthew Mitchell on 24/12/2012.
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

#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "CBHamming72.h"

void logError(char * format,...);
void logError(char * format,...){
	va_list argptr;
    va_start(argptr, format);
    vfprintf(stderr, format, argptr);
    va_end(argptr);
	printf("\n");
}

int main(){
	unsigned int s = (unsigned int)time(NULL);
	s = 1337544566;
	printf("Session = %ui\n",s);
	srand(s);
	uint8_t data[7] = {0x47,0x00,0x02,0x9E,0xFF,0xE1,0xF2};
	uint8_t parityBits[2];
	// p p 0 [p 1 0 0] p 0 1 1 [1]
	// Test hamming encoding for one byte
	CBHamming72Encode(data, 1, parityBits);
	if (parityBits[0] != 0xF7) {
		printf("8 BITS PARITY FAIL\n");
		return 1;
	}
	// Test hamming encoding for less than 64 bits
	CBHamming72Encode(data, 7, parityBits);
	// Check result
	// p p 0 p 1 0 0 p 0 1 1 1 0 0 0 p 0 0 0 0 0 0 0 0 0 0 0 1 0 1 0 [p 0 1 1 1 1 0 1 1 1 1 1 1 1 1 1 1 1 0 0 0 0 1 1 1 1 1 0 0 1 0]
	if (parityBits[0] != 0xD4) {
		printf("LESS THAN 64 BITS PARITY FAIL\n");
		return 1;
	}
	// Test hamming encoding for more than 64 bits
	uint8_t data2[10] = {0x82,0x90,0x0A,0x26,0xF1,0xFA,0xCE,0xEB,0x08,0x21};
	CBHamming72Encode(data2, 10, parityBits);
	//   1       2 3   4        5 6    7  89 0123   456789 0 12  345 6 78 9 01
	// pp1p000p0010100p100000000101000p1001101111000111111010110011101p1101011
	//         1     2     3
	// pp0p000p1000001p00001
	// 111001
	// 11101100
	if (parityBits[0] != 0x39 || parityBits[1] != 0x6C) {
		printf("MORE THAN 64 BITS PARITY FAIL\n");
		return 1;
	}
	// Test single correction of 16 byte data
	uint8_t data3[16] = {0xFA,0xCE,0x00,0x4B,0xBA,0xC2,0xBA,0x40,0xFE,0xD2,0x75,0x3A,0x19,0x42,0x0E,0x23};
	CBHamming72Encode(data3, 16, parityBits);
	for (uint8_t x = 0; x < 128; x++) {
		data3[x/8] ^= 1 << (x % 8);
		CBHamming72Result res = CBHamming72Check(data3, 16, parityBits);
		if (res.numCorrections != 1) {
			printf("SINGLE CORRECTION NUM FAIL ON %u\n",x);
			return 1;
		}
		if (res.err) {
			printf("SINGLE CORRECTION ERR FAIL\n");
			return 1;
		}
		if (res.corrections[0] != x/8) {
			printf("SINGLE CORRECTION SECTION FAIL ON %u\n",x);
			return 1;
		}
		// Test that the correction was a success.
		if (memcmp(data3,(uint8_t []){0xFA,0xCE,0x00,0x4B,0xBA,0xC2,0xBA,0x40,0xFE,0xD2,0x75,0x3A,0x19,0x42,0x0E,0x23},16)) {
			printf("SINGLE CORRECTION DATA FAIL ON %u\n",x);
			return 1;
		}
		free(res.corrections);
	}
	// Test parity bit correction
	for (uint8_t x = 0; x < 16; x++) {
		parityBits[x/8] ^= 1 << (x % 8);
		CBHamming72Result res = CBHamming72Check(data3, 16, parityBits);
		if (res.numCorrections != 1) {
			printf("SINGLE PARITY CORRECTION NUM FAIL ON %u\n",x);
			return 1;
		}
		if (res.err) {
			printf("SINGLE PARITY CORRECTION ERR FAIL\n");
			return 1;
		}
		if (res.corrections[0] != 128 + x/8) {
			printf("SINGLE PARITY CORRECTION SECTION FAIL ON %u\n",x);
			return 1;
		}
		// Test that the correction was a success.
		if (memcmp(parityBits,(uint8_t []){0x28, 0x30},2)) {
			printf("SINGLE PARITY CORRECTION DATA FAIL ON %u\n",x);
			return 1;
		}
		free(res.corrections);
	}
	// Test two errors in different sections
	for (uint8_t x = 0; x < 64; x++) {
		data3[x/8] ^= 1 << (x % 8);
		data3[8 + x/8] ^= 1 << (x % 8);
		CBHamming72Result res = CBHamming72Check(data3, 16, parityBits);
		if (res.numCorrections != 2) {
			printf("TWO CORRECTION NUM FAIL ON %u\n",x);
			return 1;
		}
		if (res.err) {
			printf("TWO CORRECTION ERR FAIL\n");
			return 1;
		}
		if (res.corrections[0] != x/8) {
			printf("TWO CORRECTION SECTION ONE FAIL ON %u\n",x);
			return 1;
		}
		if (res.corrections[1] != 8 + x/8) {
			printf("TWO CORRECTION SECTION TWO FAIL ON %u\n",x);
			return 1;
		}
		// Test that the corrections was a success.
		if (memcmp(data3,(uint8_t []){0xFA,0xCE,0x00,0x4B,0xBA,0xC2,0xBA,0x40,0xFE,0xD2,0x75,0x3A,0x19,0x42,0x0E,0x23},16)) {
			printf("SINGLE CORRECTION DATA FAIL ON %u\n",x);
			return 1;
		}
		free(res.corrections);
	}
	// Test double error detection
	parityBits[0] ^= 4;
	data3[3] ^= 64;
	CBHamming72Result res = CBHamming72Check(data3, 16, parityBits);
	if (res.numCorrections != 0) {
		printf("DOUBLE ERROR DETECTION NUM FAIL\n");
		return 1;
	}
	if (NOT res.err) {
		printf("DOUBLE ERROR DETECTION ERR FAIL\n");
		return 1;
	}
	return 0;
}
