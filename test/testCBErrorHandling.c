//
//  testCBErrorHandling.c
//  cbitcoin
//
//  Created by Matthew Mitchell on 22/06/2013.
//  Copyright (c) 2012 Matthew Mitchell
//  
//  This file is part of cbitcoin. It is subject to the license terms
//  in the LICENSE file found in the top-level directory of this
//  distribution and at http://www.cbitcoin.com/license.html. No part of
//  cbitcoin, including this file, may be copied, modified, propagated,
//  or distributed except according to the terms contained in the
//  LICENSE file.

#include <stdio.h>
#include "CBConstants.h"
#include <stdbool.h>
#include <time.h>

int main(){
	bool didCatch = false;
	bool didContinue = false;
	bool noThrowFinally = false;
	bool throwFinally = false;
	bool noThrowPass = false;
	try{
		try{
			try{
				for (;;) {
					throw;
				}
				printf("FAILED THROW IN FOR LOOP");
				return 1;
			}stop
			didContinue = true;
			try {
				
			}
			noThrowFinally = true;
			catch{
				printf("REACHED CATCH WITHOUT THROW");
				return 1;
			}pass;
			noThrowPass = true;
			throw;
			printf("FAILED TO THROW\n");
		}
		throwFinally = true;
		catch{
			didCatch = true;
		}pass;
		printf("FAILED TO PASS\n");
		return 1;
	}stop
	if (NOT didCatch){
		printf("FAILED TO CATCH\n");
		return 1;
	}
	if (NOT didContinue){
		printf("FAILED TO CONTINUE\n");
		return 1;
	}
	if (NOT noThrowFinally){
		printf("FAILED TO DO NO THROW FINALLY\n");
		return 1;
	}
	if (NOT throwFinally){
		printf("FAILED TO DO THROW FINALLY\n");
		return 1;
	}
	if (NOT noThrowPass){
		printf("FAILED TO PASS NO THROW\n");
		return 1;
	}
	return 0;
}


