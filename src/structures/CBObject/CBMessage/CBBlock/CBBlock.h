//
//  CBBlock.h
//  cbitcoin
//
//  Created by Matthew Mitchell on 01/05/2012.
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
 @brief Structure that holds a bitcoin block. Blocks contain transaction information and use a proof of work system to show that they are legitimate. Inherits CBMessage
*/

#ifndef CBBLOCKH
#define CBBLOCKH

//  Includes

#include "CBTransaction.h"

/**
 @brief Structure for CBBlock objects. @see CBBlock.h
*/
typedef struct{
	CBMessage base; /**< CBMessage base structure */
	CBByteArray * hash; /**< The hash for this block. NULL if it needs to be calculated or set. */
	u_int32_t version;
	CBByteArray * prevBlockHash; /**< The previous block hash. */
	CBByteArray * merkleRoot; /**< The merkle tree root hash. */
	u_int32_t time; /**< Timestamp for the block. The network uses 32 bits. The protocol can be future proofed by detecting overflows when going through the blocks. So if a block's time overflows such that the time is less than the median of the last 10 blocks, the block can be seen by adding the first 32 bits of the network time and finally the timestamp can be tested against the network time. The overflow problem can therefore be fixed by a workaround but it is a shame Satoshi did not use 64 bits. */
	u_int32_t difficulty; /**< The calculated difficulty for this block. */
	u_int32_t nounce; /**< Nounce used in generating the block. */
	u_int32_t transactionNum; /**< Number of transactions in the block. */
	CBTransaction ** transactions; /**< The transactions included in this block. NULL if only the header has been received. */
} CBBlock;

/**
 @brief Creates a new CBBlock object. Set the members after creating the block object.
 @returns A new CBBlock object.
 */
CBBlock * CBNewBlock(CBEvents * events);
/**
 @brief Creates a new CBBlock object.
 @param data Serialised block data.
 @returns A new CBBlock object.
 */
CBBlock * CBNewBlockFromData(CBByteArray * data,CBEvents * events);
/**
 @brief Creates a new CBBlock object with the genesis information for the bitcoin block chain. This will have serialised data as well as object data.
 @param data Serialised block data.
 @returns A new CBBlock object.
 */
CBBlock * CBNewBlockGenesis(CBEvents * events);

/**
 @brief Gets a CBBlock from another object. Use this to avoid casts.
 @param self The object to obtain the CBBlock from.
 @returns The CBBlock object.
 */
CBBlock * CBGetBlock(void * self);

/**
 @brief Initialises a CBBlock object
 @param self The CBBlock object to initialise
 @returns true on success, false on failure.
 */
bool CBInitBlock(CBBlock * self,CBEvents * events);
/**
 @brief Initialises a CBBlock object from serialised data
 @param self The CBBlock object to initialise
 @param data The serialised data.
 @returns true on success, false on failure.
 */
bool CBInitBlockFromData(CBBlock * self,CBByteArray * data,CBEvents * events);

/**
 @brief Initialises a CBBlock object with the genesis information for the bitcoin block chain. This will have serialised data as well as object data.
 @param self The CBBlock object to initialise.
 @param data Serialised block data.
 @returns A new CBBlock object.
 */
bool CBInitBlockGenesis(CBBlock * self,CBEvents * events);

/**
 @brief Frees a CBBlock object.
 @param self The CBBlock object to free.
 */
void CBFreeBlock(void * vself);
 
//  Functions

/**
 @brief Calculates the hash for a block.
 @param self The CBBlock object. This should be serialised.
 @returns The hash for the block. This is a 32 byte long, double SHA-256 hash.
 */
CBByteArray * CBBlockCalculateHash(CBBlock * self);
/**
 @brief Deserialises a CBBlock so that it can be used as an object.
 @param self The CBBlock object
 @param transactions If true deserialise transactions. If false there do not deserialise for transactions.
 @returns The length read on success, 0 on failure.
 */
u_int32_t CBBlockDeserialise(CBBlock * self,bool transactions);
/**
 @brief Retrieves or calculates the hash for a block. Hashes taken from this fuction are cached.
 @param self The CBBlock object. This should be serialised.
 @returns The hash for the block. This is a 32 byte long, double SHA-256 hash.
 */
CBByteArray * CBBlockGetHash(CBBlock * self);
/**
 @brief Serialises a CBBlock to the byte data.
 @param self The CBBlock object
 @param transactions If true serialise transactions. If false there do not serialise for transactions.
 @returns The length read on success, 0 on failure.
 */
u_int32_t CBBlockSerialise(CBBlock * self,bool transactions);

#endif
