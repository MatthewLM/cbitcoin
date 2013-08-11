//
//  CBBlockHeaders.h
//  cbitcoin
//
//  Created by Matthew Mitchell on 12/07/2012.
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
 @brief A message used to send and receive block headers. Inherits CBMessage
*/

#ifndef CBBLOCKHEADERSH
#define CBBLOCKHEADERSH

//  Includes

#include "CBBlock.h"

// Cosntants

#define CB_BLOCK_HEADERS_MAX_SIZE 178009
#define CBGetBlockHeaders(x) ((CBBlockHeaders *)x)

/**
 @brief Structure for CBBlockHeaders objects. @see CBBlockHeaders.h
*/
typedef struct{
	CBMessage base; /**< CBMessage base structure */
	uint16_t headerNum; /**< The number of headers. */
	CBBlock ** blockHeaders; /**< The block headers as CBBlock objects with no transactions. The number of transactions is given however. */
} CBBlockHeaders;

/**
 @brief Creates a new CBBlockHeaders object.
 @returns A new CBBlockHeaders object.
*/
CBBlockHeaders * CBNewBlockHeaders(void);
/**
@brief Creates a new CBBlockHeaders object from serialised data.
 @param data Serialised CBBlockHeaders data.
 @returns A new CBBlockHeaders object.
*/
CBBlockHeaders * CBNewBlockHeadersFromData(CBByteArray * data);

/**
 @brief Initialises a CBBlockHeaders object
 @param self The CBBlockHeaders object to initialise
*/
void CBInitBlockHeaders(CBBlockHeaders * self);
/**
 @brief Initialises a CBBlockHeaders object from serialised data
 @param self The CBBlockHeaders object to initialise
 @param data The serialised data.
*/
void CBInitBlockHeadersFromData(CBBlockHeaders * self, CBByteArray * data);

/**
 @brief Releases and frees all of the objects stored by the CBBlockHeaders object.
 @param self The CBBlockHeaders object to destory.
 */
void CBDestroyBlockHeaders(void * self);
/**
 @brief Frees a CBBlockHeaders object and also calls CBDestroyBlockHeaders.
 @param self The CBBlockHeaders object to free.
 */
void CBFreeBlockHeaders(void * self);
 
//  Functions

/**
 @brief Adds a CBBlock into the block header list.
 @param self The CBBlockHeaders object
 @param address The CBBlock to add.
 */
void CBBlockHeadersAddBlockHeader(CBBlockHeaders * self, CBBlock * header);
/**
 @brief Calculates the length needed to serialise the object.
 @param self The CBBlockHeaders object.
 @returns The length
 */
uint32_t CBBlockHeadersCalculateLength(CBBlockHeaders * self);
/**
 @brief Deserialises a CBBlockHeaders so that it can be used as an object.
 @param self The CBBlockHeaders object
 @returns The length read on success, 0 on failure.
*/
uint32_t CBBlockHeadersDeserialise(CBBlockHeaders * self);
/**
 @brief Serialises a CBBlockHeaders to the byte data.
 @param self The CBBlockHeaders object
 @param force Serialises everything, replacing any previous serialisation of children objects.
 @returns The length written on success, 0 on failure.
*/
uint32_t CBBlockHeadersSerialise(CBBlockHeaders * self, bool force);
/**
 @brief Takes a CBBlock for the block header list. This does not retain the CBBlock so you can pass an CBBlock into this while releasing control from the calling function.
 @param self The CBBlockHeaders object
 @param address The CBBlock to take.
 */
void CBBlockHeadersTakeBlockHeader(CBBlockHeaders * self, CBBlock * header);

#endif
