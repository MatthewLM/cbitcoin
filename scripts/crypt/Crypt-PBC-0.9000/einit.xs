element_t *
element_init_G1(pairing)
    pairing_t * pairing

    PREINIT:
    element_t * element = malloc( sizeof(element_t) );

    CODE:
    element_init_G1(*element, *pairing);
    RETVAL = element;

    OUTPUT:
    RETVAL

element_t *
element_init_G2(pairing)
    pairing_t * pairing

    PREINIT:
    element_t * element = malloc( sizeof(element_t) );

    CODE:
    element_init_G2(*element, *pairing);
    RETVAL = element;

    OUTPUT:
    RETVAL

element_t *
element_init_GT(pairing)
    pairing_t * pairing

    PREINIT:
    element_t * element = malloc( sizeof(element_t) );

    CODE:
    element_init_GT(*element, *pairing);
    RETVAL = element;

    OUTPUT:
    RETVAL

element_t *
element_init_Zr(pairing)
    pairing_t * pairing

    PREINIT:
    element_t * element = malloc( sizeof(element_t) );

    CODE:
    element_init_Zr(*element, *pairing);
    RETVAL = element;

    OUTPUT:
    RETVAL

void
element_clear(element)
    element_t * element

    CODE:
    element_clear(*element);
    free(element);

void
element_random(element)
    element_t * element

    CODE:
    element_random(*element);

void
element_set0(element)
    element_t * element

    CODE:
    element_set0(*element);

void
element_set1(element)
    element_t * element

    CODE:
    element_set1(*element);

void
element_set(a,b)
    element_t * a
    element_t * b

    CODE:
    element_set(*a, *b);

void
element_set_si(a,b)
    element_t * a
    long b

    CODE:
    element_set_si(*a, b);

void
element_set_mpz(a,m)
    element_t * a
    mpz_t     * m

    CODE:
    element_set_mpz(*a, *m);

void
element_from_hash(element,hash)
    element_t * element
    SV * hash

    PREINIT:
    STRLEN len;
    char * ptr;

    CODE:
    ptr = SvPV(hash, len);
    element_from_hash(*element, ptr, len);

void
element_from_bytes(element,data)
    element_t * element
    SV * data

    PREINIT:
    STRLEN len;
    char * ptr;

    CODE:
    ptr = SvPV(data, len);
    element_from_bytes(*element, ptr); // , len);
