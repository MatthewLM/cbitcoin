//
//  CBHDKeys.c
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

//  SEE HEADER FILE FOR DOCUMENTATION 

#include "CBHDKeys.h"

uint8_t CB_CURVE_ORDER[32] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFE,0xBA,0xAE,0xDC,0xE6,0xAF,0x48,0xA0,0x3B,0xBF,0xD2,0x5E,0x8C,0xD0,0x36,0x41,0x41};

CBHDKey * CBNewHDKey(bool priv){
	CBHDKey * key = malloc(sizeof(*key) + (priv ? sizeof(CBKeyPair) : sizeof(CBPubKeyInfo)));
	CBInitHDKey(key);
	return key;
}
CBHDKey * CBNewHDKeyFromData(uint8_t * data){
	CBHDKeyVersion versionBytes = CBArrayToInt32BigEndian(data, 0);
	CBHDKeyType type = CBHDKeyGetType(versionBytes);
	if (type == CB_HD_KEY_TYPE_UNKNOWN) {
		CBLogError("Unknown key type.");
		return NULL;
	}
	// Allocate memory for key
	CBHDKey * key = CBNewHDKey(type == CB_HD_KEY_TYPE_PRIVATE);
	return CBInitHDKeyFromData(key, data, versionBytes, type) ? key : (free(key), NULL);
}
CBKeyPair * CBNewKeyPair(bool priv){
	CBKeyPair * key = malloc(priv ? sizeof(CBKeyPair) : sizeof(CBPubKeyInfo));
	CBInitKeyPair(key);
	return key;
}

void CBInitHDKey(CBHDKey * key){
	CBInitKeyPair(key->keyPair);
}
bool CBInitHDKeyFromData(CBHDKey * key, uint8_t * data, CBHDKeyVersion versionBytes, CBHDKeyType type){
	CBInitHDKey(key);
	// Set version bytes
	key->versionBytes = versionBytes;
	// Chain code
	memcpy(key->chainCode, data + 13, 32);
	if (type == CB_HD_KEY_TYPE_PRIVATE){
		// Private
		memcpy(key->keyPair->privkey, data + 46, 32);
		// Calculate public key
		CBKeyGetPublicKey(CBHDKeyGetPrivateKey(key), CBHDKeyGetPublicKey(key));
	}else
		// Assume public
		memcpy(CBHDKeyGetPublicKey(key), data + 45, 33);
	// Depth
	key->depth = data[4];
	if (key->depth == 0) {
		// Master
		key->childID.priv = false;
		key->childID.childNumber = 0;
		for (uint8_t x = 5; x < 9; x++)
			if (data[x] != 0) {
				CBLogError("The fingerprint of the master key is not zero.");
				return false;
			}
	}else{
		// Not master
		uint32_t childNum = CBArrayToInt32BigEndian(data, 9);
		key->childID.priv = childNum >> 31;
		key->childID.childNumber = childNum & 0x8fffffff;
	}
	return true;
}
void CBInitKeyPair(CBKeyPair * key){
	key->pubkey.hashSet = false;
}

