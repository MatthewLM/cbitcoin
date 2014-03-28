//
//  CBBigInt.h
//  cbitcoin
//
//  Created by Matthew Mitchell on 28/04/2012.
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
 @brief Functions for using bytes as if they were an integer
 */

#ifndef CBBIGINTH
#define CBBIGINTH

#include "CBConstants.h"
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>

/**
 @brief Contains byte data with the length of this data to represent a large integer. The byte data is in little-endian which stores the smallest byte first.
 */
typedef struct{
	uint8_t * data; /**< The byte data. Should be little-endian */
	uint8_t length; /**< The length of this data in bytes */
	uint8_t allocLen; /**< The length of the data allocated */
}CBBigInt;

/**
 @brief Pre-allocates data for a CBBigInt
 @param bi The CBBigInt.
 @param allocLen The length to allocate in bytes.
 */
void CBBigIntAlloc(CBBigInt * bi, uint8_t allocLen);

/**
 @brief Compares a CBBigInt to an 8 bit integer. You can replicate "a op 58" as "CBBigIntCompareToUInt8(a, 58) op 0" replacing "op" with a comparison operator.
 @param a The first CBBigInt
 @returns The result of the comparison as a CBCompare constant. Returns what a is in relation to b.
 */
CBCompare CBBigIntCompareTo58(CBBigInt * a);

/**
 @brief Compares two CBBigInt. You can replicate "a op b" as "CBBigIntCompare(a, b) op 0" replacing "op" with a comparison operator.
 @param a The first CBBigInt
 @param b The second CBBigInt
 @returns The result of the comparison as a CBCompare constant. Returns what a is in relation to b.
 */
CBCompare CBBigIntCompareToBigInt(CBBigInt * a, CBBigInt * b);

/**
 @brief Calculates the result of an addition of a CBBigInt structure by another CBBigInt structure and the first CBBigInt becomes this new figure. Like "a += b".
 @param a A pointer to the CBBigInt
 @param b A pointer to the second CBBigInt
 */
void CBBigIntEqualsAdditionByBigInt(CBBigInt * a, CBBigInt * b);

/**
 @brief Calculates the result of a division of a CBBigInt structure by 58 and the CBBigInt becomes this new figure. Like "a /= 58".
 @param a A pointer to the CBBigInt
 @param ans A memory block the same size as the CBBigInt data memory block to store temporary data in calculations. Should be set with zeros.
 */
void CBBigIntEqualsDivisionBy58(CBBigInt * a, uint8_t * ans);

/**
 @brief Calculates the result of a multiplication of a CBBigInt structure by an 8 bit integer and the CBBigInt becomes this new figure. Like "a *= b".
 @param a A pointer to the CBBigInt
 @param b An 8 bit integer
 @returns true on success, false on failure
 */
void CBBigIntEqualsMultiplicationByUInt8(CBBigInt * a, uint8_t b);

/**
 @brief Calculates the result of a subtraction of a CBBigInt structure with another CBBigInt structure and the CBBigInt becomes this new figure. Like "a -= b".
 @param a A pointer to a CBBigInt
 @param b A pointer to a CBBigInt
 */
void CBBigIntEqualsSubtractionByBigInt(CBBigInt * a, CBBigInt * b);

/**
 @brief Calculates the result of a subtraction of a CBBigInt structure by an 8 bit integer and the CBBigInt becomes this new figure. Like "a -= b".
 @param a A pointer to the CBBigInt
 @param b An 8 bit integer
 */
void CBBigIntEqualsSubtractionByUInt8(CBBigInt * a, uint8_t b);

/**
 @brief Assigns a CBBigInt as the exponentiation of an unsigned 8 bit intger with another unsigned 8 bit integer. Like "a^b". Data must be freed.
 @param bi The CBBigInt. Preallocate this with at least one byte.
 @param a The base
 @param b The exponent.
 */
void CBBigIntFromPowUInt8(CBBigInt * bi, uint8_t a, uint8_t b);

/**
 @brief Returns the result of a modulo of a CBBigInt structure and 58. Like "a % 58".
 @param a The CBBigInt
 @returns The result of the modulo operation as an 8 bit integer.
 */
uint8_t CBBigIntModuloWith58(CBBigInt * a);

/**
 @brief Normalises a CBBigInt so that there are no unnecessary trailing zeros.
 @param a A pointer to the CBBigInt
 */
void CBBigIntNormalise(CBBigInt * a);

/**
 @brief Reallocates the CBBigInt if it is needed.
 @param bi The CBBigInt.
 @param allocLen The length of allocated data required.
 */
void CBBigIntRealloc(CBBigInt * bi, uint8_t allocLen);

#endif
