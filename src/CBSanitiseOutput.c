//
//  CBSanitiseOutput.c
//  cbitcoin
//
//  Created by Matthew Mitchell on 23/03/2014
//  Copyright (c) 2012 Matthew Mitchell
//  
//  This file is part of cbitcoin. It is subject to the license terms
//  in the LICENSE file found in the top-level directory of this
//  distribution and at http://www.cbitcoin.com/license.html. No part of
//  cbitcoin, including this file, may be copied, modified, propagated,
//  or distributed except according to the terms contained in the
//  LICENSE file.

//  SEE HEADER FILE FOR DOCUMENTATION

#include "CBSanitiseOutput.h"

void CBSanitiseOutput(char * str, size_t strLen) {
	
	for (size_t x = 0; x < strLen; x++)
		if (str[x] < 0x20 || str[x] > 0x7e)
			// Not in safe range
			str[x] = ' ';
	
}
