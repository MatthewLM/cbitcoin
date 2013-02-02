//
//  testCBDatabase.c
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
#include "CBDatabase.h"
#include "CBDependencies.h"
#include <time.h>
#include "stdarg.h"

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
	printf("Session = %ui\n", s);
	srand(s);
	// Test
	remove("./test_0.dat");
	remove("./test_1.dat");
	remove("./test_2.dat");
	remove("./test_log.dat");
	CBDatabase * storage = CBNewDatabase("./", "test");
	if (NOT storage) {
		printf("NEW STORAGE FAIL\n");
		return 1;
	}
	if (memcmp(storage->dataDir, "./", 3)) {
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
	CBFreeDatabase(storage);
	// Check index files are OK
	uint64_t file = CBFileOpen("./test_0.dat", false);
	uint8_t data[105];
	if (NOT CBFileRead(file, data, 10)) {
		printf("INDEX CREATE FAIL\n");
		return 1;
	}
	if (memcmp(data, (uint8_t [10]){0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00}, 10)) {
		printf("INDEX INITIAL DATA FAIL\n");
		return 1;
	}
	CBFileClose(file);
	file = CBFileOpen("./test_1.dat", false);
	if (NOT CBFileRead(file, data, 4)) {
		printf("DELETION INDEX CREATE FAIL\n");
		return 1;
	}
	if (memcmp(data, (uint8_t [4]){0x00, 0x00, 0x00, 0x00}, 4)) {
		printf("DELETION INDEX INITIAL DATA FAIL\n");
		return 1;
	}
	CBFileClose(file);
	// Test opening of initial files
	storage = (CBDatabase *)CBNewDatabase("./", "test");
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
	// Try inserting a value
	uint8_t key[7];
	key[0] = 6;
	for (int x = 1; x < 7; x++) {
		key[x] = rand();
	}
	CBDatabaseWriteValue(storage, key, (uint8_t *)"Hi There Mate!", 15);
	CBDatabaseCommit(storage);
	// Now check over data
	char readStr[15];
	CBDatabaseReadValue(storage, key, (uint8_t *)readStr, 15, 0);
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
	file = CBFileOpen("./test_0.dat", false);
	if (NOT CBFileRead(file, data, 27)) {
		printf("INSERT SINGLE VAL INDEX READ FAIL\n");
		return 1;
	}
	if (memcmp(data, (uint8_t [27]){
		1, 0, 0, 0, // One value
		2, 0, // File 2 is last
		15, 0, 0, 0, // Size of file is 15
		key[0], key[1], key[2], key[3], key[4], key[5], key[6], 
		2, 0, // File index 2
		0, 0, 0, 0, // Position 0
		15, 0, 0, 0, // Data length is 15
	}, 27)) {
		printf("INSERT SINGLE VAL INDEX DATA FAIL\n");
		return 1;
	}
	CBFileClose(file);
	file = CBFileOpen("./test_1.dat", false);
	if (NOT CBFileRead(file, data, 4)) {
		printf("INSERT SINGLE VAL DELETION INDEX READ FAIL\n");
		return 1;
	}
	if (memcmp(data, (uint8_t [4]){0, 0, 0, 0}, 4)) {
		printf("INSERT SINGLE VAL DELETION INDEX DATA FAIL\n");
		return 1;
	}
	CBFileClose(file);
	file = CBFileOpen("./test_log.dat", false);
	if (NOT CBFileRead(file, data, 35)) {
		printf("INSERT SINGLE VAL LOG FILE READ FAIL\n");
		return 1;
	}
	if (memcmp(data, (uint8_t [35]){
		0, // Inactive
		10, 0, 0, 0, // Index previously size 10
		4, 0, 0, 0, // Deletion index previously size 4
		2, 0, // Previous last file 2
		0, 0, 0, 0, // Previous last file size 0
		0, 0, // Overwrite file ID index
		0, 0, 0, 0, // Offset is zero
		10, 0, 0, 0, // Length of change is 10
		0, 0, 0, 0, 2, 0, 0, 0, 0, 0, // Previous data
	}, 35)) {
		printf("INSERT SINGLE VAL LOG FILE DATA FAIL\n");
		return 1;
	}
	CBFileClose(file);
	// Try reding a section of the value
	CBDatabaseReadValue(storage, key, (uint8_t *)readStr, 5, 3);
	if (memcmp(readStr, "There", 5)) {
		printf("READ PART VALUE FAIL\n");
		return 1;
	}
	// Try adding a new value and replacing the previous value together.
	uint8_t key2[7];
	key2[0] = 6;
	for (int x = 1; x < 7; x++) {
		key2[x] = rand();
	}
	CBDatabaseWriteValue(storage, key2, (uint8_t *)"Another one", 12);
	CBDatabaseWriteValue(storage, key, (uint8_t *)"Replacement!!!", 15);
	CBDatabaseCommit(storage);
	CBDatabaseReadValue(storage, key, (uint8_t *)readStr, 15, 0);
	if (memcmp(readStr, "Replacement!!!", 15)) {
		printf("READ REPLACED VALUE FAIL\n");
		return 1;
	}
	CBDatabaseReadValue(storage, key2, (uint8_t *)readStr, 12, 0);
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
	file = CBFileOpen("./test_0.dat", false);
	if (NOT CBFileRead(file, data, 44)) {
		printf("INSERT 2ND VAL INDEX READ FAIL\n");
		return 1;
	}
	if (memcmp(data, (uint8_t [44]){
		2, 0, 0, 0, // Two values
		2, 0, // File 2 is last
		27, 0, 0, 0, // Size of file is 27
		key[0], key[1], key[2], key[3], key[4], key[5], key[6], 
		2, 0, // File index 2
		0, 0, 0, 0, // Position 0
		15, 0, 0, 0, // Data length is 15
		key2[0], key2[1], key2[2], key2[3], key2[4], key2[5], key2[6], 
		2, 0, // File index 2
		15, 0, 0, 0, // Position 15
		12, 0, 0, 0, // Data length is 12
	}, 44)) {
		printf("INSERT 2ND VAL INDEX DATA FAIL\n");
		return 1;
	}
	CBFileClose(file);
	file = CBFileOpen("./test_1.dat", false);
	if (NOT CBFileRead(file, data, 4)) {
		printf("INSERT 2ND VAL DELETION INDEX READ FAIL\n");
		return 1;
	}
	if (memcmp(data, (uint8_t [10]){0, 0, 0, 0}, 4)) {
		printf("INSERT 2ND VAL DELETION INDEX DATA FAIL\n");
		return 1;
	}
	CBFileClose(file);
	file = CBFileOpen("./test_log.dat", false);
	if (NOT CBFileRead(file, data, 60)) {
		printf("INSERT 2ND VAL LOG FILE READ FAIL\n");
		return 1;
	}
	if (memcmp(data, (uint8_t [60]){
		0, // Inactive
		27, 0, 0, 0, // Index previously size 27
		4, 0, 0, 0, // Deletion index previously size 4
		2, 0, // Previous last file 2
		15, 0, 0, 0, // Previous last file size 15
		2, 0, // Overwrite the data file
		0, 0, 0, 0, // Offset is zero
		15, 0, 0, 0, // Length of change is 15
		'H', 'i', ' ', 'T', 'h', 'e', 'r', 'e', ' ', 'M', 'a', 't', 'e', '!', '\0', // Previous data
		0, 0, // Overwrite the index
		0, 0, 0, 0, // Offset is zero
		10, 0, 0, 0, // Length of change is 10
		1, 0, 0, 0, 2, 0, 15, 0, 0, 0, // Previous data
	}, 60)) {
		printf("INSERT 2ND VAL LOG FILE DATA FAIL\n");
		return 1;
	}
	CBFileClose(file);
	// Remove first value
	CBDatabaseRemoveValue(storage, key);
	CBDatabaseCommit(storage);
	// Check data
	CBDatabaseReadValue(storage, key2, (uint8_t *)readStr, 12, 0);
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
	file = CBFileOpen("./test_0.dat", false);
	if (NOT CBFileRead(file, data, 44)) {
		printf("DELETE 1ST VAL INDEX READ FAIL\n");
		return 1;
	}
	if (memcmp(data, (uint8_t [44]){
		2, 0, 0, 0, // Two values
		2, 0, // File 2 is last
		27, 0, 0, 0, // Size of file is 27
		key[0], key[1], key[2], key[3], key[4], key[5], key[6], 
		2, 0, // File index 2
		0, 0, 0, 0, // Position 0
		0, 0, 0, 0, // Data length is 0 ie. deleted
		key2[0], key2[1], key2[2], key2[3], key2[4], key2[5], key2[6], 
		2, 0, // File index 2
		15, 0, 0, 0, // Position 15
		12, 0, 0, 0, // Data length is 12
	}, 44)) {
		printf("DELETE 1ST VAL INDEX DATA FAIL\n");
		return 1;
	}
	CBFileClose(file);
	file = CBFileOpen("./test_1.dat", false);
	if (NOT CBFileRead(file, data, 15)) {
		printf("DELETE 1ST VAL DELETION INDEX READ FAIL\n");
		return 1;
	}
	if (memcmp(data, (uint8_t [15]){
		1, 0, 0, 0, // One deletion entry.
		1, // Active deletion entry.
		0, 0, 0, 15, // Big endian length of deleted section.
		2, 0, // File-ID is 2
		0, 0, 0, 0, // Position is 0
	}, 15)) {
		printf("DELETE 1ST VAL DELETION INDEX DATA FAIL\n");
		return 1;
	}
	CBFileClose(file);
	file = CBFileOpen("./test_log.dat", false);
	if (NOT CBFileRead(file, data, 43)) {
		printf("DELETE 1ST VAL LOG FILE READ FAIL\n");
		return 1;
	}
	if (memcmp(data, (uint8_t [43]){
		0, // Inactive
		44, 0, 0, 0, // Index previously size 44
		4, 0, 0, 0, // Deletion index previously size 4
		2, 0, // Previous last file 2
		27, 0, 0, 0, // Previous last file size 27
		0, 0, // Overwrite the index
		23, 0, 0, 0, // Offset is 23
		4, 0, 0, 0, // Length of change is 4
		15, 0, 0, 0, // Previous data
		1, 0, // Overwrite the deletion index
		0, 0, 0, 0, // Offset is zero
		4, 0, 0, 0, // Length of change is 4
		0, 0, 0, 0, // Previous data
	}, 43)) {
		printf("DELETE 1ST VAL LOG FILE DATA FAIL\n");
		return 1;
	}
	CBFileClose(file);
	// Increase size of second key-value to 15 to replace deleted section
	CBDatabaseWriteValue(storage, key2, (uint8_t *)"Annoying code.", 15);
	CBDatabaseCommit(storage);   
	// Look at data
	CBDatabaseReadValue(storage, key2, (uint8_t *)readStr, 15, 0);
	if (memcmp(readStr, "Annoying code.", 15)) {
		printf("INCREASE 2ND VALUE FAIL\n");
		return 1;
	}
	if (storage->lastSize != 27) {
		printf("INCREASE 2ND VAL LAST SIZE FAIL\n");
		return 1;
	}
	file = CBFileOpen("./test_0.dat", false);
	if (NOT CBFileRead(file, data, 44)) {
		printf("INCREASE 2ND VAL INDEX READ FAIL\n");
		return 1;
	}
	if (memcmp(data, (uint8_t [44]){
		2, 0, 0, 0, // Two values
		2, 0, // File 2 is last
		27, 0, 0, 0, // Size of file is 27
		key[0], key[1], key[2], key[3], key[4], key[5], key[6], 
		2, 0, // File index 2
		0, 0, 0, 0, // Position 0
		0, 0, 0, 0, // Data length is 0 ie. deleted
		key2[0], key2[1], key2[2], key2[3], key2[4], key2[5], key2[6], 
		2, 0, // File index 2
		0, 0, 0, 0, // Position 0
		15, 0, 0, 0, // Data length is 15
	}, 44)) {
		printf("INCREASE 2ND VAL INDEX DATA FAIL\n");
		return 1;
	}
	CBFileClose(file);
	file = CBFileOpen("./test_1.dat", false);
	if (NOT CBFileRead(file, data, 26)) {
		printf("INCREASE 2ND VAL DELETION INDEX READ FAIL\n");
		return 1;
	}
	if (memcmp(data, (uint8_t [26]){
		2, 0, 0, 0, // Two deletion entries.
		0, // Inactive deletion entry.
		0, 0, 0, 15, // Big endian length of deleted section.
		2, 0, // File-ID is 2
		0, 0, 0, 0, // Position is 0
		1, // Active deletion entry
		0, 0, 0, 12, // Big endian length of deleted section.
		2, 0, // File-ID is 2
		15, 0, 0, 0, // Position is 15
	}, 26)) {
		printf("INCREASE 2ND VAL DELETION INDEX DATA FAIL\n");
		return 1;
	}
	CBFileClose(file);
	file = CBFileOpen("./test_log.dat", false);
	if (NOT CBFileRead(file, data, 89)) {
		printf("INCREASE 2ND VAL LOG FILE READ FAIL\n");
		return 1;
	}
	if (memcmp(data, (uint8_t [89]){
		0, // Inactive
		44, 0, 0, 0, // Index previously size 44
		15, 0, 0, 0, // Deletion index previously size 15
		2, 0, // Previous last file 2
		27, 0, 0, 0, // Previous last file size 27
		
		2, 0, // Overwrite the data file
		0, 0, 0, 0, // Offset is 0
		15, 0, 0, 0, // Length of change is 15
		'R', 'e', 'p', 'l', 'a', 'c', 'e', 'm', 'e', 'n', 't', '!', '!', '!', '\0', 
		
		1, 0, // Overwrite the deletion index
		4, 0, 0, 0, // Offset is 4
		5, 0, 0, 0, // Length of change is 5
		1, 0, 0, 0, 15, // Previous data
		
		0, 0, // Overwrite index
		34, 0, 0, 0, // Offset is 34
		10, 0, 0, 0, // Length of change is 10
		2, 0, 15, 0, 0, 0, 12, 0, 0, 0, // Previous data
		
		1, 0, // Overwrite the deletion index
		0, 0, 0, 0, // Offset is zero
		4, 0, 0, 0, // Length of change is 4
		1, 0, 0, 0, // Previous data
	}, 89)) {
		printf("INCREASE 2ND VAL LOG FILE DATA FAIL\n");
		return 1;
	}
	CBFileClose(file);
	// Add value and try database recovery.
	uint8_t key3[7];
	key3[0] = 6;
	for (int x = 1; x < 7; x++) {
		key3[x] = rand();
	}
	CBDatabaseWriteValue(storage, key3, (uint8_t *)"cbitcoin", 9);
	CBDatabaseCommit(storage);
	// Ensure value is OK
	CBDatabaseReadValue(storage, key3, (uint8_t *)readStr, 9, 0);
	if (memcmp(readStr, "cbitcoin", 9)) {
		printf("3RD VALUE FAIL\n");
		return 1;
	}
	// Free everything
	CBFreeDatabase(storage);
	// Activate log file
	file = CBFileOpen("./test_log.dat", false);
	data[0] = 1;
	CBFileOverwrite(file, data, 1);
	CBFileClose(file);
	// Load storage object
	storage = (CBDatabase *)CBNewDatabase("./", "test");
	// Verify recovery.
	CBDatabaseReadValue(storage, key2, (uint8_t *)readStr, 15, 0);
	if (memcmp(readStr, "Annoying code.", 15)) {
		printf("RECOVERY VALUE 2 FAIL\n");
		return 1;
	}
	if (storage->lastSize != 27) {
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
	file = CBFileOpen("./test_0.dat", false);
	if (NOT CBFileRead(file, data, 44)) {
		printf("RECOVERY INDEX READ FAIL\n");
		return 1;
	}
	if (memcmp(data, (uint8_t [44]){
		2, 0, 0, 0, // Two values
		2, 0, // File 2 is last
		27, 0, 0, 0, // Size of file is 27
		key[0], key[1], key[2], key[3], key[4], key[5], key[6], 
		2, 0, // File index 2
		0, 0, 0, 0, // Position 0
		0, 0, 0, 0, // Data length is 0 ie. deleted
		key2[0], key2[1], key2[2], key2[3], key2[4], key2[5], key2[6], 
		2, 0, // File index 2
		0, 0, 0, 0, // Position 0
		15, 0, 0, 0, // Data length is 15
	}, 44)) {
		printf("RECOVERY INDEX DATA FAIL\n");
		return 1;
	}
	CBFileClose(file);
	file = CBFileOpen("./test_1.dat", false);
	if (NOT CBFileRead(file, data, 26)) {
		printf("RECOVERY DELETION INDEX READ FAIL\n");
		return 1;
	}
	if (memcmp(data, (uint8_t [26]){
		2, 0, 0, 0, // Two deletion entries.
		0, // Inactive deletion entry.
		0, 0, 0, 15, // Big endian length of deleted section.
		2, 0, // File-ID is 2
		0, 0, 0, 0, // Position is 0
		1, // Active deletion entry
		0, 0, 0, 12, // Big endian length of deleted section.
		2, 0, // File-ID is 2
		15, 0, 0, 0, // Position is 15
	}, 26)) {
		printf("RECOVERY DELETION INDEX DATA FAIL\n");
		return 1;
	}
	CBFileClose(file);
	file = CBFileOpen("./test_2.dat", false);
	if (NOT CBFileRead(file, data, 27)) {
		printf("RECOVERY DATA FILE READ FAIL\n");
		return 1;
	}
	if (memcmp(data, "Annoying code.\0Another one\0", 27)) {
		printf("RECOVERY DATA FILE DATA FAIL\n");
		return 1;
	}
	CBFileClose(file);
	// Add value with smaller length than deleted section
	CBDatabaseWriteValue(storage, key, (uint8_t *)"Maniac", 7);
	CBDatabaseCommit(storage);
	// Look at data
	CBDatabaseReadValue(storage, key, (uint8_t *)readStr, 7, 0);
	if (memcmp(readStr, "Maniac", 7)) {
		printf("SMALLER VALUE FAIL\n");
		return 1;
	}
	if (storage->lastSize != 27) {
		printf("SMALLER LAST SIZE FAIL\n");
		return 1;
	}
	file = CBFileOpen("./test_0.dat", false);
	if (NOT CBFileRead(file, data, 44)) {
		printf("SMALLER INDEX READ FAIL\n");
		return 1;
	}
	if (memcmp(data, (uint8_t [44]){
		2, 0, 0, 0, // Two values
		2, 0, // File 2 is last
		27, 0, 0, 0, // Size of file is 27
		key[0], key[1], key[2], key[3], key[4], key[5], key[6], 
		2, 0, // File index 2
		20, 0, 0, 0, // Position 20
		7, 0, 0, 0, // Data length is 7
		key2[0], key2[1], key2[2], key2[3], key2[4], key2[5], key2[6], 
		2, 0, // File index 2
		0, 0, 0, 0, // Position 0
		15, 0, 0, 0, // Data length is 15
	}, 44)) {
		printf("SMALLER INDEX DATA FAIL\n");
		return 1;
	}
	CBFileClose(file);
	file = CBFileOpen("./test_1.dat", false);
	if (NOT CBFileRead(file, data, 26)) {
		printf("SMALLER DELETION INDEX READ FAIL\n");
		return 1;
	}
	if (memcmp(data, (uint8_t [26]){
		2, 0, 0, 0, // Two deletion entries.
		0, // Inactive deletion entry.
		0, 0, 0, 15, // Big endian length of deleted section.
		2, 0, // File-ID is 2
		0, 0, 0, 0, // Position is 0
		1, // Active deletion entry
		0, 0, 0, 5, // Big endian length of deleted section.
		2, 0, // File-ID is 2
		15, 0, 0, 0, // Position is 15
	}, 26)) {
		printf("SMALLER DELETION INDEX DATA FAIL\n");
		return 1;
	}
	CBFileClose(file);
	file = CBFileOpen("./test_log.dat", false);
	if (NOT CBFileRead(file, data, 67)) {
		printf("SMALLER LOG FILE READ FAIL\n");
		return 1;
	}
	if (memcmp(data, (uint8_t [67]){
		0, // Inactive
		44, 0, 0, 0, // Index previously size 42
		26, 0, 0, 0, // Deletion index previously size 26
		2, 0, // Previous last file 2
		27, 0, 0, 0, // Previous last file size 27
		
		2, 0, // Overwrite the data file
		20, 0, 0, 0, // Offset is 20
		7, 0, 0, 0, // Length of change is 7
		'e', 'r', ' ', 'o', 'n', 'e', '\0', // Previous data
		
		1, 0, // Overwrite the deletion index
		15, 0, 0, 0, // Offset is 4
		5, 0, 0, 0, // Length of change is 5
		1, 0, 0, 0, 12, // Previous data
		
		0, 0, // Overwrite the index
		17, 0, 0, 0, // Offset is 17
		10, 0, 0, 0, // Length of change is 10
		2, 0, 0, 0, 0, 0, 0, 0, 0, 0 // Previous data
	}, 67)) {
		printf("SMALLER LOG FILE DATA FAIL\n");
		return 1;
	}
	CBFileClose(file);
	// Change the first key
	uint8_t key4[7];
	key4[0] = 6;
	for (int x = 1; x < 7; x++) {
		key4[x] = rand();
	}
	CBDatabaseChangeKey(storage, key, key4);
	CBDatabaseCommit(storage);
	// Check data
	CBDatabaseReadValue(storage, key4, (uint8_t *)readStr, 7, 0);
	if (memcmp(readStr, "Maniac", 7)) {
		printf("CHANGE KEY VALUE FAIL\n");
		return 1;
	}
	if (storage->lastSize != 27) {
		printf("CHANGE KEY LAST SIZE FAIL\n");
		return 1;
	}
	file = CBFileOpen("./test_0.dat", false);
	if (NOT CBFileRead(file, data, 44)) {
		printf("CHANGE KEY INDEX READ FAIL\n");
		return 1;
	}
	if (memcmp(data, (uint8_t [44]){
		2, 0, 0, 0, // Two values
		2, 0, // File 2 is last
		27, 0, 0, 0, // Size of file is 27
		key4[0], key4[1], key4[2], key4[3], key4[4], key4[5], key4[6], 
		2, 0, // File index 2
		20, 0, 0, 0, // Position 20
		7, 0, 0, 0, // Data length is 7
		key2[0], key2[1], key2[2], key2[3], key2[4], key2[5], key2[6], 
		2, 0, // File index 2
		0, 0, 0, 0, // Position 0
		15, 0, 0, 0, // Data length is 15
	}, 44)) {
		printf("CHANGE KEY INDEX DATA FAIL\n");
		return 1;
	}
	CBFileClose(file);
	file = CBFileOpen("./test_1.dat", false);
	if (NOT CBFileRead(file, data, 26)) {
		printf("CHANGE KEY DELETION INDEX READ FAIL\n");
		return 1;
	}
	if (memcmp(data, (uint8_t [26]){
		2, 0, 0, 0, // Two deletion entries.
		0, // Inactive deletion entry.
		0, 0, 0, 15, // Big endian length of deleted section.
		2, 0, // File-ID is 2
		0, 0, 0, 0, // Position is 0
		1, // Active deletion entry
		0, 0, 0, 5, // Big endian length of deleted section.
		2, 0, // File-ID is 2
		15, 0, 0, 0, // Position is 15
	}, 26)) {
		printf("CHANGE KEY DELETION INDEX DATA FAIL\n");
		return 1;
	}
	CBFileClose(file);
	file = CBFileOpen("./test_log.dat", false);
	if (NOT CBFileRead(file, data, 31)) {
		printf("CHANGE KEY LOG FILE READ FAIL\n");
		return 1;
	}
	if (memcmp(data, (uint8_t [31]){
		0, // Inactive
		44, 0, 0, 0, // Index previously size 44
		26, 0, 0, 0, // Deletion index previously size 26
		2, 0, // Previous last file 2
		27, 0, 0, 0, // Previous last file size 47
		
		0, 0, // Overwrite the index
		11, 0, 0, 0, // Offset is 11
		6, 0, 0, 0, // Length of change is 6
		key[1], key[2], key[3], key[4], key[5], key[6], // Previous data
	}, 31)) {
		printf("CHANGE KEY LOG FILE DATA FAIL\n");
		return 1;
	}
	CBFileClose(file);
	// Finally try reading length of values
	if (CBDatabaseGetLength(storage, key4) != 7) {
		printf("READ 1ST VAL LENGTH FAIL\n");
		return 1;
	}
	if (CBDatabaseGetLength(storage, key2) != 15) {
		printf("READ 2ND VAL LENGTH FAIL\n");
		return 1;
	}
	CBFreeDatabase(storage);
	return 0;
}
