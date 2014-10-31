INCLUDES = @CRYPTO_LIBS@ -I$(top_srcdir)/include -I.
#-DTEST_SRCDIR=$(top_srcdir)/test

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

check_PROGRAMS = $(TESTS)

COMMON_LDADD = ../lib/libcbitcoin.a @CRYPTO_LIBS@ @EVENT_LIBS@


for prog in $(TESTS) ; do \
	"$$prog"_LDADD		= $(COMMON_LDADD) ; \
	"$$prog"_SOURCES	= "$$prog".c ; \
	"$$prog"_CFLAGS		= @CHECK_CFLAGS@ $(INCLUDES)
done




# Tests
TEST_FILES = $(wildcard library/test/*.c)
TEST_BINARIES = $(patsubst library/test/%.c, bin/%, $(TEST_FILES))
TEST_OBJS = $(patsubst library/test/%.c, build/%.o, $(TEST_FILES))
test-build : clean-tests $(TEST_BINARIES)
rm -f -r cbitcoin 0 1 2 test.dat testDb test.log
$(info ALL TESTS SUCCESSFUL)
# REMEMBER to add dependencies after the objects or libraries that depend on them.
$(TEST_BINARIES): bin/%: build/%.o
$(CC) $< -L$(BINDIR) -Wl,-rpath=$(BINDIR) $(LINK_CORE) $(LINK_NETWORK) $(LINK_STORAGE) $(LINK_THREADS) $(LINK_LOGGING) $(LINK_CRYPTO) $(LINK_CORE) -lcbitcoin-file-$(if $(subst bin/testCBFile,,$(subst bin/testCBHamming72,,$@)),@NOEC@,)ec.$(LIBRARY_VERSION) $(LINK_RAND) -L/opt/local/lib -levent_core -levent_pthreads -o $@
$@
$(TEST_OBJS): build/%.o: library/test/%.c library
$(CC) -c $(CFLAGS) -I$(CURDIR)/library/dependencies/sockets/ -I$(CURDIR)/library/dependencies/storage $< -o $@
clean-tests:
rm -f $(TEST_BINARIES)
