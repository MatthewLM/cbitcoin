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
#include <openssl/sha.h>
#include <openssl/ripemd.h>
#include "stdarg.h"

void err(CBError a,char * format,...){
	va_list argptr;
    va_start(argptr, format);
    vfprintf(stderr, format, argptr);
    va_end(argptr);
	printf("\n");
}

u_int8_t * sha1(u_int8_t * data,u_int16_t len){
	u_int8_t * hash = malloc(SHA_DIGEST_LENGTH);
    SHA_CTX sha1;
    SHA_Init(&sha1);
    SHA_Update(&sha1, data, len);
    SHA_Final(hash, &sha1);
	return hash;
}
u_int8_t * sha256(u_int8_t * data,u_int16_t len){
	u_int8_t * hash = malloc(SHA256_DIGEST_LENGTH);
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, data, len);
    SHA256_Final(hash, &sha256);
	return hash;
}
u_int8_t * ripemd160(u_int8_t * data,u_int16_t len){
	u_int8_t * hash = malloc(RIPEMD160_DIGEST_LENGTH);
    RIPEMD160_CTX ripemd160;
    RIPEMD160_Init(&ripemd160);
    RIPEMD160_Update(&ripemd160, data, len);
    RIPEMD160_Final(hash, &ripemd160);
	return hash;
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
	CBDependencies dep;
	dep.sha256 = sha256;
	dep.sha1 = sha1;
	dep.ripemd160 = ripemd160;
	// Test CBTransactionInput
	// Test deserialisation
	u_int8_t data[46];
	u_int8_t * hash2 = sha256((u_int8_t *)"hello", 5);
	memmove(data, hash2, 32);
	data[32] = 0x05; // 0x514BFA05
	data[33] = 0xFA;
	data[34] = 0x4B;
	data[35] = 0x51;
	data[37] = CB_SCRIPT_OP_2;
	data[38] = CB_SCRIPT_OP_3;
	data[39] = CB_SCRIPT_OP_ADD;
	data[40] = CB_SCRIPT_OP_5;
	data[41] = CB_SCRIPT_OP_EQUAL;
	CBByteArray * bytes = CBNewByteArrayWithDataCopy(data, 46, &events);
	CBVarIntEncode(bytes, 36, CBVarIntFromUInt64(5));
	u_int32_t sequence = rand();
	CBGetByteArrayVT(bytes)->setUInt32(bytes,42,sequence);
	CBTransactionInput * input = CBNewTransactionInputFromData(net, bytes, NULL, &events);
	CBGetMessageVT(input)->deserialise(input);
	if(memcmp(hash2,CBGetByteArrayVT(input->outPointerHash)->getData(input->outPointerHash),32)){
		printf("CBTransactionInput DESERIALISED outPointerHash INCORRECT: 0x");
		for (int x = 0; x < 32; x++) {
			printf("%.2x",hash2[x]);
		}
		printf(" != 0x");
		u_int8_t * d = CBGetByteArrayVT(input->outPointerHash)->getData(input->outPointerHash);
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
	if(!CBGetScriptVT(input->scriptObject)->execute(input->scriptObject,&stack,&dep)){
		printf("CBTransactionInput DESERIALISED SCRIPT FAILURE\n");
		return 1;
	}
	if (input->sequence != sequence) {
		printf("CBTransactionInput DESERIALISED sequence INCORRECT: %i != %i\n",input->sequence,sequence);
		return 1;
	}
	CBGetObjectVT(input)->release(&input);
	// Test serialisation
	CBByteArray * scriptBytes = CBNewByteArrayWithDataCopy(data + 37, 5, &events);
	CBByteArray * outPointerHash = CBNewByteArrayWithDataCopy(data, 32, &events);
	input = CBNewTransactionInput(net, NULL, scriptBytes, outPointerHash, 0x514BFA05, &events);
	input->sequence = sequence;
	CBGetMessage(input)->bytes = CBNewByteArrayOfSize(46, &events);
	CBGetMessageVT(input)->serialise(input);
	if (!CBGetByteArrayVT(bytes)->equals(bytes,CBGetMessage(input)->bytes)) {
		printf("CBTransactionInput SERIALISATION FAILURE\n0x");
		u_int8_t * d = CBGetByteArrayVT(CBGetMessage(input)->bytes)->getData(CBGetMessage(input)->bytes);
		for (int x = 0; x < 46; x++) {
			printf("%.2x",d[x]);
		}
		printf("\n!=\n0x");
		d = CBGetByteArrayVT(bytes)->getData(bytes);
		for (int x = 0; x < 46; x++) {
			printf("%.2x",d[x]);
		}
		return 1;
	}
	CBGetObjectVT(input)->release(&input);
	CBGetObjectVT(bytes)->release(&bytes);
	// Test CBTransactionOutput
	// Test deserialisation
	bytes = CBNewByteArrayOfSize(14, &events);
	u_int64_t value = rand();
	CBGetByteArrayVT(bytes)->setUInt64(bytes,0,value);
	CBVarIntEncode(bytes, 8, CBVarIntFromUInt64(5));
	CBGetByteArrayVT(bytes)->setBytes(bytes,9,data + 37,5);
	CBTransactionOutput * output = CBNewTransactionOutputFromData(net, bytes,NULL, &events);
	CBGetMessageVT(output)->deserialise(output);
	if (output->value != value) {
		printf("CBTransactionOutput DESERIALISED value INCORRECT: %llu != %llu\n",output->value,value);
		return 1;
	}
	stack = CBNewEmptyScriptStack();
	if(!CBGetScriptVT(output->scriptObject)->execute(output->scriptObject,&stack,&dep)){
		printf("CBTransactionOutput DESERIALISED SCRIPT FAILURE\n");
		return 1;
	}
	CBGetObjectVT(output)->release(&output);
	// Test serialisation
	output = CBNewTransactionOutput(net, NULL, value, scriptBytes, &events);
	CBGetMessage(output)->bytes = CBNewByteArrayOfSize(14, &events);
	CBGetMessageVT(output)->serialise(output);
	if (!CBGetByteArrayVT(bytes)->equals(bytes,CBGetMessage(output)->bytes)) {
		printf("CBTransactionOutput SERIALISATION FAILURE\n0x");
		u_int8_t * d = CBGetByteArrayVT(CBGetMessage(output)->bytes)->getData(CBGetMessage(output)->bytes);
		for (int x = 0; x < 14; x++) {
			printf("%.2x",d[x]);
		}
		printf("\n!=\n0x");
		d = CBGetByteArrayVT(bytes)->getData(bytes);
		for (int x = 0; x < 14; x++) {
			printf("%.2x",d[x]);
		}
		return 1;
	}
	CBGetObjectVT(output)->release(&output);
	CBGetObjectVT(bytes)->release(&bytes);
	// Test CBTransaction
	// Test deserialisation
	bytes = CBNewByteArrayOfSize(187, &events);
	CBGetByteArrayVT(bytes)->setUInt32(bytes,0,CB_PROTOCOL_VERSION);
	CBVarIntEncode(bytes, 4, CBVarIntFromUInt64(3));
	for (u_int8_t y = 0; y < 32; y++) {
		hash2[y] = rand();
	}
	u_int32_t randInt = rand();
	u_int64_t randInt64 = rand();
	u_int8_t * script[5];
	script[0] = malloc(7);
	script[1] = malloc(8);
	script[2] = malloc(8);
	script[3] = malloc(7);
	script[4] = malloc(6);
	// First input test
	CBGetByteArrayVT(bytes)->setBytes(bytes,5,hash2,32);
	CBGetByteArrayVT(bytes)->setUInt32(bytes,37,randInt);
	CBVarIntEncode(bytes, 41, CBVarIntFromUInt64(7));
	script[0][0] = CB_SCRIPT_OP_4;
	script[0][1] = CB_SCRIPT_OP_2;
	script[0][2] = CB_SCRIPT_OP_SUB;
	script[0][3] = CB_SCRIPT_OP_3;
	script[0][4] = CB_SCRIPT_OP_MAX;
	script[0][5] = CB_SCRIPT_OP_3;
	script[0][6] = CB_SCRIPT_OP_EQUAL;
	CBGetByteArrayVT(bytes)->setBytes(bytes,42,script[0],7);
	CBGetByteArrayVT(bytes)->setUInt32(bytes,49,randInt);
	// Second input test
	CBGetByteArrayVT(bytes)->setBytes(bytes,53,hash2,32);
	CBGetByteArrayVT(bytes)->setUInt32(bytes,85,randInt);
	CBVarIntEncode(bytes, 89, CBVarIntFromUInt64(8));
	script[1][0] = CB_SCRIPT_OP_1;
	script[1][1] = CB_SCRIPT_OP_3;
	script[1][2] = CB_SCRIPT_OP_7;
	script[1][3] = CB_SCRIPT_OP_ROT;
	script[1][4] = CB_SCRIPT_OP_SUB;
	script[1][5] = CB_SCRIPT_OP_ADD;
	script[1][6] = CB_SCRIPT_OP_9;
	script[1][7] = CB_SCRIPT_OP_EQUAL;
	CBGetByteArrayVT(bytes)->setBytes(bytes,90,script[1],8);
	CBGetByteArrayVT(bytes)->setUInt32(bytes,98,randInt);
	// Third input test
	CBGetByteArrayVT(bytes)->setBytes(bytes,102,hash2,32);
	CBGetByteArrayVT(bytes)->setUInt32(bytes,134,randInt);
	CBVarIntEncode(bytes, 138, CBVarIntFromUInt64(8));
	script[2][0] = CB_SCRIPT_OP_TRUE;
	script[2][1] = CB_SCRIPT_OP_IF;
	script[2][2] = CB_SCRIPT_OP_3;
	script[2][3] = CB_SCRIPT_OP_ELSE;
	script[2][4] = CB_SCRIPT_OP_2;
	script[2][5] = CB_SCRIPT_OP_ENDIF;
	script[2][6] = CB_SCRIPT_OP_3;
	script[2][7] = CB_SCRIPT_OP_EQUAL;
	CBGetByteArrayVT(bytes)->setBytes(bytes,139,script[2],8);
	CBGetByteArrayVT(bytes)->setUInt32(bytes,147,randInt);
	CBVarIntEncode(bytes, 151, CBVarIntFromUInt64(2));
	// First output test
	CBGetByteArrayVT(bytes)->setUInt64(bytes,152,randInt64);
	CBVarIntEncode(bytes, 160, CBVarIntFromUInt64(7));
	script[3][0] = CB_SCRIPT_OP_3;
	script[3][1] = CB_SCRIPT_OP_DUP;
	script[3][2] = CB_SCRIPT_OP_DUP;
	script[3][3] = CB_SCRIPT_OP_ADD;
	script[3][4] = CB_SCRIPT_OP_ADD;
	script[3][5] = CB_SCRIPT_OP_9;
	script[3][6] = CB_SCRIPT_OP_EQUAL;
	CBGetByteArrayVT(bytes)->setBytes(bytes,161,script[3],7);
	// Second output test
	CBGetByteArrayVT(bytes)->setUInt64(bytes,168,randInt64);
	CBVarIntEncode(bytes, 176, CBVarIntFromUInt64(6));
	script[4][0] = CB_SCRIPT_OP_3;
	script[4][1] = CB_SCRIPT_OP_3;
	script[4][2] = CB_SCRIPT_OP_SUB;
	script[4][3] = CB_SCRIPT_OP_NOT;
	script[4][4] = CB_SCRIPT_OP_TRUE;
	script[4][5] = CB_SCRIPT_OP_BOOLAND;
	CBGetByteArrayVT(bytes)->setBytes(bytes,177,script[4],6);
	CBGetByteArrayVT(bytes)->setUInt32(bytes,183,randInt);
	CBTransaction * tx = CBNewTransactionFromData(net, bytes, &events);
	CBGetMessageVT(tx)->deserialise(tx);
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
		if(memcmp(hash2,CBGetByteArrayVT(tx->inputs[x]->outPointerHash)->getData(tx->inputs[x]->outPointerHash),32)){
			printf("TRANSACTION CBTransactionInput %i DESERIALISED outPointerHash INCORRECT: 0x\n",x);
			for (int x = 0; x < 32; x++) {
				printf("%.2x",hash2[x]);
			}
			printf(" != 0x");
			u_int8_t * d = CBGetByteArrayVT(tx->inputs[x]->outPointerHash)->getData(tx->inputs[x]->outPointerHash);
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
		if(!CBGetScriptVT(tx->inputs[x]->scriptObject)->execute(tx->inputs[x]->scriptObject,&stack,&dep)){
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
		if(!CBGetScriptVT(tx->outputs[x]->scriptObject)->execute(tx->outputs[x]->scriptObject,&stack,&dep)){
			printf("TRANSACTION CBTransactionOutput %i DESERIALISED SCRIPT FAILURE\n",x);
			return 1;
		}
	}
	CBGetObjectVT(tx)->release(&tx);
	// Test serialisation
	tx = CBNewTransaction(net, randInt, CB_PROTOCOL_VERSION, &events);
	outPointerHash = CBNewByteArrayWithDataCopy(hash2, 32, &events);
	for (u_int8_t x = 0; x < 3; x++) {
		scriptBytes = CBNewByteArrayWithDataCopy(script[x], (!x)? 7 : 8, &events);
		input = CBNewTransactionInput(net, NULL, scriptBytes, outPointerHash, randInt, &events);
		input->sequence = randInt;
		CBGetTransactionVT(tx)->addInput(tx,input);
		CBGetObjectVT(scriptBytes)->release(&scriptBytes);
		CBGetObjectVT(input)->release(&input);
	}
	CBGetObjectVT(outPointerHash)->release(&outPointerHash);
	for (u_int8_t x = 3; x < 5; x++) {
		scriptBytes = CBNewByteArrayWithDataCopy(script[x], (x == 3)? 7 : 6, &events);
		output = CBNewTransactionOutput(net, NULL, randInt64, scriptBytes, &events);
		CBGetTransactionVT(tx)->addOutput(tx,output);
		CBGetObjectVT(scriptBytes)->release(&scriptBytes);
		CBGetObjectVT(output)->release(&output);
	}
	CBGetMessage(tx)->bytes = CBNewByteArrayOfSize(187, &events);
	CBGetMessageVT(tx)->serialise(tx);
	if (!CBGetByteArrayVT(bytes)->equals(bytes,CBGetMessage(tx)->bytes)) {
		printf("CBTransaction SERIALISATION FAILURE\n0x");
		u_int8_t * d = CBGetByteArrayVT(CBGetMessage(tx)->bytes)->getData(CBGetMessage(tx)->bytes);
		for (int x = 0; x < 187; x++) {
			printf("%.2x",d[x]);
		}
		printf("\n!=\n0x");
		d = CBGetByteArrayVT(bytes)->getData(bytes);
		for (int x = 0; x < 187; x++) {
			printf("%.2x",d[x]);
		}
		return 1;
	}
	CBGetObjectVT(tx)->release(&tx);
	CBGetObjectVT(bytes)->release(&bytes);
	free(hash2);
	return 0;
}
