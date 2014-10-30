INCLUDES = @CRYPTO_LIBS@ -I$(top_srcdir)/include -I.

EXTRA_DIST = scriptCases.txt


noinst_LIBRARIES= libtest.a

libtest_a_SOURCES=  checkTransactions.h\
	testCBByteArray.c \
	testCBBase58.c \
	testCBNetworkCommunicator.c \
	testCBGetBlocks.c \
	testCBAddressBroadcast.c \
	testValidation.c \
	testCBVersion.c \
	testCBAddress.c \
	testCBAddressManager.c \
	testCBDatabase.c \
	testCBTransaction.c \
	testCBInventory.c \
	testCBBlockHeaders.c \
	testCBScript.c \
	testCBHeadersOnlyValidator.c \
	testCBCallbackQueue.c \
	testCBAccounter.c \
	testCBNodeFull.c \
	testCBHDKeys.c \
	testCBBigInt.c \
	testCBAlert.c \
	testCBFile.c \
	testCBBlock.c \
	testCBFullValidator.c \
	testCBAssociativeArray.c \
	testCBHamming72.c \

noinst_PROGRAMS = \
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
	testCBHamming72 

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

COMMON_LDADD = libtest.a ../lib/libcbitcoin.a @CRYPTO_LIBS@ @EVENT_LIBS@


testCBByteArray_LDADD = $(COMMON_LDADD) 
testCBBase58_LDADD = $(COMMON_LDADD) 
testCBNetworkCommunicator_LDADD = $(COMMON_LDADD) 
testCBGetBlocks_LDADD = $(COMMON_LDADD) 
testCBAddressBroadcast_LDADD = $(COMMON_LDADD) 
testValidation_LDADD = $(COMMON_LDADD) 
testCBVersion_LDADD = $(COMMON_LDADD) 
testCBAddress_LDADD = $(COMMON_LDADD) 
testCBAddressManager_LDADD = $(COMMON_LDADD) 
testCBDatabase_LDADD = $(COMMON_LDADD) 
testCBTransaction_LDADD = $(COMMON_LDADD) 
testCBInventory_LDADD = $(COMMON_LDADD) 
testCBBlockHeaders_LDADD = $(COMMON_LDADD) 
testCBScript_LDADD = $(COMMON_LDADD) 
testCBHeadersOnlyValidator_LDADD = $(COMMON_LDADD) 
testCBCallbackQueue_LDADD = $(COMMON_LDADD) 
testCBAccounter_LDADD = $(COMMON_LDADD) 
testCBNodeFull_LDADD = $(COMMON_LDADD) 
testCBHDKeys_LDADD = $(COMMON_LDADD) 
testCBBigInt_LDADD = $(COMMON_LDADD) 
testCBAlert_LDADD = $(COMMON_LDADD) 
testCBFile_LDADD = $(COMMON_LDADD) 
testCBBlock_LDADD = $(COMMON_LDADD) 
testCBFullValidator_LDADD = $(COMMON_LDADD) 
testCBAssociativeArray_LDADD = $(COMMON_LDADD) 
testCBHamming72_LDADD = $(COMMON_LDADD) 
