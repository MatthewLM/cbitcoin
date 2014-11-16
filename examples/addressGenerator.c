//
//  addressGenerator.c
//  cbitcoin
//
//  Created by Matthew Mitchell on 06/06/2012.
//  Copyright (c) 2012 Matthew Mitchell
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
#include "CBAddress.h"
#include "CBHDKeys.h"
#include "getLine.h"

#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

int main(){
	puts("cbitcoin version: " CB_LIBRARY_VERSION_STRING);
	puts("OpenSSL version: " OPENSSL_VERSION_TEXT);
	printf("Enter the number of keys: ");
	fflush(stdout);
	char stringMatch[31];
	getLine(stringMatch);
	unsigned long int i = strtol(stringMatch, NULL, 0);
	printf("Enter a string of text for the key (30 max): ");
	fflush(stdout);
	getLine(stringMatch);
	puts("Waiting for entropy... Move the cursor around...");
	CBKeyPair * key = CBNewKeyPair(true);
	CBKeyPairGenerate(key);
	BN_CTX * ctx = BN_CTX_new();
	EC_GROUP * group = EC_GROUP_new_by_curve_name(NID_secp256k1);
	printf("Making %lu addresses for \"%s\"\n\n", i, stringMatch);
	for (unsigned int x = 0; x < i;) {
		CBAddress * address = CBNewAddressFromRIPEMD160Hash(CBKeyPairGetHash(key), CB_PREFIX_PRODUCTION_ADDRESS, false);
		CBByteArray * string = CBChecksumBytesGetString(CBGetChecksumBytes(address));
		CBReleaseObject(address);
		bool match = true;
		int offset = 1;
		size_t matchSize = strlen(stringMatch);
		for (int y = 0; y < matchSize;) {
			char other = islower(stringMatch[y]) ? toupper(stringMatch[y]) : (isupper(stringMatch[y])? tolower(stringMatch[y]) : '\0');
			if (CBByteArrayGetByte(string, y+offset) != stringMatch[y] && CBByteArrayGetByte(string, y+offset) != other) {
				offset++;
				y = 0;
				if (string->length < matchSize + offset) {
					match = false;
					break;
				}
			}else y++;
		}
		if (match) {
			// Print key data to stdout
			printf("Private key (WIF): ");
			CBWIF * wif = CBNewWIFFromPrivateKey(key->privkey, true, CB_NETWORK_PRODUCTION, false);
			CBByteArray * str = CBChecksumBytesGetString(wif);
			CBReleaseObject(wif);
			puts((char *)CBByteArrayGetData(str));
			CBReleaseObject(str);
			// Print public key
			printf("Public key (hex): ");
			for (int y = 0; y < CB_PUBKEY_SIZE; y++)
				printf(" %.2X", key->pubkey.key[y]);
			printf("\nAddress (base-58): %s\n\n", CBByteArrayGetData(string));
			x++; // Move to next
		}
		CBReleaseObject(string);
		// Get next key
		for (int x = CB_PRIVKEY_SIZE - 1; ++key->privkey[x--] == 0;);
		// Get new public key, by adding the base point.
		EC_POINT * pub = EC_POINT_new(group);
		EC_POINT * new = EC_POINT_new(group);
		EC_POINT_oct2point(group, pub, key->pubkey.key, CB_PUBKEY_SIZE, ctx);
		EC_POINT_add(group, new, (EC_POINT *)EC_GROUP_get0_generator(group), pub, ctx);
		EC_POINT_point2oct(group, new, POINT_CONVERSION_COMPRESSED, key->pubkey.key, CB_PUBKEY_SIZE, ctx);
		EC_POINT_free(pub);
		EC_POINT_free(new);
		// Invalidate hash
		key->pubkey.hashSet = false;
	}
	EC_GROUP_free(group);
	BN_CTX_free(ctx);
	free(key);
	return 0;
}
