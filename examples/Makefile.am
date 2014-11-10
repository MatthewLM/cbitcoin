INCLUDES = @CRYPTO_LIBS@ -I$(top_srcdir)/include

noinst_PROGRAMS = \
	WIFConverter  \ 
	base58Converter  \ 
	base58ChecksumEncode  \ 
	noLowerAddressGenerator  \ 
	addressGenerator  

WIFConverter_SOURCES 		= WIFConverter.c
base58Converter_SOURCES 	= base58Converter.c
base58ChecksumEncode_SOURCES	= base58ChecksumEncode.c
noLowerAddressGenerator_SOURCES	= noLowerAddressGenerator.c
addressGenerator_SOURCES	= addressGenerator.c

COMMON_LDADD = ../src/libcbitcoin.la @CRYPTO_LIBS@ @EVENT_LIBS@

WIFConverter_LDADD		= $(COMMON_LDADD)
base58Converter_LDADD		= $(COMMON_LDADD)
base58ChecksumEncode_LDADD 	= $(COMMON_LDADD)
noLowerAddressGenerator_LDADD 	= $(COMMON_LDADD)
addressGenerator_LDADD 		= $(COMMON_LDADD)
