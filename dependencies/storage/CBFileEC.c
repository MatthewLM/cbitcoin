//
//  CBFileEC.c
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

#include "CBFileEC.h"

bool CBFileAppend(uint64_t file, uint8_t * data, uint32_t dataLen){
	uint8_t section[9];
	CBFile * fileObj = (CBFile *)file;
	// Look for appending to existing section
	uint8_t offset = fileObj->dataLength % 8;
	if (NOT CBFileSeek(file, fileObj->dataLength)) 
		return false;
	// Increase total data length.
	fileObj->dataLength += dataLen;
	if (offset) {
		// There is an existing section
		// Write to existing section.
		if (NOT CBFileWriteMidway(fileObj->rdwr, offset, &data, &dataLen))
			return false;
	}
	// Make complete sections
	uint32_t turns = dataLen/8;
	for (uint32_t x = 0; x < turns; x++) {
		// Make section
		memcpy(section, data, 8);
		CBHamming72Encode(section, 8, section + 8);
		// Write section
		if (fwrite(section, 1, 9, fileObj->rdwr) != 9)
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
		if (fwrite(section, 1, 9, fileObj->rdwr) != 9)
			return false;
	}
	// Change data size. Use the section variable for the data
	CBInt32ToArray(section, 0, fileObj->dataLength);
	CBHamming72Encode(section, 4, section + 4);
	rewind(fileObj->rdwr);
	return fwrite(section, 1, 5, fileObj->rdwr) == 5;
}
void CBFileClose(uint64_t file){
	fclose(((CBFile *)file)->rdwr);
}
bool CBFileGetLength(uint64_t file, uint32_t * length){
	*length = ((CBFile *)file)->dataLength;
	return true;
}
uint64_t CBFileOpen(char * filename, bool new){
	CBFile * fileObj = malloc(sizeof(*fileObj));
	if (NOT fileObj)
		return 0;
	fileObj->new = new;
	fileObj->rdwr = fopen(filename, new ? "wb+" : "rb+");
	if (NOT fileObj->rdwr)
		return false;
	uint8_t data[5];
	if (new) {
		// Write length
		CBInt32ToArray(data, 0, 0);
		CBHamming72Encode(data, 4, data + 4);
		if (fwrite(data, 1, 5, fileObj->rdwr) != 5) {
			CBFileClose((uint64_t)fileObj);
			return false;
		}
		fileObj->dataLength = 0;
	}else{
		// Get length
		if (NOT CBFileReadLength(fileObj->rdwr, &fileObj->dataLength)) {
			CBFileClose((uint64_t)fileObj);
			return false;
		}
	}
	// Cursor is initially 0.
	fileObj->cursor = 0;
	// Set F_FULLFSYNC
	// F_FULLFSYNC will ensure writes are stored on disk in-order. It is not necessarily important that writes are written immediately but they must be written in order to avoid data corruption. Unfortunately this makes IO operations extremely slow. ??? How to ensure in-order disk writes on other systems?
#ifdef F_FULLFSYNC
	if (fcntl(fileno(fileObj->rdwr), F_FULLFSYNC)){
		CBFileClose((uint64_t)fileObj);
		return false;
	}
#endif
	if (fseek(fileObj->rdwr, 5, SEEK_SET) == -1) {
		CBFileClose((uint64_t)fileObj);
		return false;
	}
	return (uint64_t)fileObj;
}
bool CBFileOverwrite(uint64_t file, uint8_t * data, uint32_t dataLen){
	uint8_t section[9];
	CBFile * fileObj = (CBFile *)file;
	// Add data into sections or adjust existing sections
	// First look at inserting the data midway through a section
	uint8_t offset = fileObj->cursor % 8;
	if (offset) {
		uint8_t insertLen = CBFileWriteMidway(fileObj->rdwr, offset, &data, &dataLen);
		if (NOT insertLen)
			return false;
		// Move cursor
		fileObj->cursor += insertLen;
	}
	// Overwrite entire sections
	uint32_t turns = dataLen/8;
	for (uint32_t x = 0; x < turns; x++) {
		// Make section
		memcpy(section, data, 8);
		CBHamming72Encode(section, 8, section + 8);
		// Write section
		if (fwrite(section, 1, 9, fileObj->rdwr) != 9)
			return false;
		// Modify variables
		data += 8;
		dataLen -= 8;
		fileObj->cursor += 8;
	}
	// If no more data then return
	if (NOT dataLen)
		return true;
	// Add remaining data
	memcpy(section, data, dataLen);
	// Read remaining data
	uint8_t readAmount = 8 - dataLen;
	if (fseek(fileObj->rdwr, dataLen, SEEK_CUR)
		|| fread(section + dataLen, 1, readAmount, fileObj->rdwr) != readAmount)
		return false;
	CBHamming72Encode(section, 8, section + 8);
	// Write section
	if (fseek(fileObj->rdwr, -8, SEEK_CUR)
		|| fwrite(section, 1, 9, fileObj->rdwr) != 9)
		return false;
	// Increase cursor
	fileObj->cursor += dataLen;
	// Reseek
	if (NOT CBFileSeek(file, fileObj->cursor))
		return false;
	return true;
}
bool CBFileRead(uint64_t file, uint8_t * data, uint32_t dataLen){
	uint8_t section[9];
	CBFile * fileObj = (CBFile *)file;
	while (dataLen) {
		// Read 9 byte section
		if (fread(section, 1, 9, fileObj->rdwr) != 9)
			return false;
		// Check section
		uint8_t res = CBHamming72Check(section, 8);
		if (res == CB_DOUBLE_BIT_ERROR)
			return false;
		if (res != CB_ZERO_BIT_ERROR){
			// Write corrected byte.
			long pos = ftell(fileObj->rdwr);
			if (fseek(fileObj->rdwr, -9 + res, SEEK_CUR))
				return false;
			fwrite(section + res, 1, 1, fileObj->rdwr);
			if (fseek(fileObj->rdwr, pos, SEEK_SET))
				return false;
		}
		// Copy section to data
		uint8_t offset = fileObj->cursor % 8;
		uint8_t remaining = 8 - offset;
		// dataLen is actually the bytes left for the data. remaining is for the bytes needed in this section
		if (dataLen < remaining)
			remaining = dataLen;
		memcpy(data, section + offset, remaining);
		// Adjust data pointer and length remaining
		data += remaining;
		dataLen -= remaining;
		// Adjust cursor
		fileObj->cursor += remaining;
	}
	// Reseek for next read
	if (NOT CBFileSeek(file, fileObj->cursor))
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
bool CBFileSeek(uint64_t file, uint32_t pos){
	CBFile * fileObj = (CBFile *)file;
	fileObj->cursor = pos;
	return NOT fseek(fileObj->rdwr, 5 + pos / 8 * 9, SEEK_SET);
}
bool CBFileSync(uint64_t file){
	CBFile * fileObj = (CBFile *)file;
	if (fflush(fileObj->rdwr))
		return false;
	if (fsync(fileno(fileObj->rdwr)))
		return false;
	return true;
}
bool CBFileSyncDir(char * dir){
	int dird = open(dir, 0);
	if (dird == -1)
		return false;
	if (fsync(dird)
		// If errno is EINVAL then it is likely the filesystem doesn't support directory synchronisation or does not need it. ???
		&& errno != EINVAL){
		close(dird);
		return false;
	}
	close(dird);
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
