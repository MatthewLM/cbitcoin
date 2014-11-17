#INCLUDES = @CRYPTO_LIBS@ -I$(top_srcdir)/include

lib_LTLIBRARIES = libcbitcoin-threads.la
libcbitcoin_threads_la_SOURCES = CBThreads.c  
libcbitcoin_threads_la_LIBADD = $(EVENT_LIBS)
libcbitcoin_threads_la_CFLAGS =  -I$(top_srcdir)/include 
libcbitcoin_threads_la_LDFLAGS = -module -shared 


