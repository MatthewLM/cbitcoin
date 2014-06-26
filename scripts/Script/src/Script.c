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
/* Return 1 if this script is multisig, 0 for else*/
int whatTypeOfScript(char* typearg,char* scriptstring){
	CBScript * script;
	if(!CBInitScriptFromString(script,scriptstring)){
		return 0;
	}
	if(strcmp(typearg,"multisig") == 0){
		if(CBScriptIsMultisig(script)){
			return 1;
		}
		else{
			return 0;
		}
	}
	else if(strcmp(typearg,"p2sh") == 0){
		if(CBScriptIsP2SH(script)){
			return 1;
		}
		else{
			return 0;
		}
	}
	else if(strcmp(typearg,"pubkey") == 0){
		if(CBScriptIsPubkey(script)){
			return 1;
		}
		else{
			return 0;
		}
	}
	else if(strcmp(typearg,"keyhash") == 0){
		if(CBScriptIsKeyHash(script)){
			return 1;
		}
		else{
			return 0;
		}
	}
	return 0;
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

		CBByteArray * masterString = CBNewByteArrayFromString(fn, true);

		// this line should just assign a uint8_t * pointer
		multisig[n] = CBByteArrayGetData(masterString);

		CBReleaseObject(masterString);

	}
	CBScript* finalscript =  CBNewScriptMultisigOutput(multisig,mKeys,nKeys);

	return scriptToString(finalscript);
}
