INCLUDES = @CRYPTO_LIBS@ -I$(top_srcdir)/include

noinst_LTLIBRARIES = libfileec.la libfilenoec.la libstorage.la

libfileec_la_SOURCES = CBHamming72.c CBFileEC.c 
libfilenoec_la_SOURCES = CBFileNoEC.c
libstorage_la_SOURCES = CBAccounterStorage.c CBAddressStorage.c CBBlockChainStorage.c CBNodeStorage.c CBDatabase.c

libfileec_la_CFLAGS = -I$(top_srcdir)/include
libfilenoec_la_CFLAGS = -I$(top_srcdir)/include
libstorage_la_CFLAGS = -I$(top_srcdir)/include

libfileec_la_LDFLAGS = -static
libfilenoec_la_LDFLAGS = -static
libstorage_la_LDFLAGS = -static
