//
//  CBByteArray.h
//  cbitcoin
//
//  Created by Matthew Mitchell on 30/04/2012.
//  Last modified by Matthew Mitchell on 01/05/2012.
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
 @brief Provides an interface to a memory block as bytes. Many CBByteArrays can refer to the same memory block with different offsets and lengths. Inherits CBObject
*/

#ifndef CBBYTEARRAYH
#define CBBYTEARRAYH

//  Includes

#include "CBObject.h"
#include "CBEvents.h"
#include <stdbool.h>
#include <string.h>

/**
 @brief Stores byte data that can be shared amongst many CBByteArrays
 */
typedef struct{
	u_int8_t * data; /**< Pointer to byte data */
	u_int32_t references; /**< References to this data */
}CBSharedData;

/**
 @brief Virtual function table for CBByteArray.
*/
typedef struct{
	CBObjectVT base; /**< CBObjectVT base structure */
	void * (*copy)(void *); /**< Pointer to the function used to copy a byte array */
	u_int8_t (*getByte)(void *,u_int32_t); /**< Pointer to the function used to get a byte from a byte array */
	u_int8_t * (*getData)(void *); /**< Pointer to the function used to get a pointer to the underlying data at the byte array offset */
	void (*insertByte)(void *,u_int32_t,u_int8_t); /**< Pointer to the function used to insert a byte into a byte array */
	u_int16_t (*readUInt16)(void *,u_int32_t); /**< Pointer to the function used to read a 16 bit integer from the byte array */
	u_int32_t (*readUInt32)(void *,u_int32_t); /**< Pointer to the function used to read a 32 bit integer from the byte array */
	u_int64_t (*readUInt64)(void *,u_int32_t); /**< Pointer to the function used to read a 64 bit integer from the byte array */
	void (*reverse)(void *); /**< Pointer to the function used to reverse the byte array */
	void * (*subCopy)(void *,u_int32_t,u_int32_t); /**< Pointer to the function used to copy a byte array at an offset with for a length */
}CBByteArrayVT;

/**
 @brief Structure for CBByteArray objects. @see CBByteArray.h
*/
typedef struct{
	CBObject base; /**< CBObject base structure */
	CBSharedData * sharedData; /**< Underlying byte data */
	u_int32_t offset; /**< Offset from the begining of the byte data to the begining of this array */
	u_int32_t length; /**< Length of byte array. Set to CB_BYTE_ARRAY_UNKNOWN_LENGTH if unknown. */
	CBEvents * events;
} CBByteArray;

/**
 @brief Creates an empty CBByteArray object.
 @param size Size in bytes for the new array.
 @param events CBEngine for errors.
 @returns An empty CBByteArray object.
 */
CBByteArray * CBNewByteArrayOfSize(int size,CBEvents * events);
/**
 @brief References a subsection of an CBByteArray.
 @param self The CBByteArray object to reference.
 @param offset The offset to the start of the reference.
 @param length The length of the reference.
 @returns The new CBByteArray.
 */
CBByteArray * CBNewByteArraySubReference(CBByteArray * ref,int offset,int length);
/**
 @brief Creates a new CBByteArrayVT.
 @returns A new CBByteArrayVT.
 */
CBByteArrayVT * CBCreateByteArrayVT();
/**
 @brief Sets the CBByteArrayVT function pointers.
 @param VT The CBByteArrayVT to set.
 */
void CBSetByteArrayVT(CBByteArrayVT * VT);

/**
 @brief Gets the CBByteArrayVT. Use this to avoid casts.
 @param self The object to obtain the CBByteArrayVT from.
 @returns The CBByteArrayVT.
 */
CBByteArrayVT * CBGetByteArrayVT(void * self);

/**
 @brief Gets a CBByteArray from another object. Use this to avoid casts.
 @param self The object to obtain the CBByteArray from.
 @returns The CBByteArray object.
 */
CBByteArray * CBGetByteArray(void * self);

