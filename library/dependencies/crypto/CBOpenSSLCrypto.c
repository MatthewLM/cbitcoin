//
//  CBOpenSSLCrypto.c
//  cbitcoin
//
//  Created by Matthew Mitchell on 08/08/2012.
//  Copyright (c) 2012 Matthew Mitchell
//  
//  This file is part of cbitcoin. It is subject to the license terms
//  in the LICENSE file found in the top-level directory of this
//  distribution and at http://www.cbitcoin.com/license.html. No part of
//  cbitcoin, including this file, may be copied, modified, propagated,
//  or distributed except according to the terms contained in the
//  LICENSE file.

// Includes

#include "CBDependencies.h" // cbitcoin dependencies to implement
#include <stdio.h>
#include <stdlib.h>
#include <openssl/sha.h>
#include <openssl/ripemd.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#pragma GCC diagnostic ignored "-Wdeprecated-declarations" // For OSX Lion

// Implementation

void CBAddPoints(uint8_t * point1, uint8_t * point2) {
	
	// Get group
	EC_GROUP * group = EC_GROUP_new_by_curve_name(NID_secp256k1);
	
	// Get OpenSSL representations of points
	EC_POINT * p1 = EC_POINT_new(group);
	EC_POINT * p2 = EC_POINT_new(group);
	BN_CTX * ctx = BN_CTX_new();
	EC_POINT_oct2point(group, p1, point1, 33, ctx);
	EC_POINT_oct2point(group, p2, point2, 33, ctx);
	
	// Add points together
	EC_POINT * result = EC_POINT_new(group);
	EC_POINT_add(group, result, p1, p2, ctx);
	
	// Free points
	EC_POINT_free(p1);
	EC_POINT_free(p2);
	
	// Write result to point1
	EC_POINT_point2oct(group, result, POINT_CONVERSION_COMPRESSED, point1, 33, ctx);
	
	// Free result, group and ctx
	EC_POINT_free(result);
	EC_GROUP_free(group);
	BN_CTX_free(ctx);
	
}

void CBKeyGetPublicKey(uint8_t * privKey, uint8_t * pubKey) {
	
	BIGNUM * privBn = BN_bin2bn(privKey, 32, NULL);
	EC_GROUP * group = EC_GROUP_new_by_curve_name(NID_secp256k1);
	EC_POINT * point = EC_POINT_new(group);
	BN_CTX * ctx = BN_CTX_new();
	
	EC_POINT_mul(group, point, privBn, NULL, NULL, ctx);
	EC_POINT_point2oct(group, point, POINT_CONVERSION_COMPRESSED, pubKey, 33, ctx);
	
	BN_CTX_free(ctx);
	EC_POINT_free(point);
	EC_GROUP_free(group);
	BN_free(privBn);
	
}

uint8_t CBKeySign(uint8_t * privKey, uint8_t * hash, uint8_t * signature) {
	
	unsigned int sigSize;
	EC_KEY * key = EC_KEY_new_by_curve_name(NID_secp256k1);
	BIGNUM * bn = BN_bin2bn(privKey, 32, NULL);
	
	EC_KEY_set_private_key(key, bn);
	ECDSA_sign(0, hash, 32, signature, &sigSize, key);
	
	// Free key and BIGNUM
	EC_KEY_free(key);
	BN_free(bn);
	
	return sigSize;
	
}

void CBSha160(uint8_t * data, uint16_t len, uint8_t * output) {
	
    SHA1(data, len, output);
	
}

void CBSha256(uint8_t * data, uint16_t len, uint8_t * output) {
	
	SHA256(data, len, output);
	
}

void CBSha512(uint8_t * data, uint16_t len, uint8_t * output) {
	
	SHA512(data, len, output);
	
}

void CBRipemd160(uint8_t * data, uint16_t len, uint8_t * output) {
	
	RIPEMD160(data, len, output);
	
}

bool CBEcdsaVerify(uint8_t * signature, uint8_t sigLen, uint8_t * hash, uint8_t * pubKey, uint8_t keyLen) {
	
	EC_KEY * eckey, * key;
	int res = 0;
	
	eckey = key = EC_KEY_new_by_curve_name(NID_secp256k1);
	o2i_ECPublicKey(&key, (const unsigned char **)&pubKey, keyLen);
	
	// If o2i_ECPublicKey fails it makes key equal to NULL.
	if (key != NULL)
		res = ECDSA_verify(0, hash, 32, signature, sigLen, key);
	
	EC_KEY_free(eckey);
	
	return res == 1;
	
}
