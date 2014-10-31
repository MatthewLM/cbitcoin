INCLUDES = @CRYPTO_LIBS@ -I$(top_srcdir)/include -DTEST_SRCDIR=$(top_srcdir)/test

EXTRA_DIST = scriptCases.txt


TESTS = \
	testCBByteArray \
	testCBBase58 \
	testCBNetworkCommunicator \
	testCBGetBlocks \
	testCBAddressBroadcast \
	testValidation \
	testCBVersion \
	testCBAddress \
	testCBAddressManager \
	testCBDatabase \
	testCBTransaction \
	testCBInventory \
	testCBBlockHeaders \
	testCBScript \
	testCBHeadersOnlyValidator \
	testCBCallbackQueue \
	testCBAccounter \
	testCBNodeFull \
	testCBHDKeys \
	testCBBigInt \
	testCBAlert \
	testCBFile \
	testCBBlock \
	testCBFullValidator \
	testCBAssociativeArray \
	testCBHamming72 \

check_PROGRAMS = \
	testCBByteArray \
	testCBBase58 \
	testCBNetworkCommunicator \
	testCBGetBlocks \
	testCBAddressBroadcast \
	testValidation \
	testCBVersion \
	testCBAddress \
	testCBAddressManager \
	testCBDatabase \
	testCBTransaction \
	testCBInventory \
	testCBBlockHeaders \
	testCBScript \
	testCBHeadersOnlyValidator \
	testCBCallbackQueue \
	testCBAccounter \
	testCBNodeFull \
	testCBHDKeys \
	testCBBigInt \
	testCBAlert \
	testCBFile \
	testCBBlock \
	testCBFullValidator \
	testCBAssociativeArray \
	testCBHamming72 \


noinst_PROGRAMS = $(TESTS)


COMMON_LDADD = ../lib/libcbitcoin.a @CRYPTO_LIBS@ @EVENT_LIBS@


for prog in $(TESTS) ; do \
	"$$prog"_LDADD		= $(COMMON_LDADD) ; \
	"$$prog"_SOURCES	= "$$prog".c ; \
	"$$prog"_CFLAGS		= @CHECK_CFLAGS@ $(INCLUDES)
done


