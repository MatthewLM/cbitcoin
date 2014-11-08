INCLUDES = @CRYPTO_LIBS@ -I$(top_srcdir)/include

lib_LTLIBRARIES = libcbitcoin-sockets.la
libcbitcoin_sockets_la_SOURCES = CBCallbackQueue.c  CBCallbackQueue.h  CBLibEventSockets.c  CBLibEventSockets.h  CBLibevSockets.c  CBLibevSockets.h
libcbitcoin_sockets_la_LIBADD = $(top_srcdir)/src/libcbitcoin.la  $(top_srcdir)/dependencies/threads/libcbitcoin-threads.la
libcbitcoin_sockets_la_CFLAGS = -I$(top_srcdir)/include  
# -I$(top_srcdir)/dependencies/threads
libcbitcoin_sockets_la_LDFLAGS = -module -shared 


