INCLUDES = @CRYPTO_LIBS@ -I$(top_srcdir)/include

lib_LTLIBRARIES = libcbitcoin-logging.la
libcbitcoin_logging_la_SOURCES = CBLog.c  CBLog.h
libcbitcoin_logging_la_LIBADD = $(top_srcdir)/dependencies/threads/libcbitcoin-threads.la
libcbitcoin_logging_la_CFLAGS = -I$(top_srcdir)/include  -I$(top_srcdir)/dependencies/threads
libcbitcoin_logging_la_LDFLAGS = -module -shared 


