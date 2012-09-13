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

// ??? More test cases needed. Maybe from the C++ client?

#include <stdio.h>
#include "CBScript.h"
#include <time.h>
#include "CBDependencies.h"
#include "stdarg.h"

void onErrorReceived(CBError a,char * format,...);
void onErrorReceived(CBError a,char * format,...){
	va_list argptr;
    va_start(argptr, format);
    vfprintf(stderr, format, argptr);
    va_end(argptr);
	printf("\n");
}

int main(){
	unsigned int s = (unsigned int)time(NULL);
	s = 1337544566;
	printf("Session = %ui\n",s);
	srand(s);
	FILE * f = fopen("scriptCases.txt", "r");
	if (!f) printf("FILE WONT OPEN\n");
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
		if(eof) break;
		line[strlen(line)-1] = '\0';
		if (!(line[0] == '/' && line[1] == '/') && strlen(line)){
			x++;
			CBScript * script = CBNewScriptFromString(line, onErrorReceived);
			if (!script) {
				printf("%i: {%s} INVALID\n",x,line);
				return 1;
			}else{
				CBScriptStack stack = CBNewEmptyScriptStack();
				if (x == 31) {
					printf("");
				}
				bool res = CBScriptExecute(script, &stack, NULL, NULL, 0, true);
				CBFreeScriptStack(stack);
				if (res != ((fgetc(f) == '1') ? CB_SCRIPT_VALID : CB_SCRIPT_INVALID)) {
					printf("%i: {%s} FAIL\n",x,line);
					return 1;
				}else{
					printf("%i: {%s} OK\n",x,line);
				}
				CBReleaseObject(script);
				fseek(f, 1, SEEK_CUR);
			}
		}
		free(line);
	}
	fclose(f);
	// Test PUSHDATA
	CBScript * script = CBNewScriptWithDataCopy((uint8_t []){CB_SCRIPT_OP_PUSHDATA1,0x01,0x47,CB_SCRIPT_OP_DUP,CB_SCRIPT_OP_PUSHDATA2,0x01,0x00,0x47,CB_SCRIPT_OP_EQUALVERIFY,CB_SCRIPT_OP_PUSHDATA4,0x01,0x00,0x00,0x00,0x47,CB_SCRIPT_OP_EQUAL}, 16, onErrorReceived);
	CBScriptStack stack = CBNewEmptyScriptStack();
	if(CBScriptExecute(script, &stack, NULL, NULL, 0, true) != CB_SCRIPT_VALID){
		printf("PUSHDATA TEST 1 FAIL\n");
		return 1;
	}
	CBReleaseObject(script);
	script = CBNewScriptWithDataCopy((uint8_t []){CB_SCRIPT_OP_PUSHDATA1,0x01,0x00,CB_SCRIPT_OP_DUP,CB_SCRIPT_OP_PUSHDATA2,0x01,0x00,0x00,CB_SCRIPT_OP_EQUALVERIFY,CB_SCRIPT_OP_PUSHDATA4,0x01,0x00,0x00,0x00,0x00,CB_SCRIPT_OP_EQUAL}, 16, onErrorReceived);
	stack = CBNewEmptyScriptStack();
	if(CBScriptExecute(script, &stack, NULL, NULL, 0, true) != CB_SCRIPT_VALID){
		printf("PUSHDATA TEST 2 FAIL\n");
		return 1;
	}
	CBReleaseObject(script);
	// Test stack length limit
	script = CBNewScriptWithDataCopy((uint8_t []){CB_SCRIPT_OP_TRUE}, 1, onErrorReceived);
	stack = CBNewEmptyScriptStack();
	for (int x = 0; x < 1001; x++)
		CBScriptStackPushItem(&stack, (CBScriptStackItem){NULL,0});
	if(CBScriptExecute(script, &stack, NULL, NULL, 0, true) != CB_SCRIPT_INVALID){
		printf("STACK LIMIT TEST FAIL\n");
		return 1;
	}
	CBReleaseObject(script);
	// Test P2SH
	CBScript * inputScript = CBNewScriptWithDataCopy((uint8_t []){CB_SCRIPT_OP_14,0x04,CB_SCRIPT_OP_5,CB_SCRIPT_OP_9,CB_SCRIPT_OP_ADD,CB_SCRIPT_OP_EQUAL}, 6, onErrorReceived);
	CBScript * outputScript = CBNewScriptWithDataCopy((uint8_t []){CB_SCRIPT_OP_HASH160,0x14,0x87,0xF3,0xB6,0x21,0xF1,0x8C,0x50,0x06,0x8B,0x7D,0xAB,0xA1,0x60,0xBB,0x2C,0x51,0xFD,0xD6,0xA5,0xE2,CB_SCRIPT_OP_EQUAL}, 23, onErrorReceived);
	stack = CBNewEmptyScriptStack();
	CBScriptExecute(inputScript, &stack, NULL, NULL, 0, false);
	if (CBScriptExecute(outputScript, &stack, NULL, NULL, 0, false) != CB_SCRIPT_VALID) {
		printf("OK NO PS2H FAIL\n");
		return 1;
	}
	stack = CBNewEmptyScriptStack();
	CBScriptExecute(inputScript, &stack, NULL, NULL, 0, false);
	if (CBScriptExecute(outputScript, &stack, NULL, NULL, 0, true) != CB_SCRIPT_VALID) {
		printf("OK YES PS2H FAIL\n");
		return 1;
	}
	CBByteArraySetByte(inputScript, 0, CB_SCRIPT_OP_13);
	stack = CBNewEmptyScriptStack();
	CBScriptExecute(inputScript, &stack, NULL, NULL, 0, false);
	if (CBScriptExecute(outputScript, &stack, NULL, NULL, 0, false) != CB_SCRIPT_VALID) {
		printf("BAD NO PS2H FAIL\n");
		return 1;
	}
	stack = CBNewEmptyScriptStack();
	CBScriptExecute(inputScript, &stack, NULL, NULL, 0, false);
	if (CBScriptExecute(outputScript, &stack, NULL, NULL, 0, true) != CB_SCRIPT_INVALID) {
		printf("BAD YES PS2H FAIL\n");
		return 1;
	}
	return 0;
}
