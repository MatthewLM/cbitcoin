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

void err(CBError a,char * format,...){
	va_list argptr;
    va_start(argptr, format);
    vfprintf(stderr, format, argptr);
    va_end(argptr);
	printf("\n");
}

u_int8_t * CBSha160(u_int8_t * data,u_int16_t len){
	u_int8_t * hash = malloc(SHA_DIGEST_LENGTH);
    SHA1(data, len, hash);
	return hash;
}
u_int8_t * CBSha256(u_int8_t * data,u_int16_t len){
	u_int8_t * hash = malloc(SHA256_DIGEST_LENGTH);
	SHA256(data, len, hash);
	return hash;
}
u_int8_t * CBRipemd160(u_int8_t * data,u_int16_t len){
	u_int8_t * hash = malloc(RIPEMD160_DIGEST_LENGTH);
    RIPEMD160(data, len, hash);
	return hash;
}
bool CBEcdsaVerify(u_int8_t * signature,u_int8_t sigLen,u_int8_t * hash,const u_int8_t * pubKey,u_int8_t keyLen){
	EC_KEY * key = EC_KEY_new_by_curve_name(NID_secp256k1);
	o2i_ECPublicKey(&key, &pubKey, keyLen);
	int res = ECDSA_verify(0, hash, 32, signature, sigLen, key);
	EC_KEY_free(key);
	return res == 1;
}

