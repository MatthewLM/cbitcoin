void
element_add(lhs,rhs1,rhs2)
    element_t * lhs
    element_t * rhs1
    element_t * rhs2

    CODE:
    element_add(*lhs, *rhs1, *rhs2);

void
element_sub(lhs,rhs1,rhs2)
    element_t * lhs
    element_t * rhs1
    element_t * rhs2

    CODE:
    element_sub(*lhs, *rhs1, *rhs2);

void
element_mul(lhs,rhs1,rhs2)
    element_t * lhs
    element_t * rhs1
    element_t * rhs2

    CODE:
    element_mul(*lhs, *rhs1, *rhs2);

void
element_mul_zn(lhs,rhs1,rhs2)
    element_t * lhs
    element_t * rhs1
    element_t * rhs2

    CODE:
    element_mul_zn(*lhs, *rhs1, *rhs2);

void
element_mul_mpz(lhs,rhs1,rhs2)
    element_t * lhs
    element_t * rhs1
    mpz_t * rhs2

    CODE:
    element_mul_mpz(*lhs, *rhs1, *rhs2);

void
element_mul_si(lhs,rhs1,rhs2)
    element_t * lhs
    element_t * rhs1
    long rhs2

    CODE:
    element_mul_si(*lhs, *rhs1, rhs2);

void
element_div(lhs,rhs1,rhs2)
    element_t * lhs
    element_t * rhs1
    element_t * rhs2

    CODE:
    element_div(*lhs, *rhs1, *rhs2);

void
element_double(lhs,rhs)
    element_t * lhs
    element_t * rhs

    CODE:
    element_double(*lhs,*rhs);

void
element_halve(lhs,rhs)
    element_t * lhs
    element_t * rhs

    CODE:
    element_halve(*lhs,*rhs);

void
element_square(lhs,rhs)
    element_t * lhs
    element_t * rhs

    CODE:
    element_square(*lhs,*rhs);

void
element_neg(lhs,rhs)
    element_t * lhs
    element_t * rhs

    CODE:
    element_neg(*lhs,*rhs);

void
element_invert(lhs,rhs)
    element_t * lhs
    element_t * rhs

    CODE:
    element_invert(*lhs,*rhs);

void
element_pow_zn(LHS,RHS_base,RHS_expo)
    element_t * LHS
    element_t * RHS_base
    element_t * RHS_expo

    CODE:
    element_pow_zn(*LHS, *RHS_base, *RHS_expo);

void
element_pow2_zn(x,a1,n1,a2,n2)
    element_t * x
    element_t * a1
    element_t * n1
    element_t * a2
    element_t * n2

    CODE:
    element_pow2_zn(*x,*a1,*n2,*a2,*n2); // sets x = a1^n1 times a2^n2, but n1, n2 must be elements of a ring Z_n for some integer n.

void
element_pow3_zn(x,a1,n1,a2,n2,a3,n3)
    element_t * x
    element_t * a1
    element_t * n1
    element_t * a2
    element_t * n2
    element_t * a3
    element_t * n3

    CODE:
    element_pow3_zn(*x,*a1,*n2,*a2,*n2,*a3,*n3);

void
element_pow_mpz(LHS,RHS_base,RHS_expo)
    element_t * LHS
    element_t * RHS_base
        mpz_t * RHS_expo

    CODE:
    element_pow_mpz(*LHS, *RHS_base, *RHS_expo);

void
element_pow2_mpz(x,a1,n1,a2,n2)
    element_t * x
    element_t * a1
        mpz_t * n1
    element_t * a2
        mpz_t * n2

    CODE:
    element_pow2_mpz(*x,*a1,*n2,*a2,*n2);

void
element_pow3_mpz(x,a1,n1,a2,n2,a3,n3)
    element_t * x
    element_t * a1
        mpz_t * n1
    element_t * a2
        mpz_t * n2
    element_t * a3
        mpz_t * n3

    CODE:
    element_pow3_mpz(*x,*a1,*n2,*a2,*n2,*a3,*n3);
