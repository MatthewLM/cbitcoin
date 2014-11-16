//
//  base58Converter.c
//  cbitcoin
//
//  Created by Matthew Mitchell on 21/01/2014.
//  Copyright (c) 2014 Matthew Mitchell
//
//  This file is part of cbitcoin. It is subject to the license terms
//  in the LICENSE file found in the top-level directory of this
//  distribution and at http://www.cbitcoin.com/license.html. No part of
//  cbitcoin, including this file, may be copied, modified, propagated,
//  or distributed except according to the terms contained in the
//  LICENSE file.

#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include "CBByteArray.h"
#include "CBBase58.h"

int main(int argc, char * argv[]){

	bool encode = strcmp(argv[1],"-d");

	// Read comma sperated inputs from the second argument
	char * inputs[100], * comma;
	inputs[0] = argv[2];

	int num = 1;
	for (; (comma = strchr(argv[2], ',')); num++) {
		inputs[num] = comma + 1;
		*comma = '\0';
		argv[2] = comma + 1;
	}

	for (int x = 0; x < num; x++) {
		if (encode) {

			// Convert hex string into bytes and then encode base58 string
			CBByteArray * bytes = CBNewByteArrayFromHex(inputs[x]);
			CBByteArrayReverseBytes(bytes);
			CBBigInt bi = {CBByteArrayGetData(bytes), bytes->length, bytes->length};
			char * output = CBEncodeBase58(&bi);
			puts(output);
			free(output);
			CBReleaseObject(bytes);

		}else{

			// Decode base58 string and then produce data as a hex string.
			CBBigInt bi;
			CBBigIntAlloc(&bi, strlen(inputs[x]) * 100 / 136);
			CBDecodeBase58(&bi, inputs[x]);
			printf("0x");
			for (int x = bi.length; x--;)
				printf("%02x", bi.data[x]);
			puts("");
			free(bi.data);

		}
	}

}
