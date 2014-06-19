 #use Inline C;
 use Inline C => Config => LIBS => ' -lcbitcoin.2.0 -lcbitcoin-network.2.0 -lcbitcoin-storage.2.0 -lcbitcoin-threads.2.0 -lpthread -lcbitcoin-logging.2.0 -lcbitcoin-crypto.2.0 -lcrypto -lcbitcoin.2.0 -lcbitcoin-file-ec.2.0 -lcbitcoin-rand.2.0 '
	,INC => ['-I"/home/joeldejesus/Workspace/cbitcoin/library/include"' 
	         ,'-I"/home/joeldejesus/Workspace/cbitcoin/library/dependencies/sockets"'
	         ,'-I"/home/joeldejesus/Workspace/cbitcoin/library/dependencies/threads"'
	         ,'-I"/home/joeldejesus/Workspace/cbitcoin/library/test"'
	         ]
	,CC => '/usr/bin/c99'
#	,LD => 'gcc -Wl,-rpath=../bin'
	,CLEAN_AFTER_BUILD => 0
	,BUILD_NOISY => 1 ;
  

my $tx = {};


 use Inline C => <<'END_OF_C_CODE';

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
#include <CBTransaction.h>
 
int createTX(int arg){
	CBTransaction * self = CBNewTransaction(0,1);
	CBFreeTransaction(self);
	return 1;
}


END_OF_C_CODE
