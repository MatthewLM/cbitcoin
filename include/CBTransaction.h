//
//  CBTransaction.h
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
 @brief A CBTransaction represents a movement of bitcoins and newly mined bitcoins. Inherits CBMessage
*/

#ifndef CBTRANSACTIONH
#define CBTRANSACTIONH

//  Includes

#include "CBMessage.h"
#include "CBTransactionInput.h"
#include "CBTransactionOutput.h"

// Constants

#define CB_TRANSACTION_MAX_SIZE 999915 // Block size minus the header

/**
 @brief Structure for CBTransaction objects. @see CBTransaction.h
*/
typedef struct{
	CBMessage base; /**< CBMessage base structure */
	uint8_t hash[32]; /**< The hash for this transaction. NULL if not set. */
	bool hashSet; /**< True if the hash has been set, false otherwise */
	uint32_t version; /**< Version of the transaction data format. */
	uint32_t inputNum; /**< Number of CBTransactionInputs */
	CBTransactionInput ** inputs;
	uint32_t outputNum; /**< Number of CBTransactionOutputs */
	CBTransactionOutput ** outputs;
	uint32_t lockTime; /**< Time for the transaction to be valid */
} CBTransaction;

/**
 @brief Creates a new CBTransaction object with no inputs or outputs.
 @returns A new CBTransaction object.
 */
CBTransaction * CBNewTransaction(uint32_t lockTime, uint32_t version);
/**
 @brief Creates a new CBTransaction object from byte data. Should be serialised for object data.
 @returns A new CBTransaction object.
 */
CBTransaction * CBNewTransactionFromData(CBByteArray * bytes);

/**
 @brief Gets a CBTransaction from another object. Use this to avoid casts.
 @param self The object to obtain the CBTransaction from.
 @returns The CBTransaction object.
 */
CBTransaction * CBGetTransaction(void * self);

/**
 @brief Initialises a CBTransaction object
 @param self The CBTransaction object to initialise
 @returns true on success, false on failure.
 */
bool CBInitTransaction(CBTransaction * self, uint32_t lockTime, uint32_t version);
/**
 @brief Initialises a new CBTransaction object from the byte data.
 @param self The CBTransaction object to initialise
 @param data The byte data.
 @returns true on success, false on failure.
 */
bool CBInitTransactionFromData(CBTransaction * self, CBByteArray * data);

/**
 @brief Frees a CBTransaction object.
 @param self The CBTransaction object to free.
 */
void CBFreeTransaction(void * self);
 
//  Functions

/**
 @brief Adds an CBTransactionInput to the CBTransaction.
 @param self The CBTransaction object.
 @param input The CBTransactionInput object.
 @returns true if the transaction input was added successfully and false on error.
 */
bool CBTransactionAddInput(CBTransaction * self, CBTransactionInput * input);
/**
 @brief Adds an CBTransactionInput to the CBTransaction.
 @param self The CBTransaction object.
 @param input The CBTransactionOutput object.
 @returns true if the transaction output was added successfully and false on error.
 */
bool CBTransactionAddOutput(CBTransaction * self, CBTransactionOutput * output);
/**
 @brief Calculates the hash for a transaction.
 @param self The CBTransaction object. This should be serialised.
 @param The hash for the transaction to be set. This should be 32 bytes long.
 */
void CBTransactionCalculateHash(CBTransaction * self, uint8_t * hash);
/**
 @brief Calculates the length needed to serialise the object.
 @param self The CBTransaction object.
 @returns The length read on success, 0 on failure.
 */
uint32_t CBTransactionCalculateLength(CBTransaction * self);
/**
 @brief Deserialises a CBTransaction so that it can be used as an object.
 @param self The CBTransaction object
 @returns The length read on success, 0 on failure.
 */
uint32_t CBTransactionDeserialise(CBTransaction * self);
/**
 @brief Retrieves or calculates the hash for a transaction. Hashes taken from this fuction are cached.
 @param self The CBTransaction object. This should be serialised.
 @returns The hash for the transaction. This is a 32 byte long, double SHA-256 hash and is a pointer to the hash field in the transaction.
 */
uint8_t * CBTransactionGetHash(CBTransaction * self);
/**
 @brief Gets the hash for signing or signature checking for a transaction input. The transaction input needs to contain the outPointerHash, outPointerIndex and sequence. If these are modifed afterwards then the signiture is invalid.
 @param vself The CBTransaction object.
 @param prevOutSubScript The sub script from the output. Must be the correct one or the signiture will be invalid.
 @param input The index of the input to sign.
 @param signType The type of signature to get the data for.
 @param hash The 32 byte data hash for signing or checking signatures.
 @returns CB_TX_HASH_OK if the hash has been retreived with no problems. CB_TX_HASH_BAD is returned if the hash is invalid and CB_TX_HASH_ERR is returned upon an error.
 */
CBGetHashReturn CBTransactionGetInputHashForSignature(void * vself, CBByteArray * prevOutSubScript, uint32_t input, CBSignType signType, uint8_t * hash);
/**
 @brief Determines if a transaction is a coinbase transaction or not.
 @param self The CBTransaction object.
 @returns true if the transaction is a coin-base transaction or false if not.
 */
bool CBTransactionIsCoinBase(CBTransaction * self);
/**
 @brief Serialises a CBTransaction to the byte data.
 @param self The CBTransaction object.
 @param force Serialises everything, replacing any previous serialisation of children objects.
 @returns The length read on success, 0 on failure.
 */
uint32_t CBTransactionSerialise(CBTransaction * self, bool force);
/**
 @brief Adds an CBTransactionInput to the CBTransaction without retaining it.
 @param self The CBTransaction object.
 @param input The CBTransactionInput object.
 @returns true if the transaction input was taken successfully and false on error.
 */
bool CBTransactionTakeInput(CBTransaction * self, CBTransactionInput * input);
/**
 @brief Adds an CBTransactionInput to the CBTransaction without retaining it.
 @param self The CBTransaction object.
 @param input The CBTransactionOutput object.
 @returns true if the transaction output was taken successfully and false on error.
 */
bool CBTransactionTakeOutput(CBTransaction * self, CBTransactionOutput * output);

#endif
