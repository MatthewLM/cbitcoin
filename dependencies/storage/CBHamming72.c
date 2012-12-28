//
//  CBHamming72.c
//  cbitcoin
//
//  Created by Matthew Mitchell on 24/12/2012.
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

//  SEE HEADER FILE FOR DOCUMENTATION

#include "CBHamming72.h"

CBHamming72Result CBHamming72Check(uint8_t * data, uint32_t dataLen, uint8_t * parityBits){
	// Go through each segment
	uint8_t testParity;
	CBHamming72Result res;
	res.corrections = NULL;
	res.numCorrections = 0;
	res.err = false;
	for (uint8_t x = 0; x < dataLen; x += 8) {
		uint32_t dataLeft = dataLen - x;
		if (dataLeft > 8)
			dataLeft = 8;
		CBHamming72EncodeSeqment(data + x, dataLeft, &testParity);
		// Check calculation against parity bits
		uint8_t testHammingParity = testParity & 0x7F;
		uint8_t hammingParity = *parityBits & 0x7F;
		bool needCorrection = testHammingParity != hammingParity;
		bool badTotalParity = (testParity & 0x80) != (*parityBits & 0x80);
		if (needCorrection || badTotalParity) {
			// Reallocate list for corrections
			res.corrections = realloc(res.corrections, sizeof(*res.corrections) * (++res.numCorrections));
			if (NOT res.corrections) {
				res.err = true;
				break;
			}
			if (needCorrection) {
				// Correct the bit
				uint8_t pos = testHammingParity ^ hammingParity;
				// "pos" is the hamming bit positon, correct data or parity
				// 0        1         2         3         4         5         6         7
				// 12345678901234567890123456789012345678901234567890123456789012345678901
				// pp1p000p0010100p100000000101000p1001101111000111111010110011101p1101011
				if (!(pos & (pos - 1))){
					// This is a parity bit
					*parityBits ^= pos;
					res.corrections[res.numCorrections - 1] = x/8 | 128;
				}else if (pos == 3){
					data[x] ^= 128;
					res.corrections[res.numCorrections - 1] = x;
				}else if (pos < 8){
					data[x] ^= 1 << (11 - pos);
					res.corrections[res.numCorrections - 1] = x;
				}else if (pos < 13){
					data[x] ^= 1 << (12 - pos);
					res.corrections[res.numCorrections - 1] = x;
				}else if (pos < 16){
					data[x + 1] ^= 1 << (20 - pos);
					res.corrections[res.numCorrections - 1] = x + 1;
				}else if (pos < 22){
					data[x + 1] ^= 1 << (21 - pos);
					res.corrections[res.numCorrections - 1] = x + 1;
				}else if (pos < 30){
					data[x + 2] ^= 1 << (29 - pos);
					res.corrections[res.numCorrections - 1] = x + 2;
				}else if (pos < 32){
					data[x + 3] ^= 1 << (37 - pos);
					res.corrections[res.numCorrections - 1] = x + 3;
				}else if (pos < 39){
					data[x + 3] ^= 1 << (38 - pos);
					res.corrections[res.numCorrections - 1] = x + 3;
				}else if (pos < 47){
					data[x + 4] ^= 1 << (46 - pos);
					res.corrections[res.numCorrections - 1] = x + 4;
				}else if (pos < 55){
					data[x + 5] ^= 1 << (54 - pos);
					res.corrections[res.numCorrections - 1] = x + 5;
				}else if (pos < 63){
					data[x + 6] ^= 1 << (62 - pos);
					res.corrections[res.numCorrections - 1] = x + 6;
				}else if (pos == 63){
					data[x + 7] ^= 128;
					res.corrections[res.numCorrections - 1] = x + 7;
				}else{
					data[x + 7] ^= 1 << (71 - pos);
					res.corrections[res.numCorrections - 1] = x + 7;
				}
				// Verify total parity
				CBHamming72EncodeSeqment(data + x, dataLeft, &testParity);
				if ((testParity & 0x80) != (*parityBits & 0x80)) {
					// Detected a double bit error
					res.err = true;
					free(res.corrections);
					res.numCorrections = 0;
					break;
				}
			}else{
				// Correct total parity
				*parityBits ^= 128;
				res.corrections[res.numCorrections - 1] = x/8 | 128;
			}
		}
		// Move to next parity bits
		parityBits++;
	}
	return res;
}
void CBHamming72Encode(uint8_t * data, uint32_t dataLen, uint8_t * parityBits){
	// Loop through 64 bits, creating the 8 parity bits.
	for (uint8_t x = 0; x < dataLen; x += 8) {
		uint32_t dataLeft = dataLen - x;
		if (dataLeft > 8)
			dataLeft = 8;
		CBHamming72EncodeSeqment(data + x, dataLeft, parityBits + x/8);
	}
}
void CBHamming72EncodeSeqment(uint8_t * data, uint32_t dataLen, uint8_t * parityBits){
	// First set the parity bits for this section to 1
	*parityBits = 0xFF;
	// Now loop through the bits
	uint8_t bitPos = 3; // Position of the bit for partity coverage. First position for data bit is 3 (starting from pos 1)
	for (uint8_t byte = 0; byte < dataLen; byte++) {
		for (uint8_t bitNum = 0; bitNum < 8; bitNum++) {
			bool bit = data[byte] & (128 >> bitNum);
			// Use XOR to detemine parity
			if (bitPos % 2)
				*parityBits ^= bit;
			if ((bitPos/2) % 2)
				*parityBits ^= bit << 1;
			if ((bitPos/4) % 2)
				*parityBits ^= bit << 2;
			if ((bitPos/8) % 2)
				*parityBits ^= bit << 3;
			if ((bitPos/16) % 2)
				*parityBits ^= bit << 4;
			if ((bitPos/32) % 2)
				*parityBits ^= bit << 5;
			if ((bitPos/64) % 2)
				*parityBits ^= bit << 6;
			// Calculate total parity for DED
			*parityBits ^= bit << 7;
			// Get next bit position
			bitPos++;
			// If power of two it is for parity so move along
			if (!(bitPos & (bitPos - 1)))
				bitPos++;
		}
	}
	// Add hamming parity bits to total parity
	uint8_t temp = *parityBits & 0x7F;
	temp ^= temp >> 4;
	temp ^= temp >> 2;
	temp ^= temp >> 1;
	*parityBits ^= (temp & 1) << 7;
}
