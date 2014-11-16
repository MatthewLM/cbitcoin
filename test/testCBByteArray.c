//
//  testCBBase58.c
//  cbitcoin
//
//  Created by Matthew Mitchell on 10/05/2012.
//  Copyright (c) 2012 Matthew Mitchell
//  
//  This file is part of cbitcoin. It is subject to the license terms
//  in the LICENSE file found in the top-level directory of this
//  distribution and at http://www.cbitcoin.com/license.html. No part of
//  cbitcoin, including this file, may be copied, modified, propagated,
//  or distributed except according to the terms contained in the
//  LICENSE file.

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
	CBByteArray * ba = CBNewByteArrayWithDataCopy((unsigned char *)string, (int)strlen(string) + 1);
	if (strcmp(string, (char *)CBByteArrayGetData(ba))) {
		printf("STRING COPY FAIL\n");
		return 1;
	}
	CBFreeByteArray(ba);
	return 0;
}
