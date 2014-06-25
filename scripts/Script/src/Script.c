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
    CBByteArray * addrStr = CBNewByteArrayFromString(addressString, false);
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
char* multisigToScript(SV* pubKeyArray,uint8_t mKeys, uint8_t nKeys) {
	int i, n, length;
	uint8_t** multisig = (uint8_t**)malloc((int)nKeys * sizeof(uint8_t*));
	length = (int)av_len((AV *)SvRV(pubKeyArray));
	for (n=0; n<=length; i++) {
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
