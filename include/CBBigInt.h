//
//  CBBigInt.h
//  cbitcoin
//
//  Created by Matthew Mitchell on 28/04/2012.
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
 @returns true on success, false on failure.
 */
bool CBBigIntAlloc(CBBigInt * bi, uint8_t allocLen);
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
 @returns true on success, false on failure.
 */
bool CBBigIntEqualsAdditionByBigInt(CBBigInt * a, CBBigInt * b);
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
bool CBBigIntEqualsMultiplicationByUInt8(CBBigInt * a, uint8_t b);
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
 @returns true on success, false on failure.
 */
bool CBBigIntFromPowUInt8(CBBigInt * bi, uint8_t a, uint8_t b);
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
 @returns true on success, false on failure.
 */
bool CBBigIntRealloc(CBBigInt * bi, uint8_t allocLen);

#endif
