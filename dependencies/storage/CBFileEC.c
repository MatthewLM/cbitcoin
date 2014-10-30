//
//  CBFileEC.c
//  cbitcoin
//
//  Created by Matthew Mitchell on 28/12/2012.
//  Copyright (c) 2012 Matthew Mitchell
//
//  This file is part of cbitcoin. It is subject to the license terms
//  in the LICENSE file found in the top-level directory of this
//  distribution and at http://www.cbitcoin.com/license.html. No part of
//  cbitcoin, including this file, may be copied, modified, propagated,
//  or distributed except according to the terms contained in the
//  LICENSE file.

//  SEE HEADER FILE FOR DOCUMENTATION

#include "CBFileEC.h"

bool CBFileAppend(CBDepObject file, uint8_t * data, uint32_t dataLen){
	uint8_t section[9];
	CBFile * fileObj = file.ptr;
	// Look for appending to existing section
	uint8_t offset = fileObj->dataLength % 8;
	if (! CBFileSeek(file, fileObj->dataLength, SEEK_SET))
		return false;
	// Increase total data length.
	fileObj->dataLength += dataLen;
	if (offset) {
		// There is an existing section
		// Write to existing section.
		if (! CBFileWriteMidway(fileObj->rdwr, offset, &data, &dataLen))
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
bool CBFileAppendZeros(CBDepObject file, uint32_t amount){
	CBFile * fileObj = file.ptr;
	// Look for appending to existing section
	uint8_t offset = fileObj->dataLength % 8;
	if (! CBFileSeek(file, fileObj->dataLength, SEEK_SET))
		return false;
	// Increase total data length.
	fileObj->dataLength += amount;
	uint8_t section[9] = {0,0,0,0,0,0,0,0,127};
	uint8_t * wholeSectionPtr = section;
	if (offset) {
		// There is an existing section
		// Write to existing section.
		if (! CBFileWriteMidway(fileObj->rdwr, offset, &wholeSectionPtr, &amount))
			return false;
	}
	// Add whole sections
	uint32_t numSections = amount/8 + (amount % 8 != 0);
	for (uint32_t x = 0; x < numSections; x++) {
		if (fwrite(section, 1, 9, fileObj->rdwr) != 9)
			return false;
	}
	// Change data size. Use the section variable for the data
	CBInt32ToArray(section, 0, fileObj->dataLength);
	CBHamming72Encode(section, 4, section + 4);
	rewind(fileObj->rdwr);
	return fwrite(section, 1, 5, fileObj->rdwr) == 5;
}
void CBFileClose(CBDepObject file){
	fclose(((CBFile *)file.ptr)->rdwr);
	free(file.ptr);
}
bool CBFileGetLength(CBDepObject file, uint32_t * length){
	*length = ((CBFile *)file.ptr)->dataLength;
	return true;
}
bool CBFileOpen(CBDepObject * file, char * filename, bool new){
	file->ptr = malloc(sizeof(CBFile));
	CBFile * fileObj = file->ptr;
	fileObj->new = new;
	fileObj->rdwr = fopen(filename, new ? "wb+" : "rb+");
	if (! fileObj->rdwr){
		free(file->ptr);
		return false;
	}
	uint8_t data[5];
	if (new) {
		// Write length
		CBInt32ToArray(data, 0, 0);
		CBHamming72Encode(data, 4, data + 4);
		if (fwrite(data, 1, 5, fileObj->rdwr) != 5) {
			CBFileClose(*file);
			return false;
		}
		fileObj->dataLength = 0;
	}else{
		// Get length
		if (! CBFileReadLength(fileObj->rdwr, &fileObj->dataLength)) {
			CBFileClose(*file);
			return false;
		}
	}
	// Cursor is initially 0.
	fileObj->cursor = 0;
	if (fseek(fileObj->rdwr, 5, SEEK_SET) == -1) {
		CBFileClose(*file);
		return false;
	}
	return true;
}
bool CBFileOverwrite(CBDepObject file, uint8_t * data, uint32_t dataLen){
	uint8_t section[9];
	CBFile * fileObj = file.ptr;
	// Add data into sections or adjust existing sections
	// First look at inserting the data midway through a section
	uint8_t offset = fileObj->cursor % 8;
	if (offset) {
		uint8_t insertLen = CBFileWriteMidway(fileObj->rdwr, offset, &data, &dataLen);
		if (! insertLen)
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
	if (! dataLen)
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
	if (! CBFileSeek(file, fileObj->cursor, SEEK_SET))
		return false;
	return true;
}
uint32_t CBFilePos(CBDepObject file){
	return ((CBFile *)file.ptr)->cursor;
}
bool CBFileRead(CBDepObject file, uint8_t * data, uint32_t dataLen){
	uint8_t section[9];
	CBFile * fileObj = file.ptr;
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
	if (! CBFileSeek(file, fileObj->cursor, SEEK_SET))
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
bool CBFileSeek(CBDepObject file, int32_t pos, int seek){
	CBFile * fileObj = file.ptr;
	if (seek == SEEK_SET)
		fileObj->cursor = pos;
	else if (seek == SEEK_CUR)
		fileObj->cursor += pos;
	else
		fileObj->cursor = fileObj->dataLength + pos;
	return ! fseek(fileObj->rdwr, 5 + fileObj->cursor / 8 * 9, SEEK_SET);
}
bool CBFileSync(CBDepObject file){
	CBFile * fileObj = file.ptr;
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
	return ! truncate(filename, 5 + ((newSize - 1)/8 + 1)*9);
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
