INCLUDES = @EVENT_LIBS@ @CRYPTO_LIBS@ #-I$(top_srcdir)/include

noinst_LTLIBRARIES = libsockets.la
libsockets_la_SOURCES = CBCallbackQueue.c CBLibEventSockets.c
#libsockets_la_LIBADD = $(EVENT_LIBS) 
libsockets_la_CFLAGS =  @EVENT_LIBS@ -I$(top_srcdir)/include  -I$(top_srcdir)/dependencies/threads
libsockets_la_LDFLAGS = -module -shared 


