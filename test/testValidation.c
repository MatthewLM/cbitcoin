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
#include "CBMerkleNode.h"
#include <stdarg.h>

void CBLogError(char * format, ...);
void CBLogError(char * format, ...){
	va_list argptr;
    va_start(argptr, format);
    vfprintf(stderr, format, argptr);
    va_end(argptr);
	printf("\n");
}

int main(){
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
		printf("RETARGET UPPER LIMIT FAIL 0x%x != 0x1300BF7B \n", CBCalculateTarget(0x122FDEFB, CB_TARGET_INTERVAL * 5));
		return 1;
	}
	if (CBCalculateTarget(0x1B2F9D87, CB_TARGET_INTERVAL / 5) != 0x1B0BE761) {
		printf("RETARGET LOWER LIMIT FAIL\n");
		return 1;
	}
	// Test proof of work validation
	uint8_t hashData[32];
	memset(hashData, 0, 32);
	hashData[27] = 0xFF;
	hashData[26] = 0xFF;
	if (NOT CBValidateProofOfWork(hashData, CB_MAX_TARGET)) {
		printf("CHECK POW MAX EQUAL FAIL\n");
		return 1;
	}
	if (CBValidateProofOfWork(hashData, 0x1D010000)) {
		printf("CHECK POW HIGH TARGET FAIL\n");
		return 1;
	}
	if (CBValidateProofOfWork(hashData, 0x1C800000)) {
		printf("CHECK POW BAD MANTISSA FAIL\n");
		return 1;
	}
	if (CBValidateProofOfWork(hashData, 0x1D00FFFE)) {
		printf("CHECK POW HIGH HASH MANTISSA FAIL\n");
		return 1;
	}
	if (CBValidateProofOfWork(hashData, 0x1C00FFFF)) {
		printf("CHECK POW HIGH HASH EXPONENT FAIL\n");
		return 1;
	}
	hashData[26] = 0xFE;
	if (NOT CBValidateProofOfWork(hashData, CB_MAX_TARGET)) {
		printf("CHECK POW LOW HASH MANTISSA FAIL\n");
		return 1;
	}
	hashData[27] = 0x00;
	hashData[26] = 0xFF;
	hashData[25] = 0xFF;
	if (NOT CBValidateProofOfWork(hashData, CB_MAX_TARGET)) {
		printf("CHECK POW LOW HASH EXPONENT FAIL\n");
		return 1;
	}
	hashData[25] = 0x00;
	// Test CBTransactionGetSigOps
	CBTransaction * tx = CBNewTransaction(0, 1);
	CBScript * script = CBNewScriptWithDataCopy((uint8_t [14]){CB_SCRIPT_OP_PUSHDATA1, 0x02, 0x00, 0x0F, CB_SCRIPT_OP_CHECKSIG, CB_SCRIPT_OP_CHECKSIG, 0x4, 0xAB, 0x40, 0xBE, 0x29, CB_SCRIPT_OP_CHECKSIGVERIFY, CB_SCRIPT_OP_CHECKMULTISIGVERIFY, CB_SCRIPT_OP_CHECKMULTISIG}, 14);
	CBByteArray * hash = CBNewByteArrayWithDataCopy(hashData, 32);
	CBTransactionTakeInput(tx, CBNewTransactionInput(script, CB_TRANSACTION_INPUT_FINAL, hash, 3));
	CBReleaseObject(script);
	CBReleaseObject(hash);
	script = CBNewScriptWithDataCopy((uint8_t [7]){CB_SCRIPT_OP_CHECKMULTISIG, CB_SCRIPT_OP_PUSHDATA2, 0x02, 0x00, 0x20, 0x10, CB_SCRIPT_OP_CHECKSIGVERIFY}, 7);
	hashData[26] = 0xFE;
	hash = CBNewByteArrayWithDataCopy(hashData, 32);
	CBTransactionTakeInput(tx, CBNewTransactionInput(script, CB_TRANSACTION_INPUT_FINAL, hash, 1));
	CBReleaseObject(script);
	script = CBNewScriptWithDataCopy((uint8_t [17]){CB_SCRIPT_OP_PUSHDATA4, 0x02, 0x00, 0x00, 0x00, 0x00, 0x0F, CB_SCRIPT_OP_CHECKSIG, CB_SCRIPT_OP_CHECKSIG, 0x4, 0xAB, 0x40, 0xBE, 0x29, CB_SCRIPT_OP_CHECKSIGVERIFY, CB_SCRIPT_OP_CHECKMULTISIGVERIFY, CB_SCRIPT_OP_CHECKMULTISIG}, 17);
	CBTransactionTakeOutput(tx, CBNewTransactionOutput(4500, script));
	CBReleaseObject(script);
	script = CBNewScriptWithDataCopy((uint8_t [5]){CB_SCRIPT_OP_CHECKMULTISIG, 0x02, 0x20, 0x10, CB_SCRIPT_OP_CHECKSIGVERIFY}, 5);
	CBTransactionTakeOutput(tx, CBNewTransactionOutput(39960, script));
	CBReleaseObject(script);
	if (CBTransactionGetSigOps(tx) != 128) {
		printf("GET SIG OPS FAIL %i != 128\n", CBTransactionGetSigOps(tx));
		return 1;
	}
	// Test basic transaction validation
	uint64_t value;
	if (NOT CBTransactionValidateBasic(tx, false, &value)) {
		printf("BASIC VALIDATION FAIL\n");
		return 1;
	}
	if (value != 44460) {
		printf("BASIC VALIDATION VALUE FAIL\n");
		return 1;
	}
	CBByteArraySetByte(tx->inputs[1]->prevOut.hash, 26, 0xFF);
	tx->inputs[1]->prevOut.index = 3;
	if (CBTransactionValidateBasic(tx, false, &value)) {
		printf("BASIC VALIDATION DUPLICATE PREV OUT FAIL\n");
		return 1;
	}
	CBByteArraySetByte(tx->inputs[0]->prevOut.hash, 27, 0x00);
	CBByteArraySetByte(tx->inputs[0]->prevOut.hash, 26, 0x00);
	if (CBTransactionValidateBasic(tx, false, &value)) {
		printf("BASIC VALIDATION NULL HASH FAIL\n");
		return 1;
	}
	CBByteArraySetByte(tx->inputs[0]->prevOut.hash, 31, 0x01);
	tx->outputs[0]->value = CB_MAX_MONEY - 39959;
	if (CBTransactionValidateBasic(tx, false, &value)) {
		printf("BASIC VALIDATION OVER MAX MONEY FAIL\n");
		return 1;
	}
	tx->outputs[0]->value = 0xFFFFFFFFFFFFFFFF;
	tx->outputs[1]->value = 2;
	if (CBTransactionValidateBasic(tx, false, &value)) {
		printf("BASIC VALIDATION OVERFLOW FAIL\n");
		return 1;
	}
	CBReleaseObject(tx);
	tx = CBNewTransaction(0, 1);
	script = CBNewScriptWithDataCopy((uint8_t [2]){0x01, 0x00}, 2);
	hashData[27] = 0;
	hashData[26] = 0;
	hash = CBNewByteArrayWithDataCopy(hashData, 32);
	CBTransactionTakeInput(tx, CBNewTransactionInput(script, CB_TRANSACTION_INPUT_FINAL, hash, 0xFFFFFFFF));
	CBReleaseObject(script);
	CBReleaseObject(hash);
	script = CBNewScriptWithDataCopy((uint8_t [1]){0}, 1);
	CBTransactionTakeOutput(tx, CBNewTransactionOutput(50, script));
	CBReleaseObject(script);
	if (NOT CBTransactionValidateBasic(tx, CBTransactionIsCoinBase(tx), &value)) {
		printf("BASIC VALIDATION COINBASE FAIL\n");
		return 1;
	}
	if (CBTransactionValidateBasic(tx, false, &value)) {
		printf("BASIC VALIDATION COINBASE FALSE FAIL\n");
		return 1;
	}
	tx->inputs[0]->scriptObject->length = 1;
	if (CBTransactionValidateBasic(tx, true, &value)) {
		printf("BASIC VALIDATION COINBASE SCRIPT LOW FAIL\n");
		return 1;
	}
	tx->inputs[0]->scriptObject->length = 101;
	if (CBTransactionValidateBasic(tx, true, &value)) {
		printf("BASIC VALIDATION COINBASE SCRIPT HIGH FAIL\n");
		return 1;
	}
	tx->inputNum = 0;
	if (CBTransactionValidateBasic(tx, true, &value)) {
		printf("BASIC VALIDATION NO INPUTS FAIL\n");
		return 1;
	}
	tx->inputNum = 1;
	tx->outputNum = 0;
	if (CBTransactionValidateBasic(tx, true, &value)) {
		printf("BASIC VALIDATION NO OUTPUTS FAIL\n");
		return 1;
	}
	CBReleaseObject(tx);
	// Test merkle root calculation from block 100, 004
	uint8_t hashes[192] = {
		0x2b, 0xa9, 0x74, 0xce, 0xdd, 0x1d, 0xb4, 0x7c, 0x60, 0xce, 0xb1, 0x91, 0x04, 0xd7, 0x0c, 0xaf, 0xd8, 0x8f, 0xa5, 0x41, 0x10, 0x1b, 0xc5, 0x3d, 0x41, 0xe5, 0x3f, 0x93, 0x23, 0xd4, 0x7e, 0xab
		, 0x99, 0xdb, 0x12, 0x8a, 0xd1, 0xe9, 0x24, 0x7b, 0x8f, 0x91, 0x82, 0xff, 0x57, 0xc4, 0x59, 0x49, 0x23, 0x0f, 0xf2, 0xe9, 0xc3, 0xf1, 0xdd, 0x26, 0xe6, 0xf1, 0xc9, 0x79, 0x9a, 0xe5, 0x63, 0xc7
		, 0x6b, 0x51, 0xf5, 0x41, 0xcc, 0x5a, 0x50, 0xdd, 0x61, 0xd1, 0x9e, 0xb9, 0x45, 0x23, 0xb4, 0xc7, 0x21, 0x17, 0xaa, 0x0c, 0x4e, 0x41, 0xf5, 0x73, 0x29, 0xf9, 0xee, 0x7a, 0x64, 0x3e, 0xea, 0x80
		, 0xd3, 0x8c, 0x49, 0x35, 0xa3, 0x87, 0xc0, 0xcd, 0x06, 0x58, 0xbd, 0xda, 0xf9, 0x55, 0x3c, 0xdf, 0x74, 0x32, 0x21, 0xe2, 0x48, 0xcb, 0xc0, 0x2e, 0x36, 0x0a, 0xce, 0x70, 0xfd, 0xee, 0x72, 0x1b
		, 0x73, 0x61, 0xe6, 0xf7, 0x86, 0x44, 0x8f, 0x2c, 0xca, 0xde, 0xd7, 0x7b, 0x38, 0x4a, 0xe3, 0x2d, 0x31, 0x72, 0xb8, 0x04, 0xa6, 0x9c, 0x7a, 0x7c, 0x2d, 0x9e, 0xd5, 0xf6, 0x70, 0x33, 0xe1, 0x23
		, 0x02, 0xa4, 0x4b, 0x64, 0x15, 0x8b, 0x61, 0x12, 0x39, 0x09, 0x14, 0x6a, 0x83, 0x78, 0x23, 0xb6, 0x41, 0x4a, 0x7c, 0x79, 0x2d, 0x57, 0x9a, 0x35, 0xf9, 0xb7, 0x61, 0x62, 0x8d, 0x09, 0x9a, 0x14};
	uint8_t hashes2[192];
	memcpy(hashes2, hashes, 192);
	CBCalculateMerkleRoot(hashes2, 6);
	if(memcmp(hashes2, (uint8_t []){0x77, 0xed, 0x2a, 0xf8, 0x7a, 0xa4, 0xf9, 0xf4, 0x50, 0xf8, 0xdb, 0xd1, 0x52, 0x84, 0x72, 0x0c, 0x3f, 0xd9, 0x6f, 0x56, 0x5a, 0x13, 0xc9, 0xde, 0x42, 0xa3, 0xc1, 0x44, 0x0b, 0x7f, 0xc6, 0xa5}, 32)){
		printf("MERKLE ROOT CALC FAIL\n");
		return 1;
	}
	// Test merkle tree generation
	CBByteArray * hashObjs[6];
	for (int x = 0; x < 6; x++) {
		hashObjs[x] = CBNewByteArrayWithDataCopy(hashes + x*32, 32);
	}
	CBMerkleNode * root = CBBuildMerkleTree(hashObjs, 6);
	if (memcmp(root->hash, (uint8_t []){0x77, 0xed, 0x2a, 0xf8, 0x7a, 0xa4, 0xf9, 0xf4, 0x50, 0xf8, 0xdb, 0xd1, 0x52, 0x84, 0x72, 0x0c, 0x3f, 0xd9, 0x6f, 0x56, 0x5a, 0x13, 0xc9, 0xde, 0x42, 0xa3, 0xc1, 0x44, 0x0b, 0x7f, 0xc6, 0xa5}, 32)) {
		printf("MERKLE ROOT BUILD FAIL\n");
		return 1;
	}
	if (memcmp(root->left->hash, (uint8_t []){0xf8, 0xfe, 0x55, 0x99, 0x8d, 0x04, 0xb2, 0xa0, 0xdb, 0x87, 0x6a, 0x2b, 0x0e, 0x5f, 0xfe, 0x74, 0xa9, 0x84, 0xe0, 0x2e, 0x77, 0x85, 0x39, 0x9a, 0xd1, 0xd4, 0x08, 0xee, 0xc0, 0x5d, 0x78, 0x42}, 32)) {
		printf("MERKLE LEFT BUILD FAIL\n");
		return 1;
	}
	if (memcmp(root->right->hash, (uint8_t []){0x47, 0xbd, 0xb6, 0x87, 0x85, 0x50, 0x15, 0x6e, 0x9e, 0x1e, 0xd9, 0x8a, 0x15, 0x4b, 0xdf, 0xe9, 0xf2, 0xc6, 0xd7, 0xf7, 0x52, 0xe6, 0xe4, 0x8a, 0x1f, 0x7c, 0x59, 0xfb, 0x13, 0x65, 0xd2, 0x1f}, 32)) {
		printf("MERKLE RIGHT BUILD FAIL\n");
		return 1;
	}
	if (memcmp(root->left->left->hash, (uint8_t []){0x54, 0x2b, 0x14, 0xf5, 0xcb, 0x3c, 0xf5, 0xf1, 0x9c, 0xce, 0x39, 0x51, 0x1f, 0x61, 0xa0, 0xda, 0x41, 0xcf, 0x47, 0x06, 0x15, 0xaf, 0x89, 0x46, 0x36, 0x86, 0x41, 0x98, 0x3b, 0xb8, 0xc6, 0xaa}, 32)) {
		printf("MERKLE LEFT LEFT BUILD FAIL\n");
		return 1;
	}
	if (memcmp(root->left->right->hash, (uint8_t []){0xa5, 0x76, 0x6a, 0xe3, 0xad, 0x35, 0x3c, 0x68, 0x95, 0x4b, 0xa0, 0xad, 0x93, 0x82, 0x96, 0x61, 0x81, 0x7f, 0xd4, 0x17, 0xef, 0x24, 0x6f, 0x85, 0xcd, 0x79, 0x81, 0x90, 0xd8, 0x0e, 0xee, 0x3e}, 32)) {
		printf("MERKLE LEFT RIGHT BUILD FAIL\n");
		return 1;
	}
	if (root->right->left != root->right->right) {
		printf("MERKLE DUPLICATE FAIL\n");
	}
	if (memcmp(root->right->left->hash, (uint8_t []){0xc4, 0x81, 0x75, 0x10, 0x0b, 0xfb, 0xbe, 0x50, 0x29, 0xc8, 0x27, 0x69, 0x1a, 0xa8, 0xcb, 0x97, 0x7f, 0x84, 0x51, 0x3b, 0x12, 0xf4, 0xc9, 0x58, 0x87, 0x11, 0x40, 0x94, 0x5a, 0xdd, 0xac, 0x6b}, 32)) {
		printf("MERKLE RIGHT LEFT BUILD FAIL\n");
		return 1;
	}
	if (memcmp(root->left->left->left->hash, hashes, 32)) {
		printf("MERKLE TX 1 BUILD FAIL\n");
		return 1;
	}
	if (memcmp(root->left->left->right->hash, hashes + 32, 32)) {
		printf("MERKLE TX 2 BUILD FAIL\n");
		return 1;
	}
	if (memcmp(root->left->right->left->hash, hashes + 64, 32)) {
		printf("MERKLE TX 3 BUILD FAIL\n");
		return 1;
	}
	if (memcmp(root->left->right->right->hash, hashes + 96, 32)) {
		printf("MERKLE TX 4 BUILD FAIL\n");
		return 1;
	}
	if (memcmp(root->right->left->left->hash, hashes + 128, 32)) {
		printf("MERKLE TX 5 BUILD FAIL\n");
		return 1;
	}
	if (memcmp(root->right->left->right->hash, hashes + 160, 32)) {
		printf("MERKLE TX 6 BUILD FAIL\n");
		return 1;
	}
	if (root->left->left->left->left || root->left->left->left->right) {
		printf("MERKLE TX 1 NULL BUILD FAIL\n");
		return 1;
	}
	if (root->left->left->right->left || root->left->left->right->right) {
		printf("MERKLE TX 2 NULL BUILD FAIL\n");
		return 1;
	}
	if (root->left->right->left->left || root->left->right->left->right) {
		printf("MERKLE TX 3 NULL BUILD FAIL\n");
		return 1;
	}
	if (root->left->right->right->left || root->left->right->right->right) {
		printf("MERKLE TX 4 NULL BUILD FAIL\n");
		return 1;
	}
	if (root->right->left->left->left || root->right->left->left->right) {
		printf("MERKLE TX 5 NULL BUILD FAIL\n");
		return 1;
	}
	if (root->right->left->right->left || root->right->left->right->right) {
		printf("MERKLE TX 6 NULL BUILD FAIL\n");
		return 1;
	}
	if(CBMerkleTreeGetLevel(root, 0) != root){
		printf("MERKLE LEVEL 0 INDEX 0 FAIL\n");
		return 1;
	}
	CBMerkleNode * lev = CBMerkleTreeGetLevel(root, 1);
	if(lev != root->left){
		printf("MERKLE LEVEL 1 INDEX 0 FAIL\n");
		return 1;
	}
	if(lev + 1 != root->right){
		printf("MERKLE LEVEL 1 INDEX 1 FAIL\n");
		return 1;
	}
	lev = CBMerkleTreeGetLevel(root, 2);
	if(lev != root->left->left){
		printf("MERKLE LEVEL 2 INDEX 0 FAIL\n");
		return 1;
	}
	if(lev + 1 != root->left->right){
		printf("MERKLE LEVEL 2 INDEX 1 FAIL\n");
		return 1;
	}
	if(lev + 2 != root->right->right){
		printf("MERKLE LEVEL 2 INDEX 2 FAIL\n");
		return 1;
	}
	lev = CBMerkleTreeGetLevel(root, 3);
	if(lev != root->left->left->left){
		printf("MERKLE LEVEL 3 INDEX 0 FAIL\n");
		return 1;
	}
	if(lev + 1 != root->left->left->right){
		printf("MERKLE LEVEL 3 INDEX 1 FAIL\n");
		return 1;
	}
	if(lev + 2 != root->left->right->left){
		printf("MERKLE LEVEL 3 INDEX 2 FAIL\n");
		return 1;
	}
	if(lev + 3 != root->left->right->right){
		printf("MERKLE LEVEL 3 INDEX 3 FAIL\n");
		return 1;
	}
	if(lev + 4 != root->right->left->left){
		printf("MERKLE LEVEL 3 INDEX 4 FAIL\n");
		return 1;
	}
	if(lev + 5 != root->right->left->right){
		printf("MERKLE LEVEL 3 INDEX 5 FAIL\n");
		return 1;
	}
	CBFreeMerkleTree(root);
	// Test work calculation
	CBBigInt work;
	CBCalculateBlockWork(&work, 0x1708ABCD);
	if (memcmp(work.data, (uint8_t []){0x00, 0x43, 0x23, 0x36, 0xD5, 0x1D, 0x95, 0xFB, 0x85, 0x1D}, 10)) {
		printf("BLOCK WORK CALCULATION FAIL\n");
		return 1;
	}
	free(work.data);
	CBCalculateBlockWork(&work, 0x10008F00);
	if (memcmp(work.data, (uint8_t []){0x4B, 0xCA, 0x01, 0x91, 0xE1, 0x5E, 0x05, 0xB3, 0xA4, 0x1C, 0x10, 0x19, 0xEE, 0x55, 0x30, 0x4B, 0xCA, 0x01}, 18)) {
		printf("BLOCK WORK CALCULATION TWO FAIL\n");
		return 1;
	}
	free(work.data);
	CBCalculateBlockWork(&work, CB_MAX_TARGET);
	if (memcmp(work.data, (uint8_t []){0x01, 0x00, 0x01, 0x00, 0x01}, 5)) {
		printf("BLOCK WORK CALCULATION THREE FAIL\n");
		return 1;
	}
	free(work.data);
	// Test transaction lock
	tx = CBNewTransaction(0, 1);
	CBScript * nullScript = CBNewScriptOfSize(0);
	CBTransactionTakeInput(tx, CBNewTransactionInput(nullScript, CB_TRANSACTION_INPUT_FINAL, nullScript, 0));
	if (NOT CBTransactionIsFinal(tx, 0, 0)) {
		printf("TX FINAL INPUT FAIL\n");
		return 1;
	}
	CBTransactionTakeInput(tx, CBNewTransactionInput(nullScript, CB_TRANSACTION_INPUT_FINAL - 1, nullScript, 0));
	if (NOT CBTransactionIsFinal(tx, 0, 0)) {
		printf("TX NOT FINAL INPUT NO LOCKTIME FAIL\n");
		return 1;
	}
	tx->lockTime = 1;
	if (CBTransactionIsFinal(tx, 0, 0)) {
		printf("TX NOT FINAL INPUT FAIL\n");
		return 1;
	}
	tx->lockTime = 56;
	if (NOT CBTransactionIsFinal(tx, 0, 57)) {
		printf("TX FINAL BLOCK LOCKTIME FAIL\n");
		return 1;
	}
	if (CBTransactionIsFinal(tx, 0, 56)) {
		printf("TX NOT FINAL BLOCK LOCKTIME FAIL\n");
		return 1;
	}
	tx->lockTime = 500000000;
	if (NOT CBTransactionIsFinal(tx, 500000001, 0)) {
		printf("TX FINAL TIME LOCKTIME FAIL\n");
		return 1;
	}
	if (CBTransactionIsFinal(tx, 500000000, 0)) {
		printf("TX NOT FINAL TIME LOCKTIME FAIL\n");
		return 1;
	}
	CBReleaseObject(tx->inputs[1]);
	tx->inputNum--;
	if (NOT CBTransactionIsFinal(tx, 500000000, 0)) {
		printf("TX ALL FINAL SEQUENCES FAIL\n");
		return 1;
	}
	return 0;
}
