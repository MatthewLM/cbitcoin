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

#include "CBDatabase.h"
#include <stdio.h>
#include <time.h>
#include <stdarg.h>
#include <sys/time.h>

uint64_t CBGetMilliseconds(void){
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec * 1000 + tv.tv_usec / 1000;
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
	remove("./testDb/idx_3_0.dat");
	remove("./testDb/idx_4_0.dat");
	rmdir("./testDb/");
	CBDatabase * storage = CBNewDatabase(".", "testDb", 10, 10000000, 10000000);
	if (! storage) {
		printf("NEW STORAGE FAIL\n");
		return 1;
	}
	if (strcmp(storage->dataDir, ".")) {
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
	if (! CBFileRead(file, data, 12)) {
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
	if (! CBFileRead(file, data, 4)) {
		printf("DELETION INDEX CREATE FAIL\n");
		return 1;
	}
	if (memcmp(data, (uint8_t [4]){0,0,0,0}, 4)) {
		printf("DELETION INDEX INITIAL DATA FAIL\n");
		return 1;
	}
	CBFileClose(file);
	CBFileOpen(&file, "./testDb/val_0.dat", false);
	if (! CBFileRead(file, data, 6)) {
		printf("VALUE FILE CREATE FAIL\n");
		return 1;
	}
	if (memcmp(data, (uint8_t [6]){0,0,16,0,0,0}, 6)) {
		printf("VALUE FILE INITIAL DATA FAIL\n");
		return 1;
	}
	CBFileClose(file);
	// Test opening of initial files
	storage = (CBDatabase *)CBNewDatabase(".", "testDb", 10, 10000000, 10000000);
	// Wait a second for the commit thread
	sleep(1);
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
	CBDatabaseWriteValue(index, key, (uint8_t *)"Hi There Mate!", 15);
	char readStr[18];
	// Check over current data
	CBDatabaseReadValue(index, key, (uint8_t *)readStr, 15, 0, false);
	if (memcmp(readStr, "Hi There Mate!", 15)) {
		printf("READ ALL VALUE CURRENT FAIL\n");
		return 1;
	}
	CBDatabaseStage(storage);
	// Check over staged data
	memset(readStr, 0, 15);
	CBDatabaseReadValue(index, key, (uint8_t *)readStr, 15, 0, false);
	if (memcmp(readStr, "Hi There Mate!", 15)) {
		printf("READ ALL VALUE STAGED FAIL\n");
		return 1;
	}
	CBDatabaseCommitProcess(storage);
	// Now check over commited data
	memset(readStr, 0, 15);
	CBDatabaseReadValue(index, key, (uint8_t *)readStr, 15, 0, false);
	if (memcmp(readStr, "Hi There Mate!", 15)) {
		printf("READ ALL VALUE DISK FAIL\n");
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
	if (! CBFileRead(file, data, 33)) {
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
	if (! CBFileRead(file, data, 4)) {
		printf("INSERT SINGLE VAL DELETION INDEX READ FAIL\n");
		return 1;
	}
	if (memcmp(data, (uint8_t [4]){0, 0, 0, 0}, 4)) {
		printf("INSERT SINGLE VAL DELETION INDEX DATA FAIL\n");
		return 1;
	}
	CBFileClose(file);
	CBFileOpen(&file, "./testDb/log.dat", false);
	if (! CBFileRead(file, data, 82)) {
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
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // Previous data
		CB_DATABASE_FILE_TYPE_INDEX,0, // Overwrite index file ID 0
		0, 0, // Overwrite file ID index
		13, 0, 0, 0, // Offset is 13
		20, 0, 0, 0, // Length of change is 20
		
		// Update number of elements
		0,
		CB_DATABASE_FILE_TYPE_INDEX,0,
		0,0,
		12,0,0,0,
		1,0,0,0,
		
		// Update last data file info
		0,0,16,0,0,0,
		CB_DATABASE_FILE_TYPE_DATA,0,
		0,0,
		0,0,0,0,
		6,0,0,0,
		
	}, 82)) {
		printf("INSERT SINGLE VAL LOG FILE DATA FAIL\n");
		return 1;
	}
	CBFileClose(file);
	// Try reding a section of the value
	CBDatabaseReadValue(index, key, (uint8_t *)readStr, 5, 3, false);
	if (memcmp(readStr, "There", 5)) {
		printf("READ PART VALUE FAIL\n");
		return 1;
	}
	// Try adding a new value and replacing the previous value together.
	uint8_t key2[10] = {254,9,8,7,6,5,4,3,2,1};
	CBDatabaseWriteValue(index, key2, (uint8_t *)"Another one", 12);
	CBDatabaseStage(storage);
	CBDatabaseWriteValue(index, key, (uint8_t *)"Replacement!!!", 15);
	CBDatabaseReadValue(index, key, (uint8_t *)readStr, 15, 0, false);
	if (memcmp(readStr, "Replacement!!!", 15)) {
		printf("READ REPLACED VALUE CURRENT FAIL\n");
		return 1;
	}
	CBDatabaseStage(storage);
	memset(readStr, 0, 15);
	CBDatabaseReadValue(index, key, (uint8_t *)readStr, 15, 0, false);
	if (memcmp(readStr, "Replacement!!!", 15)) {
		printf("READ REPLACED VALUE STAGED FAIL\n");
		return 1;
	}
	CBDatabaseCommitProcess(storage);
	memset(readStr, 0, 15);
	CBDatabaseReadValue(index, key, (uint8_t *)readStr, 15, 0, false);
	if (memcmp(readStr, "Replacement!!!", 15)) {
		printf("READ REPLACED VALUE DISK FAIL\n");
		return 1;
	}
	CBDatabaseReadValue(index, key2, (uint8_t *)readStr, 12, 0, false);
	if (memcmp(readStr, "Another one", 12)) {
		printf("READ 2ND VALUE FAIL\n");
		return 1;
	}
	if (storage->lastSize != 43) {
		printf("INSERT 2ND VAL LAST SIZE FAIL\n");
		return 1;
	}
	CBFileOpen(&file, "./testDb/idx_0_0.dat", false);
	if (! CBFileRead(file, data, 53)) {
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
	if (! CBFileRead(file, data, 4)) {
		printf("INSERT 2ND VAL DELETION INDEX READ FAIL\n");
		return 1;
	}
	if (memcmp(data, (uint8_t [4]){0, 0, 0, 0}, 4)) {
		printf("INSERT 2ND VAL DELETION INDEX DATA FAIL\n");
		return 1;
	}
	CBFileClose(file);
	CBFileOpen(&file, "./testDb/log.dat", false);
	if (! CBFileRead(file, data, 141)) {
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
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // Previous data
		CB_DATABASE_FILE_TYPE_INDEX,0, // Overwrite index file ID 0
		0, 0, // Overwrite file ID index
		33, 0, 0, 0, // Offset
		20, 0, 0, 0, // Length of change is 20
		
		// Set second key
		0,0,15,0,0,0,16,0,0,0,255,9,8,7,6,5,4,3,2,1,
		CB_DATABASE_FILE_TYPE_INDEX,0,
		0, 0, 
		13, 0, 0, 0,
		20, 0, 0, 0,
		
		// Update number of elements
		1,
		CB_DATABASE_FILE_TYPE_INDEX,0,
		0,0,
		12,0,0,0,
		1,0,0,0,
		
		// Overwrite first data
		'H','i',' ','T','h','e','r','e',' ','M','a','t','e','!',0,
		CB_DATABASE_FILE_TYPE_DATA,0,
		0,0,
		16,0,0,0,
		15,0,0,0,
		
		// Update last data file info
		0,0,31,0,0,0,
		CB_DATABASE_FILE_TYPE_DATA,0,
		0,0,
		0,0,0,0,
		6,0,0,0,
	}, 141)) {
		printf("INSERT 2ND VAL LOG FILE DATA FAIL\n");
		return 1;
	}
	CBFileClose(file);
	// Remove first value
	CBDatabaseRemoveValue(index, key, false);
	CBDatabaseStage(storage);
	CBDatabaseCommitProcess(storage);
	// Check data
	CBDatabaseReadValue(index, key2, (uint8_t *)readStr, 12, 0, false); // ??? Does this read from the cached index data?
	if (memcmp(readStr, "Another one", 12)) {
		printf("DELETE 1ST 2ND VALUE FAIL\n");
		return 1;
	}
	CBFileOpen(&file, "./testDb/idx_0_0.dat", false);
	if (! CBFileRead(file, data, 53)) {
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
	if (! CBFileRead(file, data, 15)) {
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
	if (! CBFileRead(file, data, 44)) {
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
		15,0,0,0,
		CB_DATABASE_FILE_TYPE_INDEX,0,
		0,0,
		35,0,0,0,
		4,0,0,0,
		
		// Update deletion entry number
		0,0,0,0,
		CB_DATABASE_FILE_TYPE_DELETION_INDEX,0,
		0,0,
		0,0,0,0,
		4,0,0,0,
	}, 44)) {
		printf("DELETE 1ST VAL LOG FILE DATA FAIL\n");
		return 1;
	}
	CBFileClose(file);
	// Increase size of second key-value to 15 to replace deleted section
	CBDatabaseWriteValue(index, key2, (uint8_t *)"Annoying code.", 15);
	CBDatabaseStage(storage);
	CBDatabaseCommitProcess(storage);
	// Look at data
	CBDatabaseReadValue(index, key2, (uint8_t *)readStr, 15, 0, false);
	if (memcmp(readStr, "Annoying code.", 15)) {
		printf("INCREASE 2ND VALUE FAIL\n");
		return 1;
	}
	CBFileOpen(&file, "./testDb/idx_0_0.dat", false);
	if (! CBFileRead(file, data, 53)) {
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
	if (! CBFileRead(file, data, 26)) {
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
	if (! CBFileRead(file, data, 101)) {
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
		'R','e','p','l','a','c','e','m','e','n','t','!','!','!',0,
		CB_DATABASE_FILE_TYPE_DATA,0,
		0,0,
		16,0,0,0,
		15,0,0,0,
		
		// Update first deletion entry
		1,0,0,0,15,
		CB_DATABASE_FILE_TYPE_DELETION_INDEX,0,
		0,0,
		4,0,0,0,
		5,0,0,0,
		
		// Update index data for second key
		0,0,12,0,0,0,31,0,0,0,
		CB_DATABASE_FILE_TYPE_INDEX,0,
		0,0,
		13,0,0,0,
		10,0,0,0,
		
		// Update deletion entry number
		1,0,0,0,
		CB_DATABASE_FILE_TYPE_DELETION_INDEX,0,
		0,0,
		0,0,0,0,
		4,0,0,0,
	}, 101)) {
		printf("INCREASE 2ND VAL LOG FILE DATA FAIL\n");
		return 1;
	}
	CBFileClose(file);
	// Add value, change value and try database recovery.
	uint8_t key3[10] = {253,9,8,7,6,5,4,3,2,1};
	CBDatabaseWriteValue(index, key3, (uint8_t *)"cbitcoin", 9);
	CBDatabaseWriteValue(index, key2, (uint8_t *)"changed", 8);
	CBDatabaseStage(storage);
	CBDatabaseCommitProcess(storage);
	// Ensure value is OK
	CBDatabaseReadValue(index, key3, (uint8_t *)readStr, 9, 0, false);
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
	storage = CBNewDatabase(".", "testDb", 10, 100000, 100000);
	if (! storage) {
		printf("RECOVERY INIT DATABASE FAIL\n");
		return 1;
	}
	index = CBLoadIndex(storage, 0, 10, nodeSize*2);
	// Verify recovery.
	CBDatabaseReadValue(index, key2, (uint8_t *)readStr, 15, 0, false);
	if (memcmp(readStr, "Annoying code.", 15)) {
		printf("RECOVERY VALUE 2 FAIL\n");
		return 1;
	}
	if (CBDatabaseReadValue(index, key3, (uint8_t *)readStr, 15, 0, false) != CB_DATABASE_INDEX_NOT_FOUND) {
		printf("RECOVERY VALUE 3 FAIL\n");
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
	// Check double CBDatabaseEnsureConsistent, to ensure that the CBDatabaseEnsureConsistent gives us a good logfile.
	CBFreeDatabase(storage);
	CBFreeIndex(index);
	storage = CBNewDatabase(".", "testDb", 10, 100000, 100000);
	if (! storage) {
		printf("RECOVERY INIT 2 DATABASE FAIL\n");
		return 1;
	}
	index = CBLoadIndex(storage, 0, 10, nodeSize*2);
	// Verify recovery.
	CBDatabaseReadValue(index, key2, (uint8_t *)readStr, 15, 0, false);
	if (memcmp(readStr, "Annoying code.", 15)) {
		printf("DOUBLE RECOVERY VALUE 2 FAIL\n");
		return 1;
	}
	if (CBDatabaseReadValue(index, key3, (uint8_t *)readStr, 15, 0, false) != CB_DATABASE_INDEX_NOT_FOUND) {
		printf("DOUBLE RECOVERY VALUE 3 FAIL\n");
		return 1;
	}
	if (storage->lastSize != 43) {
		printf("DOUBLE RECOVERY LAST SIZE FAIL\n");
		return 1;
	}
	if (storage->lastFile != 0) {
		printf("DOUBLE RECOVERY LAST FILE FAIL\n");
		return 1;
	}
	if (storage->numDeletionValues != 2) {
		printf("DOUBLE RECOVERY NUM DELETION VALUES FAIL\n");
		return 1;
	}
	// Check files
	CBFileOpen(&file, "./testDb/idx_0_0.dat", false);
	if (! CBFileRead(file, data, 53)) {
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
	if (! CBFileRead(file, data, 26)) {
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
	if (! CBFileRead(file, data, 33)) {
		printf("RECOVERY DATA FILE READ FAIL\n");
		return 1;
	}
	if (memcmp(data, "\0\0\x2B\0\0\0\0\0\0\0\0\0\0\0\0\0Annoying code.\0Another one\0", 33)) {
		printf("RECOVERY DATA FILE DATA FAIL\n");
		return 1;
	}
	CBFileClose(file);
	// Add value with smaller length than deleted section
	CBDatabaseWriteValue(index, key, (uint8_t *)"Maniac", 7);
	CBDatabaseStage(storage);
	CBDatabaseCommitProcess(storage);
	// Look at data
	CBDatabaseReadValue(index, key, (uint8_t *)readStr, 7, 0, false);
	if (memcmp(readStr, "Maniac", 7)) {
		printf("SMALLER VALUE FAIL\n");
		return 1;
	}
	if (storage->lastSize != 43) {
		printf("SMALLER LAST SIZE FAIL\n");
		return 1;
	}
	CBFileOpen(&file, "./testDb/idx_0_0.dat", false);
	if (! CBFileRead(file, data, 44)) {
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
	if (! CBFileRead(file, data, 26)) {
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
	if (! CBFileRead(file, data, 77)) {
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
		'e','r',' ','o','n','e',0,
		CB_DATABASE_FILE_TYPE_DATA,0,
		0,0,
		36,0,0,0,
		7,0,0,0,
		
		// Update deletion entry
		1,0,0,0,12,
		CB_DATABASE_FILE_TYPE_DELETION_INDEX,0,
		0,0,
		15,0,0,0,
		5,0,0,0,
		
		// Update index data for key
		0,0,255,255,255,255,16,0,0,0,
		CB_DATABASE_FILE_TYPE_INDEX,0,
		0,0,
		33,0,0,0,
		10,0,0,0,
	}, 77)) {
		printf("SMALLER DELETION LOG FILE DATA FAIL\n");
		return 1;
	}
	CBFileClose(file);
	// Change the first key
	uint8_t key4[10] = {252,9,8,7,6,5,4,3,2,1};
	CBDatabaseChangeKey(index, key, key4, false);
	// Check data in current
	memset(readStr, 0, 7);
	CBDatabaseReadValue(index, key4, (uint8_t *)readStr, 7, 0, false);
	if (memcmp(readStr, "Maniac", 7)) {
		printf("CHANGE KEY VALUE CURRENT FAIL\n");
		return 1;
	}
	CBDatabaseStage(storage);
	// Check data in staged
	memset(readStr, 0, 7);
	CBDatabaseReadValue(index, key4, (uint8_t *)readStr, 7, 0, false);
	if (memcmp(readStr, "Maniac", 7)) {
		printf("CHANGE KEY VALUE STAGED FAIL\n");
		return 1;
	}
	CBDatabaseCommitProcess(storage);
	// Check data in disk
	memset(readStr, 0, 7);
	CBDatabaseReadValue(index, key4, (uint8_t *)readStr, 7, 0, false);
	if (memcmp(readStr, "Maniac", 7)) {
		printf("CHANGE KEY VALUE IN DISK FAIL\n");
		return 1;
	}
	if (storage->lastSize != 43) {
		printf("CHANGE KEY LAST SIZE FAIL\n");
		return 1;
	}
	CBFileOpen(&file,"./testDb/idx_0_0.dat", false);
	if (! CBFileRead(file, data, 73)) {
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
	if (! CBFileRead(file, data, 26)) {
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
	if (! CBFileRead(file, data, 132)) {
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
		7,0,0,0,
		CB_DATABASE_FILE_TYPE_INDEX,0,
		0,0,
		35,0,0,0,
		4,0,0,0,
		
		// Move index elements up
		0,0,255,255,255,255,36,0,0,0,255,9,8,7,6,5,4,3,2,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		CB_DATABASE_FILE_TYPE_INDEX,0,
		0,0,
		33,0,0,0,
		40,0,0,0,
		
		// Set new key entry
		0,0,15,0,0,0,16,0,0,0,254,9,8,7,6,5,4,3,2,1,
		CB_DATABASE_FILE_TYPE_INDEX,0,
		0,0,
		13,0,0,0,
		20,0,0,0,
		
		// Update number of elements
		2,
		CB_DATABASE_FILE_TYPE_INDEX,0,
		0,0,
		12,0,0,0,
		1,0,0,0,
	}, 132)) {
		printf("CHANGE KEY LOG FILE DATA FAIL\n");
		return 1;
	}
	CBFileClose(file);
	// Try reading length of values
	uint32_t len;
	if (! CBDatabaseGetLength(index, key4, &len)) {
		printf("READ 1ST VAL LENGTH OBTAIN FAIL\n");
		return 1;
	}
	if (len != 7) {
		printf("READ 1ST VAL LENGTH FAIL\n");
		return 1;
	}
	if (! CBDatabaseGetLength(index, key2, &len)) {
		printf("READ 2ND VAL LENGTH OBTAIN FAIL\n");
		return 1;
	}
	if (len != 15) {
		printf("READ 2ND VAL LENGTH FAIL\n");
		return 1;
	}
	// Overwrite value with smaller value
	CBDatabaseWriteValue(index, key4, (uint8_t *)"Hi!", 4);
	// Check data in current
	CBDatabaseReadValue(index, key4, (uint8_t *)readStr, 4, 0, false);
	if (memcmp(readStr, "Hi!", 4)) {
		printf("OVERWRITE WITH SMALLER VALUE CURRENT FAIL\n");
		return 1;
	}
	CBDatabaseStage(storage);
	// Check data in staged
	memset(readStr, 0, 4);
	CBDatabaseReadValue(index, key4, (uint8_t *)readStr, 4, 0, false);
	if (memcmp(readStr, "Hi!", 4)) {
		printf("OVERWRITE WITH SMALLER VALUE STAGED FAIL\n");
		return 1;
	}
	CBDatabaseCommitProcess(storage);
	// Check data in disk
	memset(readStr, 0, 4);
	CBDatabaseReadValue(index, key4, (uint8_t *)readStr, 4, 0, false);
	if (memcmp(readStr, "Hi!", 4)) {
		printf("OVERWRITE WITH SMALLER VALUE DISK FAIL\n");
		return 1;
	}
	if (storage->lastSize != 43) {
		printf("OVERWRITE WITH SMALLER LAST SIZE FAIL\n");
		return 1;
	}
	CBFileOpen(&file,"./testDb/idx_0_0.dat", false);
	if (! CBFileRead(file, data, 73)) {
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
	if (! CBFileRead(file, data, 26)) {
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
	if (! CBFileRead(file, data, 74)) {
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
		'M','a','n','i',
		CB_DATABASE_FILE_TYPE_DATA,0,
		0,0,
		36,0,0,0,
		4,0,0,0,
		
		// Overwrite deletion entry
		0,0,0,0,15,0,0,16,0,0,0,
		CB_DATABASE_FILE_TYPE_DELETION_INDEX,0,
		0,0,
		4,0,0,0,
		11,0,0,0,
		
		// Change index length data
		7,0,0,0,
		CB_DATABASE_FILE_TYPE_INDEX,0,
		0,0,
		15,0,0,0,
		4,0,0,0,
	}, 74)) {
		printf("OVERWRITE WITH SMALLER LOG FILE DATA FAIL\n");
		// CHANGE BACK ??? return 1;
	}
	CBFileClose(file);
	// Try changing key to existing key
	CBDatabaseChangeKey(index, key2, key4, false);
	// Check data in current
	CBDatabaseReadValue(index, key4, (uint8_t *)readStr, 15, 0, false);
	if (memcmp(readStr, "Annoying code.", 15)) {
		printf("CHANGE KEY TO EXISTING KEY VALUE CURRENT FAIL\n");
		return 1;
	}
	CBDatabaseStage(storage);
	// Check data in staged
	memset(readStr, 0, 15);
	CBDatabaseReadValue(index, key4, (uint8_t *)readStr, 15, 0, false);
	if (memcmp(readStr, "Annoying code.", 15)) {
		printf("CHANGE KEY TO EXISTING KEY VALUE STAGED FAIL\n");
		return 1;
	}
	CBDatabaseCommitProcess(storage);
	// Check data in disk
	memset(readStr, 0, 15);
	CBDatabaseReadValue(index, key4, (uint8_t *)readStr, 15, 0, false);
	if (memcmp(readStr, "Annoying code.", 15)) {
		printf("CHANGE KEY TO EXISTING KEY VALUE DISK FAIL\n");
		return 1;
	}
	CBFileOpen(&file,"./testDb/idx_0_0.dat", false);
	if (! CBFileRead(file, data, 73)) {
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
		// CHANGE BACK ??? return 1;
	}
	CBFileClose(file);
	CBFileOpen(&file, "./testDb/del.dat", false);
	if (! CBFileRead(file, data, 37)) {
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
	if (! CBFileRead(file, data, 73)) {
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
		15,0,0,0,
		CB_DATABASE_FILE_TYPE_INDEX,0,
		0,0,
		35,0,0,0,
		4,0,0,0,
		
		// Change new key entry to point to new data
		0,0,4,0,0,0,36,0,0,0,
		CB_DATABASE_FILE_TYPE_INDEX,0,
		0,0,
		13,0,0,0,
		10,0,0,0,
		
		// Update deletion index number
		2,0,0,0,
		CB_DATABASE_FILE_TYPE_DELETION_INDEX,0,
		0,0,
		0,0,0,0,
		4,0,0,0,
	}, 73)) {
		printf("CHANGE KEY TO EXISTING KEY LOG FILE DATA FAIL\n");
		// CHANGE BACK ??? return 1;
	}
	CBFileClose(file);
	// Test reading from current and staged
	CBDatabaseWriteValue(index, key, (uint8_t *)"cbitcoin", 9);
	CBDatabaseReadValue(index, key, (uint8_t *)readStr, 9, 0, false);
	if (memcmp(readStr, "cbitcoin", 9)) {
		printf("WRITE VALUE TO CURRENT READ FAIL\n");
		return 1;
	}
	if (! CBDatabaseGetLength(index, key, &len)) {
		printf("WRITE VALUE TO CURRENT READ LEN OBTAIN FAIL\n");
		return 1;
	}
	if (len != 9){
		printf("WRITE VALUE TO CURRENT READ LEN FAIL\n");
		return 1;
	}
	CBDatabaseWriteValueSubSection(index, key, (uint8_t *)"hello", 5, 2);
	CBDatabaseReadValue(index, key, (uint8_t *)readStr, 9, 0, false);
	if (memcmp(readStr, "cbhellon", 9)) {
		printf("SUBSECTION TO CURRENT READ FAIL\n");
		return 1;
	}
	if (! CBDatabaseGetLength(index, key, &len)) {
		printf("SUBSECTION TO CURRENT READ LEN OBTAIN FAIL\n");
		return 1;
	}
	if (len != 9){
		printf("SUBSECTION TO CURRENT READ LEN FAIL\n");
		return 1;
	}
	CBDatabaseWriteValue(index, key, (uint8_t *)"replacement value", 18);
	CBDatabaseReadValue(index, key, (uint8_t *)readStr, 18, 0, false);
	if (memcmp(readStr, "replacement value", 18)) {
		printf("WRITE REPLACEMENT VALUE TO CURRENT READ FAIL\n");
		return 1;
	}
	if (! CBDatabaseGetLength(index, key, &len)) {
		printf("WRITE REPLACEMENT VALUE TO CURRENT READ OBTAIN LEN FAIL\n");
		return 1;
	}
	if (len != 18){
		printf("WRITE REPLACEMENT VALUE TO CURRENT READ LEN FAIL\n");
		return 1;
	}
	CBDatabaseStage(storage);
	// Test subsections to overwrite value write in staged
	CBDatabaseWriteValueSubSection(index, key, (uint8_t *)"first", 5, 2);
	CBDatabaseWriteValueSubSection(index, key, (uint8_t *)"second", 6, 9);
	CBDatabaseReadValue(index, key, (uint8_t *)readStr, 18, 0, false);
	if (memcmp(readStr, "refirstmesecondue", 18)) {
		printf("SUBSECTIONS OVER STAGED READ FAIL\n");
		return 1;
	}
	if (! CBDatabaseGetLength(index, key, &len)) {
		printf("SUBSECTIONS OVER STAGED READ LEN OBTAIN FAIL\n");
		return 1;
	}
	if (len != 18){
		printf("SUBSECTIONS OVER STAGED READ LEN FAIL\n");
		return 1;
	}
	CBDatabaseStage(storage);
	CBDatabaseCommit(storage);
	CBDatabaseReadValue(index, key, (uint8_t *)readStr, 18, 0, false);
	if (memcmp(readStr, "refirstmesecondue", 18)) {
		printf("SUBSECTIONS TO DISK READ FAIL\n");
		return 1;
	}
	if (! CBDatabaseGetLength(index, key, &len)) {
		printf("SUBSECTIONS TO DISK READ LEN OBTAIN FAIL\n");
		return 1;
	}
	if (len != 18){
		printf("SUBSECTIONS TO DISK READ LEN FAIL\n");
		return 1;
	}
	// Test overing subsections
	CBDatabaseWriteValueSubSection(index, key, (uint8_t *)"third", 5, 2);
	CBDatabaseStage(storage);
	CBDatabaseWriteValueSubSection(index, key, (uint8_t *)"forth", 5, 2);
	CBDatabaseWriteValueSubSection(index, key, (uint8_t *)"newwy", 5, 9);
	CBDatabaseWriteValueSubSection(index, key, (uint8_t *)"fifth", 5, 9);
	CBDatabaseReadValue(index, key, (uint8_t *)readStr, 18, 0, false);
	if (memcmp(readStr, "reforthmefifthdue", 18)) {
		printf("SUBSECTIONS TO MEM NO OVERWRITE READ FAIL\n");
		return 1;
	}
	if (! CBDatabaseGetLength(index, key, &len)) {
		printf("SUBSECTIONS TO MEM NO OVERWRITE READ LEN OBTAIN FAIL\n");
		return 1;
	}
	if (len != 18){
		printf("SUBSECTIONS TO MEM NO OVERWRITE READ LEN FAIL\n");
		return 1;
	}
	// Test reading subsection
	CBDatabaseReadValue(index, key, (uint8_t *)readStr, 9, 4, false);
	if (memcmp(readStr, "rthmefift", 9)) {
		printf("SUBSECTIONS TO TX NO OVERWRITE SUBSECTION READ FAIL\n");
		return 1;
	}
	CBDatabaseStage(storage);
	CBDatabaseReadValue(index, key, (uint8_t *)readStr, 18, 0, false);
	if (memcmp(readStr, "reforthmefifthdue", 18)) {
		printf("SUBSECTIONS TO STAGED NO OVERWRITE READ FAIL\n");
		return 1;
	}
	CBDatabaseGetLength(index, key, &len);
	if (len != 18){
		printf("SUBSECTIONS TO STAGED NO OVERWRITE READ LEN FAIL\n");
		return 1;
	}
	CBDatabaseCommit(storage);
	memset(readStr, 0, 18);
	CBDatabaseReadValue(index, key, (uint8_t *)readStr, 18, 0, false);
	if (memcmp(readStr, "reforthmefifthdue", 18)) {
		printf("SUBSECTIONS TO DISK NO OVERWRITE READ FAIL\n");
		return 1;
	}
	CBDatabaseGetLength(index, key, &len);
	if (len != 18){
		printf("SUBSECTIONS TO DISK NO OVERWRITE READ LEN FAIL\n");
		return 1;
	}
	// Test deletion with subsections
	CBDatabaseWriteValueSubSection(index, key, (uint8_t *)"sixth", 5, 2);
	CBDatabaseWriteValueSubSection(index, key, (uint8_t *)"seventh", 7, 7);
	CBDatabaseReadValue(index, key, (uint8_t *)readStr, 18, 0, false);
	if (memcmp(readStr, "resixthseventhdue", 18)) {
		printf("SUBSECTIONS TO CURRENT NO OVERWRITE SECOND READ FAIL\n");
		return 1;
	}
	CBDatabaseGetLength(index, key, &len);
	if (len != 18){
		printf("SUBSECTIONS TO CURRENT NO OVERWRITE SECOND READ LEN FAIL\n");
		return 1;
	}
	CBDatabaseStage(storage);
	CBDatabaseRemoveValue(index, key, false);
	CBDatabaseStage(storage);
	CBDatabaseCommit(storage);
	CBDatabaseGetLength(index, key, &len);
	if (len != CB_DOESNT_EXIST) {
		printf("DELETE WITH SUBSECTIONS IN DISK LEN FAIL\n");
		return 1;
	}
	// Test deleting value followed by writing value
	CBDatabaseRemoveValue(index, key4, false);
	CBDatabaseWriteValue(index, key4, (uint8_t *)"Mr. Bean", 9);
	CBDatabaseReadValue(index, key4, (uint8_t *)readStr, 9, 0, false);
	if (memcmp(readStr, "Mr. Bean", 9)) {
		printf("TEST WRITE AFTER DELETE IN CURRENT READ TX FAIL\n");
		return 1;
	}
	CBDatabaseStage(storage);
	memset(readStr, 0, 9);
	CBDatabaseReadValue(index, key4, (uint8_t *)readStr, 9, 0, false);
	if (memcmp(readStr, "Mr. Bean", 9)) {
		printf("TEST WRITE AFTER DELETE IN STAGED READ TX FAIL\n");
		return 1;
	}
	CBDatabaseCommit(storage);
	CBDatabaseReadValue(index, key4, (uint8_t *)readStr, 9, 0, false);
	if (memcmp(readStr, "Mr. Bean", 9)) {
		printf("TEST WRITE AFTER DELETE IN DISK READ TX FAIL\n");
		return 1;
	}
	// Test deleting subsection writes with overwrite.
	CBDatabaseWriteValueSubSection(index, key4, (uint8_t *)"D", 1, 0);
	CBDatabaseStage(storage);
	CBDatabaseWriteValueSubSection(index, key4, (uint8_t *)"Fl", 2, 4);
	CBDatabaseWriteValueSubSection(index, key4, (uint8_t *)"t", 1, 7);
	CBDatabaseWriteValue(index, key4, (uint8_t *)"Monkey", 7);
	CBDatabaseReadValue(index, key4, (uint8_t *)readStr, 7, 0, false);
	if (memcmp(readStr, "Monkey", 7)) {
		printf("TEST WRITE AFTER SUBSECTIONS IN CURRENT READ TX FAIL\n");
		return 1;
	}
	CBDatabaseStage(storage);
	memset(readStr, 0, 7);
	CBDatabaseReadValue(index, key4, (uint8_t *)readStr, 7, 0, false);
	if (memcmp(readStr, "Monkey", 7)) {
		printf("TEST WRITE AFTER SUBSECTIONS IN STAGED READ TX FAIL\n");
		return 1;
	}
	CBDatabaseCommit(storage);
	memset(readStr, 0, 7);
	CBDatabaseReadValue(index, key4, (uint8_t *)readStr, 7, 0, false);
	if (memcmp(readStr, "Monkey", 7)) {
		printf("TEST WRITE AFTER SUBSECTIONS IN DISK READ TX FAIL\n");
		return 1;
	}
	// Test changing key in transaction many times and writing to middle key
	CBDatabaseWriteValue(index, key4, (uint8_t *)"Change the key", 15);
	CBDatabaseChangeKey(index, key4, key3, false);
	CBDatabaseChangeKey(index, key3, key, false);
	if (CBDatabaseReadValue(index, key4, (uint8_t *)readStr, 15, 0, false) != CB_DATABASE_INDEX_NOT_FOUND) {
		printf("READ FROM OLD KEY IN CURRENT BEFORE FIRST STAGE FAIL\n");
		return 1;
	}
	CBDatabaseStage(storage);
	CBDatabaseChangeKey(index, key, key3, false);
	CBDatabaseChangeKey(index, key3, key2, false);
	CBDatabaseWriteValue(index, key3, (uint8_t *)"a value", 8);
	if (CBDatabaseReadValue(index, key4, (uint8_t *)readStr, 15, 0, false) != CB_DATABASE_INDEX_NOT_FOUND) {
		printf("READ FROM OLD KEY IN CURRENT FAIL\n");
		return 1;
	}
	CBDatabaseReadValue(index, key2, (uint8_t *)readStr, 15, 0, false);
	if (strcmp(readStr, "Change the key")) {
		printf("READ FROM NEW KEY IN CURRENT DATA FAIL\n");
		return 1;
	}
	if (! CBDatabaseGetLength(index, key2, &len)) {
		printf("READ LEN FROM NEW KEY IN CURRENT FAIL\n");
		return 1;
	}
	if (len != 15) {
		printf("READ LEN FROM NEW KEY IN CURRENT NUM FAIL\n");
		return 1;
	}
	CBDatabaseReadValue(index, key3, (uint8_t *)readStr, 8, 0, false);
	if (strcmp(readStr, "a value")) {
		printf("READ FROM NEW KEY IN CURRENT DATA FAIL\n");
		return 1;
	}
	// Test staged with change key
	CBDatabaseStage(storage);
	if (CBDatabaseReadValue(index, key4, (uint8_t *)readStr, 15, 0, false) != CB_DATABASE_INDEX_NOT_FOUND) {
		printf("READ FROM OLD KEY IN STAGED FAIL\n");
		return 1;
	}
	CBDatabaseReadValue(index, key2, (uint8_t *)readStr, 15, 0, false);
	if (strcmp(readStr, "Change the key")) {
		printf("READ FROM NEW KEY IN STAGED DATA FAIL\n");
		return 1;
	}
	CBDatabaseReadValue(index, key3, (uint8_t *)readStr, 8, 0, false);
	if (strcmp(readStr, "a value")) {
		printf("READ FROM NEW KEY IN STAGED DATA FAIL\n");
		return 1;
	}
	// Test commit with change key
	CBDatabaseCommit(storage);
	if (CBDatabaseReadValue(index, key4, (uint8_t *)readStr, 15, 0, false) != CB_DATABASE_INDEX_NOT_FOUND) {
		printf("READ FROM OLD KEY IN DISK FAIL\n");
		return 1;
	}
	CBDatabaseReadValue(index, key2, (uint8_t *)readStr, 15, 0, false);
	if (strcmp(readStr, "Change the key")) {
		printf("READ FROM NEW KEY IN DISK DATA FAIL\n");
		return 1;
	}
	CBDatabaseReadValue(index, key3, (uint8_t *)readStr, 8, 0, false);
	if (strcmp(readStr, "a value")) {
		printf("READ FROM NEW KEY IN DISK DATA FAIL\n");
		return 1;
	}
	// Test change key not in index.
	uint8_t key5[10] = {251,9,8,7,6,5,4,3,2,1};
	uint8_t key6[10] = {250,9,8,7,6,5,4,3,2,1};
	CBDatabaseWriteValue(index, key5, (uint8_t *)"ni!", 4);
	CBDatabaseChangeKey(index, key5, key6, false);
	if (CBDatabaseReadValue(index, key6, (uint8_t *)readStr, 4, 0, false) != CB_DATABASE_INDEX_FOUND) {
		printf("NOT IN INDEX READ FROM NEW KEY IN CURRENT FAIL\n");
		return 1;
	}
	if (strcmp(readStr, "ni!")) {
		printf("NOT IN INDEX READ FROM NEW KEY IN CURRENT DATA FAIL\n");
		return 1;
	}
	CBDatabaseStage(storage);
	memset(readStr, 0, 4);
	if (CBDatabaseReadValue(index, key6, (uint8_t *)readStr, 4, 0, false) != CB_DATABASE_INDEX_FOUND) {
		printf("NOT IN INDEX READ FROM NEW KEY IN STAGED FAIL\n");
		return 1;
	}
	if (strcmp(readStr, "ni!")) {
		printf("NOT IN INDEX READ FROM NEW KEY IN STAGED DATA FAIL\n");
		return 1;
	}
	CBDatabaseCommit(storage);
	memset(readStr, 0, 4);
	if (CBDatabaseReadValue(index, key6, (uint8_t *)readStr, 4, 0, false) != CB_DATABASE_INDEX_FOUND) {
		printf("NOT IN INDEX READ FROM NEW KEY IN DISK FAIL\n");
		return 1;
	}
	if (strcmp(readStr, "ni!")) {
		printf("NOT IN INDEX READ FROM NEW KEY IN DISK DATA FAIL\n");
		return 1;
	}
	// Test remove new key of change key afterwards
	CBDatabaseRemoveValue(index, key6, false);
	CBDatabaseWriteValue(index, key5, (uint8_t *)":-)", 4);
	CBDatabaseChangeKey(index, key5, key6, false);
	if (CBDatabaseReadValue(index, key6, (uint8_t *)readStr, 4, 0, false) != CB_DATABASE_INDEX_FOUND) {
		printf("REMOVE BEFOREHAND READ FROM NEW KEY IN CURRENT FAIL\n");
		return 1;
	}
	if (strcmp(readStr, ":-)")) {
		printf("REMOVE BEFOREHAND READ FROM NEW KEY IN CURRENT DATA FAIL\n");
		return 1;
	}
	CBDatabaseStage(storage);
	memset(readStr, 0, 4);
	if (CBDatabaseReadValue(index, key6, (uint8_t *)readStr, 4, 0, false) != CB_DATABASE_INDEX_FOUND) {
		printf("REMOVE BEFOREHAND READ FROM NEW KEY IN STAGED FAIL\n");
		return 1;
	}
	if (strcmp(readStr, ":-)")) {
		printf("REMOVE BEFOREHAND READ FROM NEW KEY IN STAGED DATA FAIL\n");
		return 1;
	}
	CBDatabaseCommit(storage);
	memset(readStr, 0, 4);
	if (CBDatabaseReadValue(index, key6, (uint8_t *)readStr, 4, 0, false) != CB_DATABASE_INDEX_FOUND) {
		printf("REMOVE BEFOREHAND READ FROM NEW KEY IN DISK FAIL\n");
		return 1;
	}
	if (strcmp(readStr, ":-)")) {
		printf("REMOVE BEFOREHAND READ FROM NEW KEY IN DISK DATA FAIL\n");
		return 1;
	}
	// Test removing staged change which has key not in disk.
	uint8_t key7[10] = {249,9,8,7,6,5,4,3,2,1};
	CBDatabaseWriteValue(index, key7, (uint8_t *)"elvenking", 10);
	CBDatabaseStage(storage);
	CBDatabaseRemoveValue(index, key7, false);
	if (CBDatabaseReadValue(index, key7, (uint8_t *)readStr, 10, 0, false) != CB_DATABASE_INDEX_NOT_FOUND) {
		printf("REMOVE STAGED READ CURRENT FAIL\n");
		return 1;
	}
	CBDatabaseStage(storage);
	if (CBDatabaseReadValue(index, key7, (uint8_t *)readStr, 10, 0, false) != CB_DATABASE_INDEX_NOT_FOUND) {
		printf("REMOVE STAGED READ STAGED FAIL\n");
		return 1;
	}
	CBDatabaseCommit(storage);
	if (CBDatabaseReadValue(index, key7, (uint8_t *)readStr, 10, 0, false) != CB_DATABASE_INDEX_NOT_FOUND) {
		printf("REMOVE STAGED READ DISK FAIL\n");
		return 1;
	}
	// Add write value to current over staged and commit value and then clear it
	CBDatabaseWriteValue(index, key7, (uint8_t *)"one!!one!!one!!", 16);
	CBDatabaseReadValue(index, key7, (uint8_t *)readStr, 16, 0, false);
	if (strcmp(readStr, "one!!one!!one!!")) {
		printf("WRITE IN CURRENT FAIL\n");
		return 1;
	}
	CBDatabaseStage(storage);
	CBDatabaseReadValue(index, key7, (uint8_t *)readStr, 16, 0, false);
	if (strcmp(readStr, "one!!one!!one!!")) {
		printf("WRITE IN STAGED FAIL\n");
		return 1;
	}
	CBDatabaseCommit(storage);
	CBDatabaseReadValue(index, key7, (uint8_t *)readStr, 16, 0, false);
	if (strcmp(readStr, "one!!one!!one!!")) {
		printf("WRITE IN DISK FAIL\n");
		return 1;
	}
	CBDatabaseWriteValueSubSection(index, key7, (uint8_t *)"two", 3, 5);
	CBDatabaseStage(storage);
	CBDatabaseReadValue(index, key7, (uint8_t *)readStr, 16, 0, false);
	if (strcmp(readStr, "one!!two!!one!!")) {
		printf("WRITE IN STAGED AND DISK FAIL\n");
		return 1;
	}
	CBDatabaseWriteValueSubSection(index, key7, (uint8_t *)"three", 5, 10);
	CBDatabaseReadValue(index, key7, (uint8_t *)readStr, 16, 0, false);
	if (strcmp(readStr, "one!!two!!three")) {
		printf("WRITE IN CURRENT STAGED AND DISK FAIL\n");
		return 1;
	}
	CBDatabaseClearCurrent(storage);
	CBDatabaseReadValue(index, key7, (uint8_t *)readStr, 16, 0, false);
	if (strcmp(readStr, "one!!two!!one!!")) {
		printf("CLEAR CURRENT WRITE FAIL\n");
		return 1;
	}
	// Add write value to current, delete value and then clear
	CBDatabaseWriteValueSubSection(index, key7, (uint8_t *)"four!", 5, 10);
	CBDatabaseRemoveValue(index, key7, false);
	CBDatabaseClearCurrent(storage);
	CBDatabaseReadValue(index, key7, (uint8_t *)readStr, 16, 0, false);
	if (strcmp(readStr, "one!!two!!one!!")) {
		printf("CLEAR CURRENT REMOVE CURRENT FAIL\n");
		return 1;
	}
	// Add delete of staged value and then clear.
	CBDatabaseWriteValueSubSection(index, key7, (uint8_t *)"four!", 5, 10);
	CBDatabaseStage(storage);
	CBDatabaseRemoveValue(index, key7, false);
	CBDatabaseClearCurrent(storage);
	CBDatabaseReadValue(index, key7, (uint8_t *)readStr, 16, 0, false);
	if (strcmp(readStr, "one!!two!!four!")) {
		printf("CLEAR CURRENT REMOVE STAGED FAIL\n");
		return 1;
	}
	// Add delete of commited value and then clear.
	CBDatabaseWriteValueSubSection(index, key7, (uint8_t *)"four!", 5, 10);
	CBDatabaseStage(storage);
	CBDatabaseCommit(storage);
	CBDatabaseRemoveValue(index, key7, false);
	CBDatabaseClearCurrent(storage);
	CBDatabaseReadValue(index, key7, (uint8_t *)readStr, 16, 0, false);
	if (strcmp(readStr, "one!!two!!four!")) {
		printf("CLEAR CURRENT REMOVE DISK FAIL\n");
		return 1;
	}
	// Test clearing change key
	CBDatabaseChangeKey(index, key7, key, false);
	CBDatabaseClearCurrent(storage);
	CBDatabaseReadValue(index, key7, (uint8_t *)readStr, 16, 0, false);
	if (strcmp(readStr, "one!!two!!four!")) {
		printf("CLEAR CURRENT REMOVE DISK FAIL\n");
		return 1;
	}
	// Clear with two indexes
	CBDatabaseIndex * index4 = CBLoadIndex(storage, 3, 10, 0);
	CBDatabaseWriteValue(index4, key7, (uint8_t *)"sabaton", 8);
	CBDatabaseStage(storage);
	CBDatabaseCommit(storage);
	CBDatabaseWriteValue(index, key7, (uint8_t *)"index1", 7);
	CBDatabaseChangeKey(index, key7, key, false);
	CBDatabaseWriteValue(index4, key7, (uint8_t *)"index4", 7);
	CBDatabaseChangeKey(index4, key7, key, false);
	CBDatabaseWriteValue(index4, key6, (uint8_t *)"index4-6", 9);
	CBDatabaseChangeKey(index4, key6, key2, false);
	CBDatabaseClearCurrent(storage);
	CBDatabaseReadValue(index4, key7, (uint8_t *)readStr, 8, 0, false);
	if (strcmp(readStr, "sabaton")) {
		printf("CLEAR CURRENT WITH TWO INDEXES READ FAIL\n");
		return 1;
	}
	// Delete staged without staging delete and test get length
	CBDatabaseRemoveValue(index4, key7, false);
	CBDatabaseGetLength(index4, key7, &len);
	if (len != CB_DOESNT_EXIST) {
		printf("NON STAGED DELETE OF STAGED READ LEN FAIL\n");
		return 1;
	}
	// Delete commited without staging delete and test get length
	CBDatabaseWriteValue(index4, key7, (uint8_t *)"commited", 9);
	CBDatabaseStage(storage);
	CBDatabaseCommit(storage);
	CBDatabaseRemoveValue(index4, key7, false);
	CBDatabaseGetLength(index4, key7, &len);
	if (len != CB_DOESNT_EXIST) {
		printf("NON STAGED DELETE OF COMMITED READ LEN FAIL\n");
		return 1;
	}
	CBDatabaseClearCurrent(storage);
	// Test extra data
	if (memcmp(storage->current.extraData, (uint8_t [10]){0}, 10)) {
		printf("INITIAL EXTRA DATA FAIL\n");
		return 1;
	}
	memcpy(storage->current.extraData, "Satoshi!!", 10);
	CBDatabaseStage(storage);
	if (memcmp(storage->staged.extraData, "Satoshi!!", 10)) {
		printf("COMMIT EXTRA DATA OBJECT STAGED FAIL\n");
		return 1;
	}
	CBMutexLock(storage->commitMutex);
	CBDatabaseCommitProcess(storage);
	CBMutexUnlock(storage->commitMutex);
	if (memcmp(storage->extraDataOnDisk, "Satoshi!!", 10)) {
		printf("COMMIT EXTRA DATA OBJECT ON DISK FAIL\n");
		return 1;
	}
	CBFreeDatabase(storage);
	CBFreeIndex(index4);
	storage = CBNewDatabase(".", "testDb", 10, 100000, 100000);
	if (memcmp(storage->extraDataOnDisk, "Satoshi!!", 10)) {
		printf("COMMIT EXTRA DATA REOPEN FAIL\n");
		return 1;
	}
	if (memcmp(storage->current.extraData, "Satoshi!!", 10)) {
		printf("COMMIT EXTRA DATA TX FAIL\n");
		return 1;
	}
	storage->current.extraData[1] = 'o';
	storage->current.extraData[3] = 'a';
	CBDatabaseStage(storage);
	CBDatabaseCommitProcess(storage);
	if (memcmp(storage->extraDataOnDisk, "Sotashi!!", 10)) {
		printf("SUBSECTION COMMIT EXTRA DATA OBJECT FAIL\n");
		return 1;
	}
	CBFreeIndex(index);
	CBFreeDatabase(storage);
	storage = CBNewDatabase(".", "testDb", 10, 100000, 100000);
	index = CBLoadIndex(storage, 0, 10, 10000);
	if (memcmp(storage->extraDataOnDisk, "Sotashi!!", 10)) {
		printf("SUBSECTION COMMIT EXTRA DATA REOPEN FAIL\n");
		return 1;
	}
	if (memcmp(storage->current.extraData, "Sotashi!!", 10)) {
		printf("SUBSECTION COMMIT EXTRA DATA TX FAIL\n");
		return 1;
	}
	// Create new index
	CBDatabaseIndex * index2 = CBLoadIndex(storage, 1, 3, 10000);
	for (uint16_t x = 0; x <= 500; x++) {
		data[0] = x >= 200 && x <= 300;
		data[1] = x >> 8;
		data[2] = x;
		CBDatabaseWriteValue(index2, data, data, 3);
		if (x == 250) {
			CBDatabaseStage(storage);
			CBDatabaseCommit(storage);
		}
	}
	// Add staged change keys
	for (uint16_t x = 250; x <= 300; x++) {
		data[0] = 1;
		data[1] = x >> 8;
		data[2] = x;
		data[3] = 0;
		data[4] = x >> 8;
		data[5] = x;
		CBDatabaseChangeKey(index2, data, data + 3, false);
	}
	// Add other staged change keys, to be changed again
	for (uint16_t x = 350; x <= 400; x++) {
		data[0] = 1;
		data[1] = x >> 8;
		data[2] = x;
		data[3] = 2;
		data[4] = x >> 8;
		data[5] = x;
		CBDatabaseChangeKey(index2, data, data + 3, false);
	}
	CBDatabaseStage(storage);
	// Add current writes
	for (uint16_t x = 0; x <= 500; x++){
		if (x % 2 == 0)
			continue;
		data[0] = x >= 200 && x < 250;
		data[1] = x >> 8;
		data[2] = x;
		CBDatabaseWriteValue(index2, data, data, 3);
	}
	// Add change keys
	for (uint16_t x = 200; x < 250; x++){
		data[0] = 1;
		data[1] = x >> 8;
		data[2] = x;
		data[3] = 0;
		data[4] = x >> 8;
		data[5] = x;
		CBDatabaseChangeKey(index2, data, data + 3, false);
	}
	// Add second change keys
	for (uint16_t x = 350; x <= 400; x++) {
		data[0] = 2;
		data[1] = x >> 8;
		data[2] = x;
		data[3] = 0;
		data[4] = x >> 8;
		data[5] = x;
		CBDatabaseChangeKey(index2, data, data + 3, false);
	}
	// Add deletions
	for (uint16_t x = 0; x <= 500; x += 3){
		data[0] = 0;
		data[1] = x >> 8;
		data[2] = x;
		CBDatabaseRemoveValue(index2, data, false);
	}
	// Test iterator
	CBDatabaseRangeIterator it;
	CBInitDatabaseRangeIterator(&it, (uint8_t []){0,0,0}, (uint8_t []){2,1,244}, index2);
	if (CBDatabaseRangeIteratorFirst(&it) != CB_DATABASE_INDEX_FOUND) {
		printf("ITERATOR ALL FIRST FAIL\n");
		return 1;
	}
	CBDatabaseIndex * index5 = CBLoadIndex(storage, 4, 1, 0);
	for (uint16_t x = 0; x <= 500; x++) {
		if (x % 3 == 0)
			continue;
		// Add other value to another index to insure non-interference with this index
		CBDatabaseWriteValue(index5, (uint8_t []){x}, (uint8_t []){x}, 1);
		if (! CBDatabaseRangeIteratorRead(&it, data, 3, 0)){
			printf("ITERATOR ALL READ FAIL AT %i\n", x);
			return 1;
		}
		if ((data[2] | (uint16_t)data[1] << 8) != x) {
			printf("ITERATOR ALL DATA FAIL AT %i\n", x);
			return 1;
		}
		if (CBDatabaseRangeIteratorNext(&it) != ((x == 500) ? CB_DATABASE_INDEX_NOT_FOUND : CB_DATABASE_INDEX_FOUND)) {
			printf("ITERATOR ALL ITERATE FAIL AT %i\n", x);
			return 1;
		}
	}
	CBFreeDatabaseRangeIterator(&it);
	CBInitDatabaseRangeIterator(&it, (uint8_t []){0,0,50}, (uint8_t []){0,1,194}, index2);
	if (CBDatabaseRangeIteratorFirst(&it) != CB_DATABASE_INDEX_FOUND) {
		printf("ITERATOR 50 TO 450 FIRST FAIL\n");
		return 1;
	}
	for (uint16_t x = 50; x < 450; x++) {
		if (x % 3 == 0)
			continue;
		if (! CBDatabaseRangeIteratorRead(&it, data, 3, 0)){
			printf("ITERATOR 50 TO 450 READ FAIL AT %i\n", x);
			return 1;
		}
		if ((data[2] | (uint16_t)data[1] << 8) != x) {
			printf("ITERATOR 50 TO 450 DATA FAIL AT %i\n", x);
			return 1;
		}
		if (CBDatabaseRangeIteratorNext(&it) != ((x == 449) ? CB_DATABASE_INDEX_NOT_FOUND : CB_DATABASE_INDEX_FOUND)) {
			printf("ITERATOR 50 TO 450 ITERATE FAIL AT %i\n", x);
			return 1;
		}
	}
	CBFreeDatabaseRangeIterator(&it);
	// Try getting last in iterator
	CBInitDatabaseRangeIterator(&it, (uint8_t []){0,0,0}, (uint8_t []){0,0,0}, index2);
	for (uint16_t x = 50; x < 450; x++) {
		((uint8_t *)it.maxElement)[1] = x >> 8;
		((uint8_t *)it.maxElement)[2] = x;
		((uint8_t *)it.minElement)[1] = (x-x%2) >> 8;
		((uint8_t *)it.minElement)[2] = x-x%2;
		bool deletedOnly = x % 2 == 0 && x % 3 == 0;
		if (CBDatabaseRangeIteratorLast(&it) != (deletedOnly ? CB_DATABASE_INDEX_NOT_FOUND : CB_DATABASE_INDEX_FOUND)) {
			printf("ITERATOR LAST FAIL %u\n",x);
			return 1;
		}
		if (! deletedOnly) {
			if (! CBDatabaseRangeIteratorRead(&it, data, 3, 0)){
				printf("ITERATOR LAST READ FAIL AT %u\n", x);
				return 1;
			}
			if ((data[2] | (uint16_t)data[1] << 8) != ((x % 3 == 0) ? (x-1) : x)) {
				printf("ITERATOR LAST READ DATA FAIL AT %u\n", x);
				return 1;
			}
		}
	}
	CBFreeDatabaseRangeIterator(&it);
	CBInitDatabaseRangeIterator(&it, (uint8_t []){0,1,245}, (uint8_t []){0,1,250}, index2);
	if (CBDatabaseRangeIteratorFirst(&it) != CB_DATABASE_INDEX_NOT_FOUND) {
		printf("ITERATOR OUT OF RANGE FIRST FAIL\n");
		return 1;
	}
	CBFreeDatabaseRangeIterator(&it);
	// Test two indexes at once.
	CBDatabaseWriteValue(index, (uint8_t []){251,9,8,7,6,5,4,3,2,1}, (uint8_t *)"one1", 5);
	CBDatabaseWriteValue(index2, (uint8_t []){0xFF,0xFF,0xFF}, (uint8_t *)"two2", 5);
	// Check current
	if (CBDatabaseReadValue(index, (uint8_t []){251,9,8,7,6,5,4,3,2,1}, (uint8_t *)readStr, 5, 0, false) != CB_DATABASE_INDEX_FOUND) {
		printf("TWO INDEXES FIRST INDEX READ CURRENT FAIL\n");
		return 1;
	}
	if (strcmp(readStr, "one1")) {
		printf("TWO INDEXES FIRST INDEX READ DATA CURRENT FAIL\n");
		return 1;
	}
	if (CBDatabaseReadValue(index2, (uint8_t []){0xFF,0xFF,0xFF}, (uint8_t *)readStr, 5, 0, false) != CB_DATABASE_INDEX_FOUND) {
		printf("TWO INDEXES SECOND INDEX READ CURRENT FAIL\n");
		return 1;
	}
	if (strcmp(readStr, "two2")) {
		printf("TWO INDEXES SECOND INDEX READ DATA CURRENT FAIL\n");
		return 1;
	}
	CBDatabaseStage(storage);
	// Check staged
	if (CBDatabaseReadValue(index, (uint8_t []){251,9,8,7,6,5,4,3,2,1}, (uint8_t *)readStr, 5, 0, false) != CB_DATABASE_INDEX_FOUND) {
		printf("TWO INDEXES FIRST INDEX READ STAGED FAIL\n");
		return 1;
	}
	if (strcmp(readStr, "one1")) {
		printf("TWO INDEXES FIRST INDEX READ DATA STAGED FAIL\n");
		return 1;
	}
	if (CBDatabaseReadValue(index2, (uint8_t []){0xFF,0xFF,0xFF}, (uint8_t *)readStr, 5, 0, false) != CB_DATABASE_INDEX_FOUND) {
		printf("TWO INDEXES SECOND INDEX READ STAGED FAIL\n");
		return 1;
	}
	if (strcmp(readStr, "two2")) {
		printf("TWO INDEXES SECOND INDEX READ DATA STAGED FAIL\n");
		return 1;
	}
	CBDatabaseCommit(storage);
	// Check Disk
	if (CBDatabaseReadValue(index, (uint8_t []){251,9,8,7,6,5,4,3,2,1}, (uint8_t *)readStr, 5, 0, false) != CB_DATABASE_INDEX_FOUND) {
		printf("TWO INDEXES FIRST INDEX READ DISK FAIL\n");
		return 1;
	}
	if (strcmp(readStr, "one1")) {
		printf("TWO INDEXES FIRST INDEX READ DATA DISK FAIL\n");
		return 1;
	}
	if (CBDatabaseReadValue(index2, (uint8_t []){0xFF,0xFF, 0xFF}, (uint8_t *)readStr, 5, 0, false) != CB_DATABASE_INDEX_FOUND) {
		printf("TWO INDEXES SECOND INDEX READ DISK FAIL\n");
		return 1;
	}
	if (strcmp(readStr, "two2")) {
		printf("TWO INDEXES SECOND INDEX READ DATA DISK FAIL\n");
		return 1;
	}
	// Create new index
	CBDatabaseIndex * index3 = CBLoadIndex(storage, 2, 10, (10 * CB_DATABASE_BTREE_ELEMENTS + sizeof(CBIndexNode))*10);
	// Generate 10000 key-value pairs
	uint8_t * keys = malloc(100000);
	uint8_t * values = malloc(100000);
	for (uint32_t x = 0; x < 100000; x++)
		keys[x] = rand();
	for (uint32_t x = 0; x < 100000; x++)
		values[x] = rand();
	uint8_t dataRead[10];
	// Insert first 5000, one at a time.
	for (uint16_t x = 0; x < 5000; x++) {
		CBDatabaseWriteValue(index3, keys + x*10, values + x*10, 10);
		CBDatabaseStage(storage);
		if (! CBDatabaseCommit(storage)){
			printf("INSERT FIRST 5000 FAIL COMMIT AT %u\n",x);
			return 1;
		}
		if ((x+1) % 100 == 0) {
			// Verify all of the data
			for (uint16_t y = 0; y <= x; y++) {
				if (CBDatabaseReadValue(index3, keys + y*10, dataRead, 10, 0, false) != CB_DATABASE_INDEX_FOUND) {
					printf("INSERT FIRST 5000 FAIL READ AT %u OF %u\n", y, x);
					return 1;
				}
				if (memcmp(dataRead, values + y * 10, 10)) {
					printf("INSERT FIRST 5000 FAIL READ DATA AT %u OF %u\n", y, x);
					return 1;
				}
			}
			printf("INSERTED AND CHECKED %i/5000 KEY_VALUES\n", x+1);
		}
	}
	// Delete first 1000 keys
	for (uint16_t x = 0; x < 1000; x++) {
		CBDatabaseRemoveValue(index3, keys + x*10, false);
		CBDatabaseStage(storage);
		// Verify data is deleted by staged data
		if (CBDatabaseReadValue(index3, keys + x*10, dataRead, 10, 0, false)
			!= CB_DATABASE_INDEX_NOT_FOUND) {
			printf("DELETE FIRST 1000 FAIL NOT DELETED (STAGED) AT %u\n", x);
			return 1;
		}
		if (! CBDatabaseCommit(storage)) {
			printf("DELETE FIRST 1000 FAIL COMMIT AT %u\n",x);
			return 1;
		}
		// Verify data is deleted
		if (CBDatabaseReadValue(index3, keys + x*10, dataRead, 10, 0, false)
			!= CB_DATABASE_INDEX_NOT_FOUND) {
			printf("DELETE FIRST 1000 FAIL NOT DELETED AT %u\n", x);
			return 1;
		}
		if ((x+1) % 100 == 0) {
			// Verify all of the data
			for (uint16_t y = 0; y < 1000; y++) {
				if (CBDatabaseReadValue(index3, keys + y*10, dataRead, 10, 0, false)
					!= (y > x ? CB_DATABASE_INDEX_FOUND : CB_DATABASE_INDEX_NOT_FOUND)) {
					printf("DELETE FIRST 1000 FAIL READ AT %u OF %u\n", y, x);
					return 1;
				}
				if (y > x && memcmp(dataRead, values + y*10, 10)) {
					printf("DELETE FIRST 1000 FAIL READ DATA AT %u OF %u\n", y, x);
					return 1;
				}
			}
			printf("DELETED AND CHECKED %i/1000 KEY_VALUES\n", x+1);
		}
	}
	// Add last 5000
	for (uint16_t x = 5000; x < 10000; x++) {
		CBDatabaseWriteValue(index3, keys + x*10, values + x*10, 10);
		CBDatabaseStage(storage);
		if (! CBDatabaseCommit(storage)){
			printf("INSERT LAST 5000 FAIL COMMIT AT %u\n",x);
			return 1;
		}
		if ((x+1) % 100 == 0) {
			// Verify all of the data
			for (uint16_t y = 0; y <= x; y++) {
				if (CBDatabaseReadValue(index3, keys + y*10, dataRead, 10, 0, false)
					!= (y >= 1000 ? CB_DATABASE_INDEX_FOUND : CB_DATABASE_INDEX_NOT_FOUND)) {
					printf("INSERT LAST 5000 FAIL READ AT %u OF %u\n", y, x);
					return 1;
				}
				if (y >= 1000 && memcmp(dataRead, values + y*10, 10)) {
					printf("INSERT LAST 5000 FAIL READ DATA AT %u OF %u\n", y, x);
					return 1;
				}
			}
			printf("INSERTED AND CHECKED LAST %i/5000 KEY_VALUES\n", x-4999);
		}
	}
	// Close, reopen and recheck data
	CBFreeDatabase(storage);
	CBFreeIndex(index2);
	CBFreeIndex(index3);
	storage = CBNewDatabase(".", "testDb", 10, 100000, 100000);
	if (! storage) {
		printf("REOPEN 2000 NEW DATABASE FAIL\n");
		return 1;
	}
	index3 = CBLoadIndex(storage, 2, 10, (index->keySize * CB_DATABASE_BTREE_ELEMENTS + sizeof(CBIndexNode))*10);
	if (! index3) {
		printf("REOPEN 2000 LOAD INDEX FAIL\n");
		return 1;
	}
	for (uint16_t y = 0; y < 2000; y++) {
		if (CBDatabaseReadValue(index3, keys + y*10, dataRead, 10, 0, false)
			!= (y >= 1000 ? CB_DATABASE_INDEX_FOUND : CB_DATABASE_INDEX_NOT_FOUND)) {
			printf("REOPEN 2000 KEY-VALUES FAIL READ AT %u\n", y);
			return 1;
		}
		if (y >= 1000 && memcmp(dataRead, values + y*10, 10)) {
			printf("REOPEN 2000 KEY-VALUES FAIL READ DATA AT %u\n", y);
			return 1;
		}
	}
	free(keys);
	free(values);
	// Free objects
	CBFreeDatabase(storage);
	CBFreeIndex(index);
	CBFreeIndex(index3);
	CBFreeIndex(index5);
	return 0;
}
