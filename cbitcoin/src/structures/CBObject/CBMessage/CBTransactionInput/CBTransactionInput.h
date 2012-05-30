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

/**
 @brief Virtual function table for CBTransactionInput.
*/
typedef struct{
	CBMessageVT base; /**< CBMessageVT base structure */
}CBTransactionInputVT;

/**
 @brief Structure for CBTransactionInput objects. @see CBTransactionInput.h
*/
typedef struct{
	CBMessage base; /**< CBMessage base structure */
	u_int32_t sequence; /**< The version of this transaction input. Not used in protocol v0.3.18.00. Set to 0 for transactions that may somday be open to change after broadcast, set to CB_TRANSACTION_INPUT_FINAL if this input never needs to be changed after broadcast. */
	CBScript * scriptObject; /**< Contains script information as a CBScript. */
	void * parentTransaction; /**< Transaction including this input. */
	CBByteArray * outPointerHash; /**< Hash of the transaction that includes the output for this input */
	u_int32_t outPointerIndex; /**< The index of the output for this input */ // ??? What size integer? 32 bits probably future proof.
} CBTransactionInput;

/**
 @brief Creates a new CBTransactionInput object.
 @returns A new CBTransactionInput object.
 */
CBTransactionInput * CBNewTransactionInput(CBNetworkParameters * params, void * parentTransaction,CBByteArray * scriptData,CBByteArray * outPointerHash,u_int32_t outPointerIndex,u_int32_t protocolVersion,CBEvents * events);
/**
 @brief Creates a new CBTransactionInput object from the byte data.
 @param data The byte data.
 @returns A new CBTransactionInput object.
 */
CBTransactionInput * CBNewTransactionInputFromData(CBNetworkParameters * params, CBByteArray * data,u_int32_t protocolVersion,CBEvents * events);
/**
 @brief Creates a new unsigned CBTransactionInput object and links it to a given output.
 @returns A new CBTransactionInput object.
 */
CBTransactionInput * CBNewUnsignedTransactionInput(CBNetworkParameters * params, void * parentTransaction,CBByteArray * outPointerHash,u_int32_t outPointerIndex,u_int32_t protocolVersion,CBEvents * events);

/**
 @brief Creates a new CBTransactionInputVT.
 @returns A new CBTransactionInputVT.
 */
CBTransactionInputVT * CBCreateTransactionInputVT(void);
/**
 @brief Sets the CBTransactionInputVT function pointers.
 @param VT The CBTransactionInputVT to set.
 */
void CBSetTransactionInputVT(CBTransactionInputVT * VT);

/**
 @brief Gets the CBTransactionInputVT. Use this to avoid casts.
 @param self The object to obtain the CBTransactionInputVT from.
 @returns The CBTransactionInputVT.
 */
CBTransactionInputVT * CBGetTransactionInputVT(void * self);

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
bool CBInitTransactionInput(CBTransactionInput * self,CBNetworkParameters * params, void * parentTransaction,CBByteArray * scriptData,CBByteArray * outPointerHash,u_int32_t outPointerIndex,u_int32_t protocolVersion,CBEvents * events);
/**
 @brief Initialises an unsigned CBTransactionInput object.
 @param self The CBTransactionInput object to initialise
 @returns true on success, false on failure.
 */
bool CBInitUnsignedTransactionInput(CBTransactionInput * self,CBNetworkParameters * params, void * parentTransaction,CBByteArray * outPointerHash,u_int32_t outPointerIndex,u_int32_t protocolVersion,CBEvents * events);

/**
 @brief Frees a CBTransactionInput object.
 @param self The CBTransactionInput object to free.
 */
void CBFreeTransactionInput(CBTransactionInput * self);

/**
 @brief Does the processing to free a CBTransactionInput object. Should be called by the children when freeing objects.
 @param self The CBTransactionInput object to free.
 */
void CBFreeProcessTransactionInput(CBTransactionInput * self);
 
//  Functions

/**
 @brief Deserialises a CBTransactionInput so that it can be used as an object.
 @param self The CBMessage object
 @returns true on success, false on failure.
 */
bool CBTransactionInputDeserialise(CBTransactionInput * self);
/**
 @brief Serialises a CBTransactionInput to the byte data.
 @param self The CBMessage object
 @param bytes The bytes to fill. Should be the full length needed.
 */
bool CBTransactionInputSerialise(CBTransactionInput * self);

#endif