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
 @brief Stores a bitcoin script which can be processed to determine if a transaction input is valid. A good resource is https://en.bitcoin.it/wiki/Script Inherits CBObject
*/

#ifndef CBSCRIPTH
#define CBSCRIPTH

//  Includes

#include "CBByteArray.h"
#include "CBNetworkParameters.h"
#include "CBDependencies.h"
#include <stdbool.h>

/**
 @brief Structure for a stack item
 */

typedef struct{
	u_int8_t * data; /**< Data for this stack item */
	u_int16_t length; /**< Length of this item */
} CBScriptStackItem;

/**
 @brief Structure that holds byte data in a stack.
 */

typedef struct{
	CBScriptStackItem * elements; /**< Elements in the stack */
	u_int16_t length; /**< Length of the stack */
} CBScriptStack;

/**
 @brief Virtual function table for CBScript.
*/
typedef struct{
	CBObjectVT base; /**< CBObjectVT base structure */
	bool (*execute)(void *,CBScriptStack *,CBDependencies *); /**< Executes the script with the given stack and dependencies structure. */
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
 @brief Frees a CBScriptStack
 @param stack The stack to free
 */
void CBFreeScriptStack(CBScriptStack stack);
/**
 @brief Returns a new empty stack.
 @returns The new empty stack.
 */
CBScriptStack CBNewEmptyScriptStack(void);
/**
 @brief Executes a bitcoin script.
 @param self The CBScript object with the program
 @param stack A pointer to the input stack for the program.
 @param dependencies A pointer to a CBDependencies structure object.
 @returns True is the program ended with true, false otherwise or on script failure.
 */
bool CBScriptExecute(CBScript * self,CBScriptStack * stack,CBDependencies * dependencies);
/**
 @brief Returns a copy of a stack item, "fromTop" from the top.
 @param stack A pointer to the stack.
 @param fromTop Number of items from the top to copy.
 @returns A copy of the stack item which should be freed.
 */
CBScriptStackItem CBScriptStackCopyItem(CBScriptStack * stack,u_int8_t fromTop);
/**
 @brief Evaluates the top stack item as a bool. False if 0 or -0.
 @param stack The stack.
 @returns The boolean result.
 */
bool CBScriptStackEvalBool(CBScriptStack * stack);
/**
 @brief Converts a CBScriptStackItem to an int64_t
 @param item The CBScriptStackItem to convert
 @returns A 64 bit signed integer.
 */
int64_t CBScriptStackItemToInt64(CBScriptStackItem item);
/**
 @brief Removes the top item from the stack and returns it.
 @param stack A pointer to the stack to pop the data.
 @returns The top item. This must be freed.
 */
CBScriptStackItem CBScriptStackPopItem(CBScriptStack * stack);
/**
 @brief Push data onto the stack which is freed by the stack.
 @param stack A pointer to the stack to push data onto.
 @param data The item to push on the stack.
 */
void CBScriptStackPushItem(CBScriptStack * stack,CBScriptStackItem item);
/**
 @brief Removes top item from the stack.
 @param stack A pointer to the stack to remove the data.
 */
void CBScriptStackRemoveItem(CBScriptStack * stack);
/**
 @brief Converts a int64_t to a CBScriptStackItem
 @param item Pass in a CBScriptStackItem for reallocating data.
 @param i The 64 bit signed integer.
 @returns A CBScriptStackItem.
 */
CBScriptStackItem CBInt64ToScriptStackItem(CBScriptStackItem item,int64_t i);

#endif