bool CBHDKeyDeriveChild(CBHDKey * parentKey, CBHDKeyChildID childID, CBHDKey * childKey){
	uint8_t hash[64], inputData[37];
	CBHDKeyType type = CBHDKeyGetType(parentKey->versionBytes);
	// Add childID to inputData
	CBInt32ToArrayBigEndian(inputData, 33, CBHDKeyGetChildNumber(childID));
	if (childID.priv) {
		// Private key derivation
		// Parent must be private
		if (type != CB_HD_KEY_TYPE_PRIVATE) {
			CBLogError("Attempting to derive a public-key without public-key derivation, or the key was of an unknown type.");
			return false;
		}
		// Add private key
		inputData[0] = 0;
		memcpy(inputData + 1, CBHDKeyGetPrivateKey(parentKey), 32);
	}else{
		// Public key derivation
		// Can be public or private
		if (type == CB_HD_KEY_TYPE_UNKNOWN) {
			CBLogError("Attempting to derive a key from an unknown key.");
			return false;
		}
		// Add the public key
		memcpy(inputData, CBHDKeyGetPublicKey(parentKey), 33);
	}
	// Get HMAC-SHA512 hash
	CBHDKeyHmacSha512(inputData, parentKey->chainCode, hash);
	// Copy over chain code, childID and version bytes
	memcpy(childKey->chainCode, hash + 32, 32);
	childKey->childID = childID;
	childKey->versionBytes = parentKey->versionBytes;
	// Set fingerprint of parent
	memcpy(childKey->parentFingerprint, CBHDKeyGetHash(parentKey), 4);
	// Set depth
	childKey->depth = parentKey->depth + 1;
	// Calculate key
	if (type == CB_HD_KEY_TYPE_PRIVATE) {
		// Calculating the private key
		// Add private key to first 32 bytes and modulo the order the curve
		// Split into four 64bit integers and add each one
		bool overflow = 0;
		for (uint8_t x = 4; x--;) {
			uint64_t a = CBArrayToInt64BigEndian(hash, 8*x);
			uint64_t b = CBArrayToInt64BigEndian(CBHDKeyGetPrivateKey(parentKey), 8*x) + overflow;
			uint64_t c = a + b;
			overflow = (c < b)? 1 : 0;
			CBInt64ToArrayBigEndian(CBHDKeyGetPrivateKey(childKey), 8*x, c);
		}
		if (overflow || memcmp(CBHDKeyGetPrivateKey(childKey), CB_CURVE_ORDER, 32) > 0) {
			// Take away CB_CURVE_ORDER
			bool carry = 0;
			for (uint8_t x = 4; x--;) {
				uint64_t a = CBArrayToInt64BigEndian(CBHDKeyGetPrivateKey(childKey), 8*x);
				uint64_t b = CBArrayToInt64BigEndian(CB_CURVE_ORDER, 8*x) + carry;
				carry = a < b;
				a -= b;
				CBInt64ToArrayBigEndian(CBHDKeyGetPrivateKey(childKey), 8*x, a);
			}
		}
		// Derive public key from private key
		CBKeyGetPublicKey(childKey->keyPair->privkey, CBHDKeyGetPublicKey(childKey));
		// Do not bother checking validity???
	}else{
		// Calculating the public key
		// Multiply the first 32 bytes by the base point (derive public key) and add to the public key point.
		CBKeyGetPublicKey(hash, CBHDKeyGetPublicKey(childKey));
		CBAddPoints(CBHDKeyGetPublicKey(childKey), CBHDKeyGetPublicKey(parentKey));
	}
	return true;
}
bool CBHDKeyGenerateMaster(CBHDKey * key, bool production){
	key->versionBytes = production ? CB_HD_KEY_VERSION_PROD_PRIVATE : CB_HD_KEY_VERSION_TEST_PRIVATE;
	key->childID.priv = false; // It is private but the BIP specifies 0 for the child number.
	key->childID.childNumber = 0;
	key->depth = 0;
	memset(key->parentFingerprint, 0, 4);
	if (!CBKeyPairGenerate(key->keyPair)) {
		CBLogError("Could not generate the master key");
		return false;
	}
	// Make chain code by hashing private key
	CBSha256(CBHDKeyGetPrivateKey(key), 32, key->chainCode);
	return true;
}
uint32_t CBHDKeyGetChildNumber(CBHDKeyChildID childID){
	return (childID.priv << 31) | childID.childNumber;
}
uint8_t * CBHDKeyGetHash(CBHDKey * key){
	return CBKeyPairGetHash(key->keyPair);
}
CBNetwork CBHDKeyGetNetwork(CBHDKeyVersion versionBytes){
	if (versionBytes == CB_HD_KEY_VERSION_PROD_PUBLIC
		|| versionBytes == CB_HD_KEY_VERSION_PROD_PRIVATE)
		return CB_NETWORK_PRODUCTION;
	if (versionBytes == CB_HD_KEY_VERSION_TEST_PUBLIC
		|| versionBytes == CB_HD_KEY_VERSION_TEST_PRIVATE)
		return CB_NETWORK_TEST;
	return CB_NETWORK_UNKNOWN;
}
uint8_t * CBHDKeyGetPrivateKey(CBHDKey * key){
	return key->keyPair->privkey;
}
uint8_t * CBHDKeyGetPublicKey(CBHDKey * key){
	return key->keyPair->pubkey.key;
}
CBHDKeyType CBHDKeyGetType(CBHDKeyVersion versionBytes){
	if (versionBytes == CB_HD_KEY_VERSION_PROD_PUBLIC
		|| versionBytes == CB_HD_KEY_VERSION_TEST_PUBLIC)
		return CB_HD_KEY_TYPE_PUBLIC;
	if (versionBytes == CB_HD_KEY_VERSION_PROD_PRIVATE
		|| versionBytes == CB_HD_KEY_VERSION_TEST_PRIVATE)
		return CB_HD_KEY_TYPE_PRIVATE;
	return CB_HD_KEY_TYPE_UNKNOWN;
}
CBWIF * CBHDKeyGetWIF(CBHDKey * key){
	CBNetwork network = CBHDKeyGetNetwork(key->versionBytes);
	if (network == CB_NETWORK_UNKNOWN)
		return NULL;
	if (CBHDKeyGetType(key->versionBytes) != CB_HD_KEY_TYPE_PRIVATE)
		return NULL;
	CBBase58Prefix prefix = network == CB_NETWORK_PRODUCTION ? CB_PREFIX_PRODUCTION_PRIVATE_KEY : CB_PREFIX_TEST_PRIVATE_KEY;
	return CBNewWIFFromPrivateKey(CBHDKeyGetPrivateKey(key), true, prefix, false);
}
void CBHDKeyHmacSha512(uint8_t * inputData, uint8_t * chainCode, uint8_t * output){
	uint8_t msg[192], inner[32], outer[32], hash[64]; // SHA512 has block size of 1024 bits or 128 bytes
	memcpy(msg, chainCode, 32);
	for (uint8_t x = 0; x < 32; x++) {
		inner[x] = msg[x] ^ 0x36;
		outer[x] = msg[x] ^ 0x5c;
	}
	memcpy(msg, inner, 32);
	memset(msg + 32, 0x36, 96);
	memcpy(msg + 128, inputData, 37);
	CBSha512(msg, 165, hash);
	memcpy(msg + 128, hash, 64);
	memcpy(msg, outer, 32);
	memset(msg + 32, 0x5c, 96);
	CBSha512(msg, 192, output);
}
void CBHDKeySerialise(CBHDKey * key, uint8_t * data){
	CBInt32ToArrayBigEndian(data, 0, key->versionBytes);
	if (key->depth == 0)
		// Master
		memset(data + 4, 0, 9);
	else{
		// Not master
		data[4] = key->depth;
		memcpy(data + 5, key->parentFingerprint, 4);
		// Child number
		CBInt32ToArrayBigEndian(data, 9, CBHDKeyGetChildNumber(key->childID));
	}
	// Chain code
	memcpy(data + 13, key->chainCode, 32);
	if (CBHDKeyGetType(key->versionBytes) == CB_HD_KEY_TYPE_PRIVATE) {
		// Private
		data[45] = 0;
		memcpy(data + 46, CBHDKeyGetPrivateKey(key), 32);
	}else
		// Assume public
		memcpy(data + 45, CBHDKeyGetPublicKey(key), 33);
}
bool CBKeyPairGenerate(CBKeyPair * keyPair){
	// Generate private key from a CSPRNG.
	if (!CBGet32RandomBytes(keyPair->privkey)) {
		CBLogError("Could not generate private key from 32 random bytes");
		return false;
	}
	// Get public key
	CBKeyGetPublicKey(keyPair->privkey, keyPair->pubkey.key);
	return true;
}
uint8_t * CBKeyPairGetHash(CBKeyPair * key){
	if (!key->pubkey.hashSet) {
		uint8_t hash[32];
		CBSha256(key->pubkey.key, 33, hash);
		CBRipemd160(hash, 32, key->pubkey.hash);
	}
	return key->pubkey.hash;
}
