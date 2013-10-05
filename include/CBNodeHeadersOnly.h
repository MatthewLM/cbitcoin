//
//  CBNodeHeadersOnly.h
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
 @brief Implements a headers only bitcoin node.
 */

#ifndef CBNODEHEADERSONLYH
#define CBNODEHEADERSONLYH

#include "CBNode.h"

// Constants and Macros

#define CBGetNode(x) ((CBNode *)x)
#define CB_NONE_SCANNED UINT32_MAX

typedef struct CBNodeHeadersOnly CBNodeHeadersOnly;

typedef struct{
	uint8_t hash[20];
	uint32_t numPeers; /**< The number of peer objects that have this block as required from them. */
} CBGetBlockInfo;

/**
 @brief Structure for CBNodeHeadersOnly objects. @see CBNodeHeadersOnly.h
 */
struct CBNodeHeadersOnly{
	CBNetworkCommunicator base; /**< Uses a CBNetworkCommunicator as the base structure. */
	uint32_t startScanning; /**< The timestamp from where to start scanning for transactions */
	CBAssociativeArray ourTxs; /**< Unconfirmed transactions that belong to this node. */
	CBNodeFlags flags; /**< @see CBNodeFlags */
	CBNodeCallbacks callbacks;
	CBAssociativeArray reqBlocks; /**< Blocks we require and have not yet asked with CBGetBlockInfo. */
	uint8_t numberPeersDownload; /**< The number of peers we are downloading blocks from. */
	CBAssociativeArray askedForBlocks; /**< Blocks we have asked for with getdata with CBGetBlockInfo */
	uint8_t currentBranch; /**< The current branch of the block we need from a peer when headers only. */
	uint32_t currentHeights[CB_MAX_BRANCH_CACHE]; /**< The current heights of the blocks we need for each branch when headers only. */
	CBBlockHeaders * headers; /**< Headers we have gotten from a peer that needs processing. */
	uint16_t headerIndex; /**< The index of the header we are to process. */
};

/**
 @brief Creates a new CBNode object.
 @param database The database object to use.
 @param flags Flags for the node.
 @param otherTxsSizeLimit The limit of the size of the unconfirmed transactions to hold which are not owned by the accounts. Should be at least the maximum transaction size.
 @returns A new CBNode object.
 */
CBNode * CBNewNodeHeadersOnly(CBDepObject database, CBNodeFlags flags, uint32_t otherTxsSizeLimit, CBNodeCallbacks callbacks);

/**
 @brief Initialises a CBNode object
 @param self The CBNode object to initialise
 @param database The database object to use.
 @param flags Flags for the node
 @param otherTxsSizeLimit The limit of the size of the unconfirmed transactions to hold which are not owned by the accounts. Should be at least the maximum transaction size.
 @returns true on success, false on failure.
 */
bool CBInitNodeHeadersOnly(CBNode * self, CBDepObject database, CBNodeFlags flags, uint32_t otherTxsSizeLimit, CBNodeCallbacks callbacks);

/**
 @brief Releases and frees all of the objects stored by the CBNode object.
 @param self The CBNode object to destroy.
 */
void CBDestroyNodeHeadersOnly(void * self);
/**
 @brief Frees a CBNode object and also calls CBDestroyNode.
 @param self The CBNode object to free.
 */
void CBFreeNodeHeadersOnly(void * self);

// Functions

/**
 @brief Compares two 20 bytes of hashes
 @param tx1 The first 20 byte hash
 @param tx2 The second 20 byte hash
 @returns A CBCompare value corresponding to hashes.
 */
CBCompare CBHashCompare(CBAssociativeArray * foo, void * hash1, void * hash2);
bool CBNodeHeadersOnlyAcceptType(CBNetworkCommunicator * comm, CBPeer * peer, CBMessageType type);
bool CBNodeHeadersOnlyDeleteBranchCallback(void * node, uint8_t branch);
bool CBNodeHeadersOnlyGetBlockForTxs(CBNode * self, CBPeer * peer);
bool CBNodeHeadersOnlyNewBranchCallback(void *, uint8_t branch, uint8_t parent, uint32_t blockHeight);
bool CBNodeHeadersOnlyNewValidBlock(void *, uint8_t branch, CBBlock * block, uint32_t blockHeight, bool last);
void CBNodeHeadersOnlyOnNetworkError(CBNetworkCommunicator * comm);
void CBNodeHeadersOnlyOnTimeOut(CBNetworkCommunicator * comm, void * peer, CBTimeOutType type);
/**
 @brief Frees data associated with the peer.
 @param self The CBNetworkCommunicator object.
 @param peer The peer
 */
void CBNodeHeadersOnlyPeerFree(CBNetworkCommunicator * self, CBPeer * peer);
/**
 @brief Setups the peer.
 @param self The CBNetworkCommunicator object.
 @param peer The peer
 */
void CBNodeHeadersOnlyPeerSetup(CBNetworkCommunicator * self, CBPeer * peer);
/**
 @brief Processes transactions, block headers, inventory broadcasts, header requests and data requests.
 @param self The CBNetworkCommunicator object.
 @param peer The peer
 @returns @see CBOnMessageReceivedAction.
 */
CBOnMessageReceivedAction CBNodeHeadersOnlyProcessMessage(CBNetworkCommunicator * self, CBPeer * peer);
bool CBNodeHeadersOnlyScanBlock(CBNode * self, CBBlock * block, uint32_t blockHeight, uint8_t branch);
/**
 @brief Sends data requested by an inventory broadcast.
 @param self The CBNetworkCommunicator object.
 @param peer The peer
 @returns true if the peer was disconnected, or false otherwise.
 */
bool CBNodeHeadersOnlySendRequestedData(CBNode * self, CBPeer * peer);
/**
 @brief Same as CBNodeSendRequestedData but takes void pointers and does not return anything.
 @param self The CBNetworkCommunicator object.
 @param peer The peer
 */
void CBNodeHeadersOnlySendRequestedDataVoid(void * self, void * peer);
/**
 @brief Compares two transactions by their hash
 @param tx1 The first transaction.
 @param tx2 The second transaction.
 @returns A CBCompare value corresponding to the transaction hashes.
 */
CBCompare CBTransactionCompare(CBAssociativeArray * foo, void * tx1, void * tx2);
/**
 @brief Compares a hash with a transaction's hash.
 @param hash A hash for a transaction.
 @param tx The transaction object
 @returns A CBCompare value corresponding to the transaction hashes.
 */
CBCompare CBTransactionAndHashCompare(CBAssociativeArray * foo, void * hash, void * tx);

#endif
