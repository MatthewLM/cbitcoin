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

CBNode * CBNewNode(CBDepObject database, CBNodeFlags flags, CBNodeCallbacks nodeCallbacks, CBNetworkCommunicatorCallbacks commCallbacks, CBOnMessageReceivedAction (*onMessageReceived)(CBNode *, CBPeer *, CBMessage *)){
	CBNode * self = malloc(sizeof(*self));
	CBGetObject(self)->free = CBFreeNode;
	if (CBInitNode(self, database, flags, nodeCallbacks, commCallbacks, onMessageReceived))
		return self;
	free(self);
	return NULL;
}

//  Initialiser

bool CBInitNode(CBNode * self, CBDepObject database, CBNodeFlags flags, CBNodeCallbacks nodeCallbacks, CBNetworkCommunicatorCallbacks commCallbacks, CBOnMessageReceivedAction (*onMessageReceived)(CBNode *, CBPeer *, CBMessage *)){
	self->flags = flags;
	self->database = database;
	self->callbacks = nodeCallbacks;
	self->onMessageReceived = onMessageReceived;
	// Initialise network communicator
	CBNetworkCommunicator * comm = CBGetNetworkCommunicator(self);
	commCallbacks.onMessageReceived = CBNodeOnMessageReceived;
	CBInitNetworkCommunicator(comm, CB_SERVICE_FULL_BLOCKS, commCallbacks);
	// Set network communicator fields.
	comm->flags = CB_NETWORK_COMMUNICATOR_AUTO_DISCOVERY | CB_NETWORK_COMMUNICATOR_AUTO_HANDSHAKE | CB_NETWORK_COMMUNICATOR_AUTO_PING;
	if (flags & CB_NODE_BOOTSTRAP)
		comm->flags |= CB_NETWORK_COMMUNICATOR_BOOTSTRAP;
	comm->version = CB_PONG_VERSION;
	comm->networkID = CB_PRODUCTION_NETWORK_BYTES; // ??? Add testnet support
	CBNetworkCommunicatorSetAlternativeMessages(comm, NULL, NULL);
	// Create address manager
	comm->addresses = CBNewNetworkAddressManager(CBNodeOnBadTime);
	comm->addresses->maxAddressesInBucket = 1000;
	comm->addresses->callbackHandler = self;
	// Initialise thread data
	self->shutDownThread = false;
	self->messageQueue = NULL;
	CBNewMutex(&self->blockAndTxMutex);
	CBNewMutex(&self->messageProcessMutex);
	CBNewCondition(&self->messageProcessWaitCond);
	CBNewThread(&self->messageProcessThread, CBNodeProcessMessages, self);
	// Initialise the storage objects
	if (CBNewBlockChainStorage(&self->blockChainStorage, database)) {
		if (CBNewAccounterStorage(&self->accounterStorage, database)){
			if (CBNewNodeStorage(&self->nodeStorage, database)){
				// Setup address storage
				if (CBNewAddressStorage(&comm->addrStorage, database)) {
					if (CBAddressStorageLoadAddresses(comm->addrStorage, comm->addresses)){
						comm->useAddrStorage = true;
						return true;
					}
				}else
					CBLogError("Could not create the address storage object for a node");
				CBFreeNodeStorage(self->nodeStorage);
			}else
				CBLogError("Could not create the node storage object for a node");
			CBFreeAccounterStorage(self->accounterStorage);
		}else
			CBLogError("Could not create the accounter storage object for a node");
		CBFreeBlockChainStorage(self->blockChainStorage);
	}else
		CBLogError("Could not create the block storage object for a node");
	CBDestroyNetworkCommunicator(self);
	return false;
}

//  Destructor

void CBDestroyNode(void * vself){
	CBNode * self = vself;
	// Exit thread.
	self->shutDownThread = true;
	CBMutexLock(self->messageProcessMutex);
	if (self->messageQueue == NULL)
		// The thread is waiting for messages so wake it.
		CBConditionSignal(self->messageProcessWaitCond);
	CBMutexUnlock(self->messageProcessMutex);
	CBThreadJoin(self->messageProcessThread);
	CBFreeBlockChainStorage(self->blockChainStorage);
	CBFreeAccounterStorage(self->accounterStorage);
	CBReleaseObject(self->validator);
	CBFreeMutex(self->blockAndTxMutex);
	CBFreeMutex(self->messageProcessMutex);
	CBFreeCondition(self->messageProcessWaitCond);
	CBFreeThread(self->messageProcessThread);
	CBDestroyNetworkCommunicator(vself);
}
void CBFreeNode(void * self){
	CBDestroyNode(self);
	free(self);
}

//  Functions

