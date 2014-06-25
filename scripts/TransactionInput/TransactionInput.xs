#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"

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

char* deriveChildPrivate(char* privstring,bool private,int child){
	CBHDKey* masterkey = importDataToCBHDKey(privstring);

	// generate child key
	CBHDKey * childkey = CBNewHDKey(true);
	CBHDKeyChildID childID = { private, child};
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
char* exportPublicKeyFromCBHDKey(char* privstring){
	CBHDKey* cbkey = importDataToCBHDKey(privstring);
	uint8_t* pubkey = CBHDKeyGetPublicKey(cbkey);
	free(cbkey);
	return (char*) pubkey;
}






MODULE = CBitcoin::TransactionInput	PACKAGE = CBitcoin::TransactionInput	

PROTOTYPES: DISABLE


char *
newMasterKey (arg)
	int	arg

char *
deriveChildPrivate (privstring, private, child)
	char *	privstring
	bool	private
	int	child

char *
exportWIFFromCBHDKey (privstring)
	char *	privstring

char *
exportAddressFromCBHDKey (privstring)
	char *	privstring

char *
exportPublicKeyFromCBHDKey (privstring)
	char *	privstring

