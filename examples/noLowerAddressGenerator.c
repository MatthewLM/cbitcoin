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

int main(){
	puts("cbitcoin version: " CB_LIBRARY_VERSION_STRING);
	puts("OpenSSL version: " OPENSSL_VERSION_TEXT);
	puts("Will generate addresses with as many non-lower-case letters as possible.");
	puts("Waiting for entropy... Move the cursor around...");
	CBKeyPair * key = CBNewKeyPair(true);
	CBKeyPairGenerate(key);
	#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
	BN_CTX * ctx = BN_CTX_new();
	EC_GROUP * group = EC_GROUP_new_by_curve_name(NID_secp256k1);
	for (unsigned int x = 1;;) {
		CBAddress * address = CBNewAddressFromRIPEMD160Hash(CBKeyPairGetHash(key), CB_NETWORK_PRODUCTION, false);
		CBByteArray * string = CBChecksumBytesGetString(CBGetChecksumBytes(address));
		CBReleaseObject(address);
		bool match = true;
		uint8_t amount = 0;
		for (uint8_t y = 0; y < string->length; y++) {
			if (islower(CBByteArrayGetByte(string, y))) {
				if (y < x)
					match = false;
				amount = y;
				break;
			}
		}
		if (amount == 0)
			amount = string->length;
		if (match) {
			// Print key data to stdout
			printf("No lower for %u\n", amount);
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
		if (amount == string->length)
			break;
		// Get next key
		for (uint8_t x = CB_PRIVKEY_SIZE - 1; ++key->privkey[x--] == 0;);
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
