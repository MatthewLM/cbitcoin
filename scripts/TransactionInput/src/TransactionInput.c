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
#include <CBTransactionInput.h>

CBHDKey* importDataToCBHDKey(char* privstring) {
	CBByteArray * masterString = CBNewByteArrayFromString(privstring, true);
	CBChecksumBytes * masterData = CBNewChecksumBytesFromString(masterString, false);
	CBReleaseObject(masterString);
	CBHDKey * masterkey = CBNewHDKeyFromData(CBByteArrayGetData(CBGetByteArray(masterData)));
	CBReleaseObject(masterData);
	return (CBHDKey *)masterkey;
}
//////////////////////// perl export functions /////////////

char* createTxInput(char* prevOutHashstring, int prevOutIndexstring){
	CBByteArray * prevOutHash = CBNewByteArrayFromString(prevOutHashstring, true);
	CBTransactionInput * txinput;
	//CBNewTransactionInput(CBScript * script, uint32_t sequence, CBByteArray * prevOutHash, uint32_t prevOutIndex);

}





