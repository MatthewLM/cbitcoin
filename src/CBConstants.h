//
//  CBConstants.h
//  cbitcoin
//
//  Created by Matthew Mitchell on 28/04/2012.
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

#ifndef CBCONSTANTSH
#define CBCONSTANTSH

//  Macros

#define CB_MESSAGE_MAX_SIZE 0x02000000 // 32 MB
#define CB_BLOCK_ALLOWED_TIME_DRIFT 7200 // 2 Hours from network time
#define CB_BLOCK_MAX_SIZE 1000000 // ~0.95 MB
#define CB_LIBRARY_VERSION 1 // Goes up in increments
#define CB_LIBRARY_VERSION_STRING "1.0 alpha" // Features.Fixes alpha/beta
#define CB_USER_AGENT_SEGMENT "cbitcoin:1.0(alpha)/"
#define CB_PRODUCTION_NETWORK 0 // The normal network for trading
#define CB_TEST_NETWORK 111 // The network for testing
#define CB_TRANSACTION_INPUT_FINAL 0xFFFFFFFF // Transaction input is final
#define CB_TRANSACTION_INPUT_OUT_POINTER_MESSAGE_LENGTH 36
#define CB_OUTPUT_VALUE_MINUS_ONE 0xFFFFFFFFFFFFFFFF // In twos complement it represents -1. Bitcoin uses twos compliment.
#define CB_MAX_MONEY 2100000000000000 // 21 million Bitcoins. Each bitcoin has 100 million satoshis (individual units).
#define CB_SERVICE_FULL_BLOCKS 1 // Service for full blocks. Node maintains the entire blockchain.

//  Enums

typedef enum{
	CB_ERROR_MESSAGE_LENGTH_NOT_SET,
	CB_ERROR_MESSAGE_CHECKSUM_BAD_SIZE,
	CB_ERROR_MESSAGE_NO_SERIALISATION_IMPLEMENTATION,
	CB_ERROR_MESSAGE_NO_DESERIALISATION_IMPLEMENTATION,
	CB_ERROR_MESSAGE_DESERIALISATION_BAD_BYTES,
	CB_ERROR_MESSAGE_DESERIALISATION_NULL_BYTES,
	CB_ERROR_MESSAGE_SERIALISATION_BAD_BYTES,
	CB_ERROR_MESSAGE_SERIALISATION_NULL_BYTES,
	CB_ERROR_MESSAGE_SERIALISATION_BAD_DATA,
	CB_ERROR_SHA_256_HASH_BAD_BYTE_ARRAY_LENGTH,
	CB_ERROR_BASE58_DECODE_CHECK_TOO_SHORT,
	CB_ERROR_BASE58_DECODE_CHECK_INVALID,
	CB_ERROR_TRANSACTION_FEW_INPUTS,
	CB_ERROR_TRANSACTION_FEW_OUTPUTS,
}CBError;

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
    CB_SCRIPT_OP_SMALLINTEGER = 0xfa,
    CB_SCRIPT_OP_PUBKEYS = 0xfb,
    CB_SCRIPT_OP_PUBKEYHASH = 0xfd,
    CB_SCRIPT_OP_PUBKEY = 0xfe,
    CB_SCRIPT_OP_INVALIDOPCODE = 0xff,
}CBScriptOp;

typedef enum{
	CB_SIGHASH_ALL = 0x00000001,
	CB_SIGHASH_NONE = 0x00000002,
	CB_SIGHASH_SINGLE = 0x00000003,
	CB_SIGHASH_ANYONECANPAY = 0x00000080,
}CBSignType;

typedef enum{
	CB_COMPARE_MORE_THAN = 1,
	CB_COMPARE_EQUAL = 0,
	CB_COMPARE_LESS_THAN = -1,
}CBCompare; 

#endif
