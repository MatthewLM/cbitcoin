//
//  CBTransactionInput.h
//  cbitcoin
//
//  Created by Matthew Mitchell on 02/05/2012.
//  Last modified by Matthew Mitchell on 02/05/2012.
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
	CBByteArray * scriptData; /**< Contains script information in a byte array. This only includes script data for inputs taken from an address and not newly mined coins */
	CBScript * scriptObject; /**< Contains script information as a CBScript. */
	void * parentTransaction; /**< Transaction including this input. */
	CBSha256Hash * outPointerHash; /**< Hash of the transaction that includes the output for this input */
	u_int32_t outPointerIndex; /**< The index of the output for this input */ // ??? What size integer? 32 bits probably future proof.
} CBTransactionInput;

/**
 @brief Creates a new CBTransactionInput object.
 @returns A new CBTransactionInput object.
 */
CBTransactionInput * CBNewTransactionInput(CBNetworkParameters * params, void * parentTransaction,CBByteArray * scriptData,CBSha256Hash * outPointerHash,u_int32_t outPointerIndex,CBEngine * events);
/**
 @brief Creates a new unsigned CBTransactionInput object and links it to a given output.
 @returns A new CBTransactionInput object.
 */
CBTransactionInput * CBNewUnsignedTransactionInput(CBNetworkParameters * params, void * parentTransaction,CBTransactionOutput * output,CBEngine * events);

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
bool CBInitTransactionInput(CBTransactionInput * self,CBNetworkParameters * params, void * parentTransaction,CBByteArray * scriptData,CBSha256Hash * outPointerHash,u_int32_t outPointerIndex,CBEngine * events);

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

#endif