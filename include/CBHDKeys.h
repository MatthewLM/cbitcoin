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
	unsigned char key[CB_PUBKEY_SIZE];
	unsigned char hash[20];
	bool hashSet;
} CBPubKeyInfo;

typedef struct{
	CBPubKeyInfo pubkey;
	unsigned char privkey[CB_PRIVKEY_SIZE];
} CBKeyPair;

typedef struct{
	int versionBytes;
	CBHDKeyChildID childID;
	int depth;
	unsigned char chainCode[32];
	unsigned char parentFingerprint[4];
	CBKeyPair keyPair[];
} CBHDKey;

// Constructors

CBHDKey * CBNewHDKey(bool priv);
CBHDKey * CBNewHDKeyFromData(unsigned char * data);
CBKeyPair * CBNewKeyPair(bool priv);

// Initialisers

void CBInitHDKey(CBHDKey * key);
bool CBInitHDKeyFromData(CBHDKey * key, unsigned char * data, CBHDKeyVersion versionBytes, CBHDKeyType type);
void CBInitKeyPair(CBKeyPair * key);

//  Functions

bool CBHDKeyDeriveChild(CBHDKey * parentKey, CBHDKeyChildID childID, CBHDKey * childKey);
bool CBHDKeyGenerateMaster(CBHDKey * key, bool production);
int CBHDKeyGetChildNumber(CBHDKeyChildID childID);
unsigned char * CBHDKeyGetHash(CBHDKey * key);
CBNetwork CBHDKeyGetNetwork(CBHDKeyVersion versionBytes);
unsigned char * CBHDKeyGetPrivateKey(CBHDKey * key);
unsigned char * CBHDKeyGetPublicKey(CBHDKey * key);
CBHDKeyType CBHDKeyGetType(CBHDKeyVersion versionBytes);
CBWIF * CBHDKeyGetWIF(CBHDKey * key);
void CBHDKeyHmacSha512(unsigned char * inputData, unsigned char * chainCode, unsigned char * output);
void CBHDKeySerialise(CBHDKey * key, unsigned char * data);
bool CBKeyPairGenerate(CBKeyPair * keyPair);
unsigned char * CBKeyPairGetHash(CBKeyPair * key);
void CBKeyPairGetNext(CBKeyPair * key);

#endif

