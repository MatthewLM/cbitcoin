 use Inline::Files;
 use Inline C => Config => INC => '-I/home/joeldejesus/Workspace/cbitcoin/library/include',
 LIBS => '-lCBHDKeys -lopenssl/ssl -lopenssl/ripemd -lopenssl/rand';
 use Inline C;

print "My Life", generateWIF(2),"\n";

__END__
__C__
#include <stdio.h>
#include <ctype.h>
#include <openssl/ssl.h>
#include <openssl/ripemd.h>
#include <openssl/rand.h>
#include "CBAddress.h"
#include "CBHDKeys.h"



void getLine(char * ptr);
void getLine(char * ptr) {
int c;
int len = 30;
    for(;;) {
        c = fgetc(stdin);
        if(c == EOF)
            break;
        if(--len == 0)
            break;
*ptr = c;
        if(c == '\n')
            break;
ptr++;
    }
*ptr = 0;
}

int generateWIF(int num) {
	num = 1;
	//CBKeyPair *key = CBNewKeyPair(true);
	EC_GROUP * group = EC_GROUP_new_by_curve_name(NID_secp256k1);
	return num;
}


int change(SV* var1, SV* var2) {
    sv_setpvn(var1, "Perl Rocks!", 11);
    sv_setpvn(var2, "Inline Rules!", 13);
    return 1;
}
