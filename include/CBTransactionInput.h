//
//  CBTransactionInput.h
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
 @brief Represents an input into a bitcoin transaction. Inherits CBMessage
*/

#ifndef CBTXINPUTH
#define CBTXINPUTH

//  Includes

#include "CBMessage.h"
#include "CBScript.h"
#include "CBTransactionOutput.h"

// Constants and Macros

#define CB_TX_INPUT_FINAL 0xFFFFFFFF // Transaction input is final
#define CBGetTransactionInput(x) ((CBTransactionInput *)x)

/**
 @brief Structure for previous outputs that are being spent by an input.
 */
typedef struct{
	CBByteArray * hash; /**< Hash of the transaction that includes the output for this input */
	uint32_t index; /**< The index of the output for this input */
} CBPrevOut;

/**
 @brief Structure for CBTransactionInput objects. @see CBTransactionInput.h
*/
typedef struct{
	CBMessage base; /**< CBMessage base structure */
	uint32_t sequence; /**< The version of this transaction input. Not used in protocol v0.3.18.00. Set to 0 for transactions that may somday be open to change after broadcast, set to CB_TX_INPUT_FINAL if this input never needs to be changed after broadcast. */
	CBScript * scriptObject; /**< Contains script information as a CBScript. */
	CBPrevOut prevOut; /**< A locator for a previous output being spent. */
} CBTransactionInput;

/**
 @brief Creates a new CBTransactionInput object.
 @returns A new CBTransactionInput object.
 */
CBTransactionInput * CBNewTransactionInput(CBScript * script, uint32_t sequence, CBByteArray * prevOutHash, uint32_t prevOutIndex);

/**
 @brief Creates a new CBTransactionInput object and does not retain the prevOutHash and script.
 @returns A new CBTransactionInput object.
 */
CBTransactionInput * CBNewTransactionInputTakeScriptAndHash(CBScript * script, uint32_t sequence, CBByteArray * prevOutHash, uint32_t prevOutIndex);

/**
 @brief Creates a new CBTransactionInput object from the byte data.
 @param data The byte data.
 @returns A new CBTransactionInput object.
 */
CBTransactionInput * CBNewTransactionInputFromData(CBByteArray * data);

/**
 @brief Initialises a CBTransactionInput object.
 @param self The CBTransactionInput object to initialise
 @returns true on success, false on failure.
 */
void CBInitTransactionInput(CBTransactionInput * self, CBScript * script, uint32_t sequence, CBByteArray * prevOutHash, uint32_t prevOutIndex);

/**
 @brief Initialises a CBTransactionInput object and does not retain the prevOutHash and script.
 @param self The CBTransactionInput object to initialise
 @returns true on success, false on failure.
 */
void CBInitTransactionInputTakeScriptAndHash(CBTransactionInput * self, CBScript * script, uint32_t sequence, CBByteArray * prevOutHash, uint32_t prevOutIndex);

/**
 @brief Initialises a new CBTransactionInput object from the byte data.
 @param self The CBTransactionInput object to initialise
 @param data The byte data.
 @returns true on success, false on failure.
 */
void CBInitTransactionInputFromData(CBTransactionInput * self, CBByteArray * data);

/**
 @brief Release and free the objects stored by the CBTransactionInput object.
 @param self The CBTransactionInput object to free.
 */
void CBDestroyTransactionInput(void * self);

/**
 @brief Frees a CBTransactionInput object and also calls CBDestroyTransactionInput.
 @param self The CBTransactionInput object to free.
 */
void CBFreeTransactionInput(void * self);
 
//  Functions

/**
 @brief Calculates the byte length of an input
 @param self The CBTransactionInput object
 @returns The calculated length.
 */
uint32_t CBTransactionInputCalculateLength(CBTransactionInput * self);

/**
 @brief Deserialises a CBTransactionInput so that it can be used as an object.
 @param self The CBTransactionInput object
 @returns The length read on success, 0 on failure.
 */
uint32_t CBTransactionInputDeserialise(CBTransactionInput * self);

void CBTransactionInputPrepareBytes(CBTransactionInput * self);

/**
 @brief Serialises a CBTransactionInput to the byte data.
 @param self The CBTransactionInput object
 @returns The length written on success, 0 on failure.
 */
uint32_t CBTransactionInputSerialise(CBTransactionInput * self);

#endif
