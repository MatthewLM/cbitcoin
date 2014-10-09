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
#include <CBBlock.h>

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

CBBlock * CBNewBlockFromHexString(char * dataString) {
	CBByteArray* data = hexstring_to_bytearray(dataString);
	CBBlock * self = malloc(sizeof(*self));
	CBGetObject(self)->free = CBFreeBlock;
	CBInitBlockFromData(self, data);
	return self;
}



/*
CBTransactionInput* stringToTransactionInput(char* scriptstring, int sequenceInt, char* prevOutHashString, int prevOutIndexInt){
	// prevOutHash is stored as a hex
	CBByteArray* prevOutHashInReverse =  CBNewByteArrayFromHex(prevOutHashString);
	CBScript* script = CBNewScriptFromString(scriptstring);


	// need to reverse the prevOutHashString...
	CBByteArray* prevOutHash = CBByteArrayCopy(prevOutHashInReverse);
        CBByteArrayReverseBytes(prevOutHash);
	CBFreeByteArray(prevOutHashInReverse);

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

CBTransactionInput* serializeddata_to_obj(char* datastring){

	CBByteArray* data = hexstring_to_bytearray(datastring);

	CBTransactionInput* txinput = CBNewTransactionInputFromData(data);
	int dlen = (int)CBTransactionInputDeserialise(txinput);

	//CBTransactionInputDeserialise(txinput);
	//CBDestroyByteArray(data);
	return txinput;
}

char* obj_to_serializeddata(CBTransactionInput * txinput){
	CBTransactionInputPrepareBytes(txinput);
	int dlen = CBTransactionInputSerialise(txinput);
	CBByteArray* serializeddata = CBGetMessage(txinput)->bytes;

	char* answer = bytearray_to_hexstring(serializeddata,dlen);

	return answer;
}



//////////////////////// perl export functions /////////////
//CBTransactionInput * CBNewTransactionInput(CBScript * script, uint32_t sequence, CBByteArray * prevOutHash, uint32_t prevOutIndex)
char* create_txinput_obj(char* scriptstring, int sequenceInt, char* prevOutHashString, int prevOutIndexInt){
	CBTransactionInput* txinput = stringToTransactionInput(scriptstring,sequenceInt,prevOutHashString,prevOutIndexInt);
	char* answer = obj_to_serializeddata(txinput);
	//CBFreeTransactionInput(txinput);
	return answer;
}

char* get_script_from_obj(char* serializedDataString){
	CBTransactionInput* txinput = serializeddata_to_obj(serializedDataString);
	char* scriptstring = scriptToString(txinput->scriptObject);
	//CBFreeTransactionInput(txinput);
	return scriptstring;
}
char* get_prevOutHash_from_obj(char* serializedDataString){
	CBTransactionInput* txinput = serializeddata_to_obj(serializedDataString);
	CBByteArray* dataInReverse = txinput->prevOut.hash;
	CBByteArray* data = CBByteArrayCopy(dataInReverse);
	CBByteArrayReverseBytes(data);
	char * answer = bytearray_to_hexstring(data,data->length);
	return answer;
}
int get_prevOutIndex_from_obj(char* serializedDataString){
	CBTransactionInput* txinput = serializeddata_to_obj(serializedDataString);
	uint32_t index = txinput->prevOut.index;
	CBFreeTransactionInput(txinput);
	return (int)index;
}
int get_sequence_from_obj(char* serializedDataString){
	CBTransactionInput* txinput = serializeddata_to_obj(serializedDataString);
	uint32_t sequence = txinput->sequence;
	CBFreeTransactionInput(txinput);
	return (int)sequence;
}

*/

