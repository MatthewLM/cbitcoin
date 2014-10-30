//
//  CBNodeHeadersOnly.c
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

//  SEE HEADER FILE FOR DOCUMENTATION

#include "CBNodeHeadersOnly.h"

//  Constructor
/*
CBNode * CBNewNode(CBDepObject database, CBNodeFlags flags, uint32_t otherTxsSizeLimit, CBNodeCallbacks callbacks){
	CBNode * self = malloc(sizeof(*self));
	CBGetObject(self)->free = CBFreeNode;
	if (CBInitNode(self, database, flags, otherTxsSizeLimit, callbacks))
		return self;
	free(self);
	return NULL;
}

//  Initialiser

bool CBInitNode(CBNode * self, CBDepObject database, CBNodeFlags flags, uint32_t otherTxsSizeLimit, CBNodeCallbacks callbacks){
	self->flags = flags;
	self->otherTxsSizeLimit = otherTxsSizeLimit;
	self->otherTxsSize = 0;
	self->database = database;
	self->callbacks = callbacks;
	self->numberPeersDownload = 0;
	// Initialise network communicator
	CBInitNetworkCommunicator(CBGetNetworkCommunicator(self), (CBNetworkCommunicatorCallbacks){
		CBNodePeerSetup,
		CBNodePeerFree,
		CBNodeOnTimeOut,
		CBNodeAcceptType,
		CBNodeProcessMessage,
		CBNodeOnNetworkError
	});
	// Set network communicator fields.
	CBGetNetworkCommunicator(self)->flags = CB_NETWORK_COMMUNICATOR_AUTO_DISCOVERY | CB_NETWORK_COMMUNICATOR_AUTO_HANDSHAKE | CB_NETWORK_COMMUNICATOR_AUTO_PING;
	CBGetNetworkCommunicator(self)->version = CB_PONG_VERSION;
	// Initialise the storage objects
	if (CBNewBlockChainStorage(&self->blockChainStorage, database)) {
		if (CBNewAccounterStorage(&self->accounterStorage, database)) {
			// Initialise the validator
			self->validator = CBNewValidator(self->blockChainStorage,
											 (flags & CB_NODE_HEADERS_ONLY) ? CB_VALIDATOR_HEADERS_ONLY : 0,
											 (CBValidatorCallbacks){
												 CBNodeDeleteBranchCallback,
												 CBNodeNewBranchCallback,
												 CBNodeNewValidBlock,
												 CBNodeValidatorFinish,
												 CBNodeInvalidBlock,
												 CBNodeNoNewBranches,
												 CBNodeOnValidatorError
											 });
			if (self->validator){
				if (CBNodeStorageGetCurrentHeights(self)
					|| !(self->flags & CB_NODE_HEADERS_ONLY)) {
					if (!(self->flags & CB_NODE_SCAN_TXS)
						|| CBNodeStorageGetStartScanningTime(self)){
						// Initialise the array of required blocks
						CBInitAssociativeArray(&self->reqBlocks, CBHashCompare, NULL, free);
						CBInitAssociativeArray(&self->askedForBlocks, CBHashCompare, NULL, free);
						// Initialise the transaction arrays
						CBInitAssociativeArray(&self->ourTxs, CBTransactionCompare, NULL, CBReleaseObject);
						if (!(flags & CB_NODE_HEADERS_ONLY))
							// Only have other transactions if we are not a headers only node.
							CBInitAssociativeArray(&self->otherTxs, CBTransactionCompare, NULL, CBReleaseObject);
						return true;
					}else
						CBLogError("Could not obtain the time to start scanning.");
				}else
					CBLogError("Could not get the heights of blocks we next need for branches.");
				CBReleaseObject(self->validator);
			}else
				CBLogError("Could not create the validator object for a node");
			CBFreeAccounterStorage(self->accounterStorage);
		}else
			CBLogError("Could not create the accounter storage object for a node");
		CBFreeBlockChainStorage(self->blockChainStorage);
	}else
		CBLogError("Could not create the block storage object for a node");
	CBFreeNetworkCommunicator(self);
	return false;
}

//  Destructor

void CBDestroyNode(void * vself){
	CBNode * self = vself;
	CBFreeBlockChainStorage(self->blockChainStorage);
	CBFreeAccounterStorage(self->accounterStorage);
	CBReleaseObject(self->validator);
	CBFreeAssociativeArray(&self->reqBlocks);
	CBFreeAssociativeArray(&self->askedForBlocks);
	CBFreeAssociativeArray(&self->ourTxs);
	if (!(self->flags & CB_NODE_HEADERS_ONLY))
		CBFreeAssociativeArray(&self->otherTxs);
	CBDestroyNetworkCommunicator(vself);
}
void CBFreeNode(void * self){
	CBDestroyNode(self);
	free(self);
}

//  Functions

CBCompare CBHashCompare(CBAssociativeArray * foo, void * hash1, void * hash2){
	int res = memcmp((uint8_t *)hash1, (uint8_t *)hash2, 20);
	if (res > 0)
		return CB_COMPARE_MORE_THAN;
	if (res == 0)
		return CB_COMPARE_EQUAL;
	return CB_COMPARE_LESS_THAN;
}
bool CBNodeAcceptType(CBNetworkCommunicator * comm, CBPeer * peer, CBMessageType type){
	if (type == CB_MESSAGE_TYPE_BLOCK) {
		// Only accept block in response to getdata
		if (!peer->expectBlock)
			return false;
	}else if (type == CB_MESSAGE_TYPE_GETDATA){
		// Only accept getdata if we have advertised but not sent data
		if (!peer->advertisedData)
			return false;
	}else if (type == CB_MESSAGE_TYPE_TX){
		// Only accept tx in response to getdata
		if (!peer->expectedTxs.root->numElements)
			return false;
	}else if (type == CB_MESSAGE_TYPE_HEADERS){
		// Only accept headers in response to getheaders
		if (!peer->expectHeaders)
			return false;
	}
	// Accept all other types
	return true;
}
void CBNodeAddOtherTransaction(CBNode * self, CBTransaction * tx){
	CBPosition pos;
	pos.index = 0;
	pos.node = self->otherTxs.root;
	// Continually delete transactions until we have space.
	while (self->otherTxsSize + CBGetMessage(tx)->bytes->length > self->otherTxsSizeLimit)
		CBAssociativeArrayDelete(&self->otherTxs, pos, true);
	// Insert new transaction
	CBAssociativeArrayInsert(&self->otherTxs, tx, CBAssociativeArrayFind(&self->otherTxs, tx).position, NULL);
}
bool CBNodeCheckInputs(CBNode * self){
	CBPosition it;
	for (CBAssociativeArray * array = &self->otherTxs;; array = &self->ourTxs){
		if (CBAssociativeArrayGetFirst(array, &it)) for (;;) {
			// Loop through inputs ensuring the outputs exist for them.
			CBTransaction * tx = it.node->elements[it.index];
			bool iterate = true;
			for (uint32_t x = 0; x < tx->inputNum; x++) {
				CBErrBool res = CBBlockChainStorageUnspentOutputExists(self->validator, CBByteArrayGetData(tx->inputs[x]->prevOut.hash), tx->inputs[x]->prevOut.index);
				if (res == CB_ERROR) {
					CBLogError("Could not determine is an unspent output exists.");
					return false;
				}
				if (res == CB_FALSE){
					// There exists no unspent outputs in the block-chain storage for this transaction input. Therefore this transaction is no longer valid.
					// Lose the transaction from the accounter if it is ours
					if (array == &self->ourTxs && !CBAccounterLostUnconfirmedTransaction(self->accounterStorage, tx)) {
						CBLogError("Could not lose a transaction with missing inputs as one of our unconfirmed transactions.");
						return false;
					}
					// Delete the transaction
					CBAssociativeArrayDelete(array, it, false);
					// Find new position for iterator
					it = CBAssociativeArrayFind(array, tx).position;
					// Finally release transaction
					CBReleaseObject(tx);
					// Do not iterate
					iterate = false;
					break;
				}
			}
			if (iterate && CBAssociativeArrayIterate(array, &it))
				break;
		}
		if (array == &self->ourTxs)
			break;
	}
	return true;
}
bool CBNodeDeleteBranchCallback(void * vnode, uint8_t branch){
	CBNode * node = vnode;
	if (!CBAccounterDeleteBranch(node->accounterStorage, branch)){
		CBLogError("Could not delete the accounter branch information.");
		return false;
	}
	return true;
}
bool CBNodeGetBlockForTxs(CBNode * self, CBPeer * peer){
	uint32_t blockIndex = self->currentHeights[self->currentBranch] - self->validator->branches[self->currentBranch].startHeight;
	if (blockIndex <= self->validator->branches[self->currentBranch].lastValidation) {
		// Only getting block that has been validated
		uint8_t hash[32];
		// Get the hash
		if (!CBBlockChainStorageGetBlockHash(self->validator, self->currentBranch, self->currentHeights[self->currentBranch] - self->validator->branches[self->currentBranch].startHeight, hash)){
			CBLogError("Could not get the hash of the block we want for transactions.");
			return false;
		}
		peer->expectBlock = true;
		memcpy(peer->expectedBlock, hash, 20);
		CBInventory * getData = CBNewInventory();
		CBByteArray * hashBa = CBNewByteArrayWithDataCopy(hash, 32);
		CBInventoryTakeInventoryItem(getData, CBNewInventoryItem(CB_INVENTORY_ITEM_BLOCK, hashBa));
		CBReleaseObject(hash);
		CBGetMessage(getData)->type = CB_MESSAGE_TYPE_GETDATA;
		CBNetworkCommunicatorSendMessage(CBGetNetworkCommunicator(self), peer, CBGetMessage(getData), NULL);
		CBReleaseObject(getData);
	}
	return true;
}
bool CBNodeNewBranchCallback(void * vnode, uint8_t branch, uint8_t parent, uint32_t blockHeight){
	CBNode * node = vnode;
	if (!CBAccounterNewBranch(node->accounterStorage, branch, parent, blockHeight)){
		CBLogError("Could not create new accounter branch information.");
		return false;
	}
	return true;
}
bool CBNodeNewValidBlock(void * vpeer, uint8_t branch, CBBlock * block, uint32_t blockHeight, bool last){
	// Received newly validated block
	CBPeer * peer = vpeer;
	CBNode * self = peer->nodeObj;
	// Only scan block for transactions if CB_NODE_SCAN_TXS is set
	if (self->flags & CB_NODE_SCAN_TXS)
		return true;
	// Check to see if we are scanning yet
	if (block->time >= self->startScanning) {
		// If we are headers only, request the next full block for transactions.
		if (self->flags & CB_NODE_HEADERS_ONLY
			&& !peer->expectBlock)
			return CBNodeGetBlockForTxs(self, peer);
		// Scan the block for transactions
		if (!CBNodeScanBlock(self, block, blockHeight, branch)) {
			CBLogError("Could not scan a block for transactions.");
			return false;
		}
	}
	return true;
}
void CBNodeOnNetworkError(CBNetworkCommunicator * comm){
	return;
}
void CBNodeOnTimeOut(CBNetworkCommunicator * comm, void * peer, CBTimeOutType type){
	return;
}
void CBNodePeerFree(CBNetworkCommunicator * self, CBPeer * peer){
	CBFreeAssociativeArray(&peer->expectedTxs);
	CBReleaseObject(peer->requestedData);
}
void CBNodePeerSetup(CBNetworkCommunicator * self, CBPeer * peer){
	CBInitAssociativeArray(&peer->expectedTxs, CBHashCompare, NULL, free);
	peer->expectBlock = false;
	peer->expectHeaders = false;
	peer->requestedData = NULL;
	peer->reqBlockNum = 0;
	peer->reqBlockCursor = 0;
	peer->expectBlockInv = 0;
	peer->nodeObj = self;
}
CBOnMessageReceivedAction CBNodeProcessMessage(CBNetworkCommunicator * comm, CBPeer * peer){
	CBNode * self = CBGetNode(comm);
	bool full = !(self->flags & CB_NODE_HEADERS_ONLY);
	switch (peer->receive->type) {
		case CB_MESSAGE_TYPE_VERSION:{
			// Ask for blocks or headers, unless there is already CB_MAX_BLOCK_QUEUE or 1 (if headers only) peers giving us blocks.
			if (self->numberPeersDownload++ == (full ? CB_MAX_BLOCK_QUEUE : 1))
				break;
			CBChainDescriptor * chainDesc = CBValidatorGetChainDescriptor(self->validator);
			if (chainDesc == NULL)
				return CBNodeReturnError(self, "There was an error when trying to retrieve the block-chain descriptor.");
			CBGetBlocks * getBlocks = CBNewGetBlocks(CB_MIN_PROTO_VERSION, chainDesc, NULL);
			CBReleaseObject(chainDesc);
			CBGetMessage(getBlocks)->type = full ? CB_MESSAGE_TYPE_GETBLOCKS : CB_MESSAGE_TYPE_GETHEADERS;
			CBNetworkCommunicatorSendMessage(CBGetNetworkCommunicator(self), peer, CBGetMessage(getBlocks), NULL);
			CBReleaseObject(getBlocks);
			break;
		}
		case CB_MESSAGE_TYPE_BLOCK:{
			// The peer sent us a block
			CBBlock * block = CBGetBlock(peer->receive);
			// See if we expected this block
			if (memcmp(peer->expectedBlock, CBBlockGetHash(block), 20))
				// We did not expect this block from this peer.
				return CB_MESSAGE_ACTION_DISCONNECT;
			if (full) {
				// Do the basic validation
				CBBlockValidationResult result = CBValidatorBasicBlockValidation(self->validator, block, CBNetworkAddressManagerGetNetworkTime(CBGetNetworkCommunicator(self)->addresses));
				switch (result) {
					case CB_BLOCK_VALIDATION_BAD:
						return CB_MESSAGE_ACTION_DISCONNECT;
					case CB_BLOCK_VALIDATION_OK:
						// Queue the block for processing
						CBValidatorQueueBlock(self->validator, block, peer);
					default:
						return CB_MESSAGE_ACTION_CONTINUE;
				}
			}else{
				// Check the merkle root of the block
				uint8_t merkleRoot = CBBlockCalculateMerkleRoot(block);
				if (memcmp(merkleRoot, CBByteArrayGetData(block->merkleRoot), 32))
					// Bad merkle root
					return CB_MESSAGE_ACTION_DISCONNECT;
				// We need to scan the block.
				if (!CBNodeScanBlock(self, block, self->currentHeights[self->currentBranch]++, self->currentBranch))
					return CBNodeReturnError(self, "Could not scan a block for our transactions.");
				// Save the height of the next full block for this branch
				HERE
				// Get next block
				if (!CBNodeGetBlockForTxs(self, peer))
					return CBNodeReturnError(self, "Could not ask for the next block for transactions.");
			}
		}
		case CB_MESSAGE_TYPE_GETBLOCKS:
		case CB_MESSAGE_TYPE_GETHEADERS:{
			// The peer is requesting headers or blocks
			bool full = peer->receive->type == CB_MESSAGE_TYPE_GETBLOCKS;
			CBGetBlocks * getBlocks = CBGetGetBlocks(peer->receive);
			CBChainDescriptor * chainDesc = getBlocks->chainDescriptor;
			if (chainDesc->hashNum == 0)
				// Why do this?
				return CB_MESSAGE_ACTION_DISCONNECT;
			uint8_t branch;
			uint32_t blockIndex;
			// Go though the chain descriptor until a hash is found that we own
			bool found = false;
			for (uint16_t x = 0; x < chainDesc->hashNum; x++) {
				CBErrBool exists = CBBlockChainStorageGetBlockLocation(self->validator, CBByteArrayGetData(chainDesc->hashes[x]), &branch, &blockIndex);
				if (exists == CB_ERROR)
					return CBNodeReturnError(self, "Could not look for block with chain descriptor hash.");
				if (exists == CB_TRUE){
					// We have a block that we own.
					found = true;
					break;
				}
			}
			// Get the chain path for the main chain
			CBChainPath mainChainPath = CBValidatorGetMainChainPath(self->validator);
			CBChainPathPoint intersection;
			if (found){
				// Get the chain path for the block header we have found
				CBChainPath peerChainPath = CBValidatorGetChainPath(self->validator, branch, blockIndex);
				// Determine where the intersection is on the main chain
				intersection = CBValidatorGetChainIntersection(&mainChainPath, &peerChainPath);
				// Check that the intersection is above the block index of the last block given for this branch
				if (intersection.blockIndex < peer->lastBlockIndex[mainChainPath.points[intersection.chainPathIndex].branch])
					return CB_MESSAGE_ACTION_DISCONNECT;
			}else{
				// Start at block 1
				intersection.blockIndex = 1;
				intersection.chainPathIndex = mainChainPath.numBranches - 1;
			}
			CBMessage * message;
			// Now provide headers from this intersection up to 2000 blocks or block inventory items up to 500, the last one we have or the stopAtHash.
			for (uint8_t x = 0; x < (full ? 500 : 2000); x++) {
				// Check to see if we reached the last block in the main chain.
				if (intersection.chainPathIndex == 0
					&& intersection.blockIndex == mainChainPath.points[0].blockIndex) {
					if (x == 0)
						// The intersection is at the last block. The peer is up-to-date with us
						return CB_MESSAGE_ACTION_CONTINUE;
					break;
				}
				// Move to the next block
				if (intersection.blockIndex == mainChainPath.points[intersection.chainPathIndex].blockIndex) {
					// Move to next branch
					intersection.chainPathIndex--;
					intersection.blockIndex = 0;
				}else{
					// Move to the next block
					intersection.blockIndex++;
				}
				// Get the hash
				uint8_t hash[32];
				if (! CBBlockChainStorageGetBlockHash(self->validator, mainChainPath.points[intersection.chainPathIndex].branch, intersection.blockIndex, hash)) {
					if (x != 0) CBReleaseObject(message);
					return CBNodeReturnError(self, "Could not obtain a hash for a block.");
				}
				// Check to see if we are at stopAtHash
				if (memcmp(hash, CBByteArrayGetData(getBlocks->stopAtHash), 32) == 0){
					if (x == 0)
						// The stop at hash was the next block wanted which makes no sense
						return CB_MESSAGE_ACTION_DISCONNECT;
					break;
				}
				if (x == 0){
					// Create inventory or block headers object to send to peer.
					if (full) {
						message = CBGetMessage(CBNewInventory());
						message->type = CB_MESSAGE_TYPE_INV;
					}else{
						message = CBGetMessage(CBNewBlockHeaders());
						message->type = CB_MESSAGE_TYPE_HEADERS;
					}
				}
				// Add block header or add inventory item
				if (full) {
					CBByteArray * hashObj = CBNewByteArrayWithDataCopy(hash, 32);
					CBInventoryTakeInventoryItem(CBGetInventory(message), CBNewInventoryItem(CB_INVENTORY_ITEM_BLOCK, hashObj));
					CBReleaseObject(hashObj);
				}else
					CBBlockHeadersTakeBlockHeader(CBGetBlockHeaders(message), CBBlockChainStorageGetBlockHeader(self->validator, mainChainPath.points[intersection.chainPathIndex].branch, intersection.blockIndex));
			}
			if (full)
				// If we advertised blocks, add to the advertised number
				peer->advertisedData += CBGetInventory(message)->itemNum;
			else
				// Else the peer will also know about the headers and may ask for blocks
				peer->advertisedData += CBGetBlockHeaders(message)->headerNum;
			// Record the block index of the last block hash given for this branch
			peer->lastBlockIndex[mainChainPath.points[intersection.chainPathIndex].branch] = intersection.blockIndex;
			// Send the message
			CBNetworkCommunicatorSendMessage(CBGetNetworkCommunicator(self), peer, message, NULL);
			CBReleaseObject(message);
			break;
		}
		case CB_MESSAGE_TYPE_GETDATA:{
			// The peer is requesting data. It should only be requesting transactions or blocks that we own.
			CBInventory * getData = CBGetInventory(peer->receive);
			// Ensure it is requesting no more data than we have advertised and not satisfied
			if (getData->itemNum > peer->advertisedData)
				return CB_MESSAGE_ACTION_DISCONNECT;
			// Begin sending requested data, or add the requested data to the eisting requested data.
			if (peer->requestedData == NULL) {
				peer->sendDataIndex = 0;
				peer->requestedData = getData;
				CBRetainObject(peer->requestedData);
				if (CBNodeSendRequestedData(self, peer))
					// Return since the node was stoped.
					return CB_MESSAGE_ACTION_RETURN;
			}else{
				// Add to existing requested data
				uint16_t addNum = getData->itemNum;
				if (addNum + peer->requestedData->itemNum > 50000)
					// Adjust to give no more than 50000
					addNum = 50000 - peer->requestedData->itemNum;
				for (uint16_t x = 0; x < getData->itemNum; x++)
					CBInventoryAddInventoryItem(peer->requestedData, CBInventoryGetInventoryItem(getData, x));
			}
			break;
		}
		case CB_MESSAGE_TYPE_HEADERS:{
			// The peer has headers for us.
			CBBlockHeaders * headers = CBGetBlockHeaders(peer->receive);
			if (headers->headerNum == 0)
				// The peer has no more headers for us, so we end here
				return CB_MESSAGE_ACTION_CONTINUE;
			// Do basic validation on all headers
			for (uint16_t x = 0; x < headers->headerNum; x++) {
				CBBlockValidationResult result = CBValidatorBasicBlockValidation(self->validator, headers->blockHeaders[x], CBNetworkAddressManagerGetNetworkTime(CBGetNetworkCommunicator(self)->addresses));
				switch (result) {
					case CB_BLOCK_VALIDATION_BAD:
						return CB_MESSAGE_ACTION_DISCONNECT;
					case CB_BLOCK_VALIDATION_OK:
						break;
					default:
						return CB_MESSAGE_ACTION_CONTINUE;
				}
			}
			// Queue the first header for processing
			CBValidatorQueueBlock(self->validator, headers->blockHeaders[0], peer);
			// Save the headers message
			self->headers = headers;
			CBRetainObject(headers);
			self->headerIndex = 0;
			MOVE THIS TO VALIDATOR CALLBACK;
			// Ask for new headers from this peer unless, we are to disconnect it
			CBByteArray * nullHash = CBNewByteArrayOfSize(32);
			memset(CBByteArrayGetData(nullHash), 0, 32);
			CBChainDescriptor * chainDesc = CBValidatorGetChainDescriptor(self->validator);
			CBGetBlocks * getHeaders = CBNewGetBlocks(1, chainDesc, nullHash);
			CBGetMessage(getHeaders)->type = CB_MESSAGE_TYPE_GETHEADERS;
			CBReleaseObject(chainDesc);
			CBReleaseObject(nullHash);
			CBNetworkCommunicatorSendMessage(CBGetNetworkCommunicator(self), peer, CBGetMessage(getHeaders), NULL);
			CBReleaseObject(getHeaders);
		}
		case CB_MESSAGE_TYPE_INV:{
			// The peer has sent us an inventory broadcast
			CBInventory * inv = CBGetInventory(peer->receive);
			// Get data request for objects we want.
			CBInventory * getData = NULL;
			for (uint16_t x = 0; x < inv->itemNum; x++) {
				CBInventoryItem * item = CBInventoryGetInventoryItem(inv, x);
				uint8_t * hash = CBByteArrayGetData(item->hash);
				if (item->type == CB_INVENTORY_ITEM_BLOCK) {
					if (peer->reqBlockNum != 0 && peer->expectBlockInv > time(NULL) - 10)
						// We are still obtaining the required blocks from this peer, do not accept any unsolicited blocks until we are done getting the ones we have asked for.
						continue;
					// Check if we have the block
					CBErrBool exists = CBBlockChainStorageBlockExists(self->validator, hash);
					if (exists == CB_ERROR)
						return CBNodeReturnError(self, "Could not determine if we have a block in an inventory message.");
					if (exists == CB_TRUE)
						// Ignore this one.
						continue;
					// Ensure we have not asked for this block yet.
					CBFindResult res = CBAssociativeArrayFind(&self->askedForBlocks, hash);
					if (res.found)
						continue;
					// Add block to required blocks from peer
					memcpy(peer->reqBlocks[peer->reqBlockNum++], hash, 20);
					CBGetBlockInfo * getBlkInfo;
					if (peer->reqBlockNum == 0) {
						// Ask for first block
						memcpy(peer->expectedBlock, hash, 20);
						peer->expectBlock = true;
						CBInventoryAddInventoryItem(getData, item);
						// Record that this block has been asked for
						getBlkInfo = malloc(sizeof(*getBlkInfo));
						memcpy(getBlkInfo->hash, hash, 20);
						// Will will not require this from any other peers.
						getBlkInfo->numPeers = 0;
						CBAssociativeArrayInsert(&self->askedForBlocks, getBlkInfo, res.position, NULL);
					}else{
						// Record that this block is required
						CBFindResult res = CBAssociativeArrayFind(&self->reqBlocks, hash);
						if (res.found) {
							// Previously found as required. Increase the number of peers we can get this block from.
							getBlkInfo = CBFindResultToPointer(res);
							getBlkInfo->numPeers++;
						}else{
							// Add new required block
							getBlkInfo = malloc(sizeof(*getBlkInfo));
							memcpy(getBlkInfo->hash, hash, 20);
							getBlkInfo->numPeers = 1;
							CBAssociativeArrayInsert(&self->reqBlocks, getBlkInfo, res.position, NULL);
						}
					}
				}else if (item->type == CB_INVENTORY_ITEM_TX){
					// Check if we have the transaction
					self->ourTxs.compareFunc = CBTransactionAndHashCompare;
					CBFindResult res = CBAssociativeArrayFind(&self->ourTxs, hash);
					self->ourTxs.compareFunc = CBTransactionCompare;
					if (res.found)
						continue;
					if (full) {
						self->otherTxs.compareFunc = CBTransactionAndHashCompare;
						CBFindResult res = CBAssociativeArrayFind(&self->otherTxs, hash);
						self->otherTxs.compareFunc = CBTransactionCompare;
						if (res.found)
							continue;
						CBErrBool exists = CBBlockChainStorageTransactionExists(self->validator, hash);
						if (exists == CB_ERROR)
							return CBNodeReturnError(self, "Could not determine if we have a transaction (blockchain) from an inventory message.");
						if (exists == CB_TRUE)
							continue;
					}else{
						// Since we do not keep all transactions, check with the accounter
						CBErrBool exists = CBAccounterTransactionExists(self->accounterStorage, hash);
						if (exists == CB_ERROR)
							return CBNodeReturnError(self, "Could not determine if we have a transaction (accounter) from an inventory message.");
						if (exists == CB_TRUE)
							continue;
					}
					// We do not have the transaction, so add it to getdata
					if (getData == NULL)
						getData = CBNewInventory();
					CBInventoryAddInventoryItem(getData, item);
				}else continue;
			}
			// Send getdata if not NULL
			CBGetMessage(getData)->type = CB_MESSAGE_TYPE_GETDATA;
			if (getData != NULL)
				CBNetworkCommunicatorSendMessage(CBGetNetworkCommunicator(self), peer, CBGetMessage(getData), NULL);
		}
		default:
			break;
	}
	return CB_MESSAGE_ACTION_CONTINUE;
}
CBOnMessageReceivedAction CBNodeReturnError(CBNode * self, char * err){
	CBLogError(err);
	self->callbacks.onFatalNodeError(self);
	CBFreeNode(self);
	return CB_MESSAGE_ACTION_RETURN;
}
bool CBNodeScanBlock(CBNode * self, CBBlock * block, uint32_t blockHeight, uint8_t branch){
	// Look through each transaction
	for (uint32_t x = 0; x < block->transactionNum; x++) {
		// Check to see if the transaction was in the otherTxs array
		CBFindResult res = CBAssociativeArrayFind(&self->otherTxs, block->transactions[x]);
		if (res.found){
			// Remove the transaction from the array as it is now in a block.
			CBAssociativeArrayDelete(&self->otherTxs, res.position, true);
			// Decrease the size of the other transactions
			self->otherTxsSize -= CBGetMessage(block->transactions[x])->bytes->length;
			continue;
		}
		// Check to see if the transaction was in in the ourTxs array.
		res = CBAssociativeArrayFind(&self->ourTxs, block->transactions[x]);
		if (res.found) {
			// Remove the transaction from the array as it is now in a block.
			CBAssociativeArrayDelete(&self->ourTxs, res.position, true);
			// Move the unconfirmed transaction accounter infomation to the branch.
			if (!CBAccounterUnconfirmedTransactionToBranch(self->accounterStorage, block->transactions[x], blockHeight, branch)){
				CBLogError("Could not move a branchless transaction's account information to a branch.");
				return false;
			}
		}else{
			// Add transaction to accounter information
			if (!CBAccounterFoundTransaction(self->accounterStorage, block->transactions[x], blockHeight, block->time, branch)) {
				CBLogError("Could not add account information for a transaction found in a block.");
				return false;
			}
		}
	}
}
bool CBNodeSendRequestedData(CBNode * self, CBPeer * peer){
	// See if the request has been satisfied
	if (peer->sendDataIndex == peer->requestedData->itemNum) {
		// We have sent everything
		CBReleaseObject(peer->requestedData);
		peer->requestedData = NULL;
		return false;
	}
	// Get this item
	CBInventoryItem * item = CBInventoryGetInventoryItem(peer->requestedData, peer->sendDataIndex);
	switch (item->type) {
		case CB_INVENTORY_ITEM_TX:{
			// First look for our transactions
			// Change the comparison function to accept a hash in place of a transaction object.
			self->ourTxs.compareFunc = CBTransactionAndHashCompare;
			CBFindResult res = CBAssociativeArrayFind(&self->ourTxs, CBByteArrayGetData(item->hash));
			// Change the comparison function back
			self->ourTxs.compareFunc = CBTransactionCompare;
			if (res.found)
				// The peer was requesting one of our transactions
				CBNetworkCommunicatorSendMessage(CBGetNetworkCommunicator(self), peer, CBFindResultToPointer(res), CBNodeSendRequestedDataVoid);
			else{
				// Now check the other transactions
				self->otherTxs.compareFunc = CBTransactionAndHashCompare;
				res = CBAssociativeArrayFind(&self->otherTxs, CBByteArrayGetData(item->hash));
				self->otherTxs.compareFunc = CBTransactionCompare;
				if (res.found)
					// The peer was requesting another transaction
					CBNetworkCommunicatorSendMessage(CBGetNetworkCommunicator(self), peer, CBFindResultToPointer(res), CBNodeSendRequestedDataVoid);
				else{
					// The peer was requesting something we do not have!
					CBNetworkCommunicatorDisconnect(CBGetNetworkCommunicator(self), peer, CB_24_HOURS, false);
					return true;
				}
			}
			break;
		}
		case CB_INVENTORY_ITEM_BLOCK:{
			// Ensure that this is not a headers-only node
			if (self->flags & CB_NODE_HEADERS_ONLY) {
				CBNetworkCommunicatorDisconnect(CBGetNetworkCommunicator(self), peer, CB_24_HOURS, false);
				return true;
			}
			// Look for the block to give
			uint8_t branch;
			uint32_t index;
			CBErrBool exists = CBBlockChainStorageGetBlockLocation(self->validator, CBByteArrayGetData(item->hash), &branch, &index);
			if (exists == CB_ERROR) {
				CBLogError("Could not get the location for a block when a peer requested it of us.");
				self->callbacks.onFatalNodeError(self);
				CBFreeNode(self);
				return true;
			}
			if (exists == CB_FALSE) {
				// The peer is requesting a block we do not have. Obviously we may have removed the block since, but this is unlikely
				CBNetworkCommunicatorDisconnect(CBGetNetworkCommunicator(self), peer, CB_24_HOURS, false);
				return true;
			}
			// Load the block from storage
			CBBlock * block = CBBlockChainStorageLoadBlock(self->validator, index, branch);
			if (block == NULL) {
				CBLogError("Could not load a block from storage.");
				self->callbacks.onFatalNodeError(self);
				CBFreeNode(self);
				return true;
			}
			CBGetMessage(block)->type = CB_MESSAGE_TYPE_BLOCK;
			CBNetworkCommunicatorSendMessage(CBGetNetworkCommunicator(self), peer, CBGetMessage(block), CBNodeSendRequestedDataVoid);
		}
		case CB_INVENTORY_ITEM_ERROR:
			return false;
	}
	// If the item index is 249, then release the inventory items, move the rest down and then reduce the item number
	if (peer->sendDataIndex == 249) {
		for (uint16_t x = 0; x < 249; x++)
			CBReleaseObject(peer->requestedData->items[0][x]);
		memmove(peer->requestedData->items, peer->requestedData->items + sizeof(*peer->requestedData->items), (peer->requestedData->itemNum / 250) * sizeof(*peer->requestedData->items));
		peer->requestedData->itemNum -= 250;
	}
	// Now move to the next piece of data
	peer->sendDataIndex++;
	// Lower the unsatisfied advertised data number
	peer->advertisedData--;
	return false;
}
void CBNodeSendRequestedDataVoid(void * self, void * peer){
	CBNodeSendRequestedData(self, peer);
}
CBCompare CBTransactionCompare(CBAssociativeArray * foo, void * tx1, void * tx2){
	CBTransaction * tx1Obj = tx1;
	CBTransaction * tx2Obj = tx2;
	int res = memcmp(CBTransactionGetHash(tx1Obj), CBTransactionGetHash(tx2Obj), 32);
	if (res > 0)
		return CB_COMPARE_MORE_THAN;
	if (res == 0)
		return CB_COMPARE_EQUAL;
	return CB_COMPARE_LESS_THAN;
}
CBCompare CBTransactionAndHashCompare(CBAssociativeArray * foo,void * hash, void * tx){
	CBTransaction * txObj = tx;
	int res = memcmp((uint8_t *)hash, CBTransactionGetHash(txObj), 32);
	if (res > 0)
		return CB_COMPARE_MORE_THAN;
	if (res == 0)
		return CB_COMPARE_EQUAL;
	return CB_COMPARE_LESS_THAN;
}

REMEMBER ABOUT NODES BRANCH WORKING. ENSURE SAME BRANCH OF BLOCKS SINCE LAST GETBLOCKS
HEADERS ONLY THINK ABOUT GETTING TXS FROM BLOCKS
GET DATA TIMEOUT FOR BLOCKS?
TIMEOUT WAITING FOR BLOCKS?
CHECK RETURN OF CBNetworkCommunicatorSendMessage?
EVERYTHING THAT REQUIRES STORAGE ACCESS MAKE INTO THREAD?
ADD MUTEXES
 HEADERS ONLY THINK ABOUT GETTING TXS FROM BLOCKS*/
