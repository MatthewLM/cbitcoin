#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"

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
