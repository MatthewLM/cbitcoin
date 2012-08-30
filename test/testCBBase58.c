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
#include "CBBase58.h"
#include <time.h>

void err(CBError a,char * b,...);
void err(CBError a,char * b,...){
	printf("%s\n",b);
}

int main(){
	unsigned int s = (unsigned int)time(NULL);
	printf("Session = %ui\n",s);
	srand(s);
	// Test checked decode
	CBEvents events;
	events.onErrorReceived = err;
	CBDecodeBase58Checked("1D5A1q5d192j5gYuWiP3CSE5fcaaZxe6E9", &events); // Valid
	printf("END VALID\n");
	CBDecodeBase58Checked("1qBd3Y9D8HhzA4bYSKgkPw8LsX4wCcbqBX", &events); // Invalid
	unsigned char * test = malloc(29);
	// ??? Test for:
	// c5f88541634fb7bade5f94ff671d1febdcbda116d2da779038ed767989
	test[0] = 0xc5;
	test[1] = 0xf8;
	test[2] = 0x85;
	test[3] = 0x41;
	test[4] = 0x63;
	test[5] = 0x4f;
	test[6] = 0xb7;
	test[7] = 0xba;
	test[8] = 0xde;
	test[9] = 0x5f;
	test[10] = 0x94;
	test[11] = 0xff;
	test[12] = 0x67;
	test[13] = 0x1d;
	test[14] = 0x1f;
	test[15] = 0xeb;
	test[16] = 0xdc;
	test[17] = 0xbd;
	test[18] = 0xa1;
	test[19] = 0x16;
	test[20] = 0xd2;
	test[21] = 0xda;
	test[22] = 0x77;
	test[23] = 0x90;
	test[24] = 0x38;
	test[25] = 0xed;
	test[26] = 0x76;
	test[27] = 0x79;
	test[28] = 0x89;
	char * str = CBEncodeBase58(test,29);
	printf("%s\n",str);
	if (strcmp(str, "7EyVQVmCjB3siBN8DdtuG3ws5jW9xsnT25vbt5eU")) {
		printf("7EyVQVmCjB3siBN8DdtuG3ws5jW9xsnT25vbt5eU FAIL\n");
		return 1;
	}
	free(str);
	unsigned char * verify = malloc(29);
	for (int x = 0; x < 10000; x++) {
		for (int y = 0; y < 29; y++) {
			test[y] = rand();
			verify[y] = test[y];
		}
		printf("0x");
		for (int y = 0; y < 29; y++) {
			printf("%.2x",verify[y]);
		}
		str = CBEncodeBase58(test,29);
		printf(" -> %s -> \n",str);
		CBBigInt bi = CBDecodeBase58(str);
		free(str);
		printf("0x");
		for (int y = 0; y < 29; y++) {
			printf("%.2x",bi.data[y]);
			if (bi.data[y] != verify[y]) {
				printf(" = FAIL\n");
				return 1;
			}
		}
		printf(" = OK\n");
		free(bi.data);
	}
	return 0;
}