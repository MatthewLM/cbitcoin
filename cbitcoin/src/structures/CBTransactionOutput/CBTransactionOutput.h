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
#include "CBTransaction.h"

/**
 @brief Virtual function table for CBTransactionOutput.
*/
typedef struct{
	CBMessageVT base; /**< CBMessageVT base structure */
}CBTransactionOutputVT;

/**
 @brief Structure for CBTransactionOutput objects. @see CBTransactionOutput.h
*/
typedef struct{
	CBMessage base; /**< CBMessage base structure */
	int64_t value; /**< The transaction value */
	CBByteArray * scriptData; /**< The output script byte data */
	CBScript * scriptObject; /**< The output script object */
	bool availableForSpending; /**< True is output can be spent */
	void * spentBy; /**< CBTransactionInput that spent this output if any */
	CBTransaction * parentTransaction; /**< Transaction containing this output */
	u_int32_t scriptLen; /**< Length of script */
} CBTransactionOutput;

/**
 @brief Creates a new CBTransactionOutput object.
 @returns A new CBTransactionOutput object.
 */
CBTransactionOutput * CBNewTransactionOutputDeserialisation(CBNetworkParameters * params, CBTransaction * parent, CBByteArray * payload,u_int32_t offset,bool parseLazy,bool parseRetain);

/**
 @brief Creates a new CBTransactionOutputVT.
 @returns A new CBTransactionOutputVT.
 */
CBTransactionOutputVT * CBCreateTransactionOutputVT(void);
/**
 @brief Sets the CBTransactionOutputVT function pointers.
 @param VT The CBTransactionOutputVT to set.
 */
void CBSetTransactionOutputVT(CBTransactionOutputVT * VT);

/**
 @brief Gets the CBTransactionOutputVT. Use this to avoid casts.
 @param self The object to obtain the CBTransactionOutputVT from.
 @returns The CBTransactionOutputVT.
 */
CBTransactionOutputVT * CBGetTransactionOutputVT(void * self);

/**
 @brief Gets a CBTransactionOutput from another object. Use this to avoid casts.
 @param self The object to obtain the CBTransactionOutput from.
 @returns The CBTransactionOutput object.
 */
CBTransactionOutput * CBGetTransactionOutput(void * self);

/**
 @brief Initialises a CBTransactionOutput object
 @param self The CBTransactionOutput object to initialise
 @returns true on success, false on failure.
 */
bool CBInitTransactionOutputDeserialisation(CBTransactionOutput * self,CBNetworkParameters * params, CBTransaction * parent, CBByteArray * payload, u_int32_t offset,bool parseLazy,bool parseRetain);

/**
 @brief Frees a CBTransactionOutput object.
 @param self The CBTransactionOutput object to free.
 */
void CBFreeTransactionOutput(CBTransactionOutput * self);

/**
 @brief Does the processing to free a CBTransactionOutput object. Should be called by the children when freeing objects.
 @param self The CBTransactionOutput object to free.
 */
void CBFreeProcessTransactionOutput(CBTransactionOutput * self);
 
//  Functions

#endif