//
//  CBByteArray.h
//  cbitcoin
//
//  Created by Matthew Mitchell on 30/04/2012.
//  Copyright (c) 2012 Matthew Mitchell
//  
//  This file is part of cbitcoin. It is subject to the license terms
//  in the LICENSE file found in the top-level directory of this
//  distribution and at http://www.cbitcoin.com/license.html. No part of
//  cbitcoin, including this file, may be copied, modified, propagated,
//  or distributed except according to the terms contained in the
//  LICENSE file.

/**
 @file
 @brief Provides an interface to a memory block as bytes. Many CBByteArrays can refer to the same memory block with different offsets and lengths. Inherits CBObject
*/

#ifndef CBBYTEARRAYH
#define CBBYTEARRAYH

//  Includes

#include "CBObject.h"
#include "CBDependencies.h"
#include "CBVarInt.h"
#include "CBSanitiseOutput.h"
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

// Getter

#define CBGetByteArray(x) ((CBByteArray *)x)

/**
 @brief Stores byte data that can be shared amongst many CBByteArrays
 */
typedef struct{
	uint8_t * data; /**< Pointer to byte data */
	uint32_t references; /**< References to this data */
}CBSharedData;

/**
 @brief Structure for CBByteArray objects. @see CBByteArray.h
*/
typedef struct{
	CBObject base; /**< CBObject base structure */
	CBSharedData * sharedData; /**< Underlying byte data */
	uint32_t offset; /**< Offset from the begining of the byte data to the begining of this array */
	uint32_t length; /**< Length of byte array. */
} CBByteArray;

/**
 @brief Creates a CBByteArray object from a C string. The termination character is not included in the new CBByteArray.
 @param string The string to put into a CBByteArray.
 @param terminator If true, the termination character is included in the CBByteArray.
 @returns A new CBByteArray object.
 */
CBByteArray * CBNewByteArrayFromString(char * string, bool terminator);

/**
 @brief Creates an empty CBByteArray object.
 @param size Size in bytes for the new array. Can be zero.
 @returns An empty CBByteArray object.
 */
CBByteArray * CBNewByteArrayOfSize(uint32_t size);

/**
 @brief Creates a CBByteArray object referencing another CBByteArrayObject
 @param ref The CBByteArray to reference.
 @param offset The offset to the start of the reference in the reference CBByteArray.
 @param length The length of the new CBByteArray. If 0 the length is set to be the same as the reference CBByteArray.
 @returns An empty CBByteArray object.
 */
CBByteArray * CBNewByteArraySubReference(CBByteArray * ref, uint32_t offset, uint32_t length);

/**
 @brief Creates a new CBByteArray using data.
 @param data The data. This should be dynamically allocated. The new CBByteArray object will take care of it's memory management so do not free this data once passed into this constructor.
 @param size Size in bytes for the new array.
 @returns The new CBByteArray object.
 */
CBByteArray * CBNewByteArrayWithData(uint8_t * data, uint32_t size);

/**
 @brief Creates a new CBByteArray using data which is copied.
 @param data The data. This data is copied.
 @param size Size in bytes for the new array.
 @returns The new CBByteArray object.
 */
CBByteArray * CBNewByteArrayWithDataCopy(uint8_t * data, uint32_t size);
CBByteArray * CBNewByteArrayFromHex(char * hex);

/**
 @brief Initialises a CBByteArray object from a C string. The termination character is not included in the new CBByteArray.
 @param self The CBByteArray object to initialise
 @param string The string to put into a CBByteArray.
 @param terminator If true, include the termination character.
 */
void CBInitByteArrayFromString(CBByteArray * self, char * string, bool terminator);

/**
 @brief Initialises an empty CBByteArray object
 @param self The CBByteArray object to initialise
 @param size Size in bytes for the new array.
 */
void CBInitByteArrayOfSize(CBByteArray * self, uint32_t size);

/**
 @brief Initialises a reference CBByteArray to a subsection of an CBByteArray.
 @param self The CBByteArray object to initialise.
 @param ref The CBByteArray object to reference.
 @param offset The offset to the start of the reference.
 @param length The length of the reference.
 */
void CBInitByteArraySubReference(CBByteArray * self, CBByteArray * ref, uint32_t offset, uint32_t length);

