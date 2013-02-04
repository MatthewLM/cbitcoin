//
//  testCBTransaction.c
//  cbitcoin
//
//  Created by Matthew Mitchell on 01/07/2012.
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
#include "CBBlock.h"
#include <time.h>
#include "openssl/ssl.h"
#include "openssl/ripemd.h"
#include "openssl/rand.h"
#include "stdarg.h"

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
	// Test genesis block
	CBByteArray * genesisMerkleRoot = CBNewByteArrayWithDataCopy((uint8_t []){0x3B, 0xA3, 0xED, 0xFD, 0x7A, 0x7B, 0x12, 0xB2, 0x7A, 0xC7, 0x2C, 0x3E, 0x67, 0x76, 0x8F, 0x61, 0x7F, 0xC8, 0x1B, 0xC3, 0x88, 0x8A, 0x51, 0x32, 0x3A, 0x9F, 0xB8, 0xAA, 0x4B, 0x1E, 0x5E, 0x4A}, 32);
	CBByteArray * genesisInScript = CBNewByteArrayWithDataCopy((uint8_t [77]){0x04, 0xFF, 0xFF, 0x00, 0x1D, 0x01, 0x04, 0x45, 0x54, 0x68, 0x65, 0x20, 0x54, 0x69, 0x6D, 0x65, 0x73, 0x20, 0x30, 0x33, 0x2F, 0x4A, 0x61, 0x6E, 0x2F, 0x32, 0x30, 0x30, 0x39, 0x20, 0x43, 0x68, 0x61, 0x6E, 0x63, 0x65, 0x6C, 0x6C, 0x6F, 0x72, 0x20, 0x6F, 0x6E, 0x20, 0x62, 0x72, 0x69, 0x6E, 0x6B, 0x20, 0x6F, 0x66, 0x20, 0x73, 0x65, 0x63, 0x6F, 0x6E, 0x64, 0x20, 0x62, 0x61, 0x69, 0x6C, 0x6F, 0x75, 0x74, 0x20, 0x66, 0x6F, 0x72, 0x20, 0x62, 0x61, 0x6E, 0x6B, 0x73}, 77);
	CBByteArray * genesisOutScript = CBNewByteArrayWithDataCopy((uint8_t [67]){0x41, 0x04, 0x67, 0x8A, 0xFD, 0xB0, 0xFE, 0x55, 0x48, 0x27, 0x19, 0x67, 0xF1, 0xA6, 0x71, 0x30, 0xB7, 0x10, 0x5C, 0xD6, 0xA8, 0x28, 0xE0, 0x39, 0x09, 0xA6, 0x79, 0x62, 0xE0, 0xEA, 0x1F, 0x61, 0xDE, 0xB6, 0x49, 0xF6, 0xBC, 0x3F, 0x4C, 0xEF, 0x38, 0xC4, 0xF3, 0x55, 0x04, 0xE5, 0x1E, 0xC1, 0x12, 0xDE, 0x5C, 0x38, 0x4D, 0xF7, 0xBA, 0x0B, 0x8D, 0x57, 0x8A, 0x4C, 0x70, 0x2B, 0x6B, 0xF1, 0x1D, 0x5F, 0xAC}, 67);
	// Test hash
	CBBlock * genesisBlock = CBNewBlockGenesis();
	uint8_t calcHash[32];
	CBBlockCalculateHash(genesisBlock, calcHash);
	if(memcmp(genesisBlock->hash, calcHash, 32)){
		printf("GENESIS BLOCK HASH FAIL\n0x");
		uint8_t * d = genesisBlock->hash;
		for (int x = 0; x < 32; x++) {
			printf("%.2X", d[x]);
		}
		printf("\n!=\n0x");
		d = calcHash;
		for (int x = 0; x < 32; x++) {
			printf("%.2X", d[x]);
		}
		return 1;
	}
	// Test deserialised data
	if (genesisBlock->version != 1) {
		printf("GENESIS BLOCK VERSION FAIL\n");
		return 1;
	}
	for (int x = 0; x < 32; x++) {
		if(CBByteArrayGetByte(genesisBlock->prevBlockHash, x) != 0){
			printf("GENESIS BLOCK PREV FAIL\n");
			return 1;
		}
	}
	if (CBByteArrayCompare(genesisBlock->merkleRoot, genesisMerkleRoot)) {
		printf("GENESIS BLOCK MERKLE ROOT FAIL\n0x");
		uint8_t * d = CBByteArrayGetData(genesisBlock->merkleRoot);
		for (int x = 0; x < 32; x++) {
			printf("%.2X", d[x]);
		}
		printf("\n!=\n0x");
		d = CBByteArrayGetData(genesisMerkleRoot);
		for (int x = 0; x < 32; x++) {
			printf("%.2X", d[x]);
		}
		return 1;
	}
	if (genesisBlock->time != 1231006505) {
		printf("GENESIS BLOCK TIME FAIL\n0x");
		return 1;
	}
	if (genesisBlock->target != 0x1D00FFFF) {
		printf("GENESIS BLOCK DIFFICULTY FAIL\n0x");
		return 1;
	}
	if (genesisBlock->nonce != 2083236893) {
		printf("GENESIS BLOCK DIFFICULTY FAIL\n0x");
		return 1;
	}
	if (genesisBlock->transactionNum != 1) {
		printf("GENESIS BLOCK TRANSACTION NUM FAIL\n0x");
		return 1;
	}
	CBTransaction * genesisCoinBase = genesisBlock->transactions[0];
	if (genesisCoinBase->inputNum != 1) {
		printf("GENESIS BLOCK TRANSACTION INPUT NUM FAIL\n0x");
		return 1;
	}
	if (genesisCoinBase->outputNum != 1) {
		printf("GENESIS BLOCK TRANSACTION OUTPUT NUM FAIL\n0x");
		return 1;
	}
	if (genesisCoinBase->version != 1) {
		printf("GENESIS BLOCK TRANSACTION VERSION FAIL\n0x");
		return 1;
	}
	if (genesisCoinBase->lockTime != 0) {
		printf("GENESIS BLOCK TRANSACTION LOCK TIME FAIL\n0x");
		return 1;
	}
	if (genesisCoinBase->inputs[0]->scriptObject->length != 0x4D) {
		printf("GENESIS BLOCK TRANSACTION INPUT SCRIPT LENGTH FAIL\n0x");
		return 1;
	}
	if (genesisCoinBase->outputs[0]->scriptObject->length != 0x43) {
		printf("GENESIS BLOCK TRANSACTION OUTPUT SCRIPT LENGTH FAIL\n0x");
		return 1;
	}
	for (int x = 0; x < 32; x++) {
		if(CBByteArrayGetByte(genesisCoinBase->inputs[0]->prevOut.hash, x) != 0){
			printf("GENESIS BLOCK TRANSACTION INPUT OUT POINTER HASH FAIL\n");
			return 1;
		}
	}
	if (genesisCoinBase->inputs[0]->prevOut.index != 0xFFFFFFFF) {
		printf("GENESIS BLOCK TRANSACTION INPUT OUT POINTER INDEX FAIL\n0x");
		return 1;
	}
	if (genesisCoinBase->inputs[0]->sequence != CB_TRANSACTION_INPUT_FINAL) {
		printf("GENESIS BLOCK TRANSACTION INPUT SEQUENCE FAIL\n0x");
		return 1;
	}
	if (CBByteArrayCompare(genesisCoinBase->inputs[0]->scriptObject, genesisInScript)) {
		printf("GENESIS BLOCK IN SCRIPT FAIL\n0x");
		uint8_t * d = CBByteArrayGetData(genesisCoinBase->inputs[0]->scriptObject);
		for (int x = 0; x < genesisCoinBase->inputs[0]->scriptObject->length; x++) {
			printf("%.2X", d[x]);
		}
		printf("\n!=\n0x");
		d = CBByteArrayGetData(genesisInScript);
		for (int x = 0; x < genesisInScript->length; x++) {
			printf("%.2X", d[x]);
		}
		return 1;
	}
	if (genesisCoinBase->outputs[0]->value != 5000000000) {
		printf("GENESIS BLOCK TRANSACTION OUTPUT VALUE FAIL\n0x");
		return 1;
	}
	if (CBByteArrayCompare(genesisCoinBase->outputs[0]->scriptObject, genesisOutScript)) {
		printf("GENESIS BLOCK OUT SCRIPT FAIL\n0x");
		uint8_t * d = CBByteArrayGetData(genesisCoinBase->outputs[0]->scriptObject);
		for (int x = 0; x < genesisCoinBase->outputs[0]->scriptObject->length; x++) {
			printf("%.2X", d[x]);
		}
		printf("\n!=\n0x");
		d = CBByteArrayGetData(genesisOutScript);
		for (int x = 0; x < genesisOutScript->length; x++) {
			printf("%.2X", d[x]);
		}
		return 1;
	}
	// Test serialisation into genesis block
	CBBlock * block = CBNewBlock();
	block->version = 1;
	uint8_t * zeroHash = malloc(32);
	memset(zeroHash, 0, 32);
	block->prevBlockHash = CBNewByteArrayWithData(zeroHash, 32);
	block->merkleRoot = genesisMerkleRoot;
	block->target = 0x1D00FFFF;
	block->time = 1231006505;
	block->nonce = 2083236893;
	block->transactionNum = 1;
	block->transactions = malloc(sizeof(*block->transactions));
	block->transactions[0] = CBNewTransaction(0, 1);
	CBRetainObject(block->prevBlockHash); // Retain for the zero hash in the input
	CBTransactionTakeInput(block->transactions[0], CBNewTransactionInput(genesisInScript, CB_TRANSACTION_INPUT_FINAL, block->prevBlockHash, 0xFFFFFFFF));
	CBTransactionTakeOutput(block->transactions[0], CBNewTransactionOutput(5000000000, genesisOutScript));
	CBGetMessage(block)->bytes = CBNewByteArrayOfSize(CBGetMessage(genesisBlock)->bytes->length);
	CBBlockSerialise(block, true, true);
	if (CBByteArrayCompare(CBGetMessage(block)->bytes, CBGetMessage(genesisBlock)->bytes)) {
		printf("SERIALISATION TO GENESIS BLOCK FAIL\n0x");
		uint8_t * d = CBByteArrayGetData(CBGetMessage(block)->bytes);
		for (int x = 0; x < CBGetMessage(block)->bytes->length; x++) {
			printf("%.2X", d[x]);
		}
		printf("\n!=\n0x");
		d = CBByteArrayGetData(CBGetMessage(genesisBlock)->bytes);
		for (int x = 0; x < CBGetMessage(genesisBlock)->bytes->length; x++) {
			printf("%.2X", d[x]);
		}
		return 1;
	}
	CBReleaseObject(genesisBlock);
	CBReleaseObject(block);
	// Test serailisation of a block, and then move a transaction. Then reserialise without forcing full serialisation
	block = CBNewBlock();
	block->prevBlockHash = CBNewByteArrayOfSize(32);
	memset(CBByteArrayGetData(block->prevBlockHash), 0, 32);
	block->merkleRoot = CBNewByteArrayOfSize(32);
	block->target = 0x1D00FFFF;
	block->time = 1231006505;
	block->nonce = 2083236893;
	block->transactionNum = 2;
	block->transactions = malloc(sizeof(*block->transactions) * 2);
	block->transactions[0] = CBNewTransaction(0, 1);
	CBTransactionTakeInput(block->transactions[0], CBNewTransactionInput(genesisInScript, CB_TRANSACTION_INPUT_FINAL, block->prevBlockHash, 0xFFFFFFFF));
	for (uint8_t x = 0; x < 2; x++) {
		CBTransactionTakeOutput(block->transactions[0], CBNewTransactionOutput(5000000000, genesisOutScript));
	}
	block->transactions[1] = CBNewTransaction(0, 1);
	CBTransactionTakeInput(block->transactions[1], CBNewTransactionInput(genesisInScript, CB_TRANSACTION_INPUT_FINAL, block->prevBlockHash, 0xFFFFFFFF));
	CBTransactionTakeOutput(block->transactions[1], CBNewTransactionOutput(5000000000, genesisOutScript));
	CBGetMessage(block)->bytes = CBNewByteArrayOfSize(CBBlockCalculateLength(block, true));
	if (CBBlockSerialise(block, true, true) != CBBlockCalculateLength(block, true)){
		printf("SERAILISATION OF OTHER BLOCK FAIL\n");
		return 1;
	}
	block->transactions[0]->outputNum--;
	CBTransactionSerialise(block->transactions[0], true);
	if (CBBlockSerialise(block, true, false) != CBBlockCalculateLength(block, true)){
		printf("SERAILISATION AFTER MOVEMENT FAIL\n");
		return 1;
	}
	if (CBBlockDeserialise(block, true) != CBBlockCalculateLength(block, true)){
		printf("DESERAILISATION AFTER MOVEMENT FAIL\n");
		return 1;
	}
	return 0;
}
