//
//  CBScript.h
//  cbitcoin
//
//  Created by Matthew Mitchell on 02/05/2012.
//  Copyright (c) 2012 Matthew Mitchell
//  
//  This file is part of cbitcoin. It is subject to the license terms
//  in the LICENSE file found in the top-level directory of this
//  distribution and at http://www.cbitcoin.com/license.html. No part of
//  cbitcoin, including this file, may be copied, modified, propagated,
//  or distributed except according to the terms contained in the
//  LICENSE file.

/**
 @file
 @brief Functions for bitcoin scripts which can be processed to determine if a transaction input is valid. A good resource is https://en.bitcoin.it/wiki/Script A CBScript object is an alias to a CBByteArray object.
*/

#ifndef CBSCRIPTH
#define CBSCRIPTH

//  Includes

#include "CBByteArray.h"
#include "CBDependencies.h"
#include "CBHDKeys.h"
#include <stdbool.h>

// Constants and Macros

#define CB_NOT_A_NUMBER_OP UINT8_MAX
#define CB_NOT_A_PUSH_OP UINT32_MAX
#define CBGetScript(x) ((CBScript *)x)

typedef enum{
	CB_SIGHASH_ALL = 0x00000001,
	CB_SIGHASH_NONE = 0x00000002,
	CB_SIGHASH_SINGLE = 0x00000003,
	CB_SIGHASH_ANYONECANPAY = 0x00000080,
}CBSignType;

/*
 @brief The return values for CBScriptExecute. @see CBScript.h
 */
typedef enum{
	CB_SCRIPT_TRUE, /**< Script validates as true. */
	CB_SCRIPT_FALSE, /**< Script validates as false. */
	CB_SCRIPT_INVALID, /**< Script does not validate */
} CBScriptExecuteReturn;

