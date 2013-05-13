//
//  CBScript.c
//  cbitcoin
//
//  Created by Matthew Mitchell on 02/05/2012.
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

//  SEE HEADER FILE FOR DOCUMENTATION ??? This code should validate or invalidate scripts exactly the same as the C++ client. That introduces some difficultly. Needs good testing.

#include "CBScript.h"

//  Constructor

CBScript * CBNewScriptFromReference(CBByteArray * program, uint32_t offset, uint32_t len){
	return CBNewByteArraySubReference(program, offset, len);
}
CBScript * CBNewScriptOfSize(uint32_t size){
	return CBNewByteArrayOfSize(size);
}
CBScript * CBNewScriptFromString(char * string){
	CBScript * self = malloc(sizeof(*self));
	if (NOT self) {
		CBLogError("Cannot allocate %i bytes of memory in CBNewScriptFromString\n", sizeof(*self));
		return NULL;
	}
	CBGetObject(self)->free = CBFreeByteArray;
	if (CBInitScriptFromString(self, string))
		return self;
	free(self);
	return NULL;
}
CBScript * CBNewScriptWithData(uint8_t * data, uint32_t size){
	return CBNewByteArrayWithData(data, size);
}
CBScript * CBNewScriptWithDataCopy(uint8_t * data, uint32_t size){
	return CBNewByteArrayWithDataCopy(data, size);
}

//  Object Getter

CBScript * CBGetScript(void * self){
	return self;
}

//  Initialiser

