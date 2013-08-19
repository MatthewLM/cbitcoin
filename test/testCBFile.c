//
//  testCBFile.c
//  cbitcoin
//
//  Created by Matthew Mitchell on 31/12/2012.
//  Copyright (c) 2012 Matthew Mitchell
//  
//  This file is part of cbitcoin. It is subject to the license terms
//  in the LICENSE file found in the top-level directory of this
//  distribution and at http://www.cbitcoin.com/license.html. No part of
//  cbitcoin, including this file, may be copied, modified, propagated,
//  or distributed except according to the terms contained in the
//  LICENSE file.

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
	CBDepObject file;
	if (CBFileOpen(&file,"./test.dat", false)) {
		printf("OPEN NOT EXIST FAIL\n");
		return 1;
	}
	if (! CBFileOpen(&file, "./test.dat", true)) {
		printf("OPEN NEW FAIL\n");
		return 1;
	}
	uint32_t len;
	if (! CBFileGetLength(file, &len)) {
		printf("NEW DATA GET LEN FAIL\n");
		return 1;
	}
	if (len) {
		printf("NEW DATA LEN FAIL\n");
		return 1;
	}
	if (! CBFileAppend(file, (uint8_t *)"Hello There World!", 18)) {
		printf("NEW APPEND FAIL\n");              
		return 1;
	}
	if (! CBFileSeek(file, 0)) {
		printf("NEW SEEK FAIL\n");
		return 1;
	}
	uint8_t data[44];
	if (! CBFileRead(file, data, 11)) {
		printf("NEW READ FAIL\n");
		return 1;
	}
	if (memcmp(data, "Hello There", 11)) {
		printf("NEW READ DATA FAIL\n");
		return 1;
	}
	if (! CBFileRead(file, data, 7)) {
		printf("NEW READ SEEK FAIL\n");
		return 1;
	}
	if (memcmp(data, " World!", 7)) {
		printf("NEW READ SEEK DATA FAIL\n");
		return 1;
	}
	if (! CBFileSeek(file, 6)) {
		printf("NEW SEEK 6 FAIL\n");
		return 1;
	}
	if (! CBFileOverwrite(file, (uint8_t *)"Good People.", 12)) {
		printf("NEW OVERWRITE FAIL\n");
		return 1;
	}
	if (! CBFileSeek(file, 0)) {
		printf("NEW SEEK AFTER OVERWRITE FAIL\n");
		return 1;
	}
	if (! CBFileRead(file, data, 18)) {
		printf("NEW READ ALL FAIL\n");
		return 1;
	}
	if (memcmp(data, "Hello Good People.", 18)) {
		printf("NEW OVERWRITE DATA FAIL\n");
		return 1;
	}
	if (! CBFileAppend(file, (uint8_t *)" Now Goodbye!", 13)) {
		printf("NEW 2ND APPEND FAIL\n");
		return 1;
	}
	if (! CBFileSeek(file, 0)) {
		printf("NEW SEEK AFTER 2ND APPEND FAIL\n");
		return 1;
	}
	if (! CBFileRead(file, data, 31)) {
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
	if (! CBFileOpen(&file, "./test.dat", false)){
		printf("EXISTING OPEN FAIL\n");
		return 1;
	}
	CBFileGetLength(file, &len);
	if (len != 31) {
		printf("EXISTING DATA LENGTH FAIL\n");
		return 1;
	}
	if (! CBFileRead(file, data, 31)) {
		printf("EXISTING READ FAIL\n");
		return 1;
	}                       
	if (memcmp(data, "Hello Good People. Now Goodbye!", 31)) {
		printf("EXISTING READ DATA FAIL\n");
		return 1;
	}
	if (! CBFileSeek(file, 6)) {
		printf("EXISTING SEEK FAIL\n");
		return 1;
	}
	if (! CBFileOverwrite(file, (uint8_t *)"Great World!", 12)) {
		printf("EXISTING OVERWRITE FAIL\n");
		return 1;
	}
	if (! CBFileAppend(file, (uint8_t *)" Hello Again.", 13)) {
		printf("EXISTING APPEND FAIL\n");
		return 1;
	}
	if (! CBFileSeek(file, 0)) {
		printf("EXISTING SEEK TO START FAIL\n");
		return 1;
	}
	if (! CBFileRead(file, data, 44)) {
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
	rewind(((CBFile *)file.ptr)->rdwr);
	fwrite((uint8_t []){44}, 1, 1, ((CBFile *)file.ptr)->rdwr);
	fseek(((CBFile *)file.ptr)->rdwr, 5, SEEK_SET);
	fwrite((uint8_t []){74}, 1, 1, ((CBFile *)file.ptr)->rdwr);
	CBFileClose(file);
	if (! CBFileOpen(&file,"./test.dat", false)) {
		printf("LENGTH BIT ERR OPEN FAIL\n");
		return 1;
	}
	CBFileGetLength(file, &len);
	if (len != 44) {
		printf("LENGTH BIT ERR LENGTH FAIL\n");
		return 1;
	}
	if (! CBFileRead(file, data, 1)) {
		printf("DATA BIT ERR READ FAIL\n");
		return 1;
	}
	if (data[0] != 'H') {
		printf("DATA BIT ERR READ DATA FAIL\n");
		return 1;
	}
	if (! CBFileSeek(file, 6)) {
		printf("DOUBLE OVERWRITE SEEK FAIL\n");
		return 1;
	}
	if (! CBFileOverwrite(file, (uint8_t *)"Super Lands.", 12)) {
		printf("DOUBLE OVERWRITE ONE FAIL\n");
		return 1;
	}
	if (! CBFileOverwrite(file, (uint8_t *)" Very Lovely.", 13)) {
		printf("DOUBLE OVERWRITE TWO FAIL\n");
		return 1;
	}
	if (! CBFileSeek(file, 6)) {
		printf("DOUBLE OVERWRITE READ SEEK FAIL\n");
		return 1;
	}
	if (! CBFileRead(file, data, 15)) {
		printf("DOUBLE OVERWRITE READ FAIL\n");
		return 1;
	}
	if (memcmp(data, "Super Lands. Very Lovely.", 15)) {
		printf("DOUBLE OVERWRITE READ DATA FAIL\n");
		return 1;
	}
	CBFileClose(file);
	// Test Truncation
	if (! CBFileTruncate("./test.dat", 31)) {
		printf("TRUNCATE FAIL\n");
		return 1;
	}
	if (! CBFileOpen(&file,"./test.dat", false)) {
		printf("TRUNCATE OPEN FAIL\n");
		return 1;
	}
	CBFileGetLength(file, &len);
	if (len != 31) {
		printf("TRUNCATE LENGTH FAIL\n");
		return 1;
	}
	if (! CBFileRead(file, data, 31)) {
		printf("TRUNCATE READ FAIL\n");
		return 1;
	}
	if (memcmp(data, "Hello Super Lands. Very Lovely.", 31)) {
		printf("TRUNCATE READ DATA FAIL\n");
		return 1;
	}
	// Test append with zeros.
	if (! CBFileAppendZeros(file, 30)) {
		printf("APPEND ZERO FAIL\n");
		return 1;
	}
	CBFileGetLength(file, &len);
	if (len != 61) {
		printf("APPEND ZERO LEN FAIL\n");
		return 1;
	}
	if (! CBFileSeek(file, 41)) {
		printf("APPEND ZERO SEEK FAIL\n");
		return 1;
	}
	if (! CBFileRead(file, data, 20)) {
		printf("APPEND ZERO READ FAIL\n");
		return 1;
	}
	if (memcmp(data, (uint8_t []){0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, 20)) {
		printf("APPEND ZERO READ DATA FAIL\n");
		return 1;
	}
	CBFileClose(file);
	if (! CBFileOpen(&file, "./test.dat", false)) {
		printf("APPEND ZERO REOPEN FAIL");
		return 1;
	}
	CBFileGetLength(file, &len);
	if (len != 61) {
		printf("APPEND ZERO REOPEN LEN FAIL\n");
		return 1;
	}
	if (! CBFileSeek(file, 41)) {
		printf("APPEND ZERO REOPEN SEEK FAIL\n");
		return 1;
	}
	if (! CBFileRead(file, data, 20)) {
		printf("APPEND ZERO REOPEN READ FAIL\n");
		return 1;
	}
	if (memcmp(data, (uint8_t []){0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, 20)) {
		printf("APPEND ZERO REOPEN READ DATA FAIL\n");
		return 1;
	}
	return 0;
}
