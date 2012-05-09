//
//  testCBBase58.c
//  cbitcoin
//
//  Created by Matthew Mitchell on 26/04/2012.
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
	unsigned char test[] = {0x00,0xA4,0xA7,0xA4,0xA7,0xA4,0xA7,0xA4,0xA7,0xA4,0xA7,0xA4,0xA7,0xA4,0xA7,0xA4,0xA7,0xA4,0xA7,0xA4,};
	CBEncodeBase58(str,test,20);
	printf("%s\n",str);
}