bool CBInitScriptFromString(CBScript * self, char * string){
	uint8_t * data = NULL;
	uint32_t dataLast = 0;
	bool space = false;
	bool line = true;
	for (char * cursor = string; *cursor != '\0';) {
		switch (*cursor) {
			case 'O':
				if (NOT (space ^ line)){
					free(data);
					return false; // Must be lines or a space before operation and cannot be both.
				}
				space = false;
				line = false;
				uint8_t * temp = realloc(data, dataLast + 1);
				if (NOT temp) {
					free(data);
					return false;
				}
				data = temp;
				if(NOT strncmp(cursor, "OP_FALSE", 8)){
					data[dataLast] = CB_SCRIPT_OP_0;
					cursor += 8;
				}else if(NOT strncmp(cursor, "OP_1NEGATE", 10)){
					data[dataLast] = CB_SCRIPT_OP_1NEGATE;
					cursor += 10;
				}else if(NOT strncmp(cursor, "OP_VERNOTIF", 11)){
					data[dataLast] = CB_SCRIPT_OP_VERNOTIF;
					cursor += 11;
				}else if(NOT strncmp(cursor, "OP_VERIFY", 9)){ // OP_VERIFY before OP_VERIF and OP_VER
					data[dataLast] = CB_SCRIPT_OP_VERIFY;
					cursor += 9;
				}else if(NOT strncmp(cursor, "OP_VERIF", 8)){
					data[dataLast] = CB_SCRIPT_OP_VERIF;
					cursor += 8;
				}else if(NOT strncmp(cursor, "OP_VER", 6)){
					data[dataLast] = CB_SCRIPT_OP_VER;
					cursor += 6;
				}else if(NOT strncmp(cursor, "OP_IFDUP", 8)){ // OP_IFDUP before OP_IF
					data[dataLast] = CB_SCRIPT_OP_IFDUP;
					cursor += 8;
				}else if(NOT strncmp(cursor, "OP_IF", 5)){
					data[dataLast] = CB_SCRIPT_OP_IF;
					cursor += 5;
				}else if(NOT strncmp(cursor, "OP_NOTIF", 8)){
					data[dataLast] = CB_SCRIPT_OP_NOTIF;
					cursor += 8;
				}else if(NOT strncmp(cursor, "OP_ELSE", 7)){
					data[dataLast] = CB_SCRIPT_OP_ELSE;
					cursor += 7;
				}else if(NOT strncmp(cursor, "OP_ENDIF", 8)){
					data[dataLast] = CB_SCRIPT_OP_ENDIF;
					cursor += 8;
				}else if(NOT strncmp(cursor, "OP_RETURN", 9)){
					data[dataLast] = CB_SCRIPT_OP_RETURN;
					cursor += 9;
				}else if(NOT strncmp(cursor, "OP_TOALTSTACK", 13)){
					data[dataLast] = CB_SCRIPT_OP_TOALTSTACK;
					cursor += 13;
				}else if(NOT strncmp(cursor, "OP_FROMALTSTACK", 15)){
					data[dataLast] = CB_SCRIPT_OP_FROMALTSTACK;
					cursor += 15;
				}else if(NOT strncmp(cursor, "OP_2DROP", 8)){
					data[dataLast] = CB_SCRIPT_OP_2DROP;
					cursor += 8;
				}else if(NOT strncmp(cursor, "OP_2DUP", 7)){
					data[dataLast] = CB_SCRIPT_OP_2DUP;
					cursor += 7;
				}else if(NOT strncmp(cursor, "OP_3DUP", 7)){
					data[dataLast] = CB_SCRIPT_OP_3DUP;
					cursor += 7;
				}else if(NOT strncmp(cursor, "OP_2OVER", 8)){
					data[dataLast] = CB_SCRIPT_OP_2OVER;
					cursor += 8;
				}else if(NOT strncmp(cursor, "OP_2ROT", 7)){
					data[dataLast] = CB_SCRIPT_OP_2ROT;
					cursor += 7;
				}else if(NOT strncmp(cursor, "OP_2SWAP", 8)){
					data[dataLast] = CB_SCRIPT_OP_2SWAP;
					cursor += 8;
				}else if(NOT strncmp(cursor, "OP_DEPTH", 8)){
					data[dataLast] = CB_SCRIPT_OP_DEPTH;
					cursor += 8;
				}else if(NOT strncmp(cursor, "OP_DROP", 7)){
					data[dataLast] = CB_SCRIPT_OP_DROP;
					cursor += 7;
				}else if(NOT strncmp(cursor, "OP_DUP", 6)){
					data[dataLast] = CB_SCRIPT_OP_DUP;
					cursor += 6;
				}else if(NOT strncmp(cursor, "OP_NIP", 6)){
					data[dataLast] = CB_SCRIPT_OP_NIP;
					cursor += 6;
				}else if(NOT strncmp(cursor, "OP_OVER", 7)){
					data[dataLast] = CB_SCRIPT_OP_OVER;
					cursor += 7;
				}else if(NOT strncmp(cursor, "OP_PICK", 7)){
					data[dataLast] = CB_SCRIPT_OP_PICK;
					cursor += 7;
				}else if(NOT strncmp(cursor, "OP_ROLL", 7)){
					data[dataLast] = CB_SCRIPT_OP_ROLL;
					cursor += 7;
				}else if(NOT strncmp(cursor, "OP_ROT", 6)){
					data[dataLast] = CB_SCRIPT_OP_ROT;
					cursor += 6;
				}else if(NOT strncmp(cursor, "OP_SWAP", 7)){
					data[dataLast] = CB_SCRIPT_OP_SWAP;
					cursor += 7;
				}else if(NOT strncmp(cursor, "OP_TUCK", 7)){
					data[dataLast] = CB_SCRIPT_OP_TUCK;
					cursor += 7;
				}else if(NOT strncmp(cursor, "OP_CAT", 6)){
					data[dataLast] = CB_SCRIPT_OP_CAT;
					cursor += 6;
				}else if(NOT strncmp(cursor, "OP_SUBSTR", 9)){
					data[dataLast] = CB_SCRIPT_OP_SUBSTR;
					cursor += 9;
				}else if(NOT strncmp(cursor, "OP_LEFT", 7)){
					data[dataLast] = CB_SCRIPT_OP_LEFT;
					cursor += 7;
				}else if(NOT strncmp(cursor, "OP_RIGHT", 8)){
					data[dataLast] = CB_SCRIPT_OP_RIGHT;
					cursor += 8;
				}else if(NOT strncmp(cursor, "OP_SIZE", 7)){
					data[dataLast] = CB_SCRIPT_OP_SIZE;
					cursor += 7;
				}else if(NOT strncmp(cursor, "OP_INVERT", 9)){
					data[dataLast] = CB_SCRIPT_OP_INVERT;
					cursor += 9;
				}else if(NOT strncmp(cursor, "OP_AND", 6)){
					data[dataLast] = CB_SCRIPT_OP_AND;
					cursor += 6;
				}else if(NOT strncmp(cursor, "OP_OR", 5)){
					data[dataLast] = CB_SCRIPT_OP_OR;
					cursor += 5;
				}else if(NOT strncmp(cursor, "OP_XOR", 6)){
					data[dataLast] = CB_SCRIPT_OP_XOR;
					cursor += 6;
				}else if(NOT strncmp(cursor, "OP_EQUALVERIFY", 14)){
					data[dataLast] = CB_SCRIPT_OP_EQUALVERIFY;
					cursor += 14;
				}else if(NOT strncmp(cursor, "OP_EQUAL", 8)){
					data[dataLast] = CB_SCRIPT_OP_EQUAL;
					cursor += 8;
				}else if(NOT strncmp(cursor, "OP_RESERVED1", 12)){
					data[dataLast] = CB_SCRIPT_OP_RESERVED1;
					cursor += 12;
				}else if(NOT strncmp(cursor, "OP_RESERVED2", 12)){
					data[dataLast] = CB_SCRIPT_OP_RESERVED2;
					cursor += 12;
				}else if(NOT strncmp(cursor, "OP_RESERVED", 11)){
					data[dataLast] = CB_SCRIPT_OP_RESERVED;
					cursor += 11;
				}else if(NOT strncmp(cursor, "OP_1ADD", 7)){
					data[dataLast] = CB_SCRIPT_OP_1ADD;
					cursor += 7;
				}else if(NOT strncmp(cursor, "OP_1SUB", 7)){
					data[dataLast] = CB_SCRIPT_OP_1SUB;
					cursor += 7;
				}else if(NOT strncmp(cursor, "OP_2MUL", 7)){
					data[dataLast] = CB_SCRIPT_OP_2MUL;
					cursor += 7;
				}else if(NOT strncmp(cursor, "OP_2DIV", 7)){
					data[dataLast] = CB_SCRIPT_OP_2DIV;
					cursor += 7;
				}else if(NOT strncmp(cursor, "OP_NEGATE", 9)){
					data[dataLast] = CB_SCRIPT_OP_NEGATE;
					cursor += 9;
				}else if(NOT strncmp(cursor, "OP_ABS", 6)){
					data[dataLast] = CB_SCRIPT_OP_ABS;
					cursor += 6;
				}else if(NOT strncmp(cursor, "OP_NOT", 6)){
					data[dataLast] = CB_SCRIPT_OP_NOT;
					cursor += 6;
				}else if(NOT strncmp(cursor, "OP_0NOTEQUAL", 12)){
					data[dataLast] = CB_SCRIPT_OP_0NOTEQUAL;
					cursor += 12;
				}else if(NOT strncmp(cursor, "OP_ADD", 6)){
					data[dataLast] = CB_SCRIPT_OP_ADD;
					cursor += 6;
				}else if(NOT strncmp(cursor, "OP_SUB", 6)){
					data[dataLast] = CB_SCRIPT_OP_SUB;
					cursor += 6;
				}else if(NOT strncmp(cursor, "OP_MUL", 6)){
					data[dataLast] = CB_SCRIPT_OP_MUL;
					cursor += 6;
				}else if(NOT strncmp(cursor, "OP_DIV", 6)){
					data[dataLast] = CB_SCRIPT_OP_DIV;
					cursor += 6;
				}else if(NOT strncmp(cursor, "OP_MOD", 6)){
					data[dataLast] = CB_SCRIPT_OP_MOD;
					cursor += 6;
				}else if(NOT strncmp(cursor, "OP_LSHIFT", 9)){
					data[dataLast] = CB_SCRIPT_OP_LSHIFT;
					cursor += 9;
				}else if(NOT strncmp(cursor, "OP_RSHIFT", 9)){
					data[dataLast] = CB_SCRIPT_OP_RSHIFT;
					cursor += 9;
				}else if(NOT strncmp(cursor, "OP_BOOLAND", 10)){
					data[dataLast] = CB_SCRIPT_OP_BOOLAND;
					cursor += 10;
				}else if(NOT strncmp(cursor, "OP_BOOLOR", 9)){
					data[dataLast] = CB_SCRIPT_OP_BOOLOR;
					cursor += 9;
				}else if(NOT strncmp(cursor, "OP_NUMEQUALVERIFY", 17)){
					data[dataLast] = CB_SCRIPT_OP_NUMEQUALVERIFY;
					cursor += 17;
				}else if(NOT strncmp(cursor, "OP_NUMEQUAL", 11)){
					data[dataLast] = CB_SCRIPT_OP_NUMEQUAL;
					cursor += 11;
				}else if(NOT strncmp(cursor, "OP_NUMNOTEQUAL", 14)){
					data[dataLast] = CB_SCRIPT_OP_NUMNOTEQUAL;
					cursor += 14;
				}else if(NOT strncmp(cursor, "OP_LESSTHANOREQUAL", 18)){
					data[dataLast] = CB_SCRIPT_OP_LESSTHANOREQUAL;
					cursor += 18;
				}else if(NOT strncmp(cursor, "OP_LESSTHAN", 11)){
					data[dataLast] = CB_SCRIPT_OP_LESSTHAN;
					cursor += 11;
				}else if(NOT strncmp(cursor, "OP_GREATERTHANOREQUAL", 21)){
					data[dataLast] = CB_SCRIPT_OP_GREATERTHANOREQUAL;
					cursor += 21;
				}else if(NOT strncmp(cursor, "OP_GREATERTHAN", 14)){
					data[dataLast] = CB_SCRIPT_OP_GREATERTHAN;
					cursor += 14;
				}else if(NOT strncmp(cursor, "OP_MIN", 6)){
					data[dataLast] = CB_SCRIPT_OP_MIN;
					cursor += 6;
				}else if(NOT strncmp(cursor, "OP_MAX", 6)){
					data[dataLast] = CB_SCRIPT_OP_MAX;
					cursor += 6;
				}else if(NOT strncmp(cursor, "OP_WITHIN", 9)){
					data[dataLast] = CB_SCRIPT_OP_WITHIN;
					cursor += 9;
				}else if(NOT strncmp(cursor, "OP_RIPEMD160", 12)){
					data[dataLast] = CB_SCRIPT_OP_RIPEMD160;
					cursor += 12;
				}else if(NOT strncmp(cursor, "OP_SHA1", 7)){
					data[dataLast] = CB_SCRIPT_OP_SHA1;
					cursor += 7;
				}else if(NOT strncmp(cursor, "OP_SHA256", 9)){
					data[dataLast] = CB_SCRIPT_OP_SHA256;
					cursor += 9;
				}else if(NOT strncmp(cursor, "OP_HASH160", 10)){
					data[dataLast] = CB_SCRIPT_OP_HASH160;
					cursor += 10;
				}else if(NOT strncmp(cursor, "OP_HASH256", 10)){
					data[dataLast] = CB_SCRIPT_OP_HASH256;
					cursor += 10;
				}else if(NOT strncmp(cursor, "OP_CODESEPARATOR", 16)){
					data[dataLast] = CB_SCRIPT_OP_CODESEPARATOR;
					cursor += 16;
				}else if(NOT strncmp(cursor, "OP_CHECKSIGVERIFY", 17)){
					data[dataLast] = CB_SCRIPT_OP_CHECKSIGVERIFY;
					cursor += 17;
				}else if(NOT strncmp(cursor, "OP_CHECKSIG", 11)){
					data[dataLast] = CB_SCRIPT_OP_CHECKSIG;
					cursor += 11;
				}else if(NOT strncmp(cursor, "OP_CHECKMULTISIGVERIFY", 22)){
					data[dataLast] = CB_SCRIPT_OP_CHECKMULTISIGVERIFY;
					cursor += 22;
				}else if(NOT strncmp(cursor, "OP_CHECKMULTISIG", 16)){
					data[dataLast] = CB_SCRIPT_OP_CHECKMULTISIG;
					cursor += 16;
				}else if(NOT strncmp(cursor, "OP_NOP10", 8)){
					data[dataLast] = CB_SCRIPT_OP_NOP10;
					cursor += 8;
				}else if(NOT strncmp(cursor, "OP_NOP1", 7)){
					data[dataLast] = CB_SCRIPT_OP_NOP1;
					cursor += 7;
				}else if(NOT strncmp(cursor, "OP_NOP2", 7)){
					data[dataLast] = CB_SCRIPT_OP_NOP2;
					cursor += 7;
				}else if(NOT strncmp(cursor, "OP_NOP3", 7)){
					data[dataLast] = CB_SCRIPT_OP_NOP3;
					cursor += 7;
				}else if(NOT strncmp(cursor, "OP_NOP4", 7)){
					data[dataLast] = CB_SCRIPT_OP_NOP4;
					cursor += 7;
				}else if(NOT strncmp(cursor, "OP_NOP5", 7)){
					data[dataLast] = CB_SCRIPT_OP_NOP5;
					cursor += 7;
				}else if(NOT strncmp(cursor, "OP_NOP6", 7)){
					data[dataLast] = CB_SCRIPT_OP_NOP6;
					cursor += 7;
				}else if(NOT strncmp(cursor, "OP_NOP7", 7)){
					data[dataLast] = CB_SCRIPT_OP_NOP7;
					cursor += 7;
				}else if(NOT strncmp(cursor, "OP_NOP8", 7)){
					data[dataLast] = CB_SCRIPT_OP_NOP8;
					cursor += 7;
				}else if(NOT strncmp(cursor, "OP_NOP9", 7)){
					data[dataLast] = CB_SCRIPT_OP_NOP9;
					cursor += 7;
				}else if(NOT strncmp(cursor, "OP_NOP", 6)){
					data[dataLast] = CB_SCRIPT_OP_NOP;
					cursor += 6;
				}else if(NOT strncmp(cursor, "OP_0", 4)){
					data[dataLast] = CB_SCRIPT_OP_0;
					cursor += 4;
				}else if(NOT strncmp(cursor, "OP_10", 5)){
					data[dataLast] = CB_SCRIPT_OP_10;
					cursor += 5;
				}else if(NOT strncmp(cursor, "OP_11", 5)){
					data[dataLast] = CB_SCRIPT_OP_11;
					cursor += 5;
				}else if(NOT strncmp(cursor, "OP_12", 5)){
					data[dataLast] = CB_SCRIPT_OP_12;
					cursor += 5;
				}else if(NOT strncmp(cursor, "OP_13", 5)){
					data[dataLast] = CB_SCRIPT_OP_13;
					cursor += 5;
				}else if(NOT strncmp(cursor, "OP_14", 5)){
					data[dataLast] = CB_SCRIPT_OP_14;
					cursor += 5;
				}else if(NOT strncmp(cursor, "OP_15", 5)){
					data[dataLast] = CB_SCRIPT_OP_15;
					cursor += 5;
				}else if(NOT strncmp(cursor, "OP_16", 5)){
					data[dataLast] = CB_SCRIPT_OP_16;
					cursor += 5;
				}else if(NOT strncmp(cursor, "OP_1", 4)){
					data[dataLast] = CB_SCRIPT_OP_1;
					cursor += 4;
				}else if(NOT strncmp(cursor, "OP_TRUE", 7)){
					data[dataLast] = CB_SCRIPT_OP_1;
					cursor += 7;
				}else if(NOT strncmp(cursor, "OP_2", 4)){
					data[dataLast] = CB_SCRIPT_OP_2;
					cursor += 4;
				}else if(NOT strncmp(cursor, "OP_3", 4)){
					data[dataLast] = CB_SCRIPT_OP_3;
					cursor += 4;
				}else if(NOT strncmp(cursor, "OP_4", 4)){
					data[dataLast] = CB_SCRIPT_OP_4;
					cursor += 4;
				}else if(NOT strncmp(cursor, "OP_5", 4)){
					data[dataLast] = CB_SCRIPT_OP_5;
					cursor += 4;
				}else if(NOT strncmp(cursor, "OP_6", 4)){
					data[dataLast] = CB_SCRIPT_OP_6;
					cursor += 4;
				}else if(NOT strncmp(cursor, "OP_7", 4)){
					data[dataLast] = CB_SCRIPT_OP_7;
					cursor += 4;
				}else if(NOT strncmp(cursor, "OP_8", 4)){
					data[dataLast] = CB_SCRIPT_OP_8;
					cursor += 4;
				}else if(NOT strncmp(cursor, "OP_9", 4)){
					data[dataLast] = CB_SCRIPT_OP_9;
					cursor += 4;
				}else{
					free(data);
					return false;
				}
				dataLast++;
				break;
			case ' ':
				if (space){
					free(data);
					return false; // Only one space
				}
				space = true;
				cursor++;
				break;
			case '\n':
				line = true;
				cursor++;
				break;
			case '0':
				if (NOT (space ^ line)){
					free(data);
					return false; // Must be lines or a space before hex and cannot be both.
				}
				space = false;
				line = false;
				if (cursor[1] == 'x') {
					// Is hex, interpret hex digits.
					char interpret[3];
					char * spaceFind = strchr(cursor, ' ');
					char * lineFind = strchr(cursor, '\n');
					uint32_t diff;
					if (spaceFind == NULL)
						if (lineFind == NULL)
							diff = (uint32_t)strlen(cursor);
						else
							diff = (uint32_t)(lineFind - cursor);
					else
						if (lineFind == NULL)
							diff = (uint32_t)(spaceFind - cursor);
						else if (lineFind < spaceFind)
							diff = (uint32_t)(lineFind - cursor);
						else
							diff = (uint32_t)(spaceFind - cursor);
					diff -= 2; // For the "0x"
					if (diff & 1) {
						// Is odd
						cursor[1] = '0';
						cursor--;
						diff++;
					}
					uint32_t num = diff/2;
					// Create push operations. The number of bytes to push is represented in little endian.
					if (num < 76) {
						uint8_t * temp = realloc(data, dataLast + 1);
						if (NOT temp) {
							free(data);
							return false;
						}
						data = temp;
						data[dataLast] = num;
						dataLast++;
					}else if (num < 256){
						uint8_t * temp = realloc(data, dataLast + 2);
						if (NOT temp) {
							free(data);
							return false;
						}
						data = temp;
						data[dataLast] = CB_SCRIPT_OP_PUSHDATA1;
						dataLast++;
						data[dataLast] = num;
						dataLast++;
					}else if (num < 65536){
						uint8_t * temp = realloc(data, dataLast + 3);
						if (NOT temp) {
							free(data);
							return false;
						}
						data = temp;
						data[dataLast] = CB_SCRIPT_OP_PUSHDATA2;
						dataLast++;
						data[dataLast] = num;
						dataLast++;
						data[dataLast] = num >> 8;
						dataLast++;
					}else{
						uint8_t * temp = realloc(data, dataLast + 5);
						if (NOT temp) {
							free(data);
							return false;
						}
						data = temp;
						data[dataLast] = CB_SCRIPT_OP_PUSHDATA4;
						dataLast++;
						data[dataLast] = num;
						dataLast++;
						data[dataLast] = num >> 8;
						dataLast++;
						data[dataLast] = num >> 16;
						dataLast++;
						data[dataLast] = num >> 24;
						dataLast++;
					}
					while (true) {
						cursor += 2;
						interpret[0] = cursor[0];
						if ((interpret[0] >= '0' && interpret[0] <= '9') 
							|| (interpret[0] >= 'a' && interpret[0] <= 'f') 
							|| (interpret[0] >= 'A' && interpret[0] <= 'F')) {
							interpret[1] = cursor[1];
							interpret[2] = '\0';
							if ((interpret[1] >= '0' && interpret[1] <= '9') 
								|| (interpret[1] >= 'a' && interpret[1] <= 'f') 
								|| (interpret[1] >= 'A' && interpret[1] <= 'F')) {
								// Both
								uint8_t * temp = realloc(data, dataLast + 1);
								if (NOT temp) {
									free(data);
									return false;
								}
								data = temp;
								data[dataLast] = strtoul(interpret, NULL, 16);
								dataLast++;
							}else{
								free(data);
								return false; // In pairs
							}
						}else if(interpret[0] == ' '
								 || interpret[0] == '\n'
								 || interpret[0] == '\0'){
							// End
							break;
						}else{
							free(data);
							return false; // Needs to be space, line or end.
						}
					}
					break;
				}else{
					free(data);
					return false; // Not x for hex.
				}
			default:
				free(data);
				return false;
		}
	}
	// Got data. Now create script byte array for data
	if (NOT CBInitByteArrayWithData(self, data, dataLast))
		return false;
	return true;
}

