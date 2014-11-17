INCLUDES = @CRYPTO_LIBS@ -I$(top_srcdir)/include

lib_LTLIBRARIES = libcbitcoin.la

libcbitcoin_la_SOURCES= \
	CBAlert.c  \
	CBThreadPoolQueue.c  \
	CBInventory.c  \
	CBBigInt.c  \
	CBBlock.c  \
	CBValidationFunctions.c  \
	CBByteArray.c  \
	CBMerkleNode.c  \
	CBValidator.c  \
	CBScript.c  \
	CBTransactionOutput.c  \
	CBTransactionInput.c  \
	CBBase58.c  \
	CBTransaction.c  \
	CBNode.c  \
	CBPeer.c  \
	CBHDKeys.c  \
	CBDependencies.c  \
	CBWIF.c  \
	CBNetworkCommunicator.c  \
	CBNodeHeadersOnly.c  \
	CBObject.c  \
	CBNetworkAddressList.c  \
	CBChainDescriptor.c  \
	CBNetworkFunctions.c  \
	CBAssociativeArray.c  \
	CBNetworkAddress.c  \
	CBGetBlocks.c  \
	CBAddress.c  \
	CBChecksumBytes.c  \
	CBVersion.c  \
	CBInventoryItem.c  \
	CBMessage.c  \
	CBSanitiseOutput.c  \
	CBNetworkAddressManager.c  \
	CBVarInt.c  \
	CBNodeFull.c  \
	CBBlockHeaders.c  \
	CBPingPong.c \
	CBOpenSSLCrypto.c \
	CBLog.c CBLog.h \
	CBRand.c \
	CBCallbackQueue.c  CBLibEventSockets.c  CBLibevSockets.c CBCallbackQueue.h  CBLibEventSockets.h  CBLibevSockets.h \
	CBAccounterStorage.h CBBlockChainStorage.h CBFileDependencies.h CBFileNoEC.h CBNodeStorage.h CBAddressStorage.h CBDatabase.h CBFileEC.h CBHamming72.h \
	CBAccounterStorage.c  CBAddressStorage.c  CBBlockChainStorage.c  CBDatabase.c  CBFileEC.c  CBFileNoEC.c  CBHamming72.c  CBNodeStorage.c	 \
	CBThreads.c  CBThreads.h

#libcbitcoin_la_LIBADD = $(top_srcdir)/dependencies/libdep.la
libcbitcoin_la_CFLAGS = -I$(top_srcdir)/include 
libcbitcoin_la_LDFLAGS = -module -shared -version-info 2:0:1
# -I$(top_srcdir)/common


