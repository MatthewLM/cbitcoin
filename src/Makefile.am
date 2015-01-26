INCLUDES = -DTEST_SRCDIR=\"$(top_srcdir)/build\" -I$(top_srcdir)/include \
        -I$(top_srcdir)/dependencies/storage  -I$(top_srcdir)/dependencies/logging \
        -I$(top_srcdir)/dependencies/threads  -I$(top_srcdir)/dependencies/sockets

bin_PROGRAMS = spv

spv_SOURCES = spv.c


LIBADD = $(EVENT_LIBS) ../lib/libcbitcoin.la

COMMON_LDADD = @EVENT_LIBS@ ../lib/libcbitcoin.la


spv_LDADD           = $(COMMON_LDADD)      ../dependencies/storage/libfileec.la ../dependencies/storage/libstorage.la
