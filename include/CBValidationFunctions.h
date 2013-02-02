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

/**
 @file
 @brief Here are functions for doing parts of bitcoin validation. cbitcoin is designed to make a broad variety of bitcoin development easier and is not designed to implement a client node. Therefore, cbitcoin does not provide provide functions for validating block chains as there are different methods for doing so. So wether you want to use SPV, block-chain pruning, full validation or whatever, you can use these functions to help. Also see CBScript.h and CBMerkleNode.h for validation. CBNetworkCommunicator.h has message size checks and message deserialisation functions check messages for syntax and sizes.
 */

#ifndef CBVALIDATIONFUNCTIONSH
#define CBVALIDATIONFUNCTIONSH

#include <stdint.h>
#include "CBConstants.h"
#include "CBBlock.h"

// Constants

#define CB_LOCKTIME_THRESHOLD 500000000 // Below this value it is a blok number, else it is a time.
#define CB_TARGET_INTERVAL 1209600 // Two week interval
#define CB_MAX_TARGET 0x1D00FFFF
#define CB_MAX_MONEY 21000000LL * CB_ONE_BITCOIN // 21 million Bitcoins.

/**
 @brief Calculates the block reward at a particular block height
 @param blockHeight The height
 @returns The reward for generating the block, not including transaction fees.
 */
uint64_t CBCalculateBlockReward(uint64_t blockHeight);
/**
 @brief Calculates the block work which is 256^31 divided by the target
 @param work The block work to be created as a CBBigInt.
 @param target The target to calculate the work for.
 @returns true on success, false on failure.
 */
bool CBCalculateBlockWork(CBBigInt * work, uint32_t target);
/**
 @brief Calculates the merkle root from a list of hashes.
 @param hashes The hashes stored as continuous byte data with each 32 byte hash after each other. The data pointed to by "hashes" will be modified and will result in the merkle root as the first 32 bytes.
 @param hashNum The number of hashes in the memory block
 */
void CBCalculateMerkleRoot(uint8_t * hashes, uint32_t hashNum);
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
 @brief Validates a transaction has outputs and inputs, is below the maximum block size, has outputs that do not overflow and that there are no duplicate spent transaction outputs. This function also checks coinbase transactions have input scripts between 2 and 100 bytes and non-coinbase transactions do not have a NULL previous output. Further validation can be done by checking the transaction against input transactions. With simplified payment verification instead the validation is done through trusting miners but the basic validation can still be done with these basic checks.
 @param tx The transaction to validate. This should be deserialised.
 @param coinbase true to validate for a coinbase transaction, false to validate for a non-coinbase transaction.
 @param outputValue Pointer to the integer to hold the output value calculated by this function.
 @returns true if transaction passes basic validation or false.
 */
bool CBTransactionValidateBasic(CBTransaction * tx, bool coinbase, uint64_t * outputValue);
/**
 @brief Determines if a transaction is final and therefore can exist in a block. A transaction is final if the lockTime has been reached or if all inputs are final.
 @param tx The transaction.
 @param time The time for determining if the transaction is final.
 @param height The block height for determining if the transaction is final.
 @returns true if final and false if not final.
 */
bool CBTransactionIsFinal(CBTransaction * tx, uint64_t time, uint64_t height);
/**
 @brief Validates proof of work.
 @brief hash Block hash.
 @returns true if the validation passed, false otherwise.
 */
bool CBValidateProofOfWork(uint8_t * hash, uint32_t target);

#endif
