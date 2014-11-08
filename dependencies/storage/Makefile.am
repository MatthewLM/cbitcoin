INCLUDES = @CRYPTO_LIBS@ -I$(top_srcdir)/include

lib_LTLIBRARIES = libcbitcoin-storage.la
libcbitcoin_storage_la_SOURCES = CBAccounterStorage.c  CBAddressStorage.h     CBDatabase.c          CBFileEC.c    CBFileNoEC.h   CBNodeStorage.c CBAccounterStorage.h  CBBlockChainStorage.c  CBDatabase.h  CBFileEC.h    CBHamming72.c  CBNodeStorage.h CBAddressStorage.c    CBBlockChainStorage.h  CBFileDependencies.h  CBFileNoEC.c  CBHamming72.h
libcbitcoin_storage_la_LIBADD = $(top_srcdir)/src/libcbitcoin.la
libcbitcoin_storage_la_CFLAGS = -I$(top_srcdir)/include 
libcbitcoin_storage_la_LDFLAGS = -module -shared 


