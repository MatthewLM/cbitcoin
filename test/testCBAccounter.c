//
//  testCBAccounter.c
//  cbitcoin
//
//  Created by Matthew Mitchell on 16/02/2013.
//  Copyright (c) 2012 Matthew Mitchell
//
//  This file is part of cbitcoin. It is subject to the license terms
//  in the LICENSE file found in the top-level directory of this
//  distribution and at http://www.cbitcoin.com/license.html. No part of
//  cbitcoin, including this file, may be copied, modified, propagated,
//  or distributed except according to the terms contained in the
//  LICENSE file.

#include <stdio.h>
#include "CBAccounter.h"
#include <time.h>

void CBLogError(char * b, ...);
void CBLogError(char * b, ...){
	printf("%s\n", b);
	exit(EXIT_FAILURE);
}

int main(){
	remove("./acnt_log.dat");
	remove("./acnt_0.dat");
	remove("./acnt_1.dat");
	remove("./acnt_2.dat");
	uint64_t accounter = CBNewAccounter("./");
	
	return 0;
}
