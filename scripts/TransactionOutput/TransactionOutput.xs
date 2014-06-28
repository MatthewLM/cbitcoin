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
#include <CBTransactionOutput.h>


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
// CBScript * script = CBNewScriptFromString(scriptstring);

CBTransactionOutput* stringToTransactionOutput(char* scriptstring, int valueInt){

	CBScript* script = CBNewScriptFromString(scriptstring);

	CBTransactionOutput* answer = CBNewTransactionOutput((uint64_t) valueInt,script);
	//CBFreeScript(script);
	//CBDestroyByteArray(prevOutHash);
	return answer;
}

CBTransactionOutput* serializeddata_to_obj(char* datastring){

	CBByteArray* data = hexstring_to_bytearray(datastring);

	CBTransactionOutput* txoutput = CBNewTransactionOutputFromData(data);
	int dlen = (int)CBTransactionOutputDeserialise(txoutput);

	//CBTransactionInputDeserialise(txinput);
	//CBDestroyByteArray(data);
	return txoutput;
}

char* obj_to_serializeddata(CBTransactionOutput * txoutput){
	CBTransactionOutputPrepareBytes(txoutput);
	int dlen = CBTransactionOutputSerialise(txoutput);
	CBByteArray* serializeddata = CBGetMessage(txoutput)->bytes;

	char* answer = bytearray_to_hexstring(serializeddata,dlen);

	return answer;
}



//////////////////////// perl export functions /////////////
//CBTransactionInput * CBNewTransactionInput(CBScript * script, uint32_t sequence, CBByteArray * prevOutHash, uint32_t prevOutIndex)
char* create_txoutput_obj(char* scriptstring, int valueInt){
	CBTransactionOutput* txoutput = stringToTransactionOutput(scriptstring,valueInt);
	char* answer = obj_to_serializeddata(txoutput);
	//CBFreeTransactionOutput(txoutput);
	return answer;
}

char* get_script_from_obj(char* serializedDataString){
	CBTransactionOutput* txoutput = serializeddata_to_obj(serializedDataString);
	char* scriptstring = scriptToString(txoutput->scriptObject);
	//CBFreeTransactionOutput(txoutput);
	return scriptstring;
}

int get_value_from_obj(char* serializedDataString){
	CBTransactionOutput* txoutput = serializeddata_to_obj(serializedDataString);
	uint64_t value = txoutput->value;
	CBFreeTransactionOutput(txoutput);
	return (int)value;
}





MODULE = CBitcoin::TransactionOutput	PACKAGE = CBitcoin::TransactionOutput	

PROTOTYPES: DISABLE


char *
create_txoutput_obj (scriptstring, valueInt)
	char *	scriptstring
	int	valueInt

char *
get_script_from_obj (serializedDataString)
	char *	serializedDataString

int
get_value_from_obj (serializedDataString)
	char *	serializedDataString

