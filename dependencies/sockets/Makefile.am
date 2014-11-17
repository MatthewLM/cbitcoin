INCLUDES = @EVENT_LIBS@ @CRYPTO_LIBS@ #-I$(top_srcdir)/include

lib_LTLIBRARIES = libcbitcoin-sockets.la
libcbitcoin_sockets_la_SOURCES = CBCallbackQueue.c CBLibEventSockets.c
#libcbitcoin_sockets_la_LDADD = $(EVENT_LIBS)
libcbitcoin_sockets_la_LIBADD = $(top_srcdir)/dependencies/threads/libcbitcoin-threads.la $(EVENT_LIBS)
libcbitcoin_sockets_la_CFLAGS =  @EVENT_LIBS@ -I$(top_srcdir)/include  -I$(top_srcdir)/dependencies/threads
libcbitcoin_sockets_la_LDFLAGS = -module -shared 


