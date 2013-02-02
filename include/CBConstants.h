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

#define CB_LIBRARY_VERSION 5 // Goes up in increments
#define CB_LIBRARY_VERSION_STRING "2.0 pre-alpha" // Features.Non-features pre-alpha/alpha/beta
#define CB_USER_AGENT_SEGMENT "/cbitcoin:2.0(pre-alpha)/"
#define CB_PRODUCTION_NETWORK_BYTE 0 // The normal network for trading
#define CB_TEST_NETWORK_BYTE 111 // The network for testing
#define CB_PRODUCTION_NETWORK_BYTES 0xD9B4BEF9 // The normal network for trading
#define CB_TEST_NETWORK_BYTES 0xDAB5BFFA // The network for testing
#define CB_ONE_BITCOIN 100000000LL // Each bitcoin has 100 million satoshis (individual units).
#define CB_24_HOURS 86400
#define NOT ! // Better readability than !
#define CBInt16ToArray(arr, offset, i) arr[offset] = i; \
									 arr[offset + 1] = (i) >> 8;
#define CBInt32ToArray(arr, offset, i) CBInt16ToArray(arr, offset, i) \
									 arr[offset + 2] = (i) >> 16; \
							         arr[offset + 3] = (i) >> 24;
#define CBInt64ToArray(arr, offset, i) CBInt32ToArray(arr, offset, i) \
									 arr[offset + 4] = (i) >> 32; \
									 arr[offset + 5] = (i) >> 40; \
									 arr[offset + 6] = (i) >> 48; \
									 arr[offset + 7] = (i) >> 56;
#define CBArrayToInt16(arr, offset) (arr[offset] \
                                     | (uint16_t)arr[offset + 1] << 8)
#define CBArrayToInt32(arr, offset) (CBArrayToInt16(arr, offset) \
                                     | (uint32_t)arr[offset + 2] << 16 \
									 | (uint32_t)arr[offset + 3] << 24)
#define CBArrayToInt48(arr, offset) (CBArrayToInt32(arr, offset) \
                                     | (uint64_t)arr[offset + 4] << 32 \
                                     | (uint64_t)arr[offset + 5] << 40) 
#define CBArrayToInt64(arr, offset) (CBArrayToInt48(arr, offset) \
                                     | (uint64_t)arr[offset + 6] << 48 \
                                     | (uint64_t)arr[offset + 7] << 56)
#define CBInt32ToArrayBigEndian(arr, offset, i) arr[offset] = (i) >> 24; \
										      arr[offset + 1] = (i) >> 16;\
                                              arr[offset + 2] = (i) >> 8; \
                                              arr[offset + 3] = (i);
#define CBArrayToInt32BigEndian(arr, offset) (arr[offset] << 24 \
                                              | (uint16_t)arr[offset + 1] << 16 \
                                              | (uint32_t)arr[offset + 2] << 8 \
                                              | (uint32_t)arr[offset + 3])

//  Enums

typedef enum{
	CB_COMPARE_MORE_THAN = 1, 
	CB_COMPARE_EQUAL = 0, 
	CB_COMPARE_LESS_THAN = -1, 
}CBCompare;

#endif
