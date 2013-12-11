//
//  CBLog.c
//  cbitcoin
//
//  Created by Matthew Mitchell on 06/10/2013.
//  Copyright (c) 2012 Matthew Mitchell
//
//  This file is part of cbitcoin. It is subject to the license terms
//  in the LICENSE file found in the top-level directory of this
//  distribution and at http://www.cbitcoin.com/license.html. No part of
//  cbitcoin, including this file, may be copied, modified, propagated,
//  or distributed except according to the terms contained in the
//  LICENSE file.

#include "CBLog.h"

pthread_mutex_t logMutex = PTHREAD_MUTEX_INITIALIZER;

// Implementation

void CBLog(CBLogType type, char * prog, char * format, va_list argptr){
	char date[20];
	time_t now = time(NULL);
	strftime(date, sizeof(date), "%d/%m/%Y %H:%M:%S", gmtime(&now));
	pthread_mutex_lock(&logMutex);
	FILE * output = type == CB_LOG_ERROR ? stderr : stdout;
	fprintf(output, " %c | %-10s | %s GMT | %02u | ", type, prog, date, CBGetTID());
	// Write to buffer to replace new lines
	va_list bufCount;
	va_copy(bufCount, argptr);
	char buf[vsnprintf(NULL, 0, format, bufCount) + 1], *bufPtr;
	va_end(bufCount);
	vsprintf(buf, format, argptr);
	va_end(argptr);
	bufPtr = buf;
	for (;;){
		char * nl = strchr(bufPtr, '\n');
		if (nl != NULL)
			*nl = '\0';
		puts(bufPtr);
		if (nl != NULL) {
			bufPtr = nl + 1;
			fputs("   |            |                         |    | ", output);
		}else
			break;
	}
	if (type == CB_LOG_ERROR) {
		// Print stacktrace
		void * array[20];
		int size = backtrace(array, 20);
		char ** strings = backtrace_symbols(array, size);
		fputs("   |            |                         |    |\n", stderr);
		fputs("   |            |                         |    | ERROR VERSION: " CB_LIBRARY_VERSION_STRING "\n", stderr);
		fputs("   |            |                         |    | ERROR STACK TRACE:\n", stderr);
		for (int x = 2; x < size; x++)
			fprintf(stderr, "   |            |                         |    | %s\n", strings[x]);
		free(strings);
		fputs("   |            |                         |    |\n", stderr);
	}
	pthread_mutex_unlock(&logMutex);
}

void CBLogError(char * format, ...){
	va_list argptr;
    va_start(argptr, format);
	CBLog(CB_LOG_ERROR, "cbitcoin", format, argptr);
}

void CBLogWarning(char * format, ...){
	va_list argptr;
    va_start(argptr, format);
	CBLog(CB_LOG_WARNING, "cbitcoin", format, argptr);
}

void CBLogVerbose(char * format, ...){
	va_list argptr;
    va_start(argptr, format);
	CBLog(CB_LOG_VERBOSE, "cbitcoin", format, argptr);
}
