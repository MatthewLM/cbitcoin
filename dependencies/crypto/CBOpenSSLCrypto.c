//
//  CBOpenSSLCrypto.c
//  cbitcoin
//
//  Created by Matthew Mitchell on 08/08/2012.
//  Copyright (c) 2012 Matthew Mitchell
//  
//  This file is part of cbitcoin.
//
//  cbitcoin is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//  
//  cbitcoin is distributed in the hope that it will be useful, 
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//  
//  You should have received a copy of the GNU General Public License
//  along with cbitcoin.  If not, see <http://www.gnu.org/licenses/>.

// Includes

#include "CBDependencies.h" // cbitcoin dependencies to implement
#include <stdio.h>
#include <stdlib.h>
#include <openssl/sha.h>
#include <openssl/ripemd.h>
#include <openssl/ssl.h>

#pragma GCC diagnostic ignored "-Wdeprecated-declarations" // For OSX Lion

// Implementation

// Generate a private key from just the secret parameter
int EC_KEY_regenerate_key(EC_KEY *eckey, BIGNUM *priv_key)
{
    int ok = 0;
    BN_CTX *ctx = NULL;
    EC_POINT *pub_key = NULL;

    if (!eckey) return 0;

    const EC_GROUP *group = EC_KEY_get0_group(eckey);

    if ((ctx = BN_CTX_new()) == NULL)
        goto err;

    pub_key = EC_POINT_new(group);

    if (pub_key == NULL)
        goto err;

    if (!EC_POINT_mul(group, pub_key, priv_key, NULL, NULL, ctx))
        goto err;

    EC_KEY_set_private_key(eckey,priv_key);
    EC_KEY_set_public_key(eckey,pub_key);

    ok = 1;

err:

    if (pub_key)
        EC_POINT_free(pub_key);
    if (ctx != NULL)
        BN_CTX_free(ctx);

    return(ok);
}


void CBSha160(uint8_t * data, uint16_t len, uint8_t * output){
    SHA1(data, len, output);
}
void CBSha256(uint8_t * data, uint16_t len, uint8_t * output){
	SHA256(data, len, output);
}
void CBRipemd160(uint8_t * data, uint16_t len, uint8_t * output){
	RIPEMD160(data, len, output);
}
bool CBEcdsaVerify(uint8_t * signature, uint8_t sigLen, uint8_t * hash, const uint8_t * pubKey, uint8_t keyLen){
	EC_KEY * key = EC_KEY_new_by_curve_name(NID_secp256k1);
	o2i_ECPublicKey(&key, &pubKey, keyLen);
	int res = ECDSA_verify(0, hash, 32, signature, sigLen, key);
	EC_KEY_free(key);
	return res == 1;
}
bool CBEcdsaSign(uint8_t * hash, uint8_t * privkey, unsigned int *nSig, uint8_t **sig) {
    EC_KEY *pkey = EC_KEY_new_by_curve_name(NID_secp256k1);
    BIGNUM *bn = BN_bin2bn(privkey,32,BN_new());
    if (!EC_KEY_regenerate_key(pkey,bn)) {
        printf("creating failed\n");
	EC_KEY_free(pkey);
        BN_clear_free(bn);
        return 0;        
    }
    BN_clear_free(bn);
    if (!EC_KEY_check_key(pkey)) {
        printf("checking failed\n");
	EC_KEY_free(pkey);
        return 0;
    }
    *nSig = ECDSA_size(pkey);
    *sig = malloc(*nSig + 1);
    if (!ECDSA_sign(0, hash, 32, *sig, nSig, pkey)) {
        printf("signing failed\n");
        EC_KEY_free(pkey);
        return 0;
    }
    EC_KEY_free(pkey);
    printf("signed: %u\n", *nSig);
    return 1;
}
