//
//  noLowerAddressGenerator.c
//  cbitcoin
//
//  Created by Matthew Mitchell on 18/01/2014.
//  Copyright (c) 2014 Matthew Mitchell
//  
//  This file is part of cbitcoin. It is subject to the license terms
//  in the LICENSE file found in the top-level directory of this
//  distribution and at http://www.cbitcoin.com/license.html. No part of
//  cbitcoin, including this file, may be copied, modified, propagated,
//  or distributed except according to the terms contained in the
//  LICENSE file.

#include <stdio.h>
#include <ctype.h>
#include <openssl/ssl.h>
#include <openssl/ripemd.h>
#include <openssl/rand.h>
#include <pthread.h>
#include "CBAddress.h"
#include "CBHDKeys.h"
#include "CBDependencies.h"
#include "getLine.h"

#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

int cpuNum = 0;
uint8_t prefix;
int highest = 0;

CBDepObject outputMutex;

void CBAddressGenThread(void * vkey) {

	CBAddress address;
	CBKeyPair * key = vkey;

	for (;;) {

		CBInitAddressFromRIPEMD160Hash(&address, CBKeyPairGetHash(key), prefix, false);
		CBByteArray * string = CBChecksumBytesGetString(CBGetChecksumBytes(&address));
		CBDestroyAddress(&address);

		bool match = true;
		uint8_t amount = 0;

		for (uint8_t y = 0; y < string->length - 1; y++) {
			if (islower(CBByteArrayGetByte(string, y))) {
				if (y <= highest)
					match = false;
				amount = y;
				break;
			}
		}

		if (amount == 0)
			amount = string->length - 1;

		if (match) {

			CBWIF wif;
			CBInitWIFFromPrivateKey(&wif, key->privkey, true, CB_NETWORK_PRODUCTION, false);
			CBByteArray * str = CBChecksumBytesGetString(&wif);
			CBDestroyWIF(&wif);

			char publicKeyStr[CB_PUBKEY_SIZE*2 + 1];
			CBBytesToString(key->pubkey.key, 0, CB_PUBKEY_SIZE, publicKeyStr, false);

			// Print key data to stdout

			CBMutexLock(outputMutex);

			printf("No lower for %u\n", amount);
			printf("Private key (WIF): ");
			puts((char *)CBByteArrayGetData(str));
			printf("Public key (hex): %s\n", publicKeyStr);
			printf("Address (base-58): %s\n\n", CBByteArrayGetData(string));
			
			// Set new highest
			highest = amount; 

			CBMutexUnlock(outputMutex);

			CBReleaseObject(str);

		}

		CBReleaseObject(string);

		if (amount == string->length - 1)
			// Got the complete address
			exit(0);

		CBKeyPairGetNext(key);

	}

}

int main() {

	char prefixStr[4];

	puts("cbitcoin version: " CB_LIBRARY_VERSION_STRING);
	puts("OpenSSL version: " OPENSSL_VERSION_TEXT);
	puts("Will generate addresses with as many non-lower-case letters as possible.");

	printf("Type the number of the address prefix (0 default for Bitcoin pubkeyhash):");
	getLine(prefixStr);

	prefix = strlen(prefixStr)? atoi(prefixStr): 0;

	// Get number of cores for threads
	cpuNum = CBGetNumberOfCores();

	puts("Waiting for entropy... Move the cursor around...");

	CBNewMutex(&outputMutex);
	
	CBKeyPair * keys[cpuNum];

	for (int x = 0; x < cpuNum; x++) {

		CBDepObject thread;

		keys[x] = CBNewKeyPair(true);
		CBKeyPairGenerate(keys[x]);
		CBNewThread(&thread, CBAddressGenThread, keys[x]);

	}

	pthread_exit(NULL);	

}

