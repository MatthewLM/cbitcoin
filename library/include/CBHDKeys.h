//
//  CBHDKeys.h
//  cbitcoin
//
//  Created by Matthew Mitchell on 19/10/2013.
//  Copyright (c) 2013 Matthew Mitchell
//  
//  This file is part of cbitcoin. It is subject to the license terms
//  in the LICENSE file found in the top-level directory of this
//  distribution and at http://www.cbitcoin.com/license.html. No part of
//  cbitcoin, including this file, may be copied, modified, propagated,
//  or distributed except according to the terms contained in the
//  LICENSE file.

/**
 @file
 @brief See BIP0032
*/

#ifndef CBHDKEYSH
#define CBHDKEYSH

//  Includes

#include "CBDependencies.h"
#include "CBBigInt.h"
#include "CBWIF.h"

// Macros

#define CB_PUBKEY_SIZE 33
#define CB_PRIVKEY_SIZE 32
#define CB_HD_KEY_STR_SIZE 82

// Enums

typedef enum{
	CB_HD_KEY_VERSION_PROD_PRIVATE = 0x0488ADE4,
	CB_HD_KEY_VERSION_PROD_PUBLIC = 0x0488B21E,
	CB_HD_KEY_VERSION_TEST_PRIVATE = 0x04358394,
	CB_HD_KEY_VERSION_TEST_PUBLIC = 0x043587CF,
} CBHDKeyVersion;

typedef enum{
	CB_HD_KEY_TYPE_PRIVATE,
	CB_HD_KEY_TYPE_PUBLIC,
	CB_HD_KEY_TYPE_UNKNOWN
} CBHDKeyType;

// Structures

typedef struct{
	unsigned int priv:1;
	unsigned int childNumber:31;
} CBHDKeyChildID;

typedef struct{
	uint8_t key[CB_PUBKEY_SIZE];
	uint8_t hash[20];
	bool hashSet;
} CBPubKeyInfo;

typedef struct{
	CBPubKeyInfo pubkey;
	uint8_t privkey[CB_PRIVKEY_SIZE];
} CBKeyPair;

typedef struct{
	uint32_t versionBytes;
	CBHDKeyChildID childID;
	uint8_t depth;
	uint8_t chainCode[32];
	uint8_t parentFingerprint[4];
	CBKeyPair keyPair[];
} CBHDKey;

// Constructors

CBHDKey * CBNewHDKey(bool priv);
CBHDKey * CBNewHDKeyFromData(uint8_t * data);
CBKeyPair * CBNewKeyPair(bool priv);

// Initialisers

void CBInitHDKey(CBHDKey * key);
bool CBInitHDKeyFromData(CBHDKey * key,uint8_t * data, CBHDKeyVersion versionBytes, CBHDKeyType type);
void CBInitKeyPair(CBKeyPair * key);

//  Functions

bool CBHDKeyDeriveChild(CBHDKey * parentKey, CBHDKeyChildID childID, CBHDKey * childKey);
bool CBHDKeyGenerateMaster(CBHDKey * key, bool production);
uint32_t CBHDKeyGetChildNumber(CBHDKeyChildID childID);
uint8_t * CBHDKeyGetHash(CBHDKey * key);
CBNetwork CBHDKeyGetNetwork(CBHDKeyVersion versionBytes);
uint8_t * CBHDKeyGetPrivateKey(CBHDKey * key);
uint8_t * CBHDKeyGetPublicKey(CBHDKey * key);
CBHDKeyType CBHDKeyGetType(CBHDKeyVersion versionBytes);
CBWIF * CBHDKeyGetWIF(CBHDKey * key);
void CBHDKeyHmacSha512(uint8_t * inputData, uint8_t * chainCode, uint8_t * output);
void CBHDKeySerialise(CBHDKey * key, uint8_t * data);
bool CBKeyPairGenerate(CBKeyPair * keyPair);
uint8_t * CBKeyPairGetHash(CBKeyPair * key);

#endif
