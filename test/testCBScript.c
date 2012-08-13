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
#include <openssl/sha.h>
#include <openssl/ripemd.h>
#include <openssl/bn.h>

uint8_t * CBSha160(uint8_t * data,uint16_t len){
	uint8_t * hash = malloc(SHA_DIGEST_LENGTH);
    SHA1(data, len, hash);
	return hash;
}
uint8_t * CBSha256(uint8_t * data,uint16_t len){
	uint8_t * hash = malloc(SHA256_DIGEST_LENGTH);
	SHA256(data, len, hash);
	return hash;
}
uint8_t * CBRipemd160(uint8_t * data,uint16_t len){
	uint8_t * hash = malloc(RIPEMD160_DIGEST_LENGTH);
    RIPEMD160(data, len, hash);
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
		line[strlen(line)-1] = '\0';
		if (!(line[0] == '/' && line[1] == '/') && strlen(line)){
			x++;
			CBScript * script = CBNewScriptFromString(net, line, &events);
			if (!script) {
				printf("%i: {%s} INVALID\n",x,line);
				return 1;
			}else{
				CBScriptStack stack = CBNewEmptyScriptStack();
				bool res = CBScriptExecute(script, &stack, NULL, NULL, 0);
				if (res != ((fgetc(f) == '1') ? true : false)) {
					printf("%i: {%s} FAIL\n",x,line);
					return 1;
				}else{
					printf("%i: {%s} OK\n",x,line);
				}
				CBReleaseObject(script);
				fseek(f, 1, SEEK_CUR);
			}
		}
		if(eof) break;
		free(line);
	}
	fclose(f);
	// Test PUSHDATA
	CBScript * script = CBNewScriptWithDataCopy(net, (uint8_t []){CB_SCRIPT_OP_PUSHDATA1,0x01,0x47,CB_SCRIPT_OP_DUP,CB_SCRIPT_OP_PUSHDATA2,0x01,0x00,0x47,CB_SCRIPT_OP_EQUALVERIFY,CB_SCRIPT_OP_PUSHDATA4,0x01,0x00,0x00,0x00,0x47,CB_SCRIPT_OP_EQUAL}, 16, &events);
	CBScriptStack stack = CBNewEmptyScriptStack();
	if(NOT CBScriptExecute(script, &stack, NULL, NULL, 0)){
		printf("PUSHDATA TEST 1 FAIL\n");
		return 1;
	}
	CBReleaseObject(script);
	script = CBNewScriptOfSize(net, 16, &events);
	script = CBNewScriptWithDataCopy(net, (uint8_t []){CB_SCRIPT_OP_PUSHDATA1,0x01,0x00,CB_SCRIPT_OP_DUP,CB_SCRIPT_OP_PUSHDATA2,0x01,0x00,0x00,CB_SCRIPT_OP_EQUALVERIFY,CB_SCRIPT_OP_PUSHDATA4,0x01,0x00,0x00,0x00,0x00,CB_SCRIPT_OP_EQUAL}, 16, &events);
	stack = CBNewEmptyScriptStack();
	if(NOT CBScriptExecute(script, &stack, NULL, NULL, 0)){
		printf("PUSHDATA TEST 2 FAIL\n");
		return 1;
	}
	CBReleaseObject(script);
	// Test stack length limit
	script = CBNewScriptWithDataCopy(net, (uint8_t []){CB_SCRIPT_OP_TRUE}, 1, &events);
	stack = CBNewEmptyScriptStack();
	for (int x = 0; x < 1001; x++)
		CBScriptStackPushItem(&stack, (CBScriptStackItem){NULL,0});
	if(CBScriptExecute(script, &stack, NULL, NULL, 0)){
		printf("STACK LIMIT TEST FAIL\n");
		return 1;
	}
	CBReleaseObject(script);
	return 0;
}
