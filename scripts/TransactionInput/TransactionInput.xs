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

CBTransactionInput* serializeddata_to_obj(char* datastring){
	uint32_t length = (uint32_t)sizeof(datastring);

	/*CBByteArray* data = CBNewByteArrayOfSize(length);
	for(uint32_t i=0;i<length;i++){
		CBByteArraySetByte(data,i,(uint8_t)datastring[i]);
	}*/
	//CBByteArray* data = CBNewByteArrayWithData(datastring,length);
	CBByteArray* data = CBNewByteArrayFromHex(datastring);

	CBTransactionInput* txinput = CBNewTransactionInputFromData(data);
	int dlen = (int)CBTransactionInputDeserialise(txinput);
	printf( "Part 2(length=%d;%d): Sequence:%d and prevOutIndex:%d\n",length, dlen,txinput->sequence, txinput->prevOut.index );
	//CBTransactionInputDeserialise(txinput);
	//CBDestroyByteArray(data);
	return txinput;
}

char* obj_to_serializeddata(CBTransactionInput * txinput){
	CBTransactionInputPrepareBytes(txinput);
	CBTransactionInputSerialise(txinput);
	CBByteArray* serializeddata = CBGetMessage(txinput)->bytes;

	// style 1
	CBTransactionInput* newtxinput = CBNewTransactionInputFromData(serializeddata);
	int dlen = (int)CBTransactionInputDeserialise(newtxinput);
	// style 3


	//CBTransactionInput* newtxinphut = serializeddata_to_obj((char *)CBByteArrayGetData(serializeddata));
	//uint8_t* answer = CBByteArrayGetData(serializeddata);

	char* answer = malloc(dlen*sizeof(char*));
	CBByteArrayToString(serializeddata, 0, dlen, answer, 0);


	printf( "Part 1 (length=%s;%d) Sequence:%d and prevOutIndex:%d\n", answer,dlen,newtxinput->sequence, newtxinput->prevOut.index );
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
	CBByteArray* data = txinput->prevOut.hash;
	char * answer = (char*)CBByteArrayGetData(data);
	//CBFreeTransactionInput(txinput);
	//CBDestroyByteArray(data);
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




MODULE = CBitcoin::TransactionInput	PACKAGE = CBitcoin::TransactionInput	

PROTOTYPES: DISABLE


char *
create_txinput_obj (scriptstring, sequenceInt, prevOutHashString, prevOutIndexInt)
	char *	scriptstring
	int	sequenceInt
	char *	prevOutHashString
	int	prevOutIndexInt

char *
get_script_from_obj (serializedDataString)
	char *	serializedDataString

char *
get_prevOutHash_from_obj (serializedDataString)
	char *	serializedDataString

int
get_prevOutIndex_from_obj (serializedDataString)
	char *	serializedDataString

int
get_sequence_from_obj (serializedDataString)
	char *	serializedDataString

