//
//  asprintf.c
//  cbitcoin
//
//  Created by Matthew Mitchell on 24/12/2013.
//  Copyright (c) 2013 Matthew Mitchell
//
//  This file is part of cbitcoin. It is subject to the license terms
//  in the LICENSE file found in the top-level directory of this
//  distribution and at http://www.cbitcoin.com/license.html. No part of
//  cbitcoin, including this file, may be copied, modified, propagated,
//  or distributed except according to the terms contained in the
//  LICENSE file.

#include "asprintf.h"

#if ! defined(CB_HAVE_ASPRINTF)

int asprintf(char ** strp, const char * fmt, ...){
	va_list argptr;
    va_start(argptr, fmt);
	va_list countList;
	va_copy(countList, argptr);
	int res = vsnprintf(NULL, 0, fmt, countList);
	if (res == -1)
		return -1;
	*strp = malloc(res + 1);
	va_end(countList);
	res = vsprintf(*strp, fmt, argptr);
	va_end(argptr);
	return res;
}

#endif
