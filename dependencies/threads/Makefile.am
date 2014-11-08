INCLUDES = @CRYPTO_LIBS@ -I$(top_srcdir)/include

lib_LTLIBRARIES = libcbitcoin-threads.la
libcbitcoin_threads_la_SOURCES = CBThreads.c  CBThreads.h 
libcbitcoin_threads_la_LIBADD = $(top_srcdir)/src/libcbitcoin.la
libcbitcoin_threads_la_CFLAGS =  @CRYPTO_LIBS@ -I$(top_srcdir)/include 
libcbitcoin_threads_la_LDFLAGS = -module -shared 


