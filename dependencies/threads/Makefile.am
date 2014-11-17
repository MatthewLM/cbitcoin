#INCLUDES = @CRYPTO_LIBS@ -I$(top_srcdir)/include

noinst_LTLIBRARIES = libthreads.la
libthreads_la_SOURCES = CBThreads.c 
libthreads_la_LIBADD = $(EVENT_LIBS)
libthreads_la_CFLAGS =  -I$(top_srcdir)/include @EVENT_LIBS@
libthreads_la_LDFLAGS = -static


