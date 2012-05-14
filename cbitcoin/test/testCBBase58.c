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
	char str[28];
	unsigned char * test = malloc(4);
	test[0] = 0xBE;
	test[1] = 0xFA;
	test[2] = 0x0B;
	test[3] = 0x00;
	CBEncodeBase58(str,test,4);
	printf("%s\n",str);
	CBBigInt bi = CBDecodeBase58("152Ny");
	printf("0x");
	for (int x = 0; x < bi.length; x++) {
		printf("%.2x",bi.data[x]);
	}
	printf("\n");
	free(bi.data);
}
