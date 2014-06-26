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
	CBFreeScript(script);
	CBDestroyByteArray(prevOutHash);
	return txinput;
}

char* obj_to_serializeddata(CBTransactionInput * txinput){
	CBTransactionInputPrepareBytes(txinput);
	CBTransactionInputSerialise(txinput);
	CBByteArray* serializeddata = CBGetMessage(txinput)->bytes;
	return (char *)CBByteArrayGetData(serializeddata);
}
CBTransactionInput* serializeddata_to_obj(char* datastring){
	CBByteArray* data = CBNewByteArrayFromString(datastring,true);
	CBTransactionInput* txinput = CBNewTransactionInputFromData(data);
	CBDestroyByteArray(data);
	return txinput;
}

//////////////////////// perl export functions /////////////
//CBTransactionInput * CBNewTransactionInput(CBScript * script, uint32_t sequence, CBByteArray * prevOutHash, uint32_t prevOutIndex)
char* create_txinput_obj(char* scriptstring, int sequenceInt, char* prevOutHashString, int prevOutIndexInt){
	CBTransactionInput* txinput = stringToTransactionInput(scriptstring,sequenceInt,prevOutHashString,prevOutIndexInt);
	char* answer = obj_to_serializeddata(txinput);
	CBFreeTransactionInput(txinput);
	return answer;
}

char* get_script_from_obj(char* serializedDataString){
	CBTransactionInput* txinput = serializeddata_to_obj(serializedDataString);
	char* scriptstring = scriptToString(txinput->scriptObject);
	CBFreeTransactionInput(txinput);
	return scriptstring;
}



