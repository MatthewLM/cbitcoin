//
//  testCBSafeOutput.c
//  cbitcoin
//
//  Created by Matthew Mitchell on 21/10/2012.
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

#include "CBSafeOutput.h"
#include <stdarg.h>
#include <stdio.h>

void logError(char * format,...);
void logError(char * format,...){
	va_list argptr;
    va_start(argptr, format);
    vfprintf(stderr, format, argptr);
    va_end(argptr);
	printf("\n");
}

int main(){
	// Add operations to test
	CBSafeOutput output;
	CBSafeOutputAlloc(&output, 6);
	// Save
	CBSafeOutputAddSaveFileOperation(&output, "./test1", "Hello World!", 13);
	CBSafeOutputAddSaveFileOperation(&output, "./test2", "Hahahaha", 9);
	CBSafeOutputAddSaveFileOperation(&output, "./test2", "Blah blah", 10);
	// Truncate
	CBSafeOutputAddTruncateFileOperation(&output, "./test1", 9);
	// Overwrite and append
	CBSafeOutputAddFile(&output, "./test1", 2, 2);
	CBSafeOutputAddOverwriteOperation(&output, 0, "Scary", 5);
	CBSafeOutputAddOverwriteOperation(&output, 6, "L", 1);
	CBSafeOutputAddAppendOperation(&output, "m", 1);
	CBSafeOutputAddAppendOperation(&output, "?!?", 4);
	// Rename
	CBSafeOutputAddRenameFileOperation(&output, "./test1", "./Mr Bean");
	uint64_t backup = CBFileOpen("./test.bak", CB_FILE_MODE_WRITE_AND_READ);
	CHNAGE IT SO THAT THE SAFE OUTPUT ALLOWS FOR MORE THAN ONE OPERATION ON A FILE
	if (CBSafeOutputCommit(&output, backup) != CB_SAFE_OUTPUT_OK){
		printf("COMMIT FAIL\n");
		return 1;
	}
	char read[14];
	uint64_t file1 = CBFileOpen("./test1", CB_FILE_MODE_READ);
	if (NOT CBFileRead(file1, read, 14)) {
		printf("COMMIT FILE 1 READ FAIL\n");
		return 1;
	}
	if (memcmp(read, "Scary Worm?!?", 14)) {
		printf("COMMIT FILE 1 DATA FAIL\n");
		return 1;
	}
	CBFileClose(file1);
	uint64_t file2 = CBFileOpen("./Mr Bean", CB_FILE_MODE_READ);
	if (NOT CBFileRead(file2,read,10)) {
		printf("COMMIT FILE 2 FAIL\n");
		return 1; 
	}
	if (memcmp(read, "Blah blah", 10)) {
		printf("COMMIT FILE 2 DATA FAIL\n");
		return 1;
	}
	CBFileClose(file2);
	CBFileClose(backup);
	// ??? Add fail-case tests. Perhaps by modifying permissions to create failures. Test recovery function.
	return 0;
}
