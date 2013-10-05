//
//  testCBScript.c
//  cbitcoin
//
//  Created by Matthew Mitchell on 22/05/2012.
//  Copyright (c) 2012 Matthew Mitchell
//  
//  This file is part of cbitcoin. It is subject to the license terms
//  in the LICENSE file found in the top-level directory of this
//  distribution and at http://www.cbitcoin.com/license.html. No part of
//  cbitcoin, including this file, may be copied, modified, propagated,
//  or distributed except according to the terms contained in the
//  LICENSE file.

#include <stdio.h>
#include "CBScript.h"
#include <time.h>
#include "CBDependencies.h"
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
	FILE * f = fopen("test/scriptCases.txt", "r");
	if (!f){
		printf("FILE WONT OPEN\n");
		return 1;
	}
	for (uint16_t x = 0;;) {
		char * line = NULL;
		uint16_t lineLen = 0;
		bool eof = false;
		while (true) {
			line = realloc(line, lineLen + 101);
			if(!fgets(line + lineLen, 101, f)){
				eof = true;
				break;
			}
			if (line[strlen(line)-1] == '\n') break; // Got to the end of the line
			lineLen += 100;
		}
		if (eof) break;
		line[strlen(line)-1] = '\0';
		if (!(line[0] == '/' && line[1] == '/') && strlen(line)){
			x++;
			CBScript * script = CBNewScriptFromString(line);
			if (!script) {
				printf("%i: {%s} INVALID\n", x, line);
				return 1;
			}else{
				CBScriptStack stack = CBNewEmptyScriptStack();
				CBScriptExecuteReturn res = CBScriptExecute(script, &stack, NULL, NULL, 0, true);
				CBFreeScriptStack(stack);
				char c = fgetc(f);
				if ((c == '1' && res != CB_SCRIPT_TRUE)
					|| (c == '0' && (res != CB_SCRIPT_INVALID && res != CB_SCRIPT_FALSE))) {
					printf("%i: {%s} FAIL\n", x, line);
					return 1;
				}else{
					printf("%i: {%s} OK\n", x, line);
				}
				CBReleaseObject(script);
				fseek(f, 1, SEEK_CUR);
			}
		}
		free(line);
	}
	fclose(f);
	// Test PUSHDATA
	CBScript * script = CBNewScriptWithDataCopy((uint8_t []){CB_SCRIPT_OP_PUSHDATA1, 0x01, 0x47, CB_SCRIPT_OP_DUP, CB_SCRIPT_OP_PUSHDATA2, 0x01, 0x00, 0x47, CB_SCRIPT_OP_EQUALVERIFY, CB_SCRIPT_OP_PUSHDATA4, 0x01, 0x00, 0x00, 0x00, 0x47, CB_SCRIPT_OP_EQUAL}, 16);
	CBScriptStack stack = CBNewEmptyScriptStack();
	if(CBScriptExecute(script, &stack, NULL, NULL, 0, true) != CB_SCRIPT_TRUE){
		printf("PUSHDATA TEST 1 FAIL\n");
		return 1;
	}
	CBReleaseObject(script);
	script = CBNewScriptWithDataCopy((uint8_t []){CB_SCRIPT_OP_PUSHDATA1, 0x01, 0x00, CB_SCRIPT_OP_DUP, CB_SCRIPT_OP_PUSHDATA2, 0x01, 0x00, 0x00, CB_SCRIPT_OP_EQUALVERIFY, CB_SCRIPT_OP_PUSHDATA4, 0x01, 0x00, 0x00, 0x00, 0x00, CB_SCRIPT_OP_EQUAL}, 16);
	stack = CBNewEmptyScriptStack();
	if(CBScriptExecute(script, &stack, NULL, NULL, 0, true) != CB_SCRIPT_TRUE){
		printf("PUSHDATA TEST 2 FAIL\n");
		return 1;
	}
	CBReleaseObject(script);
	// Test stack length limit
	script = CBNewScriptWithDataCopy((uint8_t []){CB_SCRIPT_OP_TRUE}, 1);
	stack = CBNewEmptyScriptStack();
	for (int x = 0; x < 1001; x++)
		CBScriptStackPushItem(&stack, (CBScriptStackItem){NULL, 0});
	if(CBScriptExecute(script, &stack, NULL, NULL, 0, true) != CB_SCRIPT_INVALID){
		printf("STACK LIMIT TEST FAIL\n");
		return 1;
	}
	CBReleaseObject(script);
	// Test P2SH
	CBScript * inputScript = CBNewScriptWithDataCopy((uint8_t []){CB_SCRIPT_OP_14, 0x04, CB_SCRIPT_OP_5, CB_SCRIPT_OP_9, CB_SCRIPT_OP_ADD, CB_SCRIPT_OP_EQUAL}, 6);
	CBScript * outputScript = CBNewScriptWithDataCopy((uint8_t []){CB_SCRIPT_OP_HASH160, 0x14, 0x87, 0xF3, 0xB6, 0x21, 0xF1, 0x8C, 0x50, 0x06, 0x8B, 0x7D, 0xAB, 0xA1, 0x60, 0xBB, 0x2C, 0x51, 0xFD, 0xD6, 0xA5, 0xE2, CB_SCRIPT_OP_EQUAL}, 23);
	stack = CBNewEmptyScriptStack();
	CBScriptExecute(inputScript, &stack, NULL, NULL, 0, false);
	if (CBScriptExecute(outputScript, &stack, NULL, NULL, 0, false) != CB_SCRIPT_TRUE) {
		printf("OK NO PS2H FAIL\n");
		return 1;
	}
	stack = CBNewEmptyScriptStack();
	CBScriptExecute(inputScript, &stack, NULL, NULL, 0, false);
	if (CBScriptExecute(outputScript, &stack, NULL, NULL, 0, true) != CB_SCRIPT_TRUE) {
		printf("OK YES PS2H FAIL\n");
		return 1;
	}
	CBByteArraySetByte(inputScript, 0, CB_SCRIPT_OP_13);
	stack = CBNewEmptyScriptStack();
	CBScriptExecute(inputScript, &stack, NULL, NULL, 0, false);
	if (CBScriptExecute(outputScript, &stack, NULL, NULL, 0, false) != CB_SCRIPT_TRUE) {
		printf("BAD NO PS2H FAIL\n");
		return 1;
	}
	stack = CBNewEmptyScriptStack();
	CBScriptExecute(inputScript, &stack, NULL, NULL, 0, false);
	CBReleaseObject(inputScript);
	if (CBScriptExecute(outputScript, &stack, NULL, NULL, 0, true) != CB_SCRIPT_FALSE) {
		printf("BAD YES PS2H FAIL\n");
		return 1;
	}
	CBFreeScriptStack(stack);
	CBReleaseObject(outputScript);
	// Test CBScriptIsPushOnly
	script = CBNewScriptWithDataCopy((uint8_t [20]){0x02, 0x04, 0x73, CB_SCRIPT_OP_PUSHDATA1, 0x03, 0xA2, 0x70, 0x73, CB_SCRIPT_OP_PUSHDATA2, 0x01, 0x00, 0x5A, CB_SCRIPT_OP_PUSHDATA4, 0x03, 0x0, 0x0, 0x0, 0x5F, 0x70, 0x74}, 20);
	if (CBScriptIsPushOnly(script) != 4) {
		printf("IS PUSH PUSH FAIL\n");
		return 1;
	}
	CBReleaseObject(script);
	script = CBNewScriptWithDataCopy((uint8_t [13]){0x02, 0x04, 0x73, CB_SCRIPT_OP_PUSHDATA1, 0x03, 0xA2, 0x70, 0x73, CB_SCRIPT_OP_PUSHDATA2, 0x01, 0x00, 0x5A, CB_SCRIPT_OP_DEPTH}, 13);
	if (CBScriptIsPushOnly(script) != 0) {
		printf("IS NOT PUSH FAIL\n");
		return 1;
	}
	CBReleaseObject(script);
	return 0;
}
