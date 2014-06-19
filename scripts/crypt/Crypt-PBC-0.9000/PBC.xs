#include <pbc.h>

#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"

#include "ppport.h"

MODULE = Crypt::PBC		PACKAGE = Crypt::PBC		

PROTOTYPES: ENABLE

INCLUDE: pairing.xs
INCLUDE:   einit.xs
INCLUDE:  earith.xs
INCLUDE:   ecomp.xs

void
element_fprintf(stream,format,element)
    FILE * stream
    char * format
    element_t * element

    CODE:
    element_fprintf(stream, format, *element);

SV * 
export_element(element)
    element_t * element

    PREINIT:
    char *buf;
    int len;
    
    CODE:
    len = element_length_in_bytes(*element);
    buf = malloc( len + 2 );

    // My bug posted to the pbc-dev newsgroup, where I was getting different
    // results for different elements that test equal?  Yeah, the following
    // line was not present when I got that result.  I'm awesome. 11/15/06
    element_to_bytes(buf, *element);

    RETVAL = newSVpvn(buf, len);

    free(buf);

    OUTPUT:
    RETVAL

mpz_t *
element_to_mpz(element)
    element_t * element

    PREINIT:
    mpz_t * ret = malloc (sizeof(mpz_t));

    CODE:
    mpz_init(*ret);
    element_to_mpz(*ret, *element);
    RETVAL = ret;

    OUTPUT:
    RETVAL

int 
element_length_in_bytes(element)
    element_t * element

    CODE:
    RETVAL = element_length_in_bytes(*element);

    OUTPUT:
    RETVAL

SV *
element_order(element)
    element_t * element

    PREINIT:
    int i;
    char *c;

    CODE:
    i = mpz_sizeinbase(element[0]->field->order, 10);
    c = malloc(i + 2);

    mpz_get_str(c, 10, element[0]->field->order);

    RETVAL = newSVpv(c, strlen(c));
    free(c);

    OUTPUT:
    RETVAL
