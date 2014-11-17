#INCLUDES = @CRYPTO_LIBS@ #-I$(top_srcdir)/include

noinst_LTLIBRARIES = libsockets.la
libsockets_la_SOURCES = CBCallbackQueue.c CBLibEventSockets.c
libsockets_la_LIBADD = $(top_srcdir)/dependencies/threads/libthreads.la
libsockets_la_CFLAGS =  -I$(top_srcdir)/include  -I$(top_srcdir)/dependencies/threads
#libsockets_la_LDFLAGS = -module -shared 


