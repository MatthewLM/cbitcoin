//
//  CBTransactionOutput.h
//  cbitcoin
//
//  Created by Matthew Mitchell on 03/05/2012.
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
 @brief Describes how the bitcoins can be spent from the output. Inherits CBObject
*/

#ifndef CBTXOUTPUTH
#define CBTXOUTPUTH

//  Includes

#include "CBMessage.h"
#include "CBScript.h"

// Constants and Macros

#define CB_OUTPUT_VALUE_MINUS_ONE 0xFFFFFFFFFFFFFFFF // In twos complement it represents -1. Bitcoin uses twos compliment.
#define CBGetTransactionOutput(x) ((CBTransactionOutput *)x)

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
 @brief Creates a new CBTransactionOutput object and does not retain the script object.
 @returns A new CBTransactionOutput object.
 */
CBTransactionOutput * CBNewTransactionOutputTakeScript(uint64_t value, CBScript * script);

/**
 @brief Creates a new CBTransactionOutput object from byte data. Should be serialised for object data.
 @returns A new CBTransactionOutput object.
 */
CBTransactionOutput * CBNewTransactionOutputFromData(CBByteArray * data);

/**
 @brief Initialises a CBTransactionOutput object.
 @param self The CBTransactionOutput object to initialise.
 */
void CBInitTransactionOutput(CBTransactionOutput * self, uint64_t value, CBScript * script);

/**
 @brief Initialises a CBTransactionOutput object and does not retain the script object.
 @param self The CBTransactionOutput object to initialise.
 */
void CBInitTransactionOutputTakeScript(CBTransactionOutput * self, uint64_t value, CBScript * script);

/**
 @brief Initialises a CBTransactionOutput object.
 @param self The CBTransactionOutput object to initialise.
 */
void CBInitTransactionOutputFromData(CBTransactionOutput * self, CBByteArray * data);

/**
 @brief Release and free all of the objects stored by the CBTransactionOutput object.
 @param self The CBTransactionOutput object to destroy.
 */
void CBDestroyTransactionOutput(void * self);

/**
 @brief Frees a CBTransactionOutput object and also calls CBDestroyTransactionOutput.
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
 @brief Create a hash that identifies this output by the public-key hash, the P2SH hash or twenty bytes of the SHA-256 of a multisig script. Nothing is set, if the output is not supported.
 @param self The CBTransactionOutput object.
 @param hash The 20 hash bytes to set.
 @returns true on success, false on failure.
 */
bool CBTransactionOuputGetHash(CBTransactionOutput * self, uint8_t * hash);

/**
 @brief Determines the type of this output.
 @param self The CBTransactionOutput object
 @returns The transaction output type.
 */
CBScriptOutputType CBTransactionOutputGetType(CBTransactionOutput * self);

void CBTransactionOutputPrepareBytes(CBTransactionOutput * self);

/**
 @brief Serialises a CBTransactionOutput to the byte data.
 @param self The CBTransactionOutput object
 @returns The length written on success, 0 on failure.
 */
uint32_t CBTransactionOutputSerialise(CBTransactionOutput * self);

#endif
