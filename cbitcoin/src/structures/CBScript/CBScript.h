//
//  CBScript.h
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
 @brief Stores a bitcoin script which can be processed to determine if a transaction input is valid. Inherits CBObject
*/

#ifndef CBSCRIPTH
#define CBSCRIPTH

//  Includes

#include "CBByteArray.h"
#include "CBNetworkParameters.h"
#include <stdbool.h>

/**
 @brief Virtual function table for CBScript.
*/
typedef struct{
	CBObjectVT base; /**< CBObjectVT base structure */
	u_int8_t (*getByte)(void *); /**< Pointer to the function used to get a byte from the program and move along the cursor. */
	u_int16_t (*readUInt16)(void *); /**< Pointer to the function used to get a 16 bit integer from the program and move along the cursor. */
	u_int32_t (*readUInt32)(void *); /**< Pointer to the function used to get a 32 bit integer from the program and move along the cursor. */
	u_int64_t (*readUInt64)(void *); /**< Pointer to the function used to get a 64 bit integer from the program and move along the cursor. */
}CBScriptVT;

/**
 @brief Structure for CBScript objects. @see CBScript.h
*/
typedef struct{
	CBObject base; /**< CBObject base structure */
	CBByteArray * program;
	int cursor;
	CBNetworkParameters * params;
	CBEvents * events;
} CBScript;

/**
 @brief Creates a new CBScript object.
 @returns A new CBScript object.
 */
CBScript * CBNewScript(CBNetworkParameters * params,CBByteArray * program,CBEvents * events);

/**
 @brief Creates a new CBScriptVT.
 @returns A new CBScriptVT.
 */
CBScriptVT * CBCreateScriptVT(void);
/**
 @brief Sets the CBScriptVT function pointers.
 @param VT The CBScriptVT to set.
 */
void CBSetScriptVT(CBScriptVT * VT);

/**
 @brief Gets the CBScriptVT. Use this to avoid casts.
 @param self The object to obtain the CBScriptVT from.
 @returns The CBScriptVT.
 */
CBScriptVT * CBGetScriptVT(void * self);

/**
 @brief Gets a CBScript from another object. Use this to avoid casts.
 @param self The object to obtain the CBScript from.
 @returns The CBScript object.
 */
CBScript * CBGetScript(void * self);

/**
 @brief Initialises a CBScript object
 @param self The CBScript object to initialise
 @returns true on success, false on failure.
 */
bool CBInitScript(CBScript * self,CBNetworkParameters * params,CBByteArray * program,CBEvents * events);

/**
 @brief Frees a CBScript object.
 @param self The CBScript object to free.
 */
void CBFreeScript(CBScript * self);

/**
 @brief Does the processing to free a CBScript object. Should be called by the children when freeing objects.
 @param self The CBScript object to free.
 */
void CBFreeProcessScript(CBScript * self);
 
//  Functions

/**
 @brief Gets a byte from the program and moves along the cursor
 @param self The CBScript object with the program
 @returns A byte.
 */
u_int8_t CBScriptGetByte(CBScript * self);
/**
 @brief Reads a 16 bit integer from the program and moves along the cursor
 @param self The CBScript object with the program
 @returns A 16 bit integer
 */
u_int16_t CBScriptReadUInt16(CBScript * self);
/**
 @brief Reads a 32 bit integer from the program and moves along the cursor
 @param self The CBScript object with the program
 @returns A 32 bit integer
 */
u_int32_t CBScriptReadUInt32(CBScript * self);
/**
 @brief Reads a 64 bit integer from the program and moves along the cursor
 @param self The CBScript object with the program
 @returns A 64 bit integer
 */
u_int64_t CBScriptReadUInt64(CBScript * self);

#endif