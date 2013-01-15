//
//  testCBFile.c
//  cbitcoin
//
//  Created by Matthew Mitchell on 31/12/2012.
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

#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "CBFileEC.h"

void CBLogError(char * format, ...);
void CBLogError(char * format, ...){
	va_list argptr;
    va_start(argptr, format);
    vfprintf(stderr, format, argptr);
    va_end(argptr);
	printf("\n");
}

int main(){
	unsigned int s = (unsigned int)time(NULL);
	s = 1337544566;
	printf("Session = %ui\n", s);
	srand(s);
	remove("./test.dat");
	// Test newly opening file
	uint64_t file = CBFileOpen("./test.dat", false);
	if (file) {
		printf("OPEN NOT EXIST FAIL\n");
		return 1;
	}
	file = CBFileOpen("./test.dat", true);
	if (NOT file) {
		printf("OPEN NEW FAIL\n");
		return 1;
	}
	uint32_t len;
	if (NOT CBFileGetLength(file, &len)) {
		printf("NEW DATA GET LEN FAIL\n");
		return 1;
	}
	if (len) {
		printf("NEW DATA LEN FAIL\n");
		return 1;
	}
	if (NOT CBFileAppend(file, (uint8_t *)"Hello There World!", 18)) {
		printf("NEW APPEND FAIL\n");              
		return 1;
	}
	if (NOT CBFileSeek(file, 0)) {
		printf("NEW SEEK FAIL\n");
		return 1;
	}
	uint8_t data[44];
	if (NOT CBFileRead(file, data, 11)) {
		printf("NEW READ FAIL\n");
		return 1;
	}
	if (memcmp(data, "Hello There", 11)) {
		printf("NEW READ DATA FAIL\n");
		return 1;
	}
	if (NOT CBFileRead(file, data, 7)) {
		printf("NEW READ SEEK FAIL\n");
		return 1;
	}
	if (memcmp(data, " World!", 7)) {
		printf("NEW READ SEEK DATA FAIL\n");
		return 1;
	}
	if (NOT CBFileSeek(file, 6)) {
		printf("NEW SEEK 6 FAIL\n");
		return 1;
	}
	if (NOT CBFileOverwrite(file, (uint8_t *)"Good People.", 12)) {
		printf("NEW OVERWRITE FAIL\n");
		return 1;
	}
	if (NOT CBFileSeek(file, 0)) {
		printf("NEW SEEK AFTER OVERWRITE FAIL\n");
		return 1;
	}
	if (NOT CBFileRead(file, data, 18)) {
		printf("NEW READ ALL FAIL\n");
		return 1;
	}
	if (memcmp(data, "Hello Good People.", 18)) {
		printf("NEW OVERWRITE DATA FAIL\n");
		return 1;
	}
	if (NOT CBFileAppend(file, (uint8_t *)" Now Goodbye!", 13)) {
		printf("NEW 2ND APPEND FAIL\n");
		return 1;
	}
	if (NOT CBFileSeek(file, 0)) {
		printf("NEW SEEK AFTER 2ND APPEND FAIL\n");
		return 1;
	}
	if (NOT CBFileRead(file, data, 31)) {
		printf("NEW READ AFTER 2ND APPEND FAIL\n");
		return 1;
	}
	if (memcmp(data, "Hello Good People. Now Goodbye!", 31)) {
		printf("NEW 2ND APPEND DATA FAIL\n");
		return 1;
	}
	CBFileGetLength(file, &len);
	if (len != 31) {
		printf("NEW 2ND APPEND DATA LENGTH FAIL\n");
		return 1;
	}
	// Close and then reopen file
	CBFileClose(file);
	file = CBFileOpen("./test.dat", false);
	if (NOT file){
		printf("EXISTING OPEN FAIL\n");
		return 1;
	}
	CBFileGetLength(file, &len);
	if (len != 31) {
		printf("EXISTING DATA LENGTH FAIL\n");
		return 1;
	}
	if (NOT CBFileRead(file, data, 31)) {
		printf("EXISTING READ FAIL\n");
		return 1;
	}                       
	if (memcmp(data, "Hello Good People. Now Goodbye!", 31)) {
		printf("EXISTING READ DATA FAIL\n");
		return 1;
	}
	if (NOT CBFileSeek(file, 6)) {
		printf("EXISTING SEEK FAIL\n");
		return 1;
	}
	if (NOT CBFileOverwrite(file, (uint8_t *)"Great World!", 12)) {
		printf("EXISTING OVERWRITE FAIL\n");
		return 1;
	}
	if (NOT CBFileAppend(file, (uint8_t *)" Hello Again.", 13)) {
		printf("EXISTING APPEND FAIL\n");
		return 1;
	}
	if (NOT CBFileSeek(file, 0)) {
		printf("EXISTING SEEK TO START FAIL\n");
		return 1;
	}
	if (NOT CBFileRead(file, data, 44)) {
		printf("EXISTING READ CHANGES FAIL\n");
		return 1;
	}                                   
	if (memcmp(data, "Hello Great World! Now Goodbye! Hello Again.", 44)) {
		printf("EXISTING READ CHANGES DATA FAIL\n");
		return 1;
	}
	CBFileGetLength(file, &len);
	if (len != 44) {
		printf("EXISTING CHANGES DATA LENGTH FAIL\n");
		return 1;
	}
	// Test hamming code correction
	rewind(((CBFile *)file)->rdwr);
	fwrite((uint8_t []){44}, 1, 1, ((CBFile *)file)->rdwr);
	fseek(((CBFile *)file)->rdwr, 5, SEEK_SET);
	fwrite((uint8_t []){74}, 1, 1, ((CBFile *)file)->rdwr);
	CBFileClose(file);
	file = CBFileOpen("./test.dat", false);
	if (NOT file) {
		printf("LENGTH BIT ERR OPEN FAIL\n");
		return 1;
	}
	CBFileGetLength(file, &len);
	if (len != 44) {
		printf("LENGTH BIT ERR LENGTH FAIL\n");
		return 1;
	}
	if (NOT CBFileRead(file, data, 1)) {
		printf("DATA BIT ERR READ FAIL\n");
		return 1;
	}
	if (data[0] != 'H') {
		printf("DATA BIT ERR READ DATA FAIL\n");
		return 1;
	}
	if (NOT CBFileSeek(file, 6)) {
		printf("DOUBLE OVERWRITE SEEK FAIL\n");
		return 1;
	}
	if (NOT CBFileOverwrite(file, (uint8_t *)"Super Lands.", 12)) {
		printf("DOUBLE OVERWRITE ONE FAIL\n");
		return 1;
	}
	if (NOT CBFileOverwrite(file, (uint8_t *)" Very Lovely.", 13)) {
		printf("DOUBLE OVERWRITE TWO FAIL\n");
		return 1;
	}
	if (NOT CBFileSeek(file, 6)) {
		printf("DOUBLE OVERWRITE READ SEEK FAIL\n");
		return 1;
	}
	if (NOT CBFileRead(file, data, 15)) {
		printf("DOUBLE OVERWRITE READ FAIL\n");
		return 1;
	}
	if (memcmp(data, "Super Lands. Very Lovely.", 15)) {
		printf("DOUBLE OVERWRITE READ DATA FAIL\n");
		return 1;
	}
	CBFileClose(file);
	// Test Truncation
	if (NOT CBFileTruncate("./test.dat", 31)) {
		printf("TRUNCATE FAIL\n");
		return 1;
	}
	file = CBFileOpen("./test.dat", false);
	if (NOT file) {
		printf("TRUNCATE OPEN FAIL\n");
		return 1;
	}
	CBFileGetLength(file, &len);
	if (len != 31) {
		printf("TRUNCATE LENGTH FAIL\n");
		return 1;
	}
	if (NOT CBFileRead(file, data, 31)) {
		printf("TRUNCATE READ FAIL\n");
		return 1;
	}
	if (memcmp(data, "Hello Super Lands. Very Lovely.", 31)) {
		printf("TRUNCATE READ DATA FAIL\n");
		return 1;
	}
	return 0;
}
