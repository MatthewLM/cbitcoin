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

	return (CBTransactionInput*)CBNewTransactionInput(
			CBNewScriptFromString(scriptstring),
			(uint32_t)sequenceInt,
			prevOutHash,
			(uint32_t)prevOutIndexInt
		);
}


//////////////////////// perl export functions /////////////
//CBTransactionInput * CBNewTransactionInput(CBScript * script, uint32_t sequence, CBByteArray * prevOutHash, uint32_t prevOutIndex)