int main(){
	unsigned int s = (unsigned int)time(NULL);
	s = 1337544566;
	printf("Session = %ui\n",s);
	srand(s);
	CBNetworkParameters * net = CBNewNetworkParameters();
	net->networkCode = 0;
	CBEvents events;
	events.onErrorReceived = err;
	// Test CBTransactionInput
	// Test deserialisation
	u_int8_t * hash2 = CBSha256((u_int8_t *)"hello", 5);
	CBByteArray * bytes = CBNewByteArrayOfSize(42, &events);
	CBByteArraySetBytes(bytes, 0, hash2, 32);
	CBByteArraySetBytes(bytes, 32, (u_int8_t []){0x05,0xFA,0x4B,0x51}, 4);
	CBVarIntEncode(bytes, 36, CBVarIntFromUInt64(1));
	CBByteArraySetByte(bytes, 37, CB_SCRIPT_OP_TRUE);
	u_int32_t sequence = rand();
	CBByteArraySetUInt32(bytes, 38, sequence);
	CBTransactionInput * input = CBNewTransactionInputFromData(net, bytes, NULL, &events);
	CBTransactionInputDeserialise(input);
	if(memcmp(hash2,CBByteArrayGetData(input->outPointerHash),32)){
		printf("CBTransactionInput DESERIALISED outPointerHash INCORRECT: 0x");
		for (int x = 0; x < 32; x++) {
			printf("%.2x",hash2[x]);
		}
		printf(" != 0x");
		u_int8_t * d = CBByteArrayGetData(input->outPointerHash);
		for (int x = 0; x < 32; x++) {
			printf("%.2x",d[x]);
		}
		return 1;
	}
	if (input->outPointerIndex != 0x514BFA05) {
		printf("CBTransactionInput DESERIALISED outPointerIndex INCORRECT: %i != 0x514BFA05\n",input->outPointerIndex);
		return 1;
	}
	CBScriptStack stack = CBNewEmptyScriptStack();
	if(!CBScriptExecute(input->scriptObject, &stack, NULL, NULL, 0)){
		printf("CBTransactionInput DESERIALISED SCRIPT FAILURE\n");
		return 1;
	}
	if (input->sequence != sequence) {
		printf("CBTransactionInput DESERIALISED sequence INCORRECT: %i != %i\n",input->sequence,sequence);
		return 1;
	}
	CBReleaseObject(&input);
	// Test serialisation
	CBScript * scriptObj = CBNewScriptWithDataCopy(net, (u_int8_t []){CB_SCRIPT_OP_TRUE},1 , &events);
	CBByteArray * outPointerHash = CBNewByteArrayWithDataCopy(hash2, 32, &events);
	input = CBNewTransactionInput(net, NULL, scriptObj, outPointerHash, 0x514BFA05, &events);
	input->sequence = sequence;
	CBGetMessage(input)->bytes = CBNewByteArrayOfSize(42, &events);
	CBTransactionInputSerialise(input);
	if (!CBByteArrayEquals(bytes, CBGetMessage(input)->bytes)) {
		printf("CBTransactionInput SERIALISATION FAILURE\n0x");
		u_int8_t * d = CBByteArrayGetData(CBGetMessage(input)->bytes);
		for (int x = 0; x < 42; x++) {
			printf("%.2x",d[x]);
		}
		printf("\n!=\n0x");
		d = CBByteArrayGetData(bytes);
		for (int x = 0; x < 42; x++) {
			printf("%.2x",d[x]);
		}
		return 1;
	}
	CBReleaseObject(&input);
	CBReleaseObject(&bytes);
	// Test CBTransactionOutput
	// Test deserialisation
	bytes = CBNewByteArrayOfSize(10, &events);
	u_int64_t value = rand();
	CBByteArraySetUInt64(bytes, 0, value);
	CBVarIntEncode(bytes, 8, CBVarIntFromUInt64(1));
	CBByteArraySetByte(bytes, 9, CB_SCRIPT_OP_TRUE);
	CBTransactionOutput * output = CBNewTransactionOutputFromData(net, bytes, NULL, &events);
	CBTransactionOutputDeserialise(output);
	if (output->value != value) {
		printf("CBTransactionOutput DESERIALISED value INCORRECT: %llu != %llu\n",output->value,value);
		return 1;
	}
	stack = CBNewEmptyScriptStack();
	if(!CBScriptExecute(output->scriptObject, &stack, NULL, NULL, 0)){
		printf("CBTransactionOutput DESERIALISED SCRIPT FAILURE\n");
		return 1;
	}
	CBReleaseObject(&output);
	// Test serialisation
	output = CBNewTransactionOutput(net, NULL, value, scriptObj, &events);
	CBGetMessage(output)->bytes = CBNewByteArrayOfSize(10, &events);
	CBTransactionOutputSerialise(output);
	if (!CBByteArrayEquals(bytes, CBGetMessage(output)->bytes)) {
		printf("CBTransactionOutput SERIALISATION FAILURE\n0x");
		u_int8_t * d = CBByteArrayGetData(CBGetMessage(output)->bytes);
		for (int x = 0; x < 14; x++) {
			printf("%.2x",d[x]);
		}
		printf("\n!=\n0x");
		d = CBByteArrayGetData(bytes);
		for (int x = 0; x < 14; x++) {
			printf("%.2x",d[x]);
		}
		return 1;
	}
	CBReleaseObject(&output);
	CBReleaseObject(&bytes);
	// Test CBTransaction
	// Test deserialisation
	// Set script data
	u_int8_t * scripts[6];
	scripts[0] = (u_int8_t []){CB_SCRIPT_OP_4,CB_SCRIPT_OP_2,CB_SCRIPT_OP_SUB,CB_SCRIPT_OP_3,CB_SCRIPT_OP_MAX,CB_SCRIPT_OP_3,CB_SCRIPT_OP_EQUAL};
	scripts[1] = (u_int8_t []){CB_SCRIPT_OP_1,CB_SCRIPT_OP_3,CB_SCRIPT_OP_7,CB_SCRIPT_OP_ROT,CB_SCRIPT_OP_SUB,CB_SCRIPT_OP_ADD,CB_SCRIPT_OP_9,CB_SCRIPT_OP_EQUAL};
	scripts[2] = (u_int8_t []){CB_SCRIPT_OP_TRUE,CB_SCRIPT_OP_IF,CB_SCRIPT_OP_3,CB_SCRIPT_OP_ELSE,CB_SCRIPT_OP_2,CB_SCRIPT_OP_ENDIF,CB_SCRIPT_OP_3,CB_SCRIPT_OP_EQUAL};
	scripts[3] = (u_int8_t []){CB_SCRIPT_OP_3,CB_SCRIPT_OP_DUP,CB_SCRIPT_OP_DUP,CB_SCRIPT_OP_ADD,CB_SCRIPT_OP_ADD,CB_SCRIPT_OP_9,CB_SCRIPT_OP_EQUAL};
	scripts[4] = (u_int8_t []){CB_SCRIPT_OP_3,CB_SCRIPT_OP_3,CB_SCRIPT_OP_SUB,CB_SCRIPT_OP_NOT,CB_SCRIPT_OP_TRUE,CB_SCRIPT_OP_BOOLAND};
	for (u_int8_t y = 0; y < 32; y++) {
		hash2[y] = rand();
	}
	u_int32_t randInt = rand();
	u_int64_t randInt64 = rand();
	bytes = CBNewByteArrayOfSize(187, &events);
	// Protocol version
	CBByteArraySetUInt32(bytes, 0, CB_PROTOCOL_VERSION);
	// Input number
	CBVarIntEncode(bytes, 4, CBVarIntFromUInt64(3));
	// First input test
	CBByteArraySetBytes(bytes, 5, hash2, 32);
	CBByteArraySetUInt32(bytes, 37,randInt);
	CBVarIntEncode(bytes, 41, CBVarIntFromUInt64(7));
	CBByteArraySetBytes(bytes, 42, scripts[0], 7);
	CBByteArraySetUInt32(bytes, 49,randInt);
	// Second input test
	CBByteArraySetBytes(bytes, 53, hash2, 32);
	CBByteArraySetUInt32(bytes, 85,randInt);
	CBVarIntEncode(bytes, 89, CBVarIntFromUInt64(8));
	CBByteArraySetBytes(bytes, 90, scripts[1], 8);
	CBByteArraySetUInt32(bytes, 98,randInt);
	// Third input test
	CBByteArraySetBytes(bytes, 102, hash2, 32);
	CBByteArraySetUInt32(bytes, 134,randInt);
	CBVarIntEncode(bytes, 138, CBVarIntFromUInt64(8));
	CBByteArraySetBytes(bytes, 139, scripts[2], 8);
	CBByteArraySetUInt32(bytes, 147,randInt);
	// Output number
	CBVarIntEncode(bytes, 151, CBVarIntFromUInt64(2));
	// First output test
	CBByteArraySetUInt64(bytes, 152, randInt64);
	CBVarIntEncode(bytes, 160, CBVarIntFromUInt64(7));
	CBByteArraySetBytes(bytes, 161, scripts[3], 7);
	// Second output test
	CBByteArraySetUInt64(bytes, 168, randInt64);
	CBVarIntEncode(bytes, 176, CBVarIntFromUInt64(6));
	CBByteArraySetBytes(bytes, 177, scripts[4], 7);
	// Lock time
	CBByteArraySetUInt32(bytes, 183, randInt);
	CBTransaction * tx = CBNewTransactionFromData(net, bytes, &events);
	CBTransactionDeserialise(tx);
	if (tx->protocolVersion != CB_PROTOCOL_VERSION) {
		printf("TRANSACTION DESERIALISATION VERSION FAIL. %u != %u\n",tx->protocolVersion,CB_PROTOCOL_VERSION);
		return 1;
	}
	if (tx->inputNum != 3) {
		printf("TRANSACTION DESERIALISATION INPUT NUM FAIL. %u != 3\n",tx->inputNum);
		return 1;
	}
	if (tx->outputNum != 2) {
		printf("TRANSACTION DESERIALISATION OUTPUT NUM FAIL. %u != 2\n",tx->outputNum);
		return 1;
	}
	if (tx->lockTime != randInt) {
		printf("TRANSACTION DESERIALISATION LOCK TIME FAIL. %u != %u\n",tx->lockTime,randInt);
		return 1;
	}
	for (u_int8_t x = 0; x < 3; x++) {
		if(memcmp(hash2,CBByteArrayGetData(tx->inputs[x]->outPointerHash),32)){
			printf("TRANSACTION CBTransactionInput %i DESERIALISED outPointerHash INCORRECT: 0x\n",x);
			for (int x = 0; x < 32; x++) {
				printf("%.2x",hash2[x]);
			}
			printf(" != 0x");
			u_int8_t * d = CBByteArrayGetData(tx->inputs[x]->outPointerHash);
			for (int x = 0; x < 32; x++) {
				printf("%.2x",d[x]);
			}
			return 1;
		}
		if (tx->inputs[x]->outPointerIndex != randInt) {
			printf("TRANSACTION CBTransactionInput %i DESERIALISED outPointerIndex INCORRECT: %u != %u\n",x,tx->inputs[x]->outPointerIndex,randInt);
			return 1;
		}
		CBScriptStack stack = CBNewEmptyScriptStack();
		if(!CBScriptExecute(tx->inputs[x]->scriptObject, &stack, NULL, NULL, 0)){
			printf("TRANSACTION CBTransactionInput %i DESERIALISED SCRIPT FAILURE\n",x);
			return 1;
		}
		if (tx->inputs[x]->sequence != randInt) {
			printf("TRANSACTION CBTransactionInput %i DESERIALISED sequence INCORRECT: %i != %i\n",x,tx->inputs[x]->sequence,randInt);
			return 1;
		}
	}
	for (u_int8_t x = 0; x < 2; x++) {
		if (tx->outputs[x]->value != randInt64) {
			printf("TRANSACTION CBTransactionOutput %i DESERIALISED value INCORRECT: %llu != %llu\n",x,tx->outputs[x]->value,randInt64);
			return 1;
		}
		stack = CBNewEmptyScriptStack();
		if(!CBScriptExecute(tx->outputs[x]->scriptObject, &stack, NULL, NULL, 0)){
			printf("TRANSACTION CBTransactionOutput %i DESERIALISED SCRIPT FAILURE\n",x);
			return 1;
		}
	}
	CBReleaseObject(&tx);
	// Test serialisation
	tx = CBNewTransaction(net, randInt, CB_PROTOCOL_VERSION, &events);
	outPointerHash = CBNewByteArrayWithDataCopy(hash2, 32, &events);
	for (u_int8_t x = 0; x < 3; x++) {
		scriptObj = CBNewScriptWithDataCopy(net, scripts[x], (!x)? 7 : 8, &events);
		input = CBNewTransactionInput(net, NULL, scriptObj, outPointerHash, randInt, &events);
		input->sequence = randInt;
		CBTransactionAddInput(tx, input);
		CBReleaseObject(&scriptObj);
	}
	CBReleaseObject(&outPointerHash);
	for (u_int8_t x = 3; x < 5; x++) {
		scriptObj = CBNewScriptWithDataCopy(net, scripts[x], (x == 3)? 7 : 6, &events);
		output = CBNewTransactionOutput(net, NULL, randInt64, scriptObj, &events);
		CBTransactionTakeOutput(tx, output);
		CBReleaseObject(&scriptObj);
	}
	CBGetMessage(tx)->bytes = CBNewByteArrayOfSize(187, &events);
	CBTransactionSerialise(tx);
	if (!CBByteArrayEquals(bytes, CBGetMessage(tx)->bytes)) {
		printf("CBTransaction SERIALISATION FAILURE\n0x");
		u_int8_t * d = CBByteArrayGetData(CBGetMessage(tx)->bytes);
		for (int x = 0; x < 187; x++) {
			printf("%.2x",d[x]);
		}
		printf("\n!=\n0x");
		d = CBByteArrayGetData(bytes);
		for (int x = 0; x < 187; x++) {
			printf("%.2x",d[x]);
		}
		return 1;
	}
	CBReleaseObject(&tx);
	CBReleaseObject(&bytes);
	free(hash2);
	// Make keys for testing
	EC_KEY * keys[21];
	u_int8_t * pubKeys[21];
	u_int8_t pubSizes[21];
	unsigned int sigSizes[21];
	unsigned char testKey[279] = {0x30, 0x82, 0x01, 0x13, 0x02, 0x01, 0x01, 0x04, 0x20, 0x87, 0x5f, 0xc0, 0x45, 0x40, 0xf0, 0x2b, 0x8a, 0x6c, 0x21, 0x18, 0xb7, 0x87, 0xe3, 0xe6, 0x6b, 0x3d, 0xd4, 0x77, 0x32, 0xf5, 0x62, 0x7d, 0x12, 0x30, 0xac, 0x08, 0x61, 0x1e, 0x78, 0xd6, 0xbe, 0xa0, 0x81, 0xa5, 0x30, 0x81, 0xa2, 0x02, 0x01, 0x01, 0x30, 0x2c, 0x06, 0x07, 0x2a, 0x86, 0x48, 0xce, 0x3d, 0x01, 0x01, 0x02, 0x21, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfe, 0xff, 0xff, 0xfc, 0x2f, 0x30, 0x06, 0x04, 0x01, 0x00, 0x04, 0x01, 0x07, 0x04, 0x41, 0x04, 0x79, 0xbe, 0x66, 0x7e, 0xf9, 0xdc, 0xbb, 0xac, 0x55, 0xa0, 0x62, 0x95, 0xce, 0x87, 0x0b, 0x07, 0x02, 0x9b, 0xfc, 0xdb, 0x2d, 0xce, 0x28, 0xd9, 0x59, 0xf2, 0x81, 0x5b, 0x16, 0xf8, 0x17, 0x98, 0x48, 0x3a, 0xda, 0x77, 0x26, 0xa3, 0xc4, 0x65, 0x5d, 0xa4, 0xfb, 0xfc, 0x0e, 0x11, 0x08, 0xa8, 0xfd, 0x17, 0xb4, 0x48, 0xa6, 0x85, 0x54, 0x19, 0x9c, 0x47, 0xd0, 0x8f, 0xfb, 0x10, 0xd4, 0xb8, 0x02, 0x21, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfe, 0xba, 0xae, 0xdc, 0xe6, 0xaf, 0x48, 0xa0, 0x3b, 0xbf, 0xd2, 0x5e, 0x8c, 0xd0, 0x36, 0x41, 0x41, 0x02, 0x01, 0x01, 0xa1, 0x44, 0x03, 0x42, 0x00, 0x04, 0x86, 0x6e, 0xb3, 0xaf, 0x67, 0x49, 0x74, 0x0c, 0xc1, 0x42, 0xde, 0xe7, 0x52, 0x28, 0x20, 0x81, 0x34, 0xd8, 0xe4, 0x07, 0x94, 0x0d, 0xfe, 0x86, 0xdc, 0xb5, 0x50, 0x5a, 0xab, 0xb0, 0x4f, 0xdc, 0xe5, 0x37, 0x34, 0x63, 0x6a, 0xda, 0x8f, 0x28, 0x7b, 0x70, 0xf0, 0x7f, 0xda, 0x11, 0xc6, 0xa1, 0x29, 0x1f, 0xe6, 0x01, 0xd3, 0x13, 0xad, 0x52, 0x3a, 0x72, 0x28, 0xf3, 0x65, 0x2c, 0x71, 0x83};
	for (u_int8_t x = 0; x < 21; x++){
		keys[x] = EC_KEY_new_by_curve_name(NID_secp256k1);
		if (x == 3) {
			// Test this key because it hashes into values that may be mistaken for code separators if the script interpreter is incorrect.
			const unsigned char * ptestKey = testKey;
			d2i_ECPrivateKey(&keys[x], &ptestKey, 279);
		}else{
			if(!EC_KEY_generate_key(keys[x])){
				printf("GENERATE KEY FAIL\n"); 
				return 1;
			}
		}
		pubSizes[x] = i2o_ECPublicKey(keys[x], NULL);
		if(!pubSizes[x]){
			printf("PUB KEY TO DATA ZERO\n"); 
			return 1;
		}
		pubKeys[x] = malloc(pubSizes[x]);
		u_int8_t * pubKey2 = pubKeys[x];
		if(i2o_ECPublicKey(keys[x], &pubKey2) != pubSizes[x]){
			printf("PUB KEY TO DATA FAIL\n"); 
			return 1;
		}
		sigSizes[x] = ECDSA_size(keys[x]);
	}
	// Test signing transaction and OP_CHECKSIG
	tx = CBNewTransaction(net, 0, CB_PROTOCOL_VERSION, &events);
	// Make outputs
	for (int x = 0; x < 4; x++){
		CBScript * outputsScript = CBNewScriptWithDataCopy(net, (u_int8_t []){CB_SCRIPT_OP_TRUE}, 1, &events);
		CBTransactionTakeOutput(tx, CBNewTransactionOutput(net, tx, 1000, outputsScript, &events));
		CBReleaseObject(&outputsScript);
	}
	// Make inputs
	u_int8_t * hash = CBSha256((u_int8_t []){0x56,0x21,0xF2}, 3);
	for (int x = 0; x < 4; x++){
		bytes = CBNewByteArrayWithDataCopy(hash, 32, &events);
		CBTransactionTakeInput(tx, CBNewTransactionInput(net, tx, NULL, bytes, 0, &events));
	}
	free(hash);
	CBReleaseObject(&bytes);
	// Make standard output script.
	u_int8_t * subScript = malloc(25);
	subScript[0] = CB_SCRIPT_OP_DUP;
	subScript[1] = CB_SCRIPT_OP_HASH160;
	subScript[2] = 20;
	u_int8_t * keyHash = CBSha256(pubKeys[0], pubSizes[0]);
	hash = CBRipemd160(keyHash, 32);
	free(keyHash);
	memmove(subScript + 3, hash, 20);
	free(hash);
	subScript[23] = CB_SCRIPT_OP_EQUALVERIFY;
	subScript[24] = CB_SCRIPT_OP_CHECKSIG;
	CBByteArray * subScriptByteArray = CBNewByteArrayWithData(subScript, 25, &events);
	// Make input signatures for testing different hash types
	CBSignType hashTypes[4] = {CB_SIGHASH_ALL,CB_SIGHASH_SINGLE,CB_SIGHASH_NONE,CB_SIGHASH_ALL | CB_SIGHASH_ANYONECANPAY};
	for (int x = 0; x < 4; x++) {
		hash = CBTransactionGetInputHashForSignature(tx, subScriptByteArray, x, hashTypes[x]);
		u_int8_t * signature = malloc(sigSizes[x] + 1);
		ECDSA_sign(0, hash, 32, signature, &sigSizes[0], keys[0]);
		sigSizes[0]++;
		signature[sigSizes[0] - 1] = hashTypes[x];
		CBScript * inputScript = CBNewScriptOfSize(net, sigSizes[0] + pubSizes[0] + 2, &events);
		CBByteArraySetByte(CBGetByteArray(inputScript), 0, sigSizes[0]);
		CBByteArraySetBytes(CBGetByteArray(inputScript), 1, signature, sigSizes[0]);
		CBByteArraySetByte(CBGetByteArray(inputScript), 1 + sigSizes[0], pubSizes[0]);
		CBByteArraySetBytes(CBGetByteArray(inputScript), 2 + sigSizes[0], pubKeys[0], pubSizes[0]);
		tx->inputs[x]->scriptObject = inputScript; // No need to release script.
		free(signature);
	}
	CBScript * outputScript = CBNewScriptFromReference(net, subScriptByteArray, 0, 0, &events);
	// Test SIGHASH_ALL
	// Execute the transaction scripts to verify correctness.
	stack = CBNewEmptyScriptStack();
	CBScriptExecute(tx->inputs[0]->scriptObject, &stack, NULL, NULL, 0);
	if (!CBScriptExecute(outputScript, &stack, CBTransactionGetInputHashForSignature, tx, 0)) {
		printf("SIGHASH_ALL FAIL\n");
		return 1;
	}
	// Test modified first output value
	tx->outputs[0]->value++;
	stack = CBNewEmptyScriptStack();
	CBScriptExecute(tx->inputs[0]->scriptObject, &stack, NULL, NULL, 0);
	if (CBScriptExecute(outputScript, &stack, CBTransactionGetInputHashForSignature, tx, 0)) {
		printf("SIGHASH_ALL AND MODIFIED FIRST OUTPUT VALUE FAIL\n");
		return 1;
	}
	tx->outputs[0]->value--;
	// Test modified first output script
	CBByteArraySetByte(CBGetByteArray(tx->outputs[0]->scriptObject), 0, CBByteArrayGetByte(CBGetByteArray(tx->outputs[0]->scriptObject), 0) + 1);
	stack = CBNewEmptyScriptStack();
	CBScriptExecute(tx->inputs[0]->scriptObject, &stack, NULL, NULL, 0);
	if (CBScriptExecute(outputScript, &stack, CBTransactionGetInputHashForSignature, tx, 0)) {
		printf("SIGHASH_ALL AND MODIFIED FIRST OUTPUT SCRIPT FAIL\n");
		return 1;
	}
	CBByteArraySetByte(CBGetByteArray(tx->outputs[0]->scriptObject), 0, CBByteArrayGetByte(CBGetByteArray(tx->outputs[0]->scriptObject), 0) - 1);
	// Test modified second output value
	tx->outputs[1]->value++;
	stack = CBNewEmptyScriptStack();
	CBScriptExecute(tx->inputs[0]->scriptObject, &stack, NULL, NULL, 0);
	if (CBScriptExecute(outputScript, &stack, CBTransactionGetInputHashForSignature, tx, 0)) {
		printf("SIGHASH_ALL AND MODIFIED FIRST OUTPUT VALUE FAIL\n");
		return 1;
	}
	tx->outputs[1]->value--;
	// Test modified second output script
	CBByteArraySetByte(CBGetByteArray(tx->outputs[1]->scriptObject), 0, CBByteArrayGetByte(CBGetByteArray(tx->outputs[1]->scriptObject), 0) + 1);
	stack = CBNewEmptyScriptStack();
	CBScriptExecute(tx->inputs[0]->scriptObject, &stack, NULL, NULL, 0);
	if (CBScriptExecute(outputScript, &stack, CBTransactionGetInputHashForSignature, tx, 0)) {
		printf("SIGHASH_ALL AND MODIFIED SECOND OUTPUT SCRIPT FAIL\n");
		return 1;
	}
	CBByteArraySetByte(CBGetByteArray(tx->outputs[1]->scriptObject), 0, CBByteArrayGetByte(CBGetByteArray(tx->outputs[1]->scriptObject), 0) - 1);
	// Test modified lock time
	tx->lockTime++;
	stack = CBNewEmptyScriptStack();
	CBScriptExecute(tx->inputs[0]->scriptObject, &stack, NULL, NULL, 0);
	if (CBScriptExecute(outputScript, &stack, CBTransactionGetInputHashForSignature, tx, 0)) {
		printf("SIGHASH_ALL AND MODIFIED LOCK TIME FAIL\n");
		return 1;
	}
	tx->lockTime--;
	// Test modified input outPointerHash
	CBByteArraySetByte(tx->inputs[0]->outPointerHash, 0, CBByteArrayGetByte(tx->inputs[0]->outPointerHash, 0) + 1);
	stack = CBNewEmptyScriptStack();
	CBScriptExecute(tx->inputs[0]->scriptObject, &stack, NULL, NULL, 0);
	if (CBScriptExecute(outputScript, &stack, CBTransactionGetInputHashForSignature, tx, 0)) {
		printf("SIGHASH_ALL AND MODIFIED INPUT OUT POINTER HASH FAIL\n");
		return 1;
	}
	CBByteArraySetByte(tx->inputs[0]->outPointerHash, 0, CBByteArrayGetByte(tx->inputs[0]->outPointerHash, 0) - 1);
	// Test modified input outPointerIndex
	tx->inputs[0]->outPointerIndex++;
	stack = CBNewEmptyScriptStack();
	CBScriptExecute(tx->inputs[0]->scriptObject, &stack, NULL, NULL, 0);
	if (CBScriptExecute(outputScript, &stack, CBTransactionGetInputHashForSignature, tx, 0)) {
		printf("SIGHASH_ALL AND MODIFIED INPUT OUT POINTER INDEX FAIL\n");
		return 1;
	}
	tx->inputs[0]->outPointerIndex--;
	// Test modified input sequence
	tx->inputs[0]->sequence++;
	stack = CBNewEmptyScriptStack();
	CBScriptExecute(tx->inputs[0]->scriptObject, &stack, NULL, NULL, 0);
	if (CBScriptExecute(outputScript, &stack, CBTransactionGetInputHashForSignature, tx, 0)) {
		printf("SIGHASH_ALL AND MODIFIED INPUT SEQUENCE FAIL\n");
		return 1;
	}
	tx->inputs[0]->sequence--;
	// Test false signature
	CBByteArray * tempBytes = CBNewByteArraySubReference(CBGetByteArray(tx->inputs[0]->scriptObject), 1, sigSizes[0]);
	CBByteArrayReverseBytes(tempBytes);
	stack = CBNewEmptyScriptStack();
	CBScriptExecute(tx->inputs[0]->scriptObject, &stack, NULL, NULL, 0);
	if (CBScriptExecute(outputScript, &stack, CBTransactionGetInputHashForSignature, tx, 0)) {
		printf("SIGHASH_ALL AND FALSE SIGNATURE FAIL\n");
		return 1;
	}
	CBByteArrayReverseBytes(tempBytes);
	CBReleaseObject(&tempBytes);
	// Test false public key
	tempBytes = CBNewByteArraySubReference(CBGetByteArray(tx->inputs[0]->scriptObject), 2 + sigSizes[0], pubSizes[0]);
	CBByteArrayReverseBytes(tempBytes);
	stack = CBNewEmptyScriptStack();
	CBScriptExecute(tx->inputs[0]->scriptObject, &stack, NULL, NULL, 0);
	if (CBScriptExecute(outputScript, &stack, CBTransactionGetInputHashForSignature, tx, 0)) {
		printf("SIGHASH_ALL AND FALSE PUBLIC KEY FAIL\n");
		return 1;
	}
	CBByteArrayReverseBytes(tempBytes);
	CBReleaseObject(&tempBytes);
	// Test too few items on stack
	CBGetByteArray(tx->inputs[0]->scriptObject)->length = sigSizes[0] + 1;
	stack = CBNewEmptyScriptStack();
	CBScriptExecute(tx->inputs[0]->scriptObject, &stack, NULL, NULL, 0);
	if (CBScriptExecute(outputScript, &stack, CBTransactionGetInputHashForSignature, tx, 0)) {
		printf("OP_CHECKSIG AND TOO FEW ITEMS FAIL\n");
		return 1;
	}
	// Test SIGHASH_SINGLE
	// Execute the transaction scripts to verify correctness.
	stack = CBNewEmptyScriptStack();
	CBScriptExecute(tx->inputs[1]->scriptObject, &stack, NULL, NULL, 0);
	if (!CBScriptExecute(outputScript, &stack, CBTransactionGetInputHashForSignature, tx, 1)) {
		printf("SIGHASH_SINGLE FAIL\n");
		return 1;
	}
	// Test modified first output value
	tx->outputs[0]->value++;
	stack = CBNewEmptyScriptStack();
	CBScriptExecute(tx->inputs[1]->scriptObject, &stack, NULL, NULL, 0);
	if (!CBScriptExecute(outputScript, &stack, CBTransactionGetInputHashForSignature, tx, 1)) {
		printf("SIGHASH_SINGLE AND MODIFIED FIRST OUTPUT VALUE FAIL\n");
		return 1;
	}
	tx->outputs[0]->value--;
	// Test modified first output script
	CBByteArraySetByte(CBGetByteArray(tx->outputs[0]->scriptObject), 0, CBByteArrayGetByte(CBGetByteArray(tx->outputs[0]->scriptObject), 0) + 1);
	stack = CBNewEmptyScriptStack();
	CBScriptExecute(tx->inputs[1]->scriptObject, &stack, NULL, NULL, 0);
	if (!CBScriptExecute(outputScript, &stack, CBTransactionGetInputHashForSignature, tx, 1)) {
		printf("SIGHASH_SINGLE AND MODIFIED FIRST OUTPUT SCRIPT FAIL\n");
		return 1;
	}
	CBByteArraySetByte(CBGetByteArray(tx->outputs[0]->scriptObject), 0, CBByteArrayGetByte(CBGetByteArray(tx->outputs[0]->scriptObject), 0) - 1);
	// Test modified second output value
	tx->outputs[1]->value++;
	stack = CBNewEmptyScriptStack();
	CBScriptExecute(tx->inputs[1]->scriptObject, &stack, NULL, NULL, 0);
	if (CBScriptExecute(outputScript, &stack, CBTransactionGetInputHashForSignature, tx, 1)) {
		printf("SIGHASH_SINGLE AND MODIFIED FIRST OUTPUT VALUE FAIL\n");
		return 1;
	}
	tx->outputs[1]->value--;
	// Test modified second output script
	CBByteArraySetByte(CBGetByteArray(tx->outputs[1]->scriptObject), 0, CBByteArrayGetByte(CBGetByteArray(tx->outputs[1]->scriptObject), 0) + 1);
	stack = CBNewEmptyScriptStack();
	CBScriptExecute(tx->inputs[1]->scriptObject, &stack, NULL, NULL, 0);
	if (CBScriptExecute(outputScript, &stack, CBTransactionGetInputHashForSignature, tx, 1)) {
		printf("SIGHASH_SINGLE AND MODIFIED SECOND OUTPUT SCRIPT FAIL\n");
		return 1;
	}
	CBByteArraySetByte(CBGetByteArray(tx->outputs[1]->scriptObject), 0, CBByteArrayGetByte(CBGetByteArray(tx->outputs[1]->scriptObject), 0) - 1);
	// Test modified third output value. Should be ignored
	tx->outputs[2]->value++;
	stack = CBNewEmptyScriptStack();
	CBScriptExecute(tx->inputs[1]->scriptObject, &stack, NULL, NULL, 0);
	if (!CBScriptExecute(outputScript, &stack, CBTransactionGetInputHashForSignature, tx, 1)) {
		printf("SIGHASH_SINGLE AND MODIFIED FIRST OUTPUT VALUE FAIL\n");
		return 1;
	}
	tx->outputs[2]->value--;
	// Test modified third output script
	CBByteArraySetByte(CBGetByteArray(tx->outputs[2]->scriptObject), 0, CBByteArrayGetByte(CBGetByteArray(tx->outputs[2]->scriptObject), 0) + 1);
	stack = CBNewEmptyScriptStack();
	CBScriptExecute(tx->inputs[1]->scriptObject, &stack, NULL, NULL, 0);
	if (!CBScriptExecute(outputScript, &stack, CBTransactionGetInputHashForSignature, tx, 1)) {
		printf("SIGHASH_SINGLE AND MODIFIED SECOND OUTPUT SCRIPT FAIL\n");
		return 1;
	}
	CBByteArraySetByte(CBGetByteArray(tx->outputs[2]->scriptObject), 0, CBByteArrayGetByte(CBGetByteArray(tx->outputs[2]->scriptObject), 0) - 1);
	// Test modified lock time
	tx->lockTime++;
	stack = CBNewEmptyScriptStack();
	CBScriptExecute(tx->inputs[1]->scriptObject, &stack, NULL, NULL, 0);
	if (CBScriptExecute(outputScript, &stack, CBTransactionGetInputHashForSignature, tx, 1)) {
		printf("SIGHASH_SINGLE AND MODIFIED LOCK TIME FAIL\n");
		return 1;
	}
	tx->lockTime--;
	// Test modified first input outPointerHash
	CBByteArraySetByte(tx->inputs[0]->outPointerHash, 0, CBByteArrayGetByte(tx->inputs[0]->outPointerHash, 0) + 1);
	stack = CBNewEmptyScriptStack();
	CBScriptExecute(tx->inputs[1]->scriptObject, &stack, NULL, NULL, 0);
	if (CBScriptExecute(outputScript, &stack, CBTransactionGetInputHashForSignature, tx, 1)) {
		printf("SIGHASH_SINGLE AND MODIFIED FIRST INPUT OUT POINTER HASH FAIL\n");
		return 1;
	}
	CBByteArraySetByte(tx->inputs[0]->outPointerHash, 0, CBByteArrayGetByte(tx->inputs[0]->outPointerHash, 0) - 1);
	// Test modified first input outPointerIndex
	tx->inputs[0]->outPointerIndex++;
	stack = CBNewEmptyScriptStack();
	CBScriptExecute(tx->inputs[1]->scriptObject, &stack, NULL, NULL, 0);
	if (CBScriptExecute(outputScript, &stack, CBTransactionGetInputHashForSignature, tx, 1)) {
		printf("SIGHASH_SINGLE AND MODIFIED FIRST INPUT OUT POINTER INDEX FAIL\n");
		return 1;
	}
	tx->inputs[0]->outPointerIndex--;
	// Test modified first input sequence
	tx->inputs[0]->sequence++;
	stack = CBNewEmptyScriptStack();
	CBScriptExecute(tx->inputs[1]->scriptObject, &stack, NULL, NULL, 0);
	if (!CBScriptExecute(outputScript, &stack, CBTransactionGetInputHashForSignature, tx, 1)) {
		printf("SIGHASH_SINGLE AND MODIFIED FIRST INPUT SEQUENCE FAIL\n");
		return 1;
	}
	tx->inputs[0]->sequence--;
	// Test modified second input outPointerHash
	CBByteArraySetByte(tx->inputs[1]->outPointerHash, 0, CBByteArrayGetByte(tx->inputs[1]->outPointerHash, 0) + 1);
	stack = CBNewEmptyScriptStack();
	CBScriptExecute(tx->inputs[1]->scriptObject, &stack, NULL, NULL, 0);
	if (CBScriptExecute(outputScript, &stack, CBTransactionGetInputHashForSignature, tx, 1)) {
		printf("SIGHASH_SINGLE AND MODIFIED SECOND INPUT OUT POINTER HASH FAIL\n");
		return 1;
	}
	CBByteArraySetByte(tx->inputs[1]->outPointerHash, 0, CBByteArrayGetByte(tx->inputs[1]->outPointerHash, 0) - 1);
	// Test modified second input outPointerIndex
	tx->inputs[1]->outPointerIndex++;
	stack = CBNewEmptyScriptStack();
	CBScriptExecute(tx->inputs[1]->scriptObject, &stack, NULL, NULL, 0);
	if (CBScriptExecute(outputScript, &stack, CBTransactionGetInputHashForSignature, tx, 1)) {
		printf("SIGHASH_SINGLE AND MODIFIED SECOND INPUT OUT POINTER INDEX FAIL\n");
		return 1;
	}
	tx->inputs[1]->outPointerIndex--;
	// Test modified second input sequence
	tx->inputs[1]->sequence++;
	stack = CBNewEmptyScriptStack();
	CBScriptExecute(tx->inputs[1]->scriptObject, &stack, NULL, NULL, 0);
	if (CBScriptExecute(outputScript, &stack, CBTransactionGetInputHashForSignature, tx, 1)) {
		printf("SIGHASH_SINGLE AND MODIFIED SECOND INPUT SEQUENCE FAIL\n");
		return 1;
	}
	tx->inputs[1]->sequence--;
	// Test SIGHASH_NONE
	// Execute the transaction scripts to verify correctness.
	stack = CBNewEmptyScriptStack();
	CBScriptExecute(tx->inputs[2]->scriptObject, &stack, NULL, NULL, 0);
	if (!CBScriptExecute(outputScript, &stack, CBTransactionGetInputHashForSignature, tx, 2)) {
		printf("SIGHASH_NONE FAIL\n");
		return 1;
	}
	// Test modified first output value
	tx->outputs[0]->value++;
	stack = CBNewEmptyScriptStack();
	CBScriptExecute(tx->inputs[2]->scriptObject, &stack, NULL, NULL, 0);
	if (!CBScriptExecute(outputScript, &stack, CBTransactionGetInputHashForSignature, tx, 2)) {
		printf("SIGHASH_NONE AND MODIFIED FIRST OUTPUT VALUE FAIL\n");
		return 1;
	}
	tx->outputs[0]->value--;
	// Test modified first output script
	CBByteArraySetByte(CBGetByteArray(tx->outputs[0]->scriptObject), 0, CBByteArrayGetByte(CBGetByteArray(tx->outputs[0]->scriptObject), 0) + 1);
	stack = CBNewEmptyScriptStack();
	CBScriptExecute(tx->inputs[2]->scriptObject, &stack, NULL, NULL, 0);
	if (!CBScriptExecute(outputScript, &stack, CBTransactionGetInputHashForSignature, tx, 2)) {
		printf("SIGHASH_NONE AND MODIFIED FIRST OUTPUT SCRIPT FAIL\n");
		return 1;
	}
	CBByteArraySetByte(CBGetByteArray(tx->outputs[0]->scriptObject), 0, CBByteArrayGetByte(CBGetByteArray(tx->outputs[0]->scriptObject), 0) - 1);
	// Test modified third output value. 
	tx->outputs[2]->value++;
	stack = CBNewEmptyScriptStack();
	CBScriptExecute(tx->inputs[2]->scriptObject, &stack, NULL, NULL, 0);
	if (!CBScriptExecute(outputScript, &stack, CBTransactionGetInputHashForSignature, tx, 2)) {
		printf("SIGHASH_NONE AND MODIFIED THIRD OUTPUT VALUE FAIL\n");
		return 1;
	}
	tx->outputs[2]->value--;
	// Test modified third output script
	CBByteArraySetByte(CBGetByteArray(tx->outputs[2]->scriptObject), 0, CBByteArrayGetByte(CBGetByteArray(tx->outputs[2]->scriptObject), 0) + 1);
	stack = CBNewEmptyScriptStack();
	CBScriptExecute(tx->inputs[2]->scriptObject, &stack, NULL, NULL, 0);
	if (!CBScriptExecute(outputScript, &stack, CBTransactionGetInputHashForSignature, tx, 2)) {
		printf("SIGHASH_NONE AND MODIFIED THIRD OUTPUT SCRIPT FAIL\n");
		return 1;
	}
	CBByteArraySetByte(CBGetByteArray(tx->outputs[2]->scriptObject), 0, CBByteArrayGetByte(CBGetByteArray(tx->outputs[2]->scriptObject), 0) - 1);
	// Test modified lock time
	tx->lockTime++;
	stack = CBNewEmptyScriptStack();
	CBScriptExecute(tx->inputs[2]->scriptObject, &stack, NULL, NULL, 0);
	if (CBScriptExecute(outputScript, &stack, CBTransactionGetInputHashForSignature, tx, 2)) {
		printf("SIGHASH_NONE AND MODIFIED LOCK TIME FAIL\n");
		return 1;
	}
	tx->lockTime--;
	// Test modified first input outPointerHash
	CBByteArraySetByte(tx->inputs[0]->outPointerHash, 0, CBByteArrayGetByte(tx->inputs[0]->outPointerHash, 0) + 1);
	stack = CBNewEmptyScriptStack();
	CBScriptExecute(tx->inputs[2]->scriptObject, &stack, NULL, NULL, 0);
	if (CBScriptExecute(outputScript, &stack, CBTransactionGetInputHashForSignature, tx, 2)) {
		printf("SIGHASH_NONE AND MODIFIED FIRST INPUT OUT POINTER HASH FAIL\n");
		return 1;
	}
	CBByteArraySetByte(tx->inputs[0]->outPointerHash, 0, CBByteArrayGetByte(tx->inputs[0]->outPointerHash, 0) - 1);
	// Test modified first input outPointerIndex
	tx->inputs[0]->outPointerIndex++;
	stack = CBNewEmptyScriptStack();
	CBScriptExecute(tx->inputs[2]->scriptObject, &stack, NULL, NULL, 0);
	if (CBScriptExecute(outputScript, &stack, CBTransactionGetInputHashForSignature, tx, 2)) {
		printf("SIGHASH_NONE AND MODIFIED FIRST INPUT OUT POINTER INDEX FAIL\n");
		return 1;
	}
	tx->inputs[0]->outPointerIndex--;
	// Test modified first input sequence
	tx->inputs[0]->sequence++;
	stack = CBNewEmptyScriptStack();
	CBScriptExecute(tx->inputs[2]->scriptObject, &stack, NULL, NULL, 0);
	if (!CBScriptExecute(outputScript, &stack, CBTransactionGetInputHashForSignature, tx, 2)) {
		printf("SIGHASH_NONE AND MODIFIED FIRST INPUT SEQUENCE FAIL\n");
		return 1;
	}
	tx->inputs[0]->sequence--;
	// Test modified third input outPointerHash
	CBByteArraySetByte(tx->inputs[2]->outPointerHash, 0, CBByteArrayGetByte(tx->inputs[2]->outPointerHash, 0) + 1);
	stack = CBNewEmptyScriptStack();
	CBScriptExecute(tx->inputs[2]->scriptObject, &stack, NULL, NULL, 0);
	if (CBScriptExecute(outputScript, &stack, CBTransactionGetInputHashForSignature, tx, 2)) {
		printf("SIGHASH_NONE AND MODIFIED THIRD INPUT OUT POINTER HASH FAIL\n");
		return 1;
	}
	CBByteArraySetByte(tx->inputs[2]->outPointerHash, 0, CBByteArrayGetByte(tx->inputs[2]->outPointerHash, 0) - 1);
	// Test modified third input outPointerIndex
	tx->inputs[2]->outPointerIndex++;
	stack = CBNewEmptyScriptStack();
	CBScriptExecute(tx->inputs[2]->scriptObject, &stack, NULL, NULL, 0);
	if (CBScriptExecute(outputScript, &stack, CBTransactionGetInputHashForSignature, tx, 2)) {
		printf("SIGHASH_NONE AND MODIFIED THIRD INPUT OUT POINTER INDEX FAIL\n");
		return 1;
	}
	tx->inputs[2]->outPointerIndex--;
	// Test modified third input sequence
	tx->inputs[2]->sequence++;
	stack = CBNewEmptyScriptStack();
	CBScriptExecute(tx->inputs[2]->scriptObject, &stack, NULL, NULL, 0);
	if (CBScriptExecute(outputScript, &stack, CBTransactionGetInputHashForSignature, tx, 2)) {
		printf("SIGHASH_NONE AND MODIFIED THIRD INPUT SEQUENCE FAIL\n");
		return 1;
	}
	tx->inputs[2]->sequence--;
	// Test SIGHASH_ANYONECANPAY
	// Execute the transaction scripts to verify correctness.
	stack = CBNewEmptyScriptStack();
	CBScriptExecute(tx->inputs[3]->scriptObject, &stack, NULL, NULL, 0);
	if (!CBScriptExecute(outputScript, &stack, CBTransactionGetInputHashForSignature, tx, 3)) {
		printf("SIGHASH_ANYONECANPAY FAIL\n");
		return 1;
	}
	// Test modified first output value
	tx->outputs[0]->value++;
	stack = CBNewEmptyScriptStack();
	CBScriptExecute(tx->inputs[3]->scriptObject, &stack, NULL, NULL, 0);
	if (CBScriptExecute(outputScript, &stack, CBTransactionGetInputHashForSignature, tx, 3)) {
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
	CBScriptExecute(tx->inputs[3]->scriptObject, &stack, NULL, NULL, 0);
	if (CBScriptExecute(outputScript, &stack, CBTransactionGetInputHashForSignature, tx, 3)) {
		printf("SIGHASH_ANYONECANPAY AND MODIFIED FIRST OUTPUT SCRIPT FAIL\n");
		return 1;
	}
	CBByteArraySetByte(CBGetByteArray(tx->outputs[0]->scriptObject), 0, CBByteArrayGetByte(CBGetByteArray(tx->outputs[0]->scriptObject), 0) - 1);
	// Test modified lock time
	tx->lockTime++;
	stack = CBNewEmptyScriptStack();
	CBScriptExecute(tx->inputs[3]->scriptObject, &stack, NULL, NULL, 0);
	if (CBScriptExecute(outputScript, &stack, CBTransactionGetInputHashForSignature, tx, 3)) {
		printf("SIGHASH_ANYONECANPAY AND MODIFIED LOCK TIME FAIL\n");
		return 1;
	}
	tx->lockTime--;
	// Test modified first input outPointerHash
	CBByteArraySetByte(tx->inputs[0]->outPointerHash, 0, CBByteArrayGetByte(tx->inputs[0]->outPointerHash, 0) + 1);
	stack = CBNewEmptyScriptStack();
	CBScriptExecute(tx->inputs[3]->scriptObject, &stack, NULL, NULL, 0);
	if (!CBScriptExecute(outputScript, &stack, CBTransactionGetInputHashForSignature, tx, 3)) {
		printf("SIGHASH_ANYONECANPAY AND MODIFIED FIRST INPUT OUT POINTER HASH FAIL\n");
		return 1;
	}
	CBByteArraySetByte(tx->inputs[0]->outPointerHash, 0, CBByteArrayGetByte(tx->inputs[0]->outPointerHash, 0) - 1);
	// Test modified first input outPointerIndex
	tx->inputs[0]->outPointerIndex++;
	stack = CBNewEmptyScriptStack();
	CBScriptExecute(tx->inputs[3]->scriptObject, &stack, NULL, NULL, 0);
	if (!CBScriptExecute(outputScript, &stack, CBTransactionGetInputHashForSignature, tx, 3)) {
		printf("SIGHASH_ANYONECANPAY AND MODIFIED FIRST INPUT OUT POINTER INDEX FAIL\n");
		return 1;
	}
	tx->inputs[0]->outPointerIndex--;
	// Test modified first input sequence
	tx->inputs[0]->sequence++;
	stack = CBNewEmptyScriptStack();
	CBScriptExecute(tx->inputs[3]->scriptObject, &stack, NULL, NULL, 0);
	if (!CBScriptExecute(outputScript, &stack, CBTransactionGetInputHashForSignature, tx, 3)) {
		printf("SIGHASH_ANYONECANPAY AND MODIFIED FIRST INPUT SEQUENCE FAIL\n");
		return 1;
	}
	tx->inputs[0]->sequence--;
	// Test modified forth input outPointerHash
	CBByteArraySetByte(tx->inputs[3]->outPointerHash, 0, CBByteArrayGetByte(tx->inputs[3]->outPointerHash, 0) + 1);
	stack = CBNewEmptyScriptStack();
	CBScriptExecute(tx->inputs[3]->scriptObject, &stack, NULL, NULL, 0);
	if (CBScriptExecute(outputScript, &stack, CBTransactionGetInputHashForSignature, tx, 3)) {
		printf("SIGHASH_ANYONECANPAY AND MODIFIED FORTH INPUT OUT POINTER HASH FAIL\n");
		return 1;
	}
	CBByteArraySetByte(tx->inputs[3]->outPointerHash, 0, CBByteArrayGetByte(tx->inputs[3]->outPointerHash, 0) - 1);
	// Test modified forth input outPointerIndex
	tx->inputs[3]->outPointerIndex++;
	stack = CBNewEmptyScriptStack();
	CBScriptExecute(tx->inputs[3]->scriptObject, &stack, NULL, NULL, 0);
	if (CBScriptExecute(outputScript, &stack, CBTransactionGetInputHashForSignature, tx, 3)) {
		printf("SIGHASH_ANYONECANPAY AND MODIFIED FORTH INPUT OUT POINTER INDEX FAIL\n");
		return 1;
	}
	tx->inputs[3]->outPointerIndex--;
	// Test modified forth input sequence
	tx->inputs[3]->sequence++;
	stack = CBNewEmptyScriptStack();
	CBScriptExecute(tx->inputs[3]->scriptObject, &stack, NULL, NULL, 0);
	if (CBScriptExecute(outputScript, &stack, CBTransactionGetInputHashForSignature, tx, 3)) {
		printf("SIGHASH_ANYONECANPAY AND MODIFIED FORTH INPUT SEQUENCE FAIL\n");
		return 1;
	}
	// Free objects
	free(hash);
	CBReleaseObject(&subScriptByteArray);
	CBReleaseObject(&outputScript);
	CBReleaseObject(&tx);
	// Test OP_CHECKMULTISIG
	// Lower signature sizes that were incremented above for SIGHASH
	for (int x = 0; x < 4; x++)
		sigSizes[x]--;
	// Make transaction
	tx = CBNewTransaction(net, 0, CB_PROTOCOL_VERSION, &events);
	hash = CBSha256((u_int8_t []){0x56,0x21,0xF2}, 3);
	bytes = CBNewByteArrayWithData(hash, 32, &events);
	CBTransactionTakeInput(tx, CBNewTransactionInput(net, tx, NULL, bytes, 0, &events)); 
	CBReleaseObject(&bytes);
	// Make output for this transaction
	scriptObj = CBNewScriptWithDataCopy(net, (u_int8_t []){CB_SCRIPT_OP_TRUE}, 1, &events);
	CBTransactionTakeOutput(tx, CBNewTransactionOutput(net, tx, 80, scriptObj, &events));
	CBReleaseObject(&scriptObj);
	// Make previous output script with OP_CHECKMULTISIG
	outputScript = CBNewScriptWithDataCopy(net, (u_int8_t []){CB_SCRIPT_OP_CHECKMULTISIG}, 1, &events);
	// Make signatures
	u_int8_t * signatures[21];
	hash = CBTransactionGetInputHashForSignature(tx, CBGetByteArray(outputScript), 0, CB_SIGHASH_ALL);
	for (int x = 0; x < 21; x++) {
		signatures[x] = malloc(sigSizes[x] + 1);
		ECDSA_sign(0, hash, 32, signatures[x], &sigSizes[x], keys[x]);
		signatures[x][sigSizes[x]] = CB_SIGHASH_ALL;
	}
	free(hash);
	// Test one signature, one key
	scriptObj = CBNewScriptOfSize(net, sigSizes[0] + pubSizes[0] + 6, &events);
	CBByteArraySetByte(CBGetByteArray(scriptObj), 0, CB_SCRIPT_OP_0);
	CBByteArraySetByte(CBGetByteArray(scriptObj), 1, sigSizes[0] + 1);
	CBByteArraySetBytes(CBGetByteArray(scriptObj), 2, signatures[0], sigSizes[0] + 1);
	CBByteArraySetByte(CBGetByteArray(scriptObj), sigSizes[0] + 3, CB_SCRIPT_OP_1);
	CBByteArraySetByte(CBGetByteArray(scriptObj), sigSizes[0] + 4, pubSizes[0]);
	CBByteArraySetBytes(CBGetByteArray(scriptObj), sigSizes[0] + 5, pubKeys[0], pubSizes[0]);
	CBByteArraySetByte(CBGetByteArray(scriptObj), sigSizes[0] + pubSizes[0] + 5, CB_SCRIPT_OP_1);
	tx->inputs[0]->scriptObject = scriptObj;
	stack = CBNewEmptyScriptStack();
	CBScriptExecute(tx->inputs[0]->scriptObject, &stack, NULL, NULL, 0);
	if (!CBScriptExecute(outputScript, &stack, CBTransactionGetInputHashForSignature, tx, 0)) {
		printf("OP_CHECKMULTISIG - ONE SIG - ONE OK KEY - ZERO BAD KEYS FAIL\n");
		return 1;
	}
	CBReleaseObject(&scriptObj);
	// Test two signatures, two keys
	scriptObj = CBNewScriptOfSize(net, sigSizes[0] + pubSizes[0] + sigSizes[1] + pubSizes[1] + 9, &events);
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
	CBScriptExecute(tx->inputs[0]->scriptObject, &stack, NULL, NULL, 0);
	if (!CBScriptExecute(outputScript, &stack, CBTransactionGetInputHashForSignature, tx, 0)) {
		printf("OP_CHECKMULTISIG - TWO SIGS - TWO OK KEYS - ZERO BAD KEYS FAIL\n");
		return 1;
	}
	CBReleaseObject(&scriptObj);
	// Test three signatures, 4 keys with first key fail
	int len = 13; // 3 + 3*2 + 4
	for (int x = 1; x < 4; x++)
		len += sigSizes[x];
	for (int x = 0; x < 4; x++)
		len += pubSizes[x];
	scriptObj = CBNewScriptOfSize(net, len, &events);
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
	CBScriptExecute(tx->inputs[0]->scriptObject, &stack, NULL, NULL, 0);
	if (!CBScriptExecute(outputScript, &stack, CBTransactionGetInputHashForSignature, tx, 0)) {
		printf("OP_CHECKMULTISIG - THREE SIGS - THREE OK KEYS - ONE BAD KEY FAIL\n");
		return 1;
	}
	CBReleaseObject(&scriptObj);
	// Test four signatures, 6 keys with 2,6 key fail
	len = 17; 
	for (int x = 0; x < 4; x++)
		len += sigSizes[x];
	for (int x = 0; x < 6; x++)
		len += pubSizes[x];
	scriptObj = CBNewScriptOfSize(net, len, &events);
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
	CBScriptExecute(tx->inputs[0]->scriptObject, &stack, NULL, NULL, 0);
	if (!CBScriptExecute(outputScript, &stack, CBTransactionGetInputHashForSignature, tx, 0)) {
		printf("OP_CHECKMULTISIG - FOUR SIGS - FOUR OK KEYS - TWO BAD KEYS FAIL\n");
		return 1;
	}
	CBReleaseObject(&scriptObj);
	// Test five signatures, 10 keys with first 5 fail
	len = 23;
	for (int x = 5; x < 10; x++)
		len += sigSizes[x];
	for (int x = 0; x < 10; x++)
		len += pubSizes[x];
	scriptObj = CBNewScriptOfSize(net, len, &events);
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
	CBScriptExecute(tx->inputs[0]->scriptObject, &stack, NULL, NULL, 0);
	if (!CBScriptExecute(outputScript, &stack, CBTransactionGetInputHashForSignature, tx, 0)) {
		printf("OP_CHECKMULTISIG - FIVE SIGS - FIVE OK KEYS - FIVE BAD KEYS FAIL\n");
		return 1;
	}
	CBReleaseObject(&scriptObj);
	// Test five signature, 10 keys with evens fail
	len = 23;
	for (int x = 0; x < 10; x += 2)
		len += sigSizes[x];
	for (int x = 0; x < 10; x++)
		len += pubSizes[x];
	scriptObj = CBNewScriptOfSize(net, len, &events);
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
	CBScriptExecute(tx->inputs[0]->scriptObject, &stack, NULL, NULL, 0);
	if (!CBScriptExecute(outputScript, &stack, CBTransactionGetInputHashForSignature, tx, 0)) {
		printf("OP_CHECKMULTISIG - FIVE SIGS - FIVE OK KEYS ODDS - FIVE BAD KEYS EVENS FAIL\n");
		return 1;
	}
	CBReleaseObject(&scriptObj);
	// Test 20 signatures
	len = 65;
	for (int x = 0; x < 20; x++)
		len += sigSizes[x];
	for (int x = 0; x < 20; x++)
		len += pubSizes[x];
	scriptObj = CBNewScriptOfSize(net, len, &events);
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
	CBScriptExecute(tx->inputs[0]->scriptObject, &stack, NULL, NULL, 0);
	if (!CBScriptExecute(outputScript, &stack, CBTransactionGetInputHashForSignature, tx, 0)) {
		printf("OP_CHECKMULTISIG - TWENTY SIGS - TWENTY OK KEYS - ZERO BAD KEYS FAIL\n");
		return 1;
	}
	CBReleaseObject(&scriptObj);
	// Test 21 signatures failure.
	len = 68;
	for (int x = 0; x < 21; x++)
		len += sigSizes[x];
	for (int x = 0; x < 21; x++)
		len += pubSizes[x];
	scriptObj = CBNewScriptOfSize(net, len, &events);
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
	CBScriptExecute(tx->inputs[0]->scriptObject, &stack, NULL, NULL, 0);
	if (CBScriptExecute(outputScript, &stack, CBTransactionGetInputHashForSignature, tx, 0)) {
		printf("OP_CHECKMULTISIG - TWENTY ONE SIGS - TWENTY ONE OK KEYS - ZERO BAD KEYS FAIL\n");
		return 1;
	}
	CBReleaseObject(&scriptObj);
	// Test one signature, one fail key failure
	scriptObj = CBNewScriptOfSize(net, sigSizes[0] + pubSizes[1] + 6, &events);
	CBByteArraySetByte(CBGetByteArray(scriptObj), 0, CB_SCRIPT_OP_0);
	CBByteArraySetByte(CBGetByteArray(scriptObj), 1, sigSizes[0] + 1);
	CBByteArraySetBytes(CBGetByteArray(scriptObj), 2, signatures[0], sigSizes[0] + 1);
	CBByteArraySetByte(CBGetByteArray(scriptObj), sigSizes[0] + 3, CB_SCRIPT_OP_1);
	CBByteArraySetByte(CBGetByteArray(scriptObj), sigSizes[0] + 4, pubSizes[1]);
	CBByteArraySetBytes(CBGetByteArray(scriptObj), sigSizes[0] + 5, pubKeys[1], pubSizes[1]);
	CBByteArraySetByte(CBGetByteArray(scriptObj), sigSizes[0] + pubSizes[1] + 5, CB_SCRIPT_OP_1);
	tx->inputs[0]->scriptObject = scriptObj;
	stack = CBNewEmptyScriptStack();
	CBScriptExecute(tx->inputs[0]->scriptObject, &stack, NULL, NULL, 0);
	if (CBScriptExecute(outputScript, &stack, CBTransactionGetInputHashForSignature, tx, 0)) {
		printf("OP_CHECKMULTISIG - ONE SIG - ZERO OK KEYS - ONE BAD KEY FAIL\n");
		return 1;
	}
	CBReleaseObject(&scriptObj);
	// Test five signatures, 8 keys with 4 failing failure
	len = 21;
	for (int x = 4; x < 9; x++)
		len += sigSizes[x];
	for (int x = 0; x < 8; x++)
		len += pubSizes[x];
	scriptObj = CBNewScriptOfSize(net, len, &events);
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
	CBScriptExecute(tx->inputs[0]->scriptObject, &stack, NULL, NULL, 0);
	if (CBScriptExecute(outputScript, &stack, CBTransactionGetInputHashForSignature, tx, 0)) {
		printf("OP_CHECKMULTISIG - FIVE SIGS - FOUR OK KEYS - FOUR BAD KEYS FAIL\n");
		return 1;
	}else if (stack.elements[0].data){
		printf("OP_CHECKMULTISIG DOESN'T GIVE NULL ZERO\n");
		return 1;
	}
	CBReleaseObject(&scriptObj);
	// Test low key number
	scriptObj = CBNewScriptOfSize(net, sigSizes[0] + pubSizes[0] + sigSizes[1] + pubSizes[1] + 9, &events);
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
	CBScriptExecute(tx->inputs[0]->scriptObject, &stack, NULL, NULL, 0);
	if (CBScriptExecute(outputScript, &stack, CBTransactionGetInputHashForSignature, tx, 0)) {
		printf("OP_CHECKMULTISIG - LOW KEY NUMBER FAIL\n");
		return 1;
	}
	CBReleaseObject(&scriptObj);
	// Test low sig number
	scriptObj = CBNewScriptOfSize(net, sigSizes[0] + pubSizes[0] + sigSizes[1] + pubSizes[1] + 9, &events);
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
	CBScriptExecute(tx->inputs[0]->scriptObject, &stack, NULL, NULL, 0);
	if (!CBScriptExecute(outputScript, &stack, CBTransactionGetInputHashForSignature, tx, 0)) {
		printf("OP_CHECKMULTISIG - LOW SIG NUMBER FAIL\n");
		return 1;
	}
	CBReleaseObject(&scriptObj);
	// Test high key number
	scriptObj = CBNewScriptOfSize(net, sigSizes[0] + pubSizes[0] + sigSizes[1] + pubSizes[1] + 9, &events);
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
	CBScriptExecute(tx->inputs[0]->scriptObject, &stack, NULL, NULL, 0);
	if (CBScriptExecute(outputScript, &stack, CBTransactionGetInputHashForSignature, tx, 0)) {
		printf("OP_CHECKMULTISIG - HIGH KEY NUMBER FAIL\n");
		return 1;
	}
	CBReleaseObject(&scriptObj);
	// Test high sig number
	scriptObj = CBNewScriptOfSize(net, sigSizes[0] + pubSizes[0] + sigSizes[1] + pubSizes[1] + 9, &events);
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
	CBScriptExecute(tx->inputs[0]->scriptObject, &stack, NULL, NULL, 0);
	if (CBScriptExecute(outputScript, &stack, CBTransactionGetInputHashForSignature, tx, 0)) {
		printf("OP_CHECKMULTISIG - HIGH SIG NUMBER FAIL\n");
		return 1;
	}
	CBReleaseObject(&scriptObj);
	return 0;
}
