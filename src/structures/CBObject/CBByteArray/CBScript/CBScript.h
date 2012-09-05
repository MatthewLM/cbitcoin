//
//  CBScript.h
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

/**
 @file
 @brief Functions for bitcoin scripts which can be processed to determine if a transaction input is valid. A good resource is https://en.bitcoin.it/wiki/Script A CBScript object is an alias to a CBByteArray object.
*/

#ifndef CBSCRIPTH
#define CBSCRIPTH

//  Includes

#include "CBByteArray.h"
#include "CBDependencies.h"
#include <stdbool.h>

/**
 @brief Structure for a stack item
 */

typedef struct{
	uint8_t * data; /**< Data for this stack item */
	uint16_t length; /**< Length of this item */
} CBScriptStackItem;

/**
 @brief Structure that holds byte data in a stack.
 */

typedef struct{
	CBScriptStackItem * elements; /**< Elements in the stack */
	uint16_t length; /**< Length of the stack */
} CBScriptStack;

typedef CBByteArray CBScript;

/**
 @brief Creates a new CBScript object.
 @returns A new CBScript object.
 */
CBScript * CBNewScriptFromReference(CBByteArray * program,uint32_t offset,uint32_t len);
/**
 @brief Creates a new CBScript object with a given size.
 @returns A new CBScript object.
 */
CBScript * CBNewScriptOfSize(uint32_t size,CBEvents * events);
/**
 @brief Creates a new CBScript object from a string. The script text should follow the following Backus–Naur Form:
 /code
 <digit> = "0" | "1" | "2" | "3" | "4" | "5" | "6" | "7" | "8" | "9" | "a" | "b" | "c" | "d" | "e" | "f" | "A" | "B" | "C" | "D" | "E" | "F"
 <hex_digits> ::= <digit> | <digit> <hex_digits>
 <hex> ::= "0x" <hex_digits>
 <operation_name> = "0" | "FALSE" | "1NEGATE" | "RESERVED" | "1" | "TRUE" | "2" | "3" | "4" |  "5" | "6" | "7" | "8" | "9" | "10" | "11" | "12" | "13" | "14" | "15" | "16" | "NOP" | "VER" | "IF" | "NOTIF" | "VERIF" | "VERNOTIF" | "ELSE" | "ENDIF" | "VERIFY" | "RETURN" | "TOALTSTACK" | "FROMALTSTACK" | "2DROP" | "2DUP" | "3DUP" | "2OVER" | "2ROT" | "2SWAP" | "IFDUP" | "DEPTH" | "DROP" | "DUP" | "NIP" | "OVER" | "PICK" | "ROLL" | "ROT" | "SWAP" | "TUCK" | "CAT" | "SUBSTR" | "LEFT" | "RIGHT = 0x81" | "SIZE" | "INVERT" | "AND" | "OR" | "XOR" | "EQUAL" | "EQUALVERIFY" | "RESERVED1" | "RESERVED2" | "1ADD" | "1SUB" | "2MUL" | "2DIV" | "NEGATE" | "ABS" | "NOT" | "0NOTEQUAL" | "ADD" | "SUB" | "MUL" | "DIV" | "MOD" | "LSHIFT " | "RSHIFT" | "BOOLAND" | "BOOLOR" | "NUMEQUAL" | "NUMEQUALVERIFY" | "NUMNOTEQUAL" | "LESSTHAN" | "GREATERTHAN" | "LESSTHANOREQUAL" | "GREATERTHANOREQUAL" | "MIN" | "MAX" | "WITHIN" | "RIPEMD160" | "SHA1" | "SHA256" | "HASH160" | "HASH256" | "CODESEPARATOR" | "CHECKSIG" | "CHECKSIGVERIFY" | "CHECKMULTISIG" | "CHECKMULTISIGVERIFY" | "NOP1" | "NOP2" | "NOP3" | "NOP4" | "NOP5" | "NOP6" | "NOP7" | "NOP8" | "NOP9" | "NOP10"
 <operation> ::= "OP_" <operation_name>
 <lines> ::= "\n" | "\n" <lines>
 <seperator> ::= <lines> | " "
 <operation_or_hex> ::= <operation> | <hex>
 <seperated_operation_or_hex> ::= <operation_or_hex> <seperator>
 <script_body> ::= <seperated_operation_or_hex> | <seperated_operation_or_hex> <script_body> | ""
 <lines_or_none> ::= <lines> | ""
 <operation_or_hex_or_none> ::= <operation_or_hex> | ""
 <script> ::= <lines_or_none> <script_body> <operation_or_hex_or_none> 
 /endcode
 Be warned that the scripts do not use two's compliment for hex values. To represent a negative number the far left digit should be 8 or higher. Adding 8 to the far left digit give the number a negative sign such that 0x81 = -1. Negative numbers should have an even number of digits. 0x8FF will not give -255, instead 0x80FF is needed. 
 @param params The network parameters.
 @param string The string to compile into a CBScript object.
 @param events The CBEvents structure.
 @returns A new CBScript object or NULL on failure.
 */