typedef enum{
	CB_SCRIPT_OP_0 = 0x00,
    CB_SCRIPT_OP_FALSE = CB_SCRIPT_OP_0,
    CB_SCRIPT_OP_PUSHDATA1 = 0x4c,
    CB_SCRIPT_OP_PUSHDATA2 = 0x4d,
    CB_SCRIPT_OP_PUSHDATA4 = 0x4e,
    CB_SCRIPT_OP_1NEGATE = 0x4f,
    CB_SCRIPT_OP_RESERVED = 0x50,
    CB_SCRIPT_OP_1 = 0x51,
    CB_SCRIPT_OP_TRUE = CB_SCRIPT_OP_1,
    CB_SCRIPT_OP_2 = 0x52,
    CB_SCRIPT_OP_3 = 0x53,
    CB_SCRIPT_OP_4 = 0x54,
    CB_SCRIPT_OP_5 = 0x55,
    CB_SCRIPT_OP_6 = 0x56,
    CB_SCRIPT_OP_7 = 0x57,
    CB_SCRIPT_OP_8 = 0x58,
    CB_SCRIPT_OP_9 = 0x59,
    CB_SCRIPT_OP_10 = 0x5a,
    CB_SCRIPT_OP_11 = 0x5b,
    CB_SCRIPT_OP_12 = 0x5c,
    CB_SCRIPT_OP_13 = 0x5d,
    CB_SCRIPT_OP_14 = 0x5e,
    CB_SCRIPT_OP_15 = 0x5f,
    CB_SCRIPT_OP_16 = 0x60,
    CB_SCRIPT_OP_NOP = 0x61,
    CB_SCRIPT_OP_VER = 0x62,
    CB_SCRIPT_OP_IF = 0x63,
    CB_SCRIPT_OP_NOTIF = 0x64,
    CB_SCRIPT_OP_VERIF = 0x65,
    CB_SCRIPT_OP_VERNOTIF = 0x66,
    CB_SCRIPT_OP_ELSE = 0x67,
    CB_SCRIPT_OP_ENDIF = 0x68,
    CB_SCRIPT_OP_VERIFY = 0x69,
    CB_SCRIPT_OP_RETURN = 0x6a,
    CB_SCRIPT_OP_TOALTSTACK = 0x6b,
    CB_SCRIPT_OP_FROMALTSTACK = 0x6c,
    CB_SCRIPT_OP_2DROP = 0x6d,
    CB_SCRIPT_OP_2DUP = 0x6e,
    CB_SCRIPT_OP_3DUP = 0x6f,
    CB_SCRIPT_OP_2OVER = 0x70,
    CB_SCRIPT_OP_2ROT = 0x71,
    CB_SCRIPT_OP_2SWAP = 0x72,
    CB_SCRIPT_OP_IFDUP = 0x73,
    CB_SCRIPT_OP_DEPTH = 0x74,
    CB_SCRIPT_OP_DROP = 0x75,
    CB_SCRIPT_OP_DUP = 0x76,
    CB_SCRIPT_OP_NIP = 0x77,
    CB_SCRIPT_OP_OVER = 0x78,
    CB_SCRIPT_OP_PICK = 0x79,
    CB_SCRIPT_OP_ROLL = 0x7a,
    CB_SCRIPT_OP_ROT = 0x7b,
    CB_SCRIPT_OP_SWAP = 0x7c,
    CB_SCRIPT_OP_TUCK = 0x7d,
    CB_SCRIPT_OP_CAT = 0x7e, // Disabled
    CB_SCRIPT_OP_SUBSTR = 0x7f, // Disabled
    CB_SCRIPT_OP_LEFT = 0x80, // Disabled
    CB_SCRIPT_OP_RIGHT = 0x81, // Disabled
    CB_SCRIPT_OP_SIZE = 0x82,
    CB_SCRIPT_OP_INVERT = 0x83, // Disabled
    CB_SCRIPT_OP_AND = 0x84, // Disabled
    CB_SCRIPT_OP_OR = 0x85, // Disabled
    CB_SCRIPT_OP_XOR = 0x86, // Disabled
    CB_SCRIPT_OP_EQUAL = 0x87,
    CB_SCRIPT_OP_EQUALVERIFY = 0x88,
    CB_SCRIPT_OP_RESERVED1 = 0x89,
    CB_SCRIPT_OP_RESERVED2 = 0x8a,
    CB_SCRIPT_OP_1ADD = 0x8b,
    CB_SCRIPT_OP_1SUB = 0x8c,
    CB_SCRIPT_OP_2MUL = 0x8d, // Disabled
    CB_SCRIPT_OP_2DIV = 0x8e, // Disabled
    CB_SCRIPT_OP_NEGATE = 0x8f,
    CB_SCRIPT_OP_ABS = 0x90,
    CB_SCRIPT_OP_NOT = 0x91,
    CB_SCRIPT_OP_0NOTEQUAL = 0x92,
    CB_SCRIPT_OP_ADD = 0x93,
    CB_SCRIPT_OP_SUB = 0x94,
    CB_SCRIPT_OP_MUL = 0x95, // Disabled
    CB_SCRIPT_OP_DIV = 0x96, // Disabled
    CB_SCRIPT_OP_MOD = 0x97, // Disabled
    CB_SCRIPT_OP_LSHIFT = 0x98, // Disabled
    CB_SCRIPT_OP_RSHIFT = 0x99, // Disabled
    CB_SCRIPT_OP_BOOLAND = 0x9a,
    CB_SCRIPT_OP_BOOLOR = 0x9b,
    CB_SCRIPT_OP_NUMEQUAL = 0x9c,
    CB_SCRIPT_OP_NUMEQUALVERIFY = 0x9d,
    CB_SCRIPT_OP_NUMNOTEQUAL = 0x9e,
    CB_SCRIPT_OP_LESSTHAN = 0x9f,
    CB_SCRIPT_OP_GREATERTHAN = 0xa0,
    CB_SCRIPT_OP_LESSTHANOREQUAL = 0xa1,
    CB_SCRIPT_OP_GREATERTHANOREQUAL = 0xa2,
    CB_SCRIPT_OP_MIN = 0xa3,
    CB_SCRIPT_OP_MAX = 0xa4,
    CB_SCRIPT_OP_WITHIN = 0xa5,
    CB_SCRIPT_OP_RIPEMD160 = 0xa6,
    CB_SCRIPT_OP_SHA1 = 0xa7,
    CB_SCRIPT_OP_SHA256 = 0xa8,
    CB_SCRIPT_OP_HASH160 = 0xa9,
    CB_SCRIPT_OP_HASH256 = 0xaa,
    CB_SCRIPT_OP_CODESEPARATOR = 0xab,
    CB_SCRIPT_OP_CHECKSIG = 0xac,
    CB_SCRIPT_OP_CHECKSIGVERIFY = 0xad,
    CB_SCRIPT_OP_CHECKMULTISIG = 0xae,
    CB_SCRIPT_OP_CHECKMULTISIGVERIFY = 0xaf,
    CB_SCRIPT_OP_NOP1 = 0xb0,
    CB_SCRIPT_OP_NOP2 = 0xb1,
    CB_SCRIPT_OP_NOP3 = 0xb2,
    CB_SCRIPT_OP_NOP4 = 0xb3,
    CB_SCRIPT_OP_NOP5 = 0xb4,
    CB_SCRIPT_OP_NOP6 = 0xb5,
    CB_SCRIPT_OP_NOP7 = 0xb6,
    CB_SCRIPT_OP_NOP8 = 0xb7,
    CB_SCRIPT_OP_NOP9 = 0xb8,
    CB_SCRIPT_OP_NOP10 = 0xb9,
    CB_SCRIPT_OP_INVALIDOPCODE = 0xff,
}CBScriptOp;

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
CBScript * CBNewScriptFromReference(CBByteArray * program, uint32_t offset, uint32_t len);
/**
 @brief Creates a new CBScript object with a given size.
 @returns A new CBScript object.
 */
