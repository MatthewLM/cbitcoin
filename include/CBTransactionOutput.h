//
//  CBTransactionOutput.h
//  cbitcoin
//
//  Created by Matthew Mitchell on 03/05/2012.
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
 @brief Describes how the bitcoins can be spent from the output. Inherits CBObject
*/

#ifndef CBTRANSACTIONOUTPUTH
#define CBTRANSACTIONOUTPUTH

//  Includes

#include "CBMessage.h"
#include "CBScript.h"

// Constants

#define CB_OUTPUT_VALUE_MINUS_ONE 0xFFFFFFFFFFFFFFFF // In twos complement it represents -1. Bitcoin uses twos compliment.

/**
 @brief Structure for CBTransactionOutput objects. @see CBTransactionOutput.h
*/
typedef struct{
	CBMessage base; /**< CBMessage base structure */
	uint64_t value; /**< The transaction value */
	CBScript * scriptObject; /**< The output script object */
} CBTransactionOutput;

/**
 @brief Creates a new CBTransactionOutput object.
 @returns A new CBTransactionOutput object.
 */
CBTransactionOutput * CBNewTransactionOutput(uint64_t value, CBScript * script);
/**
 @brief Creates a new CBTransactionOutput object from byte data. Should be serialised for object data.
 @returns A new CBTransactionOutput object.
 */
CBTransactionOutput * CBNewTransactionOutputFromData(CBByteArray * data);

/**
 @brief Gets a CBTransactionOutput from another object. Use this to avoid casts.
 @param self The object to obtain the CBTransactionOutput from.
 @returns The CBTransactionOutput object.
 */
CBTransactionOutput * CBGetTransactionOutput(void * self);

/**
 @brief Initialises a CBTransactionOutput object.
 @param self The CBTransactionOutput object to initialise.
 @returns true on success, false on failure.
 */
bool CBInitTransactionOutput(CBTransactionOutput * self, uint64_t value, CBScript * script);
/**
 @brief Initialises a CBTransactionOutput object.
 @param self The CBTransactionOutput object to initialise.
 @returns true on success, false on failure.
 */
bool CBInitTransactionOutputFromData(CBTransactionOutput * self, CBByteArray * data);

/**
 @brief Frees a CBTransactionOutput object.
 @param self The CBTransactionOutput object to free.
 */
void CBFreeTransactionOutput(void * self);
 
//  Functions

/**
 @brief Calculates the byte length of an output
 @param self The CBTransactionOutput object
 @returns The calculated length.
 */
uint32_t CBTransactionOutputCalculateLength(CBTransactionOutput * self);
/**
 @brief Deserialises a CBTransactionOutput so that it can be used as an object.
 @param self The CBTransactionOutput object
 @returns The length read on success, 0 on failure.
 */
uint32_t CBTransactionOutputDeserialise(CBTransactionOutput * self);
/**
 @brief Serialises a CBTransactionOutput to the byte data.
 @param self The CBTransactionOutput object
 @returns The length written on success, 0 on failure.
 */
uint32_t CBTransactionOutputSerialise(CBTransactionOutput * self);

#endif
