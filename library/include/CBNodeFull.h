//
//  CBNodeFull.h
//  cbitcoin
//
//  Created by Matthew Mitchell on 23/08/2013.
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
 @brief Implements a fully validating bitcoin node.
 */

#ifndef CBNODEFULLH
#define CBNODEFULLH

#include "CBNode.h"

// Constants and Macros

#define CBGetNodeFull(x) ((CBNodeFull *)x)
#define CB_MIN_DOWNLOAD 4

typedef enum{
	CB_TX_ORPHAN,
	CB_TX_OURS,
	CB_TX_OTHER
} CBTransactionType;

// Structures

typedef struct{
	uint8_t hash[20];
	CBAssociativeArray peers; /**< An array of peers that have advertised this block. */
} CBBlockPeers;

typedef struct{
	uint8_t hash[32]; /**< The hash of the transaction an orphan transaction is dependent upon. */
	CBAssociativeArray dependants; /**< An array of orphan transactions that depend upon this transaction. @see CBOrphanDependency for unfound dependencies or CBTransaction for chain dependencies. */
}CBTransactionDependency;

typedef struct{
	CBTransaction * tx;
	bool ours;
	uint32_t height;
} CBChainTransaction;

typedef struct{
	CBTransaction * tx;
	CBTransactionType type;
	uint32_t numUnconfDeps; /**< Number of dependencies which are unconfirmed. */
} CBUnconfTransaction;

typedef struct{
	CBUnconfTransaction utx;
	uint32_t missingInputNum;
	uint64_t inputValue;
	uint32_t sigOps;
} CBOrphan;

typedef struct{
	CBOrphan * orphan;
	uint32_t inputIndex;
} CBOrphanDependency;

typedef struct{
	CBUnconfTransaction utx;
	CBAssociativeArray dependants;
	uint64_t nextRelay; /**< The time when afterwards this transaction can be relayed. */
	uint64_t timeFound;
	uint64_t inputValue;
} CBFoundTransaction;

typedef struct{
	bool chain;
	union{
		CBTransaction * chainTx;
		CBFoundTransaction * uTx;
	} txData;
} CBNewTransactionType;

typedef struct{
	CBUnconfTransaction * uTx;
	uint32_t blockHeight;
	uint32_t blockTime;
	uint8_t branch;
} CBTransactionToBranch;

typedef struct CBNewTransactionDetails CBNewTransactionDetails;

struct CBNewTransactionDetails{
	CBTransaction * tx;
	CBTransactionAccountDetailList * accountDetailList;
	uint32_t blockHeight;
	uint64_t time;
	bool ours;
	CBNewTransactionDetails * next;
};

typedef struct CBProcessQueueItem CBProcessQueueItem;

struct CBProcessQueueItem{
	void * item;
	CBProcessQueueItem * next;
};

typedef struct CBNewTransactionTypeProcessQueueItem CBNewTransactionTypeProcessQueueItem;

struct CBNewTransactionTypeProcessQueueItem{
	CBNewTransactionType item;
	CBNewTransactionTypeProcessQueueItem * next;
};

typedef struct CBFindResultProcessQueueItem CBFindResultProcessQueueItem;

struct CBFindResultProcessQueueItem{
	CBFindResult res;
	CBFindResultProcessQueueItem * next;
};

/**
 @brief Structure for CBNodeFull objects. @see CBNodeFull.h
 */
typedef struct CBNodeFull{
	CBNode base; /**< Uses a CBNode as the base structure. */
	CBAssociativeArray ourTxs; /**< Unconfirmed transactions that belong to this node. */
	CBAssociativeArray otherTxs; /**< Unconfirmed transactions that are to be relayed but do not belong to this node. This is only used by a fully validating node. */
	CBAssociativeArray orphanTxs; /**< Unconfirmed transactions with missing inputs. Stored alongside other transactions up to otherTxsSizeLimit, but orphans are removed when space is run out for other transactions. Has CBOrphanData for additional orphan information. */
	CBAssociativeArray allChainFoundTxs; /**< Uncofnirmed transactions where all of the dependencies can be found on the chain. */
	uint64_t deleteOrphanTime;
	CBAssociativeArray unfoundDependencies; /**< A array of unfound dependencies for unconfirmed transactions. @see CBTransactionDependency */
	CBAssociativeArray chainDependencies; /**< A array of dependencies in the block chain for unconfirmed transactions. @see CBTransactionDependency */
	uint32_t otherTxsSize; /**< The size of all of the transactions stored by otherTxs. */
	uint32_t otherTxsSizeLimit; /**< The limit of the size for other transactions. Should be at least the maximum transaction size. */
	uint8_t numberPeersDownload; /**< The number of peers we are downloading blocks from. */
	uint32_t numCanDownload;
	bool addingDirect;
	CBAssociativeArray askedForBlocks; /**< Blocks we have asked for with getdata with CBGetBlockInfo */
	CBAssociativeArray blockPeers; /**< For each block that has not been received this contains peers that supposedly have the block. */
	CBAssociativeArray unconfirmedTxs; /**< During a reoganisation this keeps track of transactions which are taken off the block-chain. */
	CBAssociativeArray unconfirmedTxDependencies; /**< Dependencies for lost transactions. @see CBTransactionDependency */
	CBAssociativeArray foundUnconfTxs; /**< Array of unconfirmed transactions that have been found on a reorg chain. */
	CBAssociativeArray lostUnconfTxs; /**< Array of unconfirmed transactions that have been lost by double spend found on the chain. */
	CBAssociativeArray lostChainTxs;
	CBNewTransactionDetails * newTransactions;
	CBNewTransactionDetails * newTransactionsLast;
	bool initialisedUnconfirmedTxs;
	bool initialisedFoundUnconfTxs;
	bool initialisedLostUnconfTxs;
	bool initialisedLostChainTxs;
	uint32_t forkPoint;
	CBDepObject pendingBlockDataMutex;
	CBDepObject numberPeersDownloadMutex;
	CBDepObject numCanDownloadMutex;
	CBDepObject workingMutex;
} CBNodeFull;

