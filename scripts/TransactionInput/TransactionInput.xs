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
#include <CBTransactionInput.h>

//bool CBInitScriptFromString(CBScript * self, char * string)
char* scriptToString(CBScript* script){
	char* answer = (char *)malloc(CBScriptStringMaxSize(script)*sizeof(char));
	CBScriptToString(script, answer);
	return answer;

}
// CBScript * script = CBNewScriptFromString(scriptstring);

CBTransactionInput* stringToTransactionInput(char* scriptstring, int sequenceInt, char* prevOutHashString, int prevOutIndexInt){
	// prevOutHash is stored as a hex
	CBByteArray* prevOutHash =  CBNewByteArrayFromHex(prevOutHashString);
	CBScript* script = CBNewScriptFromString(scriptstring);


	CBTransactionInput* txinput = CBNewTransactionInput(
				script,
				(uint32_t)sequenceInt,
				prevOutHash,
				(uint32_t)prevOutIndexInt
			);
	//CBFreeScript(script);
	//CBDestroyByteArray(prevOutHash);
	return txinput;
}

char* serializeByteData(CBTransactionInput * txinput){
	CBTransactionInputPrepareBytes(txinput);
	CBTransactionInputSerialise(txinput);
	CBByteArray* serializeddata = CBGetMessage(txinput)->bytes;
	return (char *)CBByteArrayGetData(serializeddata);
}

//////////////////////// perl export functions /////////////
//CBTransactionInput * CBNewTransactionInput(CBScript * script, uint32_t sequence, CBByteArray * prevOutHash, uint32_t prevOutIndex)
char* create_txinput_obj(char* scriptstring, int sequenceInt, char* prevOutHashString, int prevOutIndexInt){
	CBTransactionInput* txinput = stringToTransactionInput(scriptstring,sequenceInt,prevOutHashString,prevOutIndexInt);
	char* answer = serializeByteData(txinput);
	CBFreeTransactionInput(txinput);
	return answer;
}



MODULE = CBitcoin::TransactionInput	PACKAGE = CBitcoin::TransactionInput	

PROTOTYPES: DISABLE


char *
create_txinput_obj (scriptstring, sequenceInt, prevOutHashString, prevOutIndexInt)
	char *	scriptstring
	int	sequenceInt
	char *	prevOutHashString
	int	prevOutIndexInt

