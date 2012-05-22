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

int main(){
	unsigned int s = (unsigned int)time(NULL);
	s = 1337544566;
	printf("Session = %ui\n",s);
	srand(s);
	CBNetworkParameters * net = CBNewNetworkParameters();
	net->networkCode = 0;
	CBEvents events;
	// CONTROL TEST ONE
	u_int8_t control_test_one[18] = {CB_SCRIPT_OP_FALSE,CB_SCRIPT_OP_IF,CB_SCRIPT_OP_ELSE,CB_SCRIPT_OP_ENDIF,CB_SCRIPT_OP_TRUE,CB_SCRIPT_OP_IF,CB_SCRIPT_OP_ELSE,CB_SCRIPT_OP_ELSE,CB_SCRIPT_OP_TRUE,CB_SCRIPT_OP_ELSE,CB_SCRIPT_OP_ENDIF,CB_SCRIPT_OP_TRUE,CB_SCRIPT_OP_TRUE,CB_SCRIPT_OP_IF,CB_SCRIPT_OP_IF,CB_SCRIPT_OP_ENDIF,CB_SCRIPT_OP_ELSE,CB_SCRIPT_OP_ENDIF};
	u_int8_t * prog_heap = malloc(18);
	memmove(prog_heap, control_test_one,18);
	CBByteArray * program = CBNewByteArrayWithData(prog_heap, 18, &events);
	CBScript * script = CBNewScript(net, program, &events);
	CBGetObjectVT(program)->release(&program);
	CBScriptStack stack = CBNewEmptyScriptStack();
	if(!CBGetScriptVT(script)->execute(script,&stack)){
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
	if(CBGetScriptVT(script)->execute(script,&stack)){
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
	if(CBGetScriptVT(script)->execute(script,&stack)){
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
	if(!CBGetScriptVT(script)->execute(script,&stack)){
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
	if(!CBGetScriptVT(script)->execute(script,&stack)){
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
	if(CBGetScriptVT(script)->execute(script,&stack)){
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
	if(!CBGetScriptVT(script)->execute(script,&stack)){
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
	if(!CBGetScriptVT(script)->execute(script,&stack)){
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
	if(!CBGetScriptVT(script)->execute(script,&stack)){
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
	if(!CBGetScriptVT(script)->execute(script,&stack)){
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
	if(!CBGetScriptVT(script)->execute(script,&stack)){
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
	if(!CBGetScriptVT(script)->execute(script,&stack)){
		printf("FIRST SPLICE/BITWISE TEST FAILED\n");
		return 1;
	}
	CBFreeScriptStack(stack);
	CBGetObjectVT(script)->release(&script);
	return 0;
}
