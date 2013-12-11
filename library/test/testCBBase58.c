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
#include "CBBase58.h"
#include <time.h>

void CBLogError(char * b, ...){
	printf("%s\n", b);
}

int main(){
	unsigned int s = (unsigned int)time(NULL);
	printf("Session = %ui\n", s);
	srand(s);
	// Test checked decode
	CBBigInt bi;
	CBBigIntAlloc(&bi, 29);
	CBDecodeBase58Checked(&bi, "1D5A1q5d192j5gYuWiP3CSE5fcaaZxe6E9"); // Valid
	printf("END VALID\n");
	CBDecodeBase58Checked(&bi, "1qBd3Y9D8HhzA4bYSKgkPw8LsX4wCcbqBX"); // Invalid
	// ??? Test for:
	// c5f88541634fb7bade5f94ff671d1febdcbda116d2da779038ed767989
	bi.data[0] = 0xc5;
	bi.data[1] = 0xf8;
	bi.data[2] = 0x85;
	bi.data[3] = 0x41;
	bi.data[4] = 0x63;
	bi.data[5] = 0x4f;
	bi.data[6] = 0xb7;
	bi.data[7] = 0xba;
	bi.data[8] = 0xde;
	bi.data[9] = 0x5f;
	bi.data[10] = 0x94;
	bi.data[11] = 0xff;
	bi.data[12] = 0x67;
	bi.data[13] = 0x1d;
	bi.data[14] = 0x1f;
	bi.data[15] = 0xeb;
	bi.data[16] = 0xdc;
	bi.data[17] = 0xbd;
	bi.data[18] = 0xa1;
	bi.data[19] = 0x16;
	bi.data[20] = 0xd2;
	bi.data[21] = 0xda;
	bi.data[22] = 0x77;
	bi.data[23] = 0x90;
	bi.data[24] = 0x38;
	bi.data[25] = 0xed;
	bi.data[26] = 0x76;
	bi.data[27] = 0x79;
	bi.data[28] = 0x89;
	bi.length = 29;
	char * str = CBEncodeBase58(&bi);
	printf("%s\n", str);
	if (strcmp(str, "7EyVQVmCjB3siBN8DdtuG3ws5jW9xsnT25vbt5eU")) {
		printf("7EyVQVmCjB3siBN8DdtuG3ws5jW9xsnT25vbt5eU FAIL\n");
		return 1;
	}
	free(str);
	unsigned char * verify = malloc(29);
	for (int x = 0; x < 10000; x++) {
		for (int y = 0; y < 29; y++) {
			bi.data[y] = rand();
			verify[y] = bi.data[y];
		}
		bi.length = 29;
		printf("0x");
		for (int y = 0; y < 29; y++)
			printf("%.2x", verify[y]);
		str = CBEncodeBase58(&bi);
		printf(" -> %s -> \n", str);
		CBDecodeBase58(&bi, str);
		free(str);
		printf("0x");
		for (int y = 0; y < 29; y++) {
			printf("%.2x", bi.data[y]);
			if (bi.data[y] != verify[y]) {
				printf(" = FAIL\n");
				return 1;
			}
		}
		printf(" = OK\n");
	}
	free(bi.data);
	free(verify);
	return 0;
}
