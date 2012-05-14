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

int main(){
	char str[41];
	unsigned char * test = malloc(29);
	// ??? Test for:
	// c5f88541634fb7bade5f94ff671d1febdcbda116d2da779038ed767989
	// 7EyVQVmCjB3siBN8DdtuG3ws5jW9xsnT25vbt5eU = CORRECT
	// 7EyVQVmCjB3siBN8DdtuG3ws64y9xsnT25vbt5eU = FAILURE
	
	unsigned char * verify = malloc(29);
	for (int x = 0; x < 1000; x++) {
		for (int y = 0; y < 29; y++) {
			test[y] = rand();
			verify[y] = test[y];
		}
		printf("0x");
		for (int y = 0; y < 29; y++) {
			printf("%.2x",verify[y]);
		}
		CBEncodeBase58(str,test,29);
		printf(" -> %s -> \n",str);
		CBBigInt bi = CBDecodeBase58(str);
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