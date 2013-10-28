//
//  CBNodeFull.c
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

#include "CBNodeFull.h"

// ??? Add a more complex mutex arrangement so that several transactions can be processed at once? Or processed alongside blocks? Probably not reorgs though. That would be complex and unneccessary.
// Notify of double spends on unconfirmed transactions
// Add bloom filter
// Add fork detection with callback to warn users

//  Constructor

CBNodeFull * CBNewNodeFull(CBDepObject database, CBNodeFlags flags, uint32_t otherTxsSizeLimit, CBNodeCallbacks callbacks){
	CBNodeFull * self = malloc(sizeof(*self));
	CBGetObject(self)->free = CBFreeNode;
	if (CBInitNodeFull(self, database, flags, otherTxsSizeLimit, callbacks))
		return self;
	free(self);
	return NULL;
}

//  Initialiser

bool CBInitNodeFull(CBNodeFull * self, CBDepObject database, CBNodeFlags flags, uint32_t otherTxsSizeLimit, CBNodeCallbacks callbacks){
	// Initialise the node
	if (!CBInitNode(CBGetNode(self), database, flags, callbacks, (CBNetworkCommunicatorCallbacks){
		CBNodeFullPeerSetup,
		CBNodeFullPeerFree,
		CBNodeFullOnTimeOut,
		CBNodeFullAcceptType,
		NULL,
		CBNodeFullOnNetworkError
	}, CBNodeFullProcessMessage)) {
		CBLogError("Could not initialise the base node.");
		return false;
	}
	self->otherTxsSizeLimit = otherTxsSizeLimit;
	self->otherTxsSize = 0;
	self->numberPeersDownload = 0;
	self->numCanDownload = 0;
	self->deleteOrphanTime = CBGetMilliseconds();
	self->initialisedFoundUnconfTxsNew = false;
	self->initialisedFoundUnconfTxsPrev = false;
	self->initialisedLostUnconfTxs = false;
	self->initialisedOurLostTxs = false;
	self->newTransactions = NULL;
	self->addingDirect = false;
	// Initialise the validator
	CBGetNode(self)->validator = CBNewValidator(CBGetNode(self)->blockChainStorage, 0, (CBValidatorCallbacks){
		CBNodeFullStartValidation,
		CBNodeFullAlreadyValidated,
		CBNodeFullIsOrphan,
		CBNodeFullDeleteBranchCallback,
		CBNodeFullPeerWorkingOnBranch,
		CBNodeFullNewBranchCallback,
		CBNodeFullAddBlock,
		CBNodeFullRemoveBlock,
		CBNodeFullValidatorFinish,
		CBNodeFullInvalidBlock,
		CBNodeFullNoNewBranches,
		CBNodeOnValidatorError,
	 });
	if (CBGetNode(self)->validator){
		// Initialise all of the arrays except for the orphans.
		CBInitAssociativeArray(&self->ourTxs, CBTransactionPtrCompare, NULL, CBFreeFoundTransaction);
		CBInitAssociativeArray(&self->otherTxs, CBTransactionPtrCompare, NULL, CBFreeFoundTransaction);
		CBInitAssociativeArray(&self->chainDependencies, CBHashCompare, NULL, CBFreeTransactionDependency);
		CBInitAssociativeArray(&self->allChainFoundTxs, CBPtrCompare, NULL, NULL);
		if (CBNodeStorageLoadUnconfTxs(self)){
			// Initialise the other transaction arrays
			CBInitAssociativeArray(&self->orphanTxs, CBTransactionPtrCompare, NULL, CBFreeOrphan);
			CBInitAssociativeArray(&self->unfoundDependencies, CBHashCompare, NULL, CBFreeTransactionDependency);
			// Initialise the arrays for blocks we need
			CBInitAssociativeArray(&self->blockPeers, CBHashCompare, NULL, free);
			CBInitAssociativeArray(&self->askedForBlocks, CBHashCompare, NULL, NULL); // Uses data freed by blockPeers
			// Initialise the mutexes
			CBNewMutex(&self->pendingBlockDataMutex);
			CBNewMutex(&self->numberPeersDownloadMutex);
			CBNewMutex(&self->numCanDownloadMutex);
			CBNewMutex(&self->workingMutex);
			return true;
		}else{
			CBFreeAssociativeArray(&self->ourTxs);
			CBFreeAssociativeArray(&self->otherTxs);
			CBFreeAssociativeArray(&self->chainDependencies);
			CBFreeAssociativeArray(&self->allChainFoundTxs);
			CBLogError("Could not load our unconfirmed transactions.");
		}
	}else
		CBLogError("Could not create the validator object for a node");
	CBDestroyNode(self);
	return false;
}

//  Destructor

void CBDestroyNodeFull(void * vself){
	CBNodeFull * self = vself;
	CBFreeAssociativeArray(&self->blockPeers);
	CBFreeAssociativeArray(&self->askedForBlocks);
	CBFreeAssociativeArray(&self->ourTxs);
	CBFreeAssociativeArray(&self->otherTxs);
	CBFreeAssociativeArray(&self->orphanTxs);
	CBFreeAssociativeArray(&self->unfoundDependencies);
	CBFreeAssociativeArray(&self->chainDependencies);
	CBFreeAssociativeArray(&self->allChainFoundTxs);
	CBFreeMutex(self->pendingBlockDataMutex);
	CBFreeMutex(self->numberPeersDownloadMutex);
	CBFreeMutex(self->numCanDownloadMutex);
	CBFreeMutex(self->workingMutex);
	CBDestroyNode(vself);
}
void CBFreeNodeFull(void * self){
	CBDestroyNodeFull(self);
	free(self);
}

