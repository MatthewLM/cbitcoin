//
//  testCBTransaction.c
//  cbitcoin
//
//  Created by Matthew Mitchell on 30/05/2012.
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
#include "CBTransaction.h"
#include <time.h>
#include "openssl/ssl.h"
#include "openssl/ripemd.h"
#include "openssl/rand.h"
#include "stdarg.h"
#include <inttypes.h>

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
	// Test CBTransactionInput
	// Test deserialisation
	uint8_t hash[32];
	CBSha256((uint8_t *)"hello", 5, hash);
	CBByteArray * bytes = CBNewByteArrayOfSize(42);
	CBByteArraySetBytes(bytes, 0, hash, 32);
	CBByteArraySetBytes(bytes, 32, (uint8_t []){0x05, 0xFA, 0x4B, 0x51}, 4);
	CBVarIntEncode(bytes, 36, CBVarIntFromUInt64(1));
	CBByteArraySetByte(bytes, 37, CB_SCRIPT_OP_TRUE);
	uint32_t sequence = rand();
	CBByteArraySetInt32(bytes, 38, sequence);
	CBTransactionInput * input = CBNewTransactionInputFromData(bytes);
	CBTransactionInputDeserialise(input);
	if(memcmp(hash, CBByteArrayGetData(input->prevOut.hash), 32)){
		printf("CBTransactionInput DESERIALISED outPointerHash INCORRECT: 0x");
		for (int x = 0; x < 32; x++) {
			printf("%.2x", hash[x]);
		}
		printf(" != 0x");
		uint8_t * d = CBByteArrayGetData(input->prevOut.hash);
		for (int x = 0; x < 32; x++) {
			printf("%.2x", d[x]);
		}
		return 1;
	}
	if (input->prevOut.index != 0x514BFA05) {
		printf("CBTransactionInput DESERIALISED outPointerIndex INCORRECT: %i != 0x514BFA05\n", input->prevOut.index);
		return 1;
	}
	CBScriptStack stack = CBNewEmptyScriptStack();
	if(CBScriptExecute(input->scriptObject, &stack, NULL, NULL, 0, false) != CB_SCRIPT_TRUE){
		printf("CBTransactionInput DESERIALISED SCRIPT FAILURE\n");
		return 1;
	}
	if (input->sequence != sequence) {
		printf("CBTransactionInput DESERIALISED sequence INCORRECT: %i != %i\n", input->sequence, sequence);
		return 1;
	}
	CBReleaseObject(input);
	// Test serialisation
	CBScript * scriptObj = CBNewScriptWithDataCopy((uint8_t []){CB_SCRIPT_OP_TRUE}, 1 );
	CBByteArray * outPointerHash = CBNewByteArrayWithDataCopy(hash, 32);
	input = CBNewTransactionInput(scriptObj, sequence, outPointerHash, 0x514BFA05);
	CBGetMessage(input)->bytes = CBNewByteArrayOfSize(42);
	CBTransactionInputSerialise(input);
	if (CBByteArrayCompare(bytes, CBGetMessage(input)->bytes)) {
		printf("CBTransactionInput SERIALISATION FAILURE\n0x");
		uint8_t * d = CBByteArrayGetData(CBGetMessage(input)->bytes);
		for (int x = 0; x < 42; x++) {
			printf("%.2x", d[x]);
		}
		printf("\n!=\n0x");
		d = CBByteArrayGetData(bytes);
		for (int x = 0; x < 42; x++) {
			printf("%.2x", d[x]);
		}
		return 1;
	}
	CBReleaseObject(input);
	CBReleaseObject(bytes);
	// Test CBTransactionOutput
	// Test deserialisation
	bytes = CBNewByteArrayOfSize(10);
	uint64_t value = rand();
	CBByteArraySetInt64(bytes, 0, value);
	CBVarIntEncode(bytes, 8, CBVarIntFromUInt64(1));
	CBByteArraySetByte(bytes, 9, CB_SCRIPT_OP_TRUE);
	CBTransactionOutput * output = CBNewTransactionOutputFromData(bytes);
	CBTransactionOutputDeserialise(output);
	if (output->value != value) {
		printf("CBTransactionOutput DESERIALISED value INCORRECT: %" PRIu64 " != %" PRIu64 "\n", output->value, value);
		return 1;
	}
	stack = CBNewEmptyScriptStack();
	if(CBScriptExecute(output->scriptObject, &stack, NULL, NULL, 0, false) != CB_SCRIPT_TRUE){
		printf("CBTransactionOutput DESERIALISED SCRIPT FAILURE\n");
		return 1;
	}
	CBReleaseObject(output);
	// Test serialisation
	output = CBNewTransactionOutput(value, scriptObj);
	CBGetMessage(output)->bytes = CBNewByteArrayOfSize(10);
	CBTransactionOutputSerialise(output);
	if (CBByteArrayCompare(bytes, CBGetMessage(output)->bytes)) {
		printf("CBTransactionOutput SERIALISATION FAILURE\n0x");
		uint8_t * d = CBByteArrayGetData(CBGetMessage(output)->bytes);
		for (int x = 0; x < 14; x++) {
			printf("%.2x", d[x]);
		}
		printf("\n!=\n0x");
		d = CBByteArrayGetData(bytes);
		for (int x = 0; x < 14; x++) {
			printf("%.2x", d[x]);
		}
		return 1;
	}
	CBReleaseObject(output);
	CBReleaseObject(bytes);
	// Test CBTransaction
	// Test deserialisation
	// Set script data
	uint8_t * scripts[6];
	scripts[0] = (uint8_t []){CB_SCRIPT_OP_4, CB_SCRIPT_OP_2, CB_SCRIPT_OP_SUB, CB_SCRIPT_OP_3, CB_SCRIPT_OP_MAX, CB_SCRIPT_OP_3, CB_SCRIPT_OP_EQUAL};
	scripts[1] = (uint8_t []){CB_SCRIPT_OP_1, CB_SCRIPT_OP_3, CB_SCRIPT_OP_7, CB_SCRIPT_OP_ROT, CB_SCRIPT_OP_SUB, CB_SCRIPT_OP_ADD, CB_SCRIPT_OP_9, CB_SCRIPT_OP_EQUAL};
	scripts[2] = (uint8_t []){CB_SCRIPT_OP_TRUE, CB_SCRIPT_OP_IF, CB_SCRIPT_OP_3, CB_SCRIPT_OP_ELSE, CB_SCRIPT_OP_2, CB_SCRIPT_OP_ENDIF, CB_SCRIPT_OP_3, CB_SCRIPT_OP_EQUAL};
	scripts[3] = (uint8_t []){CB_SCRIPT_OP_3, CB_SCRIPT_OP_DUP, CB_SCRIPT_OP_DUP, CB_SCRIPT_OP_ADD, CB_SCRIPT_OP_ADD, CB_SCRIPT_OP_9, CB_SCRIPT_OP_EQUAL};
	scripts[4] = (uint8_t []){CB_SCRIPT_OP_3, CB_SCRIPT_OP_3, CB_SCRIPT_OP_SUB, CB_SCRIPT_OP_NOT, CB_SCRIPT_OP_TRUE, CB_SCRIPT_OP_BOOLAND};
	for (uint8_t y = 0; y < 32; y++) {
		hash[y] = rand();
	}
	uint32_t randInt = rand();
	uint64_t randInt64 = rand();
	bytes = CBNewByteArrayOfSize(187);
	// Protocol version
	CBByteArraySetInt32(bytes, 0, 2);
	// Input number
	CBVarIntEncode(bytes, 4, CBVarIntFromUInt64(3));
	// First input test
	CBByteArraySetBytes(bytes, 5, hash, 32);
	CBByteArraySetInt32(bytes, 37, randInt);
	CBVarIntEncode(bytes, 41, CBVarIntFromUInt64(7));
	CBByteArraySetBytes(bytes, 42, scripts[0], 7);
	CBByteArraySetInt32(bytes, 49, randInt);
	// Second input test
	CBByteArraySetBytes(bytes, 53, hash, 32);
	CBByteArraySetInt32(bytes, 85, randInt);
	CBVarIntEncode(bytes, 89, CBVarIntFromUInt64(8));
	CBByteArraySetBytes(bytes, 90, scripts[1], 8);
	CBByteArraySetInt32(bytes, 98, randInt);
	// Third input test
	CBByteArraySetBytes(bytes, 102, hash, 32);
	CBByteArraySetInt32(bytes, 134, randInt);
	CBVarIntEncode(bytes, 138, CBVarIntFromUInt64(8));
	CBByteArraySetBytes(bytes, 139, scripts[2], 8);
	CBByteArraySetInt32(bytes, 147, randInt);
	// Output number
	CBVarIntEncode(bytes, 151, CBVarIntFromUInt64(2));
	// First output test
	CBByteArraySetInt64(bytes, 152, randInt64);
	CBVarIntEncode(bytes, 160, CBVarIntFromUInt64(7));
	CBByteArraySetBytes(bytes, 161, scripts[3], 7);
	// Second output test
	CBByteArraySetInt64(bytes, 168, randInt64);
	CBVarIntEncode(bytes, 176, CBVarIntFromUInt64(6));
	CBByteArraySetBytes(bytes, 177, scripts[4], 7);
	// Lock time
	CBByteArraySetInt32(bytes, 183, randInt);
	CBTransaction * tx = CBNewTransactionFromData(bytes);
	CBTransactionDeserialise(tx);
	if (tx->version != 2) {
		printf("TRANSACTION DESERIALISATION VERSION FAIL. %u != 2\n", tx->version);
		return 1;
	}
	if (tx->inputNum != 3) {
		printf("TRANSACTION DESERIALISATION INPUT NUM FAIL. %u != 3\n", tx->inputNum);
		return 1;
	}
	if (tx->outputNum != 2) {
		printf("TRANSACTION DESERIALISATION OUTPUT NUM FAIL. %u != 2\n", tx->outputNum);
		return 1;
	}
	if (tx->lockTime != randInt) {
		printf("TRANSACTION DESERIALISATION LOCK TIME FAIL. %u != %u\n", tx->lockTime, randInt);
		return 1;
	}
	for (uint8_t x = 0; x < 3; x++) {
		if(memcmp(hash, CBByteArrayGetData(tx->inputs[x]->prevOut.hash), 32)){
			printf("TRANSACTION CBTransactionInput %i DESERIALISED outPointerHash INCORRECT: 0x\n", x);
			for (int x = 0; x < 32; x++) {
				printf("%.2x", hash[x]);
			}
			printf(" != 0x");
			uint8_t * d = CBByteArrayGetData(tx->inputs[x]->prevOut.hash);
			for (int x = 0; x < 32; x++) {
				printf("%.2x", d[x]);
			}
			return 1;
		}
		if (tx->inputs[x]->prevOut.index != randInt) {
			printf("TRANSACTION CBTransactionInput %i DESERIALISED outPointerIndex INCORRECT: %u != %u\n", x, tx->inputs[x]->prevOut.index, randInt);
			return 1;
		}
		CBScriptStack stack = CBNewEmptyScriptStack();
		if(CBScriptExecute(tx->inputs[x]->scriptObject, &stack, NULL, NULL, 0, false) != CB_SCRIPT_TRUE){
			printf("TRANSACTION CBTransactionInput %i DESERIALISED SCRIPT FAILURE\n", x);
			return 1;
		}
		if (tx->inputs[x]->sequence != randInt) {
			printf("TRANSACTION CBTransactionInput %i DESERIALISED sequence INCORRECT: %i != %i\n", x, tx->inputs[x]->sequence, randInt);
			return 1;
		}
	}
	for (uint8_t x = 0; x < 2; x++) {
		if (tx->outputs[x]->value != randInt64) {
			printf("TRANSACTION CBTransactionOutput %i DESERIALISED value INCORRECT: %" PRIu64 " != %" PRIu64 "\n", x, tx->outputs[x]->value, randInt64);
			return 1;
		}
		stack = CBNewEmptyScriptStack();
		if(CBScriptExecute(tx->outputs[x]->scriptObject, &stack, NULL, NULL, 0, false) != CB_SCRIPT_TRUE){
			printf("TRANSACTION CBTransactionOutput %i DESERIALISED SCRIPT FAILURE\n", x);
			return 1;
		}
	}
	CBReleaseObject(tx);
	// Test serialisation
	tx = CBNewTransaction(randInt, 2);
	outPointerHash = CBNewByteArrayWithDataCopy(hash, 32);
	for (uint8_t x = 0; x < 3; x++) {
		scriptObj = CBNewScriptWithDataCopy(scripts[x], (!x)? 7 : 8);
		input = CBNewTransactionInput(scriptObj, randInt, outPointerHash, randInt);
		CBTransactionAddInput(tx, input);
		CBReleaseObject(scriptObj);
	}
	CBReleaseObject(outPointerHash);
	for (uint8_t x = 3; x < 5; x++) {
		scriptObj = CBNewScriptWithDataCopy(scripts[x], (x == 3)? 7 : 6);
		output = CBNewTransactionOutput(randInt64, scriptObj);
		CBTransactionTakeOutput(tx, output);
		CBReleaseObject(scriptObj);
	}
	CBGetMessage(tx)->bytes = CBNewByteArrayOfSize(187);
	CBTransactionSerialise(tx, true);
	if (CBByteArrayCompare(bytes, CBGetMessage(tx)->bytes)) {
		printf("CBTransaction SERIALISATION FAILURE\n0x");
		uint8_t * d = CBByteArrayGetData(CBGetMessage(tx)->bytes);
		for (int x = 0; x < 187; x++) {
			printf("%.2x", d[x]);
		}
		printf("\n!=\n0x");
		d = CBByteArrayGetData(bytes);
		for (int x = 0; x < 187; x++) {
			printf("%.2x", d[x]);
		}
		return 1;
	}
	if (CBTransactionCalculateLength(tx) != 187) {
		printf("TRANSACTION LENGTH CALC FAILURE\n");
		return 1;
	}
	CBReleaseObject(tx);
	CBReleaseObject(bytes);
	// Make keys for testing
	EC_KEY * keys[21];
	uint8_t * pubKeys[21];
	uint8_t pubSizes[21];
	unsigned int sigSizes[21];
	unsigned char testKey[279] = {0x30, 0x82, 0x01, 0x13, 0x02, 0x01, 0x01, 0x04, 0x20, 0x87, 0x5f, 0xc0, 0x45, 0x40, 0xf0, 0x2b, 0x8a, 0x6c, 0x21, 0x18, 0xb7, 0x87, 0xe3, 0xe6, 0x6b, 0x3d, 0xd4, 0x77, 0x32, 0xf5, 0x62, 0x7d, 0x12, 0x30, 0xac, 0x08, 0x61, 0x1e, 0x78, 0xd6, 0xbe, 0xa0, 0x81, 0xa5, 0x30, 0x81, 0xa2, 0x02, 0x01, 0x01, 0x30, 0x2c, 0x06, 0x07, 0x2a, 0x86, 0x48, 0xce, 0x3d, 0x01, 0x01, 0x02, 0x21, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfe, 0xff, 0xff, 0xfc, 0x2f, 0x30, 0x06, 0x04, 0x01, 0x00, 0x04, 0x01, 0x07, 0x04, 0x41, 0x04, 0x79, 0xbe, 0x66, 0x7e, 0xf9, 0xdc, 0xbb, 0xac, 0x55, 0xa0, 0x62, 0x95, 0xce, 0x87, 0x0b, 0x07, 0x02, 0x9b, 0xfc, 0xdb, 0x2d, 0xce, 0x28, 0xd9, 0x59, 0xf2, 0x81, 0x5b, 0x16, 0xf8, 0x17, 0x98, 0x48, 0x3a, 0xda, 0x77, 0x26, 0xa3, 0xc4, 0x65, 0x5d, 0xa4, 0xfb, 0xfc, 0x0e, 0x11, 0x08, 0xa8, 0xfd, 0x17, 0xb4, 0x48, 0xa6, 0x85, 0x54, 0x19, 0x9c, 0x47, 0xd0, 0x8f, 0xfb, 0x10, 0xd4, 0xb8, 0x02, 0x21, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfe, 0xba, 0xae, 0xdc, 0xe6, 0xaf, 0x48, 0xa0, 0x3b, 0xbf, 0xd2, 0x5e, 0x8c, 0xd0, 0x36, 0x41, 0x41, 0x02, 0x01, 0x01, 0xa1, 0x44, 0x03, 0x42, 0x00, 0x04, 0x86, 0x6e, 0xb3, 0xaf, 0x67, 0x49, 0x74, 0x0c, 0xc1, 0x42, 0xde, 0xe7, 0x52, 0x28, 0x20, 0x81, 0x34, 0xd8, 0xe4, 0x07, 0x94, 0x0d, 0xfe, 0x86, 0xdc, 0xb5, 0x50, 0x5a, 0xab, 0xb0, 0x4f, 0xdc, 0xe5, 0x37, 0x34, 0x63, 0x6a, 0xda, 0x8f, 0x28, 0x7b, 0x70, 0xf0, 0x7f, 0xda, 0x11, 0xc6, 0xa1, 0x29, 0x1f, 0xe6, 0x01, 0xd3, 0x13, 0xad, 0x52, 0x3a, 0x72, 0x28, 0xf3, 0x65, 0x2c, 0x71, 0x83};
	for (uint8_t x = 0; x < 21; x++){
		keys[x] = EC_KEY_new_by_curve_name(NID_secp256k1);
		if (x == 3) {
			// Test this key because it hashes into values that may be mistaken for code separators if the script interpreter is incorrect.
			const unsigned char * ptestKey = testKey;
			d2i_ECPrivateKey(&keys[x], &ptestKey, 279);
		}else{
			if(NOT EC_KEY_generate_key(keys[x])){
				printf("GENERATE KEY FAIL\n"); 
				return 1;
			}
		}
		pubSizes[x] = i2o_ECPublicKey(keys[x], NULL);
		if(NOT pubSizes[x]){
			printf("PUB KEY TO DATA ZERO\n"); 
			return 1;
		}
		pubKeys[x] = malloc(pubSizes[x]);
		uint8_t * pubKey2 = pubKeys[x];
		if(i2o_ECPublicKey(keys[x], &pubKey2) != pubSizes[x]){
			printf("PUB KEY TO DATA FAIL\n"); 
			return 1;
		}
		sigSizes[x] = ECDSA_size(keys[x]);
	}
	// Test signing transaction and OP_CHECKSIG
	tx = CBNewTransaction(0, 2);
	// Make outputs
	for (int x = 0; x < 4; x++){
		CBScript * outputsScript = CBNewScriptWithDataCopy((uint8_t []){CB_SCRIPT_OP_TRUE}, 1);
		CBTransactionTakeOutput(tx, CBNewTransactionOutput(1000, outputsScript));
		CBReleaseObject(outputsScript);
	}
	// Make inputs
	CBSha256((uint8_t []){0x56, 0x21, 0xF2}, 3, hash);
	for (int x = 0; x < 4; x++){
		bytes = CBNewByteArrayWithDataCopy(hash, 32);
		CBTransactionTakeInput(tx, CBNewUnsignedTransactionInput(0, bytes, 0));
	}
	CBReleaseObject(bytes);
	uint8_t keyHash[32];
	CBScript * outputScripts[4];
	// Make input signatures for testing different hash types
	CBSignType hashTypes[4] = {CB_SIGHASH_ALL, CB_SIGHASH_SINGLE, CB_SIGHASH_NONE, CB_SIGHASH_ALL | CB_SIGHASH_ANYONECANPAY};
	for (int x = 0; x < 4; x++) {
		outputScripts[x] = CBNewByteArrayOfSize(25);
		CBByteArraySetByte(outputScripts[x], 0, CB_SCRIPT_OP_DUP);
		CBByteArraySetByte(outputScripts[x], 1, CB_SCRIPT_OP_HASH160);
		CBByteArraySetByte(outputScripts[x], 2, 20);
		CBSha256(pubKeys[x], pubSizes[x], keyHash);
		CBRipemd160(keyHash, 32, hash);
		CBByteArraySetBytes(outputScripts[x], 3, hash, 20);
		CBByteArraySetByte(outputScripts[x], 23, CB_SCRIPT_OP_EQUALVERIFY);
		CBByteArraySetByte(outputScripts[x], 24, CB_SCRIPT_OP_CHECKSIG);
		CBTransactionGetInputHashForSignature(tx, outputScripts[x], x, hashTypes[x], hash);
		uint8_t * signature = malloc(sigSizes[x] + 1);
		ECDSA_sign(0, hash, 32, signature, &sigSizes[x], keys[x]);
		sigSizes[x]++;
		signature[sigSizes[x] - 1] = hashTypes[x];
		CBScript * inputScript = CBNewScriptOfSize(sigSizes[x] + pubSizes[x] + 2);
		CBByteArraySetByte(CBGetByteArray(inputScript), 0, sigSizes[x]);
		CBByteArraySetBytes(CBGetByteArray(inputScript), 1, signature, sigSizes[x]);
		CBByteArraySetByte(CBGetByteArray(inputScript), 1 + sigSizes[x], pubSizes[x]);
		CBByteArraySetBytes(CBGetByteArray(inputScript), 2 + sigSizes[x], pubKeys[x], pubSizes[x]);
		tx->inputs[x]->scriptObject = inputScript; // No need to release script.
		free(signature);
	}
	// Test SIGHASH_ALL
	// Execute the transaction scripts to verify correctness.
	stack = CBNewEmptyScriptStack();
	CBScriptExecute(tx->inputs[0]->scriptObject, &stack, NULL, NULL, 0, false);
	if (CBScriptExecute(outputScripts[0], &stack, CBTransactionGetInputHashForSignature, tx, 0, false) != CB_SCRIPT_TRUE) {
		printf("SIGHASH_ALL FAIL\n");
		return 1;
	}
	// Test modified first output value
	tx->outputs[0]->value++;
	stack = CBNewEmptyScriptStack();
	CBScriptExecute(tx->inputs[0]->scriptObject, &stack, NULL, NULL, 0, false);
	if (CBScriptExecute(outputScripts[0], &stack, CBTransactionGetInputHashForSignature, tx, 0, false) != CB_SCRIPT_FALSE) {
		printf("SIGHASH_ALL AND MODIFIED FIRST OUTPUT VALUE FAIL\n");
		return 1;
	}
	tx->outputs[0]->value--;
	// Test modified first output script
	CBByteArraySetByte(CBGetByteArray(tx->outputs[0]->scriptObject), 0, CBByteArrayGetByte(CBGetByteArray(tx->outputs[0]->scriptObject), 0) + 1);
	stack = CBNewEmptyScriptStack();
	CBScriptExecute(tx->inputs[0]->scriptObject, &stack, NULL, NULL, 0, false);
	if (CBScriptExecute(outputScripts[0], &stack, CBTransactionGetInputHashForSignature, tx, 0, false) != CB_SCRIPT_FALSE) {
		printf("SIGHASH_ALL AND MODIFIED FIRST OUTPUT SCRIPT FAIL\n");
		return 1;
	}
	CBByteArraySetByte(CBGetByteArray(tx->outputs[0]->scriptObject), 0, CBByteArrayGetByte(CBGetByteArray(tx->outputs[0]->scriptObject), 0) - 1);
	// Test modified second output value
	tx->outputs[1]->value++;
	stack = CBNewEmptyScriptStack();
	CBScriptExecute(tx->inputs[0]->scriptObject, &stack, NULL, NULL, 0, false);
	if (CBScriptExecute(outputScripts[0], &stack, CBTransactionGetInputHashForSignature, tx, 0, false) != CB_SCRIPT_FALSE) {
		printf("SIGHASH_ALL AND MODIFIED FIRST OUTPUT VALUE FAIL\n");
		return 1;
	}
	tx->outputs[1]->value--;
	// Test modified second output script
	CBByteArraySetByte(CBGetByteArray(tx->outputs[1]->scriptObject), 0, CBByteArrayGetByte(CBGetByteArray(tx->outputs[1]->scriptObject), 0) + 1);
	stack = CBNewEmptyScriptStack();
	CBScriptExecute(tx->inputs[0]->scriptObject, &stack, NULL, NULL, 0, false);
	if (CBScriptExecute(outputScripts[0], &stack, CBTransactionGetInputHashForSignature, tx, 0, false) != CB_SCRIPT_FALSE) {
		printf("SIGHASH_ALL AND MODIFIED SECOND OUTPUT SCRIPT FAIL\n");
		return 1;
	}
	CBByteArraySetByte(CBGetByteArray(tx->outputs[1]->scriptObject), 0, CBByteArrayGetByte(CBGetByteArray(tx->outputs[1]->scriptObject), 0) - 1);
	// Test modified lock time
	tx->lockTime++;
	stack = CBNewEmptyScriptStack();
	CBScriptExecute(tx->inputs[0]->scriptObject, &stack, NULL, NULL, 0, false);
	if (CBScriptExecute(outputScripts[0], &stack, CBTransactionGetInputHashForSignature, tx, 0, false) != CB_SCRIPT_FALSE) {
		printf("SIGHASH_ALL AND MODIFIED LOCK TIME FAIL\n");
		return 1;
	}
	tx->lockTime--;
	// Test modified input outPointerHash
	CBByteArraySetByte(tx->inputs[0]->prevOut.hash, 0, CBByteArrayGetByte(tx->inputs[0]->prevOut.hash, 0) + 1);
	stack = CBNewEmptyScriptStack();
	CBScriptExecute(tx->inputs[0]->scriptObject, &stack, NULL, NULL, 0, false);
	if (CBScriptExecute(outputScripts[0], &stack, CBTransactionGetInputHashForSignature, tx, 0, false) != CB_SCRIPT_FALSE) {
		printf("SIGHASH_ALL AND MODIFIED INPUT OUT POINTER HASH FAIL\n");
		return 1;
	}
	CBByteArraySetByte(tx->inputs[0]->prevOut.hash, 0, CBByteArrayGetByte(tx->inputs[0]->prevOut.hash, 0) - 1);
	// Test modified input outPointerIndex
	tx->inputs[0]->prevOut.index++;
	stack = CBNewEmptyScriptStack();
	CBScriptExecute(tx->inputs[0]->scriptObject, &stack, NULL, NULL, 0, false);
	if (CBScriptExecute(outputScripts[0], &stack, CBTransactionGetInputHashForSignature, tx, 0, false) != CB_SCRIPT_FALSE) {
		printf("SIGHASH_ALL AND MODIFIED INPUT OUT POINTER INDEX FAIL\n");
		return 1;
	}
	tx->inputs[0]->prevOut.index--;
	// Test modified input sequence
	tx->inputs[0]->sequence++;
	stack = CBNewEmptyScriptStack();
	CBScriptExecute(tx->inputs[0]->scriptObject, &stack, NULL, NULL, 0, false);
	if (CBScriptExecute(outputScripts[0], &stack, CBTransactionGetInputHashForSignature, tx, 0, false) != CB_SCRIPT_FALSE) {
		printf("SIGHASH_ALL AND MODIFIED INPUT SEQUENCE FAIL\n");
		return 1;
	}
	tx->inputs[0]->sequence--;
	// Test false signature
	CBByteArray * tempBytes = CBNewByteArraySubReference(CBGetByteArray(tx->inputs[0]->scriptObject), 1, sigSizes[0]);
	CBByteArrayReverseBytes(tempBytes);
	stack = CBNewEmptyScriptStack();
	CBScriptExecute(tx->inputs[0]->scriptObject, &stack, NULL, NULL, 0, false);
	if (CBScriptExecute(outputScripts[0], &stack, CBTransactionGetInputHashForSignature, tx, 0, false) != CB_SCRIPT_FALSE) {
		printf("SIGHASH_ALL AND FALSE SIGNATURE FAIL\n");
		return 1;
	}
	CBByteArrayReverseBytes(tempBytes);
	CBReleaseObject(tempBytes);
	// Test false public key
	tempBytes = CBNewByteArraySubReference(CBGetByteArray(tx->inputs[0]->scriptObject), 2 + sigSizes[0], pubSizes[0]);
	CBByteArrayReverseBytes(tempBytes);
	stack = CBNewEmptyScriptStack();
	CBScriptExecute(tx->inputs[0]->scriptObject, &stack, NULL, NULL, 0, false);
	if (CBScriptExecute(outputScripts[0], &stack, CBTransactionGetInputHashForSignature, tx, 0, false) != CB_SCRIPT_INVALID) {
		printf("SIGHASH_ALL AND FALSE PUBLIC KEY FAIL\n");
		return 1;
	}
	CBByteArrayReverseBytes(tempBytes);
	CBReleaseObject(tempBytes);
	// Test too few items on stack
	CBGetByteArray(tx->inputs[0]->scriptObject)->length = sigSizes[0] + 1;
	stack = CBNewEmptyScriptStack();
	CBScriptExecute(tx->inputs[0]->scriptObject, &stack, NULL, NULL, 0, false);
	if (CBScriptExecute(outputScripts[0], &stack, CBTransactionGetInputHashForSignature, tx, 0, false) != CB_SCRIPT_INVALID) {
		printf("OP_CHECKSIG AND TOO FEW ITEMS FAIL\n");
		return 1;
	}
	// Test SIGHASH_SINGLE
	// Execute the transaction scripts to verify correctness.
	stack = CBNewEmptyScriptStack();
	CBScriptExecute(tx->inputs[1]->scriptObject, &stack, NULL, NULL, 0, false);
	if (CBScriptExecute(outputScripts[1], &stack, CBTransactionGetInputHashForSignature, tx, 1, false) != CB_SCRIPT_TRUE) {
		printf("SIGHASH_SINGLE FAIL\n");
		return 1;
	}
	// Test modified first output value
	tx->outputs[0]->value++;
	stack = CBNewEmptyScriptStack();
	CBScriptExecute(tx->inputs[1]->scriptObject, &stack, NULL, NULL, 0, false);
	if (CBScriptExecute(outputScripts[1], &stack, CBTransactionGetInputHashForSignature, tx, 1, false) != CB_SCRIPT_TRUE) {
		printf("SIGHASH_SINGLE AND MODIFIED FIRST OUTPUT VALUE FAIL\n");
		return 1;
	}
	tx->outputs[0]->value--;
	// Test modified first output script
	CBByteArraySetByte(CBGetByteArray(tx->outputs[0]->scriptObject), 0, CBByteArrayGetByte(CBGetByteArray(tx->outputs[0]->scriptObject), 0) + 1);
	stack = CBNewEmptyScriptStack();
	CBScriptExecute(tx->inputs[1]->scriptObject, &stack, NULL, NULL, 0, false);
	if (CBScriptExecute(outputScripts[1], &stack, CBTransactionGetInputHashForSignature, tx, 1, false) != CB_SCRIPT_TRUE) {
		printf("SIGHASH_SINGLE AND MODIFIED FIRST OUTPUT SCRIPT FAIL\n");
		return 1;
	}
	CBByteArraySetByte(CBGetByteArray(tx->outputs[0]->scriptObject), 0, CBByteArrayGetByte(CBGetByteArray(tx->outputs[0]->scriptObject), 0) - 1);
	// Test modified second output value
	tx->outputs[1]->value++;
	stack = CBNewEmptyScriptStack();
	CBScriptExecute(tx->inputs[1]->scriptObject, &stack, NULL, NULL, 0, false);
	if (CBScriptExecute(outputScripts[1], &stack, CBTransactionGetInputHashForSignature, tx, 1, false) != CB_SCRIPT_FALSE) {
		printf("SIGHASH_SINGLE AND MODIFIED FIRST OUTPUT VALUE FAIL\n");
		return 1;
	}
	tx->outputs[1]->value--;
	// Test modified second output script
	CBByteArraySetByte(CBGetByteArray(tx->outputs[1]->scriptObject), 0, CBByteArrayGetByte(CBGetByteArray(tx->outputs[1]->scriptObject), 0) + 1);
	stack = CBNewEmptyScriptStack();
	CBScriptExecute(tx->inputs[1]->scriptObject, &stack, NULL, NULL, 0, false);
	if (CBScriptExecute(outputScripts[1], &stack, CBTransactionGetInputHashForSignature, tx, 1, false) != CB_SCRIPT_FALSE) {
		printf("SIGHASH_SINGLE AND MODIFIED SECOND OUTPUT SCRIPT FAIL\n");
		return 1;
	}
	CBByteArraySetByte(CBGetByteArray(tx->outputs[1]->scriptObject), 0, CBByteArrayGetByte(CBGetByteArray(tx->outputs[1]->scriptObject), 0) - 1);
	// Test modified third output value. Should be ignored
	tx->outputs[2]->value++;
	stack = CBNewEmptyScriptStack();
	CBScriptExecute(tx->inputs[1]->scriptObject, &stack, NULL, NULL, 0, false);
	if (CBScriptExecute(outputScripts[1], &stack, CBTransactionGetInputHashForSignature, tx, 1, false) != CB_SCRIPT_TRUE) {
		printf("SIGHASH_SINGLE AND MODIFIED FIRST OUTPUT VALUE FAIL\n");
		return 1;
	}
	tx->outputs[2]->value--;
	// Test modified third output script
	CBByteArraySetByte(CBGetByteArray(tx->outputs[2]->scriptObject), 0, CBByteArrayGetByte(CBGetByteArray(tx->outputs[2]->scriptObject), 0) + 1);
	stack = CBNewEmptyScriptStack();
	CBScriptExecute(tx->inputs[1]->scriptObject, &stack, NULL, NULL, 0, false);
	if (CBScriptExecute(outputScripts[1], &stack, CBTransactionGetInputHashForSignature, tx, 1, false) != CB_SCRIPT_TRUE) {
		printf("SIGHASH_SINGLE AND MODIFIED SECOND OUTPUT SCRIPT FAIL\n");
		return 1;
	}
	CBByteArraySetByte(CBGetByteArray(tx->outputs[2]->scriptObject), 0, CBByteArrayGetByte(CBGetByteArray(tx->outputs[2]->scriptObject), 0) - 1);
	// Test modified lock time
	tx->lockTime++;
	stack = CBNewEmptyScriptStack();
	CBScriptExecute(tx->inputs[1]->scriptObject, &stack, NULL, NULL, 0, false);
	if (CBScriptExecute(outputScripts[1], &stack, CBTransactionGetInputHashForSignature, tx, 1, false) != CB_SCRIPT_FALSE) {
		printf("SIGHASH_SINGLE AND MODIFIED LOCK TIME FAIL\n");
		return 1;
	}
	tx->lockTime--;
	// Test modified first input outPointerHash
	CBByteArraySetByte(tx->inputs[0]->prevOut.hash, 0, CBByteArrayGetByte(tx->inputs[0]->prevOut.hash, 0) + 1);
	stack = CBNewEmptyScriptStack();
	CBScriptExecute(tx->inputs[1]->scriptObject, &stack, NULL, NULL, 0, false);
	if (CBScriptExecute(outputScripts[1], &stack, CBTransactionGetInputHashForSignature, tx, 1, false) != CB_SCRIPT_FALSE) {
		printf("SIGHASH_SINGLE AND MODIFIED FIRST INPUT OUT POINTER HASH FAIL\n");
		return 1;
	}
	CBByteArraySetByte(tx->inputs[0]->prevOut.hash, 0, CBByteArrayGetByte(tx->inputs[0]->prevOut.hash, 0) - 1);
	// Test modified first input outPointerIndex
	tx->inputs[0]->prevOut.index++;
	stack = CBNewEmptyScriptStack();
	CBScriptExecute(tx->inputs[1]->scriptObject, &stack, NULL, NULL, 0, false);
	if (CBScriptExecute(outputScripts[1], &stack, CBTransactionGetInputHashForSignature, tx, 1, false) != CB_SCRIPT_FALSE) {
		printf("SIGHASH_SINGLE AND MODIFIED FIRST INPUT OUT POINTER INDEX FAIL\n");
		return 1;
	}
	tx->inputs[0]->prevOut.index--;
	// Test modified first input sequence
	tx->inputs[0]->sequence++;
	stack = CBNewEmptyScriptStack();
	CBScriptExecute(tx->inputs[1]->scriptObject, &stack, NULL, NULL, 0, false);
	if (CBScriptExecute(outputScripts[1], &stack, CBTransactionGetInputHashForSignature, tx, 1, false) != CB_SCRIPT_TRUE) {
		printf("SIGHASH_SINGLE AND MODIFIED FIRST INPUT SEQUENCE FAIL\n");
		return 1;
	}
	tx->inputs[0]->sequence--;
	// Test modified second input outPointerHash
	CBByteArraySetByte(tx->inputs[1]->prevOut.hash, 0, CBByteArrayGetByte(tx->inputs[1]->prevOut.hash, 0) + 1);
	stack = CBNewEmptyScriptStack();
	CBScriptExecute(tx->inputs[1]->scriptObject, &stack, NULL, NULL, 0, false);
	if (CBScriptExecute(outputScripts[1], &stack, CBTransactionGetInputHashForSignature, tx, 1, false) != CB_SCRIPT_FALSE) {
		printf("SIGHASH_SINGLE AND MODIFIED SECOND INPUT OUT POINTER HASH FAIL\n");
		return 1;
	}
	CBByteArraySetByte(tx->inputs[1]->prevOut.hash, 0, CBByteArrayGetByte(tx->inputs[1]->prevOut.hash, 0) - 1);
	// Test modified second input outPointerIndex
	tx->inputs[1]->prevOut.index++;
	stack = CBNewEmptyScriptStack();
	CBScriptExecute(tx->inputs[1]->scriptObject, &stack, NULL, NULL, 0, false);
	if (CBScriptExecute(outputScripts[1], &stack, CBTransactionGetInputHashForSignature, tx, 1, false) != CB_SCRIPT_FALSE) {
		printf("SIGHASH_SINGLE AND MODIFIED SECOND INPUT OUT POINTER INDEX FAIL\n");
		return 1;
	}
	tx->inputs[1]->prevOut.index--;
	// Test modified second input sequence
	tx->inputs[1]->sequence++;
	stack = CBNewEmptyScriptStack();
	CBScriptExecute(tx->inputs[1]->scriptObject, &stack, NULL, NULL, 0, false);
	if (CBScriptExecute(outputScripts[1], &stack, CBTransactionGetInputHashForSignature, tx, 1, false) != CB_SCRIPT_FALSE) {
		printf("SIGHASH_SINGLE AND MODIFIED SECOND INPUT SEQUENCE FAIL\n");
		return 1;
	}
	tx->inputs[1]->sequence--;
	// Test SIGHASH_NONE
	// Execute the transaction scripts to verify correctness.
	stack = CBNewEmptyScriptStack();
	CBScriptExecute(tx->inputs[2]->scriptObject, &stack, NULL, NULL, 0, false);
	if (CBScriptExecute(outputScripts[2], &stack, CBTransactionGetInputHashForSignature, tx, 2, false) != CB_SCRIPT_TRUE) {
		printf("SIGHASH_NONE FAIL\n");
		return 1;
	}
	// Test modified first output value
	tx->outputs[0]->value++;
	stack = CBNewEmptyScriptStack();
	CBScriptExecute(tx->inputs[2]->scriptObject, &stack, NULL, NULL, 0, false);
	if (CBScriptExecute(outputScripts[2], &stack, CBTransactionGetInputHashForSignature, tx, 2, false) != CB_SCRIPT_TRUE) {
		printf("SIGHASH_NONE AND MODIFIED FIRST OUTPUT VALUE FAIL\n");
		return 1;
	}
	tx->outputs[0]->value--;
	// Test modified first output script
	CBByteArraySetByte(CBGetByteArray(tx->outputs[0]->scriptObject), 0, CBByteArrayGetByte(CBGetByteArray(tx->outputs[0]->scriptObject), 0) + 1);
	stack = CBNewEmptyScriptStack();
	CBScriptExecute(tx->inputs[2]->scriptObject, &stack, NULL, NULL, 0, false);
	if (CBScriptExecute(outputScripts[2], &stack, CBTransactionGetInputHashForSignature, tx, 2, false) != CB_SCRIPT_TRUE) {
		printf("SIGHASH_NONE AND MODIFIED FIRST OUTPUT SCRIPT FAIL\n");
		return 1;
	}
	CBByteArraySetByte(CBGetByteArray(tx->outputs[0]->scriptObject), 0, CBByteArrayGetByte(CBGetByteArray(tx->outputs[0]->scriptObject), 0) - 1);
	// Test modified third output value. 
	tx->outputs[2]->value++;
	stack = CBNewEmptyScriptStack();
	CBScriptExecute(tx->inputs[2]->scriptObject, &stack, NULL, NULL, 0, false);
	if (CBScriptExecute(outputScripts[2], &stack, CBTransactionGetInputHashForSignature, tx, 2, false) != CB_SCRIPT_TRUE) {
		printf("SIGHASH_NONE AND MODIFIED THIRD OUTPUT VALUE FAIL\n");
		return 1;
	}
	tx->outputs[2]->value--;
	// Test modified third output script
	CBByteArraySetByte(CBGetByteArray(tx->outputs[2]->scriptObject), 0, CBByteArrayGetByte(CBGetByteArray(tx->outputs[2]->scriptObject), 0) + 1);
	stack = CBNewEmptyScriptStack();
	CBScriptExecute(tx->inputs[2]->scriptObject, &stack, NULL, NULL, 0, false);
	if (CBScriptExecute(outputScripts[2], &stack, CBTransactionGetInputHashForSignature, tx, 2, false) != CB_SCRIPT_TRUE) {
		printf("SIGHASH_NONE AND MODIFIED THIRD OUTPUT SCRIPT FAIL\n");
		return 1;
	}
	CBByteArraySetByte(CBGetByteArray(tx->outputs[2]->scriptObject), 0, CBByteArrayGetByte(CBGetByteArray(tx->outputs[2]->scriptObject), 0) - 1);
	// Test modified lock time
	tx->lockTime++;
	stack = CBNewEmptyScriptStack();
	CBScriptExecute(tx->inputs[2]->scriptObject, &stack, NULL, NULL, 0, false);
	if (CBScriptExecute(outputScripts[2], &stack, CBTransactionGetInputHashForSignature, tx, 2, false) != CB_SCRIPT_FALSE) {
		printf("SIGHASH_NONE AND MODIFIED LOCK TIME FAIL\n");
		return 1;
	}
	tx->lockTime--;
	// Test modified first input outPointerHash
	CBByteArraySetByte(tx->inputs[0]->prevOut.hash, 0, CBByteArrayGetByte(tx->inputs[0]->prevOut.hash, 0) + 1);
	stack = CBNewEmptyScriptStack();
	CBScriptExecute(tx->inputs[2]->scriptObject, &stack, NULL, NULL, 0, false);
	if (CBScriptExecute(outputScripts[2], &stack, CBTransactionGetInputHashForSignature, tx, 2, false) != CB_SCRIPT_FALSE) {
		printf("SIGHASH_NONE AND MODIFIED FIRST INPUT OUT POINTER HASH FAIL\n");
		return 1;
	}
	CBByteArraySetByte(tx->inputs[0]->prevOut.hash, 0, CBByteArrayGetByte(tx->inputs[0]->prevOut.hash, 0) - 1);
	// Test modified first input outPointerIndex
	tx->inputs[0]->prevOut.index++;
	stack = CBNewEmptyScriptStack();
	CBScriptExecute(tx->inputs[2]->scriptObject, &stack, NULL, NULL, 0, false);
	if (CBScriptExecute(outputScripts[2], &stack, CBTransactionGetInputHashForSignature, tx, 2, false) != CB_SCRIPT_FALSE) {
		printf("SIGHASH_NONE AND MODIFIED FIRST INPUT OUT POINTER INDEX FAIL\n");
		return 1;
	}
	tx->inputs[0]->prevOut.index--;
	// Test modified first input sequence
	tx->inputs[0]->sequence++;
	stack = CBNewEmptyScriptStack();
	CBScriptExecute(tx->inputs[2]->scriptObject, &stack, NULL, NULL, 0, false);
	if (CBScriptExecute(outputScripts[2], &stack, CBTransactionGetInputHashForSignature, tx, 2, false) != CB_SCRIPT_TRUE) {
		printf("SIGHASH_NONE AND MODIFIED FIRST INPUT SEQUENCE FAIL\n");
		return 1;
	}
	tx->inputs[0]->sequence--;
	// Test modified third input outPointerHash
	CBByteArraySetByte(tx->inputs[2]->prevOut.hash, 0, CBByteArrayGetByte(tx->inputs[2]->prevOut.hash, 0) + 1);
	stack = CBNewEmptyScriptStack();
	CBScriptExecute(tx->inputs[2]->scriptObject, &stack, NULL, NULL, 0, false);
	if (CBScriptExecute(outputScripts[2], &stack, CBTransactionGetInputHashForSignature, tx, 2, false) != CB_SCRIPT_FALSE) {
		printf("SIGHASH_NONE AND MODIFIED THIRD INPUT OUT POINTER HASH FAIL\n");
		return 1;
	}
	CBByteArraySetByte(tx->inputs[2]->prevOut.hash, 0, CBByteArrayGetByte(tx->inputs[2]->prevOut.hash, 0) - 1);
	// Test modified third input outPointerIndex
	tx->inputs[2]->prevOut.index++;
	stack = CBNewEmptyScriptStack();
	CBScriptExecute(tx->inputs[2]->scriptObject, &stack, NULL, NULL, 0, false);
	if (CBScriptExecute(outputScripts[2], &stack, CBTransactionGetInputHashForSignature, tx, 2, false) != CB_SCRIPT_FALSE) {
		printf("SIGHASH_NONE AND MODIFIED THIRD INPUT OUT POINTER INDEX FAIL\n");
		return 1;
	}
	tx->inputs[2]->prevOut.index--;
	// Test modified third input sequence
	tx->inputs[2]->sequence++;
	stack = CBNewEmptyScriptStack();
	CBScriptExecute(tx->inputs[2]->scriptObject, &stack, NULL, NULL, 0, false);
	if (CBScriptExecute(outputScripts[2], &stack, CBTransactionGetInputHashForSignature, tx, 2, false) != CB_SCRIPT_FALSE) {
		printf("SIGHASH_NONE AND MODIFIED THIRD INPUT SEQUENCE FAIL\n");
		return 1;
	}
	tx->inputs[2]->sequence--;
	// Test SIGHASH_ANYONECANPAY
	// Execute the transaction scripts to verify correctness.
	stack = CBNewEmptyScriptStack();
	CBScriptExecute(tx->inputs[3]->scriptObject, &stack, NULL, NULL, 0, false);
	if (CBScriptExecute(outputScripts[3], &stack, CBTransactionGetInputHashForSignature, tx, 3, false) != CB_SCRIPT_TRUE) {
		printf("SIGHASH_ANYONECANPAY FAIL\n");
		return 1;
	}
	// Test modified first output value
	tx->outputs[0]->value++;
	stack = CBNewEmptyScriptStack();
	CBScriptExecute(tx->inputs[3]->scriptObject, &stack, NULL, NULL, 0, false);
	if (CBScriptExecute(outputScripts[3], &stack, CBTransactionGetInputHashForSignature, tx, 3, false) != CB_SCRIPT_FALSE) {
		printf("SIGHASH_ANYONECANPAY AND MODIFIED FIRST OUTPUT VALUE FAIL\n");
		return 1;
	}else if (stack.elements[0].data){
		printf("OP_CHECKSIG DOESN'T GIVE NULL ZERO\n");
		return 1;
	}
	tx->outputs[0]->value--;
	// Test modified first output script
	CBByteArraySetByte(CBGetByteArray(tx->outputs[0]->scriptObject), 0, CBByteArrayGetByte(CBGetByteArray(tx->outputs[0]->scriptObject), 0) + 1);
	stack = CBNewEmptyScriptStack();
	CBScriptExecute(tx->inputs[3]->scriptObject, &stack, NULL, NULL, 0, false);
	if (CBScriptExecute(outputScripts[3], &stack, CBTransactionGetInputHashForSignature, tx, 3, false) != CB_SCRIPT_FALSE) {
		printf("SIGHASH_ANYONECANPAY AND MODIFIED FIRST OUTPUT SCRIPT FAIL\n");
		return 1;
	}
	CBByteArraySetByte(CBGetByteArray(tx->outputs[0]->scriptObject), 0, CBByteArrayGetByte(CBGetByteArray(tx->outputs[0]->scriptObject), 0) - 1);
	// Test modified lock time
	tx->lockTime++;
	stack = CBNewEmptyScriptStack();
	CBScriptExecute(tx->inputs[3]->scriptObject, &stack, NULL, NULL, 0, false);
	if (CBScriptExecute(outputScripts[3], &stack, CBTransactionGetInputHashForSignature, tx, 3, false) != CB_SCRIPT_FALSE) {
		printf("SIGHASH_ANYONECANPAY AND MODIFIED LOCK TIME FAIL\n");
		return 1;
	}
	tx->lockTime--;
	// Test modified first input outPointerHash
	CBByteArraySetByte(tx->inputs[0]->prevOut.hash, 0, CBByteArrayGetByte(tx->inputs[0]->prevOut.hash, 0) + 1);
	stack = CBNewEmptyScriptStack();
	CBScriptExecute(tx->inputs[3]->scriptObject, &stack, NULL, NULL, 0, false);
	if (CBScriptExecute(outputScripts[3], &stack, CBTransactionGetInputHashForSignature, tx, 3, false) != CB_SCRIPT_TRUE) {
		printf("SIGHASH_ANYONECANPAY AND MODIFIED FIRST INPUT OUT POINTER HASH FAIL\n");
		return 1;
	}
	CBByteArraySetByte(tx->inputs[0]->prevOut.hash, 0, CBByteArrayGetByte(tx->inputs[0]->prevOut.hash, 0) - 1);
	// Test modified first input outPointerIndex
	tx->inputs[0]->prevOut.index++;
	stack = CBNewEmptyScriptStack();
	CBScriptExecute(tx->inputs[3]->scriptObject, &stack, NULL, NULL, 0, false);
	if (CBScriptExecute(outputScripts[3], &stack, CBTransactionGetInputHashForSignature, tx, 3, false) != CB_SCRIPT_TRUE) {
		printf("SIGHASH_ANYONECANPAY AND MODIFIED FIRST INPUT OUT POINTER INDEX FAIL\n");
		return 1;
	}
	tx->inputs[0]->prevOut.index--;
	// Test modified first input sequence
	tx->inputs[0]->sequence++;
	stack = CBNewEmptyScriptStack();
	CBScriptExecute(tx->inputs[3]->scriptObject, &stack, NULL, NULL, 0, false);
	if (CBScriptExecute(outputScripts[3], &stack, CBTransactionGetInputHashForSignature, tx, 3, false) != CB_SCRIPT_TRUE) {
		printf("SIGHASH_ANYONECANPAY AND MODIFIED FIRST INPUT SEQUENCE FAIL\n");
		return 1;
	}
	tx->inputs[0]->sequence--;
	// Test modified forth input outPointerHash
	CBByteArraySetByte(tx->inputs[3]->prevOut.hash, 0, CBByteArrayGetByte(tx->inputs[3]->prevOut.hash, 0) + 1);
	stack = CBNewEmptyScriptStack();
	CBScriptExecute(tx->inputs[3]->scriptObject, &stack, NULL, NULL, 0, false);
	if (CBScriptExecute(outputScripts[3], &stack, CBTransactionGetInputHashForSignature, tx, 3, false) != CB_SCRIPT_FALSE) {
		printf("SIGHASH_ANYONECANPAY AND MODIFIED FORTH INPUT OUT POINTER HASH FAIL\n");
		return 1;
	}
	CBByteArraySetByte(tx->inputs[3]->prevOut.hash, 0, CBByteArrayGetByte(tx->inputs[3]->prevOut.hash, 0) - 1);
	// Test modified forth input outPointerIndex
	tx->inputs[3]->prevOut.index++;
	stack = CBNewEmptyScriptStack();
	CBScriptExecute(tx->inputs[3]->scriptObject, &stack, NULL, NULL, 0, false);
	if (CBScriptExecute(outputScripts[3], &stack, CBTransactionGetInputHashForSignature, tx, 3, false) != CB_SCRIPT_FALSE) {
		printf("SIGHASH_ANYONECANPAY AND MODIFIED FORTH INPUT OUT POINTER INDEX FAIL\n");
		return 1;
	}
	tx->inputs[3]->prevOut.index--;
	// Test modified forth input sequence
	tx->inputs[3]->sequence++;
	stack = CBNewEmptyScriptStack();
	CBScriptExecute(tx->inputs[3]->scriptObject, &stack, NULL, NULL, 0, false);
	if (CBScriptExecute(outputScripts[3], &stack, CBTransactionGetInputHashForSignature, tx, 3, false) != CB_SCRIPT_FALSE) {
		printf("SIGHASH_ANYONECANPAY AND MODIFIED FORTH INPUT SEQUENCE FAIL\n");
		return 1;
	}
	// Free objects
	for (uint8_t x = 0; x < 4; x++)
		CBReleaseObject(outputScripts[x]);
	CBReleaseObject(tx);
	// Test OP_CHECKMULTISIG
	// Lower signature sizes that were incremented above for SIGHASH
	for (int x = 0; x < 4; x++)
		sigSizes[x]--;
	// Make transaction
	tx = CBNewTransaction(0, 2);
	CBSha256((uint8_t []){0x56, 0x21, 0xF2}, 3, hash);
	bytes = CBNewByteArrayWithDataCopy(hash, 32);
	CBTransactionTakeInput(tx, CBNewUnsignedTransactionInput(0, bytes, 0));
	CBReleaseObject(bytes);
	// Make output for this transaction
	scriptObj = CBNewScriptWithDataCopy((uint8_t []){CB_SCRIPT_OP_TRUE}, 1);
	CBTransactionTakeOutput(tx, CBNewTransactionOutput(80, scriptObj));
	CBReleaseObject(scriptObj);
	// Make previous output script with OP_CHECKMULTISIG
	CBScript * outputScript = CBNewScriptWithDataCopy((uint8_t []){CB_SCRIPT_OP_CHECKMULTISIG}, 1);
	// Make signatures
	uint8_t * signatures[21];
	/* (gdb) p/x *signature->data @ signature->length
	$9 = {0x30, 0x45, 0x2, 0x20, 0x3d, 0xc1, 0xcd, 0x89, 0xe5, 0xc7, 0xcc, 0xba, 0xfe, 0xc6, 0x8a, 0x6e, 0x53, 0xc6, 0x56, 0xfb, 0x28, 0x64, 0xb7, 0x55, 0x2e, 0xf5, 0xdc, 0xd1, 0xf9, 0xb4, 0x3e, 0x1c, 0x84, 0xa4, 0xc7, 0x2c, 0x2, 0x21, 0x0, 0xe2, 0xe7, 0x34, 0x54, 0x4c, 0x22, 0x3e, 0x7c, 0x5f, 0x50, 0x9c, 0xec, 0xb2, 0x8d, 0xd4, 0x9b, 0x1a, 0x7a, 0x31, 0xe9, 0xd1, 0xfa, 0x35, 0x2d, 0x2b, 0xbf, 0xd0, 0xd1, 0x55, 0x65, 0xca, 0x32, 0x1}
	(gdb) p/x *publicKey.data @ publicKey.length
	$10 = {0x4, 0x8d, 0xdf, 0xc4, 0xeb, 0xd9, 0x8d, 0xcf, 0x48, 0x89, 0x75, 0x11, 0x55, 0x96, 0x5a, 0xd7, 0x97, 0x4e, 0x6b, 0x6e, 0xeb, 0xfe, 0x1f, 0xd, 0x6d, 0x5b, 0xf6, 0x59, 0xa3, 0xbe, 0x8f, 0x55, 0xd5, 0x6b, 0x11, 0x6d, 0x7f, 0x65, 0x65, 0xaa, 0x7e, 0x48, 0xe9, 0x6f, 0x2a, 0x63, 0xac, 0x31, 0x1f, 0x95, 0xa, 0x16, 0x54, 0x8c, 0x19, 0xab, 0x97, 0x27, 0x12, 0x48, 0xa2, 0xc6, 0xc8, 0xc1, 0x18}
	(gdb) p/x *hash @ 32
	$11 = {0xbb, 0x6a, 0xf7, 0xe1, 0x70, 0xbe, 0x98, 0x80, 0x52, 0x27, 0x45, 0xdc, 0xa, 0x94, 0xa7, 0x4c, 0x60, 0xaf, 0xa5, 0x9d, 0xb, 0x3c, 0xd6, 0x53, 0x68, 0x3b, 0x8d, 0xd2, 0xf5, 0x3b, 0x80, 0x4d} */
	CBTransactionGetInputHashForSignature(tx, outputScript, 0, CB_SIGHASH_ALL, hash);
	for (int x = 0; x < 21; x++) {
		signatures[x] = malloc(sigSizes[x] + 1);
		ECDSA_sign(0, hash, 32, signatures[x], &sigSizes[x], keys[x]);
		signatures[x][sigSizes[x]] = CB_SIGHASH_ALL;
	}
	// Test one signature, one key
	scriptObj = CBNewScriptOfSize(sigSizes[0] + pubSizes[0] + 6);
	CBByteArraySetByte(CBGetByteArray(scriptObj), 0, CB_SCRIPT_OP_0);
	CBByteArraySetByte(CBGetByteArray(scriptObj), 1, sigSizes[0] + 1);
	CBByteArraySetBytes(CBGetByteArray(scriptObj), 2, signatures[0], sigSizes[0] + 1);
	CBByteArraySetByte(CBGetByteArray(scriptObj), sigSizes[0] + 3, CB_SCRIPT_OP_1);
	CBByteArraySetByte(CBGetByteArray(scriptObj), sigSizes[0] + 4, pubSizes[0]);
	CBByteArraySetBytes(CBGetByteArray(scriptObj), sigSizes[0] + 5, pubKeys[0], pubSizes[0]);
	CBByteArraySetByte(CBGetByteArray(scriptObj), sigSizes[0] + pubSizes[0] + 5, CB_SCRIPT_OP_1);
	tx->inputs[0]->scriptObject = scriptObj;
	stack = CBNewEmptyScriptStack();
	CBScriptExecute(tx->inputs[0]->scriptObject, &stack, NULL, NULL, 0, false);
	if (CBScriptExecute(outputScript, &stack, CBTransactionGetInputHashForSignature, tx, 0, false) != CB_SCRIPT_TRUE) {
		printf("OP_CHECKMULTISIG - ONE SIG - ONE OK KEY - ZERO BAD KEYS FAIL\n");
		return 1;
	}
	CBReleaseObject(scriptObj);
	// Test two signatures, two keys
	scriptObj = CBNewScriptOfSize(sigSizes[0] + pubSizes[0] + sigSizes[1] + pubSizes[1] + 9);
	CBByteArraySetByte(CBGetByteArray(scriptObj), 0, CB_SCRIPT_OP_0);
	int cursor = 1;
	for (int x = 0; x < 2; x++) {
		CBByteArraySetByte(CBGetByteArray(scriptObj), cursor, sigSizes[x] + 1);
		cursor++;
		CBByteArraySetBytes(CBGetByteArray(scriptObj), cursor, signatures[x], sigSizes[x] + 1);
		cursor += sigSizes[x] + 1;
	}
	CBByteArraySetByte(CBGetByteArray(scriptObj), cursor, CB_SCRIPT_OP_2);
	cursor++;
	for (int x = 0; x < 2; x++) {
		CBByteArraySetByte(CBGetByteArray(scriptObj), cursor, pubSizes[x]);
		cursor++;
		CBByteArraySetBytes(CBGetByteArray(scriptObj), cursor, pubKeys[x], pubSizes[x]);
		cursor += pubSizes[x];
	}
	CBByteArraySetByte(CBGetByteArray(scriptObj), cursor, CB_SCRIPT_OP_2);
	tx->inputs[0]->scriptObject = scriptObj;
	stack = CBNewEmptyScriptStack();
	CBScriptExecute(tx->inputs[0]->scriptObject, &stack, NULL, NULL, 0, false);
	if (CBScriptExecute(outputScript, &stack, CBTransactionGetInputHashForSignature, tx, 0, false) != CB_SCRIPT_TRUE) {
		printf("OP_CHECKMULTISIG - TWO SIGS - TWO OK KEYS - ZERO BAD KEYS FAIL\n");
		return 1;
	}
	CBReleaseObject(scriptObj);
	// Test three signatures, 4 keys with first key fail
	int len = 13; // 3 + 3*2 + 4
	for (int x = 1; x < 4; x++)
		len += sigSizes[x];
	for (int x = 0; x < 4; x++)
		len += pubSizes[x];
	scriptObj = CBNewScriptOfSize(len);
	CBByteArraySetByte(CBGetByteArray(scriptObj), 0, CB_SCRIPT_OP_0);
	cursor = 1;
	for (int x = 1; x < 4; x++) {
		CBByteArraySetByte(CBGetByteArray(scriptObj), cursor, sigSizes[x] + 1);
		cursor++;
		CBByteArraySetBytes(CBGetByteArray(scriptObj), cursor, signatures[x], sigSizes[x] + 1);
		cursor += sigSizes[x] + 1;
	}
	CBByteArraySetByte(CBGetByteArray(scriptObj), cursor, CB_SCRIPT_OP_3);
	cursor++;
	for (int x = 0; x < 4; x++) {
		CBByteArraySetByte(CBGetByteArray(scriptObj), cursor, pubSizes[x]);
		cursor++;
		CBByteArraySetBytes(CBGetByteArray(scriptObj), cursor, pubKeys[x], pubSizes[x]);
		cursor += pubSizes[x];
	}
	CBByteArraySetByte(CBGetByteArray(scriptObj), cursor, CB_SCRIPT_OP_4);
	tx->inputs[0]->scriptObject = scriptObj;
	stack = CBNewEmptyScriptStack();
	CBScriptExecute(tx->inputs[0]->scriptObject, &stack, NULL, NULL, 0, false);
	if (CBScriptExecute(outputScript, &stack, CBTransactionGetInputHashForSignature, tx, 0, false) != CB_SCRIPT_TRUE) {
		printf("OP_CHECKMULTISIG - THREE SIGS - THREE OK KEYS - ONE BAD KEY FAIL\n");
		return 1;
	}
	CBReleaseObject(scriptObj);
	// Test four signatures, 6 keys with 2, 6 key fail
	len = 17; 
	for (int x = 0; x < 4; x++)
		len += sigSizes[x];
	for (int x = 0; x < 6; x++)
		len += pubSizes[x];
	scriptObj = CBNewScriptOfSize(len);
	CBByteArraySetByte(CBGetByteArray(scriptObj), 0, CB_SCRIPT_OP_0);
	cursor = 1;
	for (int x = 0; x < 4; x++) {
		CBByteArraySetByte(CBGetByteArray(scriptObj), cursor, sigSizes[x] + 1);
		cursor++;
		CBByteArraySetBytes(CBGetByteArray(scriptObj), cursor, signatures[x], sigSizes[x] + 1);
		cursor += sigSizes[x] + 1;
	}
	CBByteArraySetByte(CBGetByteArray(scriptObj), cursor, CB_SCRIPT_OP_4);
	cursor++;
	for (int x = 0; x < 6; x++) {
		int y;
		switch (x) {
			case 0:
				y = 0;
				break;
			case 1:
				y = 4;
				break;
			case 2:
				y = 1;
				break;
			case 3:
				y = 2;
				break;
			case 4:
				y = 3;
				break;
			case 5:
				y = 5;
				break;
		}
		CBByteArraySetByte(CBGetByteArray(scriptObj), cursor, pubSizes[y]);
		cursor++;
		CBByteArraySetBytes(CBGetByteArray(scriptObj), cursor, pubKeys[y], pubSizes[y]);
		cursor += pubSizes[y];
	}
	CBByteArraySetByte(CBGetByteArray(scriptObj), cursor, CB_SCRIPT_OP_6);
	tx->inputs[0]->scriptObject = scriptObj;
	stack = CBNewEmptyScriptStack();
	CBScriptExecute(tx->inputs[0]->scriptObject, &stack, NULL, NULL, 0, false);
	if (CBScriptExecute(outputScript, &stack, CBTransactionGetInputHashForSignature, tx, 0, false) != CB_SCRIPT_TRUE) {
		printf("OP_CHECKMULTISIG - FOUR SIGS - FOUR OK KEYS - TWO BAD KEYS FAIL\n");
		return 1;
	}
	CBReleaseObject(scriptObj);
	// Test five signatures, 10 keys with first 5 fail
	len = 23;
	for (int x = 5; x < 10; x++)
		len += sigSizes[x];
	for (int x = 0; x < 10; x++)
		len += pubSizes[x];
	scriptObj = CBNewScriptOfSize(len);
	CBByteArraySetByte(CBGetByteArray(scriptObj), 0, CB_SCRIPT_OP_0);
	cursor = 1;
	for (int x = 5; x < 10; x++) {
		CBByteArraySetByte(CBGetByteArray(scriptObj), cursor, sigSizes[x] + 1);
		cursor++;
		CBByteArraySetBytes(CBGetByteArray(scriptObj), cursor, signatures[x], sigSizes[x] + 1);
		cursor += sigSizes[x] + 1;
	}
	CBByteArraySetByte(CBGetByteArray(scriptObj), cursor, CB_SCRIPT_OP_5);
	cursor++;
	for (int x = 0; x < 10; x++) {
		CBByteArraySetByte(CBGetByteArray(scriptObj), cursor, pubSizes[x]);
		cursor++;
		CBByteArraySetBytes(CBGetByteArray(scriptObj), cursor, pubKeys[x], pubSizes[x]);
		cursor += pubSizes[x];
	}
	CBByteArraySetByte(CBGetByteArray(scriptObj), cursor, CB_SCRIPT_OP_10);
	tx->inputs[0]->scriptObject = scriptObj;
	stack = CBNewEmptyScriptStack();
	CBScriptExecute(tx->inputs[0]->scriptObject, &stack, NULL, NULL, 0, false);
	if (CBScriptExecute(outputScript, &stack, CBTransactionGetInputHashForSignature, tx, 0, false) != CB_SCRIPT_TRUE) {
		printf("OP_CHECKMULTISIG - FIVE SIGS - FIVE OK KEYS - FIVE BAD KEYS FAIL\n");
		return 1;
	}
	CBReleaseObject(scriptObj);
	// Test five signature, 10 keys with evens fail
	len = 23;
	for (int x = 0; x < 10; x += 2)
		len += sigSizes[x];
	for (int x = 0; x < 10; x++)
		len += pubSizes[x];
	scriptObj = CBNewScriptOfSize(len);
	CBByteArraySetByte(CBGetByteArray(scriptObj), 0, CB_SCRIPT_OP_0);
	cursor = 1;
	for (int x = 0; x < 10; x += 2) {
		CBByteArraySetByte(CBGetByteArray(scriptObj), cursor, sigSizes[x] + 1);
		cursor++;
		CBByteArraySetBytes(CBGetByteArray(scriptObj), cursor, signatures[x], sigSizes[x] + 1);
		cursor += sigSizes[x] + 1;
	}
	CBByteArraySetByte(CBGetByteArray(scriptObj), cursor, CB_SCRIPT_OP_5);
	cursor++;
	for (int x = 0; x < 10; x++) {
		CBByteArraySetByte(CBGetByteArray(scriptObj), cursor, pubSizes[x]);
		cursor++;
		CBByteArraySetBytes(CBGetByteArray(scriptObj), cursor, pubKeys[x], pubSizes[x]);
		cursor += pubSizes[x];
	}
	CBByteArraySetByte(CBGetByteArray(scriptObj), cursor, CB_SCRIPT_OP_10);
	tx->inputs[0]->scriptObject = scriptObj;
	stack = CBNewEmptyScriptStack();
	CBScriptExecute(tx->inputs[0]->scriptObject, &stack, NULL, NULL, 0, false);
	if (CBScriptExecute(outputScript, &stack, CBTransactionGetInputHashForSignature, tx, 0, false) != CB_SCRIPT_TRUE) {
		printf("OP_CHECKMULTISIG - FIVE SIGS - FIVE OK KEYS ODDS - FIVE BAD KEYS EVENS FAIL\n");
		return 1;
	}
	CBReleaseObject(scriptObj);
	// Test 20 signatures
	len = 65;
	for (int x = 0; x < 20; x++)
		len += sigSizes[x];
	for (int x = 0; x < 20; x++)
		len += pubSizes[x];
	scriptObj = CBNewScriptOfSize(len);
	CBByteArraySetByte(CBGetByteArray(scriptObj), 0, CB_SCRIPT_OP_0);
	cursor = 1;
	for (int x = 0; x < 20; x++) {
		CBByteArraySetByte(CBGetByteArray(scriptObj), cursor, sigSizes[x] + 1);
		cursor++;
		CBByteArraySetBytes(CBGetByteArray(scriptObj), cursor, signatures[x], sigSizes[x] + 1);
		cursor += sigSizes[x] + 1;
	}
	CBByteArraySetByte(CBGetByteArray(scriptObj), cursor, 0x01);
	cursor++;
	CBByteArraySetByte(CBGetByteArray(scriptObj), cursor, 0x14);
	cursor++;
	for (int x = 0; x < 20; x++) {
		CBByteArraySetByte(CBGetByteArray(scriptObj), cursor, pubSizes[x]);
		cursor++;
		CBByteArraySetBytes(CBGetByteArray(scriptObj), cursor, pubKeys[x], pubSizes[x]);
		cursor += pubSizes[x];
	}
	CBByteArraySetByte(CBGetByteArray(scriptObj), cursor, 0x01);
	cursor++;
	CBByteArraySetByte(CBGetByteArray(scriptObj), cursor, 0x14);
	tx->inputs[0]->scriptObject = scriptObj;
	stack = CBNewEmptyScriptStack();
	CBScriptExecute(tx->inputs[0]->scriptObject, &stack, NULL, NULL, 0, false);
	if (CBScriptExecute(outputScript, &stack, CBTransactionGetInputHashForSignature, tx, 0, false) != CB_SCRIPT_TRUE) {
		printf("OP_CHECKMULTISIG - TWENTY SIGS - TWENTY OK KEYS - ZERO BAD KEYS FAIL\n");
		return 1;
	}
	CBReleaseObject(scriptObj);
	// Test 21 signatures failure.
	len = 68;
	for (int x = 0; x < 21; x++)
		len += sigSizes[x];
	for (int x = 0; x < 21; x++)
		len += pubSizes[x];
	scriptObj = CBNewScriptOfSize(len);
	CBByteArraySetByte(CBGetByteArray(scriptObj), 0, CB_SCRIPT_OP_0);
	cursor = 1;
	for (int x = 0; x < 21; x++) {
		CBByteArraySetByte(CBGetByteArray(scriptObj), cursor, sigSizes[x] + 1);
		cursor++;
		CBByteArraySetBytes(CBGetByteArray(scriptObj), cursor, signatures[x], sigSizes[x] + 1);
		cursor += sigSizes[x] + 1;
	}
	CBByteArraySetByte(CBGetByteArray(scriptObj), cursor, 0x01);
	cursor++;
	CBByteArraySetByte(CBGetByteArray(scriptObj), cursor, 0x15);
	cursor++;
	for (int x = 0; x < 21; x++) {
		CBByteArraySetByte(CBGetByteArray(scriptObj), cursor, pubSizes[x]);
		cursor++;
		CBByteArraySetBytes(CBGetByteArray(scriptObj), cursor, pubKeys[x], pubSizes[x]);
		cursor += pubSizes[x];
	}
	CBByteArraySetByte(CBGetByteArray(scriptObj), cursor, 0x01);
	cursor++;
	CBByteArraySetByte(CBGetByteArray(scriptObj), cursor, 0x15);
	tx->inputs[0]->scriptObject = scriptObj;
	stack = CBNewEmptyScriptStack();
	CBScriptExecute(tx->inputs[0]->scriptObject, &stack, NULL, NULL, 0, false);
	if (CBScriptExecute(outputScript, &stack, CBTransactionGetInputHashForSignature, tx, 0, false) != CB_SCRIPT_INVALID) {
		printf("OP_CHECKMULTISIG - TWENTY ONE SIGS - TWENTY ONE OK KEYS - ZERO BAD KEYS FAIL\n");
		return 1;
	}
	CBReleaseObject(scriptObj);
	// Test one signature, one fail key failure
	scriptObj = CBNewScriptOfSize(sigSizes[0] + pubSizes[1] + 6);
	CBByteArraySetByte(CBGetByteArray(scriptObj), 0, CB_SCRIPT_OP_0);
	CBByteArraySetByte(CBGetByteArray(scriptObj), 1, sigSizes[0] + 1);
	CBByteArraySetBytes(CBGetByteArray(scriptObj), 2, signatures[0], sigSizes[0] + 1);
	CBByteArraySetByte(CBGetByteArray(scriptObj), sigSizes[0] + 3, CB_SCRIPT_OP_1);
	CBByteArraySetByte(CBGetByteArray(scriptObj), sigSizes[0] + 4, pubSizes[1]);
	CBByteArraySetBytes(CBGetByteArray(scriptObj), sigSizes[0] + 5, pubKeys[1], pubSizes[1]);
	CBByteArraySetByte(CBGetByteArray(scriptObj), sigSizes[0] + pubSizes[1] + 5, CB_SCRIPT_OP_1);
	tx->inputs[0]->scriptObject = scriptObj;
	stack = CBNewEmptyScriptStack();
	CBScriptExecute(tx->inputs[0]->scriptObject, &stack, NULL, NULL, 0, false);
	if (CBScriptExecute(outputScript, &stack, CBTransactionGetInputHashForSignature, tx, 0, false) != CB_SCRIPT_FALSE) {
		printf("OP_CHECKMULTISIG - ONE SIG - ZERO OK KEYS - ONE BAD KEY FAIL\n");
		return 1;
	}
	CBReleaseObject(scriptObj);
	// Test five signatures, 8 keys with 4 failing failure
	len = 21;
	for (int x = 4; x < 9; x++)
		len += sigSizes[x];
	for (int x = 0; x < 8; x++)
		len += pubSizes[x];
	scriptObj = CBNewScriptOfSize(len);
	CBByteArraySetByte(CBGetByteArray(scriptObj), 0, CB_SCRIPT_OP_0);
	cursor = 1;
	for (int x = 4; x < 9; x++) {
		CBByteArraySetByte(CBGetByteArray(scriptObj), cursor, sigSizes[x] + 1);
		cursor++;
		CBByteArraySetBytes(CBGetByteArray(scriptObj), cursor, signatures[x], sigSizes[x] + 1);
		cursor += sigSizes[x] + 1;
	}
	CBByteArraySetByte(CBGetByteArray(scriptObj), cursor, CB_SCRIPT_OP_5);
	cursor++;
	for (int x = 0; x < 8; x++) {
		CBByteArraySetByte(CBGetByteArray(scriptObj), cursor, pubSizes[x]);
		cursor++;
		CBByteArraySetBytes(CBGetByteArray(scriptObj), cursor, pubKeys[x], pubSizes[x]);
		cursor += pubSizes[x];
	}
	CBByteArraySetByte(CBGetByteArray(scriptObj), cursor, CB_SCRIPT_OP_8);
	tx->inputs[0]->scriptObject = scriptObj;
	stack = CBNewEmptyScriptStack();
	CBScriptExecute(tx->inputs[0]->scriptObject, &stack, NULL, NULL, 0, false);
	if (CBScriptExecute(outputScript, &stack, CBTransactionGetInputHashForSignature, tx, 0, false) != CB_SCRIPT_FALSE) {
		printf("OP_CHECKMULTISIG - FIVE SIGS - FOUR OK KEYS - FOUR BAD KEYS FAIL\n");
		return 1;
	}else if (stack.elements[0].data){
		printf("OP_CHECKMULTISIG DOESN'T GIVE NULL ZERO\n");
		return 1;
	}
	CBReleaseObject(scriptObj);
	// Test low key number
	scriptObj = CBNewScriptOfSize(sigSizes[0] + pubSizes[0] + sigSizes[1] + pubSizes[1] + 9);
	CBByteArraySetByte(CBGetByteArray(scriptObj), 0, CB_SCRIPT_OP_0);
	cursor = 1;
	for (int x = 0; x < 2; x++) {
		CBByteArraySetByte(CBGetByteArray(scriptObj), cursor, sigSizes[x] + 1);
		cursor++;
		CBByteArraySetBytes(CBGetByteArray(scriptObj), cursor, signatures[x], sigSizes[x] + 1);
		cursor += sigSizes[x] + 1;
	}
	CBByteArraySetByte(CBGetByteArray(scriptObj), cursor, CB_SCRIPT_OP_2);
	cursor++;
	for (int x = 0; x < 2; x++) {
		CBByteArraySetByte(CBGetByteArray(scriptObj), cursor, pubSizes[x]);
		cursor++;
		CBByteArraySetBytes(CBGetByteArray(scriptObj), cursor, pubKeys[x], pubSizes[x]);
		cursor += pubSizes[x];
	}
	CBByteArraySetByte(CBGetByteArray(scriptObj), cursor, CB_SCRIPT_OP_1);
	tx->inputs[0]->scriptObject = scriptObj;
	stack = CBNewEmptyScriptStack();
	CBScriptExecute(tx->inputs[0]->scriptObject, &stack, NULL, NULL, 0, false);
	if (CBScriptExecute(outputScript, &stack, CBTransactionGetInputHashForSignature, tx, 0, false) != CB_SCRIPT_INVALID) {
		printf("OP_CHECKMULTISIG - LOW KEY NUMBER FAIL\n");
		return 1;
	}
	CBReleaseObject(scriptObj);
	// Test low sig number
	scriptObj = CBNewScriptOfSize(sigSizes[0] + pubSizes[0] + sigSizes[1] + pubSizes[1] + 9);
	CBByteArraySetByte(CBGetByteArray(scriptObj), 0, CB_SCRIPT_OP_0);
	cursor = 1;
	for (int x = 0; x < 2; x++) {
		CBByteArraySetByte(CBGetByteArray(scriptObj), cursor, sigSizes[x] + 1);
		cursor++;
		CBByteArraySetBytes(CBGetByteArray(scriptObj), cursor, signatures[x], sigSizes[x] + 1);
		cursor += sigSizes[x] + 1;
	}
	CBByteArraySetByte(CBGetByteArray(scriptObj), cursor, CB_SCRIPT_OP_1);
	cursor++;
	for (int x = 0; x < 2; x++) {
		CBByteArraySetByte(CBGetByteArray(scriptObj), cursor, pubSizes[x]);
		cursor++;
		CBByteArraySetBytes(CBGetByteArray(scriptObj), cursor, pubKeys[x], pubSizes[x]);
		cursor += pubSizes[x];
	}
	CBByteArraySetByte(CBGetByteArray(scriptObj), cursor, CB_SCRIPT_OP_2);
	tx->inputs[0]->scriptObject = scriptObj;
	stack = CBNewEmptyScriptStack();
	CBScriptExecute(tx->inputs[0]->scriptObject, &stack, NULL, NULL, 0, false);
	if (CBScriptExecute(outputScript, &stack, CBTransactionGetInputHashForSignature, tx, 0, false) != CB_SCRIPT_TRUE) {
		printf("OP_CHECKMULTISIG - LOW SIG NUMBER FAIL\n");
		return 1;
	}
	CBReleaseObject(scriptObj);
	// Test high key number
	scriptObj = CBNewScriptOfSize(sigSizes[0] + pubSizes[0] + sigSizes[1] + pubSizes[1] + 9);
	CBByteArraySetByte(CBGetByteArray(scriptObj), 0, CB_SCRIPT_OP_0);
	cursor = 1;
	for (int x = 0; x < 2; x++) {
		CBByteArraySetByte(CBGetByteArray(scriptObj), cursor, sigSizes[x] + 1);
		cursor++;
		CBByteArraySetBytes(CBGetByteArray(scriptObj), cursor, signatures[x], sigSizes[x] + 1);
		cursor += sigSizes[x] + 1;
	}
	CBByteArraySetByte(CBGetByteArray(scriptObj), cursor, CB_SCRIPT_OP_2);
	cursor++;
	for (int x = 0; x < 2; x++) {
		CBByteArraySetByte(CBGetByteArray(scriptObj), cursor, pubSizes[x]);
		cursor++;
		CBByteArraySetBytes(CBGetByteArray(scriptObj), cursor, pubKeys[x], pubSizes[x]);
		cursor += pubSizes[x];
	}
	CBByteArraySetByte(CBGetByteArray(scriptObj), cursor, CB_SCRIPT_OP_3);
	tx->inputs[0]->scriptObject = scriptObj;
	stack = CBNewEmptyScriptStack();
	CBScriptExecute(tx->inputs[0]->scriptObject, &stack, NULL, NULL, 0, false);
	if (CBScriptExecute(outputScript, &stack, CBTransactionGetInputHashForSignature, tx, 0, false) != CB_SCRIPT_INVALID) {
		printf("OP_CHECKMULTISIG - HIGH KEY NUMBER FAIL\n");
		return 1;
	}
	CBReleaseObject(scriptObj);
	// Test high sig number
	scriptObj = CBNewScriptOfSize(sigSizes[0] + pubSizes[0] + sigSizes[1] + pubSizes[1] + 9);
	CBByteArraySetByte(CBGetByteArray(scriptObj), 0, CB_SCRIPT_OP_0);
	cursor = 1;
	for (int x = 0; x < 2; x++) {
		CBByteArraySetByte(CBGetByteArray(scriptObj), cursor, sigSizes[x] + 1);
		cursor++;
		CBByteArraySetBytes(CBGetByteArray(scriptObj), cursor, signatures[x], sigSizes[x] + 1);
		cursor += sigSizes[x] + 1;
	}
	CBByteArraySetByte(CBGetByteArray(scriptObj), cursor, CB_SCRIPT_OP_3);
	cursor++;
	for (int x = 0; x < 2; x++) {
		CBByteArraySetByte(CBGetByteArray(scriptObj), cursor, pubSizes[x]);
		cursor++;
		CBByteArraySetBytes(CBGetByteArray(scriptObj), cursor, pubKeys[x], pubSizes[x]);
		cursor += pubSizes[x];
	}
	CBByteArraySetByte(CBGetByteArray(scriptObj), cursor, CB_SCRIPT_OP_2);
	tx->inputs[0]->scriptObject = scriptObj;
	stack = CBNewEmptyScriptStack();
	CBScriptExecute(tx->inputs[0]->scriptObject, &stack, NULL, NULL, 0, false);
	if (CBScriptExecute(outputScript, &stack, CBTransactionGetInputHashForSignature, tx, 0, false) != CB_SCRIPT_INVALID) {
		printf("OP_CHECKMULTISIG - HIGH SIG NUMBER FAIL\n");
		return 1;
	}
	CBReleaseObject(scriptObj);
	// Test hash calculation
	bytes = CBNewByteArrayWithDataCopy((uint8_t []){0x1, 0x0, 0x0, 0x0, 0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0xff, 0xff, 0xff, 0xff, 0x7, 0x4, 0xff, 0xff, 0x0, 0x1d, 0x1, 0x4, 0xff, 0xff, 0xff, 0xff, 0x1, 0x0, 0xf2, 0x5, 0x2a, 0x1, 0x0, 0x0, 0x0, 0x43, 0x41, 0x4, 0x96, 0xb5, 0x38, 0xe8, 0x53, 0x51, 0x9c, 0x72, 0x6a, 0x2c, 0x91, 0xe6, 0x1e, 0xc1, 0x16, 0x0, 0xae, 0x13, 0x90, 0x81, 0x3a, 0x62, 0x7c, 0x66, 0xfb, 0x8b, 0xe7, 0x94, 0x7b, 0xe6, 0x3c, 0x52, 0xda, 0x75, 0x89, 0x37, 0x95, 0x15, 0xd4, 0xe0, 0xa6, 0x4, 0xf8, 0x14, 0x17, 0x81, 0xe6, 0x22, 0x94, 0x72, 0x11, 0x66, 0xbf, 0x62, 0x1e, 0x73, 0xa8, 0x2c, 0xbf, 0x23, 0x42, 0xc8, 0x58, 0xee, 0xac, 0x0, 0x0, 0x0, 0x0}, 134);
	tx = CBNewTransactionFromData(bytes);
	if (memcmp(CBTransactionGetHash(tx), (uint8_t []){0x98, 0x20, 0x51, 0xfD, 0x1E, 0x4B, 0xA7, 0x44, 0xBB, 0xBE, 0x68, 0x0E, 0x1F, 0xEE, 0x14, 0x67, 0x7B, 0xA1, 0xA3, 0xC3, 0x54, 0x0B, 0xF7, 0xB1, 0xCD, 0xB6, 0x06, 0xE8, 0x57, 0x23, 0x3E, 0x0E}, 32)) {
		printf("TRANSACTION GET HASH FAIL\n");
		return 1;
	}
	// Test hash change after reserailisation
	CBTransactionDeserialise(tx);
	tx->lockTime = 50;
	CBTransactionSerialise(tx, false);
	if (NOT memcmp(CBTransactionGetHash(tx), (uint8_t []){0x98, 0x20, 0x51, 0xfD, 0x1E, 0x4B, 0xA7, 0x44, 0xBB, 0xBE, 0x68, 0x0E, 0x1F, 0xEE, 0x14, 0x67, 0x7B, 0xA1, 0xA3, 0xC3, 0x54, 0x0B, 0xF7, 0xB1, 0xCD, 0xB6, 0x06, 0xE8, 0x57, 0x23, 0x3E, 0x0E}, 32)) {
		printf("TRANSACTION CHANGE HASH FAIL\n");
		return 1;
	}
	// Test zero inputs serialisation
	tx->inputNum = 0;
	if (CBTransactionSerialise(tx, false)) {
		printf("TRANSACTION ZERO INPUTS SERAILISE FAIL\n");
		return 1;
	}
	tx->inputNum = 1;
	// Test zero outputs serialisation
	tx->outputNum = 0;
	if (CBTransactionSerialise(tx, false)) {
		printf("TRANSACTION ZERO OUTPUTS SERAILISE FAIL\n");
		return 1;
	}
	CBReleaseObject(bytes);
	CBReleaseObject(tx);
	// Test zero inputs deserialisation
	bytes = CBNewByteArrayWithDataCopy((uint8_t [86]){0x1, 0x0, 0x0, 0x0, 0x0, 0x1, 0x0, 0xf2, 0x5, 0x2a, 0x1, 0x0, 0x0, 0x0, 0x43, 0x41, 0x4, 0x96, 0xb5, 0x38, 0xe8, 0x53, 0x51, 0x9c, 0x72, 0x6a, 0x2c, 0x91, 0xe6, 0x1e, 0xc1, 0x16, 0x0, 0xae, 0x13, 0x90, 0x81, 0x3a, 0x62, 0x7c, 0x66, 0xfb, 0x8b, 0xe7, 0x94, 0x7b, 0xe6, 0x3c, 0x52, 0xda, 0x75, 0x89, 0x37, 0x95, 0x15, 0xd4, 0xe0, 0xa6, 0x4, 0xf8, 0x14, 0x17, 0x81, 0xe6, 0x22, 0x94, 0x72, 0x11, 0x66, 0xbf, 0x62, 0x1e, 0x73, 0xa8, 0x2c, 0xbf, 0x23, 0x42, 0xc8, 0x58, 0xee, 0xac, 0x0, 0x0, 0x0, 0x0}, 86);
	tx = CBNewTransactionFromData(bytes);
	if (CBTransactionDeserialise(tx)) {
		printf("TRANSACTION ZERO INPUTS DESERAILISE FAIL\n");
		return 1;
	}
	CBReleaseObject(bytes);
	CBReleaseObject(tx);
	// Test zero outputs deserialisation
	bytes = CBNewByteArrayWithDataCopy((uint8_t [58]){0x1, 0x0, 0x0, 0x0, 0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0xff, 0xff, 0xff, 0xff, 0x7, 0x4, 0xff, 0xff, 0x0, 0x1d, 0x1, 0x4, 0xff, 0xff, 0xff, 0xff, 0x0, 0x0, 0x0, 0x0, 0x0}, 58);
	tx = CBNewTransactionFromData(bytes);
	if (CBTransactionDeserialise(tx)) {
		printf("TRANSACTION ZERO INPUTS DESERAILISE FAIL\n");
		return 1;
	}
	CBReleaseObject(bytes);
	CBReleaseObject(tx);
	return 0;
}
