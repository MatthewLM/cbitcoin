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
 @brief Virtual function table for CBTransaction.
*/
typedef struct{
	CBMessageVT base; /**< CBMessageVT base structure */
	void (*addInput)(void *, CBTransactionInput *); /**< Pointer to the function used to add an output to the transaction. */
	void (*addOutput)(void *, CBTransactionOutput *); /**< Pointer to the function used to add an output to the transaction. */
}CBTransactionVT;

/**
 @brief Structure for CBTransaction objects. @see CBTransaction.h
*/
typedef struct{
	CBMessage base; /**< CBMessage base structure */
	u_int32_t protocolVersion; /**< Version of the bitcoin protocol. */
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
CBTransaction * CBNewTransaction(CBNetworkParameters * params, u_int32_t lockTime, u_int32_t protocolVersion, CBEvents * events);
/**
 @brief Creates a new CBTransaction object from byte data. Should be serialised for object data.
 @returns A new CBTransaction object.
 */
CBTransaction * CBNewTransactionFromData(CBNetworkParameters * params, CBByteArray * bytes, CBEvents * events);

/**
 @brief Creates a new CBTransactionVT.
 @returns A new CBTransactionVT.
 */
CBTransactionVT * CBCreateTransactionVT(void);
/**
 @brief Sets the CBTransactionVT function pointers.
 @param VT The CBTransactionVT to set.
 */
void CBSetTransactionVT(CBTransactionVT * VT);

/**
 @brief Gets the CBTransactionVT. Use this to avoid casts.
 @param self The object to obtain the CBTransactionVT from.
 @returns The CBTransactionVT.
 */
CBTransactionVT * CBGetTransactionVT(void * self);

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
bool CBInitTransaction(CBTransaction * self,CBNetworkParameters * params, u_int32_t lockTime, u_int32_t protocolVersion, CBEvents * events);
/**
 @brief Initialises a new CBTransaction object from the byte data.
 @param self The CBTransaction object to initialise
 @param data The byte data.
 @returns true on success, false on failure.
 */
bool CBInitTransactionFromData(CBTransaction * self,CBNetworkParameters * params, CBByteArray * data,CBEvents * events);

/**
 @brief Frees a CBTransaction object.
 @param self The CBTransaction object to free.
 */
void CBFreeTransaction(CBTransaction * self);

/**
 @brief Does the processing to free a CBTransaction object. Should be called by the children when freeing objects.
 @param self The CBTransaction object to free.
 */
void CBFreeProcessTransaction(CBTransaction * self);
 
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
 @returns true on success, false on failure.
 */
u_int32_t CBTransactionDeserialise(CBTransaction * self);
/**
 @brief Serialises a CBTransaction to the byte data.
 @param self The CBTransaction object
 @param bytes The bytes to fill. Should be the full length needed.
 @returns true on success, false on failure.
 */
u_int32_t CBTransactionSerialise(CBTransaction * self);

#endif