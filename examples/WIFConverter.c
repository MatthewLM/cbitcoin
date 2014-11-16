//
//  WIFConverter.c
//  cbitcoin
//
//  Created by Matthew Mitchell on 16/02/2014.
//  Copyright (c) 2014 Matthew Mitchell
//
//  This file is part of cbitcoin. It is subject to the license terms
//  in the LICENSE file found in the top-level directory of this
//  distribution and at http://www.cbitcoin.com/license.html. No part of
//  cbitcoin, including this file, may be copied, modified, propagated,
//  or distributed except according to the terms contained in the
//  LICENSE file.

#include "CBWIF.h"

int main(int argc, char * argv[]){

	CBWIF wif;

	// Decode old data
	CBByteArray old;
	CBInitByteArrayFromString(&old, argv[1], false);
	CBInitWIFFromString(&wif, &old, false);
	CBDestroyByteArray(&old);
	bool useCompression = CBWIFUseCompression(&wif);

	// Get key
	int key[32];
	CBWIFGetPrivateKey(&wif, key);
	CBDestroyWIF(&wif);

	// Get the new WIF with the prefix we want
	CBInitWIFFromPrivateKey(&wif, key, useCompression, argc == 3 ? atoi(argv[2]): 0, false);
	CBByteArray * new = CBChecksumBytesGetString(CBGetChecksumBytes(&wif));
	puts((char *)CBByteArrayGetData(new));
	CBReleaseObject(new);
	CBDestroyWIF(&wif);

}
