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


//////////////////////// perl export functions /////////////
int addressToScript(char* addressString){
	CBAddress * address = CBNewAddressFromString(CBNewByteArrayFromString(addressString, true), false);
	// extract script from address, get uint8_t * 20 byte ripemd 160 bit hash (equal to address without checksum bytes)
	uint8_t* addrraw = CBByteArrayGetData(CBGetByteArray(address));
	CBFreeAddress(address);


	CBScript * self;
	CBInitScriptPubKeyHashOutput(self,addrraw);
	return 1;
/*
	char* output;
	CBScriptToString(self,output);
	return output;*/
}






MODULE = CBitcoin::Script	PACKAGE = CBitcoin::Script	

PROTOTYPES: DISABLE


int
addressToScript (addressString)
	char *	addressString