void CBFreeFoundTransaction(void * vfndTx){
	CBFoundTransaction * fndTx = vfndTx;
	CBReleaseObject(fndTx->utx.tx);
	CBFreeAssociativeArray(&fndTx->dependants);
	free(fndTx);
}
void CBFreeOrphan(void * vorphanData){
	CBOrphan * orphanData = vorphanData;
	CBReleaseObject(orphanData->utx.tx);
	free(orphanData);
}
void CBFreeTransactionDependency(void * vtxDep){
	CBTransactionDependency * dep = vtxDep;
	CBFreeAssociativeArray(&dep->dependants);
	free(dep);
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
bool CBNodeFullAcceptType(CBNetworkCommunicator * comm, CBPeer * peer, CBMessageType type){
	switch (type) {
		case CB_MESSAGE_TYPE_BLOCK:
			// Only accept block in response to getdata
			if (!peer->expectBlock)
				return false;
			break;
		case CB_MESSAGE_TYPE_TX:
			// Only accept tx in response to getdata
			if (!peer->expectedTxs.root->numElements)
				return false;
			break;
		case CB_MESSAGE_TYPE_HEADERS:
			return false;
		default:
			// Accept all other types
			break;
	}
	return true;
}
bool CBNodeFullAddBlock(void * vpeer, uint8_t branch, CBBlock * block, uint32_t blockHeight, CBAddBlockType addType){
	CBNodeFull * self = CBGetPeer(vpeer)->nodeObj;
	// If last block call newBlock
	if (addType == CB_ADD_BLOCK_LAST)
		CBGetNode(self)->callbacks.newBlock(CBGetNode(self), block, blockHeight, self->forkPoint);
	// Loop through and process transactions.
	for (uint32_t x = 1; x < block->transactionNum; x++) {
		CBTransaction * tx = block->transactions[x];
		// If the transaction is ours, check for lost transactions from the other chain.
		if (self->initialisedOurLostTxs) {
			// Take out this transaction from the ourLostTxs array if it exists in there
			CBFindResult res = CBAssociativeArrayFind(&self->ourLostTxs, tx);
			if (res.found){
				// Remove dependencies and then the lost tx
				for (uint32_t y = 0; y < tx->inputNum; y++)
					CBNodeFullRemoveFromDependency(&self->ourLostTxDependencies, CBByteArrayGetData(tx->inputs[y]->prevOut.hash), tx);
				CBAssociativeArrayDelete(&self->ourLostTxs, res.position, true);
				// We refound a transaction of ours we lost on the other chain. If this is a new block, add the transaction to the accounter.
				if (addType != CB_ADD_BLOCK_PREV) {
					// Get timestamp
					uint64_t time;
					if (!CBAccounterGetTransactionTime(CBGetNode(self)->accounterStorage, CBTransactionGetHash(tx), &time)) {
						CBLogError("Could not get the time of a transaction.");
						return false;
					}
					// Add transaction
					if (CBNodeFullFoundTransaction(self, tx, time, blockHeight, branch, addType == CB_ADD_BLOCK_LAST) != CB_TRUE) {
						CBLogError("Could not process a found transaction that was a lost transaction of ours.");
						return false;
					}
				}
				// We can end here for one of our transactions moving from one chain to another.
				continue;
			}
		}
		// We will record unconfirmed transactions found on the chain, but only remove when we finish.
		// THIS IS IMPORTANT so that if there is a failed reorganisation the unconfirmed transactions will remain in place.
		CBUnconfTransaction * uTx = CBNodeFullGetAnyTransaction(self, CBTransactionGetHash(tx));
		if (uTx != NULL) {
			if (addType == CB_ADD_BLOCK_LAST){
				// As this is the last block, we know it is certain. Therefore process moving from unconfirmed to chain.
				if (! CBNodeFullUnconfToChain(self, uTx, blockHeight, branch, true))
					return false;
			}else{
				// Else we will need to record this for processing later as this block is tentative
				CBTransactionToBranch * txToBranch = malloc(sizeof(*txToBranch));
				txToBranch->uTx = uTx;
				txToBranch->blockHeight = blockHeight;
				txToBranch->branch = branch;
				if (addType == CB_ADD_BLOCK_NEW) {
					// Record transaction to foundUnconfTxsNew
					if (! self->initialisedFoundUnconfTxsNew)
						CBInitAssociativeArray(&self->foundUnconfTxsNew, CBPtrCompare, NULL, free);
					CBAssociativeArrayInsert(&self->foundUnconfTxsNew, txToBranch, CBAssociativeArrayFind(&self->foundUnconfTxsNew, txToBranch).position, NULL);
				}else{
					// Record transaction to foundUnconfTxsPrev
					if (! self->initialisedFoundUnconfTxsPrev)
						CBInitAssociativeArray(&self->foundUnconfTxsPrev, CBPtrCompare, NULL, free);
					CBAssociativeArrayInsert(&self->foundUnconfTxsPrev, txToBranch, CBAssociativeArrayFind(&self->foundUnconfTxsPrev, txToBranch).position, NULL);
				}
			}
		}else{
			// This transaction is not one of the unconfirmed ones we have
			// If this transaction spends a chain dependency of an unconfirmed transaction or one of our lost txs, then we should lose that transaction as a double spend, as well as any further dependants, including unconfirmed transactions (unless it was ours).
			for (uint32_t y = 0; y < tx->inputNum; y++) {
				uint8_t * prevHash = CBByteArrayGetData(tx->inputs[y]->prevOut.hash);
				// See if it's a chain dependency of at least one unconfirmed transaction
				CBFindResult res = CBAssociativeArrayFind(&self->chainDependencies, prevHash);
				if (res.found) {
					// Yes it is. Now look for a dependant spending the same output by looping through them.
					// ??? Could be optimised by placing additional information with chain dependencies. Is it worth it though?
					CBTransactionDependency * dep = CBFindResultToPointer(res);
					CBAssociativeArrayForEach(CBUnconfTransaction * dependant, &dep->dependants){
						bool found = false;
						for (uint32_t z = 0; z < dependant->tx->inputNum; z++) {
							if (!memcmp(prevHash, CBByteArrayGetData(dependant->tx->inputs[z]->prevOut.hash), 32)
								&& tx->inputs[y]->prevOut.index == dependant->tx->inputs[z]->prevOut.index) {
								// The prevouts are the same! Add this to the lostUnconfTxs if the block is tentative, otherwise we can remove the unconfirmed transaction straight away.
								if (addType == CB_ADD_BLOCK_LAST) {
									// Remove now.
									if (! CBNodeFullRemoveUnconfTx(self, dependant))
										return false;
								}else{
									// Remove upon completion of reorg
									if (! self->initialisedLostUnconfTxs)
										CBInitAssociativeArray(&self->lostUnconfTxs, CBTransactionPtrCompare, NULL, NULL);
									CBAssociativeArrayInsert(&self->lostUnconfTxs, dependant, CBAssociativeArrayFind(&self->lostUnconfTxs, dependant).position, NULL);
								}
								// We have found the dependant that spends the same output.
								found = true;
								break;
							}
						}
						// If we have found the dependant that spends the same output, no other dependant can, so end the loop here.
						if (found)
							break;
					}
				}
				// Now for our lost txs
				if (self->initialisedOurLostTxs) {
					CBFindResult res = CBAssociativeArrayFind(&self->ourLostTxDependencies, prevHash);
					if (res.found) {
						// We need to loop through the dependants and see if they also spend from the same output
						CBTransactionDependency * dep = CBFindResultToPointer(res);
						// The dependants of ourLostTxDependencies can only be CBTransaction
						CBAssociativeArrayForEach(CBTransaction * dependant, &dep->dependants) {
							bool found = false;
							for (uint32_t z = 0; z < dependant->inputNum; z++) {
								if (!memcmp(prevHash, CBByteArrayGetData(dependant->inputs[z]->prevOut.hash), 32)
									&& tx->inputs[y]->prevOut.index == dependant->inputs[z]->prevOut.index) {
									// Yes this transaction of ours that we lost cannot be made unconfirmed sue to double spend.
									// Remove it and any further dependant transactions of ours.
									CBNodeFullLoseOurDependants(self, CBTransactionGetHash(dependant));
									// And the unconf dependants 
									CBNodeFullLoseUnconfDependants(self, CBTransactionGetHash(dependant));
									// Remove the dependant from the dependency.
									CBAssociativeArrayDelete(&dep->dependants, CBCurrentPosition, true);
									// Remove dependency if it is empty
									if (dep->dependants.root->numElements == 0)
										CBAssociativeArrayDelete(&self->ourLostTxDependencies, res.position, true);
									// Remove transaction from ourLostTxs
									CBAssociativeArrayDelete(&self->ourLostTxs, CBAssociativeArrayFind(&self->ourLostTxs, dependant).position, true);
									found = true;
									break;
								}
							}
							if (found)
								break;
						}
					}
				}
			}
			// Give to accounter if not previously validated
			if (addType != CB_ADD_BLOCK_PREV) {
				if (CBNodeFullFoundTransaction(self, tx, block->time, blockHeight, branch, addType == CB_ADD_BLOCK_LAST) == CB_ERROR) {
					CBLogError("Could not process a found transaction found in a block.");
					return false;
				}
			}
		}
	}
	if (addType == CB_ADD_BLOCK_LAST) {
		// We can process through recorded double spends and found unconf transactions
		if (self->initialisedOurLostTxs) {
			// Start with our lost transactions, adding them back to unconfirmed
			CBAssociativeArrayForEach(CBTransaction * tx, &self->ourLostTxs) {
				// Make found transaction
				CBFoundTransaction * fndTx = malloc(sizeof(*fndTx));
				fndTx->utx.tx = tx;
				fndTx->utx.numUnconfDeps = 0;
				fndTx->utx.type = CB_TX_OURS;
				// Calculate input value.
				fndTx->inputValue = 0;
				for (uint32_t x = 0; x < tx->inputNum; x++) {
					// We need to get the previous output value
					uint8_t * prevOutHash = CBByteArrayGetData(tx->inputs[x]->prevOut.hash);
					// See if we have this transaction as unconfirmed.
					CBFoundTransaction * fndTx = CBNodeFullGetFoundTransaction(self, prevOutHash);
					if (fndTx == NULL) {
						// Load from chain
						bool coinbase;
						uint32_t height;
						CBTransactionOutput * output = CBBlockChainStorageLoadUnspentOutput(CBGetNode(self)->validator, prevOutHash, tx->inputs[x]->prevOut.index, &coinbase, &height);
						if (output == NULL) {
							CBLogError("Could not load an output for counting the input value for a transaction of ours becomind unconfirmed.");
							return false;
						}
						fndTx->inputValue += output->value;
						CBReleaseObject(output);
					}else
						fndTx->inputValue += fndTx->utx.tx->outputs[tx->inputs[x]->prevOut.index]->value;
				}
				CBRetainObject(fndTx);
				CBAssociativeArrayInsert(&self->ourTxs, fndTx, CBAssociativeArrayFind(&self->ourTxs, fndTx).position, NULL);
				// If this was a chain dependency, move dependency data to CBFoundTransaction, else init empty dependant's array
				CBFindResult res = CBAssociativeArrayFind(&self->chainDependencies, CBTransactionGetHash(fndTx->utx.tx));
				if (res.found) {
					CBTransactionDependency * dep = CBFindResultToPointer(res);
					fndTx->dependants = dep->dependants;
					CBAssociativeArrayDelete(&self->chainDependencies, res.position, false);
					free(dep);
					// Loop through dependants and add to the number of unconf dependencies.
					CBAssociativeArrayForEach(CBFoundTransaction * dependant, &fndTx->dependants)
						if (dependant->utx.numUnconfDeps++ == 0)
							// Came off allChainFoundTxs
							CBAssociativeArrayDelete(&self->allChainFoundTxs, CBAssociativeArrayFind(&self->allChainFoundTxs, dependant).position, false);
				}else
					CBInitAssociativeArray(&fndTx->dependants, CBTransactionPtrCompare, NULL, NULL);
				// Remove our tx from the node storage.
				if (!CBNodeStorageRemoveOurTx(CBGetNode(self)->nodeStorage, fndTx->utx.tx)) {
					CBLogError("Could not remove our transaction moving onto the chain.");
					return false;
				}
				// Get time of the transaction
				uint64_t time;
				if (!CBAccounterGetTransactionTime(CBGetNode(self)->accounterStorage, CBTransactionGetHash(fndTx->utx.tx), &time)) {
					CBLogError("Could not get the time of a transaction.");
					return false;
				}
				// Add to accounter again.
				if (CBNodeFullFoundTransaction(self, fndTx->utx.tx, time, 0, CB_NO_BRANCH, true) == CB_ERROR) {
					CBLogError("Could not process one of our lost transactions that should become unconfirmed.");
					return false;
				}
				// Call transactionUnconfirmed
				CBGetNode(self)->callbacks.transactionUnconfirmed(CBGetNode(self), CBTransactionGetHash(fndTx->utx.tx));
			}
			// Now add dependencies of our transactions.
			CBAssociativeArrayForEach(CBTransactionDependency * dep, &self->ourLostTxDependencies) {
				// See if this dependency was one of our transactions we added back as unconfirmed
				CBFoundTransaction * fndTx = CBNodeFullGetOurTransaction(self, dep->hash);
				if (fndTx) {
					// Add these dependants to the CBFoundTransaction of our transaction we added back as unconfirmed.
					// Loop though dependants
					CBAssociativeArrayForEach(CBTransaction * dependant, &dep->dependants) {
						// Add into the dependants of the CBFoundTransaction
						CBFoundTransaction * fndDep = CBNodeFullGetOurTransaction(self, CBTransactionGetHash(dependant));
						CBAssociativeArrayInsert(&fndTx->dependants, fndDep, CBAssociativeArrayFind(&fndTx->dependants, fndDep).position, NULL);
						// Add to the number of unconf dependencies
						fndDep->utx.numUnconfDeps++;
					}
				}else{
					// Add these dependants to the chain dependency
					// Loop though dependants
					CBAssociativeArrayForEach(CBTransaction * dependant, &dep->dependants) {
						// Add into the dependants of the CBTransactionDependency
						CBFoundTransaction * fndDep = CBNodeFullGetOurTransaction(self, CBTransactionGetHash(dependant));
						CBNodeFullAddToDependency(&self->chainDependencies, dep->hash, fndDep, CBTransactionPtrCompare);
					}
				}
			}
			// Loop through ourLostTxs again and find those with no unconf dependencies
			CBAssociativeArrayForEach(CBTransaction * tx, &self->ourLostTxs) {
				CBFoundTransaction * fndTx = CBNodeFullGetOurTransaction(self, CBTransactionGetHash(tx));
				if (fndTx->utx.numUnconfDeps == 0)
					// All chain deperencies so add to allChainFoundTxs
					CBAssociativeArrayInsert(&self->allChainFoundTxs, fndTx, CBAssociativeArrayFind(&self->allChainFoundTxs, fndTx).position, NULL);
			}
			// Free ourLostsTxs and ourLostTxDependencies
			CBFreeAssociativeArray(&self->ourLostTxs);
			CBFreeAssociativeArray(&self->ourLostTxDependencies);
			self->initialisedOurLostTxs = false;
		}
		// Next process found unconf transactions which were previously validated on the block chain.
		for (uint8_t x = 0; x < 2; x++) {
			if ((x == 0) ? self->initialisedFoundUnconfTxsPrev : self->initialisedFoundUnconfTxsNew) {
				CBAssociativeArray * array = (x == 0) ? &self->foundUnconfTxsPrev : &self->foundUnconfTxsNew;
				CBAssociativeArrayForEach(CBTransactionToBranch * txToBranch, array)
					if (!CBNodeFullUnconfToChain(self, txToBranch->uTx, txToBranch->blockHeight, txToBranch->branch, x == 1)) {
						CBLogError("Could not process an unconfirmed transaction moving onto the chain.");
						return false;
					}
				// Free array
				CBFreeAssociativeArray(array);
			}
		}
		self->initialisedFoundUnconfTxsPrev = false;
		self->initialisedFoundUnconfTxsNew = false;
		// Process the removed unconf transactions
		if (!CBNodeFullRemoveLostUnconfTxs(self)) {
			CBLogError("Could not remove lost unconfirmed transactions from double spends in the chain.");
			return false;
		}
		// Process new transactions
		while (self->newTransactions != NULL) {
			// Call newTransaction
			CBGetNode(self)->callbacks.newTransaction(CBGetNode(self), self->newTransactions->tx, self->newTransactions->time, self->newTransactions->blockHeight, self->newTransactions->accountDetailList);
			// Get next details
			CBNewTransactionDetails * temp = self->newTransactions;
			self->newTransactions = self->newTransactions->next;
			free(temp);
		}
		// Relay again unconfirmed transactions that have not been relayed for so long
		// Create upto 4 invs with 1/4 of transactions or upto the inv limit
		CBInventory * invs[4] = {NULL};
		uint8_t currentInv = 0;
		CBAssociativeArray dependantsProccessed;
		CBInitAssociativeArray(&dependantsProccessed, CBPtrCompare, NULL, NULL);
		CBAssociativeArrayForEach(CBFoundTransaction * firstTx, &self->allChainFoundTxs){
			// Relay this fndTx and then dependants...
			CBProcessQueueItem * queue = &(CBProcessQueueItem){firstTx, NULL};
			CBProcessQueueItem * last = queue;
			while (queue != NULL) {
				CBFoundTransaction * fndTx = queue->item;
				if (fndTx->timeFound < CBGetMilliseconds() - 7200000 && fndTx->utx.type != CB_TX_OURS) {
					// Two hours old, forget it to allow for other transactions. Reuse lostUnconfTxs
					if (!self->initialisedLostUnconfTxs) {
						self->initialisedLostUnconfTxs = true;
						CBInitAssociativeArray(&self->lostUnconfTxs, CBPtrCompare, NULL, NULL);
					}
					CBAssociativeArrayInsert(&self->lostUnconfTxs, fndTx, CBAssociativeArrayFind(&self->lostUnconfTxs, fndTx).position, NULL);
				}else{
					if (fndTx->nextRelay < CBGetMilliseconds()) {
						// Initialise inv if not already
						if (invs[currentInv] == NULL){
							invs[currentInv] = CBNewInventory();
							CBGetMessage(invs[currentInv])->type = CB_MESSAGE_TYPE_INV;
						}
						// Add this transaction
						CBByteArray * hashObj = CBNewByteArrayWithDataCopy(CBTransactionGetHash(fndTx->utx.tx), 32);
						CBInventoryTakeInventoryItem(invs[currentInv], CBNewInventoryItem(CB_INVENTORY_ITEM_TX, hashObj));
						CBReleaseObject(hashObj);
						// Get next relay time
						fndTx->nextRelay = CBGetMilliseconds() + 1800000;
					}
					// Add dependants to queue, which have not been processed already.
					CBAssociativeArrayForEach(CBFoundTransaction * dep, &fndTx->dependants){
						CBFindResult res = CBAssociativeArrayFind(&dependantsProccessed, dep);
						if (! res.found){
							last->next = malloc(sizeof(*last->next));
							last = last->next;
							last->item = dep;
							last->next = NULL;
							CBAssociativeArrayInsert(&dependantsProccessed, dep, res.position, NULL);
						}
					}
				}
				// Move to next in queue
				CBProcessQueueItem * next = queue->next;
				// Free this queue item unless first
				if (fndTx != firstTx)
					free(queue);
				queue = next;
			}
			// Move to the next inv
			if (currentInv == 3)
				currentInv = 0;
			else
				currentInv++;
		}
		CBFreeAssociativeArray(&dependantsProccessed);
		// Loop through and send invs to peers
		CBMutexLock(CBGetNetworkCommunicator(self)->peersMutex);
		CBAssociativeArrayForEach(CBPeer * peer, &CBGetNetworkCommunicator(self)->addresses->peers){
			if (peer->handshakeStatus | CB_HANDSHAKE_GOT_VERSION)
				CBNodeSendMessageOnNetworkThread(CBGetNetworkCommunicator(self), peer, CBGetMessage(invs[currentInv]), NULL);
			// Rotate through invs
			if (currentInv == 3 || invs[++currentInv] == NULL)
				currentInv = 0;
		}
		CBMutexUnlock(CBGetNetworkCommunicator(self)->peersMutex);
		// Release invs
		for (uint8_t x = 0; x < 4; x++)
			if (invs[x] != NULL) CBReleaseObject(invs[x]);
		// Remove old transactions
		if (!CBNodeFullRemoveLostUnconfTxs(self)) {
			CBLogError("Could not remove old unconfirmed transactions.");
			return false;
		}
		// Update block height
		CBGetNetworkCommunicator(self)->blockHeight = blockHeight;
	}else if (self->forkPoint == CB_NO_FORK)
		// Set the fork point if not already
		self->forkPoint = blockHeight;
	// Remove this block from blockPeers and askedForBlocks
	CBNodeFullFinishedWithBlock(self, block);
	return true;
}
bool CBNodeFullAddBlockDirectly(CBNodeFull * self, CBBlock * block){
	self->addingDirect = true;
	if (!CBValidatorAddBlockDirectly(CBGetNode(self)->validator, block, &(CBPeer){.nodeObj=self})){
		CBLogError("Could not add a block directly to the validator.");
		return false;
	}
	self->addingDirect = false;
	return true;
}
CBErrBool CBNodeFullAddOurOrOtherFoundTransaction(CBNodeFull * self, bool ours, CBUnconfTransaction utx, CBFoundTransaction ** fndTxPtr, uint64_t txInputValue){
	char txStr[CB_TX_HASH_STR_SIZE];
	CBTransactionHashToString(utx.tx, txStr);
	// Create found transaction data
	CBFoundTransaction * fndTx = malloc(sizeof(*fndTx));
	if (fndTxPtr != NULL)
		*fndTxPtr = fndTx;
	fndTx->utx = utx;
	fndTx->timeFound = CBGetMilliseconds();
	fndTx->nextRelay = fndTx->timeFound + 1800000;
	fndTx->inputValue = txInputValue;
	CBInitAssociativeArray(&fndTx->dependants, CBTransactionPtrCompare, NULL, NULL);
	if (ours){
		// Store in ourTxs storage
		fndTx->utx.type = CB_TX_OURS;
		CBAssociativeArrayInsert(&self->ourTxs, fndTx, CBAssociativeArrayFind(&self->ourTxs, fndTx).position, NULL);
		// We save to storage
		if (!CBNodeStorageAddOurTx(CBGetNode(self)->nodeStorage, utx.tx)){
			CBLogError("Unable to add our transaction %s to storage.", txStr);
			return CB_ERROR;
		}
		CBLogVerbose("Added transaction %s as ours and unconfirmed.", txStr);
	}else{
		// Store in the otherTxs array, if we have space for it.
		bool fits = true;
		while (self->otherTxsSize + CBGetMessage(utx.tx)->bytes->length > self->otherTxsSizeLimit) {
			// Remove orphan transaction
			if (!CBNodeFullRemoveOrphan(self)) {
				fits = false;
				break;
			}
		}
		if (!fits) {
			// Cannot add this transaction
			CBFreeFoundTransaction(fndTx);
			return CB_FALSE;
		}
		// Add other transaction.
		fndTx->utx.type = CB_TX_OTHER;
		self->otherTxsSize += CBGetMessage(utx.tx)->bytes->length;
		CBAssociativeArrayInsert(&self->otherTxs, fndTx, CBAssociativeArrayFind(&self->otherTxs, fndTx).position, NULL);
		// We save to storage
		if (!CBNodeStorageAddOtherTx(CBGetNode(self)->nodeStorage, utx.tx)){
			CBLogError("Unable to add other transaction %s to storage.", txStr);
			return CB_ERROR;
		}
		CBLogVerbose("Added transaction %s as other and unconfirmed.", txStr);
	}
	// If no unconfirmed dependencies, we can add this to allChainFoundTxs
	if (utx.numUnconfDeps == 0)
		CBAssociativeArrayInsert(&self->allChainFoundTxs, fndTx, CBAssociativeArrayFind(&self->allChainFoundTxs, fndTx).position, NULL);
	// Relay to all peers
	CBInventory * inv = CBNewInventory();
	CBGetMessage(inv)->type = CB_MESSAGE_TYPE_INV;
	CBByteArray * txHash = CBNewByteArrayWithDataCopy(CBTransactionGetHash(utx.tx), 32);
	CBInventoryTakeInventoryItem(inv, CBNewInventoryItem(CB_INVENTORY_ITEM_TX, txHash));
	CBReleaseObject(txHash);
	CBMutexLock(CBGetNetworkCommunicator(self)->peersMutex);
	CBAssociativeArrayForEach(CBPeer * peer, &CBGetNetworkCommunicator(self)->addresses->peers)
	if (peer->handshakeStatus | CB_HANDSHAKE_GOT_VERSION)
		CBNodeSendMessageOnNetworkThread(CBGetNetworkCommunicator(self), peer, CBGetMessage(inv), NULL);
	CBMutexUnlock(CBGetNetworkCommunicator(self)->peersMutex);
	CBReleaseObject(inv);
	return true;
}
void CBNodeFullAddToDependency(CBAssociativeArray * deps, uint8_t * hash, void * el, CBCompare (*compareFunc)(CBAssociativeArray *, void *, void *)){
	CBTransactionDependency * txDep;
	CBFindResult res = CBAssociativeArrayFind(deps, hash);
	if (!res.found){
		// Create dependency
		txDep = malloc(sizeof(*txDep));
		memcpy(txDep->hash, hash, 32);
		CBInitAssociativeArray(&txDep->dependants, compareFunc, NULL, NULL);
		// Insert into dependencies
		CBAssociativeArrayInsert(deps, txDep, res.position, NULL);
	}else
		txDep = CBFindResultToPointer(res);
	res = CBAssociativeArrayFind(&txDep->dependants, el);
	if (!res.found)
		// Not found, therefore add this as a dependant.
		CBAssociativeArrayInsert(&txDep->dependants, el, res.position, NULL);
}
CBErrBool CBNodeFullAddTransactionAsFound(CBNodeFull * self, CBUnconfTransaction utx, CBFoundTransaction ** fndTx, uint64_t txInputValue){
	// Everything is OK so check transaction through accounter
	uint64_t timestamp = time(NULL);
	CBErrBool ours = CBNodeFullFoundTransaction(self, utx.tx, timestamp, 0, CB_NO_BRANCH, true);
	if (ours == CB_ERROR){
		CBLogError("Could not process an unconfirmed transaction with the accounter.");
		return CB_ERROR;
	}
	return CBNodeFullAddOurOrOtherFoundTransaction(self, ours, utx, fndTx, txInputValue);
}
uint64_t CBNodeFullAlreadyValidated(void * vpeer, CBTransaction * tx){
	CBFoundTransaction * fndTx = CBNodeFullGetFoundTransaction(CBGetPeer(vpeer)->nodeObj, CBTransactionGetHash(tx));
	return (fndTx == NULL)? 0 : fndTx->inputValue;
}
void CBNodeFullAskForBlock(CBNodeFull * self, CBPeer * peer, uint8_t * hash){
	// Make this block expected from this peer
	memcpy(peer->expectedBlock, peer->reqBlocks[peer->reqBlockCursor].reqBlock, 20);
	// Create getdata message
	CBInventory * getData = CBNewInventory();
	CBByteArray * hashObj = CBNewByteArrayWithDataCopy(hash, 32);
	CBInventoryTakeInventoryItem(getData, CBNewInventoryItem(CB_INVENTORY_ITEM_BLOCK, hashObj));
	CBReleaseObject(hashObj);
	CBGetMessage(getData)->type = CB_MESSAGE_TYPE_GETDATA;
	// Send getdata
	CBNodeSendMessageOnNetworkThread(CBGetNetworkCommunicator(self), peer, CBGetMessage(getData), NULL);
}
bool CBNodeFullBroadcastTransaction(CBNodeFull * self, CBTransaction * tx, uint32_t numUnconfDeps, uint64_t txInputValue){
	// Add to accounter
	if (CBNodeFullFoundTransaction(self, tx, time(NULL), 0, CB_NO_BRANCH, true) != CB_TRUE) {
		CBLogError("Could not add a broadcast transaction to the accounter.");
		return false;
	}
	return CBNodeFullAddOurOrOtherFoundTransaction(self, true, (CBUnconfTransaction){tx, CB_TX_OURS, numUnconfDeps}, NULL, txInputValue);
}
void CBNodeFullCancelBlock(CBNodeFull * self, CBPeer * peer){
	CBMutexLock(self->pendingBlockDataMutex);
	// Add block to be downloaded by another peer, and remove this peer from this block's peers. If this peer was the only one to have the block, then forget it.
	CBFindResult res = CBAssociativeArrayFind(&self->blockPeers, peer->expectedBlock);
	CBBlockPeers * blockPeers = CBFindResultToPointer(res);
	if (blockPeers->peers.root->numElements == 1 && blockPeers->peers.root->children[0] == NULL) {
		// Only this peer has the block so delete the blockPeers and end.
		CBAssociativeArrayDelete(&self->blockPeers, res.position, true);
		return;
	}
	// Remove this peer from the block's peers, if found
	res = CBAssociativeArrayFind(&blockPeers->peers, peer);
	if (res.found)
		CBAssociativeArrayDelete(&blockPeers->peers, res.position, true);
	// Now get the first peer we can get this block from.
	CBPosition it;
	CBAssociativeArrayGetFirst(&blockPeers->peers, &it);
	CBPeer * next = it.node->elements[it.index];
	// Now add this block to be got next.
	if (next->downloading) {
		// Remove block from askedFor
		CBAssociativeArrayDelete(&self->askedForBlocks, CBAssociativeArrayFind(&self->askedForBlocks, peer->expectedBlock).position, false);
		// Find the block in the required blocks on this node and reposition it to be the next block to get.
		uint16_t prevX = UINT16_MAX;
		uint16_t highestX = 0;
		for (uint16_t x = next->reqBlockStart;; x = next->reqBlocks[x].next) {
			if (memcmp(next->reqBlocks[x].reqBlock, peer->expectedBlock, 32)) {
				// Found the block, so reposition
				if (prevX != UINT16_MAX)
					next->reqBlocks[prevX].next = next->reqBlocks[x].next;
				next->reqBlocks[x].next = next->reqBlocks[next->reqBlockCursor].next;
				next->reqBlocks[next->reqBlockCursor].next = x;
				break;
			}
			if (next->reqBlocks[x].next == CB_END_REQ_BLOCKS) {
				// Got to the end. The peer must have advertised this block in a previous inventory
				uint16_t replace;
				if (highestX != 499){
					// Room to use another element
					next->reqBlocks[highestX+1].next = next->reqBlocks[next->reqBlockCursor].next;
					next->reqBlocks[next->reqBlockCursor].next = highestX + 1;
					replace = highestX + 1;
				}else{
					// Must use either block before current or last block
					if (next->reqBlockCursor == next->reqBlockStart){
						// Must use last
						next->reqBlocks[prevX].next = CB_END_REQ_BLOCKS;
						next->reqBlocks[x].next = next->reqBlocks[next->reqBlockCursor].next;
						next->reqBlocks[next->reqBlockCursor].next = x;
						replace = x;
					}else{
						// Use first
						uint16_t replace = next->reqBlockStart;
						next->reqBlockStart = next->reqBlocks[replace].next;
						next->reqBlocks[replace].next = next->reqBlocks[next->reqBlockCursor].next;
						next->reqBlocks[next->reqBlockCursor].next = replace;
					}
				}
				memcpy(next->reqBlocks[replace].reqBlock, peer->expectedBlock, 32);
				break;
			}
			prevX = x;
		}
	}else{
		// We need to start downloading from this next peer, just for this block.
		next->downloading = true;
		next->expectBlock = true;
		CBNodeFullAskForBlock(self, next, peer->expectedBlock);
	}
	CBMutexUnlock(self->pendingBlockDataMutex);
}
bool CBNodeFullDeleteBranchCallback(void * vpeer, uint8_t branch){
	CBNode * node = CBGetPeer(vpeer)->nodeObj;
	if (!CBAccounterDeleteBranch(node->accounterStorage, branch)){
		CBLogError("Could not delete the accounter branch information.");
		return false;
	}
	return true;
}
void CBNodeFullDeleteOrphanFromDependencies(CBNodeFull * self, CBOrphan * orphan){
	for (uint32_t x = 0; x < orphan->utx.tx->inputNum; x++) {
		// Get prevout hash
		uint8_t * hash = CBByteArrayGetData(orphan->utx.tx->inputs[x]->prevOut.hash);
		// Look for found transaction
		CBFoundTransaction * fndTx = CBNodeFullGetFoundTransaction(self, hash);
		if (fndTx != NULL)
			// Remove orphan dependant from found transaction data
			CBAssociativeArrayDelete(&fndTx->dependants, CBAssociativeArrayFind(&fndTx->dependants, orphan).position, true);
		else{
			// Look for unfound transaction dependencies
			CBFindResult res = CBAssociativeArrayFind(&self->unfoundDependencies, hash);
			if (res.found) {
				// We have an unfound transaction dependency, remove this dependant
				CBTransactionDependency * txDep = CBFindResultToPointer(res);
				CBAssociativeArrayDelete(&txDep->dependants, CBAssociativeArrayFind(&txDep->dependants, &(CBOrphanDependency){orphan, x}).position, true);
				// Do not delete transaction dependency, even if empty. What's the point? It will be deleted later.
			}else
				// Must be a chain dependency
				CBNodeFullRemoveFromDependency(&self->chainDependencies, hash, orphan);
		}
	}
}
void CBNodeFullDownloadFromSomePeer(CBNodeFull * self){
	// Loop through peers to find one we can download from
	CBMutexLock(CBGetNetworkCommunicator(self)->peersMutex);
	// ??? Order peers by block height
	CBAssociativeArrayForEach(CBPeer * peer, &CBGetNetworkCommunicator(self)->addresses->peers){
		if (!peer->downloading && !peer->upload && !peer->upToDate && peer->versionMessage && peer->versionMessage->services | CB_SERVICE_FULL_BLOCKS) {
			// Ask this peer for blocks.
			// Send getblocks
			if (!CBNodeFullSendGetBlocks(self, peer))
				CBLogError("Could not send getblocks for a new peer.");
				// Ignore error.
			else{
				// We don't know what branch the peer is going to give us yet
				peer->branchWorkingOn = CB_NO_BRANCH;
				peer->allowNewBranch = true;
				peer->downloading = true;
			}
		}
	}
	CBMutexUnlock(CBGetNetworkCommunicator(self)->peersMutex);
}
void CBNodeFullFinishedWithBlock(CBNodeFull * self, CBBlock * block){
	if (self->addingDirect)
		return;
	CBMutexLock(self->pendingBlockDataMutex);
	CBAssociativeArrayDelete(&self->askedForBlocks, CBAssociativeArrayFind(&self->askedForBlocks, CBBlockGetHash(block)).position, false);
	CBAssociativeArrayDelete(&self->blockPeers, CBAssociativeArrayFind(&self->blockPeers, CBBlockGetHash(block)).position, true);
	CBMutexUnlock(self->pendingBlockDataMutex);
}
CBErrBool CBNodeFullFoundTransaction(CBNodeFull * self, CBTransaction * tx, uint64_t time, uint32_t blockHeight, uint8_t branch, bool callNow){
	// Add transaction
	CBTransactionAccountDetailList * list;
	CBErrBool ours = CBAccounterFoundTransaction(CBGetNode(self)->accounterStorage, tx, blockHeight, time, branch, &list);
	if (ours == CB_ERROR) {
		CBLogError("Could not process a transaction with the accounter.");
		return false;
	}
	// If ours, we will need to call the callback...
	if (ours == CB_TRUE) {
		if (callNow){
			// We can call the callback straightaway
			CBGetNode(self)->callbacks.newTransaction(CBGetNode(self), tx, time, blockHeight, list);
			CBFreeTransactionAccountDetailList(list);
		}else{
			// Record in newTransactions.
			CBNewTransactionDetails * details = malloc(sizeof(*details));
			details->tx = tx;
			details->time = time;
			details->blockHeight = blockHeight;
			details->accountDetailList = list;
			if (self->newTransactions == NULL)
				self->newTransactions = self->newTransactionsLast = details;
			else
				self->newTransactionsLast = self->newTransactionsLast->next = details;
		}
	}
	return true;
}
CBUnconfTransaction * CBNodeFullGetAnyTransaction(CBNodeFull * self, uint8_t * hash){
	CBUnconfTransaction * uTx = (CBUnconfTransaction *)CBNodeFullGetOurTransaction(self, hash);
	if (uTx == NULL){
		uTx = (CBUnconfTransaction *)CBNodeFullGetOtherTransaction(self, hash);
		if (uTx == NULL){
			uTx = (CBUnconfTransaction *)CBNodeFullGetOrphanTransaction(self, hash);
			if (uTx == NULL)
				return NULL;
			else
				uTx->type = CB_TX_ORPHAN;
		}else
			uTx->type = CB_TX_OTHER;
	}else
		uTx->type = CB_TX_OURS;
	return uTx;
}
CBFoundTransaction * CBNodeFullGetFoundTransaction(CBNodeFull * self, uint8_t * hash){
	CBFoundTransaction * tx = CBNodeFullGetOtherTransaction(self, hash);
	if (tx == NULL)
		tx = CBNodeFullGetOurTransaction(self, hash);
	return tx;
}
CBOrphan * CBNodeFullGetOrphanTransaction(CBNodeFull * self, uint8_t * hash){
	self->orphanTxs.compareFunc = CBTransactionPtrAndHashCompare;
	CBFindResult res = CBAssociativeArrayFind(&self->orphanTxs, hash);
	self->orphanTxs.compareFunc = CBTransactionPtrCompare;
	if (res.found)
		return CBFindResultToPointer(res);
	return NULL;
}
CBFoundTransaction * CBNodeFullGetOtherTransaction(CBNodeFull * self, uint8_t * hash){
	self->otherTxs.compareFunc = CBTransactionPtrAndHashCompare;
	CBFindResult res = CBAssociativeArrayFind(&self->otherTxs, hash);
	self->otherTxs.compareFunc = CBTransactionPtrCompare;
	return (res.found) ? CBFindResultToPointer(res) : NULL;
}
CBFoundTransaction * CBNodeFullGetOurTransaction(CBNodeFull * self, uint8_t * hash){
	self->ourTxs.compareFunc = CBTransactionPtrAndHashCompare;
	CBFindResult res = CBAssociativeArrayFind(&self->ourTxs, hash);
	self->ourTxs.compareFunc = CBTransactionPtrCompare;
	return (res.found) ? CBFindResultToPointer(res) : NULL;
}
void CBNodeFullHasNotGivenBlockInv(void * vpeer){
	// Give a NOT_GIVEN_INV message to process queue
	CBPeer * peer = vpeer;
	CBMutexLock(peer->invResponseMutex);
	CBMessage * message = CBNewMessageByObject();
	message->type = CB_MESSAGE_TYPE_NOT_GIVEN_INV;
	CBNodeOnMessageReceived(peer->nodeObj, peer, message);
	CBReleaseObject(message);
	CBMutexUnlock(peer->invResponseMutex);
}
CBErrBool CBNodeFullHasTransaction(CBNodeFull * self, uint8_t * hash){
	if (CBNodeFullGetAnyTransaction(self, hash) != NULL)
		return CB_TRUE;
	return CBBlockChainStorageTransactionExists(CBGetNode(self)->validator, hash);
}
bool CBNodeFullInvalidBlock(void * vpeer, CBBlock * block){
	CBPeer * peer = vpeer;
	CBNodeFull * self = peer->nodeObj;
	if (self->initialisedOurLostTxs) {
		// Delete our lost transactions and their dependencies.
		CBFreeAssociativeArray(&self->ourLostTxDependencies);
		CBFreeAssociativeArray(&self->ourLostTxs);
		self->initialisedOurLostTxs = false;
	}
	if (self->initialisedFoundUnconfTxsNew) {
		// Delete the found unconfirmed transactions array for newly validated blocks.
		CBFreeAssociativeArray(&self->foundUnconfTxsNew);
		self->initialisedFoundUnconfTxsNew = false;
	}
	if (self->initialisedFoundUnconfTxsPrev) {
		// Delete the found unconfirmed transactions array for previously validated blocks.
		CBFreeAssociativeArray(&self->foundUnconfTxsPrev);
		self->initialisedFoundUnconfTxsPrev = false;
	}
	if (self->initialisedLostUnconfTxs) {
		// Delete the lost unconfirmed transactions array
		CBFreeAssociativeArray(&self->lostUnconfTxs);
		self->initialisedLostUnconfTxs = false;
	}
	// Finished with the block chain
	CBMutexUnlock(CBGetNode(self)->blockAndTxMutex);
	// Disconnect this node
	CBNetworkCommunicatorDisconnect(peer->nodeObj, peer, CB_24_HOURS, false);
	// Also look for other nodes that could have provided this block
	CBMutexLock(self->pendingBlockDataMutex);
	CBFindResult res = CBAssociativeArrayFind(&self->blockPeers, CBBlockGetHash(block));
	CBBlockPeers * blockPeers = CBFindResultToPointer(res);
	CBAssociativeArrayForEach(CBPeer * other, &blockPeers->peers)
		// Disconnect the peer
		CBNetworkCommunicatorDisconnect(other->nodeObj, other, CB_24_HOURS, false);
	// Remove block from blockPeers and askedForBlocks, as we are finished with it.
	CBAssociativeArrayDelete(&self->askedForBlocks, CBAssociativeArrayFind(&self->askedForBlocks, CBBlockGetHash(block)).position, false);
	CBAssociativeArrayDelete(&self->blockPeers, res.position, true);
	CBMutexUnlock(self->pendingBlockDataMutex);
	return true;
}
bool CBNodeFullIsOrphan(void * vpeer, CBBlock * block){
	CBPeer * peer = vpeer;
	CBNodeFull * node = peer->nodeObj;
	if (peer->upToDate)
		// Get previous block
		CBNodeFullAskForBlock(node, peer, CBByteArrayGetData(block->prevBlockHash));
	return true;
}
void CBNodeFullLoseOurDependants(CBNodeFull * self, uint8_t * hash){
	CBFindResult res = CBAssociativeArrayFind(&self->ourLostTxDependencies, hash);
	if (res.found) {
		CBFindResultProcessQueueItem * queue = &(CBFindResultProcessQueueItem){res, NULL};
		CBFindResultProcessQueueItem * last = queue;
		CBFindResultProcessQueueItem * first = queue;
		while (queue != NULL) {
			// This is a dependency, so we want to remove our lost txs.
			// Loop through dependants
			CBTransactionDependency * dep = CBFindResultToPointer(queue->res);
			CBAssociativeArrayForEach(CBTransaction * dependant, &dep->dependants) {
				// See if the dependant has been dealt with already
				CBFindResult depRes = CBAssociativeArrayFind(&self->ourLostTxs, dependant);
				if (depRes.found) {
					// Now add this lost transaction dependency information if it exists to the queue
					res = CBAssociativeArrayFind(&self->ourLostTxDependencies, CBTransactionGetHash(dependant));
					if (res.found) {
						last->next = malloc(sizeof(*last->next));
						last = last->next;
						last->res = res;
						last->next = NULL;
					}
					// Remove transaction from ourLostTxs
					CBAssociativeArrayDelete(&self->ourLostTxs, depRes.position, true);
				}
			}
			// Remove dependency
			CBAssociativeArrayDelete(&self->ourLostTxDependencies, queue->res.position, true);
			// Move to next in queue
			CBFindResultProcessQueueItem * next = queue->next;
			// Free this queue item unless first
			if (queue != first)
				free(queue);
			queue = next;
		}
	}
}
void CBNodeFullLoseUnconfDependants(CBNodeFull * self, uint8_t * hash){
	CBFindResult res = CBAssociativeArrayFind(&self->chainDependencies, hash);
	if (res.found) {
		// This transaction is indeed a dependency of at least one transaction we have as unconfirmed
		// Loop through dependants
		CBTransactionDependency * dep = CBFindResultToPointer(res);
		CBAssociativeArrayForEach(CBTransaction * dependant, &dep->dependants) {
			// Add transaction to lostUnconfTxs
			if (! self->initialisedLostUnconfTxs)
				CBInitAssociativeArray(&self->lostUnconfTxs, CBPtrCompare, NULL, NULL);
			CBAssociativeArrayInsert(&self->lostUnconfTxs, dependant, CBAssociativeArrayFind(&self->lostUnconfTxs, dependant).position, NULL);
		}
	}
}
bool CBNodeFullNewBranchCallback(void * vpeer, uint8_t branch, uint8_t parent, uint32_t blockHeight){
	CBNode * node = CBGetPeer(vpeer)->nodeObj;
	if (!CBAccounterNewBranch(node->accounterStorage, branch, parent, blockHeight)){
		CBLogError("Could not create new accounter branch information.");
		return false;
	}
	return true;
}
bool CBNodeFullNoNewBranches(void * vpeer, CBBlock * block){
	CBPeer * peer = vpeer;
	CBNodeFull * self = peer->nodeObj;
	// Can't accept blocks from this peer at the moment
	CBMutexUnlock(CBGetNode(self)->blockAndTxMutex);
	CBNodeFullPeerDownloadEnd(self, peer);
	// Remove this block from blockPeers and askedForBlocks
	CBNodeFullFinishedWithBlock(self, block);
	return true;
}
void CBNodeFullOnNetworkError(CBNetworkCommunicator * comm){
	CBNode * node = CBGetNode(comm);
	node->callbacks.onFatalNodeError(node);
}
bool CBNodeFullOnTimeOut(CBNetworkCommunicator * comm, void * vpeer){
	// An expected response timed out.
	CBPeer * peer = vpeer;
	CBNodeFull * self = peer->nodeObj;
	// If an expected message is a verack or pong then we should disconnect the node. Also look for expecting block or inv
	if (peer->typesExpectedNum > 1)
		// Must be expecting verack or pong so disconnect the node.
		return true;
	if (peer->typesExpected[0] == CB_MESSAGE_TYPE_BLOCK) {
		CBLogWarning("%s has not given us the requested block in time.", peer->peerStr);
		// The peer has not given us the block in time.
		CBNodeFullPeerDownloadEnd(self, peer);
		CBNodeFullCancelBlock(self, peer);
		// Do not disconnect this node.
		return false;
	}
	// Must be verack or pong so disconnect the node.
	return true;
}
void CBNodeFullPeerDownloadEnd(CBNodeFull * self, CBPeer * peer){
	CBMutexLock(self->numberPeersDownloadMutex);
	self->numberPeersDownload--;
	CBMutexUnlock(self->numberPeersDownloadMutex);
	peer->expectBlock = false;
	peer->downloading = false;
	// Not working on any branches
	if (peer->branchWorkingOn != CB_NO_BRANCH) {
		CBMutexLock(self->workingMutex);
		CBGetNode(self)->validator->branches[peer->branchWorkingOn].working--;
		CBMutexUnlock(self->workingMutex);
		peer->branchWorkingOn = CB_NO_BRANCH;
		peer->allowNewBranch = true;
	}
	// See if we can download from any other peers
	CBNodeFullDownloadFromSomePeer(self);
}
void CBNodeFullPeerFree(void * vpeer){
	CBPeer * peer = vpeer;
	CBNodeFull * fullNode = peer->nodeObj;
	if (peer->invResponseTimerStarted)
		CBEndTimer(peer->invResponseTimer);
	// Wait until timer callback unlocks
	CBMutexLock(peer->invResponseMutex);
	CBMutexUnlock(peer->invResponseMutex);
	CBFreeMutex(peer->invResponseMutex);
	if (peer->downloading){
		// If expected a block from the peer, we should ask another
		if (peer->expectBlock)
			CBNodeFullCancelBlock(fullNode, peer);
		// Not downloading from this peer anymore
		CBMutexLock(fullNode->numberPeersDownloadMutex);
		fullNode->numberPeersDownload--;
		CBMutexUnlock(fullNode->numberPeersDownloadMutex);
		if (peer->branchWorkingOn != CB_NO_BRANCH){
			CBMutexLock(fullNode->workingMutex);
			CBGetNode(peer->nodeObj)->validator->branches[peer->branchWorkingOn].working--;
			CBMutexUnlock(fullNode->workingMutex);
		}
	}
	// If can download from this peer, decrement numCanDownload
	if (peer->versionMessage && peer->versionMessage->services | CB_SERVICE_FULL_BLOCKS) {
		CBMutexLock(fullNode->numCanDownloadMutex);
		fullNode->numCanDownload--;
		CBMutexUnlock(fullNode->numCanDownloadMutex);
	}
	CBFreeAssociativeArray(&peer->expectedTxs);
	if (peer->requestedData != NULL) CBReleaseObject(peer->requestedData);
	// See if we can download from any other peers
	peer->downloading = true; // Ensure not this one.
	CBNodeFullDownloadFromSomePeer(fullNode);
}
void CBNodeFullPeerSetup(CBNetworkCommunicator * self, CBPeer * peer){
	CBInitAssociativeArray(&peer->expectedTxs, CBHashCompare, NULL, free);
	peer->downloading = false;
	peer->expectBlock = false;
	peer->requestedData = NULL;
	peer->nodeObj = self;
	peer->upToDate = false;
	peer->invResponseTimerStarted = false;
	CBNewMutex(&peer->invResponseMutex);
}
bool CBNodeFullPeerWorkingOnBranch(void * vpeer, uint8_t branch){
	CBPeer * peer = vpeer;
	CBNodeFull * self = peer->nodeObj;
	if (peer->branchWorkingOn == branch)
		// Using branch as before
		return true;
	if (!peer->allowNewBranch)
		// The peer gave us a non-sequentual block in an inv
		return false;
	CBMutexLock(self->workingMutex);
	if (peer->branchWorkingOn != CB_NO_BRANCH)
		// Peer is no longer working on this branch
		CBGetNode(self)->validator->branches[peer->branchWorkingOn].working--;
	// Now working on this branch
	CBGetNode(self)->validator->branches[branch].working++;
	CBMutexUnlock(self->workingMutex);
	peer->branchWorkingOn = branch;
	return true;
}
CBOnMessageReceivedAction CBNodeFullProcessMessage(CBNode * node, CBPeer * peer, CBMessage * message){
	CBNodeFull * self = CBGetNodeFull(node);
	CBValidator * validator = node->validator;
	switch (message->type) {
		case CB_MESSAGE_TYPE_VERSION:{
			// Disconnect if the peer can't give us blocks but we don't have at least CB_MIN_DOWNLOAD peers we can download from.
			CBMutexLock(self->numCanDownloadMutex);
			if (CBGetVersion(message)->services | CB_SERVICE_FULL_BLOCKS) {
				self->numCanDownload++;
			}else if (self->numCanDownload < CB_MIN_DOWNLOAD)
				return CB_MESSAGE_ACTION_DISCONNECT;
			CBMutexUnlock(self->numCanDownloadMutex);
			// Give our transactions
			CBInventory * inv = NULL;
			CBAssociativeArray dependantsProccessed;
			CBInitAssociativeArray(&dependantsProccessed, CBPtrCompare, NULL, NULL);
			CBMutexLock(CBGetNode(self)->blockAndTxMutex);
			CBAssociativeArrayForEach(CBFoundTransaction * firstTx, &self->allChainFoundTxs){
				// Relay this fndTx and then dependants...
				CBProcessQueueItem * queue = &(CBProcessQueueItem){firstTx, NULL};
				CBProcessQueueItem * last = queue;
				while (queue != NULL) {
					CBFoundTransaction * fndTx = queue->item;
					// Initialise inv if not already
					if (inv == NULL)
						inv = CBNewInventory();
					// Add this transaction
					CBByteArray * hashObj = CBNewByteArrayWithDataCopy(CBTransactionGetHash(fndTx->utx.tx), 32);
					CBInventoryTakeInventoryItem(inv, CBNewInventoryItem(CB_INVENTORY_ITEM_TX, hashObj));
					CBReleaseObject(hashObj);
					// Add dependants to queue
					CBAssociativeArrayForEach(CBFoundTransaction * dep, &fndTx->dependants){
						CBFindResult res = CBAssociativeArrayFind(&dependantsProccessed, dep);
						if (! res.found){
							last->next = malloc(sizeof(*last->next));
							last = last->next;
							last->item = dep;
							last->next = NULL;
							CBAssociativeArrayInsert(&dependantsProccessed, dep, res.position, NULL);
						}
					}
					// Move to next in queue
					CBProcessQueueItem * next = queue->next;
					// Free this queue item unless first
					if (fndTx != firstTx)
						free(queue);
					queue = next;
				}
			}
			CBMutexUnlock(CBGetNode(self)->blockAndTxMutex);
			CBFreeAssociativeArray(&dependantsProccessed);
			// Send inventory
			if (inv != NULL){
				CBGetMessage(inv)->type = CB_MESSAGE_TYPE_INV;
				CBNodeSendMessageOnNetworkThread(CBGetNetworkCommunicator(self), peer, CBGetMessage(inv), NULL);
				CBReleaseObject(inv);
			}
			// Ask for blocks, unless there is already CB_MAX_BLOCK_QUEUE peers giving us blocks or the peer advertises a block height equal or less than ours.
			CBMutexLock(self->numberPeersDownloadMutex);
			if (CBGetVersion(message)->blockHeight <= CBGetNetworkCommunicator(self)->blockHeight
				|| self->numberPeersDownload == CB_MAX_BLOCK_QUEUE)
				break;
			self->numberPeersDownload++;
			CBMutexUnlock(self->numberPeersDownloadMutex);
			// Send getblocks
			if (!CBNodeFullSendGetBlocks(self, peer))
				return CBNodeReturnError(node, "Could not send getblocks for a new peer.");
			// We don't know what branch the peer is going to give us yet
			peer->branchWorkingOn = CB_NO_BRANCH;
			peer->allowNewBranch = true;
			peer->downloading = true;
			break;
		}
		case CB_MESSAGE_TYPE_GETBLOCKS:
			return CBNodeSendBlocksInvOrHeaders(CBGetNode(self), peer, CBGetGetBlocks(message), true);
		case CB_MESSAGE_TYPE_BLOCK:{
			// The peer sent us a block
			CBBlock * block = CBGetBlock(message);
			char blockStr[CB_BLOCK_HASH_STR_SIZE];
			CBBlockHashToString(block, blockStr);
			CBLogVerbose("%s gave us the block %s.", peer->peerStr, blockStr);
			// See if we expected this block
			if (memcmp(peer->expectedBlock, CBBlockGetHash(block), 20)){
				// We did not expect this block from this peer.
				CBLogWarning("%s sent us an unexpected block.", peer->peerStr);
				return CB_MESSAGE_ACTION_DISCONNECT;
			}
			// Do the basic validation
			CBBlockValidationResult result = CBValidatorBasicBlockValidation(validator, block, CBNetworkAddressManagerGetNetworkTime(CBGetNetworkCommunicator(self)->addresses));
			switch (result) {
				case CB_BLOCK_VALIDATION_BAD:
					CBLogWarning("The block %s did not pass basic validation.", blockStr);
					return CB_MESSAGE_ACTION_DISCONNECT;
				case CB_BLOCK_VALIDATION_OK:
					CBLogVerbose("The block %s passed basic validation.", blockStr);
					// Queue the block for processing
					CBValidatorQueueBlock(validator, block, peer);
				default:
					return CB_MESSAGE_ACTION_CONTINUE;
			}
		}
		case CB_MESSAGE_TYPE_GETDATA:{
			// The peer is requesting data. It should only be requesting transactions or blocks that we own.
			CBInventory * getData = CBGetInventory(message);
			// Begin sending requested data, or add the requested data to the eisting requested data.
			if (peer->requestedData == NULL) {
				peer->sendDataIndex = 0;
				peer->requestedData = getData;
				CBRetainObject(peer->requestedData);
				if (CBNodeFullSendRequestedData(self, peer))
					// Return since the node was stoped.
					return CB_MESSAGE_ACTION_RETURN;
			}else{
				// Add to existing requested data
				uint16_t addNum = getData->itemNum;
				if (addNum + peer->requestedData->itemNum > 50000)
					// Adjust to give no more than 50000
					addNum = 50000 - peer->requestedData->itemNum;
				for (uint16_t x = 0; x < addNum; x++)
					CBInventoryAddInventoryItem(peer->requestedData, CBInventoryGetInventoryItem(getData, x));
			}
			break;
		}
		case CB_MESSAGE_TYPE_INV:{
			// The peer has sent us an inventory broadcast
			CBInventory * inv = CBGetInventory(message);
			// Get data request for objects we want.
			CBInventory * getData = NULL;
			uint16_t reqBlockNum = 0;
			for (uint16_t x = 0; x < inv->itemNum; x++) {
				CBInventoryItem * item = CBInventoryGetInventoryItem(inv, x);
				uint8_t * hash = CBByteArrayGetData(item->hash);
				if (item->type == CB_INVENTORY_ITEM_BLOCK) {
					if (reqBlockNum == 0 && peer->expectBlock){
						// We are still obtaining the required blocks from this peer, do not accept any more blocks until we are done getting the previous ones.
						CBLogVerbose("Ignoring blocks from %s as we are still downloading from them.", peer->peerStr);
						continue;
					}
					// Check if we have the block
					CBMutexLock(node->blockAndTxMutex);
					CBErrBool exists = CBBlockChainStorageBlockExists(validator, hash);
					CBMutexUnlock(node->blockAndTxMutex);
					if (exists == CB_ERROR)
						return CBNodeReturnError(node, "Could not determine if we have a block in an inventory message.");
					if (exists == CB_TRUE)
						// Ignore this one.
						continue;
					// Ensure we have not asked for this block yet.
					CBMutexLock(self->pendingBlockDataMutex);
					CBFindResult res = CBAssociativeArrayFind(&self->askedForBlocks, hash);
					if (res.found){
						CBMutexUnlock(self->pendingBlockDataMutex);
						continue;
					}
					char blockStr[40];
					CBByteArrayToString(item->hash, 0, 20, blockStr);
					CBLogVerbose("%s gave us hash of block %s", peer->peerStr, blockStr);
					// Add block to required blocks from peer
					memcpy(peer->reqBlocks[reqBlockNum].reqBlock, hash, 20);
					peer->reqBlocks[reqBlockNum].next = reqBlockNum + 1;
					// Create or get block peers information
					CBBlockPeers * blockPeers;
					res = CBAssociativeArrayFind(&self->blockPeers, hash);
					if (res.found)
						blockPeers = CBFindResultToPointer(res);
					else{
						blockPeers = malloc(sizeof(*blockPeers));
						memcpy(blockPeers->hash, hash, 20);
						CBInitAssociativeArray(&blockPeers->peers, CBPtrCompare, NULL, NULL);
						CBAssociativeArrayInsert(&self->blockPeers, blockPeers, res.position, NULL);
					}
					if (reqBlockNum++ == 0) {
						// Ask for first block
						memcpy(peer->expectedBlock, hash, 20);
						peer->expectBlock = true;
						// Create getdata message if not already and then add this block hash as an item.
						if (getData == NULL)
							getData = CBNewInventory();
						CBInventoryAddInventoryItem(getData, item);
						// Record that this block has been asked for
						// Thus we will not require this from any other peers, unless there is a timeout
						CBAssociativeArrayInsert(&self->askedForBlocks, blockPeers, CBAssociativeArrayFind(&self->askedForBlocks, blockPeers).position, NULL);
						// Set reqBlockCursor and reqBlockStart for peer
						peer->reqBlockCursor = 0;
						peer->reqBlockStart = 0;
						// We expect a block
						CBGetMessage(getData)->expectResponse = CB_MESSAGE_TYPE_BLOCK;
					}else
						// Record that this peer has this block
						CBAssociativeArrayInsert(&blockPeers->peers, peer, CBAssociativeArrayFind(&blockPeers->peers, peer).position, NULL);
					CBMutexUnlock(self->pendingBlockDataMutex);
				}else if (item->type == CB_INVENTORY_ITEM_TX){
					// Check if we have the transaction already
					CBMutexLock(node->blockAndTxMutex);
					CBErrBool exists = CBNodeFullHasTransaction(self, hash);
					CBMutexUnlock(node->blockAndTxMutex);
					if (exists == CB_ERROR)
						return CBNodeReturnError(node, "Could not determine if we have a transaction from an inventory message.");
					if (exists == CB_TRUE)
						continue;
					// Ensure we do not expect this transaction
					CBFindResult res = CBAssociativeArrayFind(&peer->expectedTxs, hash);
					if (res.found)
						continue;
					char txStr[40];
					CBByteArrayToString(item->hash, 0, 20, txStr);
					CBLogVerbose("%s gave us hash of transaction %s", peer->peerStr, txStr);
					// We do not have the transaction, so add it to getdata
					if (getData == NULL)
						getData = CBNewInventory();
					CBInventoryAddInventoryItem(getData, item);
					// Also add it to expectedTxs
					uint8_t * insertHash = malloc(20);
					memcpy(insertHash, hash, 20);
					CBAssociativeArrayInsert(&peer->expectedTxs, insertHash, res.position, NULL);
				}else continue;
			}
			if (reqBlockNum != 0) {
				// Has blocks in inventory
				CBEndTimer(peer->invResponseTimer);
				peer->invResponseTimerStarted = false;
				// Add CB_END_REQ_BLOCKS to last required block
				peer->reqBlocks[reqBlockNum-1].next = CB_END_REQ_BLOCKS;
			}
			// Send getdata if not NULL
			if (getData != NULL){
				CBGetMessage(getData)->type = CB_MESSAGE_TYPE_GETDATA;
				CBNodeSendMessageOnNetworkThread(CBGetNetworkCommunicator(self), peer, CBGetMessage(getData), NULL);
			}
			break;
		}
		case CB_MESSAGE_TYPE_TX:{
			CBTransaction * tx = CBGetTransaction(message);
			char txStr[CB_TX_HASH_STR_SIZE];
			CBTransactionHashToString(tx, txStr);
			CBLogVerbose("%s gave us the transaction %s", peer->peerStr, txStr);
			// Ensure we expected this transaction
			CBFindResult res = CBAssociativeArrayFind(&peer->expectedTxs, CBTransactionGetHash(tx));
			if (!res.found){
				// Did not expect this transaction
				CBLogWarning("%s gave us an unexpected transaction.", peer->peerStr);
				return CB_MESSAGE_ACTION_DISCONNECT;
			}
			CBMutexLock(node->blockAndTxMutex);
			// Ensure inbetween requesting this transaction and receiving it, we have not found it yet.
			CBErrBool has = CBNodeFullHasTransaction(self, CBTransactionGetHash(tx));
			if (has != CB_FALSE) {
				CBMutexUnlock(node->blockAndTxMutex);
				if (has == CB_ERROR)
					return CBNodeReturnError(node, "Could not determine if we have a transaction sent to us already.");
				break;
			}
			// Remove from expected transactions
			CBAssociativeArrayDelete(&peer->expectedTxs, res.position, true);
			// Check the validity of the transaction
			uint64_t outputValue;
			if (!CBTransactionValidateBasic(tx, false, &outputValue))
				return CB_MESSAGE_ACTION_DISCONNECT;
			// Macro for breaking and freeing mutex
			#define breakMutex CBMutexUnlock(node->blockAndTxMutex); break
			// Check is standard
			if (node->flags & CB_NODE_CHECK_STANDARD
				&& !CBTransactionIsStandard(tx)){
				// Ignore
				CBLogVerbose("Transaction %s is non-standard.", txStr);
				breakMutex;
			}
			CBBlockBranch * mainBranch = &validator->branches[validator->mainBranch];
			uint32_t lastHeight = mainBranch->startHeight + mainBranch->lastValidation;
			// Check that the transaction is final, else ignore
			if (! CBTransactionIsFinal(tx, CBNetworkAddressManagerGetNetworkTime(CBGetNetworkCommunicator(self)->addresses), lastHeight)){
				CBLogVerbose("Transaction %s is not final.", txStr);
				breakMutex;
			}
			// Check sigops
			uint32_t sigOps = CBTransactionGetSigOps(tx);
			if (sigOps > CB_MAX_SIG_OPS){
				CBMutexUnlock(node->blockAndTxMutex);
				CBLogWarning("Transaction %s has an invalid sig-op count", txStr);
				return CB_MESSAGE_ACTION_DISCONNECT;
			}
			// Look for unspent outputs being spent
			uint64_t inputValue = 0;
			CBOrphan * orphan = NULL;
			CBInventory * orphanGetData = NULL;
			bool wouldBeOrphan = false; // True whether or not orphan is actually stored.
			CBUnconfTransaction utx = {tx,0};
			// Data for holding type of dependency
			struct{
				enum{
					CB_DEP_FOUND,
					CB_DEP_UNFOUND,
					CB_DEP_CHAIN
				} type;
				void * data;
			}depData[tx->inputNum];
			// Macros for freeing orphan data
			#define CBFreeOrphanData \
				if (orphanGetData) \
					CBReleaseObject(orphanGetData); \
				if (orphan) \
					free(orphan); \
				CBMutexUnlock(node->blockAndTxMutex);
			#define CBTxReturnDisconnect CBFreeOrphanData return CB_MESSAGE_ACTION_DISCONNECT;
			#define CBTxReturnContinue CBFreeOrphanData return CB_MESSAGE_ACTION_CONTINUE;
			#define CBTxReturnError(x) CBFreeOrphanData return CBNodeReturnError(node, x);
			for (uint32_t x = 0; x < tx->inputNum; x++) {
				uint8_t * prevHash = CBByteArrayGetData(tx->inputs[x]->prevOut.hash);
				// First look for unconfirmed unspent outputs
				CBFoundTransaction * prevTx = CBNodeFullGetFoundTransaction(self, prevHash);
				CBTransactionOutput * output;
				if (prevTx != NULL) {
					// Found transaction as unconfirmed, check for output
					if (prevTx->utx.tx->outputNum < tx->inputs[x]->prevOut.index + 1){
						// Not enough outputs
						CBLogWarning("Transaction %s, input %u references an output that does not exist.", txStr, x);
						CBTxReturnDisconnect
					}
					output = prevTx->utx.tx->outputs[tx->inputs[x]->prevOut.index];
					// Ensure not spent, else ignore.
					if (output->spent){
						CBLogVerbose("Transaction %s, input %u references an output that is spent. Ignoring.", txStr, x);
						CBTxReturnContinue
					}
					// Increase the amount of unconfirmed dependencies this transaction has
					utx.numUnconfDeps++;
					// Dependency is found unconf
					depData[x].type = CB_DEP_FOUND;
					depData[x].data = prevTx;
				}else{
					// Else the dependency data is definitely the previous hash
					depData[x].data = prevHash;
					// Look in block-chain
					CBErrBool exists = CBBlockChainStorageUnspentOutputExists(validator, prevHash, tx->inputs[x]->prevOut.index);
					if (exists == CB_ERROR)
						CBTxReturnError("Could not determine if an unspent output exists.")
					if (exists == CB_FALSE){
						// No unspent output, request the dependency in get data. This is an orphan transaction.
						// UNLESS the transaction was there and the output index was bad. Check for the transaction
						exists = CBBlockChainStorageTransactionExists(validator, prevHash);
						if (exists == CB_ERROR)
							CBTxReturnError("Could not determine if a transaction exists.")
						if (exists == CB_TRUE)
							CBLogWarning("Transaction %s, input %u references an output that does not exist in the chain.", txStr, x);
							CBTxReturnDisconnect
						wouldBeOrphan = true;
						if (orphanGetData == NULL || orphanGetData->itemNum < 50000) {
							// Do not request transaction if we already expect it from this peer
							CBFindResult res = CBAssociativeArrayFind(&peer->expectedTxs, prevHash);
							if (!res.found) {
								char depTxStr[40];
								CBByteArrayToString(tx->inputs[x]->prevOut.hash, 0, 20, depTxStr);
								CBLogVerbose("Requiring dependency transaction %s from %s", depTxStr, peer->peerStr);
								// Create getdata if not already
								if (orphanGetData == NULL)
									orphanGetData = CBNewInventory();
								CBRetainObject(tx->inputs[x]->prevOut.hash);
								CBInventoryTakeInventoryItem(orphanGetData, CBNewInventoryItem(CB_INVENTORY_ITEM_TX, tx->inputs[x]->prevOut.hash));
								// Add to expected transactions from this peer.
								uint8_t * insertHash = malloc(20);
								memcpy(insertHash, prevHash, 20);
								CBAssociativeArrayInsert(&peer->expectedTxs, insertHash, res.position, NULL);
							}
						}
						// Dependency is unfound
						depData[x].type = CB_DEP_UNFOUND;
						continue;
					}
					// Load the unspent output
					CBErrBool ok = CBValidatorLoadUnspentOutputAndCheckMaturity(validator, tx->inputs[x]->prevOut, lastHeight, &output);
					if (ok == CB_ERROR)
						CBTxReturnError("There was an error getting an unspent output from storage.")
					if (ok == CB_FALSE){
						// Not mature so ignore
						CBLogVerbose("Transaction %s is not mature.", txStr);
						CBTxReturnContinue
					}
					// Dependency is on chain
					depData[x].type = CB_DEP_CHAIN;
				}
				CBInputCheckResult inRes = CBValidatorVerifyScripts(validator, tx, x, output, &sigOps, CBGetNode(self)->flags & CB_NODE_CHECK_STANDARD);
				if (inRes == CB_INPUT_BAD) {
					if (!res.found) CBReleaseObject(output);
					CBLogWarning("Transaction %s, input %u is invalid.", txStr, x);
					CBTxReturnDisconnect
				}
				if (inRes == CB_INPUT_NON_STD) {
					if (!res.found) CBReleaseObject(output);
					CBLogVerbose("Transaction %s, input %u is non-standard. Ignoring transaction.", txStr, x);
					CBTxReturnContinue
				}
				// Add to input value
				inputValue += output->value;
				// If we looked in block chain release the output
				if (!res.found) CBReleaseObject(output);
			}
			// Check that the input value is equal or more than the output value
			if (inputValue < outputValue){
				CBLogWarning("Transaction %s has an invalid output value.", txStr);
				CBTxReturnDisconnect
			}
			CBUnconfTransaction * uTx; // Will be set for insertion to unconf, unfound and chain dependencies.
			if (wouldBeOrphan) {
				// This transaction depends upon outputs that do not exist yet, add to orphans and send getdata for dependencies.
				if (orphanGetData) {
					CBGetMessage(orphanGetData)->type = CB_MESSAGE_TYPE_GETDATA;
					CBNodeSendMessageOnNetworkThread(CBGetNetworkCommunicator(self), peer, CBGetMessage(orphanGetData), NULL);
					CBReleaseObject(orphanGetData);
				}
				bool room = true;
				// 300000 equals 10 minutes times 500 bytes a transaction
				bool canRemove = (CBGetMilliseconds() - self->deleteOrphanTime) > 300000 / self->otherTxsSizeLimit + 1;
				while (self->otherTxsSize + CBGetMessage(tx)->bytes->length > self->otherTxsSizeLimit) {
					if (!canRemove || !CBNodeFullRemoveOrphan(self)) {
						room = false;
						break;
					}
				}
				if (room){
					// Allocate memory for orphan data
					orphan = malloc(sizeof(*orphan));
					self->otherTxsSize += CBGetMessage(tx)->bytes->length;
					// Finalise orphan data
					orphan->inputValue = inputValue;
					orphan->sigOps = sigOps;
					orphan->utx = utx;
					// The number of missing inputs is equal to the dependencies we want with the getdata
					orphan->missingInputNum = orphanGetData->itemNum;
					// Add orphan to array
					CBAssociativeArrayInsert(&self->orphanTxs, orphan, CBAssociativeArrayFind(&self->orphanTxs, orphan).position, NULL);
					uTx = (CBUnconfTransaction *)orphan;
					CBLogVerbose("Added transaction %s as orphan", txStr);
				}else
					break;
			}else{
				// Add this transaction as found (complete)
				CBProcessQueueItem * queue = &(CBProcessQueueItem){
					.next = NULL
				};
				CBErrBool res = CBNodeFullAddTransactionAsFound(self, utx, (CBFoundTransaction **)&queue->item, inputValue);
				uTx = (CBUnconfTransaction *)queue->item;
				if (res == CB_ERROR)
					return CBNodeReturnError(node, "Could not add a transaction as found.");
				if (res == CB_TRUE) {
					CBProcessQueueItem * last = queue;
					// Now check to see if any orphans depend upon this transaction. When an orphan is found (complete) then we add the orphan to the queue of complete transactions whose dependants need to be processed. After processing the found transaction, then proceed to process the next transaction in the queue.
					while (queue != NULL) {
						// See if this transaction has orphan dependants.
						CBFoundTransaction * fndTx = queue->item;
						CBFindResult res = CBAssociativeArrayFind(&self->unfoundDependencies, CBTransactionGetHash(fndTx->utx.tx));
						if (res.found) {
							// Have dependants, loop through orphan dependants, process the inputs and check for newly completed transactions.
							// Get orphan dependants
							CBAssociativeArray * orphanDependants = &((CBTransactionDependency *)CBFindResultToPointer(res))->dependants;
							// Remove this dependency from unfoundDependencies, but do not free yet
							CBAssociativeArrayDelete(&self->unfoundDependencies, res.position, false);
							CBOrphanDependency * delOrphanDep = NULL; // Set when orphan is removed so that we can ignore fututure dependencies
							CBOrphanDependency * lastOrphanDep = NULL; // Used to detect change in orphan so that we can finally move the orphan to the foundDep array.
							CBAssociativeArrayForEach(CBOrphanDependency * orphanDep, orphanDependants) {
								if (delOrphanDep && delOrphanDep->orphan == orphanDep->orphan)
									continue;
								if (lastOrphanDep && lastOrphanDep->orphan != orphanDep->orphan)
									// The next orphan, so add the last one to the foundDep array
									CBAssociativeArrayInsert(&fndTx->dependants, lastOrphanDep->orphan, CBAssociativeArrayFind(&fndTx->dependants, lastOrphanDep->orphan).position, NULL);
								// Check orphan input
								CBTransactionOutput * output = tx->outputs[orphanDep->orphan->utx.tx->inputs[orphanDep->inputIndex]->prevOut.index];
								CBInputCheckResult inRes = CBValidatorVerifyScripts(validator, orphanDep->orphan->utx.tx, orphanDep->inputIndex, output, &orphanDep->orphan->sigOps, node->flags & CB_NODE_CHECK_STANDARD);
								bool rm = false;
								if (inRes == CB_INPUT_OK) {
									// Increase the input value
									orphanDep->orphan->inputValue += output->value;
									// Increase the number of unconfirmed dependencies.
									orphanDep->orphan->utx.numUnconfDeps++;
									// Reduce the number of missing inputs and if there are no missing inputs left do final processing on orphan
									if (--orphanDep->orphan->missingInputNum == 0) {
										rm = true; // No longer an orphan
										// Calculate output value
										outputValue = 0;
										for (uint32_t x = 0; x < orphanDep->orphan->utx.tx->outputNum; x++)
											// Overflows have already been checked.
											outputValue += orphanDep->orphan->utx.tx->outputs[x]->value;
										// Ensure output value is eual to or less than input value
										if (outputValue <= orphanDep->orphan->inputValue){
											// The orphan is OK, so add it to ours or other and the queue.
											CBFoundTransaction * fndTx;
											CBErrBool res = CBNodeFullAddTransactionAsFound(self, orphanDep->orphan->utx, &fndTx, orphanDep->orphan->inputValue);
											if (res == CB_ERROR) {
												// Free data
												while (queue != NULL) {
													CBProcessQueueItem * next = queue->next;
													if (fndTx->utx.tx != tx)
														// Only free if not first.
														free(queue);
													queue = next;
												}
												// Return with error
												return CBNodeReturnError(node, "There was an error adding a previously orphan transaction as found.");
											}
											if (res == CB_TRUE) {
												// Successfully added transaction as found, now add to queue
												last->next = malloc(sizeof(*last->next));
												last = last->next;
												last->item = fndTx;
												last->next = NULL;
											}
										}else
											// Else orphan is not OK. We will need to remove the orphan from dependencies.
											CBNodeFullDeleteOrphanFromDependencies(self, orphanDep->orphan);
									}
								}else{
									// We are to remove this orphan because of an invalid or non-standard input
									rm = true;
									// Since this orphan failed before satisfying all inputs, check for other input dependencies (of unprocessed inputs ie. for unfound transactions) and remove this orphan as a dependant. We have already removed this transaction's unforund dependencies from the array, so it will not interfere with that.
									CBNodeFullDeleteOrphanFromDependencies(self, orphanDep->orphan);
								}
								if (rm) {
									// Remove orphan
									CBAssociativeArrayDelete(&self->orphanTxs, CBAssociativeArrayFind(&self->orphanTxs, orphanDep->orphan).position, true);
									// Ignore other dependencies for this orphan
									delOrphanDep = orphanDep;
									continue;
								}
							}
							if (lastOrphanDep)
								// Add the last orphan to the dependants
								CBAssociativeArrayInsert(&fndTx->dependants, lastOrphanDep->orphan->utx.tx, CBAssociativeArrayFind(&fndTx->dependants, lastOrphanDep->orphan).position, NULL);
						}
						// Completed processing for this found transaction.
						// Free the transaction dependency
						CBFreeTransactionDependency(CBFindResultToPointer(res));
						// Move to next in the queue
						CBProcessQueueItem * next = queue->next;
						// Free this queue item unless first
						if (fndTx->utx.tx != tx)
							free(queue);
						queue = next;
					}
					// Stage the changes
					if (!CBStorageDatabaseStage(node->database))
						return CBNodeReturnError(node, "Unable to stage changes of a new unconfirmed transaction to the database.");
				}else
					// Could not add transaction
					breakMutex;
			}
			// Add as dependant
			for (uint32_t x = 0; x < tx->inputNum; x++) {
				if (depData[x].type == CB_DEP_CHAIN) {
					// Add this transaction as a dependant of the previous chain transaction, if it is not been done so already
					CBNodeFullAddToDependency(&self->chainDependencies, depData[x].data, uTx, CBTransactionPtrCompare);
				}else if (depData[x].type == CB_DEP_FOUND){
					// Add this transaction as a dependant of the previous found unconfirmed transaction, if it is not been done so already
					CBFoundTransaction * prevTx = depData[x].data;
					CBFindResult res = CBAssociativeArrayFind(&prevTx->dependants, uTx);
					if (!res.found)
						// Not found, therefore add this as a dependant.
						CBAssociativeArrayInsert(&prevTx->dependants, uTx, res.position, NULL);
				}else{
					// Add this orphan to the dependencies of an unfound transaction.
					// See if the dependency exists
					CBFindResult res = CBAssociativeArrayFind(&self->unfoundDependencies, depData[x].data);
					CBTransactionDependency * txDep;
					if (!res.found) {
						// Create the transaction dependency
						txDep = malloc(sizeof(*txDep));
						memcpy(txDep->hash, depData[x].data, 32);
						CBInitAssociativeArray(&txDep->dependants, CBOrphanDependencyCompare, NULL, free);
						// Insert transaction dependency
						CBAssociativeArrayInsert(&self->unfoundDependencies, txDep, res.position, NULL);
					}else
						txDep = CBFindResultToPointer(res);
					// Add orphan dependency
					CBOrphanDependency * orphDep = malloc(sizeof(*orphDep));
					orphDep->inputIndex = x;
					orphDep->orphan = orphan;
					// Insert orphan dependency
					CBAssociativeArrayInsert(&txDep->dependants, orphDep, CBAssociativeArrayFind(&txDep->dependants, orphDep).position, NULL);
				}
			}
			breakMutex;
		}
		case CB_MESSAGE_TYPE_NOT_GIVEN_INV:
			// Timer expired but not given block inventory
			CBNodeFullPeerDownloadEnd(peer->nodeObj, peer);
			peer->upToDate = true;
			break;
		default:
			break;
	}
	return CB_MESSAGE_ACTION_CONTINUE;
}
bool CBNodeFullRemoveBlock(void * vpeer, uint8_t branch, CBBlock * block){
	CBNodeFull * self = CBGetPeer(vpeer)->nodeObj;
	// Loop through and process transactions
	for (uint32_t x = 1; x < block->transactionNum; x++) {
		CBTransaction * tx = block->transactions[x];
		// See if this transaction is ours or not.
		CBErrBool ours = CBAccounterIsOurs(CBGetNode(self)->accounterStorage, CBTransactionGetHash(tx));
		if (ours == CB_ERROR) {
			CBLogError("Could not determine if a transaction is ours.");
			return false;
		}
		if (ours == CB_TRUE) {
			if (! self->initialisedOurLostTxs) {
				// Initialise the lost transactions array
				CBInitAssociativeArray(&self->ourLostTxs, CBPtrCompare, NULL, CBReleaseObject);
				// Initialise the array for the dependencies of the lost transactions.
				CBInitAssociativeArray(&self->ourLostTxDependencies, CBHashCompare, NULL, CBFreeTransactionDependency);
				self->initialisedOurLostTxs = true;
			}
			// Add this transaction to the lostTxs array
			CBAssociativeArrayInsert(&self->ourLostTxs, tx, CBAssociativeArrayFind(&self->ourLostTxs, tx).position, NULL);
			CBRetainObject(tx);
			// Add dependencies
			for (uint32_t y = 0; y < tx->inputNum; y++)
				CBNodeFullAddToDependency(&self->ourLostTxDependencies, CBByteArrayGetData(tx->inputs[y]->prevOut.hash), tx, CBPtrCompare);
		}else{
			// If this transaction was a chain dependency of an unconfirmed transaction and was not our transaction then we should lose that transaction.
			CBNodeFullLoseUnconfDependants(self, CBTransactionGetHash(tx));
			// If this transaction is a dependency of one or more of our lost transactions, and it is not one of our transactions then we should remove our lost transactions depending on this and then any further dependencies of our transaction.
			if (self->initialisedOurLostTxs)
				CBNodeFullLoseOurDependants(self, CBTransactionGetHash(tx));
		}
	}
	return true;
}
void CBNodeFullRemoveFromDependency(CBAssociativeArray * deps, uint8_t * hash, void * el){
	CBFindResult res = CBAssociativeArrayFind(deps, hash);
	if (!res.found)
		return;
	CBTransactionDependency * dep = CBFindResultToPointer(res);
	CBFindResult res2 = CBAssociativeArrayFind(&dep->dependants, el);
	if (!res2.found)
		return;
	// Remove element from dependants.
	CBAssociativeArrayDelete(&dep->dependants, res2.position, true);
	// See if the array is empty. If it is then we can remove this dependant
	if (CBAssociativeArrayIsEmpty(&dep->dependants))
		CBAssociativeArrayDelete(deps, res.position, true);
}
bool CBNodeFullRemoveLostUnconfTxs(CBNodeFull * self){
	if (self->initialisedLostUnconfTxs) {
		CBAssociativeArrayForEach(CBUnconfTransaction * tx, &self->lostUnconfTxs){
			// If ours call doubleSpend and remove from storage
			if (tx->type == CB_TX_OURS){
				CBGetNode(self)->callbacks.doubleSpend(CBGetNode(self), CBTransactionGetHash(tx->tx));
				if (!CBNodeStorageRemoveOurTx(CBGetNode(self)->nodeStorage, tx->tx)) {
					CBLogError("Could not remove our lost transaction.");
					return false;
				}
			}else if (tx->type == CB_TX_OTHER && !CBNodeStorageRemoveOtherTx(CBGetNode(self)->nodeStorage, tx->tx)) {
				CBLogError("Could not remove other lost transaction.");
				return false;
			}
			if (!CBNodeFullRemoveUnconfTx(self, tx)) {
				CBLogError("Could not process an unconfirmed transaction being lost.");
				return false;
			}
		}
		// Free lostUnconfTxs
		CBFreeAssociativeArray(&self->lostUnconfTxs);
		self->initialisedLostUnconfTxs = false;
	}
	return true;
}
bool CBNodeFullRemoveOrphan(CBNodeFull * self){
	CBPosition pos;
	if (!CBAssociativeArrayGetFirst(&self->orphanTxs, &pos))
		// No orphans to remove.
		return false;
	CBOrphan * orphan = pos.node->elements[pos.index];
	self->otherTxsSize -= CBGetMessage(orphan->utx.tx)->bytes->length;
	// Delete this orphan as a dependant
	CBNodeFullDeleteOrphanFromDependencies(self, orphan);
	// Now remove from orphan array
	CBAssociativeArrayDelete(&self->orphanTxs, pos, true);
	return true;
}
bool CBNodeFullRemoveUnconfTx(CBNodeFull * self, CBUnconfTransaction * txRm){
	CBProcessQueueItem * queue = &(CBProcessQueueItem){txRm, NULL};
	CBProcessQueueItem * last = queue;
	while (queue != NULL) {
		CBUnconfTransaction * uTx = last->item;
		CBRetainObject(uTx->tx);
		if (uTx->type == CB_TX_ORPHAN)
			CBAssociativeArrayDelete(&self->orphanTxs, CBAssociativeArrayFind(&self->orphanTxs, uTx->tx).position, true);
		else{ // CB_TX_OURS or CB_TX_OTHER
			CBFoundTransaction * fndTx = (CBFoundTransaction *)uTx;
			// Add dependants to queue
			CBAssociativeArrayForEach(CBUnconfTransaction * dependant, &fndTx->dependants) {
				last->next = malloc(sizeof(*last->next));
				last = last->next;
				last->item = dependant;
				last->next = NULL;
			}
			// Remove from ours or other array
			CBAssociativeArray * arr = (uTx->type == CB_TX_OURS) ? &self->ourTxs : &self->otherTxs;
			CBAssociativeArrayDelete(arr, CBAssociativeArrayFind(arr, fndTx).position, true);
			// Also delete from allChainFoundTxs if in there
			CBFindResult res = CBAssociativeArrayFind(&self->allChainFoundTxs, fndTx);
			if (res.found)
				CBAssociativeArrayDelete(&self->allChainFoundTxs, res.position, true);
		}
		// Remove from dependencies
		for (uint32_t x = 0; x < uTx->tx->inputNum; x++){
			uint8_t * prevHash = CBByteArrayGetData(uTx->tx->inputs[x]->prevOut.hash);
			// See if the previous hash is of a found transaction
			CBFoundTransaction * fndTx = CBNodeFullGetFoundTransaction(self, prevHash);
			if (fndTx)
				CBAssociativeArrayDelete(&fndTx->dependants, CBAssociativeArrayFind(&fndTx->dependants, uTx).position, true);
			else
				// Remove from chain dependency if it exists in one.
				CBNodeFullRemoveFromDependency(&self->chainDependencies, prevHash, uTx);
		}
		// Remove from accounter
		if (uTx->type == CB_TX_OURS && ! CBAccounterLostBranchlessTransaction(CBGetNode(self)->accounterStorage, uTx->tx)) {
			CBLogError("Could not lose unconfirmed transaction from the accounter.");
			return false;
		}
		// Move to next in the queue
		CBProcessQueueItem * next = queue->next;
		// Free this queue item unless first
		if (uTx != txRm)
			free(queue);
		queue = next;
	}
	return true;
}
bool CBNodeFullSendGetBlocks(CBNodeFull * self, CBPeer * peer){
	// Lock for access to block chain.
	CBMutexLock(CBGetNode(self)->blockAndTxMutex);
	CBChainDescriptor * chainDesc = CBValidatorGetChainDescriptor(CBGetNode(self)->validator);
	CBMutexUnlock(CBGetNode(self)->blockAndTxMutex);
	if (chainDesc == NULL){
		CBLogError("There was an error when trying to retrieve the block-chain descriptor.");
		return false;
	}
	char blockStr[41];
	if (chainDesc->hashNum == 1)
		strcpy(blockStr, "genesis");
	else
		CBByteArrayToString(chainDesc->hashes[0], 0, 20, blockStr);
	CBLogVerbose("Sending getblocks to %s with block %s as the latest.", peer->peerStr, blockStr);
	CBGetBlocks * getBlocks = CBNewGetBlocks(CB_MIN_PROTO_VERSION, chainDesc, NULL);
	CBReleaseObject(chainDesc);
	CBGetMessage(getBlocks)->type = CB_MESSAGE_TYPE_GETBLOCKS;
	CBNodeSendMessageOnNetworkThread(CBGetNetworkCommunicator(self), peer, CBGetMessage(getBlocks), NULL);
	CBReleaseObject(getBlocks);
	// Check in the future that the peer did send us an inv of blocks
	CBStartTimer(CBGetNetworkCommunicator(self)->eventLoop, &peer->invResponseTimer, CBGetNetworkCommunicator(self)->responseTimeOut, CBNodeFullHasNotGivenBlockInv, peer);
	peer->invResponseTimerStarted = true;
	return true;
}
bool CBNodeFullSendRequestedData(CBNodeFull * self, CBPeer * peer){
	// See if the request has been satisfied
	if (peer->sendDataIndex == peer->requestedData->itemNum) {
		// We have sent everything
		CBReleaseObject(peer->requestedData);
		peer->requestedData = NULL;
		return false;
	}
	// Get this item
	CBInventoryItem * item = CBInventoryGetInventoryItem(peer->requestedData, peer->sendDataIndex);
	// Lock for block or transaction access
	CBMutexLock(CBGetNode(self)->blockAndTxMutex);
	switch (item->type) {
		case CB_INVENTORY_ITEM_TX:{
			// Look at all unconfirmed transactions
			// Change the comparison function to accept a hash in place of a transaction object.
			CBFoundTransaction * fndTx = CBNodeFullGetFoundTransaction(self, CBByteArrayGetData(item->hash));
			if (fndTx != NULL){
				// The peer was requesting an unconfirmed transaction
				CBGetMessage(fndTx->utx.tx)->type = CB_MESSAGE_TYPE_TX;
				CBNodeSendMessageOnNetworkThread(CBGetNetworkCommunicator(self), peer, CBGetMessage(fndTx->utx.tx), CBNodeFullSendRequestedDataVoid);
			}
			// Else the peer was requesting something we do not have as unconfirmed so ignore.
			break;
		}
		case CB_INVENTORY_ITEM_BLOCK:{
			// Look for the block to give
			uint8_t branch;
			uint32_t index;
			CBErrBool exists = CBBlockChainStorageGetBlockLocation(CBGetNode(self)->validator, CBByteArrayGetData(item->hash), &branch, &index);
			if (exists == CB_ERROR) {
				CBLogError("Could not get the location for a block when a peer requested it of us.");
				CBGetNode(self)->callbacks.onFatalNodeError(CBGetNode(self));
				CBFreeNodeFull(self);
				return true;
			}
			if (exists == CB_FALSE) {
				// The peer is requesting a block we do not have. Obviously we may have removed the block since, but this is unlikely
				CBNetworkCommunicatorDisconnect(CBGetNetworkCommunicator(self), peer, CB_24_HOURS, false);
				return true;
			}
			// Load the block from storage
			CBBlock * block = CBBlockChainStorageLoadBlock(CBGetNode(self)->validator, index, branch);
			if (block == NULL) {
				CBLogError("Could not load a block from storage.");
				CBGetNode(self)->callbacks.onFatalNodeError(CBGetNode(self));
				CBFreeNodeFull(self);
				return true;
			}
			CBGetMessage(block)->type = CB_MESSAGE_TYPE_BLOCK;
			CBNodeSendMessageOnNetworkThread(CBGetNetworkCommunicator(self), peer, CBGetMessage(block), CBNodeFullSendRequestedDataVoid);
		}
		case CB_INVENTORY_ITEM_ERROR:
			return false;
	}
	CBMutexUnlock(CBGetNode(self)->blockAndTxMutex);
	// If the item index is 249, then release the inventory items, move the rest down and then reduce the item number
	if (peer->sendDataIndex == 249) {
		for (uint16_t x = 0; x < 249; x++)
			CBReleaseObject(peer->requestedData->items[0][x]);
		memmove(peer->requestedData->items, peer->requestedData->items + sizeof(*peer->requestedData->items), (peer->requestedData->itemNum / 250) * sizeof(*peer->requestedData->items));
		peer->requestedData->itemNum -= 250;
	}
	// Now move to the next piece of data
	peer->sendDataIndex++;
	return false;
}
void CBNodeFullSendRequestedDataVoid(void * self, void * peer){
	CBNodeFullSendRequestedData(self, peer);
}
bool CBNodeFullStartValidation(void * vpeer){
	CBMutexLock(CBGetNode(CBGetPeer(vpeer)->nodeObj)->blockAndTxMutex);
	CBGetNodeFull(CBGetPeer(vpeer)->nodeObj)->forkPoint = CB_NO_FORK;
	return true;
}
bool CBNodeFullUnconfToChain(CBNodeFull * self, CBUnconfTransaction * uTx, uint32_t blockHeight, uint8_t branch, bool new){
	// ??? Remove redundant code with CBNodeFullRemoveUnconfTx?
	CBTransaction * tx;
	tx = uTx->tx;
	if (uTx->type == CB_TX_ORPHAN) {
		CBOrphan * orphan = (CBOrphan *)uTx;
		CBRetainObject(tx);
		CBAssociativeArrayDelete(&self->orphanTxs, CBAssociativeArrayFind(&self->orphanTxs, orphan).position, true);
	}else{ // CB_TX_OURS or CB_TX_OTHER
		CBFoundTransaction * fndTx = (CBFoundTransaction *)uTx;
		// Change to chain dependency
		CBTransactionDependency * dep = malloc(sizeof(*dep));
		memcpy(dep->hash, CBTransactionGetHash(tx), 32);
		dep->dependants = fndTx->dependants;
		CBAssociativeArrayInsert(&self->chainDependencies, dep, CBAssociativeArrayFind(&self->chainDependencies, dep).position, NULL);
		// Remove from ours or other array
		CBAssociativeArray * arr = (uTx->type == CB_TX_OURS) ? &self->ourTxs : &self->otherTxs;
		CBAssociativeArrayDelete(arr, CBAssociativeArrayFind(arr, fndTx).position, false);
		// Also delete from allChainFoundTxs if in there
		CBFindResult res = CBAssociativeArrayFind(&self->allChainFoundTxs, fndTx);
		if (res.found)
			CBAssociativeArrayDelete(&self->allChainFoundTxs, res.position, true);
		// Free CBFoundTransaction data without freeing the dependants data
		free(fndTx);
	}
	// Remove from dependencies
	for (uint32_t x = 0; x < tx->inputNum; x++){
		uint8_t * prevHash = CBByteArrayGetData(tx->inputs[x]->prevOut.hash);
		// See if the previous hash is of a found transaction
		CBFoundTransaction * fndTx = CBNodeFullGetFoundTransaction(self, prevHash);
		if (fndTx)
			CBAssociativeArrayDelete(&fndTx->dependants, CBAssociativeArrayFind(&fndTx->dependants, uTx).position, true);
		else
			// Remove from chain dependency if it exists in one.
			CBNodeFullRemoveFromDependency(&self->chainDependencies, prevHash, uTx);
	}
	// Move to chain on accounter
	if (uTx->type == CB_TX_OURS && new && ! CBAccounterBranchlessTransactionToBranch(CBGetNode(self)->accounterStorage, tx, blockHeight, branch)) {
		CBLogError("Could not make unconf transaction a confirmed one on the accounter.");
		return false;
	}
	// Call transactionConfirmed
	CBGetNode(self)->callbacks.transactionConfirmed(CBGetNode(self), CBTransactionGetHash(tx), blockHeight);
	return true;
}
bool CBNodeFullValidatorFinish(void * vpeer, CBBlock * block){
	CBPeer * peer = vpeer;
	CBNodeFull * self = peer->nodeObj;
	// Stage changes
	if (!CBStorageDatabaseStage(CBGetNode(self)->database)) {
		CBLogError("Could not stage changes to the block-chain and accounts.");
		return false;
	}
	// Finished with the block chain
	CBMutexUnlock(CBGetNode(self)->blockAndTxMutex);
	// Return if adding direct
	if (self->addingDirect)
		return true;
	// Get the next required block
	CBMutexLock(self->pendingBlockDataMutex);
	// Get the next block or ask for new blocks only if not up to date
	if (peer->upToDate){
		peer->expectBlock = false;
		peer->downloading = false;
		return true;
	}
	while ((peer->reqBlockCursor = peer->reqBlocks[peer->reqBlockCursor].next) != CB_END_REQ_BLOCKS) {
		// We can get this next block if and only if it has not been asked for and the block peer information still exists, otherwise it is being got by another peer or it has been processed already.
		uint8_t * hash = peer->reqBlocks[peer->reqBlockCursor].reqBlock;
		CBFindResult askedForRes = CBAssociativeArrayFind(&self->askedForBlocks, hash);
		if (!askedForRes.found) {
			// Has not yet been asked for by another node. Do we have the block peer information?
			CBFindResult res = CBAssociativeArrayFind(&self->blockPeers, hash);
			if (res.found) {
				// Yes we have found it, this means the block is waiting to be received and processed.
				CBNodeFullAskForBlock(self, peer, hash);
				// Add to askedFor
				CBAssociativeArrayInsert(&self->askedForBlocks, CBFindResultToPointer(res), askedForRes.position, NULL);
				CBMutexUnlock(self->pendingBlockDataMutex);
				return true;
			}
		}
	}
	CBMutexUnlock(self->pendingBlockDataMutex);
	// The node is allowed to give a new branch with a new inventory
	peer->allowNewBranch = true;
	// No next required blocks for this node, now allowing for inv response.
	peer->expectBlock = false;
	// Ask for new block inventory.
	if (!CBNodeFullSendGetBlocks(self, peer))
		return CBNodeReturnError(CBGetNode(self), "Could not send getblocks for getting the next blocks whilst downloading from a peer.");
	return true;
}
CBCompare CBOrphanDependencyCompare(CBAssociativeArray * foo, void * vorphDep1, void * vorphDep2){
	CBOrphanDependency * orphDep1 = vorphDep1;
	CBOrphanDependency * orphDep2 = vorphDep2;
	if (orphDep1->orphan > orphDep2->orphan)
		return CB_COMPARE_MORE_THAN;
	if (orphDep1->orphan < orphDep2->orphan)
		return CB_COMPARE_LESS_THAN;
	if (orphDep1->inputIndex > orphDep2->inputIndex)
		return CB_COMPARE_MORE_THAN;
	if (orphDep1->inputIndex < orphDep2->inputIndex)
		return CB_COMPARE_LESS_THAN;
	return CB_COMPARE_EQUAL;
}
