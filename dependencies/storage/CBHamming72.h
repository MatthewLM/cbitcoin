//
//  CBHamming72.h
//  cbitcoin
//
//  Created by Matthew Mitchell on 14/12/2012.
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
 @brief Contains an encoder and decoder for hamming (72,64) SECDED codes.
 */

#ifndef CBHAMMING72H
#define CBHAMMING72H

#include "CBConstants.h"
#include <stdbool.h>
#include <stdlib.h>

/**
 @brief The result of CBHamming72Check
 */
typedef struct{
	bool err; /**< True when an unrecoverable double bit error has been detected or on a memory error. */
	uint32_t numCorrections; /**< The number of corrections made. */
	uint32_t * corrections; /**< A newly allocated list of which sections have been corrected with the number of the byte, starting at zero. For instance if an error was correted in the 3rd byte of the second section the number would be 10. If it begins with the first bit set ( > 0x7F) then except the first bit, it is the number of the parity byte which has been corrected. NULL if none. */
} CBHamming72Result;

/**
 @brief Checks the parity bits against the data and corrects single bit errors or detects two bit errors.
 @param data A pointer to the data bytes.
 @param dataLen The length of the data.
 @param parityBits A pointer to bytes to the parity bits.
 @returns @see CBHamming72Result
 */
CBHamming72Result CBHamming72Check(uint8_t * data, uint32_t dataLen, uint8_t * parityBits);
/**
 @brief Encodes parity bits for hamming (72,64) SECDED codes.
 @param data A pointer to the data bytes.
 @param dataLen The length of the data to encode.
 @param parityBits A pointer to bytes to encode the parity bits. This should be 1 byte for every 8 bytes of data.
 */
void CBHamming72Encode(uint8_t * data, uint32_t dataLen, uint8_t * parityBits);
/**
 @brief Encodes parity bits for hamming (72,64) SECDED codes for a 64 bit segment of data.
 @param data A pointer to the data bytes.
 @param dataLen The length of the data to encode, must be 8 or less.
 @param parityBits A pointer to a byte to encode the parity bits.
 */
void CBHamming72EncodeSeqment(uint8_t * data, uint32_t dataLen, uint8_t * parityBits);

#endif
