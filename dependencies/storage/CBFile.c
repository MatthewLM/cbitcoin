//
//  CBFile.c
//  cbitcoin
//
//  Created by Matthew Mitchell on 28/12/2012.
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

#include "CBFile.h"

bool CBFileAppend(CBFile * file, uint8_t * data, uint32_t dataLen){
	uint8_t section[9];
	// Look for appending to existing section
	uint8_t offset = file->dataLength % 8;
	if (NOT CBFileSeek(file, file->dataLength)) 
		return false;
	// Increase total data length.
	file->dataLength += dataLen;
	if (offset) {
		// There is an existing section
		// Write to existing section.
		if (NOT CBFileWriteMidway(file->rdwr, offset, &data, &dataLen))
			return false;
	}
	// Make complete sections
	uint32_t turns = dataLen/8;
	for (uint32_t x = 0; x < turns; x++) {
		// Make section
		memcpy(section, data, 8);
		CBHamming72Encode(section, 8, section + 8);
		// Write section
		if (fwrite(section, 1, 9, file->rdwr) != 9)
			return false;
		// Modify variables
		data += 8;
		dataLen -= 8;
	}
	// If there is more data add it to the end.
	if (dataLen){
		// Make final section
		// Add remaining data
		memcpy(section, data, dataLen);
		// Remaining data does not matter. It is not used and can be anything. Encode parity bits.
		CBHamming72Encode(section, 8, section + 8);
		// Write section
		if (fwrite(section, 1, 9, file->rdwr) != 9)
			return false;
	}
	// Change data size. Use the section variable for the data
	CBInt32ToArray(section, 0, file->dataLength);
	CBHamming72Encode(section, 4, section + 4);
	rewind(file->rdwr);
	return fwrite(section, 1, 5, file->rdwr) == 5;
}
void CBFileClose(CBFile * file){
	fclose(file->rdwr);
}
bool CBFileOpen(CBFile * file, char * filename, bool new){
	file->new = new;
	file->rdwr = fopen(filename, new ? "wb+" : "rb+");
	if (NOT file->rdwr)
		return false;
	uint8_t data[5];
	if (new) {
		// Write length
		CBInt32ToArray(data, 0, 0);
		CBHamming72Encode(data, 4, data + 4);
		if (fwrite(data, 1, 5, file->rdwr) != 5) {
			CBFileClose(file);
			return false;
		}
		file->dataLength = 0;
	}else{
		// Get length
		if (NOT CBFileReadLength(file->rdwr, &file->dataLength)) {
			CBFileClose(file);
			return false;
		}
	}
	// Cursor is initially 0.
	file->cursor = 0;
	// Set F_FULLFSYNC
#ifdef F_FULLFSYNC
	if (fcntl(fileno(file->rdwr), F_FULLFSYNC)){
		CBFileClose(file);
		return false;
	}
#endif
	return NOT fseek(file->rdwr, 5, SEEK_SET);
}
bool CBFileOverwrite(CBFile * file, uint8_t * data, uint32_t dataLen){
	uint8_t section[9];
	// Add data into sections or adjust existing sections
	// First look at inserting the data midway through a section
	uint8_t offset = file->cursor % 8;
	if (offset) {
		uint8_t insertLen = CBFileWriteMidway(file->rdwr, offset, &data, &dataLen);
		if (NOT insertLen)
			return false;
		// Move cursor
		file->cursor += insertLen;
	}
	// Overwrite entire sections
	uint32_t turns = dataLen/8;
	for (uint32_t x = 0; x < turns; x++) {
		// Make section
		memcpy(section, data, 8);
		CBHamming72Encode(section, 8, section + 8);
		// Write section
		if (fwrite(section, 1, 9, file->rdwr) != 9)
			return false;
		// Modify variables
		data += 8;
		dataLen -= 8;
		file->cursor += 8;
	}
	// If no more data then return
	if (NOT dataLen)
		return true;
	// Add remaining data
	memcpy(section, data, dataLen);
	// Read remaining data
	uint8_t readAmount = 8 - dataLen;
	if (fseek(file->rdwr, dataLen, SEEK_CUR)
		|| fread(section + dataLen, 1, readAmount, file->rdwr) != readAmount)
		return false;
	CBHamming72Encode(section, 8, section + 8);
	// Write section
	if (fseek(file->rdwr, -8, SEEK_CUR)
		|| fwrite(section, 1, 9, file->rdwr) != 9)
		return false;
	// Increase cursor
	file->cursor += dataLen;
	// Reseek
	if (NOT CBFileSeek(file, file->cursor))
		return false;
	return true;
}
bool CBFileRead(CBFile * file, uint8_t * data, uint32_t dataLen){
	uint8_t section[9];
	while (dataLen) {
		// Read 9 byte section
		if (fread(section, 1, 9, file->rdwr) != 9)
			return false;
		// Check section
		uint8_t res = CBHamming72Check(section, 8);
		if (res == CB_DOUBLE_BIT_ERROR)
			return false;
		if (res != CB_ZERO_BIT_ERROR){
			// Write corrected byte.
			long pos = ftell(file->rdwr);
			if (fseek(file->rdwr, -9 + res, SEEK_CUR))
				return false;
			fwrite(section + res, 1, 1, file->rdwr);
			if (fseek(file->rdwr, pos, SEEK_SET))
				return false;
		}
		// Copy section to data
		uint8_t offset = file->cursor % 8;
		uint8_t remaining = 8 - offset;
		// dataLen is actually the bytes left for the data. remaining is for the bytes needed in this section
		if (dataLen < remaining)
			remaining = dataLen;
		memcpy(data, section + offset, remaining);
		// Adjust data pointer and length remaining
		data += remaining;
		dataLen -= remaining;
		// Adjust cursor
		file->cursor += remaining;
	}
	// Reseek for next read
	if (NOT CBFileSeek(file, file->cursor))
		return false;
	return true;
}
bool CBFileReadLength(FILE * rd, uint32_t * length){
	// Get length
	uint8_t data[5];
	if (fread(data, 1, 5, rd) != 5){
		fclose(rd);
		return false;
	}
	// Check integrity of data
	uint8_t res = CBHamming72Check(data, 4);
	if (res == CB_DOUBLE_BIT_ERROR)
		return false;
	if (res != CB_ZERO_BIT_ERROR) {
		// Write correction to file
		if (fseek(rd, res, SEEK_SET)
			|| fwrite(data + res, 1, 1, rd) != 1) {
			fclose(rd);
			return false;
		}
	}
	*length = CBArrayToInt32(data, 0);
	return true;
}
bool CBFileSeek(CBFile * file, uint32_t pos){
	file->cursor = pos;
	return NOT fseek(file->rdwr, 5 + pos / 8 * 9, SEEK_SET);
}
bool CBFileSync(CBFile * file){
	if (fflush(file->rdwr))
		return false;
	if (fsync(fileno(file->rdwr)))
		return false;
	return true;
}
bool CBFileSyncDir(char * dir){
	int dird = open(dir, 0);
	if (dird == -1)
		return false;
	if (fsync(dird))
		return false;
	return true;
}
bool CBFileTruncate(char * filename, uint32_t newSize){
	int file = open(filename, O_WRONLY);
	if (file == -1)
		return false;
	// Write new data length to file.
	uint8_t data[5];
	CBInt32ToArray(data, 0, newSize);
	CBHamming72Encode(data, 4, data + 4);
	if (write(file, data, 5) != 5)
		return false;
	if (fsync(file))
		return false;
	return NOT truncate(filename, 5 + ((newSize - 1)/8 + 1)*9);
}
uint8_t CBFileWriteMidway(FILE * rd, uint8_t offset, uint8_t ** data, uint32_t * dataLen){
	uint8_t section[9];
	// Read existing section then check and correct data.
	if (fread(section, 1, 9, rd) != 9)
		return 0;
	if (CBHamming72Check(section, 8) == CB_DOUBLE_BIT_ERROR)
		return 0;
	// Insert new data
	uint8_t insertLen = 8 - offset;
	if (*dataLen < insertLen)
		insertLen = *dataLen;
	memcpy(section + offset, *data, insertLen);
	// Encode hamming parity
	CBHamming72Encode(section, 8, section + 8);
	// Write data to file
	if (fseek(rd, -9, SEEK_CUR))
		return 0;
	if (fwrite(section, 1, 9, rd) != 9)
		return 0;
	// Move forward data pointer and lower dataLen to remaining length
	*data += insertLen;
	*dataLen -= insertLen;
	return insertLen;
}