/**
 @brief Initialises an empty CBByteArray object
 @param self The CBByteArray object to initialise
 @param size Size in bytes for the new array.
 @param events CBEngine for errors.
 @returns true on success, false on failure.
 */
bool CBInitByteArrayOfSize(CBByteArray * self,int size,CBEvents * events);
/**
 @brief Initialises a reference CBByteArray to a subsection of an CBByteArray.
 @param ref The CBByteArray object to reference.
 @param offset The offset to the start of the reference.
 @param length The length of the reference.
 @returns true on success, false on failure.
 */
bool CBInitByteArraySubReference(CBByteArray * self,CBByteArray * ref,int offset,int length);
/**
 @brief Frees a CBByteArray object.
 @param self The CBByteArray object to free.
 */
void CBFreeByteArray(CBByteArray * self);

/**
 @brief Does the processing to free a CBByteArray object. Should be called by the children when freeing objects. This function will decrement the reference counter for the underlying shared data and free this data if no further CBByteArrays hold it.
 @param self The CBByteArray object to free.
 */
void CBFreeProcessByteArray(CBByteArray * self);
 
//  Functions

/**
 @brief Adjusts the length of the array by the adjustment amount. If adjusting by CB_BYTE_ARRAY_UNKNOWN_LENGTH the new length will be CB_BYTE_ARRAY_UNKNOWN_LENGTH.
 @param self The CBByteArray object.
 @param adjustment The amount to adjust the length by. Can be CB_BYTE_ARRAY_UNKOWN_LENGTH.
 */
void CBByteArrayAdjustLength(CBByteArray * self,int adjustment);

/**
 @brief Copies a CBByteArray
 @param self The CBByteArray object to copy
 @returns The new CBByteArray.
 */
CBByteArray * CBByteArrayCopy(CBByteArray * self);

/**
 @brief Get a byte from the CBByteArray object. A byte will be returned from self->offset+index in the underlying data.
 @param self The CBByteArray object.
 @param index The index in the array to get the byte from
 @returns The byte
 */
u_int8_t CBByteArrayGetByte(CBByteArray * self,int index);
/**
 @brief Get a pointer to the underlying data starting at self->offset.
 @param self The CBByteArray object.
 @returns The pointer
 */
u_int8_t * CBByteArrayGetData(CBByteArray * self);
/**
 @brief Insert a byte into array. This will be inserted at self->offset+index in the underlying data.
 @param self The CBByteArray object.
 @param index The index in the array to insert the byte
 @param byte The byte to be inserted.
 */
void CBByteArrayInsertByte(CBByteArray * self,int index,u_int8_t byte);
/**
 @brief Reads an unsigned 16 bit integer from a CBByteArray
 @param self The CBByteArray object
 @param offset Offset to where to start the read
 @returns An unsigned 16 bit integer.
 */
u_int16_t CBByteArrayReadUInt16(CBByteArray * self,int offset);
/**
 @brief Reads an unsigned 32 bit integer from a CBByteArray
 @param self The CBByteArray object
 @param offset Offset to where to start the read
 @returns An unsigned 32 bit integer.
 */
u_int32_t CBByteArrayReadUInt32(CBByteArray * self,int offset);
/**
 @brief Reads an unsigned 64 bit integer from the CBByteArray
 @param self The CBByteArray object
@param offset Offset to where to start the read
 @returns An unsigned 64 bit integer.
 */
u_int64_t CBByteArrayReadUInt64(CBByteArray * self,int offset);
/**
 @brief Reverses the bytes.
 @param self The CBByteArray object to reverse
 */
void CBByteArrayReverseBytes(CBByteArray * self);
/**
 @brief Copies a subsection of an CBByteArray.
 @param self The CBByteArray object to copy from.
 @param offset The offset to the start of the copy.
 @param length The length of the copy.
 @returns The new CBByteArray.
 */
CBByteArray * CBByteArraySubCopy(CBByteArray * self,int offset,int length);

#endif