INCLUDES = @CRYPTO_LIBS@ -I$(top_srcdir)/include

lib_LTLIBRARIES = libcbitcoin-crypto.la
libcbitcoin_crypto_la_SOURCES = CBOpenSSLCrypto.c
libcbitcoin_crypto_la_LIBADD = $(top_srcdir)/src/libcbitcoin.la
libcbitcoin_crypto_la_CFLAGS = -I$(top_srcdir)/include 
libcbitcoin_crypto_la_LDFLAGS = -module -shared 


