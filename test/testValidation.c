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
	CBByteArray * hash = CBNewByteArrayWithDataCopy(hashData, 32, &events);
	if (NOT CBValidateProofOfWork(hash, CB_MAX_TARGET)) {
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
	if (NOT CBValidateProofOfWork(hash, CB_MAX_TARGET)) {
		printf("CHECK POW LOW HASH MANTISSA FAIL\n");
		return 1;
	}
	CBByteArraySetByte(hash, 4, 0x00);
	CBByteArraySetByte(hash, 5, 0xFF);
	CBByteArraySetByte(hash, 6, 0xFF);
	if (NOT CBValidateProofOfWork(hash, CB_MAX_TARGET)) {
		printf("CHECK POW LOW HASH EXPONENT FAIL\n");
		return 1;
	}
	CBReleaseObject(hash);
	// Test CBTransactionGetSigOps
	CBTransaction * tx = CBNewTransaction(0, 1, &events);
	CBScript * script = CBNewScriptWithDataCopy((uint8_t [14]){CB_SCRIPT_OP_PUSHDATA1,0x02,0x00,0x0F,CB_SCRIPT_OP_CHECKSIG,CB_SCRIPT_OP_CHECKSIG,0x4,0xAB,0x40,0xBE,0x29,CB_SCRIPT_OP_CHECKSIGVERIFY,CB_SCRIPT_OP_CHECKMULTISIGVERIFY,CB_SCRIPT_OP_CHECKMULTISIG}, 14, &events);
	hash = CBNewByteArrayWithDataCopy(hashData, 32, &events);
	CBTransactionTakeInput(tx, CBNewTransactionInput(script, CB_TRANSACTION_INPUT_FINAL, hash, 3, &events));
	CBReleaseObject(script);
	CBReleaseObject(hash);
	script = CBNewScriptWithDataCopy((uint8_t [7]){CB_SCRIPT_OP_CHECKMULTISIG,CB_SCRIPT_OP_PUSHDATA2,0x02,0x00,0x20,0x10,CB_SCRIPT_OP_CHECKSIGVERIFY}, 7, &events);
	hashData[5] = 0xFE;
	hash = CBNewByteArrayWithDataCopy(hashData, 32, &events);
	CBTransactionTakeInput(tx, CBNewTransactionInput(script, CB_TRANSACTION_INPUT_FINAL, hash, 1, &events));
	CBReleaseObject(script);
	script = CBNewScriptWithDataCopy((uint8_t [17]){CB_SCRIPT_OP_PUSHDATA4,0x02,0x00,0x00,0x00,0x00,0x0F,CB_SCRIPT_OP_CHECKSIG,CB_SCRIPT_OP_CHECKSIG,0x4,0xAB,0x40,0xBE,0x29,CB_SCRIPT_OP_CHECKSIGVERIFY,CB_SCRIPT_OP_CHECKMULTISIGVERIFY,CB_SCRIPT_OP_CHECKMULTISIG}, 17, &events);
	CBTransactionTakeOutput(tx, CBNewTransactionOutput(4500, script, &events));
	CBReleaseObject(script);
	script = CBNewScriptWithDataCopy((uint8_t [5]){CB_SCRIPT_OP_CHECKMULTISIG,0x02,0x20,0x10,CB_SCRIPT_OP_CHECKSIGVERIFY}, 5, &events);
	CBTransactionTakeOutput(tx, CBNewTransactionOutput(39960, script, &events));
	CBReleaseObject(script);
	if (CBTransactionGetSigOps(tx) != 128) {
		printf("GET SIG OPS FAIL %i != 128\n",CBTransactionGetSigOps(tx));
		return 1;
	}
	// Test basic transaction validation
	if (NOT CBTransactionValidateBasic(tx, false)) {
		printf("BASIC VALIDATION FAIL\n");
		return 1;
	}
	CBByteArraySetByte(tx->inputs[1]->prevOut.hash, 5, 0xFF);
	tx->inputs[1]->prevOut.index = 3;
	if (CBTransactionValidateBasic(tx, false)) {
		printf("BASIC VALIDATION DUPLICATE PREV OUT FAIL\n");
		return 1;
	}
	CBByteArraySetByte(tx->inputs[0]->prevOut.hash, 4, 0x00);
	CBByteArraySetByte(tx->inputs[0]->prevOut.hash, 5, 0x00);
	if (CBTransactionValidateBasic(tx, false)) {
		printf("BASIC VALIDATION NULL HASH FAIL\n");
		return 1;
	}
	CBByteArraySetByte(tx->inputs[0]->prevOut.hash, 0, 0x01);
	tx->outputs[0]->value = CB_MAX_MONEY - 39959;
	if (CBTransactionValidateBasic(tx, false)) {
		printf("BASIC VALIDATION OVER MAX MONEY FAIL\n");
		return 1;
	}
	tx->outputs[0]->value = 0xFFFFFFFFFFFFFFFF;
	tx->outputs[1]->value = 2;
	if (CBTransactionValidateBasic(tx, false)) {
		printf("BASIC VALIDATION OVERFLOW FAIL\n");
		return 1;
	}
	CBReleaseObject(tx);
	tx = CBNewTransaction(0, 1, &events);
	script = CBNewScriptWithDataCopy((uint8_t [2]){0x01,0x00}, 2, &events);
	hashData[4] = 0;
	hashData[5] = 0;
	hash = CBNewByteArrayWithDataCopy(hashData, 32, &events);
	CBTransactionTakeInput(tx, CBNewTransactionInput(script, CB_TRANSACTION_INPUT_FINAL, hash, 0xFFFFFFFF, &events));
	CBReleaseObject(script);
	CBReleaseObject(hash);
	script = CBNewScriptWithDataCopy((uint8_t [1]){0}, 1, &events);
	CBTransactionTakeOutput(tx, CBNewTransactionOutput(50, script, &events));
	CBReleaseObject(script);
	if (NOT CBTransactionValidateBasic(tx, CBTransactionIsCoinBase(tx))) {
		printf("BASIC VALIDATION COINBASE FAIL\n");
		return 1;
	}
	if (CBTransactionValidateBasic(tx, false)) {
		printf("BASIC VALIDATION COINBASE FALSE FAIL\n");
		return 1;
	}
	CBReleaseObject(tx);
	free(hashData);
	// Test merkle root calculation from block 100,004
	uint8_t hashes[192] = {
		0x2b,0xa9,0x74,0xce,0xdd,0x1d,0xb4,0x7c,0x60,0xce,0xb1,0x91,0x04,0xd7,0x0c,0xaf,0xd8,0x8f,0xa5,0x41,0x10,0x1b,0xc5,0x3d,0x41,0xe5,0x3f,0x93,0x23,0xd4,0x7e,0xab
		,0x99,0xdb,0x12,0x8a,0xd1,0xe9,0x24,0x7b,0x8f,0x91,0x82,0xff,0x57,0xc4,0x59,0x49,0x23,0x0f,0xf2,0xe9,0xc3,0xf1,0xdd,0x26,0xe6,0xf1,0xc9,0x79,0x9a,0xe5,0x63,0xc7
		,0x6b,0x51,0xf5,0x41,0xcc,0x5a,0x50,0xdd,0x61,0xd1,0x9e,0xb9,0x45,0x23,0xb4,0xc7,0x21,0x17,0xaa,0x0c,0x4e,0x41,0xf5,0x73,0x29,0xf9,0xee,0x7a,0x64,0x3e,0xea,0x80
		,0xd3,0x8c,0x49,0x35,0xa3,0x87,0xc0,0xcd,0x06,0x58,0xbd,0xda,0xf9,0x55,0x3c,0xdf,0x74,0x32,0x21,0xe2,0x48,0xcb,0xc0,0x2e,0x36,0x0a,0xce,0x70,0xfd,0xee,0x72,0x1b
		,0x73,0x61,0xe6,0xf7,0x86,0x44,0x8f,0x2c,0xca,0xde,0xd7,0x7b,0x38,0x4a,0xe3,0x2d,0x31,0x72,0xb8,0x04,0xa6,0x9c,0x7a,0x7c,0x2d,0x9e,0xd5,0xf6,0x70,0x33,0xe1,0x23
		,0x02,0xa4,0x4b,0x64,0x15,0x8b,0x61,0x12,0x39,0x09,0x14,0x6a,0x83,0x78,0x23,0xb6,0x41,0x4a,0x7c,0x79,0x2d,0x57,0x9a,0x35,0xf9,0xb7,0x61,0x62,0x8d,0x09,0x9a,0x14};
	CBCalculateMerkleRoot(hashes, 6);
	if(memcmp(hashes, (uint8_t []){0x77,0xed,0x2a,0xf8,0x7a,0xa4,0xf9,0xf4,0x50,0xf8,0xdb,0xd1,0x52,0x84,0x72,0x0c,0x3f,0xd9,0x6f,0x56,0x5a,0x13,0xc9,0xde,0x42,0xa3,0xc1,0x44,0x0b,0x7f,0xc6,0xa5}, 32)){
		printf("MERKLE ROOT CALC FAIL\n");
		return 1;
	}
	return 0;
}
