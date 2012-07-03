//
//  CBByteArray.h
//  cbitcoin
//
//  Created by Matthew Mitchell on 30/04/2012.
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
CBByteArray * CBNewByteArrayOfSize(u_int32_t size,CBEvents * events);
/**
 @brief Creates a CBByteArray object referencing another CBByteArrayObject
 @param ref The CBByteArray to reference.
 @param offset The offset to the start of the reference in the reference CBByteArray.
 @param length The length of the new CBByteArray. If 0 the length is set to be the same as the reference CBByteArray.
 @returns An empty CBByteArray object.
 */
CBByteArray * CBNewByteArraySubReference(CBByteArray * ref,u_int32_t offset,u_int32_t length);
/**
 @brief Creates a new CBByteArray using data.
 @param data The data. This should be dynamically allocated. The new CBByteArray object will take care of it's memory management so do not free this data once passed into this constructor.
 @param size Size in bytes for the new array.
 @param events CBEngine for errors.
 @returns The new CBByteArray object.
 */
CBByteArray * CBNewByteArrayWithData(u_int8_t * data,u_int32_t size,CBEvents * events);
/**
 @brief Creates a new CBByteArray using data which is copied.
 @param data The data. This data is copied.
 @param size Size in bytes for the new array.
 @param events CBEngine for errors.
 @returns The new CBByteArray object.
 */
CBByteArray * CBNewByteArrayWithDataCopy(u_int8_t * data,u_int32_t size,CBEvents * events);

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
bool CBInitByteArrayOfSize(CBByteArray * self,u_int32_t size,CBEvents * events);
/**
 @brief Initialises a reference CBByteArray to a subsection of an CBByteArray.
 @param self The CBByteArray object to initialise.
 @param ref The CBByteArray object to reference.
 @param offset The offset to the start of the reference.
 @param length The length of the reference. If 0 the length is set to be the same as the reference CBByteArray.
 @returns true on success, false on failure.
 */
bool CBInitByteArraySubReference(CBByteArray * self,CBByteArray * ref,u_int32_t offset,u_int32_t length);
/**
 @brief Creates a new CBByteArray using data.
 @param self The CBByteArray object to initialise
 @param data The data. This should be dynamically allocated. The new CBByteArray object will take care of it's memory management so do not free this data once passed into this constructor.
 @param size Size in bytes for the new array.
 @param events CBEngine for errors.
 @returns true on success, false on failure.
 */
bool CBInitByteArrayWithData(CBByteArray * self,u_int8_t * data,u_int32_t size,CBEvents * events);
/**
 @brief Creates a new CBByteArray using data which is copied.
 @param self The CBByteArray object to initialise
 @param data The data. This data is copied.
 @param size Size in bytes for the new array.
 @param events CBEngine for errors.
 @returns true on success, false on failure.
 */
bool CBInitByteArrayWithDataCopy(CBByteArray * self,u_int8_t * data,u_int32_t size,CBEvents * events);

/**
 @brief Frees a CBByteArray object.
 @param self The CBByteArray object to free.
 */
void CBFreeByteArray(void * self);

/**
 @brief Releases a reference to shared byte data and frees the data if necessary.
 @param self The CBByteArray object with the CBSharedData
 */
void CBByteArrayReleaseSharedData(CBByteArray * self);
 
//  Functions

/**
 @brief Copies a CBByteArray
 @param self The CBByteArray object to copy
 @returns The new CBByteArray.
 */
CBByteArray * CBByteArrayCopy(CBByteArray * self);
/**
 @brief Copies another byte array to this byte array.
 @param self The CBByteArray object.
 @param writeOffset The offset to begin writing
 @param source The CBByteArray to copy from.
 */
void CBByteArrayCopyByteArray(CBByteArray * self,u_int32_t writeOffset,CBByteArray * source);
/**
 @brief Copies a section of another byte array to this byte array.
 @param self The CBByteArray object.
 @param writeOffset The offset to begin writing
 @param source The CBByteArray to copy from.
 @param readOffset The offset of the source array to begin reading.
 @param length The length to copy.
 */
void CBByteArrayCopySubByteArray(CBByteArray * self,u_int32_t writeOffset,CBByteArray * source,u_int32_t readOffset,u_int32_t length);
/**
 @brief Compares a CBByteArray to another CBByteArray and returns true if equal.
 @param self The CBByteArray object to compare
 @param second Another CBByteArray object to compare with
 @returns True if equal, false if not equal.
 */
bool CBByteArrayEquals(CBByteArray * self,CBByteArray * second);
/**
 @brief Get a byte from the CBByteArray object. A byte will be returned from self->offset+index in the underlying data.
 @param self The CBByteArray object.
 @param index The index in the array to get the byte from
 @returns The byte
 */
