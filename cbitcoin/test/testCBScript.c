//
//  testCBScript.c
//  cbitcoin
//
//  Created by Matthew Mitchell on 22/05/2012.
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
#include "CBScript.h"
#include <time.h>
#include <openssl/sha.h>
#include <openssl/ripemd.h>

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
	CBDependencies dep;
	dep.sha256 = sha256;
	dep.sha1 = sha1;
	dep.ripemd160 = ripemd160;
	// CONTROL TEST ONE
	u_int8_t control_test_one[18] = {CB_SCRIPT_OP_FALSE,CB_SCRIPT_OP_IF,CB_SCRIPT_OP_ELSE,CB_SCRIPT_OP_ENDIF,CB_SCRIPT_OP_TRUE,CB_SCRIPT_OP_IF,CB_SCRIPT_OP_ELSE,CB_SCRIPT_OP_ELSE,CB_SCRIPT_OP_TRUE,CB_SCRIPT_OP_ELSE,CB_SCRIPT_OP_ENDIF,CB_SCRIPT_OP_TRUE,CB_SCRIPT_OP_TRUE,CB_SCRIPT_OP_IF,CB_SCRIPT_OP_IF,CB_SCRIPT_OP_ENDIF,CB_SCRIPT_OP_ELSE,CB_SCRIPT_OP_ENDIF};
	u_int8_t * prog_heap = malloc(18);
	memmove(prog_heap, control_test_one,18);
	CBByteArray * program = CBNewByteArrayWithData(prog_heap, 18, &events);
	CBScript * script = CBNewScript(net, program, &events);
	CBGetObjectVT(program)->release(&program);
	CBScriptStack stack = CBNewEmptyScriptStack();
	if(!CBGetScriptVT(script)->execute(script,&stack,&dep)){
		printf("FIRST CONTROL TEST FAILED\n");
		return 1;
	}
	CBFreeScriptStack(stack);
	CBGetObjectVT(script)->release(&script);
	// CONTROL TEST TWO
	u_int8_t control_test_two[4] = {CB_SCRIPT_OP_TRUE,CB_SCRIPT_OP_IF,CB_SCRIPT_OP_ENDIF,CB_SCRIPT_OP_ENDIF};
	prog_heap = malloc(4);
	memmove(prog_heap, control_test_two,4);
	program = CBNewByteArrayWithData(prog_heap, 4, &events);
	script = CBNewScript(net, program, &events);
	CBGetObjectVT(program)->release(&program);
	stack = CBNewEmptyScriptStack();
	if(CBGetScriptVT(script)->execute(script,&stack,&dep)){
		printf("SECOND CONTROL TEST FAILED\n");
		return 1;
	}
	CBFreeScriptStack(stack);
	CBGetObjectVT(script)->release(&script);
	// CONTROL TEST THREE
	u_int8_t control_test_three[5] = {CB_SCRIPT_OP_FALSE,CB_SCRIPT_OP_IF,CB_SCRIPT_OP_ELSE,CB_SCRIPT_OP_ENDIF,CB_SCRIPT_OP_ELSE};
	prog_heap = malloc(5);
	memmove(prog_heap, control_test_three,5);
	program = CBNewByteArrayWithData(prog_heap, 5, &events);
	script = CBNewScript(net, program, &events);
	CBGetObjectVT(program)->release(&program);
	stack = CBNewEmptyScriptStack();
	if(CBGetScriptVT(script)->execute(script,&stack,&dep)){
		printf("THIRD CONTROL TEST FAILED\n");
		return 1;
	}
	CBFreeScriptStack(stack);
	CBGetObjectVT(script)->release(&script);
	// CONTROL TEST FOUR
	u_int8_t control_test_four[6] = {CB_SCRIPT_OP_TRUE,CB_SCRIPT_OP_NOTIF,CB_SCRIPT_OP_FALSE,CB_SCRIPT_OP_ELSE,CB_SCRIPT_OP_TRUE,CB_SCRIPT_OP_ENDIF};
	prog_heap = malloc(6);
	memmove(prog_heap, control_test_four,6);
	program = CBNewByteArrayWithData(prog_heap, 6, &events);
	script = CBNewScript(net, program, &events);
	CBGetObjectVT(program)->release(&program);
	stack = CBNewEmptyScriptStack();
	if(!CBGetScriptVT(script)->execute(script,&stack,&dep)){
		printf("FOURTH CONTROL TEST FAILED\n");
		return 1;
	}
	CBFreeScriptStack(stack);
	CBGetObjectVT(script)->release(&script);
	// CONTROL TEST FIVE
	u_int8_t control_test_five[3] = {CB_SCRIPT_OP_TRUE,CB_SCRIPT_OP_TRUE,CB_SCRIPT_OP_VERIFY};
	prog_heap = malloc(3);
	memmove(prog_heap, control_test_five,3);
	program = CBNewByteArrayWithData(prog_heap, 3, &events);
	script = CBNewScript(net, program, &events);
	CBGetObjectVT(program)->release(&program);
	stack = CBNewEmptyScriptStack();
	if(!CBGetScriptVT(script)->execute(script,&stack,&dep)){
		printf("FIFTH CONTROL TEST FAILED\n");
		return 1;
	}
	CBFreeScriptStack(stack);
	CBGetObjectVT(script)->release(&script);
	// CONTROL TEST SIX
	u_int8_t control_test_six[3] = {CB_SCRIPT_OP_TRUE,CB_SCRIPT_OP_FALSE,CB_SCRIPT_OP_VERIFY};
	prog_heap = malloc(3);
	memmove(prog_heap, control_test_six,3);
	program = CBNewByteArrayWithData(prog_heap, 3, &events);
	script = CBNewScript(net, program, &events);
	CBGetObjectVT(program)->release(&program);
	stack = CBNewEmptyScriptStack();
	if(CBGetScriptVT(script)->execute(script,&stack,&dep)){
		printf("SIXTH CONTROL TEST FAILED\n");
		return 1;
	}
	CBFreeScriptStack(stack);
	CBGetObjectVT(script)->release(&script);
	// STACK TEST ONE
	u_int8_t stack_test_one[3] = {CB_SCRIPT_OP_TRUE,CB_SCRIPT_OP_TOALTSTACK,CB_SCRIPT_OP_FROMALTSTACK};
	prog_heap = malloc(3);
	memmove(prog_heap, stack_test_one,3);
	program = CBNewByteArrayWithData(prog_heap, 3, &events);
	script = CBNewScript(net, program, &events);
	CBGetObjectVT(program)->release(&program);
	stack = CBNewEmptyScriptStack();
	if(!CBGetScriptVT(script)->execute(script,&stack,&dep)){
		printf("FIRST STACK TEST FAILED\n");
		return 1;
	}
	CBFreeScriptStack(stack);
	CBGetObjectVT(script)->release(&script);
	// STACK TEST TWO
	u_int8_t stack_test_two[9] = {CB_SCRIPT_OP_FALSE,CB_SCRIPT_OP_IFDUP,CB_SCRIPT_OP_TRUE,CB_SCRIPT_OP_IFDUP,CB_SCRIPT_OP_DROP,CB_SCRIPT_OP_VERIFY,CB_SCRIPT_OP_NOTIF,CB_SCRIPT_OP_TRUE,CB_SCRIPT_OP_ENDIF};
	prog_heap = malloc(9);
	memmove(prog_heap, stack_test_two,9);
	program = CBNewByteArrayWithData(prog_heap, 9, &events);
	script = CBNewScript(net, program, &events);
	CBGetObjectVT(program)->release(&program);
	stack = CBNewEmptyScriptStack();
	if(!CBGetScriptVT(script)->execute(script,&stack,&dep)){
		printf("SECOND STACK TEST FAILED\n");
		return 1;
	}
	CBFreeScriptStack(stack);
	CBGetObjectVT(script)->release(&script);
	// STACK TEST THREE
	u_int8_t stack_test_three[6] = {CB_SCRIPT_OP_FALSE,CB_SCRIPT_OP_TRUE,CB_SCRIPT_OP_DEPTH,CB_SCRIPT_OP_ROLL,CB_SCRIPT_OP_DEPTH,CB_SCRIPT_OP_PICK};
	prog_heap = malloc(6);
	memmove(prog_heap, stack_test_three,6);
	program = CBNewByteArrayWithData(prog_heap, 6, &events);
	script = CBNewScript(net, program, &events);
	CBGetObjectVT(program)->release(&program);
	stack = CBNewEmptyScriptStack();
	if(!CBGetScriptVT(script)->execute(script,&stack,&dep)){
		printf("THIRD STACK TEST FAILED\n");
		return 1;
	}
	CBFreeScriptStack(stack);
	CBGetObjectVT(script)->release(&script);
	// STACK TEST FOUR
	u_int8_t stack_test_four[6] = {CB_SCRIPT_OP_FALSE,CB_SCRIPT_OP_DUP,CB_SCRIPT_OP_TRUE,CB_SCRIPT_OP_NIP,CB_SCRIPT_OP_SWAP,CB_SCRIPT_OP_DROP};
	prog_heap = malloc(6);
	memmove(prog_heap, stack_test_four,6);
	program = CBNewByteArrayWithData(prog_heap, 6, &events);
	script = CBNewScript(net, program, &events);
	CBGetObjectVT(program)->release(&program);
	stack = CBNewEmptyScriptStack();
	if(!CBGetScriptVT(script)->execute(script,&stack,&dep)){
		printf("FORTH STACK TEST FAILED\n");
		return 1;
	}
	CBFreeScriptStack(stack);
	CBGetObjectVT(script)->release(&script);
	// STACK TEST FIVE
	u_int8_t stack_test_five[21] = {CB_SCRIPT_OP_FALSE,CB_SCRIPT_OP_TRUE,CB_SCRIPT_OP_OVER,CB_SCRIPT_OP_ROT,CB_SCRIPT_OP_3DUP,CB_SCRIPT_OP_2ROT,CB_SCRIPT_OP_TUCK,CB_SCRIPT_OP_2DROP,CB_SCRIPT_OP_2DUP,CB_SCRIPT_OP_2ROT,CB_SCRIPT_OP_2OVER,CB_SCRIPT_OP_2SWAP,CB_SCRIPT_OP_SWAP,CB_SCRIPT_OP_NIP,CB_SCRIPT_OP_NIP,CB_SCRIPT_OP_NIP,CB_SCRIPT_OP_NIP,CB_SCRIPT_OP_NIP,CB_SCRIPT_OP_NIP,CB_SCRIPT_OP_NIP,CB_SCRIPT_OP_NIP}; 
	prog_heap = malloc(21);
	memmove(prog_heap, stack_test_five,21);
	program = CBNewByteArrayWithData(prog_heap, 21, &events);
	script = CBNewScript(net, program, &events);
	CBGetObjectVT(program)->release(&program);
	stack = CBNewEmptyScriptStack();
	if(!CBGetScriptVT(script)->execute(script,&stack,&dep)){
		printf("FIFTH STACK TEST FAILED\n");
		return 1;
	}
	CBFreeScriptStack(stack);
	CBGetObjectVT(script)->release(&script);
	// SPLICE/BITWISE TEST ONE
	u_int8_t splice_bitwise_test_one[10] = {CB_SCRIPT_OP_TRUE,5,1,2,3,4,5,CB_SCRIPT_OP_SIZE,CB_SCRIPT_OP_5,CB_SCRIPT_OP_EQUALVERIFY}; 
	prog_heap = malloc(10);
	memmove(prog_heap, splice_bitwise_test_one,10);
	program = CBNewByteArrayWithData(prog_heap, 10, &events);
	script = CBNewScript(net, program, &events);
	CBGetObjectVT(program)->release(&program);
	stack = CBNewEmptyScriptStack();
	if(!CBGetScriptVT(script)->execute(script,&stack,&dep)){
		printf("FIRST SPLICE/BITWISE TEST FAILED\n");
		return 1;
	}
	CBFreeScriptStack(stack);
	CBGetObjectVT(script)->release(&script);
	// ARITHMETIC TEST ONE
	u_int8_t arithmetic_test_one[13] = {4,0xFF,0xFF,0xFF,0x7F,CB_SCRIPT_OP_1ADD,5,0x00,0x00,0x00,0x80,0x00,CB_SCRIPT_OP_EQUAL}; 
	prog_heap = malloc(13);
	memmove(prog_heap, arithmetic_test_one,13);
	program = CBNewByteArrayWithData(prog_heap, 13, &events);
	script = CBNewScript(net, program, &events);
	CBGetObjectVT(program)->release(&program);
	stack = CBNewEmptyScriptStack();
	if(!CBGetScriptVT(script)->execute(script,&stack,&dep)){
		printf("FIRST ARITHMETIC TEST FAILED\n");
		return 1;
	}
	CBFreeScriptStack(stack);
	CBGetObjectVT(script)->release(&script);
	// ARITHMETIC TEST TWO
	u_int8_t arithmetic_test_two[7] = {1,0xAE,CB_SCRIPT_OP_1SUB,CB_SCRIPT_OP_NEGATE,1,0x2F,CB_SCRIPT_OP_EQUAL}; 
	prog_heap = malloc(7);
	memmove(prog_heap, arithmetic_test_two,7);
	program = CBNewByteArrayWithData(prog_heap, 7, &events);
	script = CBNewScript(net, program, &events);
	CBGetObjectVT(program)->release(&program);
	stack = CBNewEmptyScriptStack();
	if(!CBGetScriptVT(script)->execute(script,&stack,&dep)){
		printf("SECOND ARITHMETIC TEST FAILED\n");
		return 1;
	}
	CBFreeScriptStack(stack);
	CBGetObjectVT(script)->release(&script);
	// ARITHMETIC TEST THREE
	u_int8_t arithmetic_test_three[8] = {1,0xAE,CB_SCRIPT_OP_1SUB,CB_SCRIPT_OP_ABS,1,0x2F,CB_SCRIPT_OP_ABS,CB_SCRIPT_OP_EQUAL}; 
	prog_heap = malloc(8);
	memmove(prog_heap, arithmetic_test_three,8);
	program = CBNewByteArrayWithData(prog_heap, 8, &events);
	script = CBNewScript(net, program, &events);
	CBGetObjectVT(program)->release(&program);
	stack = CBNewEmptyScriptStack();
	if(!CBGetScriptVT(script)->execute(script,&stack,&dep)){
		printf("THIRD ARITHMETIC TEST FAILED\n");
		return 1;
	}
	CBFreeScriptStack(stack);
	CBGetObjectVT(script)->release(&script);
	// ARITHMETIC TEST FOUR
	u_int8_t arithmetic_test_four[8] = {1,0xAE,CB_SCRIPT_OP_NOT,1,0xFA,CB_SCRIPT_OP_0NOTEQUAL,CB_SCRIPT_OP_1SUB,CB_SCRIPT_OP_EQUAL}; 
	prog_heap = malloc(8);
	memmove(prog_heap, arithmetic_test_four,8);
	program = CBNewByteArrayWithData(prog_heap, 8, &events);
	script = CBNewScript(net, program, &events);
	CBGetObjectVT(program)->release(&program);
	stack = CBNewEmptyScriptStack();
	if(!CBGetScriptVT(script)->execute(script,&stack,&dep)){
		printf("FORTH ARITHMETIC TEST FAILED\n");
		return 1;
	}
	CBFreeScriptStack(stack);
	CBGetObjectVT(script)->release(&script);
	// ARITHMETIC TEST FIVE
	u_int8_t arithmetic_test_five[20] = {4,0xFF,0xFF,0xFF,0x7F,CB_SCRIPT_OP_5,CB_SCRIPT_OP_SUB,4,0xFF,0xFA,0xFF,0x7F,CB_SCRIPT_OP_ADD,5,0xF9,0xFA,0xFF,0xFF,0x00,CB_SCRIPT_OP_EQUAL}; 
	prog_heap = malloc(20);
	memmove(prog_heap, arithmetic_test_five,20);
	program = CBNewByteArrayWithData(prog_heap, 20, &events);
	script = CBNewScript(net, program, &events);
	CBGetObjectVT(program)->release(&program);
	stack = CBNewEmptyScriptStack();
	if(!CBGetScriptVT(script)->execute(script,&stack,&dep)){
		printf("FIFTH ARITHMETIC TEST FAILED\n");
		return 1;
	}
	CBFreeScriptStack(stack);
	CBGetObjectVT(script)->release(&script);
	// ARITHMETIC TEST SIX
	u_int8_t arithmetic_test_six[8] = {CB_SCRIPT_OP_TRUE,CB_SCRIPT_OP_FALSE,CB_SCRIPT_OP_BOOLOR,CB_SCRIPT_OP_TRUE,CB_SCRIPT_OP_FALSE,CB_SCRIPT_OP_BOOLAND,CB_SCRIPT_OP_BOOLAND,CB_SCRIPT_OP_NOT}; 
	prog_heap = malloc(8);
	memmove(prog_heap, arithmetic_test_six,8);
	program = CBNewByteArrayWithData(prog_heap, 8, &events);
	script = CBNewScript(net, program, &events);
	CBGetObjectVT(program)->release(&program);
	stack = CBNewEmptyScriptStack();
	if(!CBGetScriptVT(script)->execute(script,&stack,&dep)){
		printf("SIXTH ARITHMETIC TEST FAILED\n");
		return 1;
	}
	CBFreeScriptStack(stack);
	CBGetObjectVT(script)->release(&script);
	// ARITHMETIC TEST SEVEN
	u_int8_t arithmetic_test_seven[9] = {1,0x80,CB_SCRIPT_OP_0,CB_SCRIPT_OP_NUMEQUAL,CB_SCRIPT_OP_1,CB_SCRIPT_OP_NUMEQUALVERIFY,CB_SCRIPT_OP_TRUE,CB_SCRIPT_OP_FALSE,CB_SCRIPT_OP_NUMNOTEQUAL}; 
	prog_heap = malloc(9);
	memmove(prog_heap, arithmetic_test_seven,9);
	program = CBNewByteArrayWithData(prog_heap, 9, &events);
	script = CBNewScript(net, program, &events);
	CBGetObjectVT(program)->release(&program);
	stack = CBNewEmptyScriptStack();
	if(!CBGetScriptVT(script)->execute(script,&stack,&dep)){
		printf("SEVENTH ARITHMETIC TEST FAILED\n");
		return 1;
	}
	CBFreeScriptStack(stack);
	CBGetObjectVT(script)->release(&script);
	// ARITHMETIC TEST EIGHT
	u_int8_t arithmetic_test_eight[11] = {1,34,1,67,CB_SCRIPT_OP_MAX,1,45,CB_SCRIPT_OP_MIN,1,40,CB_SCRIPT_OP_GREATERTHAN}; 
	prog_heap = malloc(11);
	memmove(prog_heap, arithmetic_test_eight,11);
	program = CBNewByteArrayWithData(prog_heap, 11, &events);
	script = CBNewScript(net, program, &events);
	CBGetObjectVT(program)->release(&program);
	stack = CBNewEmptyScriptStack();
	if(!CBGetScriptVT(script)->execute(script,&stack,&dep)){
		printf("EIGHTH ARITHMETIC TEST FAILED\n");
		return 1;
	}
	CBFreeScriptStack(stack);
	CBGetObjectVT(script)->release(&script);
	// ARITHMETIC TEST NINE
	u_int8_t arithmetic_test_nine[20] = {2,0x3A,0x02,1,0x7F,2,0x3A,0x03,CB_SCRIPT_OP_WITHIN,2,0x3A,0x02,2,0x3A,0x02,2,0x3A,0x03,CB_SCRIPT_OP_WITHIN,CB_SCRIPT_OP_BOOLAND}; 
	prog_heap = malloc(20);
	memmove(prog_heap, arithmetic_test_nine,20);
	program = CBNewByteArrayWithData(prog_heap, 20, &events);
	script = CBNewScript(net, program, &events);
	CBGetObjectVT(program)->release(&program);
	stack = CBNewEmptyScriptStack();
	if(!CBGetScriptVT(script)->execute(script,&stack,&dep)){
		printf("NINETH ARITHMETIC TEST FAILED\n");
		return 1;
	}
	CBFreeScriptStack(stack);
	CBGetObjectVT(script)->release(&script);
	// CRYPTO TEST ONE
	u_int8_t crypto_test_one[38] = {10,45,12,169,231,89,231,109,5,4,78,CB_SCRIPT_OP_SHA1,CB_SCRIPT_OP_SHA256,CB_SCRIPT_OP_RIPEMD160,CB_SCRIPT_OP_HASH256,CB_SCRIPT_OP_HASH160,20,0xc5,0xfe,0xcd,0xcc,0xae,0x90,0x9c,0x4d,0x95,0xe9,0xa4,0x9e,0x78,0x07,0x70,0xce,0x9a,0xec,0x80,0x46,CB_SCRIPT_OP_EQUAL}; 
	prog_heap = malloc(38);
	memmove(prog_heap, crypto_test_one,38);
	program = CBNewByteArrayWithData(prog_heap, 38, &events);
	script = CBNewScript(net, program, &events);
	CBGetObjectVT(program)->release(&program);
	stack = CBNewEmptyScriptStack();
	if(!CBGetScriptVT(script)->execute(script,&stack,&dep)){
		printf("FIRST CRYPTO TEST FAILED\n");
		return 1;
	}
	CBFreeScriptStack(stack);
	CBGetObjectVT(script)->release(&script);
	// C++ CLIENT TEST ONE
	
	return 0;
}