/**
 @brief Creates a new CBNodeFull object.
 @param database The database object to use.
 @param flags Flags for the node.
 @param otherTxsSizeLimit The limit of the size of the unconfirmed transactions to hold which are not owned by the accounts. Should be at least the maximum transaction size.
 @returns A new CBNodeFull object.
 */
CBNodeFull * CBNewNodeFull(CBDepObject database, CBNodeFlags flags, uint32_t otherTxsSizeLimit, CBNodeCallbacks callbacks);

/**
 @brief Initialises a CBNodeFull object
 @param self The CBNode object to initialise
 @param database The database object to use.
 @param flags Flags for the node
 @param otherTxsSizeLimit The limit of the size of the unconfirmed transactions to hold which are not owned by the accounts. Should be at least the maximum transaction size.
 @returns true on success, false on failure.
 */
bool CBInitNodeFull(CBNodeFull * self, CBDepObject database, CBNodeFlags flags, uint32_t otherTxsSizeLimit, CBNodeCallbacks callbacks);

/**
 @brief Releases and frees all of the objects stored by the CBNodeFull object.
 @param self The CBNodeFull object to destroy.
 */
void CBDestroyNodeFull(void * self);
/**
 @brief Frees a CBNodeFull object and also calls CBDestroyNodeFull.
 @param self The CBNodeFull object to free.
 */
void CBFreeNodeFull(void * self);

void CBFreeBlockPeers(void * blkPeers);
void CBFreeChainTransaction(void * vtx);
void CBFreeFoundTransaction(void * vfndTx);
void CBFreeOrphan(void * vorphanData);
void CBFreeTransactionDependency(void * vtxDep);

// Functions

/**
 @brief Compares two 20 bytes of hashes
 @param tx1 The first 20 byte hash
 @param tx2 The second 20 byte hash
 @returns A CBCompare value corresponding to hashes.
 */
