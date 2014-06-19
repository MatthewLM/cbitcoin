int
element_is0(element)
    element_t * element

    CODE:
    RETVAL = element_is0(*element);

    OUTPUT:
    RETVAL

int
element_is1(element)
    element_t * element

    CODE:
    RETVAL = element_is1(*element);

    OUTPUT:
    RETVAL

int
element_is_sqr(element)
    element_t * element

    CODE:
    RETVAL = element_is_sqr(*element);

    OUTPUT:
    RETVAL

int
element_cmp(a,b)
    element_t * a
    element_t * b

    CODE:
    RETVAL = element_cmp(*a, *b);

    OUTPUT:
    RETVAL
