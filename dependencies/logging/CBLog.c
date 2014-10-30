//
//  CBLog.c
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

#include "CBLog.h"

pthread_mutex_t logMutex = PTHREAD_MUTEX_INITIALIZER;
FILE * logFile = NULL;

// Implementation

void CBLog(CBLogType type, char * prog, char * format, va_list argptr) {
	
	char date[20];
	bool err = type == CB_LOG_ERROR;
	
	time_t now = time(NULL);
	strftime(date, sizeof(date), "%d/%m/%Y %H:%M:%S", gmtime(&now));
	
	pthread_mutex_lock(&logMutex);
	
	CBPrintf(err, " %c | %-10s | %s GMT | %02u | ", type, prog, date, CBGetTID());
	
	// Write to buffer to replace new lines
	va_list bufCount;
	va_copy(bufCount, argptr);
	char buf[vsnprintf(NULL, 0, format, bufCount) + 1], *bufPtr;
	va_end(bufCount);
	vsprintf(buf, format, argptr);
	va_end(argptr);
	
	// Set to begining of buffer
	bufPtr = buf;
	
	for (;;){
		char * nl = strchr(bufPtr, '\n');
		if (nl != NULL)
			*nl = '\0';
		
		CBPuts(bufPtr, err);
		CBPuts("\n", err);
		
		if (nl != NULL) {
			bufPtr = nl + 1;
			CBPuts("   |            |                         |    | ", err);
		}else
			break;
	}
	
	if (type == CB_LOG_ERROR) {
		// Print stacktrace
		void * array[20];
		int size = backtrace(array, 20);
		char ** strings = backtrace_symbols(array, size);
		CBPuts("   |            |                         |    |\n", true);
		CBPuts("   |            |                         |    | ERROR VERSION: " CB_LIBRARY_VERSION_STRING "\n", true);
		CBPuts("   |            |                         |    | ERROR STACK TRACE:\n", true);
		for (int x = 2; x < size; x++)
			CBPrintf(err, "   |            |                         |    | %s\n", strings[x]);
		free(strings);
		CBPuts("   |            |                         |    |\n", true);
	}
	
	pthread_mutex_unlock(&logMutex);
	
}

void CBLogError(char * format, ...) {
	
	va_list argptr;
    va_start(argptr, format);
	CBLog(CB_LOG_ERROR, "cbitcoin", format, argptr);
	
}

void CBLogWarning(char * format, ...) {
	
	va_list argptr;
    va_start(argptr, format);
	CBLog(CB_LOG_WARNING, "cbitcoin", format, argptr);
	
}

void CBLogVerbose(char * format, ...) {
	
	va_list argptr;
    va_start(argptr, format);
	CBLog(CB_LOG_VERBOSE, "cbitcoin", format, argptr);
	
}

void CBLogFile(char * file) {
	
	// Set the file
	if (logFile)
		fclose(logFile);
	logFile = fopen(file, "a");
	fputs("\n\n\n\n\n", logFile);
	
}

void CBPrintf(bool err, char * message, ...) {
	
	va_list argptr;
    va_start(argptr, message);
	if (logFile){
		va_list argptr2;
		va_copy(argptr2, argptr);
		vfprintf(logFile, message, argptr2);
		va_end(argptr2);
	}
	vfprintf(err ? stderr : stdout, message, argptr);
	va_end(argptr);
	
}

void CBPuts(char * message, bool err) {
	
	if (logFile)
		fputs(message, logFile);
	fputs(message, err ? stderr : stdout);
	
}
