//
//  testCBBlockChainStorage.c
//  cbitcoin
//
//  Created by Matthew Mitchell on 11/11/2012.
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
#include "CBBlockChainStorage.h"
#include "CBDependencies.h"
#include <time.h>
#include "stdarg.h"

void logError(char * format,...);
void logError(char * format,...){
	va_list argptr;
    va_start(argptr, format);
    vfprintf(stderr, format, argptr);
    va_end(argptr);
	printf("\n");
}

int main(){
	unsigned int s = (unsigned int)time(NULL);
	s = 1337544566;
	printf("Session = %ui\n",s);
	srand(s);
	// Test
	remove("./0.dat");
	remove("./1.dat");
	uint64_t storage = CBNewBlockChainStorage("./", logError);
	CBFreeBlockChainStorage(storage);
	// Check index files are OK
	FILE * index = fopen("./0.dat", "rb");
	uint8_t data[10];
	if (fread(data, 1, 10, index) != 10) {
		printf("INDEX CREATE FAIL\n");
		return 0;
	}
	if (memcmp(data, (uint8_t [10]){0,0,0,0,1,0,0,0,0,0}, 10)) {
		printf("INDEX INITIAL DATA FAIL\n");
		return 0;
	}
	fclose(index);
	index = fopen("./1.dat", "rb");
	if (fread(data, 1, 4, index) != 4) {
		printf("DELETION INDEX CREATE FAIL\n");
		return 0;
	}
	if (memcmp(data, (uint8_t [10]){0,0,0,0}, 4)) {
		printf("DELETION INDEX INITIAL DATA FAIL\n");
		return 0;
	}
	fclose(index);
	// Test opening of initial files
	CBBlockChainStorage * storageObj = (CBBlockChainStorage *)CBNewBlockChainStorage("./", logError);
	if (storageObj->index != NULL) {
		printf("INDEX NULL FAIL\n");
		return 0;
	}
	if (storageObj->numValues != 0) {
		printf("INDEX NUM VALUES FAIL\n");
		return 0;
	}
	if (storageObj->lastFile != 2) {
		printf("INDEX NUM FILES FAIL\n");
		return 0;
	}
	if (storageObj->lastSize != 0) {
		printf("INDEX LAST SIZE FAIL\n");
		return 0;
	}
	if (storageObj->deletionIndex != NULL) {
		printf("DELETION INDEX NULL FAIL\n");
		return 0;
	}
	if (storageObj->numDeletions != 0) {
		printf("DELETION NUM FAIL\n");
		return 0;
	}
	if (storageObj->numValueWrites != 0) {
		printf("INITIAL NUM VALUE WRITES FAIL\n");
		return 0;
	}
	CBFreeBlockChainStorage((uint64_t)storageObj);
	return 0;
}
