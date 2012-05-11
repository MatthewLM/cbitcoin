//
//  CBBigInt.h
//  cbitcoin
//
//  Created by Matthew Mitchell on 28/04/2012.
//  Last modified by Matthew Mitchell on 11/05/2012.
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

/**
 @brief Contains byte data with the length of this data to represent a large integer. The byte data is in little-endian which stores the smallest byte first. 
 */
typedef struct{
	u_int8_t * data; /**< The byte data. Should be little-endian */
	u_int8_t length; /**< The length of this data in bytes */
}CBBigInt;

/**
 @brief For a power of two, determine the the log2. Eg. 8 would be 3
 @param b An 8 bit integer
 @returns An 8 bit integer
 */
u_int8_t CBPowerOf2Log2(u_int8_t a);
/**
 @brief For an 8 bit integer determine the the log2 and floor it. Eg. 23 would be 4
 @param b An 8 bit integer
 @returns An 8 bit integer
 */
u_int8_t CBFloorLog2(u_int8_t a);
/**
 @brief For an 8 bit integer, determine the the position of the first bit. Eg 00010101 would give 4
 @param b An 8 bit integer
 @returns An 8 bit integer
 */
u_int8_t CBBase2MSBPos(u_int8_t a);
/**
 @brief Compares two CBBigInt. You can replicate "a op b" as "CBBigIntCompare(a,b) op 0" replacing "op" with a comparison operator.
 @param a The first CBBigInt
 @param b The second CBBigInt
 @returns The result of the comparison as a CBCompare constant. Returns what a is in relation to b.
 */
CBCompare CBBigIntCompare(CBBigInt a,CBBigInt b);
/**
 @brief Compares a CBBigInt to an 8 bit integer. You can replicate "a op b" as "CBBigIntCompareToUInt8(a,b) op 0" replacing "op" with a comparison operator.
 @param a The first CBBigInt
 @param b An 8 bit integer
 @returns The result of the comparison as a CBCompare constant. Returns what a is in relation to b.
 */
CBCompare CBBigIntCompareToUInt8(CBBigInt a,u_int8_t b);
/**
 @brief Calculates the result of a division of a CBBigInt structure by an 8 bit integer and the CBBigInt becomes this new figure. Like "a /= b".
 @param a A pointer to the CBBigInt
 @param b An 8 bit integer
 @param ans A memory block the same size as the CBBigInt data memory block to store temporary data in calculations. Should be set with zeros.
 */
void CBBigIntEqualsDivisionByUInt8(CBBigInt * a,u_int8_t b,u_int8_t * ans);
/**
 @brief Calculates the result of a right bit shift of a CBBigInt structure by an 8 bit integer and the CBBigInt becomes this new figure. Like "a >>= b".
 @param a A pointer to the CBBigInt
 @param b An 8 bit integer
 */
void CBBigIntEqualsRightShiftByUInt8(CBBigInt * a,u_int8_t b);
/**
 @brief Calculates the result of a multiplication of a CBBigInt structure by an 8 bit integer and the CBBigInt becomes this new figure. Like "a *= b".
 @param a A pointer to the CBBigInt
 @param b An 8 bit integer
 @param ans A memory block the same size as the CBBigInt data memory block to store temporary data in calculations. Should be set with zeros.
 */
void CBBigIntEqualsMultiplicationByUInt8(CBBigInt * a,u_int8_t b,u_int8_t * ans);
/**
 @brief Calculates the result of a subtraction of a CBBigInt structure by an 8 bit integer and the CBBigInt becomes this new figure. Like "a -= b".
 @param a A pointer to the CBBigInt
 @param b An 8 bit integer
 */
void CBBigIntEqualsSubtractionByUInt8(CBBigInt * a,u_int8_t b);
/**
 @brief Returns the result of a modulo of a CBBigInt structure and an 8 bit integer. Like "a % b".
 @param a The CBBigInt
 @param b An 8 bit integer
 @returns The result of the modulo operation as an 8 bit integer.
 */
u_int8_t CBBigIntModuloWithUInt8(CBBigInt a,u_int8_t b);
/**
 @brief Makes a new CBBigInt from an exponentiation of an unsigned 8 bit intger with another unsigned 8 bit integer. Like "a^b".
 @param a The base
 @param b The exponent.
 @returns The new CBBigInt. Free the CBBigInt data when done.
 */
CBBigInt CBBigIntFromPowUInt8(u_int8_t a,u_int8_t b);
/**
 @brief Normalises a CBBigInt so that there are no uneccessary trailing zeros.
 @param a A pointer to the CBBigInt
 */
void CBBigIntNormalise(CBBigInt * a);

#endif
