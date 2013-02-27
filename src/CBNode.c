//
//  CBNode.c
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

//  SEE HEADER FILE FOR DOCUMENTATION

#include "CBNode.h"

//  Constructor

/*CBNode * CBNewNode(char * dataDir, CBNodeFlags flags, uint32_t otherTxsSizeLimit){
	CBNode * self = malloc(sizeof(*self));
	if (NOT self) {
		CBLogError("Cannot allocate %i bytes of memory in CBNewNode\n", sizeof(*self));
		return NULL;
	}
	CBGetObject(self)->free = CBFreeNode;
	if (CBInitNode(self, dataDir, flags, otherTxsSizeLimit))
		return self;
	free(self);
	return NULL;
}

//  Object Getter

CBNode * CBGetNode(void * self){
	return self;
}

//  Initialiser

bool CBInitNode(CBNode * self, char * dataDir, CBNodeFlags flags, uint32_t otherTxsSizeLimit){
	// Set flags and other transactions size limit
	self->flags = flags;
	self->otherTxsSizeLimit = otherTxsSizeLimit;
	// Initialise the validator and storage object
	self->blockStorage = CBNewBlockChainStorage(dataDir);
	if (NOT self->blockStorage) {
		CBLogError("Could not create the block storage object for a headers only node");
		return false;
	}
	self->validator = CBNewValidator(self->blockStorage, (flags & CB_NODE_HEADERS_ONLY) ? CB_VALIDATOR_HEADERS_ONLY : 0);
	if (NOT self->validator) {
		CBFreeBlockChainStorage(self->blockStorage);
		CBLogError("Could not create the validator object for a headers only node");
		return false;
	}
	// Initialise network communicator
	if (NOT CBInitNetworkCommunicator(CBGetNetworkCommunicator(self))){
		CBFreeBlockChainStorage(self->blockStorage);
		CBReleaseObject(self->validator);
		return false;
	}
	// Initialise the transaction arrays
	if (NOT CBInitAssociativeArray(&self->ourTxs, CBTransactionCompare, CBReleaseObject)) {
		CBFreeBlockChainStorage(self->blockStorage);
		CBReleaseObject(self->validator);
		CBDestroyNetworkCommunicator(self);
		CBLogError("Could not initialise the transaction array for our transactions for a headers only node");
		return false;
	}
	if (flags & CB_NODE_HEADERS_ONLY) {
		// Only have re-requests for headers-only nodes.
		if (NOT CBInitAssociativeArray(&self->reRequest, CBKeyCompare, free)) {
			CBFreeBlockChainStorage(self->blockStorage);
			CBReleaseObject(self->validator);
			CBFreeAssociativeArray(&self->ourTxs);
			CBFreeAssociativeArray(&self->otherTxs);
			CBDestroyNetworkCommunicator(self);
			CBLogError("Could not initialise the transaction array for other transactions for a headers only node");
			return false;
		}
	}else{
		// Only have other transactions if we are not a headers only node.
		if (NOT CBInitAssociativeArray(&self->otherTxs, CBTransactionCompare, CBReleaseObject)) {
			CBFreeBlockChainStorage(self->blockStorage);
			CBReleaseObject(self->validator);
			CBFreeAssociativeArray(&self->ourTxs);
			CBFreeAssociativeArray(&self->otherTxs);
			CBDestroyNetworkCommunicator(self);
			CBLogError("Could not initialise the transaction array for other transactions for a headers only node");
			return false;
		}
	}
	// Set network communicator fields.
	CBGetNetworkCommunicator(self)->flags = CB_NETWORK_COMMUNICATOR_AUTO_DISCOVERY | CB_NETWORK_COMMUNICATOR_AUTO_HANDSHAKE | CB_NETWORK_COMMUNICATOR_AUTO_PING;
	CBGetNetworkCommunicator(self)->version = CB_PONG_VERSION;
	CBNetworkCommunicatorSetAlternativeMessages(CBGetNetworkCommunicator(self), NULL, NULL);
	// Set callbacks
	CBGetNetworkCommunicator(self)->onMessageReceived = CBNodeProcessMessage;
	CBGetNetworkCommunicator(self)->onPeerConnection = CBNodePeerSetup;
	CBGetNetworkCommunicator(self)->onPeerDisconnection = CBNodePeerFree;
	// Set size of the other transactions array
	self->otherTxsSize = 0;
	// Get the scanning info
	if (NOT CBAccounterStorageGetScanningInfo(self->accounter, &self->scanInfo)) {
		CBLogError("Could not get the scanning info for a headers-only node.");
		CBDestroyNode(self);
		return false;
	}
	// Process unprocessed blocks
	// Get the main chain path
	CBChainPath mainPath = CBValidatorGetMainChainPath(self->validator);
	// Get the chain path for the scanner
	CBChainPath scanPath = CBValidatorGetChainPath(self->validator, self->scanInfo.lastBranch, self->scanInfo.lastBlockIndex);
	// Get intersection
	CBChainPathPoint point = CBValidatorGetChainIntersection(&mainPath, &scanPath);
	if (NOT CBNodeSyncScan(self, &mainPath, point.chainPathIndex, point.blockIndex)) {
		CBLogError("Failure when loading a node: Could not synchronise the validator with the accounter.");
		CBDestroyNode(self);
		return false;
	}
	return true;
}

//  Destructor

void CBDestroyNode(void * vself){
	CBNode * self = vself;
	CBFreeBlockChainStorage(self->blockStorage);
	CBReleaseObject(self->validator);
	CBFreeAssociativeArray(&self->ourTxs);
	if (NOT (self->flags & CB_NODE_HEADERS_ONLY))
		CBFreeAssociativeArray(&self->otherTxs);
	else
		CBFreeAssociativeArray(&self->reRequest);
	CBDestroyNetworkCommunicator(vself);
}
void CBFreeNode(void * self){
	CBDestroyNode(self);
	free(self);
}

//  Functions

bool CBNodeAddOtherTransaction(CBNode * self, CBTransaction * tx){
	CBPosition pos;
	pos.index = 0;
	pos.node = self->otherTxs.root;
	// Continually delete transactions until we have space.
	while (self->otherTxsSize + CBGetMessage(tx)->bytes->length > self->otherTxsSizeLimit)
		CBAssociativeArrayDelete(&self->otherTxs, pos, true);
	// Insert new transaction
	if (NOT CBAssociativeArrayInsert(&self->otherTxs, tx, CBAssociativeArrayFind(&self->otherTxs, tx).position, NULL)){
		CBLogError("Could not insert a transaction in the unconfirmed transactions array, not owned by accounts");
		return false;
	}
	return true;
}
bool CBNodeCheckInputs(CBNode * self){
	CBPosition it;
	for (CBAssociativeArray * array = &self->otherTxs; array == &self->otherTxs; array = &self->ourTxs)
		if (CBAssociativeArrayGetFirst(array, &it)) for (;;) {
			// Loop through inputs ensuring the outputs exist for them.
			CBTransaction * tx = it.node->elements[it.index];
			bool lose = false;
			for (uint32_t x = 0; x < tx->inputNum; x++) {
				if (NOT CBBlockChainStorageUnspentOutputExists(self->validator, CBByteArrayGetData(tx->inputs[x]->prevOut.hash), tx->inputs[x]->prevOut.index)){
					// There exists no unspent outputs for this transaction input.
					lose = true;
					break;
				}
			}
			if (lose) {
				// Lose the transaction from the accounter
				if (NOT CBAccounterLostTransaction(self->accounter, tx)) {
					CBLogError("Could not lose a transaction with missing inputs after a reorganistion.");
					return false;
				}
				// Delete the transaction
				CBAssociativeArrayDelete(array, it, true);
				// If owned by accounts, remove from storage
				if (array == &self->ourTxs) {
					if (NOT CBAccounterStorageDeleteUnconfirmedTransaction(self->accounter, tx)) {
						CBLogError("Could not delete a lost transaction from storage for unconfirmed transactions owned by accounts.");
						return false;
					}
				}
			}else if (CBAssociativeArrayIterate(array, &it))
				break;
		}
	return true;
}
bool CBNodeCheckInputsAndSave(CBNode * self){
	// Check transactions for lost inputs
	if (NOT (self->flags & CB_NODE_HEADERS_ONLY)
		&& NOT CBNodeCheckInputs(self)) {
		CBLogError("Could not check the inputs and lose transactions with mising inputs.");
		return false;
	}
	// Save scanning information
	if (NOT CBAccounterStorageSaveScanningInfo(self->accounter, &self->scanInfo)) {
		CBLogError("Could not save the updated scanner information.");
		return false;
	}
	// Commit changes to accounter information
	if (NOT CBAccounterStorageCommit(self->accounter)) {
		CBLogError("Could not commit to accounter storage.");
		return false;
	}
	return true;
}
bool CBNodeFoundBlock(CBNode * self, CBBlock * block, uint8_t branch, uint32_t blockIndex){
	// Check to see if we are scanning yet
	uint32_t blockHeight = self->validator->branches[branch].startHeight + blockIndex;
	if (block->time >= self->scanInfo.startScanning) {
		// Look through each transaction
		for (uint32_t x = 0; x < block->transactionNum; x++) {
			if (self->flags & CB_NODE_HEADERS_ONLY) {
				// We are headers only so add transaction directly
				if (NOT CBAccounterFoundTransaction(self->accounter, block->transactions[x], blockHeight, block->time))
					return false;
			}else{
				// Check if the transaction exists in one of the arrays
				CBFindResult res = CBAssociativeArrayFind(&self->otherTxs, block->transactions[x]);
				if (res.found){
					// Remove the transaction from the array as it is now in a block.
					CBAssociativeArrayDelete(&self->otherTxs, res.position, true);
					// Decrease the size of the other transactions
					self->otherTxsSize -= CBGetMessage(block->transactions[x])->bytes->length;
				}else{
					CBFindResult res = CBAssociativeArrayFind(&self->ourTxs, block->transactions[x]);
					if (res.found) {
						// Remove the transaction from the array as it is now in a block.
						CBAssociativeArrayDelete(&self->ourTxs, res.position, true);
						// Change the status of the transaction to confirmed.
						bool foo;
						if (NOT CBAccounterTransactionChangeStatus(self->accounter, block->transactions[x], blockHeight, &foo)){
							CBLogError("Could not change the status of a transaction by the accounter.");
							return false;
						}
						// Remove from storage
						if (NOT CBAccounterStorageDeleteUnconfirmedTransaction(self->accounter, block->transactions[x])) {
							CBLogError("Could not remove an unconfirmed transaction belonging to an account or accounts from the storage.");
							return false;
						}
					}else
						// Give this previously unfound transaction to the accounter to deal with.
						if (NOT CBAccounterFoundTransaction(self->accounter, block->transactions[x], blockHeight, block->time))
							return false;
				}
			}
		}
	}
	// Update scanning information
	self->scanInfo.lastBranch = branch;
	self->scanInfo.lastBlockIndex = blockIndex;
	return true;
}
void CBNodePeerFree(void * self, void * vpeer){
	CBPeer * peer = vpeer;
	CBFreeAssociativeArray(&peer->expectedObjects);
}
void CBNodePeerSetup(void * self, void * vpeer){
	CBPeer * peer = vpeer;
	if (NOT CBInitAssociativeArray(&peer->expectedObjects, CBKeyCompare, free)) {
		CBLogError("Could not create the expected objects array for a peer");
		CBNetworkCommunicatorDisconnect(CBGetNetworkCommunicator(self), peer, 0, false);
	}
}
CBOnMessageReceivedAction CBNodeProcessBlock(CBNode * self, CBBlock * block){
	// Validate block
	CBBlockProcessResult result = CBValidatorProcessBlock(self->validator, block, CBNetworkAddressManagerGetNetworkTime(CBGetNetworkCommunicator(self)->addresses));
	switch (result.status) {
		case CB_BLOCK_STATUS_ERROR:
			// There was an error with processing the block.
		case CB_BLOCK_STATUS_BAD:
			// The peer sent us a bad block
			return CB_MESSAGE_ACTION_DISCONNECT;
		case CB_BLOCK_STATUS_SIDE:
		case CB_BLOCK_STATUS_BAD_TIME:
		case CB_BLOCK_STATUS_ORPHAN:
		case CB_BLOCK_STATUS_NO_NEW:
			// In these cases just continue.
			return CB_MESSAGE_ACTION_CONTINUE;
		case CB_BLOCK_STATUS_DUPLICATE:
			// If headers-only this means we have re-requested a block to get transactions we need.
			if (NOT (self->flags & CB_NODE_HEADERS_ONLY)) {
				// First verify the merkle root
				uint8_t * merkleRoot = CBBlockCalculateMerkleRoot(block);
				if (NOT merkleRoot) {
					CBLogError("Could not calculate the merkle root for scanning transactions for a re-requested block.");
					return CB_MESSAGE_ACTION_DISCONNECT;
				}
				if (memcmp(merkleRoot, CBByteArrayGetData(block->merkleRoot), 32))
					return CB_MESSAGE_ACTION_DISCONNECT;
				// Find the block index and branch
				uint8_t branch;
				uint32_t blockIndex;
				if (NOT CBBlockChainStorageGetBlockLocation(self->validator, CBBlockGetHash(block), &branch, &blockIndex)) {
					CBLogError("Could not get the location of a duplicate block, to check that we got it to scan for transactions.");
					return CB_MESSAGE_ACTION_DISCONNECT;
				}
				// Check this is the next block we need.
				if ((blockIndex == 0
					&& self->scanInfo.lastBlockIndex == self->validator->branches[branch].parentBlockIndex
					&& self->scanInfo.lastBranch == self->validator->branches[branch].parentBranch)
					|| (blockIndex > 0
						&& self->scanInfo.lastBlockIndex == blockIndex - 1
						&& self->scanInfo.lastBranch == branch)) {
					// This is the block we want
					if (NOT CBNodeFoundBlock(self, block, branch, blockIndex))
						return CB_MESSAGE_ACTION_STOP;
					// Save
					if (NOT CBNodeCheckInputsAndSave(self)) {
						CBLogError("Could not save the accounter information for a re-requested block.");
						return false;
					}
				}else
					// The peer is giving us the blocks in the wrong order, stupid thing!
					return CB_MESSAGE_ACTION_DISCONNECT;
			}
			return CB_MESSAGE_ACTION_CONTINUE;
		case CB_BLOCK_STATUS_MAIN:
			// The block was added alone onto the main chain.
			if (NOT CBNodeFoundBlock(self, block, self->validator->mainBranch, self->validator->branches[self->validator->mainBranch].numBlocks - 1))
				return CB_MESSAGE_ACTION_STOP;
			// Check inputs and save
			if (NOT CBNodeCheckInputsAndSave(self)) {
				CBLogError("Could not check the inputs and save during a main chain extention.");
				return false;
			}
			return CB_MESSAGE_ACTION_CONTINUE;
		case CB_BLOCK_STATUS_MAIN_WITH_ORPHANS:
			// Scan all new blocks
			// First the block we processed
			if (NOT CBNodeFoundBlock(self, block, self->validator->mainBranch, self->validator->branches[self->validator->mainBranch].numBlocks - 1 - result.data.orphansAdded.numOrphansAdded)){
				CBValidatorFreeBlockProcessResultOrphans(&result);
				return CB_MESSAGE_ACTION_STOP;
			}
			// Now the orphans
			// If we are fully validating we can process the orphans here, but for headers only nodes, we need to request them again.
			if (self->flags & CB_NODE_HEADERS_ONLY) {
				// Add the orphans to be re-requested
				for (uint8_t x = 0; x < result.data.orphansAdded.numOrphansAdded; x++) {
					uint8_t * hashKey = malloc(21);
					if (NOT hashKey) {
						CBLogError("Could not allocate memory for a block hash key for re-reuesting previously orphans for transactions.");
						CBValidatorFreeBlockProcessResultOrphans(&result);
						return CB_MESSAGE_ACTION_STOP;
					}
					hashKey[0] = 20;
					memcmp(hashKey + 1, CBBlockGetHash(result.data.orphansAdded.orphans[x]), 20);
					if (NOT CBAssociativeArrayInsert(&self->reRequest, hashKey, CBAssociativeArrayFind(&self->reRequest, hashKey).position, NULL)){
						CBLogError("Could not insert a previously orphan block hash key into the array for re-reuesting transactions.");
						CBValidatorFreeBlockProcessResultOrphans(&result);
						free(hashKey);
						return CB_MESSAGE_ACTION_STOP;
					}
				}
			}else for (uint8_t x = 0; x < result.data.orphansAdded.numOrphansAdded; x++) {
				if (NOT CBNodeFoundBlock(self, result.data.orphansAdded.orphans[x], self->validator->mainBranch, self->validator->branches[self->validator->mainBranch].numBlocks - result.data.orphansAdded.numOrphansAdded + x)){
					CBValidatorFreeBlockProcessResultOrphans(&result);
					return CB_MESSAGE_ACTION_STOP;
				}
			}
			// Check inputs and save
			if (NOT CBNodeCheckInputsAndSave(self)) {
				CBLogError("Could not check the inputs and save during a main chain extention with orphans.");
				return false;
			}
			// Free the result
			CBValidatorFreeBlockProcessResultOrphans(&result);
			return CB_MESSAGE_ACTION_CONTINUE;
		case CB_BLOCK_STATUS_REORG:{
			// Lose blocks down to fork point and then find blocks up to the end of the new chain.
			CBChainPath * mainChain = &result.data.reorgData.newChain;
			uint8_t pathPoint = result.data.reorgData.start.chainPathIndex;
			if (NOT CBNodeSyncScan(self, mainChain, pathPoint, result.data.reorgData.start.blockIndex)) {
				CBLogError("Could not synchronise the acounts to a re-organised block chain.");
				return false;
			}
		}
		default:
			return CB_MESSAGE_ACTION_CONTINUE;
	}
}
CBOnMessageReceivedAction CBNodeProcessMessage(void * vself, void * vpeer){
	CBNode * self = vself;
	CBPeer * peer = vpeer;
	switch (peer->receive->type) {
		case CB_MESSAGE_TYPE_BLOCK:{
			// The peer sent us a block
			CBBlock * block = CBGetBlock(peer->receive);
			// If the peer already gave us this block, disconnect them.
			uint8_t blockKey[21];
			blockKey[0] = 21;
			memcpy(blockKey + 1, CBBlockGetHash(block), 20);
			CBFindResult res = CBAssociativeArrayFind(&peer->expectedObjects, blockKey);
			if (NOT res.found)
				// We did not expect this block from this peer.
				return CB_MESSAGE_ACTION_DISCONNECT;
			return CBNodeProcessBlock(self, block);
		}
		case CB_MESSAGE_TYPE_GETBLOCKS:
			// We are not a full node
			return CB_MESSAGE_ACTION_DISCONNECT;
		case CB_MESSAGE_TYPE_GETHEADERS:{
			// The peer is requesting headers
			CBGetBlocks * getHeaders = CBGetGetBlocks(peer->receive);
			CBChainDescriptor * chainDesc = getHeaders->chainDescriptor;
			// Go though the chain descriptor until a hash is found that we own
			uint8_t branch;
			uint32_t blockIndex;
			CBBlockHeaders * headers = CBNewBlockHeaders();
			if (NOT headers) {
				CBLogError("Could not create the block headers object for sending headers to a peer.");
				return CB_MESSAGE_ACTION_DISCONNECT;
			}
			if (chainDesc->hashNum == 0) {
				// Give the hash stop block if it exists, else send nothing.
				if (CBBlockChainStorageGetBlockLocation(self->validator, CBByteArrayGetData(getHeaders->stopAtHash), &branch, &blockIndex)) {
					// Load and give the block header
					CBBlock * header = CBBlockChainStorageGetBlockHeader(self->validator, branch, blockIndex);
					if (NOT header) {
						CBReleaseObject(headers);
						CBLogError("Could not create a block header for sending to a peer.");
						return CB_MESSAGE_ACTION_DISCONNECT;
					}
					CBBlockHeadersTakeBlockHeader(headers, header);
				}else{
					CBReleaseObject(headers);
					return CB_MESSAGE_ACTION_CONTINUE;
				}
			}else{
				bool found = false;
				for (uint16_t x = 0; x < chainDesc->hashNum; x++) {
					if (CBBlockChainStorageBlockExists(self->validator, CBByteArrayGetData(chainDesc->hashes[x]))){
						// We have a block header that we own. Get the location
						if (NOT CBBlockChainStorageGetBlockLocation(self->validator, CBByteArrayGetData(chainDesc->hashes[x]), &branch, &blockIndex)){
							CBReleaseObject(headers);
							CBLogError("Could not locate a block header in the block chain storage.");
							return CB_MESSAGE_ACTION_DISCONNECT;
						}
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
				}else{
					// Start at block 1
					intersection.blockIndex = 1;
					intersection.chainPathIndex = mainChainPath.numBranches - 1;
				}
				// Now provide headers from this intersection up to 2000 blocks, the last one we have or the stopAtHash.
				for (uint8_t x = 0; x < 2000; x++) {
					// Check to see if we reached the last block in the main chain.
					if (intersection.chainPathIndex == 0
						&& intersection.blockIndex == mainChainPath.points[intersection.chainPathIndex].blockIndex)
						break;
					// Check to see if we reached stopAtHash
					uint8_t hash[32];
					if (NOT CBBlockChainStorageGetBlockHash(self->validator, mainChainPath.points[intersection.chainPathIndex].branch, intersection.blockIndex, hash)) {
						if (x > 0)
							CBReleaseObject(headers);
						CBLogError("Could not obtain a hash for a block to check stopAtHash.");
						return CB_MESSAGE_ACTION_DISCONNECT;
					}
					if (memcmp(hash, CBByteArrayGetData(getHeaders->stopAtHash), 32) == 0) {
						if (x == 0)
							// Peer had a stopAtHash which prevented us giving any blocks
							return CB_MESSAGE_ACTION_DISCONNECT;
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
					// Add block header
					CBBlock * header = CBBlockChainStorageGetBlockHeader(self->validator, mainChainPath.points[intersection.chainPathIndex].branch, intersection.blockIndex);
					if (NOT header) {
						CBReleaseObject(headers);
						CBLogError("Could not create a block header for sending to a peer.");
						return CB_MESSAGE_ACTION_DISCONNECT;
					}
					CBBlockHeadersTakeBlockHeader(headers, header);
				}
			}
			// Send the headers
			CBNetworkCommunicatorSendMessage(CBGetNetworkCommunicator(self), peer, CBGetMessage(headers), NULL);
			CBReleaseObject(headers);
			break;
		}
		case CB_MESSAGE_TYPE_GETDATA:{
			// The peer is requesting data. It should only be requesting transactions that we own.
			CBInventoryBroadcast * getData = CBGetInventoryBroadcast(peer->receive);
			// Ensure it is requesting no more data than we have advertised and not satisfied
			if (getData->itemNum > peer->advertisedData)
				return CB_MESSAGE_ACTION_DISCONNECT;
			// Begin sending requested data.
			peer->sendDataIndex = 0;
			peer->requestedData = getData;
			CBRetainObject(peer->requestedData);
			if (CBNodeSendRequestedData(self, peer))
				// Return since the node was stoped.
				return CB_MESSAGE_ACTION_RETURN;
			break;
		}
		case CB_MESSAGE_TYPE_HEADERS:{
			// The peer has headers for us.
			CBBlockHeaders * headers = CBGetBlockHeaders(peer->receive);
			CBOnMessageReceivedAction action;
			if (headers->headerNum == 0)
				// The peer has no more headers for us, so we end here
				return CB_MESSAGE_ACTION_CONTINUE;
			for (uint16_t x = 0; x < headers->headerNum; x++) {
				action = CBNodeProcessBlock(self, headers->blockHeaders[x]);
				if (action == CB_MESSAGE_ACTION_STOP)
					return CB_MESSAGE_ACTION_STOP;
				if (action == CB_MESSAGE_ACTION_DISCONNECT)
					break;
			}
			// Ask for new headers from this peer unless, we are to disconnect it
			CBByteArray * nullHash = CBNewByteArrayOfSize(32);
			memset(CBByteArrayGetData(nullHash), 0, 32);
			CBChainDescriptor * chainDesc = CBValidatorGetChainDescriptor(self->validator);
			CBGetBlocks * getHeaders = CBNewGetBlocks(1, chainDesc, nullHash);
			CBReleaseObject(chainDesc);
			CBReleaseObject(nullHash);
			CBNetworkCommunicatorSendMessage(CBGetNetworkCommunicator(self), peer, CBGetMessage(getHeaders), NULL);
			CBReleaseObject(getHeaders);
			return action;
		}
		case CB_MESSAGE_TYPE_INV:{
			// The peer has sent us an inventory broadcast
			CBInventoryBroadcast * inv = CBGetInventoryBroadcast(peer->receive);
			// Get data request for objects we want.
			CBInventoryBroadcast * getData = NULL;
			for (uint16_t x = 0; x < inv->itemNum; x++) {
				uint8_t * hash = CBByteArrayGetData(inv->items[x]->hash);
				if (inv->items[x]->type == CB_INVENTORY_ITEM_BLOCK) {
					// Check if we have the block
					if (CBBlockChainStorageBlockExists(self->validator, hash))
						continue;
				}else if (inv->items[x]->type == CB_INVENTORY_ITEM_TX){
					// Check if we have the transaction
					self->ourTxs.compareFunc = CBTransactionAndHashCompare;
					CBFindResult res = CBAssociativeArrayFind(&self->ourTxs, hash);
					self->ourTxs.compareFunc = CBTransactionCompare;
					if (res.found)
						continue;
					if (NOT (self->flags & CB_NODE_HEADERS_ONLY)) {
						self->otherTxs.compareFunc = CBTransactionAndHashCompare;
						CBFindResult res = CBAssociativeArrayFind(&self->otherTxs, hash);
						self->otherTxs.compareFunc = CBTransactionCompare;
						if (res.found)
							continue;
						if (CBBlockChains) {
							<#statements#>
						}
					}
				}else continue;
			}
		}
		default:
			break;
	}
	return CB_MESSAGE_ACTION_CONTINUE;
}
bool CBNodeSendRequestedData(CBNode * self, CBPeer * peer){
	// See if the request has been satisfied
	if (peer->sendDataIndex == peer->requestedData->itemNum) {
		// We have sent everything
		CBReleaseObject(peer->requestedData);
		peer->requestedData = NULL;
		return false;
	}
	// Check that the type is a transaction if headers only.
	if (self->flags & CB_NODE_HEADERS_ONLY
		&& peer->requestedData->items[peer->sendDataIndex]->type != CB_INVENTORY_ITEM_TX) {
		CBNetworkCommunicatorDisconnect(CBGetNetworkCommunicator(self), peer, CB_24_HOURS, false);
		return true;
	}
	switch (peer->requestedData->items[peer->sendDataIndex]->type) {
		case CB_INVENTORY_ITEM_TX:{
			// First look for our transactions
			// Change the comparison function to accept a hash in place of a transaction object.
			self->ourTxs.compareFunc = CBTransactionAndHashCompare;
			CBFindResult res = CBAssociativeArrayFind(&self->ourTxs, CBByteArrayGetData(peer->requestedData->items[peer->sendDataIndex]->hash));
			// Change the comparison function back
			self->ourTxs.compareFunc = CBTransactionCompare;
			if (res.found)
				// The peer was requesting one of our transactions
				CBNetworkCommunicatorSendMessage(CBGetNetworkCommunicator(self), peer, CBFindResultToPointer(res), CBNodeSendRequestedDataVoid);
			else{
				// Now check the other transactions
				self->otherTxs.compareFunc = CBTransactionAndHashCompare;
				res = CBAssociativeArrayFind(&self->otherTxs, CBByteArrayGetData(peer->requestedData->items[peer->sendDataIndex]->hash));
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
			// ??? Implement
		}
		case CB_INVENTORY_ITEM_ERROR:
			return false;
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
bool CBNodeSyncScan(CBNode * self, CBChainPath * mainPath, uint8_t pathPoint, uint32_t forkBlockIndex){
	uint8_t forkBranch = mainPath->points[pathPoint].branch;
	while (self->scanInfo.lastBranch != forkBranch
		   || self->scanInfo.lastBlockIndex != forkBlockIndex) {
		// Load block
		CBBlock * block = CBBlockChainStorageLoadBlock(self->validator, self->scanInfo.lastBlockIndex, self->scanInfo.lastBranch);
		if (NOT block) {
			CBLogError("Could not load a lost block.");
			return false;
		}
		// Lose the block
		for (uint32_t x = 0; x < block->transactionNum; x++) {
			bool owned;
			if (NOT CBAccounterTransactionChangeStatus(self->accounter, block->transactions[x], CB_TX_UNCONFIRMED, &owned)) {
				CBReleaseObject(block);
				CBLogError("Could not change the status of a transaction to unconfirmed.");
				return false;
			}
			// Add back into an array.
			if (owned) {
				if (NOT CBAssociativeArrayInsert(&self->ourTxs, block->transactions[x], CBAssociativeArrayFind(&self->ourTxs, block->transactions[x]).position, NULL)) {
					CBReleaseObject(block);
					CBLogError("Could not insert a transaction from a lost block into the array for unconfirmed transactions owned by accounts.");
					return false;
				}
				if (NOT CBAccounterStorageSaveUnconfirmedTransaction(self->accounter, block->transactions[x])) {
					CBReleaseObject(block);
					CBLogError("Could not insert a transaction from a lost block into storage for unconfirmed transactions owned by accounts.");
					return false;
				}
			}else if (NOT CBNodeAddOtherTransaction(self, block->transactions[x])) {
				CBReleaseObject(block);
				CBLogError("Could not insert a transaction from a lost block into an array for unconfirmed transactions not owned by accounts.");
				return false;
			}
		}
		CBReleaseObject(block);
		// Go down a block
		if (self->scanInfo.lastBlockIndex > 0)
			self->scanInfo.lastBlockIndex--;
		else
			self->scanInfo.lastBlockIndex = self->validator->branches[self->scanInfo.lastBranch--].parentBlockIndex;
	}
	// Add blocks
	for (uint8_t x = pathPoint; x--;) {
		for (;;) {
			self->scanInfo.lastBlockIndex++;
			if (self->scanInfo.lastBlockIndex > mainPath->points[x].blockIndex){
				self->scanInfo.lastBlockIndex = 0;
				break;
			}
			// Load block
			CBBlock * block = CBBlockChainStorageLoadBlock(self->validator, self->scanInfo.lastBlockIndex, mainPath->points[pathPoint].branch);
			if (NOT block) {
				CBLogError("Could not load a lost block.");
				return false;
			}
			if (NOT CBNodeFoundBlock(self, block, mainPath->points[pathPoint].branch, self->scanInfo.lastBlockIndex)){
				CBReleaseObject(block);
				CBLogError("Failure when processing a block when syncronising accounter and validator.");
				return false;
			}
			CBReleaseObject(block);
		}
	}
	// Check inputs and save
	if (NOT CBNodeCheckInputsAndSave(self)) {
		CBLogError("Could not check the inputs and save during synchronisation between the validator and accounter.");
		return false;
	}
	return true;
}
CBCompare CBTransactionCompare(void * tx1, void * tx2){
	CBTransaction * tx1Obj = tx1;
	CBTransaction * tx2Obj = tx2;
	int res = memcmp(CBTransactionGetHash(tx1Obj), CBTransactionGetHash(tx2Obj), 32);
	if (res > 0)
		return CB_COMPARE_MORE_THAN;
	if (res == 0)
		return CB_COMPARE_EQUAL;
	return CB_COMPARE_LESS_THAN;
}
CBCompare CBTransactionAndHashCompare(void * hash, void * tx){
	CBTransaction * txObj = tx;
	int res = memcmp((uint8_t *)hash, CBTransactionGetHash(txObj), 32);
	if (res > 0)
		return CB_COMPARE_MORE_THAN;
	if (res == 0)
		return CB_COMPARE_EQUAL;
	return CB_COMPARE_LESS_THAN;
}
*/