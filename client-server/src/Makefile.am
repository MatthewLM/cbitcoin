bin_PROGRAMS = spv

spv_SOURCES =  spvmain.c

LIBADD = $(EVENT_LIBS) $(top_srcdir)/lib/libcbitcoin.la

COMMON_LDADD = @EVENT_LIBS@ $(top_srcdir)/lib/libcbitcoin.la


spv_LDADD           = $(COMMON_LDADD)      -I$(top_srcdir)/dependencies/storage/libfileec.la -I$(top_srcdir)/dependencies/storage/libstorage.la

