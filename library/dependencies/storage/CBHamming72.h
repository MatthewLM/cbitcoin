//
//  CBHamming72.h
//  cbitcoin
//
//  Created by Matthew Mitchell on 24/12/2012.
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
 @brief Contains an encoder and decoder for hamming (72, 64) SECDED codes.
 */

#ifndef CBHAMMING72H
#define CBHAMMING72H

#include "CBConstants.h"
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>

#define CB_DOUBLE_BIT_ERROR 9
#define CB_ZERO_BIT_ERROR 10

/**
 @brief Checks the parity bits against the data and corrects single bit errors or detects two bit errors.
 @param data A pointer to the data bytes. Should be no more than eight bytes of data followed by another byte for the parity bits.
 @param dataLen The length of the data, not including the parity byte.
 @returns CB_DOUBLE_BIT_ERROR if a double error was detected, CB_ZERO_BIT_ERROR if no errors were detected or the index of the byte which was corrected.
 */
uint8_t CBHamming72Check(uint8_t * data, uint32_t dataLen);

/**
 @brief Encodes parity bits for hamming (72, 64) SECDED codes.
 @param data A pointer to the data bytes. Should be no more than eight bytes of data followed by another byte to set the parity bits.
 @param dataLen The length of the data to encode, not including the parity byte.
 @param parityByte A pointer to the byte holding parity bits, to be set.
 */
void CBHamming72Encode(uint8_t * data, uint32_t dataLen, uint8_t * parityByte);

#endif
