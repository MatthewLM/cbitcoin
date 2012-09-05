//
//  CBValidationFunctions.h
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

#include "CBValidationFunctions.h"

uint64_t CBCalculateBlockReward(uint64_t blockHeight){
	return (50 * CB_ONE_BITCOIN) >> (blockHeight / 210000);
}
CBBigInt CBCalculateBlockWork(uint32_t target){
	// Get the base-256 exponent and the mantissa.
	uint8_t zeroBytes = target >> 24;
	target &= 0x00FFFFFF;
	// Allocate CBBigInt data
	CBBigInt work;
	work.length = 29 - zeroBytes;
	work.data = malloc(work.length);
	if (NOT work.data) {
		work.data = NULL;
		work.length = 0;
		return work;
	}
	// Do base-4294967296 long division and adjust for trailing zeros in target.
	uint64_t temp = 0x01000000;
	uint32_t workSeg;
	for (uint8_t x = 0;; x++) {
		workSeg = (uint32_t)(temp / target);
		uint8_t i = 31 - x * 4 - zeroBytes;
		for (uint8_t y = 0; y < 4; y++){
			work.data[i] = workSeg >> ((3 - y) * 8);
			if (NOT i)
				return work;
			i--;
		}
		temp -= workSeg * target;
		temp <<= 32;
	}
}
void CBCalculateMerkleRoot(uint8_t * hashes,uint32_t hashNum){
	uint8_t hash[32];
	for (uint32_t x = 0; hashNum != 1;) {
		if (x == hashNum - 1) {
			uint8_t dup[64];
			memcpy(dup, hashes + x * 32, 32);
			memcpy(dup + 32, hashes + x * 32, 32);
			CBSha256(dup, 64, hash);
		}else
			CBSha256(hashes + x * 32, 64, hash);
		// Double SHA256
		CBSha256(hash, 32, hashes + x * 32/2);
		x += 2;
		if (x >= hashNum) {
			if (x > hashNum)
				// The number of hashes was odd. Increment to even
				hashNum++;
			hashNum /= 2;
			x = 0;
		}
	}
}
uint32_t CBCalculateTarget(uint32_t oldTarget, uint32_t time){
	// Limitations on the factor for retargeting.
	if (time < CB_TARGET_INTERVAL / 4)
		time = CB_TARGET_INTERVAL / 4;
	if (time > CB_TARGET_INTERVAL * 4)
		time = CB_TARGET_INTERVAL * 4;
	// Get number of trailing 0 bytes (Base 256 exponent)
	uint8_t zeroBytes = oldTarget >> 24;
	// Get the three bytes for the target mantissa. Use 8 bytes for overflow. Shift along once for division.
	uint64_t num = (oldTarget & 0x00FFFFFF) << 8;
	// Modify the three bytes
	num *= time;
	num /= CB_TARGET_INTERVAL;
	// Find first byte with data. Check for flow into more or less significant byte.
	if (num & 0xF00000000) {
		// Mantissa overflowed into more significant byte.
		num >>= 16;
		num &= 0x00FFFFFF; // Clear the forth byte for the number of zero bytes.
		zeroBytes++;
	}else if (num & 0xFF000000)
		// Data within same three bytes
		num >>= 8;
	else
		// Data down a byte
		zeroBytes--;
	// Modify in the case of the first byte in the mantissa being over 0x7F... What did you say?... Oh yes, the bitcoin protocol can be dumb in places.
	if (num > 0x007FFFFF) {
		// Add trailing zero by shifting right...
		num >>= 8;
		// Compensate with additional trailing zero byte.
		zeroBytes++;
	}
	// Apply the number of trailing zero bytes into the compact target representation.
	num |= zeroBytes << 24;
	// Check if the target is too high and if it is, make it equal the maximum target.
	if (num > CB_MAX_TARGET)
		num = CB_MAX_TARGET;
	// Return the new target
	return (uint32_t) num;
}
uint32_t CBTransactionGetSigOps(CBTransaction * tx){
	uint32_t sigOps = 0;
	for (uint32_t x = 0; x < tx->inputNum; x++)
		sigOps += CBScriptGetSigOpCount(tx->inputs[x]->scriptObject, false);
	for (uint32_t x = 0; x < tx->outputNum; x++)
		sigOps += CBScriptGetSigOpCount(tx->outputs[x]->scriptObject, false);
	return sigOps;
}
bool CBTransactionIsFinal(CBTransaction * tx,uint64_t time,uint64_t height){
	if (tx->lockTime) {
		if (tx->lockTime < (tx->lockTime < CB_LOCKTIME_THRESHOLD ? (int64_t)height : (int64_t)time))
			return true;
		for (uint32_t x = 0; x < tx->inputNum; x++)
			if (tx->inputs[x]->sequence != CB_TRANSACTION_INPUT_FINAL)
				return false;
	}
	return true;
}
CBPrevOut * CBTransactionValidateBasic(CBTransaction * tx, bool coinbase, bool * err){
	*err = false;
	if (NOT tx->inputNum || NOT tx->outputNum)
		return NULL;
	uint32_t length;
	if (CBGetMessage(tx)->bytes) // Already have length
		length = CBGetMessage(tx)->bytes->length;
	else{
		// Calculate length. Worthwhile having a cache? ???
		length = CBTransactionCalculateLength(tx);
	}
	if(length > CB_BLOCK_MAX_SIZE){
		return NULL;
	}
	// Check that outputs do not overflow by ensuring they do not go over 21 million bitcoins. There was once an vulnerability in the C++ client on this where an attacker could overflow very large outputs to equal small inputs.
	uint64_t total = 0;
	for (uint32_t x = 0; x < tx->outputNum; x++) {
		if (tx->outputs[x]->value > CB_MAX_MONEY)
			return NULL;
		total += tx->outputs[x]->value;
		if (total > CB_MAX_MONEY)
			return NULL;
	}
	if (coinbase){
		// Validate input script for coinbase
		if (tx->inputs[0]->scriptObject->length < 2
			|| tx->inputs[0]->scriptObject->length > 100)
			return NULL;
	}else for (uint32_t x = 0; x < tx->inputNum; x++)
		// Check each input for null previous output hashes.
		if (CBByteArrayIsNull(tx->inputs[x]->prevOut.hash))
			return NULL;
	// Check for duplicate transaction output spends and add them to a list of CBPrevOut structures.
	CBPrevOut * prevOutputs = malloc(sizeof(*prevOutputs) * tx->inputNum);
	if (NOT prevOutputs){
		*err = true;
		return NULL;
	}
	for (uint32_t x = 0; x < tx->inputNum; x++) {
		for (uint32_t y = 0; y < x; y++)
			if (CBByteArrayEquals(prevOutputs[y].hash, tx->inputs[x]->prevOut.hash)
				&& prevOutputs[y].index == tx->inputs[x]->prevOut.index) {
				// Duplicate previous output
				free(prevOutputs);
				return NULL;
			}
		prevOutputs[x] = tx->inputs[x]->prevOut;
	}
	return prevOutputs;
}
bool CBValidateProofOfWork(CBByteArray * hash, uint32_t target){
	// Get trailing zero bytes
	uint8_t zeroBytes = target >> 24;
	// Check target is less than or equal to maximum.
	if (target > CB_MAX_TARGET)
		return false;
	// Modify the target to the mantissa (significand).
	target &= 0x00FFFFFF;
	// Check mantissa is below 0x800000.
	if (target > 0x7FFFFF)
		return false;
	// Fail if hash is above target. First check leading bytes to significant part
	for (uint8_t x = 0; x < 32 - zeroBytes; x++)
		if (CBByteArrayGetByte(hash, x))
			// A byte leading to the significant part is not zero
			return false;
	// Check significant part
	uint32_t significantPart = CBByteArrayGetByte(hash, 32 - zeroBytes) << 16;
	significantPart |= CBByteArrayGetByte(hash, 33 - zeroBytes) << 8;
	significantPart |= CBByteArrayGetByte(hash, 34 - zeroBytes);
	if (significantPart > target)
		return false;
	return true;
}
