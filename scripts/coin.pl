 #use Inline C;
 use Inline C => Config => LIBS => ' -lcbitcoin.2.0 -lcbitcoin-network.2.0 -lcbitcoin-storage.2.0 -lcbitcoin-threads.2.0 -lpthread -lcbitcoin-logging.2.0 -lcbitcoin-crypto.2.0 -lcrypto -lcbitcoin.2.0 -lcbitcoin-file-ec.2.0 -lcbitcoin-rand.2.0 '
	,INC => '-I"/home/joeldejesus/Workspace/cbitcoin/library/include"'
#	,LD => 'gcc -Wl,-rpath=../bin'
	,CLEAN_AFTER_BUILD => 0
	,BUILD_NOISY => 1 ;
  
print "9 + 16 = ", add(9, 16), "\n";
print "9 - 16 = ", subtract(9, 16), "\n";
print "WIF:".createWIF(2)."\n";

 use Inline C => <<'END_OF_C_CODE';

#include <CBHDKeys.h>
#include <CBChecksumBytes.h>
#include <CBAddress.h>
#include <CBWIF.h>


int createWIF(int arg){
	CBHDKey * newKey = CBNewHDKey(false);
	return arg;
}

int add(int x, int y) {
  return x + y;
}

int subtract(int x, int y) {
  return x - y;
}


END_OF_C_CODE
