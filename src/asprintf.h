//
//  asprintf.h
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

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#if ! defined(CB_HAVE_ASPRINTF)

int asprintf(char ** strp, const char * fmt, ...);

#endif
