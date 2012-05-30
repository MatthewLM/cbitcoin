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

/**
 @brief Virtual function table for CBTransaction.
*/
typedef struct{
	CBMessageVT base; /**< CBMessageVT base structure */
}CBTransactionVT;

/**
 @brief Structure for CBTransaction objects. @see CBTransaction.h
*/
typedef struct{
	CBMessage base; /**< CBMessage base structure */
	u_int64_t version;
	
} CBTransaction;

/**
 @brief Creates a new CBTransaction object.
 @returns A new CBTransaction object.
 */
CBTransaction * CBNewTransaction(void);

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
bool CBInitTransaction(CBTransaction * self);

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

#endif