CBScript * CBNewScriptFromString(char * string,CBEvents * events);
/**
 @brief Creates a new CBScript using data.
 @param data The data. This should be dynamically allocated. The new CBByteArray object will take care of it's memory management so do not free this data once passed into this constructor.
 @param size Size in bytes for the new array.
 @param events CBEngine for errors.
 @returns The new CBScript object.
 */
CBScript * CBNewScriptWithData(uint8_t * data,uint32_t size,CBEvents * events);
/**
 @brief Creates a new CBScript using data which is copied.
 @param data The data. This data is copied.
 @param size Size in bytes for the new array.
 @param events CBEngine for errors.
 @returns The new CBScript object.
 */
CBScript * CBNewScriptWithDataCopy(uint8_t * data,uint32_t size,CBEvents * events);

/**
 @brief Gets a CBScript from another object. Use this to avoid casts.
 @param self The object to obtain the CBScript from.
 @returns The CBScript object.
 */
CBScript * CBGetScript(void * self);

/**
 @brief Initialises a CBScript object from a string. @see CBNewScriptOfSize
 @param self The CBScript object to initialise
 @param params The network parameters.
 @param string The string to compile into a CBScript object.
 @param events The CBEvents structure.
 @returns true on success, false on failure.
 */
bool CBInitScriptFromString(CBScript * self,char * string,CBEvents * events);

/**
 @brief Does the processing to free a CBScript object. Should be called by the children when freeing objects.
 @param self The CBScript object to free.
 */
void CBFreeProcessScript(CBScript * self);
 
//  Functions

/**
 @brief Frees a CBScriptStack
 @param stack The stack to free
 */
void CBFreeScriptStack(CBScriptStack stack);
/**
 @brief Returns a new empty stack.
 @returns The new empty stack.
 */
CBScriptStack CBNewEmptyScriptStack(void);
/**
 @brief Executes a bitcoin script.
 @param self The CBScript object with the program
 @param stack A pointer to the input stack for the program.
 @param getHashForSig A pointer to the funtion to get the hash for checking the signature. Should take a CBTransaction object, input index, CBSignType and the CBDependencies object.
 @param transaction Transaction for checking the signatures.
 @param inputIndex The index of the input for the signature.
 @param p2sh If false, do not allow any P2SH matches.
 @returns CB_SCRIPT_VALID is the program ended with true, CB_SCRIPT_INVALID on script failure or CB_SCRIPT_ERR if an error occured with the interpreter such as running of of memory.
 */
CBScriptExecuteReturn CBScriptExecute(CBScript * self,CBScriptStack * stack,CBGetHashReturn (*getHashForSig)(void *, CBByteArray *, uint32_t, CBSignType, uint8_t *),void * transaction,uint32_t inputIndex,bool p2sh);
/**
 @brief Returns the number of sigops.
 @param self The CBScript object.
 @param inP2SH true when getting sigops for a P2SH script.
 @retuns the number of sigops as used for validation.
 */
uint32_t CBScriptGetSigOpCount(CBScript * self, bool inP2SH);
/**
 @brief Determines if a script object matches the P2SH template.
 @param self The CBScript object.
 @retuns true if the script matches the P2SH template, false otherwise.
 */
bool CBScriptIsP2SH(CBScript * self);
/**
 @brief Removes occurances of a signature from script data
 @param subScript The sub script to remove signatures from.
 @param subScriptLen A pointer to the length of the sub script. The length will be modified to the new length.
 @param signature The signature to be found and removed.
 */
void CBSubScriptRemoveSignature(uint8_t * subScript,uint32_t * subScriptLen,CBScriptStackItem signature);
/**
 @brief Returns a copy of a stack item, "fromTop" from the top.
 @param stack A pointer to the stack.
 @param fromTop Number of items from the top to copy.
 @returns A copy of the stack item which should be freed.
 */
CBScriptStackItem CBScriptStackCopyItem(CBScriptStack * stack,uint8_t fromTop);
/**
 @brief Evaluates the top stack item as a bool. False if 0 or -0.
 @param stack The stack.
 @returns The boolean result.
 */
bool CBScriptStackEvalBool(CBScriptStack * stack);
/**
 @brief Converts a CBScriptStackItem to an int64_t
 @param item The CBScriptStackItem to convert
 @returns A 64 bit signed integer.
 */
int64_t CBScriptStackItemToInt64(CBScriptStackItem item);
/**
 @brief Removes the top item from the stack and returns it.
 @param stack A pointer to the stack to pop the data.
 @returns The top item. This must be freed.
 */
CBScriptStackItem CBScriptStackPopItem(CBScriptStack * stack);
/**
 @brief Push data onto the stack which is freed by the stack.
 @param stack A pointer to the stack to push data onto.
 @param data The item to push on the stack.
 */
bool CBScriptStackPushItem(CBScriptStack * stack,CBScriptStackItem item);
/**
 @brief Removes top item from the stack.
 @param stack A pointer to the stack to remove the data.
 */
void CBScriptStackRemoveItem(CBScriptStack * stack);
/**
 @brief Converts a int64_t to a CBScriptStackItem
 @param item Pass in a CBScriptStackItem for reallocating data.
 @param i The 64 bit signed integer.
 @returns A CBScriptStackItem.
 */
CBScriptStackItem CBInt64ToScriptStackItem(CBScriptStackItem item,int64_t i);

#endif