/**
 @brief Creates a new CBByteArray using data.
 @param self The CBByteArray object to initialise
 @param data The data. This should be dynamically allocated. The new CBByteArray object will take care of it's memory management so do not free this data once passed into this constructor.
 @param size Size in bytes for the new array.
 */
void CBInitByteArrayWithData(CBByteArray * self, uint8_t * data, uint32_t size);

/**
 @brief Creates a new CBByteArray using data which is copied.
 @param self The CBByteArray object to initialise
 @param data The data. This data is copied.
 @param size Size in bytes for the new array.
 */
void CBInitByteArrayWithDataCopy(CBByteArray * self, uint8_t * data, uint32_t size);
void CBInitByteArrayFromHex(CBByteArray * self, char * hex);

/**
 @brief Releases and frees all fo the objects stored by the CBByteArray object.
 @param self The CBByteArray object to destroy.
 */
void CBDestroyByteArray(void * self);

/**
 @brief Frees a CBByteArray object and also calls CBDestroyByteArray.
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
 @brief Compares a CBByteArray to another CBByteArray and returns with a CBCompare value.
 @param self The CBByteArray object to compare
 @param second Another CBByteArray object to compare with
 @returns If the lengths are different, CB_COMPARE_MORE_THAN if "self" if longer, else CB_COMPARE_LESS_THAN. If the bytes are equal CB_COMPARE_EQUAL, else CB_COMPARE_MORE_THAN if the first different byte if higher in "self", otherwise CB_COMPARE_LESS_THAN. The return value can be treated like the return value to memcmp.
 */
CBCompare CBByteArrayCompare(CBByteArray * self, CBByteArray * second);

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
void CBByteArrayCopyByteArray(CBByteArray * self, uint32_t writeOffset, CBByteArray * source);

/**
 @brief Copies a section of another byte array to this byte array.
 @param self The CBByteArray object.
 @param writeOffset The offset to begin writing
 @param source The CBByteArray to copy from.
 @param readOffset The offset of the source array to begin reading.
 @param length The length to copy.
 */
void CBByteArrayCopySubByteArray(CBByteArray * self, uint32_t writeOffset, CBByteArray * source, uint32_t readOffset, uint32_t length);

/**
 @brief Get a byte from the CBByteArray object. A byte will be returned from self->offset+index in the underlying data.
 @param self The CBByteArray object.
 @param index The index in the array to get the byte from
 @returns The byte
 */
uint8_t CBByteArrayGetByte(CBByteArray * self, uint32_t index);

/**
 @brief Get a pointer to the underlying data starting at self->offset.
 @param self The CBByteArray object.
 @returns The pointer
 */
uint8_t * CBByteArrayGetData(CBByteArray * self);

/**
 @brief Get the last byte from the CBByteArray object. A byte will be returned from self->offset+self->length in the underlying data.
 @param self The CBByteArray object.
 @returns The last byte
 */
uint8_t CBByteArrayGetLastByte(CBByteArray * self);

/**
 @brief Determines if a CBByteArray is null.
 @param self The CBByteArray object.
 @returns true if all bytes are zero, else false.
 */
bool CBByteArrayIsNull(CBByteArray * self);

void CBByteArraySanitise(CBByteArray * self);

/**
 @brief Set a byte into the array. This will be set at self->offset+index in the underlying data.
 @param self The CBByteArray object.
 @param index The index in the array to set the byte
 @param byte The byte to be set.
 */
void CBByteArraySetByte(CBByteArray * self, uint32_t index, uint8_t byte);

/**
 @brief Copies a length of bytes into the array. This will be set at self->offset+index in the underlying data.
 @param self The CBByteArray object.
 @param index The index in the array to start writing.
 @param bytes The pointer to the bytes to be copied.
 @param length The number of bytes to copy.
 */
void CBByteArraySetBytes(CBByteArray * self, uint32_t index, uint8_t * bytes, uint32_t length);

/**
 @brief Writes a 16 bit integer to a CBByteArray as little-endian
 @param self The CBByteArray object
 @param offset Offset to where to start the write
 @param integer The 16 bit integer to set. The argument is an unsigned integer but signed or unsigned integers are OK to pass.
 */
