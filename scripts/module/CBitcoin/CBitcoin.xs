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


#include "ppport.h"


MODULE = CBitcoin		PACKAGE = CBitcoin		

void
hello()
CODE:
    printf("Hello, world!\n");

int
is_even(input)
    int input
CODE:
    RETVAL = (input % 2 == 0);
OUTPUT:
    RETVAL

int
newMasterKey(arg)
    int arg
CODE:
    CBHDKey * masterkey = CBNewHDKey(true);
    CBHDKeyGenerateMaster(masterkey,true);

    uint8_t * keyData = malloc(CB_HD_KEY_STR_SIZE);
    CBHDKeySerialise(masterkey, keyData);
    free(masterkey);
    CBChecksumBytes * checksumBytes = CBNewChecksumBytesFromBytes(keyData, 82, false);
    // need to figure out how to free keyData memory
    CBByteArray * str = CBChecksumBytesGetString(checksumBytes);
    CBReleaseObject(checksumBytes);
    #RETVAL = (char *)CBByteArrayGetData(str);
    RETVAL = 1;
OUTPUT:
    RETVAL
