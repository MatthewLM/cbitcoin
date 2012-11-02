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

void onErrorReceived(CCBrror a,char * format,...);
void onErrorReceived(CCBrror a,char * format,...){
	va_list argptr;
    va_start(argptr, format);
    vfprintf(stderr, format, argptr);
    va_end(argptr);
	printf("\n");
}

int main(){
	// First create two files with spme data.
	FILE * file1 = fopen("./test1", "wb+");
	fwrite("Hello World!", 1, 13, file1);
	FILE * file2 = fopen("./test2", "wb+");
	fwrite("Heave", 1, 6, file2);
	// Close files
	fclose(file1);
	fclose(file2);
	// Write two operations per file.
	CBSafeOutput * output = CBNewSafeOutput(onErrorReceived);
	CBSafeOutputAlloc(output, 2);
	CBSafeOutputAddFile(output, "./test1", 2, 0);
	CBSafeOutputAddOverwriteOperation(output, 0, "Scary", 5);
	CBSafeOutputAddOverwriteOperation(output, 9, "ds", 2);
	CBSafeOutputAddFile(output, "./test2", 1, 2);
	CBSafeOutputAddOverwriteOperation(output, 4, "y ", 2);
	CBSafeOutputAddAppendOperation(output, "Metal", 5);
	CBSafeOutputAddAppendOperation(output, "!", 2);
	FILE * backup = fopen("./test.bak", "wb+");
	if (CBSafeOutputCommit(output, backup) != CB_SAFE_OUTPUT_OK){
		printf("COMMIT FAIL\n");
		return 1;
	}
	char read[13];
	file1 = fopen("./test1", "rb");
	if (fread(read, 1, 13, file1) != 13) {
		printf("COMMIT FILE 1 SIZE FAIL\n");
		return 1;
	}
	if (memcmp(read, "Scary Words!", 13)) {
		printf("COMMIT FILE 1 DATA FAIL\n");
		return 1;
	}
	file2 = fopen("./test2", "rb");
	if (fread(read, 1, 13, file2) != 13) {
		printf("COMMIT FILE 2 SIZE FAIL\n");
		return 1; 
	}
	if (memcmp(read, "Heavy Metal!", 13)) {
		printf("COMMIT FILE 2 DATA FAIL\n");
		return 1;
	}
	fclose(backup);
	// ??? Add fail-case tests. Perhaps by modifying permissions to create failures. Test recovery function.
	return 0;
}
