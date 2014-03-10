//
//  CBConstants.h
//  cbitcoin
//
//  Created by Matthew Mitchell on 28/04/2012.
//  Copyright (c) 2012 Matthew Mitchell
//  
//  This file is part of cbitcoin. It is subject to the license terms
//  in the LICENSE file found in the top-level directory of this
//  distribution and at http://www.cbitcoin.com/license.html. No part of
//  cbitcoin, including this file, may be copied, modified, propagated,
//  or distributed except according to the terms contained in the
//  LICENSE file.

#ifndef CBCONSTANTSH
#define CBCONSTANTSH

#include <setjmp.h>
#include <stdint.h>
#include <stdbool.h>

struct CBForEachData{
	unsigned pos: 31;
	unsigned enterNested: 1;
};

//  Macros

#define CB_LIBRARY_VERSION 5 // Goes up in increments
#define CB_LIBRARY_VERSION_STRING "2.0 pre-alpha" // Non-compatible.Compatibile pre-alpha/alpha/beta
#define CB_USER_AGENT_SEGMENT "/cbitcoin:2.0(pre-alpha)/"
#define CB_PRODUCTION_NETWORK_BYTES 0xD9B4BEF9 // The normal network for trading
#define CB_TEST_NETWORK_BYTES 0xDAB5BFFA // The network for testing
#define CB_ONE_BITCOIN 100000000LL // Each bitcoin has 100 million satoshis (individual units).
#define CB_HOUR 3600
#define CB_24_HOURS 24*CB_HOUR
#define CB_SCAN_START ((CB_NETWORK_TIME_ALLOWED_TIME_DRIFT + CB_BLOCK_ALLOWED_TIME_DRIFT)*2)
#define CB_NO_SCAN 0xFFFFFFFFFFFFFFFF
#define CB_MAX_MESSAGE_SIZE 0x02000000
#define CBInt16ToArray(arr, offset, i) (arr)[offset] = (uint8_t)(i); \
									 (arr)[offset + 1] = (uint8_t)((i) >> 8);
#define CBInt32ToArray(arr, offset, i) CBInt16ToArray(arr, offset, i) \
									 (arr)[offset + 2] = (uint8_t)((i) >> 16); \
							         (arr)[offset + 3] = (uint8_t)((i) >> 24);
#define CBInt64ToArray(arr, offset, i) CBInt32ToArray(arr, offset, i) \
									 (arr)[offset + 4] = (uint8_t)((uint64_t)(i) >> 32); \
									 (arr)[offset + 5] = (uint8_t)((uint64_t)(i) >> 40); \
									 (arr)[offset + 6] = (uint8_t)((uint64_t)(i) >> 48); \
									 (arr)[offset + 7] = (uint8_t)((uint64_t)(i) >> 56);
#define CBArrayToInt16(arr, offset) ((arr)[offset] \
                                     | (uint16_t)(arr)[offset + 1] << 8)
#define CBArrayToInt32(arr, offset) (CBArrayToInt16(arr, offset) \
                                     | (uint32_t)(arr)[offset + 2] << 16 \
									 | (uint32_t)(arr)[offset + 3] << 24)
#define CBArrayToInt48(arr, offset) (CBArrayToInt32(arr, offset) \
                                     | (uint64_t)(arr)[offset + 4] << 32 \
                                     | (uint64_t)(arr)[offset + 5] << 40) 
#define CBArrayToInt64(arr, offset) (CBArrayToInt48(arr, offset) \
                                     | (uint64_t)(arr)[offset + 6] << 48 \
                                     | (uint64_t)(arr)[offset + 7] << 56)
#define CBInt32ToArrayBigEndian(arr, offset, i) (arr)[offset] = (i) >> 24; \
											    (arr)[offset + 1] = (i) >> 16;\
											    (arr)[offset + 2] = (i) >> 8; \
											    (arr)[offset + 3] = (i);
#define CBInt64ToArrayBigEndian(arr, offset, i) CBInt32ToArrayBigEndian(arr, offset + 4, i) \
												(arr)[offset + 0] = (uint8_t)((uint64_t)(i) >> 56); \
												(arr)[offset + 1] = (uint8_t)((uint64_t)(i) >> 48);\
												(arr)[offset + 2] = (uint8_t)((uint64_t)(i) >> 40); \
												(arr)[offset + 3] = (uint8_t)((uint64_t)(i) >> 32);
#define CBArrayToInt32BigEndian(arr, offset) ((arr)[offset] << 24 \
                                              | (uint16_t)(arr)[offset + 1] << 16 \
                                              | (uint32_t)(arr)[offset + 2] << 8 \
                                              | (uint32_t)(arr)[offset + 3])
#define CBArrayToInt64BigEndian(arr, offset) (CBArrayToInt32BigEndian(arr, offset + 4) \
											  | (uint64_t)(arr)[offset] << 56 \
											  | (uint64_t)(arr)[offset + 1] << 48 \
											  | (uint64_t)(arr)[offset + 2] << 40 \
											  | (uint64_t)(arr)[offset + 3] << 32)
#define CBForEach(el, arr) \
	for (struct CBForEachData s = {0, true}; s.enterNested && sizeof(arr) && sizeof(arr[0])*(s.pos+1) <= sizeof(arr) ; s.enterNested = !s.enterNested, s.pos++) \
		for (el = arr[s.pos];s.enterNested;s.enterNested = !s.enterNested)

//  Enums

typedef enum{
	CB_COMPARE_MORE_THAN = 1,
	CB_COMPARE_EQUAL = 0,
	CB_COMPARE_LESS_THAN = -1,
}CBCompare;

/**
 @brief Enum for functions returing a boolean value but with an additional value for errors.
*/
typedef enum{
	CB_FALSE,
	CB_TRUE,
	CB_ERROR,
} CBErrBool;

typedef enum{
	CB_NETWORK_PRODUCTION,
	CB_NETWORK_TEST,
	CB_NETWORK_UNKNOWN
} CBNetwork;

typedef enum{
	CB_PREFIX_PRODUCTION_ADDRESS = 0,
	CB_PREFIX_TEST_ADDRESS = 111,
	CB_PREFIX_PRODUCTION_PRIVATE_KEY = 128,
	CB_PREFIX_TEST_PRIVATE_KEY = 239
} CBBase58Prefix;

typedef enum{
	CB_ERROR_UNREACHABLE,
	CB_ERROR_NO_PEERS,
	CB_ERROR_BAD_TIME,
	CB_ERROR_VALIDATION,
	CB_ERROR_GENERAL,
} CBErrorReason;

#endif
