//
//  CBLog.h
//  cbitcoin
//
//  Created by Matthew Mitchell on 06/10/2013.
//  Copyright (c) 2013 Matthew Mitchell
//
//  This file is part of cbitcoin. It is subject to the license terms
//  in the LICENSE file found in the top-level directory of this
//  distribution and at http://www.cbitcoin.com/license.html. No part of
//  cbitcoin, including this file, may be copied, modified, propagated,
//  or distributed except according to the terms contained in the
//  LICENSE file.

#ifndef CBLOGH
#define CBLOGH

#include "CBDependencies.h" // cbitcoin dependencies to implement
#include "CBThreads.h"
#include "CBSanitiseOutput.h"
#include <stdarg.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <execinfo.h>

typedef enum{
	CB_LOG_ERROR = 'E',
	CB_LOG_WARNING = 'W',
	CB_LOG_VERBOSE = 'V',
} CBLogType;

void CBLog(CBLogType type, char * prog, char * format, va_list list);
void CBPrintf(bool err, char * message, ...);
void CBPuts(char * message, bool err);

#endif
