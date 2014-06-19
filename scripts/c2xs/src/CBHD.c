#include <stdio.h>
#include <ctype.h>
#include <openssl/ssl.h>
#include <openssl/ripemd.h>
#include <openssl/rand.h>
#include <CBHDKeys.h>
#include <CBChecksumBytes.h>
#include <CBAddress.h>
#include <CBWIF.h>
#include <CBByteArray.h>
#include <CBBase58.h>

CBHDKey* importDataToCBHDKey(char* privstring) {
	CBByteArray * masterString = CBNewByteArrayFromString(privstring, true);
	CBChecksumBytes * masterData = CBNewChecksumBytesFromString(masterString, false);
	CBReleaseObject(masterString);
	CBHDKey * masterkey = CBNewHDKeyFromData(CBByteArrayGetData(CBGetByteArray(masterData)));
	CBReleaseObject(masterData);
	return (CBHDKey *)masterkey;
}
//////////////////////// perl export functions /////////////

char* newMasterKey(int arg){
	CBHDKey * masterkey = CBNewHDKey(true);
	CBHDKeyGenerateMaster(masterkey,true);

	uint8_t * keyData = malloc(CB_HD_KEY_STR_SIZE);
	CBHDKeySerialise(masterkey, keyData);
	free(masterkey);
	CBChecksumBytes * checksumBytes = CBNewChecksumBytesFromBytes(keyData, 82, false);
	// need to figure out how to free keyData memory
	CBByteArray * str = CBChecksumBytesGetString(checksumBytes);
	CBReleaseObject(checksumBytes);
	return (char *)CBByteArrayGetData(str);
}

char* deriveChildPrivate(char* privstring,bool hard,int child){
	CBHDKey* masterkey = importDataToCBHDKey(privstring);

	// generate child key
	CBHDKey * childkey = CBNewHDKey(true);
	CBHDKeyChildID childID = { hard, child};
	CBHDKeyDeriveChild(masterkey, childID, childkey);
	free(masterkey);

	uint8_t * keyData = malloc(CB_HD_KEY_STR_SIZE);
	CBHDKeySerialise(childkey, keyData);
	free(childkey);

	CBChecksumBytes * checksumBytes = CBNewChecksumBytesFromBytes(keyData, 82, false);
	// need to figure out how to free keyData memory
	CBByteArray * str = CBChecksumBytesGetString(checksumBytes);
	CBReleaseObject(checksumBytes);
	return (char *)CBByteArrayGetData(str);
}

char* exportWIFFromCBHDKey(char* privstring){
	CBHDKey* cbkey = importDataToCBHDKey(privstring);
	CBWIF * wif = CBHDKeyGetWIF(cbkey);
	free(cbkey);
	CBByteArray * str = CBChecksumBytesGetString(wif);
	CBFreeWIF(wif);
	return (char *)CBByteArrayGetData(str);
}


char* exportAddressFromCBHDKey(char* privstring){
	CBHDKey* cbkey = importDataToCBHDKey(privstring);
	CBAddress * address = CBNewAddressFromRIPEMD160Hash(CBHDKeyGetHash(cbkey), CB_PREFIX_PRODUCTION_ADDRESS, false);
	free(cbkey);
	CBByteArray * addressstring = CBChecksumBytesGetString(CBGetChecksumBytes(address));
	CBReleaseObject(address);
	return (char *)CBByteArrayGetData(addressstring);
}

char* newWIF(int arg){
	CBKeyPair * key = CBNewKeyPair(true);
	CBKeyPairGenerate(key);
	CBWIF * wif = CBNewWIFFromPrivateKey(key->privkey, true, CB_NETWORK_PRODUCTION, false);
	free(key);
	CBByteArray * str = CBChecksumBytesGetString(wif);
	CBFreeWIF(wif);
	return (char *)CBByteArrayGetData(str);
}


char* publickeyFromWIF(char* wifstring){
	CBByteArray * old = CBNewByteArrayFromString(wifstring,true);
	CBWIF * wif = CBNewWIFFromString(old, false);
	CBDestroyByteArray(old);
	uint8_t  privKey[32];
	CBWIFGetPrivateKey(wif,privKey);
	CBFreeWIF(wif);
	CBKeyPair * key = CBNewKeyPair(true);
	CBInitKeyPair(key);
	memcpy(key->privkey, privKey, 32);
	CBKeyGetPublicKey(key->privkey, key->pubkey.key);
	return (char *)CBByteArrayGetData(CBNewByteArrayWithDataCopy(key->pubkey.key,CB_PUBKEY_SIZE));

}

char* addressFromPublicKey(char* pubkey){
	CBByteArray * pubkeystring = CBNewByteArrayFromString(pubkey, false);
	//CBChecksumBytes * walletKeyData = CBNewChecksumBytesFromString(walletKeyString, false);
	//CBHDKey * cbkey = CBNewHDKeyFromData(CBByteArrayGetData(CBGetByteArray(walletKeyData)));


	//CBByteArray * old = CBNewByteArrayFromString(pubkey,false);

	CBKeyPair * key = CBNewKeyPair(false);
	memcpy(key->pubkey.key, CBByteArrayGetData(CBGetByteArray(pubkeystring)), CB_PUBKEY_SIZE);
	CBDestroyByteArray(pubkeystring);
	// this code came from CBKeyPairGetHash definition
	uint8_t hash[32];
	CBSha256(key->pubkey.key, 33, hash);
	CBRipemd160(hash, 32, key->pubkey.hash);

	CBAddress * address = CBNewAddressFromRIPEMD160Hash(key->pubkey.hash, CB_PREFIX_PRODUCTION_ADDRESS, true);
	free(key);
	CBByteArray * addressstring = CBChecksumBytesGetString(CBGetChecksumBytes(address));
	CBReleaseObject(address);

	return (char *)CBByteArrayGetData(addressstring);
}

char* createWIF(int arg){
	CBKeyPair * key = CBNewKeyPair(true);
	CBKeyPairGenerate(key);
	CBWIF * wif = CBNewWIFFromPrivateKey(key->privkey, true, CB_NETWORK_PRODUCTION, false);
	CBByteArray * str = CBChecksumBytesGetString(wif);
	CBReleaseObject(wif);
	//return (char *)CBByteArrayGetData(str);
	CBReleaseObject(str);
	CBAddress * address = CBNewAddressFromRIPEMD160Hash(CBKeyPairGetHash(key), CB_PREFIX_PRODUCTION_ADDRESS, false);
	CBByteArray * string = CBChecksumBytesGetString(CBGetChecksumBytes(address));
	return (char *)CBByteArrayGetData(string);
	//CBReleaseObject(key);
	//CBReleaseObject(address);
}




