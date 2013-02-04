//
//  testCBBase58.c
//  cbitcoin
//
//  Created by Matthew Mitchell on 10/05/2012.
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
#include "CBByteArray.h"
#include <time.h>
#include "stdarg.h"
#include "string.h"

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
	// Test string
	char * string = "Hello World!";
	CBByteArray * ba = CBNewByteArrayWithDataCopy((uint8_t *)string, (uint32_t)strlen(string) + 1);
	if (strcmp(string, (char *)CBByteArrayGetData(ba))) {
		printf("STRING COPY FAIL\n");
		return 1;
	}
	return 0;
}
