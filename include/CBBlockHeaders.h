//
//  CBBlockHeaders.h
//  cbitcoin
//
//  Created by Matthew Mitchell on 12/07/2012.
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
 @brief A message used to send and receive block headers. Inherits CBMessage
*/

#ifndef CBBLOCKHEADERSH
#define CBBLOCKHEADERSH

//  Includes

#include "CBBlock.h"

// Cosntants

#define CB_BLOCK_HEADERS_MAX_SIZE 178009

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
 @brief Gets a CBBlockHeaders from another object. Use this to avoid casts.
 @param self The object to obtain the CBBlockHeaders from.
 @returns The CBBlockHeaders object.
*/
CBBlockHeaders * CBGetBlockHeaders(void * self);

/**
 @brief Initialises a CBBlockHeaders object
 @param self The CBBlockHeaders object to initialise
 @returns true on success, false on failure.
*/
bool CBInitBlockHeaders(CBBlockHeaders * self);
/**
 @brief Initialises a CBBlockHeaders object from serialised data
 @param self The CBBlockHeaders object to initialise
 @param data The serialised data.
 @returns true on success, false on failure.
*/
bool CBInitBlockHeadersFromData(CBBlockHeaders * self, CBByteArray * data);

/**
 @brief Frees a CBBlockHeaders object.
 @param self The CBBlockHeaders object to free.
 */
void CBFreeBlockHeaders(void * self);
 
//  Functions

/**
 @brief Adds a CBBlock into the block header list.
 @param self The CBBlockHeaders object
 @param address The CBBlock to add.
 @returns true if the block header was added successfully or false on error.
 */
bool CBBlockHeadersAddBlockHeader(CBBlockHeaders * self, CBBlock * header);
/**
 @brief Calculates the length needed to serialise the object.
 @param self The CBBlockHeaders object.
 @returns The length read on success, 0 on failure.
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
 @returns true if the block header was taken successfully or false on error.
 */
bool CBBlockHeadersTakeBlockHeader(CBBlockHeaders * self, CBBlock * header);

#endif
