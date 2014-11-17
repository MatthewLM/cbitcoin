INCLUDES = @CRYPTO_LIBS@ -I$(top_srcdir)/include

noinst_LTLIBRARIES = liblogging.la
liblogging_la_SOURCES = CBLog.c  CBLog.h
#liblogging_la_LIBADD = ../threads/libthreads.la
liblogging_la_CFLAGS = -I$(top_srcdir)/include  -I$(top_srcdir)/dependencies/threads
liblogging_la_LDFLAGS = -static 


