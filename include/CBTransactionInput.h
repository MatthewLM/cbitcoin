//
//  CBTransactionInput.h
//  cbitcoin
//
//  Created by Matthew Mitchell on 02/05/2012.
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
 @brief Represents an input into a bitcoin transaction. Inherits CBMessage
*/

#ifndef CBTRANSACTIONINPUTH
#define CBTRANSACTIONINPUTH

//  Includes

#include "CBMessage.h"
#include "CBScript.h"
#include "CBTransactionOutput.h"

// Constants

#define CB_TRANSACTION_INPUT_FINAL 0xFFFFFFFF // Transaction input is final

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
	uint32_t sequence; /**< The version of this transaction input. Not used in protocol v0.3.18.00. Set to 0 for transactions that may somday be open to change after broadcast, set to CB_TRANSACTION_INPUT_FINAL if this input never needs to be changed after broadcast. */
	CBScript * scriptObject; /**< Contains script information as a CBScript. */
	CBPrevOut prevOut; /**< A locator for a previous output being spent. */
} CBTransactionInput;

/**
 @brief Creates a new CBTransactionInput object.
 @returns A new CBTransactionInput object.
 */
CBTransactionInput * CBNewTransactionInput(CBScript * script, uint32_t sequence, CBByteArray * prevOutHash, uint32_t prevOutIndex);
/**
 @brief Creates a new CBTransactionInput object from the byte data.
 @param data The byte data.
 @returns A new CBTransactionInput object.
 */
CBTransactionInput * CBNewTransactionInputFromData(CBByteArray * data);
/**
 @brief Creates a new unsigned CBTransactionInput object and links it to a given output.
 @returns A new CBTransactionInput object.
 */
CBTransactionInput * CBNewUnsignedTransactionInput(uint32_t sequence, CBByteArray * prevOutHash, uint32_t prevOutIndex);

/**
 @brief Gets a CBTransactionInput from another object. Use this to avoid casts.
 @param self The object to obtain the CBTransactionInput from.
 @returns The CBTransactionInput object.
 */
CBTransactionInput * CBGetTransactionInput(void * self);

/**
 @brief Initialises a CBTransactionInput object.
 @param self The CBTransactionInput object to initialise
 @returns true on success, false on failure.
 */
bool CBInitTransactionInput(CBTransactionInput * self, CBScript * script, uint32_t sequence, CBByteArray * prevOutHash, uint32_t prevOutIndex);
/**
 @brief Initialises a new CBTransactionInput object from the byte data.
 @param self The CBTransactionInput object to initialise
 @param data The byte data.
 @returns true on success, false on failure.
 */
bool CBInitTransactionInputFromData(CBTransactionInput * self, CBByteArray * data);
/**
 @brief Initialises an unsigned CBTransactionInput object.
 @param self The CBTransactionInput object to initialise
 @returns true on success, false on failure.
 */
bool CBInitUnsignedTransactionInput(CBTransactionInput * self, uint32_t sequence, CBByteArray * prevOutHash, uint32_t prevOutIndex);

/**
 @brief Frees a CBTransactionInput object.
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
/**
 @brief Serialises a CBTransactionInput to the byte data.
 @param self The CBTransactionInput object
 @returns The length written on success, 0 on failure.
 */
uint32_t CBTransactionInputSerialise(CBTransactionInput * self);

#endif
