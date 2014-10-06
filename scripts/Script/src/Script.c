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

// print CBByteArray to hex string
char* bytearray_to_hexstring(CBByteArray * serializeddata,uint32_t dlen){
	char* answer = malloc(dlen*sizeof(char*));
	CBByteArrayToString(serializeddata, 0, dlen, answer, 0);
	return answer;
}
CBByteArray* hexstring_to_bytearray(char* hexstring){
	CBByteArray* answer = CBNewByteArrayFromHex(hexstring);
	return answer;
}


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
		return NULL;
	}
}


//////////////////////// perl export functions /////////////


// 20 byte hex string (Hash160) to address
char* newAddressFromRIPEMD160Hash(char* hexstring){
	CBByteArray* array = hexstring_to_bytearray(hexstring);
	CBAddress * address = CBNewAddressFromRIPEMD160Hash(CBByteArrayGetData(array),CB_PREFIX_PRODUCTION_ADDRESS, true);
	CBByteArray * addressstring = CBChecksumBytesGetString(CBGetChecksumBytes(address));
	CBReleaseObject(address);
	return (char *)CBByteArrayGetData(addressstring);
}




/* Return 1 if this script is multisig, 0 for else*/
// this function does not work
char* whatTypeOfScript(char* scriptstring){
	CBScript * script = CBNewScriptFromString(scriptstring);
	if(script == NULL){
		return "NULL";
	}
	if(CBScriptIsMultisig(script)){
		return "multisig";
	}
	else if(CBScriptIsP2SH(script)){
		return "p2sh";
	}
	else if(CBScriptIsPubkey(script)){
		return "pubkey";
	}
	else if(CBScriptIsKeyHash(script)){
		return "keyhash";
	}
	else{
		return "FAILED";
	}

}



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

	int n;
	I32 length = 0;
    if ((! SvROK(pubKeyArray))
    || (SvTYPE(SvRV(pubKeyArray)) != SVt_PVAV)
    || ((length = av_len((AV *)SvRV(pubKeyArray))) < 0))
    {
        return 0;
    }
    /* Create the array which holds the return values. */
	uint8_t** multisig = (uint8_t**)malloc(nKeysInt * sizeof(uint8_t*));

	for (n=0; n<=length; n++) {
		STRLEN l;

		char * fn = SvPV (*av_fetch ((AV *) SvRV (pubKeyArray), n, 0), l);

		CBByteArray * masterString = hexstring_to_bytearray(fn);

		// this line should just assign a uint8_t * pointer
		multisig[n] = CBByteArrayGetData(masterString);

		//CBReleaseObject(masterString);

	}
	CBScript* finalscript =  CBNewScriptMultisigOutput(multisig,mKeys,nKeys);

	return scriptToString(finalscript);
}



