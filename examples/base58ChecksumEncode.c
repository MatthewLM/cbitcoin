//
//  base58ChecksumEncode.c
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

#include "CBChecksumBytes.h"

int main(int argc, char * argv[]){
	CBChecksumBytes * csb = CBNewChecksumBytesFromHex(argv[1], false);
	puts((char *)CBByteArrayGetData(CBChecksumBytesGetString(csb)));
	CBReleaseObject(csb);
}