//  Functions

void CBFreeScriptStack(CBScriptStack stack){
	for (int x = 0; x < stack.length; x++)
		free(stack.elements[x].data);
	free(stack.elements);
}
CBScriptStack CBNewEmptyScriptStack(){
	CBScriptStack stack;
	stack.elements = NULL;
	stack.length = 0;
	return stack;
}
CBScriptExecuteReturn CBScriptExecute(CBScript * self, CBScriptStack * stack, CBGetHashReturn (*getHashForSig)(void *, CBByteArray *, uint32_t, CBSignType, uint8_t *), void * transaction, uint32_t inputIndex, bool p2sh){
	// ??? Adding syntax parsing to the begining of the interpreter is maybe a good idea.
	// This looks confusing but isn't too bad, trust me.
	CBScriptStack altStack = CBNewEmptyScriptStack();
	uint16_t skipIfElseBlock = 0xffff; // Skips all instructions on or over this if/else level.
	uint16_t ifElseSize = 0; // Amount of if/else block levels
	uint32_t beginSubScript = 0;
	uint32_t cursor = 0;
	if (self->length > 10000)
		return CB_SCRIPT_INVALID; // Script is an illegal size.
	// Determine if P2SH https://en.bitcoin.it/wiki/BIP_0016
	CBScriptStackItem p2shScript;
	bool isP2SH;
	if (p2sh && CBScriptIsP2SH(self)) {
		p2shScript = CBScriptStackCopyItem(stack, 0);
		if (NOT p2shScript.data && p2shScript.length)
			return CB_SCRIPT_ERR;
		isP2SH = true;
	}else isP2SH = false;
	for (uint8_t opCount = 0; self->length - cursor > 0;) {
		CBScriptOp byte = CBByteArrayGetByte(self, cursor);
		if (byte > CB_SCRIPT_OP_16 && ++opCount > 201)
			return CB_SCRIPT_INVALID; // Too many op codes
		if (byte == CB_SCRIPT_OP_VERIF
			|| byte == CB_SCRIPT_OP_VERNOTIF
			|| byte == CB_SCRIPT_OP_CAT
			|| byte == CB_SCRIPT_OP_SUBSTR
			|| byte == CB_SCRIPT_OP_LEFT
			|| byte == CB_SCRIPT_OP_RIGHT
			|| byte == CB_SCRIPT_OP_INVERT
			|| byte == CB_SCRIPT_OP_AND
			|| byte == CB_SCRIPT_OP_OR
			|| byte == CB_SCRIPT_OP_XOR
			|| byte == CB_SCRIPT_OP_2MUL
			|| byte == CB_SCRIPT_OP_2DIV
			|| byte == CB_SCRIPT_OP_CAT
			|| byte == CB_SCRIPT_OP_MUL
			|| byte == CB_SCRIPT_OP_DIV
			|| byte == CB_SCRIPT_OP_MOD
			|| byte == CB_SCRIPT_OP_LSHIFT
			|| byte == CB_SCRIPT_OP_RSHIFT) {
			return CB_SCRIPT_INVALID; // Invalid op codes independent of execution
		}
		cursor++;
		// Control management for skipping
		if (ifElseSize >= skipIfElseBlock) { // Skip when "ifElseSize" level is over or at "skipIfElseBlock"
			if (byte == CB_SCRIPT_OP_ELSE && ifElseSize == skipIfElseBlock) {
				skipIfElseBlock = 0xffff; // No more skipping
			}else if (byte == CB_SCRIPT_OP_ENDIF){
				if (ifElseSize == skipIfElseBlock) {
					skipIfElseBlock = 0xffff; // No more skipping
				}
				ifElseSize--;
			}else if (byte == CB_SCRIPT_OP_IF){
				ifElseSize++;
			}
		}else{ // Execution for no skipping
			if (NOT byte) {
				// Push 0 onto stack. This is NULL due to counter intuitive rubbish in the C++ client.
				CBScriptStackItem item = {NULL, 0};
				if (NOT CBScriptStackPushItem(stack, item))
					return CB_SCRIPT_ERR;
			}else if (byte < 76){
				// Check size
				if ((self->length - cursor) < byte)
					return CB_SCRIPT_INVALID; // Not enough space.
				// Push data the size of the value of the byte
				CBScriptStackItem item;
				item.data = malloc(byte);
				if (NOT item.data){
					CBLogError("Run out of memory during a script push operation. Attempted to push %i bytes\n", byte);
					return CB_SCRIPT_ERR;
				}
				item.length = byte;
				memmove(item.data, CBByteArrayGetData(self) + cursor, byte);
				if (NOT CBScriptStackPushItem(stack, item))
					return CB_SCRIPT_ERR;
				cursor += byte;
			}else if (byte < 79){
				// Push data with the length of bytes represented by the next bytes. The number of bytes to push is in little endian unlike arithmetic operations which is weird.
				uint32_t amount;
				if(self->length - cursor < 1)
					return CB_SCRIPT_INVALID; // Needs at least one more byte
				if (byte == CB_SCRIPT_OP_PUSHDATA1){
					amount = CBByteArrayGetByte(self, cursor);
					cursor++;
				}else if (byte == CB_SCRIPT_OP_PUSHDATA2){
					if (self->length - cursor < 2)
						return CB_SCRIPT_INVALID; // Not enough space.
					amount = CBByteArrayReadInt16(self, cursor);
					cursor += 2;
				}else{
					if (self->length - cursor < 4)
						return CB_SCRIPT_INVALID; // Not enough space.
					amount = CBByteArrayReadInt32(self, cursor);
					cursor += 4;
				}
				// Check limitation
				if (amount > 520)
					return CB_SCRIPT_INVALID; // Size of data to push is illegal.
				// Check size
				if (self->length - cursor < amount)
					return CB_SCRIPT_INVALID; // Not enough space.
				CBScriptStackItem item;
				if (amount){
					item.data = malloc(amount);
					if (NOT item.data){
						CBLogError("Run out of memory during a script push operation. Attempted to push %i bytes\n", amount);
						return CB_SCRIPT_ERR;
					}
					memmove(item.data, CBByteArrayGetData(self) + cursor, amount);
				}else{
					item.data = NULL;
				}
				item.length = amount;
				if (NOT CBScriptStackPushItem(stack, item))
					return CB_SCRIPT_ERR;
				cursor += amount;
			}else if (byte == CB_SCRIPT_OP_1NEGATE){
				// Push -1 onto the stack
				CBScriptStackItem item;
				item.data = malloc(1);
				if (NOT item.data){
					CBLogError("Run out of memory during OP_1NEGATE\n");
					return CB_SCRIPT_ERR;
				}
				item.length = 1;
				item.data[0] = 0x81; // 10000001 Not like normal signed integers, most significant bit applies sign, making the rest of the bits take away from zero.
				if (NOT CBScriptStackPushItem(stack, item))
					return CB_SCRIPT_ERR;
			}else if(byte == CB_SCRIPT_OP_RESERVED){
				return CB_SCRIPT_INVALID;
			}else if (byte < 97){
				// Push a number onto the stack
				CBScriptStackItem item;
				item.data = malloc(1);
				if (NOT item.data){
					CBLogError("Run out of memory during OP_%i\n", byte - CB_SCRIPT_OP_1 + 1);
					return CB_SCRIPT_ERR;
				}
				item.length = 1;
				item.data[0] = byte - CB_SCRIPT_OP_1 + 1;
				if (NOT CBScriptStackPushItem(stack, item))
					return CB_SCRIPT_ERR;
			}else if (byte == CB_SCRIPT_OP_NOP){
				// Nothing...
			}else if (byte == CB_SCRIPT_OP_IF 
					  || byte == CB_SCRIPT_OP_NOTIF){
				// If top of stack is true, continue, else goto OP_ELSE or OP_ENDIF.
				ifElseSize++;
				if (NOT stack->length)
					return CB_SCRIPT_INVALID; // Stack empty
				bool res = CBScriptStackEvalBool(stack);
				if ((res && byte == CB_SCRIPT_OP_IF) 
					|| (NOT res && byte == CB_SCRIPT_OP_NOTIF))
					skipIfElseBlock = 0xffff;
				else
					skipIfElseBlock = ifElseSize; // Is skipping on this level until OP_ELSE or OP_ENDIF is reached on this level
				// Remove top stack item
				CBScriptStackRemoveItem(stack);
			}else if (byte == CB_SCRIPT_OP_ELSE){
				if (NOT ifElseSize)
					return CB_SCRIPT_INVALID; // OP_ELSE on lowest level not possible
				skipIfElseBlock = ifElseSize; // Begin skipping
			}else if (byte == CB_SCRIPT_OP_ENDIF){
				if (NOT ifElseSize)
					return CB_SCRIPT_INVALID; // OP_ENDIF on lowest level not possible
				ifElseSize--; // Lower level
			}else if (byte == CB_SCRIPT_OP_VERIFY){
				if (NOT stack->length)
					return CB_SCRIPT_INVALID; // Stack empty
				if (CBScriptStackEvalBool(stack))
					// Remove top stack item
					CBScriptStackRemoveItem(stack);
				else
					return CB_SCRIPT_INVALID; // Failed verification
			}else if (byte == CB_SCRIPT_OP_RETURN){
				return CB_SCRIPT_INVALID; // Failed verification with OP_RETURN.
			}else if (byte == CB_SCRIPT_OP_TOALTSTACK){
				if (NOT stack->length)
					return CB_SCRIPT_INVALID; // Stack empty
				if (NOT CBScriptStackPushItem(&altStack, CBScriptStackPopItem(stack)))
					return CB_SCRIPT_ERR;
			}else if (byte == CB_SCRIPT_OP_FROMALTSTACK){
				if (NOT altStack.length)
					return CB_SCRIPT_INVALID; // Alternative stack empty
				if (NOT CBScriptStackPushItem(stack, CBScriptStackPopItem(&altStack)))
					return CB_SCRIPT_ERR;
			}else if (byte == CB_SCRIPT_OP_IFDUP){
				if (NOT stack->length)
					return CB_SCRIPT_INVALID; // Stack empty
				if (CBScriptStackEvalBool(stack)){
					//Duplicate top stack item
					CBScriptStackItem item = CBScriptStackCopyItem(stack, 0);
					if (NOT item.data && item.length)
						return CB_SCRIPT_ERR;
					if (NOT CBScriptStackPushItem(stack, item))
						return CB_SCRIPT_ERR;
				}
			}else if (byte == CB_SCRIPT_OP_DEPTH){
				CBScriptStackItem temp = CBInt64ToScriptStackItem((CBScriptStackItem){NULL, 0}, stack->length);
				if (NOT temp.data && temp.length)
					return CB_SCRIPT_ERR;
				if (NOT CBScriptStackPushItem(stack, temp))
					return CB_SCRIPT_ERR;
			}else if (byte == CB_SCRIPT_OP_DROP){
				if (NOT stack->length)
					return CB_SCRIPT_INVALID; // Stack empty
				CBScriptStackRemoveItem(stack);
			}else if (byte == CB_SCRIPT_OP_DUP){
				if (NOT stack->length)
					return CB_SCRIPT_INVALID; // Stack empty
				//Duplicate top stack item
				CBScriptStackItem item = CBScriptStackCopyItem(stack, 0);
				if (NOT item.data && item.length)
					return CB_SCRIPT_ERR;
				if (NOT CBScriptStackPushItem(stack, item))
					return CB_SCRIPT_ERR;
			}else if (byte == CB_SCRIPT_OP_NIP){
				if (stack->length < 2)
					return CB_SCRIPT_INVALID; // Stack needs 2 or more elements.
				// Remove second from top item.
				stack->length--;
				free(stack->elements[stack->length-1].data);
				stack->elements[stack->length-1] = stack->elements[stack->length]; // Top item moves down
				CBScriptStackItem * temp = realloc(stack->elements, sizeof(*stack->elements)*stack->length);
				if (NOT temp) {
					return CB_SCRIPT_ERR;
				}
				stack->elements = temp;
			}else if (byte == CB_SCRIPT_OP_OVER){
				if (stack->length < 2)
					return CB_SCRIPT_INVALID; // Stack needs 2 or more elements.
				CBScriptStackItem item = CBScriptStackCopyItem(stack, 1);
				if (NOT item.data && item.length)
					return CB_SCRIPT_ERR;
				if (NOT CBScriptStackPushItem(stack, item)) // Copies second from top and pushes it on the top.
					return CB_SCRIPT_ERR;
			}else if (byte == CB_SCRIPT_OP_PICK || byte == CB_SCRIPT_OP_ROLL){
				if (stack->length < 2)
					return CB_SCRIPT_INVALID; // Stack needs 2 or more elements.
				CBScriptStackItem item = CBScriptStackPopItem(stack);
				if (item.length > 4)
					return CB_SCRIPT_INVALID; // Protocol does not except integers more than 32 bits.
				int64_t i = CBScriptStackItemToInt64(item);
				if (i < 0) // Negative
					return CB_SCRIPT_INVALID; // Must be positive
				if (i >= stack->length)
					return CB_SCRIPT_INVALID; // Must be smaller than the stack size
				if (byte == CB_SCRIPT_OP_PICK) {
					// Copy element
					CBScriptStackItem item = CBScriptStackCopyItem(stack, i);
					if (NOT item.data && item.length)
						return CB_SCRIPT_ERR;
					if (NOT CBScriptStackPushItem(stack, item))
						return CB_SCRIPT_ERR;
				}else{ // CB_SCRIPT_OP_ROLL
					// Move element.
					CBScriptStackItem temp = stack->elements[stack->length-i-1]; // Get the item to "roll"
					for (uint32_t x = 0; x < i; x++) // Move other elements down
						stack->elements[stack->length-i+x-1] = stack->elements[stack->length-i+x];
					stack->elements[stack->length-1] = temp;
				}
			}else if (byte == CB_SCRIPT_OP_ROT){
				if (stack->length < 3)
					return CB_SCRIPT_INVALID; // Stack needs 3 or more elements.
				// Rotate top three elements to the left.
				CBScriptStackItem temp = stack->elements[stack->length-3];
				stack->elements[stack->length-3] = stack->elements[stack->length-2];
				stack->elements[stack->length-2] = stack->elements[stack->length-1];
				stack->elements[stack->length-1] = temp;
			}else if (byte == CB_SCRIPT_OP_SWAP){
				if (stack->length < 2)
					return CB_SCRIPT_INVALID; // Stack needs 2 or more elements.
				CBScriptStackItem temp = stack->elements[stack->length-2];
				stack->elements[stack->length-2] = stack->elements[stack->length-1];
				stack->elements[stack->length-1] = temp;
			}else if (byte == CB_SCRIPT_OP_TUCK){
				if (stack->length < 2)
					return CB_SCRIPT_INVALID; // Stack needs 2 or more elements.
				CBScriptStackItem item = CBScriptStackCopyItem(stack, 0);
				if (NOT item.data && item.length)
					return CB_SCRIPT_ERR;
				// New copy three down.
				stack->length++;
				CBScriptStackItem * temp = realloc(stack->elements, sizeof(*stack->elements)*stack->length);
				if (NOT temp) {
					return CB_SCRIPT_ERR;
				}
				stack->elements = temp;
				stack->elements[stack->length-1] = stack->elements[stack->length-2];
				stack->elements[stack->length-2] = stack->elements[stack->length-3];
				stack->elements[stack->length-3] = item;
			}else if (byte == CB_SCRIPT_OP_2DROP){
				if (stack->length < 2)
					return CB_SCRIPT_INVALID; // Stack needs 2 or more elements.
				CBScriptStackRemoveItem(stack);
				CBScriptStackRemoveItem(stack);
			}else if (byte == CB_SCRIPT_OP_2DUP
					  || byte == CB_SCRIPT_OP_3DUP){
				if (stack->length < byte - CB_SCRIPT_OP_2DUP + 2)
					return CB_SCRIPT_INVALID; // Stack needs more elements.
				for (uint8_t x = 0; x < byte - CB_SCRIPT_OP_2DUP + 2; x++) {
					CBScriptStackItem i = CBScriptStackCopyItem(stack, byte - CB_SCRIPT_OP_2DUP + 1); 
					if (NOT i.data && i.length)
						return CB_SCRIPT_ERR;
					if (NOT CBScriptStackPushItem(stack, i))
						return CB_SCRIPT_ERR;
				}
			}else if (byte == CB_SCRIPT_OP_2OVER){
				if (stack->length < 4)
					return CB_SCRIPT_INVALID; // Stack needs 4 or more elements.
				CBScriptStackItem i = CBScriptStackCopyItem(stack, 3);
				if (NOT i.data && i.length)
					return CB_SCRIPT_ERR;
				if (NOT CBScriptStackPushItem(stack, i))
					return CB_SCRIPT_ERR;
				i = CBScriptStackCopyItem(stack, 3);
				if (NOT i.data && i.length)
					return CB_SCRIPT_ERR;
				if (NOT CBScriptStackPushItem(stack, i))
					return CB_SCRIPT_ERR;
			}else if (byte == CB_SCRIPT_OP_2ROT){
				if (stack->length < 6)
					return CB_SCRIPT_INVALID; // Stack needs 6 or more elements.
				// Rotate top three pairs of elements to the left.
				CBScriptStackItem temp = stack->elements[stack->length-6];
				CBScriptStackItem temp2 = stack->elements[stack->length-5];
				stack->elements[stack->length-6] = stack->elements[stack->length-4];
				stack->elements[stack->length-5] = stack->elements[stack->length-3];
				stack->elements[stack->length-4] = stack->elements[stack->length-2];
				stack->elements[stack->length-3] = stack->elements[stack->length-1];
				stack->elements[stack->length-2] = temp;
				stack->elements[stack->length-1] = temp2;
			}else if (byte == CB_SCRIPT_OP_2SWAP){
				if (stack->length < 4)
					return CB_SCRIPT_INVALID; // Stack needs 4 or more elements.
				CBScriptStackItem temp = stack->elements[stack->length-4];
				CBScriptStackItem temp2 = stack->elements[stack->length-3];
				stack->elements[stack->length-4] = stack->elements[stack->length-2];
				stack->elements[stack->length-3] = stack->elements[stack->length-1];
				stack->elements[stack->length-2] = temp;
				stack->elements[stack->length-1] = temp2;
			}else if (byte == CB_SCRIPT_OP_SIZE){
				if (NOT stack->length)
					return CB_SCRIPT_INVALID; // Stack empty
				CBScriptStackItem temp = CBInt64ToScriptStackItem((CBScriptStackItem){NULL, 0}, stack->elements[stack->length-1].length);
				if (NOT temp.data && temp.length)
					return CB_SCRIPT_ERR;
				if (NOT CBScriptStackPushItem(stack, temp))
					return CB_SCRIPT_ERR;
			}else if (byte == CB_SCRIPT_OP_EQUAL 
					  || byte == CB_SCRIPT_OP_EQUALVERIFY){
				if (stack->length < 2)
					return CB_SCRIPT_INVALID; // Stack needs 2 or more elements.
				CBScriptStackItem i1 = CBScriptStackPopItem(stack);
				CBScriptStackItem i2 = CBScriptStackPopItem(stack);
				bool ok = true;
				if (i1.length != i2.length)
					ok = false;
				else for (uint16_t x = 0; x < i1.length; x++)
					if (i1.data[x] != i2.data[x]) {
						ok = false;
						break;
					}
				if (byte == CB_SCRIPT_OP_EQUALVERIFY){
					if (NOT ok)
						return CB_SCRIPT_INVALID; // Failed verification
				}else{
					// Push result onto stack
					CBScriptStackItem item;
					if (ok) {
						item.data = malloc(1);
						if (NOT item.data){
							CBLogError("Run out of memory during OP_EQUAL\n");
							return CB_SCRIPT_ERR;
						}
						item.length = 1;
						item.data[0] = 1;
					}else{
						item.data = NULL;
						item.length = 0;
					}
					if (NOT CBScriptStackPushItem(stack, item))
						return CB_SCRIPT_ERR;
				}
			}else if (byte == CB_SCRIPT_OP_1ADD 
					  || byte == CB_SCRIPT_OP_1SUB){
				if (NOT stack->length)
					return CB_SCRIPT_INVALID; // Stack empty
				CBScriptStackItem item = stack->elements[stack->length-1];
				if (item.length > 4)
					return CB_SCRIPT_INVALID; // Protocol does not except integers more than 32 bits.
				// Convert to 64 bit integer.
				int64_t res = CBScriptStackItemToInt64(item);
				if (byte == CB_SCRIPT_OP_1ADD)
					res++;
				else
					res--;
				// Convert back to bitcoin format. Re-assign item as length may have changed.
				stack->elements[stack->length-1] = CBInt64ToScriptStackItem(item, res);
				if (NOT stack->elements[stack->length-1].data && stack->elements[stack->length-1].length)
					// Detected error.
					return CB_SCRIPT_ERR;
			}else if (byte == CB_SCRIPT_OP_NEGATE){
				if (NOT stack->length)
					return CB_SCRIPT_INVALID; // Stack empty
				CBScriptStackItem * item = &stack->elements[stack->length-1];
				if (item->length > 4)
					return CB_SCRIPT_INVALID; // Protocol does not except integers more than 32 bits.
				if (item->data == NULL) { // Zero
					// Zero becomes 0x80 :-( Sorry, this madness comes from the C++ client which represents zero in a horrid way.
					item->data = malloc(1);
					if (NOT item->data){
						CBLogError("Run out of memory during OP_NEGATE\n");
						return CB_SCRIPT_ERR;
					}
					item->length = 1;
					item->data[0] = 0x80; // -0
				}else if(item->data[0] != 0x80){
					item->data[item->length-1] ^= 0x80; // Toggles most significant bit.
				}else{
					// Negative zero becomes NULL. Positive zero is NULL to support weirdness with the C++ client. Arghh. ??? Needs checking over for inevitable inconsistencies with C++.
					free(item->data);
					item->data = NULL;
					item->length = 0;
				}
			}else if (byte == CB_SCRIPT_OP_ABS){
				if (NOT stack->length)
					return CB_SCRIPT_INVALID; // Stack empty
				CBScriptStackItem item = stack->elements[stack->length-1];
				if (item.length > 4)
					return CB_SCRIPT_INVALID; // Protocol does not except integers more than 32 bits.
				if (item.data != NULL) { // If not zero
					item.data[item.length-1] &= 0x7F; // Unsets most significant bit.
				}
			}else if (byte == CB_SCRIPT_OP_NOT 
					  || byte == CB_SCRIPT_OP_0NOTEQUAL){
				if (NOT stack->length)
					return CB_SCRIPT_INVALID; // Stack empty
				CBScriptStackItem item = stack->elements[stack->length-1];
				if (item.length > 4)
					return CB_SCRIPT_INVALID; // Protocol does not except integers more than 32 bits.
				bool res = CBScriptStackEvalBool(stack);
				if ((NOT res && byte == CB_SCRIPT_OP_NOT) || (res && byte == CB_SCRIPT_OP_0NOTEQUAL)) {
					item.length = 1;
					uint8_t * temp = realloc(item.data, 1);
					if (NOT temp)
						return CB_SCRIPT_ERR;
					item.data = temp;
					item.data[0] = 1;
				}else{
					// Should be zero as NULL. Remember the C++ represents zero as an empty vector. Here NULL is used. This can be slightly annoying.
					free(item.data);
					item.data = NULL;
					item.length = 0;
				}
				stack->elements[stack->length-1] = item;
			}else if (byte == CB_SCRIPT_OP_ADD 
					  || byte == CB_SCRIPT_OP_SUB 
					  || byte == CB_SCRIPT_OP_NUMEQUAL 
					  || byte == CB_SCRIPT_OP_NUMNOTEQUAL 
					  || byte == CB_SCRIPT_OP_NUMEQUALVERIFY 
					  || byte == CB_SCRIPT_OP_LESSTHAN 
					  || byte == CB_SCRIPT_OP_LESSTHANOREQUAL 
					  || byte == CB_SCRIPT_OP_GREATERTHAN 
					  || byte == CB_SCRIPT_OP_GREATERTHANOREQUAL 
					  || byte == CB_SCRIPT_OP_MIN 
					  || byte == CB_SCRIPT_OP_MAX){
				if (stack->length < 2)
					return CB_SCRIPT_INVALID; // Stack needs 2 or more elements.
				// Take top two items, removing the top one. First on which is two down will be assigned the result.
				CBScriptStackItem i1 = stack->elements[stack->length-2];
				CBScriptStackItem i2 = CBScriptStackPopItem(stack);
				if (i1.length > 4 || i2.length > 4)
					return CB_SCRIPT_INVALID; // Protocol does not except integers more than 32 bits.
				int64_t res = CBScriptStackItemToInt64(i1);
				int64_t second = CBScriptStackItemToInt64(i2);
				free(i2.data); // No longer need i2
				switch (byte) {
					case CB_SCRIPT_OP_ADD: res += second; break;
					case CB_SCRIPT_OP_SUB: res -= second; break;
					case CB_SCRIPT_OP_NUMEQUALVERIFY:
					case CB_SCRIPT_OP_NUMEQUAL: res = (res == second); break;
					case CB_SCRIPT_OP_NUMNOTEQUAL: res = (res != second); break;
					case CB_SCRIPT_OP_LESSTHAN: res = (res < second); break;
					case CB_SCRIPT_OP_LESSTHANOREQUAL: res = (res <= second); break;
					case CB_SCRIPT_OP_GREATERTHAN: res = (res > second); break;
					case CB_SCRIPT_OP_GREATERTHANOREQUAL: res = (res >= second); break;
					case CB_SCRIPT_OP_MIN: res = (res > second)? second : res; break;
					default: res = (res < second)? second : res; break;
				}
				if (byte == CB_SCRIPT_OP_NUMEQUALVERIFY){
					if (NOT res)
						return CB_SCRIPT_INVALID;
					CBScriptStackRemoveItem(stack); // Remove top item that will not hold the rest as this is OP_NUMEQUALVERIFY
				}else{
					// Convert back to bitcoin format. Re-assign item as length may have changed. i1 now goes on top.
					stack->elements[stack->length-1] = CBInt64ToScriptStackItem(i1, res);
					if (NOT stack->elements[stack->length-1].data && stack->elements[stack->length-1].length)
						// Detected error.
						return CB_SCRIPT_ERR;
				}
			}else if (byte == CB_SCRIPT_OP_BOOLAND
					  || byte == CB_SCRIPT_OP_BOOLOR){
				if (stack->length < 2)
					return CB_SCRIPT_INVALID; // Stack needs 2 or more elements
				// Take top two items, removing the top one. First on which is two down will be assigned the result.
				CBScriptStackItem i1 = stack->elements[stack->length-2];
				bool i2bool = CBScriptStackEvalBool(stack);
				CBScriptStackItem i2 = CBScriptStackPopItem(stack);
				bool i1bool = CBScriptStackEvalBool(stack);
				if (i1.length > 4 || i2.length > 4)
					return CB_SCRIPT_INVALID; // Protocol does not except integers more than 32 bits.
				free(i2.data); // No longer need i2.
				i1.length = 1;
				uint8_t * temp = realloc(i1.data, 1);
				if (NOT temp)
					return CB_SCRIPT_ERR;
				i1.data = temp;
				i1.data[0] = (byte == CB_SCRIPT_OP_BOOLAND)? i1bool && i2bool : i1bool || i2bool;
				stack->elements[stack->length-1] = i1;
			}else if (byte == CB_SCRIPT_OP_WITHIN){
				if (stack->length < 3)
					return CB_SCRIPT_INVALID; // Stack needs 3 or more elements
				CBScriptStackItem item = stack->elements[stack->length-3];
				CBScriptStackItem top = CBScriptStackPopItem(stack);
				CBScriptStackItem bottom = CBScriptStackPopItem(stack);
				if (item.length > 4 || top.length > 4 || bottom.length > 4)
					return CB_SCRIPT_INVALID; // Protocol does not except integers more than 32 bits.
				int64_t res = CBScriptStackItemToInt64(item);
				int64_t topi = CBScriptStackItemToInt64(top);
				free(top.data); // No longer need top.
				int64_t bottomi = CBScriptStackItemToInt64(bottom);
				free(bottom.data); // No longer need bottom.
				item.length = 1;
				uint8_t * temp = realloc(item.data, 1);
				if (NOT temp)
					return CB_SCRIPT_ERR;
				item.data = temp;
				item.data[0] = bottomi <= res && res < topi;
				stack->elements[stack->length-1] = item;
			}else if (byte == CB_SCRIPT_OP_RIPEMD160
					  || byte == CB_SCRIPT_OP_SHA1
					  || byte == CB_SCRIPT_OP_HASH160
					  || byte == CB_SCRIPT_OP_SHA256
					  || byte == CB_SCRIPT_OP_HASH256){
				if (NOT stack->length)
					return CB_SCRIPT_INVALID; // Stack cannot be empty
				CBScriptStackItem item = stack->elements[stack->length-1];
				uint8_t * data;
				uint8_t dataTemp[32];
				switch (byte) {
					case CB_SCRIPT_OP_RIPEMD160:
						data = malloc(20);
						if (NOT data){
							CBLogError("Run out of memory during OP_RIPEMD160\n");
							return CB_SCRIPT_ERR;
						}
						CBRipemd160(item.data, item.length, data);
						break;
					case CB_SCRIPT_OP_SHA1:
						data = malloc(20);
						if (NOT data){
							CBLogError("Run out of memory during OP_SHA1\n");
							return CB_SCRIPT_ERR;
						}
						CBSha160(item.data, item.length, data);
						break;
					case CB_SCRIPT_OP_HASH160:
						CBSha256(item.data, item.length, dataTemp);
						data = malloc(20);
						if (NOT data){
							CBLogError("Run out of memory during OP_HASH160\n");
							return CB_SCRIPT_ERR;
						}
						CBRipemd160(dataTemp, 32, data);
						break;
					case CB_SCRIPT_OP_SHA256:
						data = malloc(32);
						if (NOT data){
							CBLogError("Run out of memory during OP_SHA256\n");
							return CB_SCRIPT_ERR;
						}
						CBSha256(item.data, item.length, data);
						break;
					default:
						CBSha256(item.data, item.length, dataTemp);
						data = malloc(32);
						if (NOT data){
							CBLogError("Run out of memory during OP_HASH256\n");
							return CB_SCRIPT_ERR;
						}
						CBSha256(dataTemp, 32, data);
						break;
				}
				free(item.data);
				item.data = data;
				item.length = (byte == CB_SCRIPT_OP_SHA256 || byte == CB_SCRIPT_OP_HASH256)? 32 : 20;
				stack->elements[stack->length-1] = item;
			}else if (byte == CB_SCRIPT_OP_CODESEPARATOR){
				beginSubScript = cursor;
			}else if (byte == CB_SCRIPT_OP_CHECKSIG
					  || byte == CB_SCRIPT_OP_CHECKSIGVERIFY
					  || byte == CB_SCRIPT_OP_CHECKMULTISIG
					  || byte == CB_SCRIPT_OP_CHECKMULTISIGVERIFY){
				// Get sub script and remove OP_CODESEPARATORs
				uint32_t subScriptLen = self->length - beginSubScript;
				uint8_t * subScript = malloc(subScriptLen);
				if (NOT subScript){
					CBLogError("Run out of memory during CHECKSIG operation\n");
					return CB_SCRIPT_ERR;
				}
				uint8_t * sourceScript = CBByteArrayGetData(self);
				uint8_t * subScriptCopyPointer = subScript;
				uint8_t * lastSeparator = sourceScript + beginSubScript;
				uint8_t * end = sourceScript + self->length;
				uint8_t * ptr = lastSeparator;
				// Remove code separators for subScript.
				bool fail = false; // Checks for push failures.
				for (;;) {
					if (ptr >= end) {
						// "That's all" http://www.youtube.com/watch?v=Uhknpo6a_zE
						memmove(subScriptCopyPointer, lastSeparator, end - lastSeparator);
						break;
					}else if (*ptr == CB_SCRIPT_OP_CODESEPARATOR) {
						uint32_t len = (uint32_t)(ptr - lastSeparator); // Do not include code separator.
						memmove(subScriptCopyPointer, lastSeparator, len);
						lastSeparator = ptr + 1; // Point past code seperator for copying next time.
						subScriptCopyPointer += len;
						subScriptLen--; // One less element.
					}
					// Move to next operation
					if(*ptr < 0x4f && *ptr){ 
                                            /* ops less than 0x4f are pushdata */
						// If pushing bytes, skip these bytes. 
                                                /* ops less than 0x4c are data */
						if (*ptr < 0x4c) {
							ptr += *ptr + 1;
						}else{ 
							uint32_t move;
							if (*ptr == CB_SCRIPT_OP_PUSHDATA1){
								move = 2;
								if (ptr + 1 >= end){ //Failure
									fail = true;
									break;
								}
								move += ptr[1];
							}else if (*ptr == CB_SCRIPT_OP_PUSHDATA2){
								move = 3;
								if (ptr + 2 >= end){ //Failure
									fail = true;
									break;
								}
								move += ptr[1];
								move += ptr[2] << 8;
							}else{ // PUSHDATA4
								move = 5;
								if (ptr + 4 >= end){ //Failure
									fail = true;
									break;
								}
								move += ptr[1];
								move += ptr[2] << 8;
								move += ptr[3] << 16;
								move += ptr[4] << 24;
							}
							ptr += move;
						}
					}else{
						ptr++; // Not a push operation, move along one.
					}
				}
				if (fail) { // Push failure.
					free(subScript);
					return CB_SCRIPT_INVALID;
				}
				bool res;
				uint8_t hash[32];
				if (byte == CB_SCRIPT_OP_CHECKSIG
					|| byte == CB_SCRIPT_OP_CHECKSIGVERIFY){
					if (stack->length < 2){
						free(subScript);
						return CB_SCRIPT_INVALID; // Stack needs 2 or more elements
					}
					CBScriptStackItem publicKey = CBScriptStackPopItem(stack);
					CBScriptStackItem signature = CBScriptStackPopItem(stack);
					if (signature.data == NULL || publicKey.data == NULL) {
						// Is zero. Not valid so give res as false
						res = false;
					}else{ // Can check signature
						CBSignType signType = signature.data[signature.length-1];
						// Delete any instances of the signature
						CBSubScriptRemoveSignature(subScript, &subScriptLen, signature);
						// Complete verification
						CBByteArray * subScriptByteArray = CBNewByteArrayWithData(subScript, subScriptLen);
						if (NOT subScriptByteArray) {
							free(publicKey.data);
							free(signature.data);
							return CB_SCRIPT_ERR;
						}
						CBGetHashReturn hashRes = getHashForSig(transaction, subScriptByteArray, inputIndex, signType, hash);
						if (hashRes == CB_TX_HASH_ERR){
							free(publicKey.data);
							free(signature.data);
							CBReleaseObject(subScriptByteArray);
							return CB_SCRIPT_ERR;
						}else if (hashRes == CB_TX_HASH_OK){
							// Use minus one on the signature length because the hash type
							res = CBEcdsaVerify(signature.data, signature.length-1, hash, publicKey.data, publicKey.length);
						}else res = false;
						CBReleaseObject(subScriptByteArray);
					}
					free(publicKey.data);
					free(signature.data);
				}else{
					if (stack->length < 5){
						free(subScript);
						return CB_SCRIPT_INVALID; // Stack needs 5 or more elements. At least for numSig, numKeys, one key and signature and the dummy value due to an issue with the protocol.
					}
					int64_t numKeys = CBScriptStackItemToInt64(stack->elements[stack->length-1]);
					uint8_t sig = 3 + numKeys; // To first signature.
					uint8_t key = 2; // To first key.
					if (numKeys < 0 || numKeys > 20){
						free(subScript);
                        return CB_SCRIPT_INVALID;
					}
					opCount += numKeys;
					if (opCount > 201){
						free(subScript);
                        return CB_SCRIPT_INVALID;
					}
					if (stack->length < numKeys + 4){
						free(subScript);
						return CB_SCRIPT_INVALID; // Not enough space on stack for keys
					}
					int64_t numSigs = CBScriptStackItemToInt64(stack->elements[stack->length-2-numKeys]); // Go back the number of keys to find the number of signatures.
					if (numSigs < 0 || numSigs > numKeys){
						free(subScript);
                        return CB_SCRIPT_INVALID; // The number of signatures must be positive and blow or equal to the number of keys.
					}
					if (stack->length < 3 + numKeys + numSigs){
						free(subScript);
						return CB_SCRIPT_INVALID; // Not enough space for keys, signatures, numSig, numKeys and the dummy value.
					}
					// Remove signatures from subScript
					for (uint8_t x = 0; x < numSigs; x++) {
						CBScriptStackItem sigItem = stack->elements[stack->length-sig-x];
						CBSubScriptRemoveSignature(subScript, &subScriptLen, sigItem);
					}
					CBByteArray * subScriptByteArray = CBNewByteArrayWithData(subScript, subScriptLen);
					if (NOT subScriptByteArray)
						return CB_SCRIPT_ERR;
					res = true;
					uint8_t removeItemsNum = 3 + numKeys + numSigs;
					while (res && numSigs > 0){
                        CBScriptStackItem * signature = &stack->elements[stack->length - sig]; // Reference for changed data
                        CBScriptStackItem publicKey = stack->elements[stack->length - key];
						if (publicKey.data == NULL) {
							// Oh dear, signature is zero. Can't be validated so break from here
							res = false;
							break;
						}
						if (publicKey.data != NULL) { // If not zero can check signature.
							// Get sign type
							CBSignType signType = signature->data[signature->length-1];
							// Check signature
							CBGetHashReturn hashRes = getHashForSig(transaction, subScriptByteArray, inputIndex, signType, hash);
							if (hashRes == CB_TX_HASH_ERR){
								CBReleaseObject(subScriptByteArray);
								return CB_SCRIPT_ERR;
							}else if (hashRes == CB_TX_HASH_OK){
								// Use minus one on the signature length because the hash type
								if (CBEcdsaVerify(signature->data, signature->length-1, hash, publicKey.data, publicKey.length)){
									sig++;
									numSigs--;
								}
							}
						}
                        key++;
						numKeys--;
                        if (numSigs > numKeys)
                            res = false; // More signatures than keys. Cannot verify all signatures.
                    }
					CBReleaseObject(subScriptByteArray);
					// Remove the items from the stack including an additional dummy value because of a problem in the bitcoin protocol.
					for (uint8_t x = 0; x < removeItemsNum; x++)
						CBScriptStackRemoveItem(stack);
				}
				if (byte == CB_SCRIPT_OP_CHECKSIG
					|| byte == CB_SCRIPT_OP_CHECKMULTISIG) {
					CBScriptStackItem item;
					if (res) {
						item.data = malloc(1);
						if (NOT item.data){
							CBLogError("Run out of memory during CHECKSIG operation push.\n");
							return CB_SCRIPT_ERR;
						}
						item.length = 1;
						item.data[0] = 1;
					}else{
						item.data = NULL;
						item.length = 0;
					}
					if(NOT CBScriptStackPushItem(stack, item))
						return CB_SCRIPT_ERR;
				}else if (NOT res){
					return CB_SCRIPT_INVALID;
				}
			}else if (byte > 175 && byte < 186){
				// NOP
			}else{
				return CB_SCRIPT_INVALID;
			}
			if (stack->length + altStack.length > 1000)
				return CB_SCRIPT_INVALID; // Stack size over the limit
		}
	}
	if (ifElseSize)
		return CB_SCRIPT_INVALID; // If/Else Block(s) not terminated.
	if (NOT stack->length)
		return CB_SCRIPT_FALSE; // Stack empty.
	if (CBScriptStackEvalBool(stack)) {
		if (isP2SH){
			CBScript * p2shScriptObj = CBNewScriptWithData(p2shScript.data, p2shScript.length);
			if (NOT p2shScriptObj)
				return CB_SCRIPT_ERR;
			CBScriptStackRemoveItem(stack); // Remove OP_TRUE
			bool res = CBScriptExecute(p2shScriptObj, stack, getHashForSig, transaction, inputIndex, false);
			CBReleaseObject(p2shScriptObj);
			return res;
		}
		return CB_SCRIPT_TRUE;
	}else return CB_SCRIPT_FALSE;
}
uint32_t CBScriptGetSigOpCount(CBScript * self, bool inP2SH){
	uint32_t sigOps = 0;
	CBScriptOp lastOp = CB_SCRIPT_OP_INVALIDOPCODE;
	uint32_t cursor = 0;
	for (;cursor < self->length;) {
		CBScriptOp op = CBByteArrayGetByte(self, cursor);
		if (op < 76) {
			cursor += op + 1;
		}else if (op < 79){
			cursor++;
			if(self->length - cursor < 1)
				break; // Needs at least one more byte
			if (op == CB_SCRIPT_OP_PUSHDATA1)
				cursor += 1 + CBByteArrayGetByte(self, cursor);
			else if (op == CB_SCRIPT_OP_PUSHDATA2){
				if (self->length - cursor < 2)
					break; // Not enough space.
				cursor += 2 + CBByteArrayReadInt16(self, cursor);
			}else{
				if (self->length - cursor < 4)
					break; // Not enough space.
				cursor += 4 + CBByteArrayReadInt32(self, cursor);
			}
		}else if (op == CB_SCRIPT_OP_CHECKSIG || op == CB_SCRIPT_OP_CHECKSIGVERIFY){
			sigOps++;
			cursor++;
		}else if (op == CB_SCRIPT_OP_CHECKMULTISIG || op == CB_SCRIPT_OP_CHECKMULTISIGVERIFY){
			if (inP2SH && lastOp >= CB_SCRIPT_OP_1 && lastOp <= CB_SCRIPT_OP_16)
				sigOps += lastOp - CB_SCRIPT_OP_1 + 1;
			else sigOps += 20;
			cursor++;
		}else cursor++;
		lastOp = op;
	}
	return sigOps;
}
bool CBScriptIsP2SH(CBScript * self){
	return (self->length == 23
			&& CBByteArrayGetByte(self, 0) == CB_SCRIPT_OP_HASH160
			&& CBByteArrayGetByte(self, 1) == 0x14
			&& CBByteArrayGetByte(self, 22) == CB_SCRIPT_OP_EQUAL);
}
bool CBScriptIsPushOnly(CBScript * self){
	uint32_t x = 0;
	for (; x < self->length;) { 
		CBScriptOp op = CBByteArrayGetByte(self, x);
		if (op <= CB_SCRIPT_OP_16) {
			// Is a push operation
			x++;
			uint32_t a;
			if (NOT op
				|| op == CB_SCRIPT_OP_1NEGATE
				|| op >= CB_SCRIPT_OP_1) {
				// Single opcodes
				a = 0;
			}else if (op < 76){
				x += op;
				a = 0;
			}else if (op == CB_SCRIPT_OP_PUSHDATA1){
				a = CBByteArrayGetByte(self, x);
				x++;
			}else if (op == CB_SCRIPT_OP_PUSHDATA2){
				a = CBByteArrayReadInt16(self, x);
				x += 2;
			}else{
				a = CBByteArrayReadInt32(self, x);
				x += 4;
			}
			if (a > 520)
				return false;
			x += a;
		}else return false;
	}
	return x == self->length;
}
void CBSubScriptRemoveSignature(uint8_t * subScript, uint32_t * subScriptLen, CBScriptStackItem signature){
	if (signature.data == NULL) return; // Signature zero
	uint8_t * ptr = subScript;
	uint8_t * end = subScript + *subScriptLen;
	for (;ptr < end;) {
		if (*ptr && *ptr < 79) { // Push
			// Move to data for push and record movement to next operation. No checking of push data bounds since it should have already been done.
			uint32_t move;
			if (*ptr < 76) {
				move = *ptr;
				ptr++;
			}else if (*ptr == CB_SCRIPT_OP_PUSHDATA1){
				move = ptr[1];
				ptr += 2;
			}else if (*ptr == CB_SCRIPT_OP_PUSHDATA2){
				move = ptr[1];
				move += ptr[2] << 8;
				ptr += 3;
			}else{ // PUSHDATA4
				move = ptr[1];
				move += ptr[2] << 8;
				move += ptr[3] << 16;
				move += ptr[4] << 24;
				ptr += 5;
			}
			// Check within bounds
			if (ptr + signature.length >= end)
				break;
			// Check signature
			if (memcmp(ptr, signature.data, signature.length)) {
				// Remove signature
				memmove(ptr, ptr + signature.length, end - (ptr + signature.length));
				end -= signature.length;
				*subScriptLen -= signature.length; // Length adjustment.
			}
			// Move to next operation
			ptr += move;
		}else{
			ptr++;
		}
	}
}
CBScriptStackItem CBScriptStackCopyItem(CBScriptStack * stack, uint8_t fromTop){
	CBScriptStackItem oldItem = stack->elements[stack->length - fromTop - 1];
	CBScriptStackItem newItem;
	if (oldItem.data == NULL) { // Zero
		newItem.data = NULL;
		newItem.length = 0;
	}else{
		newItem.data = malloc(oldItem.length);
		if (NOT newItem.data){
			newItem.data = NULL;
			newItem.length = 1; // Detect not zero.
			return newItem;
		}
		newItem.length = oldItem.length;
		memmove(newItem.data, oldItem.data, newItem.length);
	}
	return newItem;
}
bool CBScriptStackEvalBool(CBScriptStack * stack){
	CBScriptStackItem item = stack->elements[stack->length-1];
	if (item.data == NULL) {
		return false;
	}
	for (uint16_t x = 0; x < item.length; x++)
		if (item.data[x] != 0){
			// Can be negative zero
			if (x == item.length - 1 && item.data[x] == 0x80)
				return false;
			else
				return true;
		}
	return false;
}
int64_t CBScriptStackItemToInt64(CBScriptStackItem item){
	if (item.data == NULL) {
		return 0;
	}
	int64_t res = 0; // May overflow 32 bits
	for (uint8_t x = 0; x < item.length; x++)
		res |= item.data[x] << (8*x);
	if (item.data[item.length-1] & 0x80) { // Negative
		// Convert signage for int64_t
		res ^= 0x80 << (8*(item.length-1)); // Removed sign bit.
		res = -res;
	}
	return res;
}
CBScriptStackItem CBScriptStackPopItem(CBScriptStack * stack){
	stack->length--;
	CBScriptStackItem item = stack->elements[stack->length];
	// Do not bother reallocing. Data will be freed at the end.
	return item;
}
bool CBScriptStackPushItem(CBScriptStack * stack, CBScriptStackItem item){
	stack->length++;
	CBScriptStackItem * temp = realloc(stack->elements, sizeof(*stack->elements)*stack->length);
	if (NOT temp)
		return false;
	stack->elements = temp;
	stack->elements[stack->length-1] = item;
	return true;
}
void CBScriptStackRemoveItem(CBScriptStack * stack){
	stack->length--;
	free(stack->elements[stack->length].data);
	// Do not bother reallocing. Data will be freed at the end.
}
CBScriptStackItem CBInt64ToScriptStackItem(CBScriptStackItem item, int64_t i){
	if (i == 0) {
		// Represented as NULL unfortunately. No other solution I'm afraid. Blame Satoshi I guess.
		free(item.data);
		item.data = NULL;
		item.length = 0;
		return item;
	}
	uint64_t ui = (i < 0)? -i : i;
	// Discover length
	for (uint8_t x = 7;; x--) {
		uint8_t b = ui >> (8*x);
		if(b){
			item.length = x + 1;
			// If byte over 0x7F add extra byte
			if (b > 0x7F)
				item.length++;
			break;
		}
		if(NOT x)
			break;
	}
	uint8_t * temp = realloc(item.data, item.length);
	if (NOT temp) {
		free(item.data);
		item.data = NULL;
		item.length = 1; // Detect err and not zero.
		return item;
	}
	item.data = temp;
	// Add data
	for (uint8_t x = 0; x < item.length; x++)
		if (x == 8)
			item.data[8] = 0; // Extra byte overflowing 8 bytes
		else
			item.data[x] = ui >> (8*x);
	// Add sign
	if (i < 0)
		item.data[item.length-1] ^= 0x80;
	return item;
}
