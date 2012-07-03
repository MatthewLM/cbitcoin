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

/**
 @brief Structure for CBTransaction objects. @see CBTransaction.h
*/
typedef struct{
	CBMessage base; /**< CBMessage base structure */
	u_int32_t version; /**< Version of the transaction data format. */
	u_int32_t inputNum; /**< Number of CBTransactionInputs */
	CBTransactionInput ** inputs;
	u_int32_t outputNum; /**< Number of CBTransactionOutputs */
	CBTransactionOutput ** outputs;
	u_int32_t lockTime; /**< Time for the transaction to be valid */
} CBTransaction;

/**
 @brief Creates a new CBTransaction object with no inputs or outputs.
 @returns A new CBTransaction object.
 */
CBTransaction * CBNewTransaction(u_int32_t lockTime, u_int32_t version, CBEvents * events);
/**
 @brief Creates a new CBTransaction object from byte data. Should be serialised for object data.
 @returns A new CBTransaction object.
 */
CBTransaction * CBNewTransactionFromData(CBByteArray * bytes, CBEvents * events);

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
bool CBInitTransaction(CBTransaction * self, u_int32_t lockTime, u_int32_t version, CBEvents * events);
/**
 @brief Initialises a new CBTransaction object from the byte data.
 @param self The CBTransaction object to initialise
 @param data The byte data.
 @returns true on success, false on failure.
 */
bool CBInitTransactionFromData(CBTransaction * self, CBByteArray * data,CBEvents * events);

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
 */
void CBTransactionAddInput(CBTransaction * self, CBTransactionInput * input);
/**
 @brief Adds an CBTransactionInput to the CBTransaction.
 @param self The CBTransaction object.
 @param input The CBTransactionOutput object.
 */
void CBTransactionAddOutput(CBTransaction * self, CBTransactionOutput * output);
/**
 @brief Deserialises a CBTransaction so that it can be used as an object.
 @param self The CBTransaction object
 @returns The length read on success, 0 on failure.
 */
u_int32_t CBTransactionDeserialise(CBTransaction * self);
/**
 @brief Gets the hash for signing or signature checking for a transaction input. The transaction input needs to contain the outPointerHash, outPointerIndex and sequence. If these are modifed afterwards then the signiture is invalid.
 @param self The CBTransaction object.
 @param prevOutSubScript The sub script from the output. Must be the correct one or the signiture will be invalid.
 @param input The index of the input to sign.
 @param signType The type of signature to get the data for.
 @returns NULL on failure or the 32 byte data hash for signing or checking signatures.
 */
u_int8_t * CBTransactionGetInputHashForSignature(CBTransaction * self, CBByteArray * prevOutSubScript, u_int32_t input, CBSignType signType);
/**
 @brief Serialises a CBTransaction to the byte data.
 @param self The CBTransaction object.
 @returns The length read on success, 0 on failure.
 */
u_int32_t CBTransactionSerialise(CBTransaction * self);
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
/**
 @brief Validates a transaction has outputs and inputs, is below the maximum block size and has outputs that do not overflow. Further validation can be done by checking the transaction against input transactions. With simplified payment verification instead the validation is done through trusting miners but the basic validation can still be done for these basic checks.
 @param self The transaction to validate. This should be deserialised.
 @return true if valid for the basic criteria, false if invalid.
 */
bool CBTransactionValidateBasic(CBTransaction * self);

#endif
