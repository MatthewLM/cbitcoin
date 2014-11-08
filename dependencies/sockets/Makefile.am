INCLUDES = @CRYPTO_LIBS@ -I$(top_srcdir)/include

lib_LTLIBRARIES = libcbitcoin-sockets.la
libcbitcoin_sockets_la_SOURCES = CBOpenSSLsockets.c
libcbitcoin_sockets_la_LIBADD = $(top_srcdir)/src/libcbitcoin.la
libcbitcoin_sockets_la_CFLAGS = -I$(top_srcdir)/include 
libcbitcoin_sockets_la_LDFLAGS = -module -shared 


