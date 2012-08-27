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