void CBNodeDisconnectPeer(CBPeer * peer){
	CBRetainObject(peer);
	CBRunOnEventLoop(CBGetNetworkCommunicator(peer->nodeObj)->eventLoop, CBNodeDisconnectPeerRun, peer, false);
}
void CBNodeDisconnectPeerRun(void * vpeer){
	CBPeer * peer = vpeer;
	CBNetworkCommunicator * comm = peer->nodeObj;
	CBNetworkCommunicatorDisconnect(comm, peer, CB_24_HOURS, false);
	CBReleaseObject(peer);
}
void CBNodeOnBadTime(void * vself){
	CBNode * self = vself;
	CBMutexUnlock(CBGetNode(self)->blockAndTxMutex);
	CBLogError("The system time is potentially incorrect. It does not match with the network time.");
	self->callbacks.onFatalNodeError(self, CB_ERROR_BAD_TIME);
	CBReleaseObject(self);
}
CBOnMessageReceivedAction CBNodeOnMessageReceived(CBNetworkCommunicator * comm, CBPeer * peer, CBMessage * message){
	CBNode * self = CBGetNode(comm);
	// Add message to queue
	CBRetainObject(message);
	CBRetainObject(peer);
	CBMutexLock(self->messageProcessMutex);
	if (self->messageQueue == NULL)
		self->messageQueue = self->messageQueueBack = malloc(sizeof(*self->messageQueueBack));
	else{
		self->messageQueueBack->next = malloc(sizeof(*self->messageQueueBack));
		self->messageQueueBack = self->messageQueueBack->next;
	}
	self->messageQueueBack->peer = peer;
	self->messageQueueBack->message = message;
	self->messageQueueBack->next = NULL;
	// Wakeup thread if this is the first in the queue
	if (self->messageQueue == self->messageQueueBack)
		// We have just added a block to the queue when there was not one before so wake-up the processing thread.
		CBConditionSignal(self->messageProcessWaitCond);
	CBMutexUnlock(self->messageProcessMutex);
	return CB_MESSAGE_ACTION_CONTINUE;
}
void CBNodeOnValidatorError(void * vself){
	CBNode * self = vself;
	CBMutexUnlock(CBGetNode(self)->blockAndTxMutex);
	CBLogError("There was a validation error.");
	self->callbacks.onFatalNodeError(self, CB_ERROR_VALIDATION);
	CBReleaseObject(self);
}
CBOnMessageReceivedAction CBNodeProcessAlert(CBNode * self, CBPeer * peer, CBAlert * alert){
	// ??? Presently ignoring alerts
	return CB_MESSAGE_ACTION_CONTINUE;
}
void CBNodeProcessMessages(void * node){
	CBNode * self = node;
	CBMutexLock(self->messageProcessMutex);
	for (;;) {
		if (self->messageQueue == NULL && !self->shutDownThread){
			// Wait for more messages
			CBConditionWait(self->messageProcessWaitCond, self->messageProcessMutex);
			CBLogVerbose("Process thread as been woken");
		}
		// Check to see if the thread should terminate
		if (self->shutDownThread){
			CBMutexUnlock(self->messageProcessMutex);
			return;
		}
		// Process next message on queue
		CBMessageQueue * toProcess = self->messageQueue;
		CBMutexUnlock(self->messageProcessMutex);
		// Handle alerts
		CBOnMessageReceivedAction action;
		if (toProcess->message->type == CB_MESSAGE_TYPE_ALERT)
			// The peer sent us an alert message
			action = CBNodeProcessAlert(self, toProcess->peer, CBGetAlert(toProcess->message));
		else
			action = self->onMessageReceived(self, toProcess->peer, toProcess->message);
		if (action == CB_MESSAGE_ACTION_DISCONNECT)
			// We need to disconnect the node
			CBNodeDisconnectPeer(toProcess->peer);
		CBMutexLock(self->messageProcessMutex);
		// Remove this from queue
		CBReleaseObject(toProcess->message);
		CBReleaseObject(toProcess->peer);
		self->messageQueue = toProcess->next;
		free(toProcess);
	}
}
CBOnMessageReceivedAction CBNodeReturnError(CBNode * self, char * err){
	CBLogError(err);
	self->callbacks.onFatalNodeError(self, CB_ERROR_GENERAL);
	CBReleaseObject(self);
	return CB_MESSAGE_ACTION_CONTINUE;
}
CBOnMessageReceivedAction CBNodeSendBlocksInvOrHeaders(CBNode * self, CBPeer * peer, CBGetBlocks * getBlocks, bool full){
	CBChainDescriptor * chainDesc = getBlocks->chainDescriptor;
	if (chainDesc->hashNum == 0)
		// Why do this?
		return CB_MESSAGE_ACTION_DISCONNECT;
	uint8_t branch;
	uint32_t blockIndex;
	// Go though the chain descriptor until a hash is found that we own
	bool found = false;
	// Lock block mutex for block chain access
	CBMutexLock(self->blockAndTxMutex);
	for (uint16_t x = 0; x < chainDesc->hashNum; x++) {
		CBErrBool exists = CBBlockChainStorageGetBlockLocation(CBGetNode(self)->validator, CBByteArrayGetData(chainDesc->hashes[x]), &branch, &blockIndex);
		if (exists == CB_ERROR) {
			CBMutexUnlock(self->blockAndTxMutex);
			return CBNodeReturnError(self, "Could not look for block with chain descriptor hash.");
		}
		if (exists == CB_TRUE) {
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
	}else{
		// Bad chain?
		CBMutexUnlock(self->blockAndTxMutex);
		return CB_MESSAGE_ACTION_DISCONNECT;
	}
	CBMessage * message;
	// Now provide headers from this intersection up to 2000 blocks or block inventory items up to 500, the last one we have or the stopAtHash.
	for (uint16_t x = 0; x < (full ? 500 : 2000); x++) {
		// Check to see if we reached the last block in the main chain.
		if (intersection.chainPathIndex == 0
			&& intersection.blockIndex == mainChainPath.points[0].blockIndex) {
			if (x == 0){
				// The intersection is at the last block. The peer is up-to-date with us
				peer->upload = false;
				peer->upToDate = true;
				CBMutexUnlock(self->blockAndTxMutex);
				return CB_MESSAGE_ACTION_CONTINUE;
			}
			break;
		}
		// Move to the next block
		if (intersection.blockIndex == mainChainPath.points[intersection.chainPathIndex].blockIndex) {
			// Move to next branch
			intersection.chainPathIndex--;
			intersection.blockIndex = 0;
		}else
			// Move to the next block
			intersection.blockIndex++;
		// Get the hash
		uint8_t hash[32];
		if (! CBBlockChainStorageGetBlockHash(self->validator, mainChainPath.points[intersection.chainPathIndex].branch, intersection.blockIndex, hash)) {
			if (x != 0) CBReleaseObject(message);
			CBMutexUnlock(self->blockAndTxMutex);
			return CBNodeReturnError(self, "Could not obtain a hash for a block.");
		}
		// Check to see if we are at stopAtHash
		if (getBlocks->stopAtHash && memcmp(hash, CBByteArrayGetData(getBlocks->stopAtHash), 32) == 0){
			if (x == 0) {
				CBMutexUnlock(self->blockAndTxMutex);
				return CB_MESSAGE_ACTION_CONTINUE;
			}
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
			char blkStr[CB_BLOCK_HASH_STR_SIZE];
			CBByteArrayToString(hashObj, 0, CB_BLOCK_HASH_STR_BYTES, blkStr, true);
			CBInventoryTakeInventoryItem(CBGetInventory(message), CBNewInventoryItem(CB_INVENTORY_ITEM_BLOCK, hashObj));
			CBReleaseObject(hashObj);
		}else
			CBBlockHeadersTakeBlockHeader(CBGetBlockHeaders(message), CBBlockChainStorageGetBlockHeader(self->validator, mainChainPath.points[intersection.chainPathIndex].branch, intersection.blockIndex));
	}
	CBMutexUnlock(self->blockAndTxMutex);
	// Send the message
	CBNodeSendMessageOnNetworkThread(CBGetNetworkCommunicator(self), peer, message, NULL);
	CBReleaseObject(message);
	// We are uploading to the peer
	peer->upload = true;
	return CB_MESSAGE_ACTION_CONTINUE;
}
void CBNodeSendMessageOnNetworkThread(CBNetworkCommunicator * self, CBPeer * peer, CBMessage * message, void (*callback)(void *, void *)){
	CBSendMessageData * data = malloc(sizeof(*data));
	*data = (CBSendMessageData){self,peer,message,callback};
	CBRetainObject(message);
	CBRetainObject(peer);
	// Shouldn't block in the cases where this thread already has a mutex used by the CBNetworkCommunicator code.
	CBRunOnEventLoop(self->eventLoop, CBNodeSendMessageOnNetworkThreadVoid, data, false);
}
void CBNodeSendMessageOnNetworkThreadVoid(void * vdata){
	CBSendMessageData * data = vdata;
	CBNetworkCommunicatorSendMessage(data->self, data->peer, data->message, data->callback);
	CBReleaseObject(data->message);
	CBReleaseObject(data->peer);
	free(data);
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
CBCompare CBTransactionPtrCompare(CBAssociativeArray * foo, void * tx1, void * tx2){
	return CBTransactionCompare(foo, *(CBTransaction **) tx1, *(CBTransaction **) tx2);
}
CBCompare CBTransactionPtrAndHashCompare(CBAssociativeArray * foo, void * hash, void * tx){
	CBTransaction ** txObj = tx;
	int res = memcmp((uint8_t *)hash, CBTransactionGetHash(*txObj), 32);
	if (res > 0)
		return CB_COMPARE_MORE_THAN;
	if (res == 0)
		return CB_COMPARE_EQUAL;
	return CB_COMPARE_LESS_THAN;
}
