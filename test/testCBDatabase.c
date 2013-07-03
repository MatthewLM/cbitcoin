//
//  testCBDatabase.c
//  cbitcoin
//
//  Created by Matthew Mitchell on 11/11/2012.
//  Copyright (c) 2012 Matthew Mitchell
//  
//  This file is part of cbitcoin. It is subject to the license terms
//  in the LICENSE file found in the top-level directory of this
//  distribution and at http://www.cbitcoin.com/license.html. No part of
//  cbitcoin, including this file, may be copied, modified, propagated,
//  or distributed except according to the terms contained in the
//  LICENSE file.

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
	remove("./testDb/del.dat");
	remove("./testDb/log.dat");
	remove("./testDb/val_0.dat");
	remove("./testDb/idx_0_0.dat");
	remove("./testDb/idx_1_0.dat");
	remove("./testDb/idx_2_0.dat");
	rmdir("./testDb/");
	CBDatabase * storage = CBNewDatabase(".", "testDb", 10);
	if (NOT storage) {
		printf("NEW STORAGE FAIL\n");
		return 1;
	}
	if (memcmp(storage->dataDir, ".", 3)) {
		printf("OBJECT DATA DIR FAIL\n");
		return 1;
	}
	if (storage->lastFile != 0) {
		printf("OBJECT LAST FILE FAIL\n");
		return 1;
	}
	if (storage->lastSize != 16) {
		printf("OBJECT LAST FILE SIZE FAIL\n");
		return 1;
	}
	if (storage->numDeletionValues) {
		printf("OBJECT NUM DEL VALS FAIL\n");
		return 1;
	}
	uint16_t nodeSize = CB_DATABASE_BTREE_ELEMENTS*26 + 7;
	CBDatabaseIndex * index = CBLoadIndex(storage, 0, 10, nodeSize*2);
	if (index == NULL) {
		printf("LOAD INDEX INIT FAIL\n");
		return 1;
	}
	CBFreeDatabase(storage);
	CBFreeIndex(index);
	// Check index files are OK
	CBDepObject file;
	CBFileOpen(&file, "./testDb/idx_0_0.dat", false);
	uint8_t data[141];
	if (NOT CBFileRead(file, data, 12)) {
		printf("INDEX CREATE FAIL\n");
		return 1;
	}
	uint8_t initSizeData[2];
	CBInt16ToArray(initSizeData, 0, 19 + CB_DATABASE_BTREE_ELEMENTS*26);
	if (memcmp(data, (uint8_t [12]){
		0,0, // First file is zero
		initSizeData[0],initSizeData[1],0,0, // Size of file is 12 plus the node length
		0,0, // Root exists in file 0
		12,0,0,0, // Root has an offset of 12
	}, 12)) {
		printf("INDEX INITIAL DATA FAIL\n");
		return 1;
	}
	CBFileClose(file);
	CBFileOpen(&file, "./testDb/del.dat", false);
	if (NOT CBFileRead(file, data, 4)) {
		printf("DELETION INDEX CREATE FAIL\n");
		return 1;
	}
	if (memcmp(data, (uint8_t [4]){0,0,0,0}, 4)) {
		printf("DELETION INDEX INITIAL DATA FAIL\n");
		return 1;
	}
	CBFileClose(file);
	CBFileOpen(&file, "./testDb/val_0.dat", false);
	if (NOT CBFileRead(file, data, 6)) {
		printf("VALUE FILE CREATE FAIL\n");
		return 1;
	}
	if (memcmp(data, (uint8_t [6]){0,0,16,0,0,0}, 6)) {
		printf("VALUE FILE INITIAL DATA FAIL\n");
		return 1;
	}
	CBFileClose(file);
	// Test opening of initial files
	storage = (CBDatabase *)CBNewDatabase(".", "testDb", 10);
	if (storage->lastFile != 0) {
		printf("STORAGE NUM FILES FAIL\n");
		return 1;
	}
	if (storage->lastSize != 16) {
		printf("STORAGE LAST SIZE FAIL\n");
		return 1;
	}
	if (storage->numDeletionValues) {
		printf("DELETION NUM FAIL\n");
		return 1;
	}
	index = CBLoadIndex(storage, 0, 10, nodeSize*2);
	// Try inserting a value
	uint8_t key[10] = {255,9,8,7,6,5,4,3,2,1};
	CBDatabaseTransaction * tx = CBNewDatabaseTransaction(storage);
	CBDatabaseWriteValue(tx, index, key, (uint8_t *)"Hi There Mate!", 15);
	CBDatabaseCommit(tx);
	CBFreeDatabaseTransaction(tx);
	// Now check over data
	char readStr[18];
	CBDatabaseReadValue(NULL, index, key, (uint8_t *)readStr, 15, 0);
	if (memcmp(readStr, "Hi There Mate!", 15)) {
		printf("READ ALL VALUE FAIL\n");
		return 1;
	}
	if (storage->lastFile != 0) {
		printf("INSERT SINGLE VAL NUM FILES FAIL\n");
		return 1;
	}
	if (storage->lastSize != 31) {
		printf("INSERT SINGLE VAL LAST SIZE FAIL\n");
		return 1;
	}
	CBFileOpen(&file, "./testDb/idx_0_0.dat", false);
	if (NOT CBFileRead(file, data, 33)) {
		printf("INSERT SINGLE VAL INDEX READ FAIL\n");
		return 1;
	}
	if (memcmp(data, (uint8_t [33]){
		0,0, // Zero still last file
		initSizeData[0],initSizeData[1],0,0, // Size of file is 16 plus the node length
		0,0, // Root exists in file 0
		12,0,0,0, // Root has an offset of 12
		1, // One element in root.
		
		0,0, // First data in file 0
		15,0,0,0, // First data length 15
		16,0,0,0, // First data offset 16
		255,9,8,7,6,5,4,3,2,1, // First data key
	}, 33)) {
		printf("INSERT SINGLE VAL INDEX DATA FAIL\n");
		return 1;
	}
	CBFileClose(file);
	CBFileOpen(&file, "./testDb/del.dat", false);
	if (NOT CBFileRead(file, data, 4)) {
		printf("INSERT SINGLE VAL DELETION INDEX READ FAIL\n");
		return 1;
	}
	if (memcmp(data, (uint8_t [4]){0, 0, 0, 0}, 4)) {
		printf("INSERT SINGLE VAL DELETION INDEX DATA FAIL\n");
		return 1;
	}
	CBFileClose(file);
	CBFileOpen(&file, "./testDb/log.dat", false);
	if (NOT CBFileRead(file, data, 82)) {
		printf("INSERT SINGLE VAL LOG FILE READ FAIL\n");
		return 1;
	}
	if (memcmp(data, (uint8_t [82]){
		0, // Inactive
		4, 0, 0, 0, // Deletion index previously size 4
		0, 0, // Previous last file 0
		16, 0, 0, 0, // Previous last file size 16
		1, // One index
		0, // Index ID is 0
		0,0, // Last index file 0
		initSizeData[0],initSizeData[1],0,0, // Last index size is 12 plus the node length
		
		// Set element
		CB_DATABASE_FILE_TYPE_INDEX,0, // Overwrite index file ID 0
		0, 0, // Overwrite file ID index
		13, 0, 0, 0, // Offset is 13
		20, 0, 0, 0, // Length of change is 20
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // Previous data
		
		// Update number of elements
		CB_DATABASE_FILE_TYPE_INDEX,0,
		0,0,
		12,0,0,0,
		1,0,0,0,
		0,
		
		// Update last data file info
		CB_DATABASE_FILE_TYPE_DATA,0,
		0,0,
		0,0,0,0,
		6,0,0,0,
		0,0,16,0,0,0,
		
	}, 82)) {
		printf("INSERT SINGLE VAL LOG FILE DATA FAIL\n");
		return 1;
	}
	CBFileClose(file);
	// Try reding a section of the value
	CBDatabaseReadValue(NULL, index, key, (uint8_t *)readStr, 5, 3);
	if (memcmp(readStr, "There", 5)) {
		printf("READ PART VALUE FAIL\n");
		return 1;
	}
	// Try adding a new value and replacing the previous value together.
	uint8_t key2[10] = {254,9,8,7,6,5,4,3,2,1};
	tx = CBNewDatabaseTransaction(storage);
	CBDatabaseWriteValue(tx, index, key2, (uint8_t *)"Another one", 12);
	CBDatabaseWriteValue(tx, index, key, (uint8_t *)"Replacement!!!", 15);
	CBDatabaseCommit(tx);
	CBFreeDatabaseTransaction(tx);
	CBDatabaseReadValue(NULL, index, key, (uint8_t *)readStr, 15, 0);
	if (memcmp(readStr, "Replacement!!!", 15)) {
		printf("READ REPLACED VALUE FAIL\n");
		return 1;
	}
	CBDatabaseReadValue(NULL, index, key2, (uint8_t *)readStr, 12, 0);
	if (memcmp(readStr, "Another one", 12)) {
		printf("READ 2ND VALUE FAIL\n");
		return 1;
	}
	if (storage->lastSize != 43) {
		printf("INSERT 2ND VAL LAST SIZE FAIL\n");
		return 1;
	}
	CBFileOpen(&file, "./testDb/idx_0_0.dat", false);
	if (NOT CBFileRead(file, data, 53)) {
		printf("INSERT 2ND VAL INDEX READ FAIL\n");
		return 1;
	}
	if (memcmp(data, (uint8_t [53]){
		0,0, // Last file
		initSizeData[0],initSizeData[1],0,0,
		0,0, // Root file
		12,0,0,0, // Root offset
		2, // Num elements
		
		0,0, // Data file
		12,0,0,0, // Length
		31,0,0,0, // Offset
		254,9,8,7,6,5,4,3,2,1, // Key
		
		0,0, // Data file
		15,0,0,0, // Length
		16,0,0,0, // Offset
		255,9,8,7,6,5,4,3,2,1, // Key
	}, 53)) {
		printf("INSERT 2ND VAL INDEX DATA FAIL\n");
		return 1;
	}
	CBFileClose(file);
	CBFileOpen(&file, "./testDb/del.dat", false);
	if (NOT CBFileRead(file, data, 4)) {
		printf("INSERT 2ND VAL DELETION INDEX READ FAIL\n");
		return 1;
	}
	if (memcmp(data, (uint8_t [4]){0, 0, 0, 0}, 4)) {
		printf("INSERT 2ND VAL DELETION INDEX DATA FAIL\n");
		return 1;
	}
	CBFileClose(file);
	CBFileOpen(&file, "./testDb/log.dat", false);
	if (NOT CBFileRead(file, data, 141)) {
		printf("INSERT 2ND VAL LOG FILE READ FAIL\n");
		return 1;
	}
	if (memcmp(data, (uint8_t [141]){
		0, // Inactive
		4, 0, 0, 0, // Deletion index previous size
		0, 0, // Previous last file 0
		31, 0, 0, 0, // Previous last file size
		1, // One index
		0, // Index ID is 0
		0,0, // Last index file 0
		initSizeData[0],initSizeData[1],0,0, // Last index size
		
		// Move first key
		CB_DATABASE_FILE_TYPE_INDEX,0, // Overwrite index file ID 0
		0, 0, // Overwrite file ID index
		33, 0, 0, 0, // Offset
		20, 0, 0, 0, // Length of change is 20
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // Previous data
		
		// Set second key
		CB_DATABASE_FILE_TYPE_INDEX,0,
		0, 0, 
		13, 0, 0, 0,
		20, 0, 0, 0,
		0,0,15,0,0,0,16,0,0,0,255,9,8,7,6,5,4,3,2,1,
		
		// Update number of elements
		CB_DATABASE_FILE_TYPE_INDEX,0,
		0,0,
		12,0,0,0,
		1,0,0,0,
		1,
		
		// Overwrite first data
		CB_DATABASE_FILE_TYPE_DATA,0,
		0,0,
		16,0,0,0,
		15,0,0,0,
		'H','i',' ','T','h','e','r','e',' ','M','a','t','e','!',0,
		
		// Update last data file info
		CB_DATABASE_FILE_TYPE_DATA,0,
		0,0,
		0,0,0,0,
		6,0,0,0,
		0,0,31,0,0,0,
	}, 141)) {
		printf("INSERT 2ND VAL LOG FILE DATA FAIL\n");
		return 1;
	}
	CBFileClose(file);
	// Remove first value
	tx = CBNewDatabaseTransaction(storage);
	CBDatabaseRemoveValue(tx, index, key);
	CBDatabaseCommit(tx);
	CBFreeDatabaseTransaction(tx);
	// Check data
	CBDatabaseReadValue(NULL, index, key2, (uint8_t *)readStr, 12, 0);
	if (memcmp(readStr, "Another one", 12)) {
		printf("DELETE 1ST 2ND VALUE FAIL\n");
		return 1;
	}
	CBFileOpen(&file, "./testDb/idx_0_0.dat", false);
	if (NOT CBFileRead(file, data, 53)) {
		printf("DELETE 1ST VAL INDEX READ FAIL\n");
		return 1;
	}
	if (memcmp(data, (uint8_t [53]){
		0,0, // Last file
		initSizeData[0],initSizeData[1],0,0,
		0,0, // Root file
		12,0,0,0, // Root offset
		2, // Num elements
		
		0,0, // Data file
		12,0,0,0, // Length
		31,0,0,0, // Offset
		254,9,8,7,6,5,4,3,2,1, // Key
		
		0,0, // Data file
		255,255,255,255, // Length (Deleted)
		16,0,0,0, // Offset
		255,9,8,7,6,5,4,3,2,1, // Key
	}, 53)) {
		printf("DELETE 1ST VAL INDEX DATA FAIL\n");
		return 1;
	}
	CBFileClose(file);
	CBFileOpen(&file,"./testDb/del.dat", false);
	if (NOT CBFileRead(file, data, 15)) {
		printf("DELETE 1ST VAL DELETION INDEX READ FAIL\n");
		return 1;
	}
	if (memcmp(data, (uint8_t [15]){
		1, 0, 0, 0, // One deletion entry.
		1, // Active deletion entry.
		0, 0, 0, 15, // Big endian length of deleted section.
		0, 0, // File-ID is 0
		16, 0, 0, 0, // Position is 16
	}, 15)) {
		printf("DELETE 1ST VAL DELETION INDEX DATA FAIL\n");
		return 1;
	}
	CBFileClose(file);
	CBFileOpen(&file, "./testDb/log.dat", false);
	if (NOT CBFileRead(file, data, 44)) {
		printf("DELETE 1ST VAL LOG FILE READ FAIL\n");
		return 1;
	}
	if (memcmp(data, (uint8_t [44]){
		0, // Inactive
		4, 0, 0, 0, // Deletion index previous size
		0, 0, // Previous last file 0
		43, 0, 0, 0, // Previous last file size
		0, // No index
		
		// Overwrite length to signal deletion
		CB_DATABASE_FILE_TYPE_INDEX,0,
		0,0,
		35,0,0,0,
		4,0,0,0,
		15,0,0,0,
		
		// Update deletion entry number
		CB_DATABASE_FILE_TYPE_DELETION_INDEX,0,
		0,0,
		0,0,0,0,
		4,0,0,0,
		0,0,0,0,
	}, 44)) {
		printf("DELETE 1ST VAL LOG FILE DATA FAIL\n");
		return 1;
	}
	CBFileClose(file);
	// Increase size of second key-value to 15 to replace deleted section
	tx = CBNewDatabaseTransaction(storage);
	CBDatabaseWriteValue(tx, index, key2, (uint8_t *)"Annoying code.", 15);
	CBDatabaseCommit(tx);
	CBFreeDatabaseTransaction(tx);
	// Look at data
	CBDatabaseReadValue(NULL, index, key2, (uint8_t *)readStr, 15, 0);
	if (memcmp(readStr, "Annoying code.", 15)) {
		printf("INCREASE 2ND VALUE FAIL\n");
		return 1;
	}
	CBFileOpen(&file, "./testDb/idx_0_0.dat", false);
	if (NOT CBFileRead(file, data, 53)) {
		printf("INCREASE 2ND VAL INDEX READ FAIL\n");
		return 1;
	}
	if (memcmp(data, (uint8_t [53]){
		0,0, // Last file
		initSizeData[0],initSizeData[1],0,0,
		0,0, // Root file
		12,0,0,0, // Root offset
		2, // Num elements
		
		0,0, // Data file
		15,0,0,0, // Length
		16,0,0,0, // Offset
		254,9,8,7,6,5,4,3,2,1, // Key
		
		0,0, // Data file
		255,255,255,255, // Length (Deleted)
		16,0,0,0, // Offset
		255,9,8,7,6,5,4,3,2,1, // Key
	}, 53)) {
		printf("INCREASE 2ND VAL INDEX DATA FAIL\n");
		return 1;
	}
	CBFileClose(file);
	CBFileOpen(&file,"./testDb/del.dat", false);
	if (NOT CBFileRead(file, data, 26)) {
		printf("INCREASE 2ND VAL DELETION INDEX READ FAIL\n");
		return 1;
	}
	if (memcmp(data, (uint8_t [26]){
		2, 0, 0, 0, // Two deletion entries.
		0, // Inactive deletion entry.
		0, 0, 0, 15, // Big endian length of deleted section.
		0, 0, // File-ID is 0
		16, 0, 0, 0, // Position is 16
		1, // Active deletion entry
		0, 0, 0, 12, // Big endian length of deleted section.
		0, 0, // File-ID is 0
		31, 0, 0, 0, // Position is 31
	}, 26)) {
		printf("INCREASE 2ND VAL DELETION INDEX DATA FAIL\n");
		return 1;
	}
	CBFileClose(file);
	CBFileOpen(&file, "./testDb/log.dat", false);
	if (NOT CBFileRead(file, data, 101)) {
		printf("INCREASE 2ND VAL LOG FILE READ FAIL\n");
		return 1;
	}
	if (memcmp(data, (uint8_t [101]) {
		0, // Inactive
		15, 0, 0, 0, // Deletion index previous size
		0, 0, // Previous last file 0
		43, 0, 0, 0, // Previous last file size
		1, // One index
		0, // Index ID is 0
		0,0, // Last index file 0
		initSizeData[0],initSizeData[1],0,0, // Last index size
		
		// Overwrite deleted section with new data
		CB_DATABASE_FILE_TYPE_DATA,0,
		0,0,
		16,0,0,0,
		15,0,0,0,
		'R','e','p','l','a','c','e','m','e','n','t','!','!','!',0,
		
		// Update first deletion entry
		CB_DATABASE_FILE_TYPE_DELETION_INDEX,0,
		0,0,
		4,0,0,0,
		5,0,0,0,
		1,0,0,0,15,
		
		// Update index data for second key
		CB_DATABASE_FILE_TYPE_INDEX,0,
		0,0,
		13,0,0,0,
		10,0,0,0,
		0,0,12,0,0,0,31,0,0,0,
		
		// Update deletion entry number
		CB_DATABASE_FILE_TYPE_DELETION_INDEX,0,
		0,0,
		0,0,0,0,
		4,0,0,0,
		1,0,0,0,
	}, 101)) {
		printf("INCREASE 2ND VAL LOG FILE DATA FAIL\n");
		return 1;
	}
	CBFileClose(file);
	// Add value and try database recovery.
	uint8_t key3[10] = {253,9,8,7,6,5,4,3,2,1};
	tx = CBNewDatabaseTransaction(storage);
	CBDatabaseWriteValue(tx, index, key3, (uint8_t *)"cbitcoin", 9);
	CBDatabaseCommit(tx);
	CBFreeDatabaseTransaction(tx);
	// Ensure value is OK
	CBDatabaseReadValue(NULL, index, key3, (uint8_t *)readStr, 9, 0);
	if (memcmp(readStr, "cbitcoin", 9)) {
		printf("3RD VALUE FAIL\n");
		return 1;
	}
	// Free everything
	CBFreeDatabase(storage);
	CBFreeIndex(index);
	// Activate log file
	CBFileOpen(&file, "./testDb/log.dat", false);
	data[0] = 1;
	CBFileOverwrite(file, data, 1);
	CBFileClose(file);
	// Load database and index
	storage = CBNewDatabase(".", "testDb", 10);
	if (NOT storage) {
		printf("RECOVERY INIT DATABASE FAIL\n");
		return 1;
	}
	index = CBLoadIndex(storage, 0, 10, nodeSize*2);
	// Verify recovery.
	CBDatabaseReadValue(NULL, index, key2, (uint8_t *)readStr, 15, 0);
	if (memcmp(readStr, "Annoying code.", 15)) {
		printf("RECOVERY VALUE 2 FAIL\n");
		return 1;
	}
	if (storage->lastSize != 43) {
		printf("RECOVERY LAST SIZE FAIL\n");
		return 1;
	}
	if (storage->lastFile != 0) {
		printf("RECOVERY LAST FILE FAIL\n");
		return 1;
	}
	if (storage->numDeletionValues != 2) {
		printf("RECOVERY NUM DELETION VALUES FAIL\n");
		return 1;
	}
	// Check files
	CBFileOpen(&file, "./testDb/idx_0_0.dat", false);
	if (NOT CBFileRead(file, data, 53)) {
		printf("INCREASE 2ND VAL INDEX READ FAIL\n");
		return 1;
	}
	if (memcmp(data, (uint8_t [53]){
		0,0, // Last file
		initSizeData[0],initSizeData[1],0,0,
		0,0, // Root file
		12,0,0,0, // Root offset
		2, // Num elements
		
		0,0, // Data file
		15,0,0,0, // Length
		16,0,0,0, // Offset
		254,9,8,7,6,5,4,3,2,1, // Key
		
		0,0, // Data file
		255,255,255,255, // Length (Deleted)
		16,0,0,0, // Offset
		255,9,8,7,6,5,4,3,2,1, // Key
	}, 53)) {
		printf("INCREASE 2ND VAL INDEX DATA FAIL\n");
		return 1;
	}
	CBFileClose(file);
	CBFileOpen(&file,"./testDb/del.dat", false);
	if (NOT CBFileRead(file, data, 26)) {
		printf("INCREASE 2ND VAL DELETION INDEX READ FAIL\n");
		return 1;
	}
	if (memcmp(data, (uint8_t [26]){
		2, 0, 0, 0, // Two deletion entries.
		0, // Inactive deletion entry.
		0, 0, 0, 15, // Big endian length of deleted section.
		0, 0, // File-ID is 0
		16, 0, 0, 0, // Position is 16
		1, // Active deletion entry
		0, 0, 0, 12, // Big endian length of deleted section.
		0, 0, // File-ID is 0
		31, 0, 0, 0, // Position is 31
	}, 26)) {
		printf("INCREASE 2ND VAL DELETION INDEX DATA FAIL\n");
		return 1;
	}
	CBFileClose(file);
	CBFileOpen(&file, "./testDb/val_0.dat", false);
	if (NOT CBFileRead(file, data, 33)) {
		printf("RECOVERY DATA FILE READ FAIL\n");
		return 1;
	}
	if (memcmp(data, "\0\0\x2B\0\0\0\0\0\0\0\0\0\0\0\0\0Annoying code.\0Another one\0", 33)) {
		printf("RECOVERY DATA FILE DATA FAIL\n");
		return 1;
	}
	CBFileClose(file);
	// Add value with smaller length than deleted section
	tx = CBNewDatabaseTransaction(storage);
	CBDatabaseWriteValue(tx, index, key, (uint8_t *)"Maniac", 7);
	CBDatabaseCommit(tx);
	CBFreeDatabaseTransaction(tx);
	// Look at data
	CBDatabaseReadValue(NULL, index, key, (uint8_t *)readStr, 7, 0);
	if (memcmp(readStr, "Maniac", 7)) {
		printf("SMALLER VALUE FAIL\n");
		return 1;
	}
	if (storage->lastSize != 43) {
		printf("SMALLER LAST SIZE FAIL\n");
		return 1;
	}
	CBFileOpen(&file, "./testDb/idx_0_0.dat", false);
	if (NOT CBFileRead(file, data, 44)) {
		printf("SMALLER INDEX READ FAIL\n");
		return 1;
	}
	if (memcmp(data, (uint8_t [53]){
		0,0, // Last file
		initSizeData[0],initSizeData[1],0,0,
		0,0, // Root file
		12,0,0,0, // Root offset
		2, // Num elements
		
		0,0, // Data file
		15,0,0,0, // Length
		16,0,0,0, // Offset
		254,9,8,7,6,5,4,3,2,1, // Key
		
		0,0, // Data file
		7,0,0,0, // Length
		36,0,0,0, // Offsetx
		255,9,8,7,6,5,4,3,2,1, // Key
	}, 53)) {
		printf("SMALLER INDEX DATA FAIL\n");
		return 1;
	}
	CBFileClose(file);
	CBFileOpen(&file, "./testDb/del.dat", false);
	if (NOT CBFileRead(file, data, 26)) {
		printf("SMALLER DELETION INDEX READ FAIL\n");
		return 1;
	}
	if (memcmp(data, (uint8_t [26]){
		2, 0, 0, 0, // Two deletion entries.
		0, // Inactive deletion entry.
		0, 0, 0, 15, // Big endian length of deleted section.
		0, 0, // File-ID is 0
		16, 0, 0, 0, // Position is 16
		1, // Active deletion entry
		0, 0, 0, 5, // Big endian length of deleted section.
		0, 0, // File-ID is 0
		31, 0, 0, 0, // Position is 31
	}, 26)) {
		printf("SMALLER DELETION INDEX DATA FAIL\n");
		return 1;
	}
	CBFileClose(file);
	CBFileOpen(&file, "./testDb/log.dat", false);
	if (NOT CBFileRead(file, data, 77)) {
		printf("SMALLER DELETION LOG FILE READ FAIL\n");
		return 1;
	}
	if (memcmp(data, (uint8_t [77]) {
		0, // Inactive
		26, 0, 0, 0, // Deletion index previous size
		0, 0, // Previous last file 0
		43, 0, 0, 0, // Previous last file size
		1, // One index
		0, // Index ID is 0
		0,0, // Last index file 0
		initSizeData[0],initSizeData[1],0,0, // Last index size
		
		// Overwrite deleted section with new data
		CB_DATABASE_FILE_TYPE_DATA,0,
		0,0,
		36,0,0,0,
		7,0,0,0,
		'e','r',' ','o','n','e',0,
		
		// Update deletion entry
		CB_DATABASE_FILE_TYPE_DELETION_INDEX,0,
		0,0,
		15,0,0,0,
		5,0,0,0,
		1,0,0,0,12,
		
		// Update index data for key
		CB_DATABASE_FILE_TYPE_INDEX,0,
		0,0,
		33,0,0,0,
		10,0,0,0,
		0,0,255,255,255,255,16,0,0,0,
	}, 77)) {
		printf("SMALLER DELETION LOG FILE DATA FAIL\n");
		return 1;
	}
	CBFileClose(file);
	// Change the first key
	uint8_t key4[10] = {252,9,8,7,6,5,4,3,2,1};
	tx = CBNewDatabaseTransaction(storage);
	CBDatabaseChangeKey(tx, index, key, key4);
	CBDatabaseCommit(tx);
	CBFreeDatabaseTransaction(tx);
	// Check data
	memset(readStr, 0, 7);
	CBDatabaseReadValue(NULL, index, key4, (uint8_t *)readStr, 7, 0);
	if (memcmp(readStr, "Maniac", 7)) {
		printf("CHANGE KEY VALUE FAIL\n");
		return 1;
	}
	if (storage->lastSize != 43) {
		printf("CHANGE KEY LAST SIZE FAIL\n");
		return 1;
	}
	CBFileOpen(&file,"./testDb/idx_0_0.dat", false);
	if (NOT CBFileRead(file, data, 73)) {
		printf("CHANGE KEY INDEX READ FAIL\n");
		return 1;
	}
	if (memcmp(data, (uint8_t [73]){
		0,0, // Last file
		initSizeData[0],initSizeData[1],0,0,
		0,0, // Root file
		12,0,0,0, // Root offset
		3, // Num elements
		
		0,0, // Data file
		7,0,0,0, // Length
		36,0,0,0, // Offset
		252,9,8,7,6,5,4,3,2,1, // Key
		
		0,0, // Data file
		15,0,0,0, // Length
		16,0,0,0, // Offset
		254,9,8,7,6,5,4,3,2,1, // Key
		
		0,0, // Data file
		255,255,255,255, // Length (Deleted)
		36,0,0,0, // Offset
		255,9,8,7,6,5,4,3,2,1, // Key
	}, 73)) {
		printf("CHANGE KEY INDEX DATA FAIL\n");
		return 1;
	}
	CBFileClose(file);
	CBFileOpen(&file, "./testDb/del.dat", false);
	if (NOT CBFileRead(file, data, 26)) {
		printf("CHANGE KEY DELETION INDEX READ FAIL\n");
		return 1;
	}
	if (memcmp(data, (uint8_t [26]){
		2, 0, 0, 0, // Two deletion entries.
		0, // Inactive deletion entry.
		0, 0, 0, 15, // Big endian length of deleted section.
		0, 0, // File-ID is 0
		16, 0, 0, 0, // Position is 16
		1, // Active deletion entry
		0, 0, 0, 5, // Big endian length of deleted section.
		0, 0, // File-ID is 0
		31, 0, 0, 0, // Position is 31
	}, 26)) {
		printf("CHANGE KEY DELETION INDEX DATA FAIL\n");
		return 1;
	}
	CBFileClose(file);
	CBFileOpen(&file, "./testDb/log.dat", false);
	if (NOT CBFileRead(file, data, 132)) {
		printf("CHANGE KEY LOG FILE READ FAIL\n");
		return 1;
	}
	if (memcmp(data, (uint8_t [132]){
		0, // Inactive
		26, 0, 0, 0, // Deletion index previous size
		0, 0, // Previous last file 0
		43, 0, 0, 0, // Previous last file size
		1, // One index
		0, // Index ID is 0
		0,0, // Last index file 0
		initSizeData[0],initSizeData[1],0,0, // Last index size
		
		// Overwrite last key index entry to deleted
		CB_DATABASE_FILE_TYPE_INDEX,0,
		0,0,
		35,0,0,0,
		4,0,0,0,
		7,0,0,0,
		
		// Move index elements up
		CB_DATABASE_FILE_TYPE_INDEX,0,
		0,0,
		33,0,0,0,
		40,0,0,0,
		0,0,255,255,255,255,36,0,0,0,255,9,8,7,6,5,4,3,2,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		
		// Set new key entry
		CB_DATABASE_FILE_TYPE_INDEX,0,
		0,0,
		13,0,0,0,
		20,0,0,0,
		0,0,15,0,0,0,16,0,0,0,254,9,8,7,6,5,4,3,2,1,
		
		// Update number of elements
		CB_DATABASE_FILE_TYPE_INDEX,0,
		0,0,
		12,0,0,0,
		1,0,0,0,
		2,
	}, 132)) {
		printf("CHANGE KEY LOG FILE DATA FAIL\n");
		return 1;
	}
	CBFileClose(file);
	// Try reading length of values
	if (CBDatabaseGetLength(NULL, index, key4) != 7) {
		printf("READ 1ST VAL LENGTH FAIL\n");
		return 1;
	}
	if (CBDatabaseGetLength(NULL, index, key2) != 15) {
		printf("READ 2ND VAL LENGTH FAIL\n");
		return 1;
	}
	// Overwrite value with smaller value
	tx = CBNewDatabaseTransaction(storage);
	CBDatabaseWriteValue(tx, index, key4, (uint8_t *)"Hi!", 4);
	CBDatabaseCommit(tx);
	CBFreeDatabaseTransaction(tx);
	// Check data
	CBDatabaseReadValue(NULL, index, key4, (uint8_t *)readStr, 4, 0);
	if (memcmp(readStr, "Hi!", 4)) {
		printf("OVERWRITE WITH SMALLER VALUE FAIL\n");
		return 1;
	}
	if (storage->lastSize != 43) {
		printf("OVERWRITE WITH SMALLER LAST SIZE FAIL\n");
		return 1;
	}
	CBFileOpen(&file,"./testDb/idx_0_0.dat", false);
	if (NOT CBFileRead(file, data, 73)) {
		printf("OVERWRITE WITH SMALLER INDEX READ FAIL\n");
		return 1;
	}
	if (memcmp(data, (uint8_t [73]){
		0,0, // Last file
		initSizeData[0],initSizeData[1],0,0,
		0,0, // Root file
		12,0,0,0, // Root offset
		3, // Num elements
		
		0,0, // Data file
		4,0,0,0, // Length
		36,0,0,0, // Offset
		252,9,8,7,6,5,4,3,2,1, // Key
		
		0,0, // Data file
		15,0,0,0, // Length
		16,0,0,0, // Offset
		254,9,8,7,6,5,4,3,2,1, // Key
		
		0,0, // Data file
		255,255,255,255, // Length (Deleted)
		36,0,0,0, // Offset
		255,9,8,7,6,5,4,3,2,1, // Key
	}, 73)) {
		printf("OVERWRITE WITH SMALLER INDEX DATA FAIL\n");
		return 1;
	}
	CBFileClose(file);
	CBFileOpen(&file, "./testDb/del.dat", false);
	if (NOT CBFileRead(file, data, 26)) {
		printf("OVERWRITE WITH SMALLER DELETION INDEX READ FAIL\n");
		return 1;
	}
	if (memcmp(data, (uint8_t [26]){
		2, 0, 0, 0, // Two deletion entries.
		1, // Active deletion entry.
		0, 0, 0, 3, // Big endian length of deleted section.
		0, 0, // File-ID is 0
		40, 0, 0, 0, // Position is 40
		1, // Active deletion entry
		0, 0, 0, 5, // Big endian length of deleted section.
		0, 0, // File-ID is 0
		31, 0, 0, 0, // Position is 31
	}, 26)) {
		printf("OVERWRITE WITH SMALLER DELETION INDEX DATA FAIL\n");
		return 1;
	}
	CBFileClose(file);
	CBFileOpen(&file, "./testDb/log.dat", false);
	if (NOT CBFileRead(file, data, 74)) {
		printf("OVERWRITE WITH SMALLER LOG FILE READ FAIL\n");
		return 1;
	}
	if (memcmp(data, (uint8_t [74]){
		0, // Inactive
		26, 0, 0, 0, // Deletion index previous size
		0, 0, // Previous last file 0
		43, 0, 0, 0, // Previous last file size
		1, // One index
		0, // Index ID is 0
		0,0, // Last index file 0
		initSizeData[0],initSizeData[1],0,0, // Last index size
		
		// Overwrite value
		CB_DATABASE_FILE_TYPE_DATA,0,
		0,0,
		36,0,0,0,
		4,0,0,0,
		'M','a','n','i',
		
		// Overwrite deletion entry
		CB_DATABASE_FILE_TYPE_DELETION_INDEX,0,
		0,0,
		4,0,0,0,
		11,0,0,0,
		0,0,0,0,15,0,0,16,0,0,0,
		
		// Change index length data
		CB_DATABASE_FILE_TYPE_INDEX,0,
		0,0,
		15,0,0,0,
		4,0,0,0,
		7,0,0,0,
	}, 74)) {
		printf("OVERWRITE WITH SMALLER LOG FILE DATA FAIL\n");
		return 1;
	}
	CBFileClose(file);
	// Try changing key to existing key
	tx = CBNewDatabaseTransaction(storage);
	CBDatabaseChangeKey(tx, index, key2, key4);
	CBDatabaseCommit(tx);
	CBFreeDatabaseTransaction(tx);
	// Check data
	CBDatabaseReadValue(NULL, index, key4, (uint8_t *)readStr, 15, 0);
	if (memcmp(readStr, "Annoying code.", 15)) {
		printf("CHANGE KEY TO EXISTING KEY VALUE FAIL\n");
		return 1;
	}
	CBFileOpen(&file,"./testDb/idx_0_0.dat", false);
	if (NOT CBFileRead(file, data, 73)) {
		printf("CHANGE KEY TO EXISTING KEY INDEX READ FAIL\n");
		return 1;
	}
	if (memcmp(data, (uint8_t [73]){
		0,0, // Last file
		initSizeData[0],initSizeData[1],0,0,
		0,0, // Root file
		12,0,0,0, // Root offset
		3, // Num elements
		
		0,0, // Data file
		15,0,0,0, // Length
		16,0,0,0, // Offset
		252,9,8,7,6,5,4,3,2,1, // Key
		
		0,0, // Data file
		255,255,255,255, // Length (Deleted)
		16,0,0,0, // Offset
		254,9,8,7,6,5,4,3,2,1, // Key
		
		0,0, // Data file
		255,255,255,255, // Length (Deleted)
		36,0,0,0, // Offset
		255,9,8,7,6,5,4,3,2,1, // Key
	}, 73)) {
		printf("CHANGE KEY TO EXISTING KEY INDEX DATA FAIL\n");
		return 1;
	}
	CBFileClose(file);
	CBFileOpen(&file, "./testDb/del.dat", false);
	if (NOT CBFileRead(file, data, 37)) {
		printf("CHANGE KEY TO EXISTING KEY DELETION INDEX READ FAIL\n");
		return 1;
	}
	if (memcmp(data, (uint8_t [37]){
		3, 0, 0, 0, // Three deletion entries.
		1, // Active deletion entry.
		0, 0, 0, 3, // Big endian length of deleted section.
		0, 0, // File-ID is 0
		40, 0, 0, 0, // Position is 40
		1, // Active deletion entry
		0, 0, 0, 5, // Big endian length of deleted section.
		0, 0, // File-ID is 0
		31, 0, 0, 0, // Position is 31
		1, // Active deletion entry
		0, 0, 0, 4, // Big endian length of deleted section.
		0, 0, // File-ID is 0
		36, 0, 0, 0, // Position is 36
	}, 37)) {
		printf("CHANGE KEY TO EXISTING KEY DELETION INDEX DATA FAIL\n");
		return 1;
	}
	CBFileClose(file);
	CBFileOpen(&file, "./testDb/log.dat", false);
	if (NOT CBFileRead(file, data, 73)) {
		printf("CHANGE KEY TO EXISTING KEY LOG FILE READ FAIL\n");
		return 1;
	}
	if (memcmp(data, (uint8_t [73]){
		0, // Inactive
		26, 0, 0, 0, // Deletion index previous size
		0, 0, // Previous last file 0
		43, 0, 0, 0, // Previous last file size
		1, // One index
		0, // Index ID is 0
		0,0, // Last index file 0
		initSizeData[0],initSizeData[1],0,0, // Last index size
		
		// Delete old key entry
		CB_DATABASE_FILE_TYPE_INDEX,0,
		0,0,
		35,0,0,0,
		4,0,0,0,
		15,0,0,0,
		
		// Change new key entry to point to new data
		CB_DATABASE_FILE_TYPE_INDEX,0,
		0,0,
		13,0,0,0,
		10,0,0,0,
		0,0,4,0,0,0,36,0,0,0,
		
		// Update deletion index number
		CB_DATABASE_FILE_TYPE_DELETION_INDEX,0,
		0,0,
		0,0,0,0,
		4,0,0,0,
		2,0,0,0,
	}, 73)) {
		printf("CHANGE KEY TO EXISTING KEY LOG FILE DATA FAIL\n");
		return 1;
	}
	CBFileClose(file);
	// Test reading from transaction object
	tx = CBNewDatabaseTransaction(storage);
	CBDatabaseWriteValue(tx, index, key, (uint8_t *)"cbitcoin", 9);
	CBDatabaseReadValue(tx, index, key, (uint8_t *)readStr, 9, 0);
	if (memcmp(readStr, "cbitcoin", 9)) {
		printf("WRITE VALUE TO TX READ FAIL\n");
		return 1;
	}
	if (CBDatabaseGetLength(tx, index, key) != 9){
		printf("WRITE VALUE TO TX READ LEN FAIL\n");
		return 1;
	}
	CBDatabaseWriteValueSubSection(tx, index, key, (uint8_t *)"hello", 5, 2);
	CBDatabaseReadValue(tx, index, key, (uint8_t *)readStr, 9, 0);
	if (memcmp(readStr, "cbhellon", 9)) {
		printf("SUBSECTION TO TX READ FAIL\n");
		return 1;
	}
	if (CBDatabaseGetLength(tx, index, key) != 9){
		printf("SUBSECTION TO TX READ LEN FAIL\n");
		return 1;
	}
	CBDatabaseWriteValue(tx, index, key, (uint8_t *)"replacement value", 18);
	CBDatabaseReadValue(tx, index, key, (uint8_t *)readStr, 18, 0);
	if (memcmp(readStr, "replacement value", 18)) {
		printf("WRITE REPLACEMENT VALUE TO TX READ FAIL\n");
		return 1;
	}
	if (CBDatabaseGetLength(tx, index, key) != 18){
		printf("WRITE REPLACEMENT VALUE TO TX READ LEN FAIL\n");
		return 1;
	}
	// Test subsections to overwrite value write
	CBDatabaseWriteValueSubSection(tx, index, key, (uint8_t *)"first", 5, 2);
	CBDatabaseWriteValueSubSection(tx, index, key, (uint8_t *)"second", 6, 9);
	CBDatabaseReadValue(tx, index, key, (uint8_t *)readStr, 18, 0);
	if (memcmp(readStr, "refirstmesecondue", 18)) {
		printf("SUBSECTIONS TO TX READ FAIL\n");
		return 1;
	}
	if (CBDatabaseGetLength(tx, index, key) != 18){
		printf("SUBSECTIONS TO TX READ LEN FAIL\n");
		return 1;
	}
	CBDatabaseCommit(tx);
	CBFreeDatabaseTransaction(tx);
	CBDatabaseReadValue(NULL, index, key, (uint8_t *)readStr, 18, 0);
	if (memcmp(readStr, "refirstmesecondue", 18)) {
		printf("SUBSECTIONS TO DISK READ FAIL\n");
		return 1;
	}
	if (CBDatabaseGetLength(NULL, index, key) != 18){
		printf("SUBSECTIONS TO DISK READ LEN FAIL\n");
		return 1;
	}
	// Test subsections to disk
	tx = CBNewDatabaseTransaction(storage);
	CBDatabaseWriteValueSubSection(tx, index, key, (uint8_t *)"third", 5, 2);
	CBDatabaseWriteValueSubSection(tx, index, key, (uint8_t *)"forth", 5, 2);
	CBDatabaseWriteValueSubSection(tx, index, key, (uint8_t *)"fifth", 5, 9);
	CBDatabaseReadValue(tx, index, key, (uint8_t *)readStr, 18, 0);
	if (memcmp(readStr, "reforthmefifthdue", 18)) {
		printf("SUBSECTIONS TO TX NO OVERWRITE READ FAIL\n");
		return 1;
	}
	if (CBDatabaseGetLength(tx, index, key) != 18){
		printf("SUBSECTIONS TO TX NO OVERWRITE READ LEN FAIL\n");
		return 1;
	}
	// Test reading subsection
	CBDatabaseReadValue(tx, index, key, (uint8_t *)readStr, 9, 4);
	if (memcmp(readStr, "rthmefift", 9)) {
		printf("SUBSECTIONS TO TX NO OVERWRITE SUBSECTION READ FAIL\n");
		return 1;
	}
	CBDatabaseCommit(tx);
	CBFreeDatabaseTransaction(tx);
	CBDatabaseReadValue(NULL, index, key, (uint8_t *)readStr, 18, 0);
	if (memcmp(readStr, "reforthmefifthdue", 18)) {
		printf("SUBSECTIONS TO DISK NO OVERWRITE READ FAIL\n");
		return 1;
	}
	if (CBDatabaseGetLength(NULL, index, key) != 18){
		printf("SUBSECTIONS TO DISK NO OVERWRITE READ LEN FAIL\n");
		return 1;
	}
	// Test deletion with subsections
	tx = CBNewDatabaseTransaction(storage);
	CBDatabaseWriteValueSubSection(tx, index, key, (uint8_t *)"sixth", 5, 2);
	CBDatabaseWriteValueSubSection(tx, index, key, (uint8_t *)"seventh", 7, 7);
	CBDatabaseReadValue(tx, index, key, (uint8_t *)readStr, 18, 0);
	if (memcmp(readStr, "resixthseventhdue", 18)) {
		printf("SUBSECTIONS TO TX NO OVERWRITE SECOND READ FAIL\n");
		return 1;
	}
	if (CBDatabaseGetLength(tx, index, key) != 18){
		printf("SUBSECTIONS TO TX NO OVERWRITE SECOND READ LEN FAIL\n");
		return 1;
	}
	CBDatabaseRemoveValue(tx, index, key);
	CBDatabaseCommit(tx);
	CBFreeDatabaseTransaction(tx);
	if (CBDatabaseGetLength(NULL, index, key) != CB_DOESNT_EXIST) {
		printf("DELETE WITH SUBSECTIONS IN TX LEN FAIL\n");
		return 1;
	}
	// Test deleting value followed by writing value
	tx = CBNewDatabaseTransaction(storage);
	CBDatabaseRemoveValue(tx, index, key4);
	CBDatabaseWriteValue(tx, index, key4, (uint8_t *)"Mr. Bean", 9);
	CBDatabaseReadValue(tx, index, key4, (uint8_t *)readStr, 9, 0);
	if (memcmp(readStr, "Mr. Bean", 9)) {
		printf("TEST WRITE AFTER DELETE IN TX READ TX FAIL\n");
		return 1;
	}
	CBDatabaseCommit(tx);
	CBFreeDatabaseTransaction(tx);
	CBDatabaseReadValue(NULL, index, key4, (uint8_t *)readStr, 9, 0);
	if (memcmp(readStr, "Mr. Bean", 9)) {
		printf("TEST WRITE AFTER DELETE IN TX READ DISK FAIL\n");
		return 1;
	}
	// Test deleting subsection writes with overwrite.
	tx = CBNewDatabaseTransaction(storage);
	CBDatabaseWriteValueSubSection(tx, index, key4, (uint8_t *)"D", 1, 0);
	CBDatabaseWriteValueSubSection(tx, index, key4, (uint8_t *)"Fl", 2, 4);
	CBDatabaseWriteValue(tx, index, key4, (uint8_t *)"Monkey", 7);
	CBDatabaseReadValue(tx, index, key4, (uint8_t *)readStr, 7, 0);
	if (memcmp(readStr, "Monkey", 7)) {
		printf("TEST WRITE AFTER SUBSECTIONS IN TX READ TX FAIL\n");
		return 1;
	}
	CBDatabaseCommit(tx);
	CBFreeDatabaseTransaction(tx);
	CBDatabaseReadValue(NULL, index, key4, (uint8_t *)readStr, 9, 0);
	if (memcmp(readStr, "Monkey", 7)) {
		printf("TEST WRITE AFTER SUBSECTIONS IN TX READ DISK FAIL\n");
		return 1;
	}
	// Test extra data
	tx = CBNewDatabaseTransaction(storage);
	if (memcmp(tx->extraData, (uint8_t [10]){0}, 10)) {
		printf("INITIAL EXTRA DATA FAIL\n");
		return 1;
	}
	memcpy(tx->extraData, "Satoshi!!", 10);
	CBDatabaseCommit(tx);
	CBFreeDatabaseTransaction(tx);
	if (memcmp(storage->extraDataOnDisk, "Satoshi!!", 10)) {
		printf("COMMIT EXTRA DATA OBJECT FAIL\n");
		return 1;
	}
	CBFreeDatabase(storage);
	storage = CBNewDatabase(".", "testDb", 10);
	if (memcmp(storage->extraDataOnDisk, "Satoshi!!", 10)) {
		printf("COMMIT EXTRA DATA REOPEN FAIL\n");
		return 1;
	}
	tx = CBNewDatabaseTransaction(storage);
	if (memcmp(tx->extraData, "Satoshi!!", 10)) {
		printf("COMMIT EXTRA DATA TX FAIL\n");
		return 1;
	}
	tx->extraData[1] = 'o';
	tx->extraData[3] = 'a';
	CBDatabaseCommit(tx);
	CBFreeDatabaseTransaction(tx);
	if (memcmp(storage->extraDataOnDisk, "Sotashi!!", 10)) {
		printf("SUBSECTION COMMIT EXTRA DATA OBJECT FAIL\n");
		return 1;
	}
	CBFreeDatabase(storage);
	storage = CBNewDatabase(".", "testDb", 10);
	if (memcmp(storage->extraDataOnDisk, "Sotashi!!", 10)) {
		printf("SUBSECTION COMMIT EXTRA DATA REOPEN FAIL\n");
		return 1;
	}
	tx = CBNewDatabaseTransaction(storage);
	if (memcmp(tx->extraData, "Sotashi!!", 10)) {
		printf("SUBSECTION COMMIT EXTRA DATA TX FAIL\n");
		return 1;
	}
	// Create new index
	CBDatabaseIndex * index2 = CBLoadIndex(storage, 1, 2, 1000);
	tx = CBNewDatabaseTransaction(storage);
	for (uint16_t x = 0; x <= 500; x += 50){
		data[0] = x >> 8;
		data[1] = x;
		CBDatabaseWriteValue(tx, index2, data, data, 2);
	}
	CBDatabaseCommit(tx);
	CBFreeDatabaseTransaction(tx);
	tx = CBNewDatabaseTransaction(storage);
	for (uint16_t x = 0; x <= 500; x++){
		if (x % 50 == 0)
			continue;
		data[0] = x >> 8;
		data[1] = x;
		CBDatabaseWriteValue(tx, index2, data, data, 2);
	}
	// Add deletions
	for (uint16_t x = 0; x <= 500; x += 3){
		data[0] = x >> 8;
		data[1] = x;
		CBDatabaseRemoveValue(tx, index2, data);
	}
	// Test iterator
	CBDatabaseRangeIterator it = {(uint8_t []){0,0}, (uint8_t []){1,244}, tx, index2};
	if (CBDatabaseRangeIteratorFirst(&it) != CB_DATABASE_INDEX_FOUND) {
		printf("ITERATOR ALL FIRST FAIL\n");
		return 1;
	}
	for (uint16_t x = 0; x < 500; x++) {
		if (x % 3 == 0)
			continue;
		if (NOT CBDatabaseRangeIteratorRead(&it, data, 2, 0)){
			printf("ITERATOR ALL READ FAIL AT %i\n", x);
			return 1;
		}
		if ((data[1] | (uint16_t)data[0] << 8) != x) {
			printf("ITERATOR ALL DATA FAIL AT %i\n", x);
			return 1;
		}
		if (CBDatabaseRangeIteratorNext(&it) != ((x == 500) ? CB_DATABASE_INDEX_NOT_FOUND : CB_DATABASE_INDEX_FOUND)) {
			printf("ITERATOR ALL ITERATE FAIL AT %i\n", x);
			return 1;
		}
	}
	CBFreeDatabaseRangeInterator(&it);
	it = (CBDatabaseRangeIterator){(uint8_t []){0,50}, (uint8_t []){1,194}, tx, index2};
	if (CBDatabaseRangeIteratorFirst(&it) != CB_DATABASE_INDEX_FOUND) {
		printf("ITERATOR 50 TO 450 FIRST FAIL\n");
		return 1;
	}
	for (uint16_t x = 50; x < 450; x++) {
		if (x % 3 == 0)
			continue;
		if (NOT CBDatabaseRangeIteratorRead(&it, data, 2, 0)){
			printf("ITERATOR 50 TO 450 READ FAIL AT %i\n", x);
			return 1;
		}
		if ((data[1] | (uint16_t)data[0] << 8) != x) {
			printf("ITERATOR 50 TO 450 DATA FAIL AT %i\n", x);
			return 1;
		}
		if (CBDatabaseRangeIteratorNext(&it) != ((x == 449) ? CB_DATABASE_INDEX_NOT_FOUND : CB_DATABASE_INDEX_FOUND)) {
			printf("ITERATOR 50 TO 450 ITERATE FAIL AT %i\n", x);
			return 1;
		}
	}
	CBFreeDatabaseRangeInterator(&it);
	it = (CBDatabaseRangeIterator){(uint8_t []){1,245}, (uint8_t []){1,250}, tx, index2};
	if (CBDatabaseRangeIteratorFirst(&it) != CB_DATABASE_INDEX_NOT_FOUND) {
		printf("ITERATOR OUT OF RANGE FIRST FAIL\n");
		return 1;
	}
	CBFreeDatabaseRangeInterator(&it);
	CBFreeDatabaseTransaction(tx);
	CBFreeIndex(index2);
	// Create new index
	CBDatabaseIndex * index3 = CBLoadIndex(storage, 2, 10, (10 * CB_DATABASE_BTREE_ELEMENTS + sizeof(CBIndexNode))*10);
	// Generate 2000 key-value pairs
	uint8_t * keys = malloc(20000);
	uint8_t * values = malloc(2001000);
	for (uint16_t x = 0; x < 20000; x++) {
		keys[x] = rand();
	}
	for (uint32_t x = 0; x < 110000; x++) {
		values[x] = rand();
	}
	uint8_t dataRead[10];
	// Insert first 1000, one at a time.
	uint8_t * valptr = values;
	for (uint16_t x = 0; x < 1000; x++) {
		tx = CBNewDatabaseTransaction(storage);
		CBDatabaseWriteValue(tx, index3, keys + x*10, valptr, (x % 10) + 1);
		if (NOT CBDatabaseCommit(tx)){
			printf("INSERT FIRST 1000 FAIL COMMIT AT %u\n",x);
			return 1;
		}
		CBFreeDatabaseTransaction(tx);
		if ((x+1) % 100 == 0) {
			// Verify all of the data
			uint32_t offset = 0;
			for (uint16_t y = 0; y <= x; y++) {
				if (CBDatabaseReadValue(NULL, index3, keys + y*10, dataRead, (y % 10) + 1, 0) != CB_DATABASE_INDEX_FOUND) {
					printf("INSERT FIRST 1000 FAIL READ AT %u OF %u\n", y, x);
					return 1;
				}
				if (memcmp(dataRead, values + offset, (y % 10) + 1)) {
					printf("INSERT FIRST 1000 FAIL READ DATA AT %u OF %u\n", y, x);
					return 1;
				}
				offset += (y % 10) + 1;
			}
			printf("INSERTED AND CHECKED %i/1000 KEY_VALUES\n", x+1);
		}
		valptr += (x % 10) + 1;
	}
	// Delete first 500 keys
	for (uint16_t x = 0; x < 500; x++) {
		tx = CBNewDatabaseTransaction(storage);
		CBDatabaseRemoveValue(tx, index3, keys + x*10);
		if (NOT CBDatabaseCommit(tx)){
			printf("DELETE FIRST 500 FAIL COMMIT AT %u\n",x);
			return 1;
		}
		CBFreeDatabaseTransaction(tx);
		if ((x+1) % 100 == 0) {
			// Verify all of the data
			uint32_t offset = 0;
			for (uint16_t y = 0; y < 1000; y++) {
				if (CBDatabaseReadValue(NULL, index3, keys + y*10, dataRead, (y % 10) + 1, 0)
					!= (y > x ? CB_DATABASE_INDEX_FOUND : CB_DATABASE_INDEX_NOT_FOUND)) {
					printf("DELETE FIRST 500 FAIL READ AT %u OF %u\n", y, x);
					return 1;
				}
				if (y > x && memcmp(dataRead, values + offset, (y % 10) + 1)) {
					printf("DELETE FIRST 500 FAIL READ DATA AT %u OF %u\n", y, x);
					return 1;
				}
				offset += (y % 10) + 1;
			}
			printf("DELETED AND CHECKED %i/500 KEY_VALUES\n", x+1);
		}
	}
	// Add last 1000
	for (uint16_t x = 1000; x < 2000; x++) {
		tx = CBNewDatabaseTransaction(storage);
		CBDatabaseWriteValue(tx, index3, keys + x*10, valptr, (x % 10) + 1);
		if (NOT CBDatabaseCommit(tx)){
			printf("INSERT LAST 1000 FAIL COMMIT AT %u\n",x);
			return 1;
		}
		CBFreeDatabaseTransaction(tx);
		if ((x+1) % 100 == 0) {
			// Verify all of the data
			uint32_t offset = 0;
			for (uint16_t y = 0; y <= x; y++) {
				if (CBDatabaseReadValue(NULL, index3, keys + y*10, dataRead, (y % 10) + 1, 0)
					!= (y >= 500 ? CB_DATABASE_INDEX_FOUND : CB_DATABASE_INDEX_NOT_FOUND)) {
					printf("INSERT LAST 1000 FAIL READ AT %u OF %u\n", y, x);
					return 1;
				}
				if (y >= 500 && memcmp(dataRead, values + offset, (y % 10) + 1)) {
					printf("INSERT LAST 1000 FAIL READ DATA AT %u OF %u\n", y, x);
					return 1;
				}
				offset += (y % 10) + 1;
			}
			printf("INSERTED AND CHECKED LAST %i/1000 KEY_VALUES\n", x-999);
		}
		valptr += (x % 10) + 1;
	}
	// Close, reopen and recheck data
	CBFreeDatabase(storage);
	CBFreeIndex(index3);
	storage = CBNewDatabase(".", "testDb", 10);
	if (NOT storage) {
		printf("REOPEN 2000 NEW DATABASE FAIL\n");
		return 1;
	}
	index3 = CBLoadIndex(storage, 2, 10, (index->keySize * CB_DATABASE_BTREE_ELEMENTS + sizeof(CBIndexNode))*10);
	if (NOT index3) {
		printf("REOPEN 2000 LOAD INDEX FAIL\n");
		return 1;
	}
	uint32_t offset = 0;
	for (uint16_t y = 0; y < 2000; y++) {
		if (CBDatabaseReadValue(NULL, index3, keys + y*10, dataRead, (y % 10) + 1, 0)
			!= (y >= 500 ? CB_DATABASE_INDEX_FOUND : CB_DATABASE_INDEX_NOT_FOUND)) {
			printf("REOPEN 2000 KEY-VALUES FAIL READ AT %u\n", y);
			return 1;
		}
		if (y >= 500 && memcmp(dataRead, values + offset, (y % 10) + 1)) {
			printf("REOPEN 2000 KEY-VALUES FAIL READ DATA AT %u\n", y);
			return 1;
		}
		offset += (y % 10) + 1;
	}
	// Free objects
	CBFreeDatabase(storage);
	CBFreeIndex(index);
	CBFreeIndex(index3);
	return 0;
}
