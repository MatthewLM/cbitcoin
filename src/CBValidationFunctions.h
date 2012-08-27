//
//  CBValidationFunctions.c
//  cbitcoin
//
//  Created by Matthew Mitchell on 26/08/2012.
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

#ifndef CBVALIDATIONFUNCTIONSH
#define CBVALIDATIONFUNCTIONSH

#include <stdint.h>
#include "CBConstants.h"
#include "CBBlock.h"

/**
 @brief Recalcultes the target for every 2016 blocks.
 @param oldTarget The old target in compact form.
 @param time The time between the newer block and older block for recalulating the target.
 @returns the new target in compact form.
 */
uint32_t CBCalculateTarget(uint32_t oldTarget, uint32_t time);
/**
 @brief Validates proof of work.
 @brief hash Block hash.
 @returns true if the validation passed, false otherwise.
 */
bool CBValidateProofOfWork(CBByteArray * hash,uint32_t target);

#endif
