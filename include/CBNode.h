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
 @brief Implements a bitcoin node, either fully validating or headers-only.
 */

#ifndef CBHEADERSONLYNODEH
#define CBHEADERSONLYNODEH

#include "CBConstants.h"
#include "CBNetworkCommunicator.h"
#include "CBValidator.h"
#include "CBAccounter.h"

// Constants

typedef enum{
	CB_NODE_SCAN_TXS, /**< Scan blocks for transactions owned by the node. */
	CB_NODE_HEADERS_ONLY, /**< Runs a headers only node */
} CBNodeFlags;

/**
 @brief Structure for CBNode objects. @see CBNode.h
 */
typedef struct{
	CBNetworkCommunicator base; /**< Uses a CBNetworkCommunicator as the base structure. */
	uint64_t blockStorage; /**< The storage object for the validation information and block headers */
	CBValidator * validator; /**< Validator for headers only validation */
	uint64_t accounter; /**< The accounter object. */
	CBAssociativeArray ourTxs; /**< Unconfirmed transactions that belong to this node. */
	CBAssociativeArray otherTxs; /**< Unconfirmed transactions that are to be relayed but do not belong to this node. This is only used by a fully validating node. */
	uint32_t otherTxsSize; /**< The size of all of the transactions stored by otherTransactions. */
	uint32_t otherTxsSizeLimit; /**< The limit of the size for other transactions. Should be at least the maximum transaction size. */
	CBNodeFlags flags;
	CBAssociativeArray reRequest; /**< An array of the block hashes (20 bytes) needed again */
} CBNode;

/**
 @brief Creates a new CBNode object.
 @param dataDir The directory for the data to be stored.
 @param flags Flags for the node.
 @param otherTxsSizeLimit The limit of the size of the unconfirmed transactions to hold which are not owned by the accounts. Should be at least the maximum transaction size.
 @returns A new CBNode object.
 */
CBNode * CBNewNode(char * dataDir, CBNodeFlags flags, uint32_t otherTxsSizeLimit);

/**
 @brief Gets a CBNode from another object. Use this to avoid casts.
 @param self The object to obtain the CBNode from.
 @returns The CBNode object.
 */
CBNode * CBGetNode(void * self);

/**
 @brief Initialises a CBNode object
 @param self The CBNode object to initialise
 @param dataDir The directory for the data to be stored.
 @param flags Flags for the node
 @param otherTxsSizeLimit The limit of the size of the unconfirmed transactions to hold which are not owned by the accounts. Should be at least the maximum transaction size.
 @returns true on success, false on failure.
 */
bool CBInitNode(CBNode * self, char * dataDir, CBNodeFlags flags, uint32_t otherTxsSizeLimit);

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

/**
 @brief Adds a transaction to the other transaction array.
 @param self The CBNode object.
 @param tx The transaction.
 @returns true on success and false on failure.
 */
bool CBNodeAddOtherTransaction(CBNode * self, CBTransaction * tx);
/**
 @brief Checks the inputs for the unconfirmed transactions and lose transactions with missing inputs.
 @param self The CBNode object.
 @returns true on success and false on failure.
 */
bool CBNodeCheckInputs(CBNode * self);
/**
 @brief Checks the inputs (unless headers only) for the unconfirmed transactions and lose transactions with missing inputs, and then saves the scanner information and commits the accounter information to storage.
 @param self The CBNode object.
 @returns true on success and false on failure.
 */
bool CBNodeCheckInputsAndSave(CBNode * self);
/**
 @brief Processes the finding of a block on the main chain.
 @param self The CBNode object.
 @param block The block found.
 @param branch The branch of the block.
 @param blockIndex The block index in the branch.
 @returns true on success and false on failure.
 */
bool CBNodeFoundBlock(CBNode * self, CBBlock * block, uint8_t branch, uint32_t blockIndex);
/**
 @brief Frees data associated with the peer.
 @param self The CBNetworkCommunicator object.
 @param peer The peer
 */
void CBNodePeerFree(void * self, void * peer);
/**
 @brief Setups the peer.
 @param self The CBNetworkCommunicator object.
 @param peer The peer
 */
void CBNodePeerSetup(void * self, void * peer);
/**
 @brief Processes a block.
 @param self The CBNode object.
 @param block The block to process.
 @returns @see CBOnMessageReceivedAction.
 */
CBOnMessageReceivedAction CBNodeProcessBlock(CBNode * self, CBBlock * block);
/**
 @brief Processes transactions, block headers, inventory broadcasts, header requests and data requests.
 @param self The CBNetworkCommunicator object.
 @param peer The peer
 @returns @see CBOnMessageReceivedAction.
 */
CBOnMessageReceivedAction CBNodeProcessMessage(void * self, void * peer);
/**
 @brief Sends data requested by an inventory broadcast.
 @param self The CBNetworkCommunicator object.
 @param peer The peer
 @returns true if the peer was disconnected, or false otherwise.
 */
bool CBNodeSendRequestedData(CBNode * self, CBPeer * peer);
/**
 @brief Same as CBNodeSendRequestedData but takes void pointers and does not return anything.
 @param self The CBNetworkCommunicator object.
 @param peer The peer
 */
void CBNodeSendRequestedDataVoid(void * self, void * peer);
/**
 @brief Syncronises accounts with the validator.
 @param self The CBNode object.
 @param mainPath The path of the main chain.
 @param pathPoint The index of the main chain path, the fork appears.
 @param blockIndex The block index of the fork point.
 @returns true on success and false on failure.
 */
bool CBNodeSyncScan(CBNode * self, CBChainPath * mainPath, uint8_t pathPoint, uint32_t forkBlockIndex);
/**
 @brief Compares two transactions by their hash
 @param tx1 The first transaction.
 @param tx2 The second transaction.
 @returns A CBCompare value corresponding to the transaction hashes.
 */
CBCompare CBTransactionCompare(void * tx1, void * tx2);
/**
 @brief Compares a hash with a transaction's hash.
 @param hash A hash for a transaction.
 @param tx The transaction object
 @returns A CBCompare value corresponding to the transaction hashes.
 */
CBCompare CBTransactionAndHashCompare(void * hash, void * tx);

#endif
