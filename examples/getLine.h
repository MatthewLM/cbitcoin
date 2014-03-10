//
//  getLine.h
//  cbitcoin
//
//  Created by Matthew Mitchell on 07/02/2014.
//  Copyright (c) 2014 Matthew Mitchell
//  
//  This file is part of cbitcoin. It is subject to the license terms
//  in the LICENSE file found in the top-level directory of this
//  distribution and at http://www.cbitcoin.com/license.html. No part of
//  cbitcoin, including this file, may be copied, modified, propagated,
//  or distributed except according to the terms contained in the
//  LICENSE file.

#include <stdio.h>

void getLine(char * ptr);
void getLine(char * ptr) {
	int c;
	int len = 30;
    for(;;) {
        c = fgetc(stdin);
        if(c == EOF)
            break;
        if(--len == 0)
            break;
		*ptr = c;
        if(c == '\n')
            break;
		ptr++;
    }
	*ptr = 0;
}
