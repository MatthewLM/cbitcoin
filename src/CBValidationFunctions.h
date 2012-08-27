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
 @brief Calculates the merkle root from a list of hashes.
 @param hashes The hashes stored as continuous byte data with each 32 byte hash after each other. The data pointed to by "hashes" will be modified and will result in the merkle root as the first 32 bytes.
 @param hashNum The number of hashes in the memory block
 */
void CBCalculateMerkleRoot(uint8_t * hashes,uint32_t hashNum);
/**
 @brief Recalcultes the target for every 2016 blocks.
 @param oldTarget The old target in compact form.
 @param time The time between the newer block and older block for recalulating the target.
 @returns the new target in compact form.
 */
uint32_t CBCalculateTarget(uint32_t oldTarget, uint32_t time);
/**
 @brief Returns the number of sigops from a transaction but does not include P2SH scripts. P2SH sigop counting requires that the previous output is known to be a P2SH output.
 @param tx The transaction.
 @returns the number of sigops for validation.
 */
uint32_t CBTransactionGetSigOps(CBTransaction * tx);
/**
 @brief Validates a transaction has outputs and inputs, is below the maximum block size, has outputs that do not overflow and that there are no duplicate spent transaction outputs. This function also checks coinbase transactions have input scripts between 2 and 100 bytes and non-coinbase transactions do not have a NULL previous output. Further validation can be done by checking the transaction against input transactions. With simplified payment verification instead the validation is done through trusting miners but the basic validation can still be done for these basic checks.
 @param tx The transaction to validate. This should be deserialised.
 @param coinbase true to validate for a coinbase transaction, false to validate for a non-coinbase transaction.
 @returns A memory block of previous transaction ouputs spent by this transaction if validation passed or NULL. The outputs can be used for further validation that the transaction is not a double-spend. The block of CBPrevOuts needs to be freed. The hashes are not retained, so the data is good until the transaction is freed. Do not try to release the hashes, thinking they have been retained in this function.
 */
CBPrevOut * CBTransactionValidateBasic(CBTransaction * tx, bool coinbase);
/**
 @brief Validates proof of work.
 @brief hash Block hash.
 @returns true if the validation passed, false otherwise.
 */
bool CBValidateProofOfWork(CBByteArray * hash,uint32_t target);

#endif
