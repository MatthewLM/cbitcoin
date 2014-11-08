INCLUDES = @CRYPTO_LIBS@ -I$(top_srcdir)/include

lib_LTLIBRARIES = libcbitcoin-storage.la
libcbitcoin_storage_la_SOURCES = CBOpenSSLstorage.c
libcbitcoin_storage_la_LIBADD = $(top_srcdir)/src/libcbitcoin.la
libcbitcoin_storage_la_CFLAGS = -I$(top_srcdir)/include 
libcbitcoin_storage_la_LDFLAGS = -module -shared 


