//
//  CBTransaction.h
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
 @brief A CBTransaction represents a movement of bitcoins and newly mined bitcoins. Inherits CBMessage
*/

#ifndef CBTXH
#define CBTXH

//  Includes

#include "CBMessage.h"
#include "CBTransactionInput.h"
#include "CBTransactionOutput.h"
#include "CBHDKeys.h"

// Constants and Macros

#define CB_TX_MAX_STANDARD_VERSION 1
#define CB_TX_MAX_STANDARD_SIZE 100000
#define CB_TX_HASH_STR_SIZE 41
#define CBGetTransaction(x) ((CBTransaction *)x)

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
 @brief Initialises a CBTransaction object
 @param self The CBTransaction object to initialise
 */
void CBInitTransaction(CBTransaction * self, uint32_t lockTime, uint32_t version);
/**
 @brief Initialises a new CBTransaction object from the byte data.
 @param self The CBTransaction object to initialise
 @param data The byte data.
 */
void CBInitTransactionFromData(CBTransaction * self, CBByteArray * data);

/**
 @brief Release and free the objects stored by the CBTransaction object.
 @param self The CBTransaction object to destroy.
 */
void CBDestroyTransaction(void * self);
/**
 @brief Frees a CBTransaction object and also calls CBDestroyTransaction.
 @param self The CBTransaction object to free.
 */
void CBFreeTransaction(void * self);
 
//  Functions

/**
 @brief Adds an CBTransactionInput to the CBTransaction.
 @param self The CBTransaction object.
 @param input The CBTransactionInput object.
 */
void CBTransactionAddInput(CBTransaction * self, CBTransactionInput * input);
/**
 @brief Adds an CBTransactionInput to the CBTransaction.
 @param self The CBTransaction object.
 @param input The CBTransactionOutput object.
 */
void CBTransactionAddOutput(CBTransaction * self, CBTransactionOutput * output);
void CBTransactionAddP2SHScript(CBTransaction * self, CBScript * p2shScript, uint32_t input);
bool CBTransactionAddSignature(CBTransaction * self, CBScript * inScript, uint16_t offset, uint8_t sigSize, CBKeyPair * key, CBByteArray * prevOutSubScript, uint32_t input, CBSignType signType);
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
 @returns true if the hash has been retreived with no problems. false is returned if the hash is invalid.
 */
bool CBTransactionGetInputHashForSignature(void * vself, CBByteArray * prevOutSubScript, uint32_t input, CBSignType signType, uint8_t * hash);
void CBTransactionHashToString(CBTransaction * self, char output[CB_TX_HASH_STR_SIZE]);
bool CBTransactionInputIsStandard(CBScript * inputScript, CBScript * outputScript, CBScript * p2sh);
/**
 @brief Determines if a transaction is a coinbase transaction or not.
 @param self The CBTransaction object.
 @returns true if the transaction is a coin-base transaction or false if not.
 */
bool CBTransactionIsCoinBase(CBTransaction * self);
bool CBTransactionIsStandard(CBTransaction * self);
void CBTransactionMakeBytes(CBTransaction * self);
/**
 @brief Serialises a CBTransaction to the byte data.
 @param self The CBTransaction object.
 @param force Serialises everything, replacing any previous serialisation of children objects.
 @returns The length read on success, 0 on failure.
 */
uint32_t CBTransactionSerialise(CBTransaction * self, bool force);
bool CBTransactionSignMultisigInput(CBTransaction * self, CBKeyPair * key, CBByteArray * prevOutSubScript, uint32_t input, CBSignType signType);
bool CBTransactionSignPubKeyHashInput(CBTransaction * self, CBKeyPair * key, CBByteArray * prevOutSubScript, uint32_t input, CBSignType signType);
bool CBTransactionSignPubKeyInput(CBTransaction * self, CBKeyPair * key, CBByteArray * prevOutSubScript, uint32_t input, CBSignType signType);
/**
 @brief Adds an CBTransactionInput to the CBTransaction without retaining it.
 @param self The CBTransaction object.
 @param input The CBTransactionInput object.
 */
void CBTransactionTakeInput(CBTransaction * self, CBTransactionInput * input);
/**
 @brief Adds an CBTransactionInput to the CBTransaction without retaining it.
 @param self The CBTransaction object.
 @param input The CBTransactionOutput object.
 */
void CBTransactionTakeOutput(CBTransaction * self, CBTransactionOutput * output);

#endif