u_int8_t CBByteArrayGetByte(CBByteArray * self,u_int32_t index);
/**
 @brief Get a pointer to the underlying data starting at self->offset.
 @param self The CBByteArray object.
 @returns The pointer
 */
u_int8_t * CBByteArrayGetData(CBByteArray * self);
/**
 @brief Get the last byte from the CBByteArray object. A byte will be returned from self->offset+self->length in the underlying data.
 @param self The CBByteArray object.
 @returns The last byte
 */
u_int8_t CBByteArrayGetLastByte(CBByteArray * self);
/**
 @brief Set a byte into the array. This will be set at self->offset+index in the underlying data.
 @param self The CBByteArray object.
 @param index The index in the array to set the byte
 @param byte The byte to be set.
 */
void CBByteArraySetByte(CBByteArray * self,u_int32_t index,u_int8_t byte);
/**
 @brief Copies a length of bytes into the array. This will be set at self->offset+index in the underlying data.
 @param self The CBByteArray object.
 @param index The index in the array to start writing.
 @param bytes The pointer to the bytes to be copied.
 @param length The number of bytes to copy.
 */
void CBByteArraySetBytes(CBByteArray * self,u_int32_t index,u_int8_t * bytes,u_int32_t length);
/**
 @brief Writes an unsigned 16 bit integer from a CBByteArray as little-endian
 @param self The CBByteArray object
 @param offset Offset to where to start the write
 @param integer The 16 bit integer to set.
 */
void CBByteArraySetUInt16(CBByteArray * self,u_int32_t offset,u_int16_t integer);
/**
 @brief Writes an unsigned 32 bit integer from a CBByteArray as little-endian
 @param self The CBByteArray object
 @param offset Offset to where to start the write
  @param integer The 32 bit integer to set.
 */
void CBByteArraySetUInt32(CBByteArray * self,u_int32_t offset,u_int32_t integer);
/**
 @brief Writes an unsigned 64 bit integer from the CBByteArray as little-endian
 @param self The CBByteArray object
 @param offset Offset to where to start the write
 @param integer The 64 bit integer to set.
 */
void CBByteArraySetUInt64(CBByteArray * self,u_int32_t offset,u_int64_t integer);
/**
 @brief Reads an unsigned 16 bit integer from a CBByteArray as little-endian
 @param self The CBByteArray object
 @param offset Offset to where to start the read
 @returns An unsigned 16 bit integer.
 */
u_int16_t CBByteArrayReadUInt16(CBByteArray * self,u_int32_t offset);
/**
 @brief Reads an unsigned 32 bit integer from a CBByteArray as little-endian
 @param self The CBByteArray object
 @param offset Offset to where to start the read
 @returns An unsigned 32 bit integer.
 */
u_int32_t CBByteArrayReadUInt32(CBByteArray * self,u_int32_t offset);
/**
 @brief Reads an unsigned 64 bit integer from the CBByteArray as little-endian
 @param self The CBByteArray object
@param offset Offset to where to start the read
 @returns An unsigned 64 bit integer.
 */
u_int64_t CBByteArrayReadUInt64(CBByteArray * self,u_int32_t offset);
/**
 @brief Reverses the bytes.
 @param self The CBByteArray object to reverse
 */
void CBByteArrayReverseBytes(CBByteArray * self);
/**
 @brief Changes the reference of this CBByteArray object to reference the underlying data of another CBByteArray. Useful for moving byte data into single underlying data by copying the data into a larger CBByteArray and then changing the reference to this new larger CBByteArray.
 @param self The CBByteArray object to change the reference for.
 @param ref The CBByteArray object to get the reference from.
 @param offset The offset to start the reference.
 @returns The new CBByteArray.
 */
void CBByteArrayChangeReference(CBByteArray * self,CBByteArray * ref,u_int32_t offset);
/**
 @brief Copies a subsection of a CBByteArray.
 @param self The CBByteArray object to copy from.
 @param offset The offset to the start of the copy.
 @param length The length of the copy.
 @returns The new CBByteArray.
 */
CBByteArray * CBByteArraySubCopy(CBByteArray * self,u_int32_t offset,u_int32_t length);
/**
 @brief References a subsection of a CBByteArray.
 @param self The CBByteArray object to reference.
 @param offset The offset to the start of the reference.
 @param length The length of the reference.
 @returns The new CBByteArray.
 */
CBByteArray * CBByteArraySubReference(CBByteArray * self,u_int32_t offset,u_int32_t length);

#endif
