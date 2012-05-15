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

#define CB_BYTE_ARRAY_UNKNOWN_LENGTH -1
#define CBFree()    objectNum--; \
					if (objectNum < 1) { \
						free(VTStore); \
						VTStore = NULL; \
					}
#define CB_MESSAGE_MAX_SIZE 0x02000000
#define CB_BLOCK_HEADER_SIZE 80
#define CB_BLOCK_ALLOWED_TIME_DRIFT 7200 // 2 Hours
#define CB_PROTOCOL_VERSION 31800 // Version 0.3.18.00
#define CB_PRODUCTION_NETWORK "org.bitcoin.production" // The normal network for trading
#define CB_TEST_NETWORK "org.bitcoin.test" // The network for testing
#define CB_TRANSACTION_INPUT_FINAL 0xFFFFFFFF // Transaction input is final
#define CB_TRANSACTION_INPUT_OUT_POINTER_MESSAGE_LENGTH 36

//  Enums

typedef enum{
	CB_ERROR_MESSAGE_LENGTH_NOT_SET,
	CB_ERROR_MESSAGE_CHECKSUM_BAD_SIZE,
	CB_ERROR_MESSAGE_NO_SERIALISATION_IMPLEMENTATION,
	CB_ERROR_SHA_256_HASH_BAD_BYTE_ARRAY_LENGTH,
	CB_ERROR_BASE58_DECODE_CHECK_TOO_SHORT,
	CB_ERROR_BASE58_DECODE_CHECK_INVALID,
}CBError;

typedef enum{
	CB_SCRIPT_OP_0 = 0,
	CB_SCRIPT_OP_PUSHDATA1 = 76,
	CB_SCRIPT_OP_PUSHDATA2 = 77,
	CB_SCRIPT_OP_PUSHDATA4 = 78,
	CB_SCRIPT_OP_DUP = 118,
    CB_SCRIPT_OP_HASH160 = 169,
    CB_SCRIPT_OP_EQUALVERIFY = 136,
    CB_SCRIPT_OP_CHECKSIG = 172,
}CBScriptOp;

typedef enum{
	CB_COMPARE_MORE_THAN = 1,
	CB_COMPARE_EQUAL = 0,
	CB_COMPARE_LESS_THAN = -1,
}CBCompare;

#endif
