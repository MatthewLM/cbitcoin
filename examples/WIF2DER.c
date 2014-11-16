//
//  WIF2DER.c
//  cbitcoin
//
//  Created by Matthew Mitchell on 02/07/2014.
//  Copyright (c) 2014 Matthew Mitchell
//
//  This file is part of cbitcoin. It is subject to the license terms
//  in the LICENSE file found in the top-level directory of this
//  distribution and at http://www.cbitcoin.com/license.html. No part of
//  cbitcoin, including this file, may be copied, modified, propagated,
//  or distributed except according to the terms contained in the
//  LICENSE file.

#include "CBWIF.h"
#include <openssl/ssl.h>


int main(int argc, char * argv[]) {

	CBWIF wif;

	if (argc != 2)
		return EXIT_FAILURE;

	// Decode WIF string
	CBByteArray str;
	CBInitByteArrayFromString(&str, argv[1], false);
	CBInitWIFFromString(&wif, &str, false);
	CBDestroyByteArray(&str);

	// Get key
	unsigned char key[32];
	CBWIFGetPrivateKey(&wif, key);
	CBDestroyWIF(&wif);

	// Create OpenSSL key
	
	EC_KEY * eckey = EC_KEY_new_by_curve_name(NID_secp256k1);
	BIGNUM * bn = BN_bin2bn(key, CB_PRIVKEY_SIZE, NULL);

	if (!EC_KEY_set_private_key(eckey, bn)) 
		return EXIT_FAILURE;

	// Create public key as OpenSSL cannot do this easily
	
	EC_GROUP * group = EC_GROUP_new_by_curve_name(NID_secp256k1);
	EC_POINT * point = EC_POINT_new(group);
	BN_CTX * ctx = BN_CTX_new();
	
	EC_POINT_mul(group, point, bn, NULL, NULL, ctx);
	
	BN_CTX_free(ctx);
	EC_GROUP_free(group);
	BN_free(bn);

	if (!EC_KEY_set_public_key(eckey, point))
		return EXIT_FAILURE;

	EC_POINT_free(point);

	// Check the key

	if (!EC_KEY_check_key(eckey))
		return EXIT_FAILURE;

	// Convert key to DER format
	
	int len = i2d_ECPrivateKey(eckey, NULL);	
	unsigned char derkey[len];
	unsigned char * derkeyPtr = derkey;
	i2d_ECPrivateKey(eckey, &derkeyPtr);

	// Freeing the EC_KEY here crashes for some reason???

	// Encode DER key as hex
	
	char out[len*2+1];	
	CBBytesToString(derkey, 0, len, out, false);

	// Print to stdout
	
	puts(out);

	return EXIT_SUCCESS;

}

