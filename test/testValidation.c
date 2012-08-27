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
	return 0;
}
