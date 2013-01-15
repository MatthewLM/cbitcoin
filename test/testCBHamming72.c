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

void CBLogError(char * format, ...);
void CBLogError(char * format, ...){
	va_list argptr;
    va_start(argptr, format);
    vfprintf(stderr, format, argptr);
    va_end(argptr);
	printf("\n");
}

int main(){
	unsigned int s = (unsigned int)time(NULL);
	s = 1337544566;
	printf("Session = %ui\n", s);
	srand(s);
	uint8_t data[8] = {0x47, 0x00, 0x02, 0x9E, 0xFF, 0xE1, 0xF2, 0};
	// p p 0 [p 1 0 0] p 0 1 1 [1]
	// Test hamming encoding for one byte
	CBHamming72Encode(data, 1, data + 7);
	if (data[7] != 0xF7) {
		printf("8 BITS PARITY FAIL\n");
		return 1;
	}
	// Test hamming encoding for less than 64 bits
	CBHamming72Encode(data, 7, data + 7);
	// Check result
	// p p 0 p 1 0 0 p 0 1 1 1 0 0 0 p 0 0 0 0 0 0 0 0 0 0 0 1 0 1 0 [p 0 1 1 1 1 0 1 1 1 1 1 1 1 1 1 1 1 0 0 0 0 1 1 1 1 1 0 0 1 0]
	if (data[7] != 0xD4) {
		printf("LESS THAN 64 BITS PARITY FAIL\n");
		return 1;
	}
	// Test hamming encoding for 64 bits
	uint8_t data2[9] = {0x82, 0x90, 0x0A, 0x26, 0xF1, 0xFA, 0xCE, 0xEB, 0};
	CBHamming72Encode(data2, 8, data2 + 8);
	if (data2[8] != 0x39) {
		printf("64 BITS PARITY FAIL\n");
		return 1;
	}
	// Test single correction of 8 byte data
	uint8_t data3[9] = {0xFA, 0xCE, 0x00, 0x4B, 0xBA, 0xC2, 0xBA, 0x40, 0};
	CBHamming72Encode(data3, 8, data3 + 8);
	for (uint8_t x = 0; x < 72; x++) {
		data3[x/8] ^= 1 << (x % 8);
		uint8_t res = CBHamming72Check(data3, 8);
		if (res != x/8) {
			printf("SINGLE CORRECTION RES FAIL ON %u\n", x);
			return 1;
		}
		// Test that the correction was a success.
		if (memcmp(data3, (uint8_t []){0xFA, 0xCE, 0x00, 0x4B, 0xBA, 0xC2, 0xBA, 0x40, 0x28}, 7)) {
			printf("SINGLE CORRECTION DATA FAIL ON %u\n", x);
			return 1;
		}
	}
	// Test zero error
	uint8_t res = CBHamming72Check(data3, 8);
	if (res != CB_ZERO_BIT_ERROR) {
		printf("ZERO ERROR RES FAIL\n");
		return 1;
	}
	// Test double error detection
	data3[8] ^= 4;
	data3[3] ^= 64;
	res = CBHamming72Check(data3, 8);
	if (res != CB_DOUBLE_BIT_ERROR) {
		printf("DOUBLE ERROR DETECTION RES FAIL\n");
		return 1;
	}
	return 0;
}