CBCompare CBHashCompare(CBAssociativeArray * foo, void * hash1, void * hash2);
bool CBNodeFullAcceptType(CBNetworkCommunicator * comm, CBPeer * peer, CBMessageType type);
bool CBNodeFullAddBlock(void *, uint8_t branch, CBBlock * block, uint32_t blockHeight, CBAddBlockType addType);
bool CBNodeFullAddBlockDirectly(CBNodeFull * self, CBBlock * block);
CBErrBool CBNodeFullAddOurOrOtherFoundTransaction(CBNodeFull * self, bool ours, CBUnconfTransaction utx, CBFoundTransaction * fndTx, uint64_t txInputValue);
void CBNodeFullAddToDependency(CBAssociativeArray * deps, uint8_t * hash, void * el, CBCompare (*compareFunc)(CBAssociativeArray *, void *, void *));
CBErrBool CBNodeFullAddTransactionAsFound(CBNodeFull * self, CBUnconfTransaction utx, CBFoundTransaction * fndTx, uint64_t txInputValue);
uint64_t CBNodeFullAlreadyValidated(void * vpeer, CBTransaction * tx);
void CBNodeFullAskForBlock(CBNodeFull * self, CBPeer * peer, uint8_t * hash);
void CBNodeFullCancelBlock(CBNodeFull * self, CBPeer * peer);
bool CBNodeFullDeleteBranchCallback(void * node, uint8_t branch);
void CBNodeFullDeleteOrphanFromDependencies(CBNodeFull * self, CBOrphan * orphan);
bool CBNodeFullDependantSpendsPrevOut(CBAssociativeArray * dependants, CBPrevOut prevOut);
void CBNodeFullDownloadFromSomePeer(CBNodeFull * self, CBPeer * notPeer);
void CBNodeFullFinishedWithBlock(CBNodeFull * self, CBBlock * block);
CBErrBool CBNodeFullFoundTransaction(CBNodeFull * self, CBTransaction * tx, uint64_t time, uint32_t blockHeight, bool callNow, bool processDependants);
CBUnconfTransaction * CBNodeFullGetAnyTransaction(CBNodeFull * self, uint8_t * hash);
CBFoundTransaction * CBNodeFullGetFoundTransaction(CBNodeFull * self, uint8_t * hash);
CBOrphan * CBNodeFullGetOrphanTransaction(CBNodeFull * self, uint8_t * hash);
CBFoundTransaction * CBNodeFullGetOtherTransaction(CBNodeFull * self, uint8_t * hash);
CBFoundTransaction * CBNodeFullGetOurTransaction(CBNodeFull * self, uint8_t * hash);
void CBNodeFullHasNotGivenBlockInv(void * vpeer);
CBErrBool CBNodeFullHasTransaction(CBNodeFull * self, uint8_t * hash);
void CBNodeFullInvalidateOrphans(CBNodeFull * self, uint8_t * txHash);
bool CBNodeFullInvalidBlock(void *, CBBlock * block);
bool CBNodeFullIsOrphan(void *, CBBlock * block);
bool CBNodeFullLoseChainTxDependants(CBNodeFull * self, uint8_t * hash, bool now);
bool CBNodeFullLostUnconfTransaction(CBNodeFull * self, CBUnconfTransaction * uTx, bool now);
bool CBNodeFullNewBranchCallback(void *, uint8_t branch, uint8_t parent, uint32_t blockHeight);
CBErrBool CBNodeFullNewUnconfirmedTransaction(CBNodeFull * self, CBPeer * peer, CBTransaction * tx);
bool CBNodeFullMakeLostChainTransaction(CBNodeFull * self, CBChainTransaction * tx, bool now);
bool CBNodeFullNoNewBranches(void *, CBBlock * block);
void CBNodeFullOnNetworkError(CBNetworkCommunicator * comm, CBErrorReason reason);
bool CBNodeFullOnTimeOut(CBNetworkCommunicator * comm, void * peer);
void CBNodeFullPeerDownloadEnd(CBNodeFull * self, CBPeer * peer);
/**
 @brief Frees data associated with the peer.
 @param peer The peer
 */
void CBNodeFullPeerFree(void * peer);
/**
 @brief Setups the peer.
 @param self The CBNetworkCommunicator object.
 @param peer The peer
 */
void CBNodeFullPeerSetup(CBNetworkCommunicator * self, CBPeer * peer);
bool CBNodeFullPeerWorkingOnBranch(void * vpeer, uint8_t branch);
/**
 @brief Processes transactions, block headers, inventory broadcasts, header requests and data requests.
 @param self The CBNetworkCommunicator object.
 @param peer The peer
 @returns @see CBOnMessageReceivedAction.
 */
CBOnMessageReceivedAction CBNodeFullProcessMessage(CBNode * self, CBPeer * peer, CBMessage * message);
bool CBNodeFullProcessNewTransaction(CBNodeFull * self, CBTransaction * tx, uint64_t time, uint32_t blockHeight, CBTransactionAccountDetailList * list, bool processDependants, bool ours);
bool CBNodeFullProcessNewTransactionProcessDependants(CBNodeFull * self, CBNewTransactionType txType);
bool CBNodeFullRemoveBlock(void *, uint8_t branch, uint32_t blockHeight, CBBlock * block);
void CBNodeFullRemoveFoundTransactionFromDependencies(CBNodeFull * self, CBUnconfTransaction * uTx);
void CBNodeFullRemoveFromDependency(CBAssociativeArray * deps, uint8_t * hash, void * el);
bool CBNodeFullRemoveLostUnconfTxs(CBNodeFull * self);
bool CBNodeFullRemoveOrphan(CBNodeFull * self);
bool CBNodeFullRemoveUnconfTx(CBNodeFull * self, CBUnconfTransaction * tx);
bool CBNodeFullSendGetBlocks(CBNodeFull * self, CBPeer * peer, uint8_t * extraBlock, CBByteArray * stopAtHash);
/**
 @brief Sends data requested by an inventory broadcast.
 @param self The CBNetworkCommunicator object.
 @param peer The peer
 */
void CBNodeFullSendRequestedData(void * self, void * peer);
/**
 @brief Same as CBNodeFullSendRequestedData but takes void pointers and does not return anything.
 @param self The CBNetworkCommunicator object.
 @param peer The peer
 */
bool CBNodeFullStartValidation(void * vpeer);
bool CBNodeFullUnconfToChain(CBNodeFull * self, CBUnconfTransaction * uTx, uint32_t blockHeight, uint32_t time);
bool CBNodeFullValidatorFinish(void *, CBBlock * block);
CBCompare CBOrphanDependencyCompare(CBAssociativeArray *, void * txDep1, void * txDep2);

#endif
