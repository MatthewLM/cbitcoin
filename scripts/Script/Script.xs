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
#include <CBScript.h>

//bool CBInitScriptFromString(CBScript * self, char * string)
char* scriptToString(CBScript* script){
	char* answer = (char *)malloc(CBScriptStringMaxSize(script)*sizeof(char));
	CBScriptToString(script, answer);
	return answer;

}

CBScript* stringToScript(char* scriptstring){
	CBScript* self;
	if(CBInitScriptFromString(self,scriptstring)){
		return self;
	}
	else{
		return false;
	}
}


//////////////////////// perl export functions /////////////



char* addressToScript(char* addressString){
    CBByteArray * addrStr = CBNewByteArrayFromString(addressString, true);
    CBAddress * addr = CBNewAddressFromString(addrStr, false);

    CBScript * script = CBNewScriptPubKeyHashOutput(CBByteArrayGetData(CBGetByteArray(addr)) + 1);

    char* answer = (char *)malloc(CBScriptStringMaxSize(script)*sizeof(char));
    CBScriptToString(script, answer);
    CBFreeScript(script);
    //printf("Script = %s\n", answer);

    return answer;
}

// CBScript * CBNewScriptPubKeyOutput(uint8_t * pubKey);
char* pubkeyToScript (char* pubKeystring){
	// convert to uint8_t *
	CBByteArray * masterString = CBNewByteArrayFromString(pubKeystring, true);
	CBScript * script = CBNewScriptPubKeyOutput(CBByteArrayGetData(masterString));
	CBReleaseObject(masterString);

	return scriptToString(script);
}

//http://stackoverflow.com/questions/1503763/how-can-i-pass-an-array-to-a-c-function-in-perl-xs#1505355
//CBNewScriptMultisigOutput(uint8_t ** pubKeys, uint8_t m, uint8_t n);
//char* multisigToScript (char** multisigConcatenated,)
char* multisigToScript(SV* pubKeyArray,int mKeysInt, int nKeysInt) {
	uint8_t mKeys, nKeys;
	mKeys = (uint8_t)mKeysInt;
	nKeys = (uint8_t)nKeysInt;

	printf( "1 - m %d\n", mKeysInt );
	int n;
	I32 length = 0;
    if ((! SvROK(pubKeyArray))
    || (SvTYPE(SvRV(pubKeyArray)) != SVt_PVAV)
    || ((length = av_len((AV *)SvRV(pubKeyArray))) < 0))
    {
        return 0;
    }
    printf( "2 - n %d\n", nKeysInt );
    /* Create the array which holds the return values. */
	uint8_t** multisig = (uint8_t**)malloc(nKeysInt * sizeof(uint8_t*));
	printf( "3 - length %d\n", length );
	for (n=0; n<=length; n++) {
		STRLEN l;
		printf( "Inside Spot Up %d\n", n );
		char * fn = SvPV (*av_fetch ((AV *) SvRV (pubKeyArray), n, 0), l);
		printf("String Inside: %s", fn);
		printf( "Inside Spot Middle 1 %d\n", n );
		CBByteArray * masterString = CBNewByteArrayFromString(fn, true);
		printf( "Inside Spot Middle 2 %d\n", n );
		// this line should just assign a uint8_t * pointer
		multisig[n] = CBByteArrayGetData(masterString);
		printf( "Inside Spot Middle 3 %d\n", n );
		CBReleaseObject(masterString);

	}
	CBScript* finalscript =  CBNewScriptMultisigOutput(multisig,mKeys,nKeys);
	printf( "Spot %d\n", 10000000 );
	return scriptToString(finalscript);
}

MODULE = CBitcoin::Script	PACKAGE = CBitcoin::Script	

PROTOTYPES: DISABLE


char *
addressToScript (addressString)
	char *	addressString

char *
pubkeyToScript (pubKeystring)
	char *	pubKeystring

char *
multisigToScript (pubKeyArray, mKeysInt, nKeysInt)
	SV *	pubKeyArray
	int	mKeysInt
	int	nKeysInt

