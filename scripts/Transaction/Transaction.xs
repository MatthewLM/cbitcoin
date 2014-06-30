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
#include <CBTransaction.h>
#include <CBTransactionInput.h>
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


CBTransactionOutput* serializeddata_to_obj(char* datastring){

	CBByteArray* data = hexstring_to_bytearray(datastring);

	CBTransaction* tx = CBNewTransactionFromData(data);
	uint32_t dlen = CBTransactionDeserialise(tx);

	//CBDestroyByteArray(data);
	return tx;
}

char* obj_to_serializeddata(CBTransaction * tx){
	CBTransactionPrepareBytes(tx);
	int dlen = CBTransactionSerialise(tx,1);
	CBByteArray* serializeddata = CBGetMessage(tx)->bytes;

	char* answer = bytearray_to_hexstring(serializeddata,dlen);

	return answer;
}

/*
 * TransactionOutput related functions
 */
char* txoutput_obj_to_serializeddata(CBTransactionOutput * txoutput){
	CBTransactionOutputPrepareBytes(txoutput);
	int dlen = CBTransactionOutputSerialise(txoutput);
	CBByteArray* serializeddata = CBGetMessage(txoutput)->bytes;

	char* answer = bytearray_to_hexstring(serializeddata,dlen);

	return answer;
}
CBTransactionOutput* txoutput_serializeddata_to_obj(char* datastring){

	CBByteArray* data = hexstring_to_bytearray(datastring);

	CBTransactionOutput* txoutput = CBNewTransactionOutputFromData(data);
	int dlen = (int)CBTransactionOutputDeserialise(txoutput);

	//CBTransactionInputDeserialise(txinput);
	//CBDestroyByteArray(data);
	return txoutput;
}
/*
 * TransactionInput related functions
 */

CBTransactionInput* txinput_serializeddata_to_obj(char* datastring){

	CBByteArray* data = hexstring_to_bytearray(datastring);

	CBTransactionInput* txinput = CBNewTransactionInputFromData(data);
	int dlen = (int)CBTransactionInputDeserialise(txinput);

	//CBTransactionInputDeserialise(txinput);
	//CBDestroyByteArray(data);
	return txinput;
}

char* txinput_obj_to_serializeddata(CBTransactionInput * txinput){
	CBTransactionInputPrepareBytes(txinput);
	int dlen = CBTransactionInputSerialise(txinput);
	CBByteArray* serializeddata = CBGetMessage(txinput)->bytes;

	char* answer = bytearray_to_hexstring(serializeddata,dlen);

	return answer;
}

//////////////////////// perl export functions /////////////
//CBTransactionInput * CBNewTransactionInput(CBScript * script, uint32_t sequence, CBByteArray * prevOutHash, uint32_t prevOutIndex)

char* create_tx_obj(int lockTime, int version, SV* inputs, SV* outputs, int numOfInputs, int numOfOutputs){
	CBTransaction* tx = CBNewTransaction((uint32_t) lockTime, (uint32_t) version);

	int n;
	int in_length, out_length;
    if ((! SvROK(inputs))
    || (SvTYPE(SvRV(inputs)) != SVt_PVAV)
    || ((in_length = av_len((AV *)SvRV(inputs))) < 0))
    {
        return 0;
    }
    if ((! SvROK(outputs))
    || (SvTYPE(SvRV(outputs)) != SVt_PVAV)
    || ((out_length = av_len((AV *)SvRV(outputs))) < 0))
    {
        return 0;
    }

    // load TransactionInput
	for (n=0; n<=in_length; n++) {
		STRLEN l;

		char * fn = SvPV (*av_fetch ((AV *) SvRV (inputs), n, 0), l);
		CBTransactionInput * inx = txinput_serializeddata_to_obj(fn);
		CBTransactionAddInput(tx,inx);
	}
	for (n=0; n<=out_length; n++) {
		STRLEN l;

		char * fn = SvPV (*av_fetch ((AV *) SvRV (outputs), n, 0), l);
		CBTransactionOutput * outx = txoutput_serializeddata_to_obj(fn);
		CBTransactionAddOutput(tx,outx);
	}
	char* answer = obj_to_serializeddata(tx);
	//CBFreeTransaction(tx);
	return answer;
}
/*
char* get_script_from_obj(char* serializedDataString){
	CBTransactionOutput* txoutput = serializeddata_to_obj(serializedDataString);
	char* scriptstring = scriptToString(txoutput->scriptObject);
	//CBFreeTransactionOutput(txoutput);
	return scriptstring;
}
*/
int get_lockTime_from_obj(char* serializedDataString){
	CBTransaction* tx = serializeddata_to_obj(serializedDataString);
	uint32_t lockTime = tx->lockTime;
	CBFreeTransaction(tx);
	return (int)lockTime;
}

int get_version_from_obj(char* serializedDataString){
	CBTransaction* tx = serializeddata_to_obj(serializedDataString);
	uint32_t version = tx->version;
	CBFreeTransaction(tx);
	return (int)version;
}



MODULE = CBitcoin::Transaction	PACKAGE = CBitcoin::Transaction	

PROTOTYPES: DISABLE


char *
create_tx_obj (lockTime, version, inputs, outputs, numOfInputs, numOfOutputs)
	int	lockTime
	int	version
	SV *	inputs
	SV *	outputs
	int	numOfInputs
	int	numOfOutputs

int
get_lockTime_from_obj (serializedDataString)
	char *	serializedDataString

int
get_version_from_obj (serializedDataString)
	char *	serializedDataString

