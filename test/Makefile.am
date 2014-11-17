INCLUDES = -DTEST_SRCDIR=\"$(top_srcdir)/build\" -I$(top_srcdir)/include \
	-I$(top_srcdir)/dependencies/storage  -I$(top_srcdir)/dependencies/logging \
	-I$(top_srcdir)/dependencies/threads  -I$(top_srcdir)/dependencies/sockets

EXTRA_DIST = scriptCases.txt 

noinst_PROGRAMS =  testCBByteArray \
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
	testCBHamming72 

TESTS = testCBByteArray \
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
	testCBHamming72 

COMMON_LDADD = ../lib/libcbitcoin.la \
	../dependencies/crypto/libcbitcoin-crypto.la \
	../dependencies/logging/libcbitcoin-logging.la \
	../dependencies/random/libcbitcoin-random.la \
	../dependencies/sockets/libcbitcoin-sockets.la \
	../dependencies/threads/libcbitcoin-threads.la 


testCBByteArray_LDADD 		= $(COMMON_LDADD) 
testCBBase58_LDADD 		= $(COMMON_LDADD) 
testCBNetworkCommunicator_LDADD	= $(COMMON_LDADD) 
testCBGetBlocks_LDADD 		= $(COMMON_LDADD) 
testCBAddressBroadcast_LDADD 	= $(COMMON_LDADD) 
testValidation_LDADD 		= $(COMMON_LDADD) 
testCBVersion_LDADD 		= $(COMMON_LDADD) 
testCBAddress_LDADD 		= $(COMMON_LDADD) 
testCBAddressManager_LDADD 	= $(COMMON_LDADD) 
testCBDatabase_LDADD 		= $(COMMON_LDADD) 	../dependencies/storage/libcbitcoin-file-ec.la ../dependencies/storage/libcbitcoin-storage.la
testCBTransaction_LDADD 	= $(COMMON_LDADD) 
testCBInventory_LDADD 		= $(COMMON_LDADD) 
testCBBlockHeaders_LDADD 	= $(COMMON_LDADD) 
testCBScript_LDADD 		= $(COMMON_LDADD) 
testCBHeadersOnlyValidator_LDADD= $(COMMON_LDADD) 
testCBCallbackQueue_LDADD 	= $(COMMON_LDADD) 
testCBAccounter_LDADD 		= $(COMMON_LDADD) 
testCBNodeFull_LDADD 		= $(COMMON_LDADD) 
testCBHDKeys_LDADD 		= $(COMMON_LDADD) 
testCBBigInt_LDADD		= $(COMMON_LDADD) 
testCBAlert_LDADD		= $(COMMON_LDADD) 
testCBFile_LDADD		= $(COMMON_LDADD) ../dependencies/storage/libcbitcoin-file-ec.la
testCBBlock_LDADD		= $(COMMON_LDADD) 
testCBFullValidator_LDADD	= $(COMMON_LDADD) 
testCBAssociativeArray_LDADD	= $(COMMON_LDADD) 
testCBHamming72_LDADD		= $(COMMON_LDADD) ../dependencies/storage/libcbitcoin-file-ec.la