CBScript * CBNewScriptOfSize(uint32_t size);
/**
 @brief Creates a new CBScript object from a string. The script text should follow the following Backus–Naur Form:
 /code
 <digit> ::= "0" | "1" | "2" | "3" | "4" | "5" | "6" | "7" | "8" | "9" | "a" | "b" | "c" | "d" | "e" | "f" | "A" | "B" | "C" | "D" | "E" | "F"
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
 @returns A new CBScript object or NULL on failure.
 */
CBScript * CBNewScriptFromString(char * string);
CBScript * CBNewScriptMultisigOutput(uint8_t ** pubKeys, uint8_t m, uint8_t n);
CBScript * CBNewScriptP2SHOutput(CBScript * script);
CBScript * CBNewScriptPubKeyHashOutput(uint8_t * pubKeyHash);
CBScript * CBNewScriptPubKeyOutput(uint8_t * pubKey);
/**
 @brief Creates a new CBScript using data.
 @param data The data. This should be dynamically allocated. The new CBByteArray object will take care of it's memory management so do not free this data once passed into this constructor.
 @param size Size in bytes for the new array.
 @returns The new CBScript object.
 */
CBScript * CBNewScriptWithData(uint8_t * data, uint32_t size);
/**
 @brief Creates a new CBScript using data which is copied.
 @param data The data. This data is copied.
 @param size Size in bytes for the new array.
 @returns The new CBScript object.
 */
CBScript * CBNewScriptWithDataCopy(uint8_t * data, uint32_t size);

/**
 @brief Initialises a CBScript object from a string. @see CBNewScriptOfSize
 @param self The CBScript object to initialise
 @param params The network parameters.
 @param string The string to compile into a CBScript object.
 @returns true on success, false on failure.
 */
bool CBInitScriptFromString(CBScript * self, char * string);
void CBInitScriptMultisigOutput(CBScript * self, uint8_t ** pubKeys, uint8_t m, uint8_t n);
void CBInitScriptP2SHOutput(CBScript * self, CBScript *  script);
void CBInitScriptPubKeyHashOutput(CBScript * self, uint8_t * pubKeyHash);
void CBInitScriptPubKeyOutput(CBScript * self, uint8_t * pubKey);

/**
 @brief Release and free all of the objects stored by the CBScript object.
 @param self The CBScript object to destroy.
 */
