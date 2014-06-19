// C: -I/usr/include/pbc -lpbc
// cat params_d.txt | c c_quicky.c

#include <stdio.h>
#include <string.h>
#include <pbc/pbc.h>

char huge1[4096] = "TEST";
char huge2[4096] = "another test";

int main() {
    pairing_t pairing;
    element_t gt, g1, g2;
    char ptr[4096];

    size_t count = fread(ptr, 1, 1024, stdin);
    if (!count) pbc_die("input error");
    if (pairing_init_set_buf(pairing, ptr, count)) pbc_die("pairing init failed");

    element_init_GT(gt, pairing);
    element_init_G1(g1, pairing);
    element_init_G2(g2, pairing);

    element_random(g1);
    element_random(g2);

    element_from_hash(g1, huge1, strlen(huge1));
    element_from_hash(g2, huge2, strlen(huge2));

    pairing_apply(gt, g1, g2, pairing);

    element_printf("gt: %B\n", gt);
}
