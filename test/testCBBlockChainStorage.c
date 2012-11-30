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
	printf("Session = %ui\n",s);
	srand(s);
	// Test
	remove("./0.dat");
	remove("./1.dat");
	remove("./2.dat");
	remove("./log.dat");
	CBBlockChainStorage * storage = (CBBlockChainStorage *)CBNewBlockChainStorage("./", logError);
	if (memcmp(storage->dataDir,"./",3)) {
		printf("OBJECT DATA DIR FAIL\n");
		return 1;
	}
	if (storage->lastFile != 2) {
		printf("OBJECT LAST FILE FAIL\n");
		return 1;
	}
	if (storage->lastSize) {
		printf("OBJECT LAST FILE SIZE FAIL\n");
		return 1;
	}
	if (storage->numDeletionValues) {
		printf("OBJECT NUM DEL VALS FAIL\n");
		return 1;
	}
	if (storage->numValues) {
		printf("OBJECT NUM IDX VALS FAIL\n");
		return 1;
	}
	if (storage->numValueWrites) {
		printf("OBJECT NUM VAL WRITES FAIL\n");
		return 1;
	}
	CBFreeBlockChainStorage((uint64_t)storage);
	// Check index files are OK
	FILE * index = fopen("./0.dat", "rb");
	uint8_t data[105];
	if (fread(data, 1, 10, index) != 10) {
		printf("INDEX CREATE FAIL\n");
		return 1;
	}
	if (memcmp(data, (uint8_t [10]){0,0,0,0,2,0,0,0,0,0}, 10)) {
		printf("INDEX INITIAL DATA FAIL\n");
		return 1;
	}
	fclose(index);
	index = fopen("./1.dat", "rb");
	if (fread(data, 1, 4, index) != 4) {
		printf("DELETION INDEX CREATE FAIL\n");
		return 1;
	}
	if (memcmp(data, (uint8_t [10]){0,0,0,0}, 4)) {
		printf("DELETION INDEX INITIAL DATA FAIL\n");
		return 1;
	}
	fclose(index);
	// Test opening of initial files
	storage = (CBBlockChainStorage *)CBNewBlockChainStorage("./", logError);
	if (storage->numValues) {
		printf("INDEX NUM VALUES FAIL\n");
		return 1;
	}
	if (storage->lastFile != 2) {
		printf("INDEX NUM FILES FAIL\n");
		return 1;
	}
	if (storage->lastSize) {
		printf("INDEX LAST SIZE FAIL\n");
		return 1;
	}
	if (storage->numDeletionValues) {
		printf("DELETION NUM FAIL\n");
		return 1;
	}
	if (storage->numValueWrites) {
		printf("INITIAL NUM VALUE WRITES FAIL\n");
		return 1;
	}
	// Try inserting a value
	uint8_t key[6];
	for (int x = 0; x < 6; x++) {
		key[x] = rand();
	}
	CBBlockChainStorageWriteValue((uint64_t)storage, key, (uint8_t *)"Hi There Mate!", 15, 0, 15);
	CBBlockChainStorageCommitData((uint64_t)storage);
	// Now check over data
	char readStr[15];
	CBBlockChainStorageReadValue((uint64_t)storage, key, (uint8_t *)readStr, 15, 0);
	if (memcmp(readStr, "Hi There Mate!", 15)) {
		printf("READ ALL VALUE FAIL\n");
		return 1;
	}
	if (storage->numValues != 1) {
		printf("INSERT SINGLE VAL INDEX NUM VALUES FAIL\n");
		return 1;
	}
	if (storage->lastFile != 2) {
		printf("INSERT SINGLE VAL NUM FILES FAIL\n");
		return 1;
	}
	if (storage->lastSize != 15) {
		printf("INSERT SINGLE VAL LAST SIZE FAIL\n");
		return 1;
	}
	index = fopen("./0.dat", "rb");
	if (fread(data, 1, 26, index) != 26) {
		printf("INSERT SINGLE VAL INDEX READ FAIL\n");
		return 1;
	}
	if (memcmp(data, (uint8_t [26]){
		1,0,0,0, // One value
		2,0, // File 2 is last
		15,0,0,0, // Size of file is 15
		key[0],key[1],key[2],key[3],key[4],key[5],
		2,0, // File index 2
		0,0,0,0, // Position 0
		15,0,0,0, // Data length is 15
	}, 26)) {
		printf("INSERT SINGLE VAL INDEX DATA FAIL\n");
		return 1;
	}
	fclose(index);
	index = fopen("./1.dat", "rb");
	if (fread(data, 1, 4, index) != 4) {
		printf("INSERT SINGLE VAL DELETION INDEX READ FAIL\n");
		return 1;
	}
	if (memcmp(data, (uint8_t [10]){0,0,0,0}, 4)) {
		printf("INSERT SINGLE VAL DELETION INDEX DATA FAIL\n");
		return 1;
	}
	fclose(index);
	index = fopen("./log.dat", "rb");
	if (fread(data, 1, 39, index) != 39) {
		printf("INSERT SINGLE VAL LOG FILE READ FAIL\n");
		return 1;
	}
	if (memcmp(data, (uint8_t [39]){
		0, // Inactive
		10,0,0,0, // Index previously size 10
		4,0,0,0, // Deletion index previously size 4
		2,0, // Previous last file 2
		0,0,0,0, // Previous last file size 0
		0,0, // Overwrite file ID index
		0,0,0,0,0,0,0,0, // Offset is zero
		10,0,0,0, // Length of change is 10
		0,0,0,0,2,0,0,0,0,0, // Previous data
	}, 39)) {
		printf("INSERT SINGLE VAL LOG FILE DATA FAIL\n");
		return 1;
	}
	fclose(index);
	// Try reding a section of the value
	CBBlockChainStorageReadValue((uint64_t)storage, key, (uint8_t *)readStr, 5, 3);
	if (memcmp(readStr, "There", 5)) {
		printf("READ PART VALUE FAIL\n");
		return 1;
	}
	// Try adding a new value and replacing the previous value together.
	uint8_t key2[6];
	for (int x = 0; x < 6; x++) {
		key2[x] = rand();
	}
	CBBlockChainStorageWriteValue((uint64_t)storage, key2, (uint8_t *)"Another one", 12, 0, 12);
	CBBlockChainStorageWriteValue((uint64_t)storage, key, (uint8_t *)"Replacement!!!", 15, 0, 15);
	CBBlockChainStorageCommitData((uint64_t)storage);
	CBBlockChainStorageReadValue((uint64_t)storage, key, (uint8_t *)readStr, 15, 0);
	if (memcmp(readStr, "Replacement!!!", 15)) {
		printf("READ REPLACED VALUE FAIL\n");
		return 1;
	}
	CBBlockChainStorageReadValue((uint64_t)storage, key2, (uint8_t *)readStr, 12, 0);
	if (memcmp(readStr, "Another one", 12)) {
		printf("READ 2ND VALUE FAIL\n");
		return 1;
	}
	if (storage->numValues != 2) {
		printf("INSERT 2ND VAL INDEX NUM VALUES FAIL\n");
		return 1;
	}
	if (storage->lastFile != 2) {
		printf("INSERT 2ND VAL NUM FILES FAIL\n");
		return 1;
	}
	if (storage->lastSize != 27) {
		printf("INSERT 2ND VAL LAST SIZE FAIL\n");
		return 1;
	}
	index = fopen("./0.dat", "rb");
	if (fread(data, 1, 42, index) != 42) {
		printf("INSERT 2ND VAL INDEX READ FAIL\n");
		return 1;
	}
	if (memcmp(data, (uint8_t [42]){
		2,0,0,0, // Two values
		2,0, // File 2 is last
		27,0,0,0, // Size of file is 27
		key[0],key[1],key[2],key[3],key[4],key[5],
		2,0, // File index 2
		0,0,0,0, // Position 0
		15,0,0,0, // Data length is 15
		key2[0],key2[1],key2[2],key2[3],key2[4],key2[5],
		2,0, // File index 2
		15,0,0,0, // Position 15
		12,0,0,0, // Data length is 12
	}, 42)) {
		printf("INSERT 2ND VAL INDEX DATA FAIL\n");
		return 1;
	}
	fclose(index);
	index = fopen("./1.dat", "rb");
	if (fread(data, 1, 4, index) != 4) {
		printf("INSERT 2ND VAL DELETION INDEX READ FAIL\n");
		return 1;
	}
	if (memcmp(data, (uint8_t [10]){0,0,0,0}, 4)) {
		printf("INSERT 2ND VAL DELETION INDEX DATA FAIL\n");
		return 1;
	}
	fclose(index);
	index = fopen("./log.dat", "rb");
	if (fread(data, 1, 68, index) != 68) {
		printf("INSERT 2ND VAL LOG FILE READ FAIL\n");
		return 1;
	}
	if (memcmp(data, (uint8_t [68]){
		0, // Inactive
		26,0,0,0, // Index previously size 26
		4,0,0,0, // Deletion index previously size 4
		2,0, // Previous last file 2
		15,0,0,0, // Previous last file size 15
		2,0, // Overwrite the data file
		0,0,0,0,0,0,0,0, // Offset is zero
		15,0,0,0, // Length of change is 15
		'H','i',' ','T','h','e','r','e',' ','M','a','t','e','!','\0', // Previous data
		0,0, // Overwrite the index
		0,0,0,0,0,0,0,0, // Offset is zero
		10,0,0,0, // Length of change is 10
		1,0,0,0,2,0,15,0,0,0, // Previous data
	}, 68)) {
		printf("INSERT 2ND VAL LOG FILE DATA FAIL\n");
		return 1;
	}
	fclose(index);
	// Remove first value
	CBBlockChainStorageRemoveValue((uint64_t)storage, key);
	CBBlockChainStorageCommitData((uint64_t)storage);
	// Check data
	CBBlockChainStorageReadValue((uint64_t)storage, key2, (uint8_t *)readStr, 12, 0);
	if (memcmp(readStr, "Another one", 12)) {
		printf("DELETE 1ST 2ND VALUE FAIL\n");
		return 1;
	}
	if (storage->numValues != 2) {
		printf("DELETE 1ST VAL INDEX NUM VALUES FAIL\n");
		return 1;
	}
	if (storage->lastFile != 2) {
		printf("DELETE 1ST VAL NUM FILES FAIL\n");
		return 1;
	}
	if (storage->lastSize != 27) {
		printf("DELETE 1ST VAL LAST SIZE FAIL\n");
		return 1;
	}
	index = fopen("./0.dat", "rb");
	if (fread(data, 1, 42, index) != 42) {
		printf("DELETE 1ST VAL INDEX READ FAIL\n");
		return 1;
	}
	if (memcmp(data, (uint8_t [42]){
		2,0,0,0, // Two values
		2,0, // File 2 is last
		27,0,0,0, // Size of file is 27
		key[0],key[1],key[2],key[3],key[4],key[5],
		2,0, // File index 2
		0,0,0,0, // Position 0
		0,0,0,0, // Data length is 0 ie. deleted
		key2[0],key2[1],key2[2],key2[3],key2[4],key2[5],
		2,0, // File index 2
		15,0,0,0, // Position 15
		12,0,0,0, // Data length is 12
	}, 42)) {
		printf("DELETE 1ST VAL INDEX DATA FAIL\n");
		return 1;
	}
	fclose(index);
	index = fopen("./1.dat", "rb");
	if (fread(data, 1, 15, index) != 15) {
		printf("DELETE 1ST VAL DELETION INDEX READ FAIL\n");
		return 1;
	}
	if (memcmp(data, (uint8_t [15]){
		1,0,0,0, // One deletion entry.
		1, // Active deletion entry.
		0,0,0,15, // Big endian length of deleted section.
		2,0, // File-ID is 2
		0,0,0,0, // Position is 0
	}, 15)) {
		printf("DELETE 1ST VAL DELETION INDEX DATA FAIL\n");
		return 1;
	}
	fclose(index);
	index = fopen("./log.dat", "rb");
	if (fread(data, 1, 51, index) != 51) {
		printf("DELETE 1ST VAL LOG FILE READ FAIL\n");
		return 1;
	}
	if (memcmp(data, (uint8_t [51]){
		0, // Inactive
		42,0,0,0, // Index previously size 42
		4,0,0,0, // Deletion index previously size 4
		2,0, // Previous last file 2
		27,0,0,0, // Previous last file size 27
		0,0, // Overwrite the index
		22,0,0,0,0,0,0,0, // Offset is 22
		4,0,0,0, // Length of change is 4
		15,0,0,0, // Previous data
		1,0, // Overwrite the deletion index
		0,0,0,0,0,0,0,0, // Offset is zero
		4,0,0,0, // Length of change is 4
		0,0,0,0, // Previous data
	}, 51)) {
		printf("DELETE 1ST VAL LOG FILE DATA FAIL\n");
		return 1;
	}
	fclose(index);
	// Increase size of second key-value to 15 to replace deleted section and use offset
	CBBlockChainStorageWriteValue((uint64_t)storage, key2, (uint8_t *)"Annoying code.", 15, 0, 15);
	CBBlockChainStorageCommitData((uint64_t)storage);   
	// Look at data
	CBBlockChainStorageReadValue((uint64_t)storage, key2, (uint8_t *)readStr, 15, 0);
	if (memcmp(readStr, "Annoying code.", 15)) {
		printf("INCREASE 2ND VALUE FAIL\n");
		return 1;
	}
	if (storage->lastSize != 27) {
		printf("INCREASE 2ND VAL LAST SIZE FAIL\n");
		return 1;
	}
	index = fopen("./0.dat", "rb");
	if (fread(data, 1, 42, index) != 42) {
		printf("DELETE 1ST VAL INDEX READ FAIL\n");
		return 1;
	}
	if (memcmp(data, (uint8_t [42]){
		2,0,0,0, // Two values
		2,0, // File 2 is last
		27,0,0,0, // Size of file is 27
		key[0],key[1],key[2],key[3],key[4],key[5],
		2,0, // File index 2
		0,0,0,0, // Position 0
		0,0,0,0, // Data length is 0 ie. deleted
		key2[0],key2[1],key2[2],key2[3],key2[4],key2[5],
		2,0, // File index 2
		0,0,0,0, // Position 0
		15,0,0,0, // Data length is 15
	}, 42)) {
		printf("INCREASE 2ND VAL INDEX DATA FAIL\n");
		return 1;
	}
	fclose(index);
	index = fopen("./1.dat", "rb");
	if (fread(data, 1, 26, index) != 26) {
		printf("INCREASE 2ND VAL DELETION INDEX READ FAIL\n");
		return 1;
	}
	if (memcmp(data, (uint8_t [26]){
		2,0,0,0, // Two deletion entries.
		0, // Inactive deletion entry.
		0,0,0,15, // Big endian length of deleted section.
		2,0, // File-ID is 2
		0,0,0,0, // Position is 0
		1, // Active deletion entry
		0,0,0,12, // Big endian length of deleted section.
		2,0, // File-ID is 2
		15,0,0,0, // Position is 15
	}, 26)) {
		printf("INCREASE 2ND VAL DELETION INDEX DATA FAIL\n");
		return 1;
	}
	fclose(index);
	index = fopen("./log.dat", "rb");
	if (fread(data, 1, 105, index) != 105) {
		printf("INCREASE 2ND VAL LOG FILE READ FAIL\n");
		return 1;
	}
	if (memcmp(data, (uint8_t [105]){
		0, // Inactive
		42,0,0,0, // Index previously size 42
		15,0,0,0, // Deletion index previously size 15
		2,0, // Previous last file 2
		27,0,0,0, // Previous last file size 27
		
		2,0, // Overwrite the data file
		0,0,0,0,0,0,0,0, // Offset is 0
		15,0,0,0, // Length of change is 15
		'R','e','p','l','a','c','e','m','e','n','t','!','!','!','\0',
		
		1,0, // Overwrite the deletion index
		4,0,0,0,0,0,0,0, // Offset is 4
		5,0,0,0, // Length of change is 5
		1,0,0,0,15, // Previous data
		
		0,0, // Overwrite index
		32,0,0,0,0,0,0,0, // Offset is 32
		10,0,0,0, // Length of change is 10
		2,0,15,0,0,0,12,0,0,0, // Previous data
		
		1,0, // Overwrite the deletion index
		0,0,0,0,0,0,0,0, // Offset is zero
		4,0,0,0, // Length of change is 4
		1,0,0,0, // Previous data
	}, 105)) {
		printf("INCREASE 2ND VAL LOG FILE DATA FAIL\n");
		return 1;
	}
	fclose(index);
	// Add the first value again with offset and larger total size outside of data
	CBBlockChainStorageWriteValue((uint64_t)storage, key, (uint8_t *)"Key-Value", 10, 5, 20);
	CBBlockChainStorageCommitData((uint64_t)storage);
	// Look at data
	CBBlockChainStorageReadValue((uint64_t)storage, key, (uint8_t *)readStr, 10, 5);
	if (memcmp(readStr, "Key-Value", 10)) {
		printf("NEW 1ST VALUE FAIL\n");
		return 1;
	}
	if (storage->lastSize != 47) {
		printf("NEW 1ST VAL LAST SIZE FAIL\n");
		return 1;
	}
	index = fopen("./0.dat", "rb");
	if (fread(data, 1, 42, index) != 42) {
		printf("NEW 1ST VAL INDEX READ FAIL\n");
		return 1;
	}
	if (memcmp(data, (uint8_t [42]){
		2,0,0,0, // Two values
		2,0, // File 2 is last
		47,0,0,0, // Size of file is 47
		key[0],key[1],key[2],key[3],key[4],key[5],
		2,0, // File index 2
		27,0,0,0, // Position at 27
		20,0,0,0, // Data length is 20
		key2[0],key2[1],key2[2],key2[3],key2[4],key2[5],
		2,0, // File index 2
		0,0,0,0, // Position 0
		15,0,0,0, // Data length is 15
	}, 42)) {
		printf("NEW 1ST VAL INDEX DATA FAIL\n");
		return 1;
	}
	fclose(index);
	index = fopen("./1.dat", "rb");
	if (fread(data, 1, 26, index) != 26) {
		printf("NEW 1ST VAL DELETION INDEX READ FAIL\n");
		return 1;
	}
	if (memcmp(data, (uint8_t [26]){
		2,0,0,0, // Two deletion entries.
		0, // Inactive deletion entry.
		0,0,0,15, // Big endian length of deleted section.
		2,0, // File-ID is 2
		0,0,0,0, // Position is 0
		1, // Active deletion entry
		0,0,0,12, // Big endian length of deleted section.
		2,0, // File-ID is 2
		15,0,0,0, // Position is 15
	}, 26)) {
		printf("NEW 1ST VAL DELETION INDEX DATA FAIL\n");
		return 1;
	}
	fclose(index);
	index = fopen("./log.dat", "rb");
	if (fread(data, 1, 63, index) != 63) {
		printf("NEW 1ST VAL LOG FILE READ FAIL\n");
		return 1;
	}
	if (memcmp(data, (uint8_t [63]){
		0, // Inactive
		42,0,0,0, // Index previously size 42
		26,0,0,0, // Deletion index previously size 26
		2,0, // Previous last file 2
		27,0,0,0, // Previous last file size 27
		
		0,0, // Overwrite the index file
		16,0,0,0,0,0,0,0, // Offset is 16
		10,0,0,0, // Length of change is 10
		2,0,0,0,0,0,0,0,0,0, // Previous data
		
		0,0, // Overwrite the index file
		0,0,0,0,0,0,0,0, // Offset is 0
		10,0,0,0, // Length of change is 10
		2,0,0,0,2,0,27,0,0,0, // Previous data.
	}, 63)) {
		printf("NEW 1ST VAL LOG FILE DATA FAIL\n");
		return 1;
	}
	fclose(index);
	// Add data to both sides of the first value
	CBBlockChainStorageWriteValue((uint64_t)storage, key, (uint8_t *)"Moon ", 5, 0, 20);
	CBBlockChainStorageWriteValue((uint64_t)storage, key, (uint8_t *)" Show", 6, 14, 20);
	CBBlockChainStorageCommitData((uint64_t)storage);
	// Look at data
	CBBlockChainStorageReadValue((uint64_t)storage, key, (uint8_t *)readStr, 20, 0);
	if (memcmp(readStr, "Moon Key-Value Show", 20)) {
		printf("SUBWRITE VALUE FAIL\n");
		return 1;
	}
	if (storage->lastSize != 47) {
		printf("SUBWRITE LAST SIZE FAIL\n");
		return 1;
	}
	index = fopen("./0.dat", "rb");
	if (fread(data, 1, 42, index) != 42) {
		printf("SUBWRITE INDEX READ FAIL\n");
		return 1;
	}
	if (memcmp(data, (uint8_t [42]){
		2,0,0,0, // Two values
		2,0, // File 2 is last
		47,0,0,0, // Size of file is 47
		key[0],key[1],key[2],key[3],key[4],key[5],
		2,0, // File index 2
		27,0,0,0, // Position at 27
		20,0,0,0, // Data length is 20
		key2[0],key2[1],key2[2],key2[3],key2[4],key2[5],
		2,0, // File index 2
		0,0,0,0, // Position 0
		15,0,0,0, // Data length is 15
	}, 42)) {
		printf("SUBWRITE INDEX DATA FAIL\n");
		return 1;
	}
	fclose(index);
	index = fopen("./1.dat", "rb");
	if (fread(data, 1, 26, index) != 26) {
		printf("SUBWRITE DELETION INDEX READ FAIL\n");
		return 1;
	}
	if (memcmp(data, (uint8_t [26]){
		2,0,0,0, // Two deletion entries.
		0, // Inactive deletion entry.
		0,0,0,15, // Big endian length of deleted section.
		2,0, // File-ID is 2
		0,0,0,0, // Position is 0
		1, // Active deletion entry
		0,0,0,12, // Big endian length of deleted section.
		2,0, // File-ID is 2
		15,0,0,0, // Position is 15
	}, 26)) {
		printf("SUBWRITE DELETION INDEX DATA FAIL\n");
		return 1;
	}
	fclose(index);
	index = fopen("./log.dat", "rb");
	if (fread(data, 1, 54, index) != 54) {
		printf("SUBWRITE LOG FILE READ FAIL\n");
		return 1;
	}
	if (memcmp(data, (uint8_t [54]){
		0, // Inactive
		42,0,0,0, // Index previously size 42
		26,0,0,0, // Deletion index previously size 26
		2,0, // Previous last file 2
		47,0,0,0, // Previous last file size 47
		
		2,0, // Overwrite the data file
		27,0,0,0,0,0,0,0, // Offset is 27
		5,0,0,0, // Length of change is 5
		0,0,0,0,0, // Previous data
		
		2,0, // Overwrite the data file
		41,0,0,0,0,0,0,0, // Offset is 41
		6,0,0,0, // Length of change is 6
		0,0,0,0,0,0, // Previous data
	}, 54)) {
		printf("SUBWRITE LOG FILE DATA FAIL\n");
		return 1;
	}
	fclose(index);
	// Re-open
	CBFreeBlockChainStorage((uint64_t)storage);
	storage = (CBBlockChainStorage *)CBNewBlockChainStorage("./", logError);
	CBBlockChainStorageReadValue((uint64_t)storage, key, (uint8_t *)readStr, 20, 0);
	if (memcmp(readStr, "Moon Key-Value Show", 20)) {
		printf("REOPEN VALUE 1 FAIL\n");
		return 1;
	}
	CBBlockChainStorageReadValue((uint64_t)storage, key2, (uint8_t *)readStr, 15, 0);
	if (memcmp(readStr, "Annoying code.", 15)) {
		printf("REOPEN VALUE 2 FAIL\n");
		return 1;
	}
	if (storage->lastSize != 47) {
		printf("REOPEN LAST SIZE FAIL\n");
		return 1;
	}
	if (storage->lastFile != 2) {
		printf("REOPEN LAST FILE FAIL\n");
		return 1;
	}
	if (storage->numValues != 2) {
		printf("REOPEN NUM VALUES FAIL\n");
		return 1;
	}
	if (storage->numDeletionValues != 2) {
		printf("REOPEN NUM DELETION VALUES FAIL\n");
		return 1;
	}
	// Add value and try database recovery.
	uint8_t key3[6];
	for (int x = 0; x < 6; x++) {
		key3[x] = rand();
	}
	CBBlockChainStorageWriteValue((uint64_t)storage, key3, (uint8_t *)"cbitcoin", 9, 0, 9);
	CBBlockChainStorageCommitData((uint64_t)storage);
	// Ensure value is OK
	CBBlockChainStorageReadValue((uint64_t)storage, key3, (uint8_t *)readStr, 9, 0);
	if (memcmp(readStr, "cbitcoin", 9)) {
		printf("3RD VALUE FAIL\n");
		return 1;
	}
	// Free everything
	CBFreeBlockChainStorage((uint64_t)storage);
	// Activate log file
	FILE * file = fopen("./log.dat", "rb+");
	data[0] = 1;
	fwrite(data, 1, 1, file);
	fclose(file);
	// Load storage object
	storage = (CBBlockChainStorage *)CBNewBlockChainStorage("./", logError);
	// Verify recovery.
	CBBlockChainStorageReadValue((uint64_t)storage, key, (uint8_t *)readStr, 20, 0);
	if (memcmp(readStr, "Moon Key-Value Show", 20)) {
		printf("RECOVERY VALUE 1 FAIL\n");
		return 1;
	}
	CBBlockChainStorageReadValue((uint64_t)storage, key2, (uint8_t *)readStr, 15, 0);
	if (memcmp(readStr, "Annoying code.", 15)) {
		printf("RECOVERY VALUE 2 FAIL\n");
		return 1;
	}
	if (storage->lastSize != 47) {
		printf("RECOVERY LAST SIZE FAIL\n");
		return 1;
	}
	if (storage->lastFile != 2) {
		printf("RECOVERY LAST FILE FAIL\n");
		return 1;
	}
	if (storage->numValues != 2) {
		printf("RECOVERY NUM VALUES FAIL\n");
		return 1;
	}
	if (storage->numDeletionValues != 2) {
		printf("RECOVERY NUM DELETION VALUES FAIL\n");
		return 1;
	}
	// Check files
	index = fopen("./0.dat", "rb");
	if (fread(data, 1, 42, index) != 42) {
		printf("RECOVERY INDEX READ FAIL\n");
		return 1;
	}
	if (memcmp(data, (uint8_t [42]){
		2,0,0,0, // Two values
		2,0, // File 2 is last
		47,0,0,0, // Size of file is 47
		key[0],key[1],key[2],key[3],key[4],key[5],
		2,0, // File index 2
		27,0,0,0, // Position at 27
		20,0,0,0, // Data length is 20
		key2[0],key2[1],key2[2],key2[3],key2[4],key2[5],
		2,0, // File index 2
		0,0,0,0, // Position 0
		15,0,0,0, // Data length is 15
	}, 42)) {
		printf("RECOVERY INDEX DATA FAIL\n");
		return 1;
	}
	fclose(index);
	index = fopen("./1.dat", "rb");
	if (fread(data, 1, 26, index) != 26) {
		printf("RECOVERY DELETION INDEX READ FAIL\n");
		return 1;
	}
	if (memcmp(data, (uint8_t [26]){
		2,0,0,0, // Two deletion entries.
		0, // Inactive deletion entry.
		0,0,0,15, // Big endian length of deleted section.
		2,0, // File-ID is 2
		0,0,0,0, // Position is 0
		1, // Active deletion entry
		0,0,0,12, // Big endian length of deleted section.
		2,0, // File-ID is 2
		15,0,0,0, // Position is 15
	}, 26)) {
		printf("RECOVERY DELETION INDEX DATA FAIL\n");
		return 1;
	}
	fclose(index);
	index = fopen("./2.dat", "rb");
	if (fread(data, 1, 47, index) != 47) {
		printf("RECOVERY DATA FILE READ FAIL\n");
		return 1;
	}
	if (memcmp(data, "Annoying code.\0Another one\0Moon Key-Value Show\0", 47)) {
		printf("RECOVERY DATA FILE DATA FAIL\n");
		return 1;
	}
	fclose(index);
	// Add value with smaller total length than before
	CBBlockChainStorageWriteValue((uint64_t)storage, key, (uint8_t *)"Maniac", 7, 0, 7);
	CBBlockChainStorageCommitData((uint64_t)storage);
	// Look at data
	CBBlockChainStorageReadValue((uint64_t)storage, key, (uint8_t *)readStr, 7, 0);
	if (memcmp(readStr, "Maniac", 7)) {
		printf("SMALLER VALUE FAIL\n");
		return 1;
	}
	if (storage->lastSize != 47) {
		printf("SMALLER LAST SIZE FAIL\n");
		return 1;
	}
	index = fopen("./0.dat", "rb");
	if (fread(data, 1, 42, index) != 42) {
		printf("SMALLER INDEX READ FAIL\n");
		return 1;
	}
	if (memcmp(data, (uint8_t [42]){
		2,0,0,0, // Two values
		2,0, // File 2 is last
		47,0,0,0, // Size of file is 47
		key[0],key[1],key[2],key[3],key[4],key[5],
		2,0, // File index 2
		27,0,0,0, // Position at 27
		7,0,0,0, // Data length is 7
		key2[0],key2[1],key2[2],key2[3],key2[4],key2[5],
		2,0, // File index 2
		0,0,0,0, // Position 0
		15,0,0,0, // Data length is 15
	}, 42)) {
		printf("SMALLER INDEX DATA FAIL\n");
		return 1;
	}
	fclose(index);
	index = fopen("./1.dat", "rb");
	if (fread(data, 1, 26, index) != 26) {
		printf("SMALLER DELETION INDEX READ FAIL\n");
		return 1;
	}
	if (memcmp(data, (uint8_t [26]){
		2,0,0,0, // Two deletion entries.
		1, // Active deletion entry.
		0,0,0,13, // Big endian length of deleted section.
		2,0, // File-ID is 2
		34,0,0,0, // Position is 34
		1, // Active deletion entry
		0,0,0,12, // Big endian length of deleted section.
		2,0, // File-ID is 2
		15,0,0,0, // Position is 15
	}, 26)) {
		printf("SMALLER DELETION INDEX DATA FAIL\n");
		return 1;
	}
	fclose(index);
	index = fopen("./log.dat", "rb");
	if (fread(data, 1, 79, index) != 79) {
		printf("SMALLER LOG FILE READ FAIL\n");
		return 1;
	}
	if (memcmp(data, (uint8_t [79]){
		0, // Inactive
		42,0,0,0, // Index previously size 42
		26,0,0,0, // Deletion index previously size 26
		2,0, // Previous last file 2
		47,0,0,0, // Previous last file size 47
		
		2,0, // Overwrite the data file
		27,0,0,0,0,0,0,0, // Offset is 27
		7,0,0,0, // Length of change is 7
		'M','o','o','n',' ','K','e', // Previous data
		
		1,0, // Overwrite the deletion index
		4,0,0,0,0,0,0,0, // Offset is 4
		11,0,0,0, // Length of change is 11
		0,0,0,0,15,2,0,0,0,0,0, // Previous data
		
		0,0, // Overwrite the index
		22,0,0,0,0,0,0,0, // Offset is 22
		4,0,0,0, // Length of change is 4
		20,0,0,0, // Previous data
	}, 79)) {
		printf("SMALLER LOG FILE DATA FAIL\n");
		return 1;
	}
	fclose(index);
	// Change the first key
	uint8_t key4[6];
	for (int x = 0; x < 6; x++) {
		key4[x] = rand();
	}
	CBBlockChainStorageChangeKey((uint64_t)storage, key, key4);
	CBBlockChainStorageCommitData((uint64_t)storage);
	// Check data
	CBBlockChainStorageReadValue((uint64_t)storage, key4, (uint8_t *)readStr, 7, 0);
	if (memcmp(readStr, "Maniac", 7)) {
		printf("CHANGE KEY VALUE FAIL\n");
		return 1;
	}
	if (storage->lastSize != 47) {
		printf("CHANGE KEY LAST SIZE FAIL\n");
		return 1;
	}
	index = fopen("./0.dat", "rb");
	if (fread(data, 1, 42, index) != 42) {
		printf("CHANGE KEY INDEX READ FAIL\n");
		return 1;
	}
	if (memcmp(data, (uint8_t [42]){
		2,0,0,0, // Two values
		2,0, // File 2 is last
		47,0,0,0, // Size of file is 47
		key4[0],key4[1],key4[2],key4[3],key4[4],key4[5],
		2,0, // File index 2
		27,0,0,0, // Position at 27
		7,0,0,0, // Data length is 7
		key2[0],key2[1],key2[2],key2[3],key2[4],key2[5],
		2,0, // File index 2
		0,0,0,0, // Position 0
		15,0,0,0, // Data length is 15
	}, 42)) {
		printf("CHANGE KEY INDEX DATA FAIL\n");
		return 1;
	}
	fclose(index);
	index = fopen("./1.dat", "rb");
	if (fread(data, 1, 26, index) != 26) {
		printf("CHANGE KEY DELETION INDEX READ FAIL\n");
		return 1;
	}
	if (memcmp(data, (uint8_t [26]){
		2,0,0,0, // Two deletion entries.
		1, // Active deletion entry.
		0,0,0,13, // Big endian length of deleted section.
		2,0, // File-ID is 2
		34,0,0,0, // Position is 34
		1, // Active deletion entry
		0,0,0,12, // Big endian length of deleted section.
		2,0, // File-ID is 2
		15,0,0,0, // Position is 15
	}, 26)) {
		printf("CHANGE KEY DELETION INDEX DATA FAIL\n");
		return 1;
	}
	fclose(index);
	index = fopen("./log.dat", "rb");
	if (fread(data, 1, 35, index) != 35) {
		printf("CHANGE KEY LOG FILE READ FAIL\n");
		return 1;
	}
	if (memcmp(data, (uint8_t [35]){
		0, // Inactive
		42,0,0,0, // Index previously size 42
		26,0,0,0, // Deletion index previously size 26
		2,0, // Previous last file 2
		47,0,0,0, // Previous last file size 47
		
		0,0, // Overwrite the index
		10,0,0,0,0,0,0,0, // Offset is 10
		6,0,0,0, // Length of change is 6
		key[0],key[1],key[2],key[3],key[4],key[5], // Previous data
	}, 35)) {
		printf("CHANGE KEY LOG FILE DATA FAIL\n");
		return 1;
	}
	fclose(index);
	CBFreeBlockChainStorage((uint64_t)storage);
	return 0;
}