void CBDestroyScript(void * self);
/**
 @brief Frees a CBScript object and also calls CBDestroyScript.
 @param self The CBScript object to free.
 */
void CBFreeScript(void * self);
 
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
CBScriptExecuteReturn CBScriptExecute(CBScript * self, CBScriptStack * stack, bool (*getHashForSig)(void *, CBByteArray *, uint32_t, CBSignType, uint8_t *), void * transaction, uint32_t inputIndex, bool p2sh);
uint8_t CBScriptGetLengthOfPushOp(uint32_t dataLen);
/**
 @brief Gets the amount being pushed from a script op at a given offset.
 @param self The script object.
 @param offset A pointer to the offset of the push operation. The offset will be moved past the end of the push operation.
 @retuns The push amount or CB_NOT_A_PUSH_OP (inc. when invalid).
 */
uint32_t CBScriptGetPushAmount(CBScript * self, uint32_t * offset);
/**
 @brief Returns the number of sigops.
 @param self The CBScript object.
 @param inP2SH true when getting sigops for a P2SH script.
 @retuns the number of sigops as used for validation.
 */
uint32_t CBScriptGetSigOpCount(CBScript * self, bool inP2SH);
/**
 @brief Determines if a script object matches the public-key hash verification template. 
 @param self The CBScript object.
 @retuns true if the script matches the template, false otherwise.
 */
bool CBScriptIsKeyHash(CBScript * self);
/**
 @brief Determines if a script object matches the multisignature template.
 @param self The CBScript object.
 @retuns true if the script matches the template, false otherwise.
 */
bool CBScriptIsMultisig(CBScript * self);
/**
 @brief Determines if a script object matches the P2SH template.
 @param self The CBScript object.
 @retuns true if the script matches the P2SH template, false otherwise.
 */
bool CBScriptIsP2SH(CBScript * self);
/**
 @brief Determines if a script object matches the public-key verification template.
 @param self The CBScript object.
 @retuns true if the script matches the public key template, false otherwise.
 */
bool CBScriptIsPubkey(CBScript * self);
/**
 @brief Determines if a script has only push operations.
 @param self The CBScript object.
 @retuns The number of push operations found if the script has only push operations, 0 otherwise. Also returns 0 when an invalid push operation if found.
 */
uint16_t CBScriptIsPushOnly(CBScript * self);
/**
 @brief Gets the number of a script operation number, which can be OP_0 or OP_1 to OP_16.
 @param op The operation.
 @retuns The number or CB_NOT_A_NUMBER_OP.
 */
uint8_t CBScriptOpGetNumber(CBScriptOp op);
/**
 @brief Removes occurances of a signature from script data
 @param subScript The sub script to remove signatures from.
 @param subScriptLen A pointer to the length of the sub script. The length will be modified to the new length.
 @param signature The signature to be found and removed.
 */
void CBSubScriptRemoveSignature(uint8_t * subScript, uint32_t * subScriptLen, CBScriptStackItem signature);
/**
 @brief Returns a copy of a stack item, "fromTop" from the top.
 @param stack A pointer to the stack.
 @param fromTop Number of items from the top to copy.
 @returns A copy of the stack item which should be freed.
 */
CBScriptStackItem CBScriptStackCopyItem(CBScriptStack * stack, uint8_t fromTop);
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
void CBScriptStackPushItem(CBScriptStack * stack, CBScriptStackItem item);
/**
 @brief Removes top item from the stack.
 @param stack A pointer to the stack to remove the data.
 */
void CBScriptStackRemoveItem(CBScriptStack * stack);
uint32_t CBScriptStringMaxSize(CBScript * self);
void CBScriptToString(CBScript * self, char * output);
void CBScriptWritePushOp(CBScript * self, uint32_t offset, uint8_t * data, uint32_t dataLen);
/**
 @brief Converts a int64_t to a CBScriptStackItem
 @param item Pass in a CBScriptStackItem for reallocating data.
 @param i The 64 bit signed integer.
 @returns A CBScriptStackItem.
 */
CBScriptStackItem CBInt64ToScriptStackItem(CBScriptStackItem item, int64_t i);

#endif
