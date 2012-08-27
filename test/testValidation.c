//
//  testValidation.c
//  cbitcoin
//
//  Created by Matthew Mitchell on 27/08/2012.
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
#include "CBValidationFunctions.h"
#include <stdarg.h>

void err(CBError a,char * format,...);
void err(CBError a,char * format,...){
	va_list argptr;
    va_start(argptr, format);
    vfprintf(stderr, format, argptr);
    va_end(argptr);
	printf("\n");
}

int main(){
	CBEvents events;
	events.onErrorReceived = err;
	// Test retargeting
	if (CBCalculateTarget(0x1D008000, CB_TARGET_INTERVAL * 2) != CB_MAX_TARGET) {
		printf("RETARGET MAXIMUM FAIL\n");
		return 1;
	}
	if (CBCalculateTarget(0x143FEBFA, CB_TARGET_INTERVAL) != 0x143FEBFA) {
		printf("RETARGET SAME FAIL\n");
		return 1;
	}
	if (CBCalculateTarget(0x1A7FFFFF, CB_TARGET_INTERVAL * 3) != 0x1B017FFF) {
		printf("RETARGET INCREASE EXPONENT FAIL\n");
		return 1;
	}
	if (CBCalculateTarget(0x1B017FFF, CB_TARGET_INTERVAL / 3) != 0x1A7FFFAA) {
		printf("RETARGET DECREASE EXPONENT FAIL\n");
		return 1;
	}
	if (CBCalculateTarget(0x1A7FFFFF, CB_TARGET_INTERVAL * 2) != 0x1B00FFFF) {
		printf("RETARGET MINUS ADJUST FAIL\n");
		return 1;
	}
	if (CBCalculateTarget(0x122FDEFB, CB_TARGET_INTERVAL * 5) != 0x1300BF7B) {
		printf("RETARGET UPPER LIMIT FAIL 0x%x != 0x1300BF7B \n",CBCalculateTarget(0x122FDEFB, CB_TARGET_INTERVAL * 5));
		return 1;
	}
	if (CBCalculateTarget(0x1B2F9D87, CB_TARGET_INTERVAL / 5) != 0x1B0BE761) {
		printf("RETARGET LOWER LIMIT FAIL\n");
		return 1;
	}
	// Test proof of work validation
	uint8_t * hashData = malloc(32);
	memset(hashData, 0, 32);
	hashData[4] = 0xFF;
	hashData[5] = 0xFF;
	CBByteArray * hash = CBNewByteArrayWithData(hashData, 32, &events);
	if (!CBValidateProofOfWork(hash, CB_MAX_TARGET)) {
		printf("CHECK POW MAX EQUAL FAIL\n");
		return 1;
	}
	if (CBValidateProofOfWork(hash, 0x1D010000)) {
		printf("CHECK POW HIGH TARGET FAIL\n");
		return 1;
	}
	if (CBValidateProofOfWork(hash, 0x1C800000)) {
		printf("CHECK POW BAD MANTISSA FAIL\n");
		return 1;
	}
	if (CBValidateProofOfWork(hash, 0x1D00FFFE)) {
		printf("CHECK POW HIGH HASH MANTISSA FAIL\n");
		return 1;
	}
	if (CBValidateProofOfWork(hash, 0x1C00FFFF)) {
		printf("CHECK POW HIGH HASH EXPONENT FAIL\n");
		return 1;
	}
	CBByteArraySetByte(hash, 5, 0xFE);
	if (!CBValidateProofOfWork(hash, CB_MAX_TARGET)) {
		printf("CHECK POW LOW HASH MANTISSA FAIL\n");
		return 1;
	}
	CBByteArraySetByte(hash, 4, 0x00);
	CBByteArraySetByte(hash, 5, 0xFF);
	CBByteArraySetByte(hash, 6, 0xFF);
	if (!CBValidateProofOfWork(hash, CB_MAX_TARGET)) {
		printf("CHECK POW LOW HASH EXPONENT FAIL\n");
		return 1;
	}
	CBReleaseObject(hash);
	return 0;
}