void CBByteArraySetInt16(CBByteArray * self, uint32_t offset, uint16_t integer);

/**
 @brief Writes a 32 bit integer to a CBByteArray as little-endian
 @param self The CBByteArray object
 @param offset Offset to where to start the write
  @param integer The 32 bit integer to set. The argument is an unsigned integer but signed or unsigned integers are OK to pass.
 */
void CBByteArraySetInt32(CBByteArray * self, uint32_t offset, uint32_t integer);

/**
 @brief Writes a 64 bit integer to a CBByteArray as little-endian
 @param self The CBByteArray object
 @param offset Offset to where to start the write
 @param integer The 64 bit integer to set. The argument is an unsigned integer but signed or unsigned integers are OK to pass.
 */
void CBByteArraySetInt64(CBByteArray * self, uint32_t offset, uint64_t integer);

/**
 @brief Writes a 16 bit integer to a CBByteArray as big-endian for port numbers.
 @param self The CBByteArray object
 @param offset Offset to where to start the write
 @param integer The 16 bit integer to set.
 */
void CBByteArraySetPort(CBByteArray * self, uint32_t offset, uint16_t integer);

/**
 @brief Writes a variable size integer into the CBByteArray.
 @param self The byte array to encode a variable size integer into.
 @param offset Offset to start encoding to.
 @param varInt Variable integer structure to write as variable integer data.
 */
void CBByteArraySetVarInt(CBByteArray * self, uint32_t offset, CBVarInt varInt);

/**
 @brief Reads a 16 bit integer from a CBByteArray as little-endian
 @param self The CBByteArray object
 @param offset Offset to where to start the read
 @returns A 16 bit integer. This can be cast to a signed integer if reading integer as a signed value.
 */
uint16_t CBByteArrayReadInt16(CBByteArray * self, uint32_t offset);

/**
 @brief Reads a 32 bit integer from a CBByteArray as little-endian
 @param self The CBByteArray object
 @param offset Offset to where to start the read
 @returns A 32 bit integer. This can be cast to a signed integer if reading integer as a signed value
 */
uint32_t CBByteArrayReadInt32(CBByteArray * self, uint32_t offset);

/**
 @brief Reads a 64 bit integer from the CBByteArray as little-endian
 @param self The CBByteArray object
 @param offset Offset to where to start the read
 @returns A 64 bit integer. This can be cast to a signed integer if reading integer as a signed value
 */
uint64_t CBByteArrayReadInt64(CBByteArray * self, uint32_t offset);

/**
 @brief Reads a 16 bit integer from the CBByteArray as big-endian for port numbers.
 @param self The CBByteArray object
 @param offset Offset to where to start the read
 @returns A 16 bit integer.
 */
uint16_t CBByteArrayReadPort(CBByteArray * self, uint32_t offset);

/**
 @brief Reads a variable size integer from a byte array into a CBVarInt structure.
 @param bytes The self array to read a variable size integer from.
 @param offset Offset to start reading from.
 @returns The CBVarInt information
 */
CBVarInt CBByteArrayReadVarInt(CBByteArray * self, uint32_t offset);
uint8_t CBByteArrayReadVarIntSize(CBByteArray * self, uint32_t offset);

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
void CBByteArrayChangeReference(CBByteArray * self, CBByteArray * ref, uint32_t offset);

/**
 @brief Copies a subsection of a CBByteArray.
 @param self The CBByteArray object to copy from.
 @param offset The offset to the start of the copy.
 @param length The length of the copy.
 @returns The new CBByteArray.
 */
CBByteArray * CBByteArraySubCopy(CBByteArray * self, uint32_t offset, uint32_t length);

/**
 @brief References a subsection of a CBByteArray.
 @param self The CBByteArray object to reference.
 @param offset The offset to the start of the reference.
 @param length The length of the reference.
 @returns The new CBByteArray.
 */
CBByteArray * CBByteArraySubReference(CBByteArray * self, uint32_t offset, uint32_t length);
void CBByteArrayToString(CBByteArray * self, uint32_t offset, uint32_t length, char * output, bool backwards);
bool CBStrHexToBytes(char * hex, uint8_t * output);

#endif
