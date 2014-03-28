//
//  CBNode.h
//  cbitcoin
//
//  Created by Matthew Mitchell on 06/02/2013.
//  Copyright (c) 2013 Matthew Mitchell
//
//  This file is part of cbitcoin. It is subject to the license terms
//  in the LICENSE file found in the top-level directory of this
//  distribution and at http://www.cbitcoin.com/license.html. No part of
//  cbitcoin, including this file, may be copied, modified, propagated,
//  or distributed except according to the terms contained in the
//  LICENSE file.

/**
 @file
 @brief The base structure for node objects, which can be fully validating or headers only.
 */

#ifndef CBNODEH
#define CBNODEH

#include "CBConstants.h"
#include "CBNetworkCommunicator.h"
#include "CBValidator.h"

// Constants and Macros

#define CBGetNode(x) ((CBNode *)x)
#define CB_NONE_SCANNED UINT32_MAX
#define CB_NO_FORK 0
#define CB_NO_BRANCH 0xFF

typedef enum{
	CB_NODE_CHECK_STANDARD = 1,
	CB_NODE_BOOTSTRAP = 2
} CBNodeFlags;

typedef struct{
	CBNetworkCommunicator * self;
	CBPeer * peer;
	CBMessage * message;
	void (*callback)(void *, void *);
} CBSendMessageData;

typedef struct CBNode CBNode;

typedef struct{
	void (*onFatalNodeError)(CBNode *, CBErrorReason reason);
	void (*newBlock)(CBNode *, CBBlock * block, uint32_t forkPoint);
	void (*newTransaction)(CBNode *, CBTransaction * tx, uint64_t timestamp, uint32_t blockHeight, CBTransactionAccountDetailList * details);
	void (*transactionConfirmed)(CBNode *, uint8_t * txHash, uint32_t blockHeight);
	void (*doubleSpend)(CBNode *, uint8_t * txHash);
	void (*transactionUnconfirmed)(CBNode *, uint8_t * txHash);
	void (*uptodate)(CBNode *, bool); /**< Called after the uptodate status changes. True if uptodate and false if we need updating from peer. */
} CBNodeCallbacks;

typedef struct CBMessageQueue CBMessageQueue;

struct CBMessageQueue{
	CBPeer * peer;
	CBMessage * message;
	CBMessageQueue * next;
};

/**
 @brief Structure for CBNode objects. @see CBNode.h
 */
struct CBNode{
	CBNetworkCommunicator base; /**< Uses a CBNetworkCommunicator as the base structure. */
	CBDepObject database;
	CBDepObject blockChainStorage; /**< The storage object for the block chain */
	CBDepObject accounterStorage; /**< The storage object for accounting. */
	CBDepObject nodeStorage;
	CBValidator * validator; /**< Validator object */
	CBDepObject blockAndTxMutex;
	CBNodeFlags flags; /**< @see CBNodeFlags */
	CBNodeCallbacks callbacks;
	bool shutDownThread;
	CBDepObject messageProcessThread;
	CBDepObject messageProcessMutex;
	CBDepObject messageProcessWaitCond;
	CBMessageQueue * messageQueue;
	CBMessageQueue * messageQueueBack;
	CBOnMessageReceivedAction (*onMessageReceived)(CBNode *, CBPeer *, CBMessage *); /**< This is the callback to handle messages on the thread.  */
};

/**
 @brief Creates a new CBNode object.
 @param database The database object to use.
 @param flags Flags for the node.
 @returns A new CBNode object.
 */
CBNode * CBNewNode(CBDepObject database, CBNodeFlags flags, CBNodeCallbacks nodeCallbacks, CBNetworkCommunicatorCallbacks commCallbacks, CBOnMessageReceivedAction (*onMessageReceived)(CBNode *, CBPeer *, CBMessage *));

/**
 @brief Initialises a CBNode object
 @param self The CBNode object to initialise
 @param database The database object to use.
 @param flags Flags for the node
 @returns true on success, false on failure.
 */
bool CBInitNode(CBNode * self, CBDepObject database, CBNodeFlags flags, CBNodeCallbacks nodeCallbacks, CBNetworkCommunicatorCallbacks commCallbacks, CBOnMessageReceivedAction (*onMessageReceived)(CBNode *, CBPeer *, CBMessage *));

/**
 @brief Releases and frees all of the objects stored by the CBNode object.
 @param self The CBNode object to destroy.
 */
void CBDestroyNode(void * self);

/**
 @brief Frees a CBNode object and also calls CBDestroyNode.
 @param self The CBNode object to free.
 */
void CBFreeNode(void * self);

// Functions

void CBNodeDisconnectPeer(CBPeer * peer);
void CBNodeDisconnectPeerRun(void * vpeer);
void CBNodeOnBadTime(void * self);
CBOnMessageReceivedAction CBNodeOnMessageReceived(CBNetworkCommunicator * comm, CBPeer * peer, CBMessage * message);
void CBNodeOnValidatorError(void * vself);
CBOnMessageReceivedAction CBNodeProcessAlert(CBNode * self, CBPeer * peer, CBAlert * alert);
void CBNodeProcessMessages(void * node);
CBOnMessageReceivedAction CBNodeReturnError(CBNode * self, char * err);
CBOnMessageReceivedAction CBNodeSendBlocksInvOrHeaders(CBNode * self, CBPeer * peer, CBGetBlocks * getBlocks, bool full);
void CBNodeSendMessageOnNetworkThread(CBNetworkCommunicator * self, CBPeer * peer, CBMessage * message, void (*callback)(void *, void *));
void CBNodeSendMessageOnNetworkThreadVoid(void * data);

/**
 @brief Compares two transactions by their hash
 @param tx1 The first transaction
 @param tx2 The second transaction
 @returns A CBCompare value corresponding to the transaction hashes.
 */
CBCompare CBTransactionCompare(CBAssociativeArray * foo, void * tx1, void * tx2);
CBCompare CBTransactionPtrCompare(CBAssociativeArray * foo, void * tx1, void * tx2);

/**
 @brief Compares a hash with a transaction's hash.
 @param hash A hash for a transaction.
 @param tx A pointer to the transaction pointer.
 @returns A CBCompare value corresponding to the transaction hashes.
 */
CBCompare CBTransactionPtrAndHashCompare(CBAssociativeArray * foo, void * hash, void * tx);

#endif
