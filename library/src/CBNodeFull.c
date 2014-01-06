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
	self->initialisedFoundUnconfTxs = false;
	self->initialisedUnconfirmedTxs = false;
	self->initialisedLostUnconfTxs = false;
	self->initialisedLostChainTxs = false;
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
			CBInitAssociativeArray(&self->blockPeers, CBHashCompare, NULL, CBFreeBlockPeers);
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

void CBFreeBlockPeers(void * vblkPeers){
	CBBlockPeers * blkPeers = vblkPeers;
	CBFreeAssociativeArray(&blkPeers->peers);
	free(blkPeers);
}
void CBFreeChainTransaction(void * vtx){
	CBChainTransaction * tx = vtx;
	CBReleaseObject(tx->tx);
	free(tx);
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
bool CBNodeFullAddBlock(void * vpeer, uint8_t branch, CBBlock * block, uint32_t blockHeight, CBAddBlockType addType) {
	CBNodeFull * self = CBGetPeer(vpeer)->nodeObj;
	// If last block call newBlock
	if (addType == CB_ADD_BLOCK_LAST) {
		// Update block height
		CBGetNetworkCommunicator(self)->blockHeight = blockHeight;
		// Call newBlock
		CBGetNode(self)->callbacks.newBlock(CBGetNode(self), block, self->forkPoint);
	}
	// Loop through and process transactions.
	for (uint32_t x = 0; x < block->transactionNum; x++) {
		CBTransaction * tx = block->transactions[x];
		// If the transaction is ours, check for lost transactions from the other chain.
		if (self->initialisedUnconfirmedTxs) {
			// Take out this transaction from the lostTxs array if it exists in there
			CBFindResult res = CBAssociativeArrayFind(&self->unconfirmedTxs, &tx);
			if (res.found){
				// Remove dependencies and then the unconfirmed tx, as it it now on the chain again
				for (uint32_t y = 0; y < tx->inputNum; y++)
					CBNodeFullRemoveFromDependency(&self->unconfirmedTxDependencies, CBByteArrayGetData(tx->inputs[y]->prevOut.hash), &tx);
				// We refound a transaction we lost from the other chain. If this is a new block, add the transaction to the accounter, if it is ours.
				CBChainTransaction * chainTx = CBFindResultToPointer(res);
				if (chainTx->ours && chainTx->height != blockHeight
					&& ! CBAccounterTransactionChangeHeight(CBGetNode(self)->accounterStorage, tx, chainTx->height, blockHeight)) {
					CBLogError("Could not process a transaction with the accounter that was a lost transactions of ours.");
					return false;
				}
				// Remove from unconfirmedTxs
				CBAssociativeArrayDelete(&self->unconfirmedTxs, res.position, true);
				// We can end here for a transaction is moving from one chain to another and nothing else.
				continue;
			}
		}
		if (x != 0) {
			// We will record unconfirmed transactions found on the chain, but only remove when we finish.
			// This is important so that if there is a failed reorganisation the unconfirmed transactions will remain in place.
			CBUnconfTransaction * uTx = CBNodeFullGetAnyTransaction(self, CBTransactionGetHash(tx));
			if (uTx != NULL) {
				if (addType == CB_ADD_BLOCK_LAST){
					// As this is the last block, we know it is certain. Therefore process moving from unconfirmed to chain.
					if (! CBNodeFullUnconfToChain(self, uTx, blockHeight, block->time))
						return false;
				}else{
					// Else we will need to record this for processing later as this block is tentative
					CBTransactionToBranch * txToBranch = malloc(sizeof(*txToBranch));
					txToBranch->uTx = uTx;
					txToBranch->blockHeight = blockHeight;
					txToBranch->blockTime = block->time;
					txToBranch->branch = branch;
					// Record transaction to foundUnconfTxs
					if (! self->initialisedFoundUnconfTxs){
						self->initialisedFoundUnconfTxs = true;
						CBInitAssociativeArray(&self->foundUnconfTxs, CBPtrCompare, NULL, free);
					}
					CBAssociativeArrayInsert(&self->foundUnconfTxs, txToBranch, CBAssociativeArrayFind(&self->foundUnconfTxs, txToBranch).position, NULL);
				}
			}else{
				// This transaction is not one of the unconfirmed ones we have
				// If this transaction spends a chain dependency of an unconfirmed transaction or one of the lost txs, then we should lose that transaction as a double spend, as well as any further dependants, including unconfirmed transactions (unless it was ours).
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
									// The prevouts are the same! Add this to the lostTxs if the block is tentative, otherwise we can remove the unconfirmed transaction straight away.
									if (! CBNodeFullLostUnconfTransaction(self, dependant, addType == CB_ADD_BLOCK_LAST))
										return false;
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
					// Now for our unconfirmed lost txs. It can't double spend any other transaction.
					if (self->initialisedUnconfirmedTxs) {
						CBFindResult res = CBAssociativeArrayFind(&self->unconfirmedTxDependencies, prevHash);
						if (res.found) {
							// We need to loop through the dependants and see if they also spend from the same output
							CBTransactionDependency * dep = CBFindResultToPointer(res);
							// The dependants of ourLostTxDependencies can only be CBTransaction
							CBAssociativeArrayForEach(CBChainTransaction * dependant, &dep->dependants) {
								bool found = false;
								for (uint32_t z = 0; z < dependant->tx->inputNum; z++) {
									if (!memcmp(prevHash, CBByteArrayGetData(dependant->tx->inputs[z]->prevOut.hash), 32)
										&& tx->inputs[y]->prevOut.index == dependant->tx->inputs[z]->prevOut.index) {
										// Yes this transaction that we lost cannot be made unconfirmed due to double spend.
										// Remove it and any further dependant transactions of ours.
										if (!CBNodeFullLoseChainTxDependants(self, CBTransactionGetHash(dependant->tx), addType == CB_ADD_BLOCK_LAST))
											return false;
										if (! CBNodeFullMakeLostChainTransaction(self, dependant, addType == CB_ADD_BLOCK_LAST))
											return false;
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
				// Found a transaction for the accounter.
				if (CBNodeFullFoundTransaction(self, tx, block->time, blockHeight, addType == CB_ADD_BLOCK_LAST, true) == CB_ERROR) {
					CBLogError("Could not process a found transaction found in a block.");
					return false;
				}
			}
		}else if (addType != CB_ADD_BLOCK_PREV) {
			// Coinbase not previously validated, so add to accounter
			if (CBNodeFullFoundTransaction(self, tx, block->time, blockHeight, addType == CB_ADD_BLOCK_LAST, true) == CB_ERROR) {
				CBLogError("Could not process a found transaction found in a block.");
				return false;
			}
		}
	}
	if (addType == CB_ADD_BLOCK_LAST) {
		// We can finalise processing for the blocks
		// Process the removed unconf transactions
		if (!CBNodeFullRemoveLostUnconfTxs(self)) {
			CBLogError("Could not remove lost unconfirmed transactions from double spends in the chain.");
			return false;
		}
		// Process the removed chain transactions
		if (self->initialisedLostChainTxs) {
			CBAssociativeArrayForEach(CBChainTransaction * tx, &self->lostChainTxs)
				CBGetNode(self)->callbacks.doubleSpend(CBGetNode(self), CBTransactionGetHash(tx->tx));
			// Free lostChainTxs
			CBFreeAssociativeArray(&self->lostChainTxs);
			self->initialisedLostChainTxs = false;
		}
		if (self->initialisedUnconfirmedTxs) {
			// Add lost transactions back to unconfirmed
			CBAssociativeArrayForEach(CBChainTransaction * tx, &self->unconfirmedTxs) {
				// Make found transaction
				CBFoundTransaction * fndTx = malloc(sizeof(*fndTx));
				fndTx->utx.tx = tx->tx;
				fndTx->utx.numUnconfDeps = 0;
				fndTx->utx.type = tx->ours ? CB_TX_OURS : CB_TX_OTHER;
				// Calculate input value.
				fndTx->inputValue = 0;
				for (uint32_t x = 0; x < tx->tx->inputNum; x++) {
					// We need to get the previous output value
					uint8_t * prevOutHash = CBByteArrayGetData(tx->tx->inputs[x]->prevOut.hash);
					uint32_t prevOutIndex = tx->tx->inputs[x]->prevOut.index;
					// See if we have this transaction as unconfirmed, ie. a dependency that was also made unconfirmed.
					CBFoundTransaction * prevFndTx = CBNodeFullGetFoundTransaction(self, prevOutHash);
					if (prevFndTx == NULL) {
						// Before we try the chain, take a look for the depedency in unconfirmedTxs, as a run of transaction may become unconfirmed
						self->unconfirmedTxs.compareFunc = CBTransactionPtrAndHashCompare;
						CBFindResult res = CBAssociativeArrayFind(&self->unconfirmedTxs, prevOutHash);
						self->unconfirmedTxs.compareFunc = CBTransactionPtrCompare;
						if (res.found) {
							// The data type used is CBTransactionOfType which contains a CBTransaction pointer as the first member.
							CBTransaction ** prevTx = CBFindResultToPointer(res);
							fndTx->inputValue += (*prevTx)->outputs[prevOutIndex]->value;
						}else{
							// Load from chain
							bool coinbase;
							uint32_t height;
							CBTransactionOutput * output = CBBlockChainStorageLoadUnspentOutput(CBGetNode(self)->validator, prevOutHash, prevOutIndex, &coinbase, &height);
							if (output == NULL) {
								CBLogError("Could not load an output for counting the input value for a transaction of ours becomind unconfirmed.");
								return false;
							}
							fndTx->inputValue += output->value;
							CBReleaseObject(output);
						}
					}else
						fndTx->inputValue += prevFndTx->utx.tx->outputs[prevOutIndex]->value;
				}
				fndTx->timeFound = CBGetMilliseconds();
				fndTx->nextRelay = fndTx->timeFound + 1800000;
				CBRetainObject(fndTx->utx.tx);
				CBAssociativeArray * txArray = tx->ours ? &self->ourTxs : &self->otherTxs;
				CBAssociativeArrayInsert(txArray, fndTx, CBAssociativeArrayFind(txArray, fndTx).position, NULL);
				// If this was a chain dependency, move dependency data to CBFoundTransaction, else init empty dependant's array
				CBFindResult res = CBAssociativeArrayFind(&self->chainDependencies, CBTransactionGetHash(fndTx->utx.tx));
				if (res.found) {
					CBTransactionDependency * dep = CBFindResultToPointer(res);
					fndTx->dependants = dep->dependants;
					CBAssociativeArrayDelete(&self->chainDependencies, res.position, false);
					free(dep);
					// Loop through dependants and add to the number of unconf dependencies.
					CBAssociativeArrayForEach(CBUnconfTransaction * dependant, &fndTx->dependants)
						if (dependant->numUnconfDeps++ == 0 && dependant->type != CB_TX_ORPHAN)
							// Came off allChainFoundTxs
							CBAssociativeArrayDelete(&self->allChainFoundTxs, CBAssociativeArrayFind(&self->allChainFoundTxs, dependant).position, false);
				}else
					CBInitAssociativeArray(&fndTx->dependants, CBTransactionPtrCompare, NULL, NULL);
				// Add our tx to the node storage.
				if (!(tx->ours ? CBNodeStorageAddOurTx : CBNodeStorageAddOtherTx)(CBGetNode(self)->nodeStorage, tx->tx)) {
					CBLogError("Could not add our transaction made unconfirmed.");
					return false;
				}
				if (tx->ours) {
					// Change height to CB_UNCONFIRMED
					if (! CBAccounterTransactionChangeHeight(CBGetNode(self)->accounterStorage, tx->tx, tx->height, CB_UNCONFIRMED)) {
						CBLogError("Could not process one of our lost transactions that should become unconfirmed.");
						return false;
					}
					// Call transactionUnconfirmed
					CBGetNode(self)->callbacks.transactionUnconfirmed(CBGetNode(self), CBTransactionGetHash(fndTx->utx.tx));
				}
			}
			// Now add dependencies of our transactions.
			CBAssociativeArrayForEach(CBTransactionDependency * dep, &self->unconfirmedTxDependencies) {
				// See if this dependency was one of our transactions we added back as unconfirmed
				CBFoundTransaction * fndTx = CBNodeFullGetFoundTransaction(self, dep->hash);
				if (fndTx) {
					// Add these dependants to the CBFoundTransaction of our transaction we added back as unconfirmed.
					// Loop though dependants
					CBAssociativeArrayForEach(CBChainTransaction * dependant, &dep->dependants) {
						// Add into the dependants of the CBFoundTransaction
						CBFoundTransaction * fndDep = CBNodeFullGetFoundTransaction(self, CBTransactionGetHash(dependant->tx));
						CBAssociativeArrayInsert(&fndTx->dependants, fndDep, CBAssociativeArrayFind(&fndTx->dependants, fndDep).position, NULL);
						// Add to the number of unconf dependencies
						fndDep->utx.numUnconfDeps++;
					}
				}else{
					// Add these dependants to the chain dependency
					// Loop though dependants
					CBAssociativeArrayForEach(CBChainTransaction * dependant, &dep->dependants) {
						// Add into the dependants of the CBTransactionDependency
						CBFoundTransaction * fndDep = CBNodeFullGetFoundTransaction(self, CBTransactionGetHash(dependant->tx));
						CBNodeFullAddToDependency(&self->chainDependencies, dep->hash, fndDep, CBTransactionPtrCompare);
					}
				}
			}
			// Loop through unconfirmedTxs again and find those with no unconf dependencies
			CBAssociativeArrayForEach(CBChainTransaction * tx, &self->unconfirmedTxs) {
				CBFoundTransaction * fndTx = CBNodeFullGetFoundTransaction(self, CBTransactionGetHash(tx->tx));
				if (fndTx->utx.numUnconfDeps == 0)
					// All chain deperencies so add to allChainFoundTxs
					CBAssociativeArrayInsert(&self->allChainFoundTxs, fndTx, CBAssociativeArrayFind(&self->allChainFoundTxs, fndTx).position, NULL);
			}
			// Free unconfirmedTxs and unconfirmedTxDependencies
			CBFreeAssociativeArray(&self->unconfirmedTxs);
			CBFreeAssociativeArray(&self->unconfirmedTxDependencies);
			self->initialisedUnconfirmedTxs = false;
		}
		// Next process found unconf transactions which are now confirmed.
		if (self->initialisedFoundUnconfTxs) {
			CBAssociativeArrayForEach(CBTransactionToBranch * txToBranch, &self->foundUnconfTxs)
				if (!CBNodeFullUnconfToChain(self, txToBranch->uTx, txToBranch->blockHeight, txToBranch->blockTime)) {
					CBLogError("Could not process an unconfirmed transaction moving onto the chain.");
					return false;
				}
			// Free array
			CBFreeAssociativeArray(&self->foundUnconfTxs);
			self->initialisedFoundUnconfTxs = false;
		}
		// Process new transactions
		while (self->newTransactions != NULL) {
			// Call newTransaction
			if (!CBNodeFullProcessNewTransaction(self, self->newTransactions->tx, self->newTransactions->time, self->newTransactions->blockHeight, self->newTransactions->accountDetailList, true, self->newTransactions->ours))
				return false;
			// Release transaction
			CBReleaseObject(self->newTransactions->tx);
			if (self->newTransactions->ours)
				CBFreeTransactionAccountDetailList(self->newTransactions->accountDetailList);
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
			if (invs[currentInv] != NULL) {
				if (currentInv == 3)
					currentInv = 0;
				else
					currentInv++;
			}
		}
		CBFreeAssociativeArray(&dependantsProccessed);
		// Loop through and send invs to peers
		currentInv = 0;
		if (invs[0] != NULL) {
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
		}
		// Remove old transactions
		if (!CBNodeFullRemoveLostUnconfTxs(self)) {
			CBLogError("Could not remove old unconfirmed transactions.");
			return false;
		}
		// If not not send in response to getblocks relay inv of this block to all peers, except the peer that gave us it.
		CBPeer * peerFrom = vpeer;
		if (!peerFrom->downloading) {
			CBInventory * inv = CBNewInventory();
			CBGetMessage(inv)->type = CB_MESSAGE_TYPE_INV;
			CBByteArray * hash = CBNewByteArrayWithDataCopy(CBBlockGetHash(block), 32);
			CBInventoryTakeInventoryItem(inv, CBNewInventoryItem(CB_INVENTORY_ITEM_BLOCK, hash));
			CBReleaseObject(hash);
			CBMutexLock(CBGetNetworkCommunicator(self)->peersMutex);
			CBAssociativeArrayForEach(CBPeer * peer, &CBGetNetworkCommunicator(self)->addresses->peers)
				if (peer != peerFrom && peer->handshakeStatus | CB_HANDSHAKE_GOT_VERSION)
					CBNodeSendMessageOnNetworkThread(CBGetNetworkCommunicator(self), peer, CBGetMessage(inv), NULL);
			CBMutexUnlock(CBGetNetworkCommunicator(self)->peersMutex);
			CBReleaseObject(inv);
		}
	}else if (self->forkPoint == CB_NO_FORK)
		// Set the fork point if not already
		self->forkPoint = blockHeight;
	return true;
}
bool CBNodeFullAddBlockDirectly(CBNodeFull * self, CBBlock * block){
	CBMutexLock(CBGetNode(self)->blockAndTxMutex);
	self->addingDirect = true;
	if (!CBValidatorAddBlockDirectly(CBGetNode(self)->validator, block, &(CBPeer){.nodeObj=self, .downloading=false})){
		CBLogError("Could not add a block directly to the validator.");
		return false;
	}
	CBMutexUnlock(CBGetNode(self)->blockAndTxMutex);
	self->addingDirect = false;
	return true;
}
CBErrBool CBNodeFullAddOurOrOtherFoundTransaction(CBNodeFull * self, bool ours, CBUnconfTransaction utx, CBFoundTransaction * fndTx, uint64_t txInputValue){
	char txStr[CB_TX_HASH_STR_SIZE];
	CBTransactionHashToString(utx.tx, txStr);
	// Create found transaction data
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
	// Retain transaction
	CBRetainObject(utx.tx);
	return CB_TRUE;
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
CBErrBool CBNodeFullAddTransactionAsFound(CBNodeFull * self, CBUnconfTransaction utx, CBFoundTransaction * fndTx, uint64_t txInputValue){
	// Everything is OK so check transaction through accounter
	uint64_t timestamp = time(NULL);
	CBErrBool ours = CBNodeFullFoundTransaction(self, utx.tx, timestamp, CB_UNCONFIRMED, true, false);
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
	memcpy(peer->expectedBlock, hash, 20);
	// Create getdata message
	CBInventory * getData = CBNewInventory();
	CBByteArray * hashObj = CBNewByteArrayWithDataCopy(hash, 32);
	CBInventoryTakeInventoryItem(getData, CBNewInventoryItem(CB_INVENTORY_ITEM_BLOCK, hashObj));
	CBReleaseObject(hashObj);
	CBGetMessage(getData)->type = CB_MESSAGE_TYPE_GETDATA;
	// Send getdata
	char blkStr[CB_BLOCK_HASH_STR_SIZE];
	CBByteArrayToString(getData->itemFront->hash, 0, CB_BLOCK_HASH_STR_BYTES, blkStr, true);
	CBLogVerbose("Asked for block %s from %s", blkStr, peer->peerStr);
	CBNodeSendMessageOnNetworkThread(CBGetNetworkCommunicator(self), peer, CBGetMessage(getData), NULL);
	CBReleaseObject(getData);
}
void CBNodeFullCancelBlock(CBNodeFull * self, CBPeer * peer){
	CBMutexLock(self->pendingBlockDataMutex);
	// Add block to be downloaded by another peer, and remove this peer from this block's peers. If this peer was the only one to have the block, then forget it.
	CBFindResult res = CBAssociativeArrayFind(&self->blockPeers, peer->expectedBlock);
	CBBlockPeers * blockPeers = CBFindResultToPointer(res);
	if (blockPeers->peers.root->numElements == 1 && blockPeers->peers.root->children[0] == NULL) {
		// Only this peer has the block so delete the blockPeers and end.
		CBAssociativeArrayDelete(&self->blockPeers, res.position, true);
		CBMutexUnlock(self->pendingBlockDataMutex);
		return;
	}
	// Remove this peer from the block's peers, if found
	CBFindResult blockPeersRes = CBAssociativeArrayFind(&blockPeers->peers, peer);
	if (blockPeersRes.found)
		CBAssociativeArrayDelete(&blockPeers->peers, blockPeersRes.position, true);
	// Now get the first peer we can get this block from.
	CBPosition it;
	if (!CBAssociativeArrayGetFirst(&blockPeers->peers, &it)){
		// No peers to download from!
		CBAssociativeArrayDelete(&self->askedForBlocks, CBAssociativeArrayFind(&self->askedForBlocks, peer->expectedBlock).position, false);
		CBAssociativeArrayDelete(&self->blockPeers, res.position, true);
		CBMutexUnlock(self->pendingBlockDataMutex);
		return;
	}
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
			CBAssociativeArrayDelete(&fndTx->dependants, CBAssociativeArrayFind(&fndTx->dependants, orphan).position, false);
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
bool CBNodeFullDependantSpendsPrevOut(CBAssociativeArray * dependants, CBPrevOut prevOut){
	CBAssociativeArrayForEach(CBUnconfTransaction * dependant, dependants)
		// Loop through inputs
		for (uint32_t y = 0; y < dependant->tx->inputNum; y++)
			if (CBByteArrayCompare(dependant->tx->inputs[y]->prevOut.hash, prevOut.hash) == CB_COMPARE_EQUAL && dependant->tx->inputs[y]->prevOut.index == prevOut.index)
				return true;
	return false;
}
void CBNodeFullDownloadFromSomePeer(CBNodeFull * self, CBPeer * notPeer){
	// Loop through peers to find one we can download from
	CBMutexLock(CBGetNetworkCommunicator(self)->peersMutex);
	// ??? Order peers by block height
	CBAssociativeArrayForEach(CBPeer * peer, &CBGetNetworkCommunicator(self)->addresses->peers){
		if (!peer->downloading && !peer->upload && !peer->upToDate && peer->versionMessage && peer->versionMessage->services | CB_SERVICE_FULL_BLOCKS && peer != notPeer) {
			// Ask this peer for blocks.
			CBLogVerbose("Selected %s for block-chain download.", peer->peerStr);
			// Send getblocks
			if (!CBNodeFullSendGetBlocks(self, peer, NULL, NULL))
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
CBErrBool CBNodeFullFoundTransaction(CBNodeFull * self, CBTransaction * tx, uint64_t time, uint32_t blockHeight, bool callNow, bool processDependants){
	// Add transaction
	CBTransactionAccountDetailList * list;
	CBErrBool ours = CBAccounterFoundTransaction(CBGetNode(self)->accounterStorage, tx, blockHeight, time, &list);
	if (ours == CB_ERROR) {
		CBLogError("Could not process a transaction with the accounter.");
		return CB_ERROR;
	}
	// We will need to call CBNodeFullProcessNewTransaction to process the transaction, including its dependants.
	if (callNow){
		// We can call the callback straightaway
		if (!CBNodeFullProcessNewTransaction(self, tx, time, blockHeight, list, processDependants, ours))
			return CB_ERROR;
		if (ours)
			CBFreeTransactionAccountDetailList(list);
	}else{
		// Record in newTransactions.
		CBNewTransactionDetails * details = malloc(sizeof(*details));
		details->tx = tx;
		CBRetainObject(tx);
		details->time = time;
		details->blockHeight = blockHeight;
		details->accountDetailList = list;
		details->ours = ours;
		details->next = NULL;
		if (self->newTransactions == NULL)
			self->newTransactions = self->newTransactionsLast = details;
		else
			self->newTransactionsLast = self->newTransactionsLast->next = details;
	}
	return ours;
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
	CBPeer * peer = vpeer;
	// Stop timer
	CBEndTimer(peer->invResponseTimer);
	peer->invResponseTimerStarted = false;
	// Give a NOT_GIVEN_INV message to process queue
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
void CBNodeFullInvalidateOrphans(CBNodeFull * self, uint8_t * txHash){
	CBFindResult res = CBAssociativeArrayFind(&self->unfoundDependencies, txHash);
	if (res.found) {
		// Delete dependant orphans
		CBTransactionDependency * txDep = CBFindResultToPointer(res);
		CBPosition it;
		while (CBAssociativeArrayGetFirst(&txDep->dependants, &it)){
			CBOrphanDependency * orphanDep = it.node->elements[it.index];
			CBOrphan * orphan = orphanDep->orphan;
			self->otherTxsSize -= CBGetMessage(orphan->utx.tx)->bytes->length;
			// Delete this orphan as a dependant
			CBNodeFullDeleteOrphanFromDependencies(self, orphan);
			// Now remove from orphan array
			CBAssociativeArrayDelete(&self->orphanTxs, CBAssociativeArrayFind(&self->orphanTxs, orphan).position, true);
		}
		// Remove the unfound dependency
		CBAssociativeArrayDelete(&self->unfoundDependencies, res.position, true);
	}
}
bool CBNodeFullInvalidBlock(void * vpeer, CBBlock * block){
	CBPeer * peer = vpeer;
	CBNodeFull * self = peer->nodeObj;
	CBLogVerbose("Invlaid block.");
	if (self->initialisedUnconfirmedTxs) {
		// Delete our lost transactions and their dependencies.
		CBFreeAssociativeArray(&self->unconfirmedTxDependencies);
		CBFreeAssociativeArray(&self->unconfirmedTxs);
		self->initialisedUnconfirmedTxs = false;
	}
	if (self->initialisedFoundUnconfTxs) {
		// Delete the found unconfirmed transactions array for newly validated blocks.
		CBFreeAssociativeArray(&self->foundUnconfTxs);
		self->initialisedFoundUnconfTxs = false;
	}
	if (self->initialisedLostUnconfTxs) {
		// Delete the lost unconfirmed transactions array
		CBFreeAssociativeArray(&self->lostUnconfTxs);
		self->initialisedLostUnconfTxs = false;
	}
	if (self->initialisedLostChainTxs) {
		// Delete the lost chain transactions array
		CBFreeAssociativeArray(&self->lostChainTxs);
		self->initialisedLostChainTxs = false;
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
	CBLogVerbose("Is orphan");
	if (peer->upToDate){
		// Get previous blocks
		CBByteArray * stopAtHash = CBNewByteArrayWithDataCopy(CBBlockGetHash(block), 32);
		bool ret = CBNodeFullSendGetBlocks(node, peer, NULL, stopAtHash);
		CBReleaseObject(stopAtHash);
		peer->allowNewBranch = true;
		peer->downloading = true;
		peer->upToDate = false;
		return ret;
	}
	return true;
}
bool CBNodeFullLoseChainTxDependants(CBNodeFull * self, uint8_t * hash, bool now){
	CBFindResult res = CBAssociativeArrayFind(&self->unconfirmedTxDependencies, hash);
	if (res.found) {
		CBFindResultProcessQueueItem * queue = &(CBFindResultProcessQueueItem){res, NULL};
		CBFindResultProcessQueueItem * last = queue;
		CBFindResultProcessQueueItem * first = queue;
		while (queue != NULL) {
			// This is a dependency, so we want to remove our lost txs.
			// Loop through dependants
			CBTransactionDependency * dep = CBFindResultToPointer(queue->res);
			CBPosition it;
			// Use CBAssociativeArrayGetFirst as dependants are removed by CBNodeFullMakeLostChainTransaction
			while (CBAssociativeArrayGetFirst(&dep->dependants, &it)) {
				CBChainTransaction * dependant = it.node->elements[it.index];
				// See if the dependant has been dealt with already
				CBFindResult depRes = CBAssociativeArrayFind(&self->unconfirmedTxs, dependant);
				if (depRes.found) {
					// Now add this lost transaction dependency information if it exists to the queue
					res = CBAssociativeArrayFind(&self->unconfirmedTxDependencies, CBTransactionGetHash(dependant->tx));
					if (res.found) {
						last->next = malloc(sizeof(*last->next));
						last = last->next;
						last->res = res;
						last->next = NULL;
					}
					if (!CBNodeFullMakeLostChainTransaction(self, dependant, now))
						return false;
				}
			}
			// Move to next in queue and free data
			CBFindResultProcessQueueItem * next = queue->next;
			if (queue != first)
				free(queue);
			queue = next;
		}
	}
	return true;
}
bool CBNodeFullLostUnconfTransaction(CBNodeFull * self, CBUnconfTransaction * uTx, bool now){
	if (now) {
		// Remove now.
		if (! CBNodeFullRemoveUnconfTx(self, uTx))
			return false;
	}else{
		// Remove upon completion of reorg
		if (! self->initialisedLostUnconfTxs){
			self->initialisedLostUnconfTxs = true;
			CBInitAssociativeArray(&self->lostUnconfTxs, CBTransactionPtrCompare, NULL, NULL);
		}
		CBAssociativeArrayInsert(&self->lostUnconfTxs, uTx, CBAssociativeArrayFind(&self->lostUnconfTxs, uTx).position, NULL);
	}
	return true;
}
bool CBNodeFullNewBranchCallback(void * vpeer, uint8_t branch, uint8_t parent, uint32_t blockHeight){
	return true;
}
CBErrBool CBNodeFullNewUnconfirmedTransaction(CBNodeFull * self, CBPeer * peer, CBTransaction * tx){
	CBNode * node = CBGetNode(self);
	CBValidator * validator = node->validator;
	char txStr[CB_TX_HASH_STR_SIZE];
	CBTransactionHashToString(tx, txStr);
	if (peer != NULL)
		CBLogVerbose("%s gave us the transaction %s", peer->peerStr, txStr);
	// Check the validity of the transaction
	uint64_t outputValue;
	if (!CBTransactionValidateBasic(tx, false, &outputValue))
		return CB_FALSE;
	// Check is standard
	if (node->flags & CB_NODE_CHECK_STANDARD
		&& !CBTransactionIsStandard(tx)){
		// Ignore
		CBLogVerbose("Transaction %s is non-standard.", txStr);
		return CB_TRUE;
	}
	CBBlockBranch * mainBranch = &validator->branches[validator->mainBranch];
	uint32_t lastHeight = mainBranch->startHeight + mainBranch->lastValidation;
	// Check that the transaction is final, else ignore
	if (! CBTransactionIsFinal(tx, CBNetworkAddressManagerGetNetworkTime(CBGetNetworkCommunicator(self)->addresses), lastHeight)){
		CBLogVerbose("Transaction %s is not final.", txStr);
		return CB_TRUE;
	}
	// Check sigops
	uint32_t sigOps = CBTransactionGetSigOps(tx);
	if (sigOps > CB_MAX_SIG_OPS){
		CBLogWarning("Transaction %s has an invalid sig-op count", txStr);
		return CB_FALSE;
	}
	// Look for unspent outputs being spent
	uint64_t inputValue = 0;
	CBOrphan * orphan = NULL;
	CBInventory * orphanGetData = NULL;
	uint32_t missingInputs = 0;
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
		free(orphan);
	#define CBTxReturnFalse CBFreeOrphanData CBNodeFullInvalidateOrphans(self, CBTransactionGetHash(tx)); return CB_FALSE;
	#define CBTxReturnTrue CBFreeOrphanData CBNodeFullInvalidateOrphans(self, CBTransactionGetHash(tx)); return CB_TRUE;
	#define CBTxReturnError CBFreeOrphanData CBNodeFullInvalidateOrphans(self, CBTransactionGetHash(tx)); return CB_ERROR;
	for (uint32_t x = 0; x < tx->inputNum; x++) {
		uint8_t * prevHash = CBByteArrayGetData(tx->inputs[x]->prevOut.hash);
		// First look for unconfirmed unspent outputs
		CBFoundTransaction * prevTx = CBNodeFullGetFoundTransaction(self, prevHash);
		CBTransactionOutput * output = NULL;
		if (prevTx != NULL) {
			// Found transaction as unconfirmed, check for output
			if (prevTx->utx.tx->outputNum < tx->inputs[x]->prevOut.index + 1){
				// Not enough outputs
				CBLogWarning("Transaction %s, input %u references an output that does not exist.", txStr, x);
				CBTxReturnFalse
			}
			output = prevTx->utx.tx->outputs[tx->inputs[x]->prevOut.index];
			CBRetainObject(output);
			// Ensure not spent, else ignore.
			if (CBNodeFullDependantSpendsPrevOut(&prevTx->dependants, tx->inputs[x]->prevOut)) {
				// Double spend
				CBLogVerbose("Transaction %s, input %u references a confirmed output that is spent by an unconfirmed transaction. Ignoring.", txStr, x);
				CBTxReturnTrue
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
			if (exists == CB_ERROR){
				CBLogError("Could not determine if an unspent output exists.");
				CBTxReturnError
			}
			if (exists == CB_FALSE){
				// No unspent output.
				if (peer == NULL) {
					// Only allow complete transactions from self.
					CBLogError("Transaction %s from self is incomplete", txStr);
					CBTxReturnFalse
				}
				// Request the dependency in get data. This is an orphan transaction.
				// UNLESS the transaction was there and the output index was bad. Check for the transaction
				exists = CBBlockChainStorageTransactionExists(validator, prevHash);
				if (exists == CB_ERROR){
					CBLogError("Could not determine if a transaction exists.");
					CBTxReturnError
				}
				if (exists == CB_TRUE){
					// Output must be spent (as we only index unspent outputs), or it doesn't exist.
					CBLogVerbose("Transaction %s, input %u references an output that is possibly spent on the chain. Ignoring.", txStr, x);
					CBTxReturnTrue
				}
				missingInputs++;
				if (orphanGetData == NULL || orphanGetData->itemNum < 50000) {
					// Do not request transaction if we already expect it from this peer, or if it is an orphan
					CBFindResult res = CBAssociativeArrayFind(&peer->expectedTxs, prevHash);
					if (!res.found && !CBNodeFullGetOrphanTransaction(self, prevHash)) {
						assert(res.position.node == peer->expectedTxs.root);
						char depTxStr[41];
						CBByteArrayToString(tx->inputs[x]->prevOut.hash, 0, 20, depTxStr, true);
						assert(res.position.node == peer->expectedTxs.root);
						CBLogVerbose("Requiring dependency transaction %s from %s", depTxStr, peer->peerStr);
						// Create getdata if not already
						assert(res.position.node == peer->expectedTxs.root);
						if (orphanGetData == NULL)
							orphanGetData = CBNewInventory();
						CBRetainObject(tx->inputs[x]->prevOut.hash);
						assert(res.position.node == peer->expectedTxs.root);
						CBInventoryTakeInventoryItem(orphanGetData, CBNewInventoryItem(CB_INVENTORY_ITEM_TX, tx->inputs[x]->prevOut.hash));
						// Add to expected transactions from this peer.
						uint8_t * insertHash = malloc(20);
						memcpy(insertHash, prevHash, 20);
						assert(res.position.node == peer->expectedTxs.root);
						CBAssociativeArrayInsert(&peer->expectedTxs, insertHash, res.position, NULL);
					}
				}
				// Dependency is unfound
				depData[x].type = CB_DEP_UNFOUND;
				continue;
			}
			// Ensure this unspent output is not being spent by an unconfirmed transaction by looking at chain dependencies.
			CBFindResult res = CBAssociativeArrayFind(&self->chainDependencies, prevHash);
			if (res.found) {
				// The transaction was spent. Check that it isn't this output
				CBTransactionDependency * dep = CBFindResultToPointer(res);
				if (CBNodeFullDependantSpendsPrevOut(&dep->dependants, tx->inputs[x]->prevOut)) {
					// Double spend
					CBLogVerbose("Transaction %s, input %u references a confirmed output that is spent by an unconfirmed transaction. Ignoring.", txStr, x);
					CBTxReturnTrue
				}
			}
			// Load the unspent output
			CBErrBool ok = CBValidatorLoadUnspentOutputAndCheckMaturity(validator, tx->inputs[x]->prevOut, lastHeight, &output);
			if (ok == CB_ERROR){
				CBLogError("There was an error getting an unspent output from storage.");
				CBTxReturnError;
			}
			if (ok == CB_FALSE){
				// Not mature so ignore
				CBLogVerbose("Transaction %s is not mature.", txStr);
				CBTxReturnTrue
			}
			// Dependency is on chain
			depData[x].type = CB_DEP_CHAIN;
		}
		CBInputCheckResult inRes = CBValidatorVerifyP2SHAndStandard(output->scriptObject, tx->inputs[x]->scriptObject, &sigOps, CBGetNode(self)->flags & CB_NODE_CHECK_STANDARD);
		if (inRes == CB_INPUT_BAD) {
			CBReleaseObject(output);
			CBLogWarning("Transaction %s, input %u failed P2SH validation.", txStr, x);
			CBTxReturnFalse
		}
		if (inRes == CB_INPUT_NON_STD) {
			CBReleaseObject(output);
			CBLogVerbose("Transaction %s, input %u is non-standard. Ignoring transaction.", txStr, x);
			CBTxReturnTrue
		}
		// Verify scripts
		if (! CBValidatorVerifyScripts(tx, x, output->scriptObject)) {
			CBReleaseObject(output);
			CBLogWarning("Transaction %s, input %u failed script validation.", txStr, x);
			CBTxReturnFalse
		}
		// Add to input value
		inputValue += output->value;
		CBReleaseObject(output);
	}
	CBUnconfTransaction * uTx; // Will be set for insertion to unconf, unfound and chain dependencies.
	if (missingInputs != 0) {
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
			CBRetainObject(utx.tx);
			// The number of missing inputs is equal to the dependencies we want
			orphan->missingInputNum = missingInputs;
			// Add orphan to array
			CBAssociativeArrayInsert(&self->orphanTxs, orphan, CBAssociativeArrayFind(&self->orphanTxs, orphan).position, NULL);
			uTx = (CBUnconfTransaction *)orphan;
			CBLogVerbose("Added transaction %s as orphan", txStr);
		}else{
			CBReleaseObject(orphanGetData);
			return CB_TRUE;
		}
	}else{
		// Check that the input value is equal or more than the output value
		if (inputValue < outputValue){
			CBLogWarning("Transaction %s has an invalid output value.", txStr);
			CBTxReturnFalse
		}
		// Add this transaction as found (complete)
		CBFoundTransaction * fndTx = malloc(sizeof(*fndTx));
		CBErrBool res = CBNodeFullAddTransactionAsFound(self, utx, fndTx, inputValue);
		uTx = (CBUnconfTransaction *)fndTx;
		if (res == CB_ERROR){
			CBLogError("Could not add a transaction as found.");
			CBTxReturnError
		}
		if (res == CB_TRUE) {
			if (!CBNodeFullProcessNewTransactionProcessDependants(self, (CBNewTransactionType){false, {.uTx = fndTx}})) {
				CBLogError("Unable to process the dependants of a newly found unconfirmed transaction.");
				CBTxReturnError
			}
			// Stage the changes
			if (!CBStorageDatabaseStage(node->database)){
				CBLogError("Unable to stage changes of a new unconfirmed transaction to the database.");
				CBTxReturnError
			}
		}else
			// Could not add transaction
			return CB_TRUE;
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
	return CB_TRUE;
}
bool CBNodeFullProcessNewTransactionProcessDependants(CBNodeFull * self, CBNewTransactionType txType){
	CBNewTransactionTypeProcessQueueItem * queue = &(CBNewTransactionTypeProcessQueueItem){txType, NULL}, * firstQueue = queue;
	CBNewTransactionTypeProcessQueueItem * last = queue;
	// Check to see if any orphans depend upon this transaction. When an orphan is found (complete) then we add the orphan to the queue of complete transactions whose dependants need to be processed. After processing the found transaction, then proceed to process the next transaction in the queue.
	while (queue != NULL) {
		// See if this transaction has orphan dependants.
		CBNewTransactionType txType = queue->item;
		CBTransaction * tx = txType.chain ? txType.txData.chainTx : txType.txData.uTx->utx.tx;
		char txStr[CB_TX_HASH_STR_SIZE];
		CBTransactionHashToString(tx, txStr);
		CBFindResult res = CBAssociativeArrayFind(&self->unfoundDependencies, CBTransactionGetHash(tx));
		if (res.found) {
			// Have dependants, loop through orphan dependants, process the inputs and check for newly completed transactions.
			// Get orphan dependants
			CBTransactionDependency * txDependency = CBFindResultToPointer(res);
			CBAssociativeArray * orphanDependants = &txDependency->dependants;
			// Remove this dependency from unfoundDependencies, but do not free yet
			CBAssociativeArrayDelete(&self->unfoundDependencies, res.position, false);
			CBOrphanDependency * delOrphanDep = NULL; // Set when orphan is removed so that we can ignore fututure dependencies
			CBUnconfTransaction * lastOrphanUTx = NULL; // Used to detect change in orphan so that we can finally move the orphan to the foundDep array, and also for adding the correct dependant information when we have finished processing the particular dependencies for an orphan.
			CBAssociativeArrayForEach(CBOrphanDependency * orphanDep, orphanDependants) {
				if (delOrphanDep && delOrphanDep->orphan == orphanDep->orphan)
					continue;
				if (lastOrphanUTx && lastOrphanUTx->tx != orphanDep->orphan->utx.tx){
					// The next orphan, so add the last one to the dependants
					if (txType.chain)
						CBNodeFullAddToDependency(&self->chainDependencies, CBTransactionGetHash(tx), lastOrphanUTx, CBTransactionPtrCompare);
					else
						CBAssociativeArrayInsert(&txType.txData.uTx->dependants, lastOrphanUTx, CBAssociativeArrayFind(&txType.txData.uTx->dependants, lastOrphanUTx).position, NULL);
				}
				// Check orphan input
				CBTransactionOutput * output = tx->outputs[orphanDep->orphan->utx.tx->inputs[orphanDep->inputIndex]->prevOut.index];
				CBInputCheckResult inRes = CBValidatorVerifyP2SHAndStandard(output->scriptObject, orphanDep->orphan->utx.tx->inputs[orphanDep->inputIndex]->scriptObject, &orphanDep->orphan->sigOps, CBGetNode(self)->flags & CB_NODE_CHECK_STANDARD);
				if (inRes == CB_INPUT_OK)
					if (!CBValidatorVerifyScripts(orphanDep->orphan->utx.tx, orphanDep->inputIndex, output->scriptObject))
						inRes = CB_INPUT_BAD;
				bool rm = false;
				if (inRes == CB_INPUT_OK) {
					// Increase the input value
					orphanDep->orphan->inputValue += output->value;
					if (!txType.chain)
						// Increase the number of unconfirmed dependencies.
						orphanDep->orphan->utx.numUnconfDeps++;
					// Reduce the number of missing inputs and if there are no missing inputs left do final processing on orphan
					if (--orphanDep->orphan->missingInputNum == 0) {
						rm = true; // No longer an orphan
						// Calculate output value
						uint64_t outputValue = 0;
						for (uint32_t x = 0; x < orphanDep->orphan->utx.tx->outputNum; x++)
							// Overflows have already been checked.
							outputValue += orphanDep->orphan->utx.tx->outputs[x]->value;
						// Ensure output value is eual to or less than input value
						if (outputValue <= orphanDep->orphan->inputValue) {
							// The orphan is OK, so add it to ours or other and the queue.
							// Reallocate orphan data to found transaction data.
							CBFoundTransaction * fndTx = malloc(sizeof(CBFoundTransaction));
							CBErrBool res = CBNodeFullAddTransactionAsFound(self, orphanDep->orphan->utx, fndTx, orphanDep->orphan->inputValue);
							if (res == CB_ERROR) {
								// Free data
								while (queue != NULL) {
									CBNewTransactionTypeProcessQueueItem * next = queue->next;
									if (queue != firstQueue)
										// Only free if not first.
										free(queue);
									queue = next;
								}
								// Return with error
								return false;
							}
							if (res == CB_TRUE) {
								// Successfully added transaction as found, now add to queue
								last->next = malloc(sizeof(*last->next));
								last = last->next;
								last->item = (CBNewTransactionType){false, {.uTx = fndTx}};
								last->next = NULL;
								// Set lastOrphanUTx
								lastOrphanUTx = (CBUnconfTransaction *)fndTx;
								// Retain transaction as we will delete the orphan.
								CBRetainObject(fndTx->utx.tx);
								// Now change all dependencies to point to found transaction and not orphan data which is to be deleted.
								for (uint32_t x = 0; x < fndTx->utx.tx->inputNum; x++) {
									if (x == orphanDep->inputIndex)
										// We haven't added this one yet, obviously.
										continue;
									// Get prevout hash
									uint8_t * hash = CBByteArrayGetData(fndTx->utx.tx->inputs[x]->prevOut.hash);
									// Look for found transaction
									CBFoundTransaction * fndTxDep = CBNodeFullGetFoundTransaction(self, hash);
									if (fndTxDep != NULL){
										CBAssociativeArrayDelete(&fndTxDep->dependants, CBAssociativeArrayFind(&fndTxDep->dependants, orphanDep->orphan).position, false);
										CBAssociativeArrayInsert(&fndTxDep->dependants, fndTx, CBAssociativeArrayFind(&fndTxDep->dependants, fndTx).position, NULL);
									}else{
										// Must be a chain dependency
										CBFindResult res = CBAssociativeArrayFind(&self->chainDependencies, hash);
										CBTransactionDependency * dep = CBFindResultToPointer(res);
										// Remove orphan data from chain dependants.
										CBAssociativeArrayDelete(&dep->dependants, CBAssociativeArrayFind(&dep->dependants, orphanDep->orphan).position, false);
										// Add found transaction data
										CBAssociativeArrayInsert(&dep->dependants, fndTx, CBAssociativeArrayFind(&dep->dependants, fndTx).position, NULL);
									}
								}
							}else{
								// Not added so remove from dependencies.
								CBNodeFullDeleteOrphanFromDependencies(self, orphanDep->orphan);
								lastOrphanUTx = NULL; // Do not add to dependants.
							}
						}else{
							// Else orphan is not OK. We will need to remove the orphan from dependencies.
							CBNodeFullDeleteOrphanFromDependencies(self, orphanDep->orphan);
							lastOrphanUTx = NULL; // Do not add to dependants.
						}
					}else
						// Else still requiring inputs
						// Set lastOrphanUTx
						lastOrphanUTx = (CBUnconfTransaction *)orphanDep->orphan;
				}else{
					// We are to remove this orphan because of an invalid or non-standard input
					rm = true;
					// Since this orphan failed before satisfying all inputs, check for other input dependencies (of unprocessed inputs ie. for unfound transactions) and remove this orphan as a dependant. We have already removed this transaction's unforund dependencies from the array, so it will not interfere with that.
					CBNodeFullDeleteOrphanFromDependencies(self, orphanDep->orphan);
					lastOrphanUTx = NULL; // Do not add to dependants.
				}
				if (rm) {
					// Remove orphan
					CBAssociativeArrayDelete(&self->orphanTxs, CBAssociativeArrayFind(&self->orphanTxs, orphanDep->orphan).position, true);
					// Ignore other dependencies for this orphan
					delOrphanDep = orphanDep;
					continue;
				}
			}
			if (lastOrphanUTx){
				// Add the last orphan to the dependants
				if (txType.chain)
					CBNodeFullAddToDependency(&self->chainDependencies, CBTransactionGetHash(tx), lastOrphanUTx, CBTransactionPtrCompare);
				else
					CBAssociativeArrayInsert(&txType.txData.uTx->dependants, lastOrphanUTx, CBAssociativeArrayFind(&txType.txData.uTx->dependants, lastOrphanUTx).position, NULL);
			}
			// Completed processing for this found transaction.
			// Free the transaction dependency
			CBFreeTransactionDependency(txDependency);
		}
		// Move to next in the queue
		CBNewTransactionTypeProcessQueueItem * next = queue->next;
		// Free this queue item unless first
		if (queue != firstQueue)
			free(queue);
		queue = next;
	}
	return true;
}
bool CBNodeFullMakeLostChainTransaction(CBNodeFull * self, CBChainTransaction * tx, bool now){
	// Lose uconf dependants
	uint8_t * hash = CBTransactionGetHash(tx->tx);
	CBFindResult res = CBAssociativeArrayFind(&self->chainDependencies, hash);
	if (res.found) {
		// This transaction is indeed a dependency of at least one transaction we have as unconfirmed
		// Loop through dependants
		CBTransactionDependency * dep = CBFindResultToPointer(res);
		CBAssociativeArrayForEach(CBUnconfTransaction * dependant, &dep->dependants)
			// Lose the unconfirmed dependant
			if (! CBNodeFullLostUnconfTransaction(self, dependant, now))
				return false;
	}
	if (tx->ours) {
		// Make this a lost chain transaction
		if (now)
			// Call doubleSpend
			CBGetNode(self)->callbacks.doubleSpend(CBGetNode(self), hash);
		else{
			// Remove upon completion of reorg
			if (! self->initialisedLostChainTxs) {
				self->initialisedLostChainTxs = true;
				CBInitAssociativeArray(&self->lostChainTxs, CBTransactionPtrCompare, NULL, CBFreeChainTransaction);
			}
			CBAssociativeArrayInsert(&self->lostChainTxs, tx, CBAssociativeArrayFind(&self->lostChainTxs, tx).position, NULL);
		}
		// Remove from accounter now
		if (! CBAccounterLostTransaction(CBGetNode(self)->accounterStorage, tx->tx, tx->height)) {
			CBLogError("Could not lose a chain transaction from the accounter.");
			return false;
		}
	}
	// Remove transaction from unconfirmedTxDependency dependants
	for (uint32_t x = 0; x < tx->tx->inputNum; x++) {
		CBFindResult res = CBAssociativeArrayFind(&self->unconfirmedTxDependencies, CBByteArrayGetData(tx->tx->inputs[x]->prevOut.hash));
		if (res.found) {
			CBTransactionDependency * dep = CBFindResultToPointer(res);
			CBAssociativeArrayForEach(CBChainTransaction * dependant, &dep->dependants){
				if (dependant == tx) {
					CBAssociativeArrayDelete(&dep->dependants, CBCurrentPosition, false);
					// If the dependency is empty, do not remove now. This would corrupt the loop going through a dependency in CBNodeFullLoseChainTxDependants
					break;
				}
			}
		}
	}
	// Remove transaction from unconfirmedTxs, and do not delete the CBTransactionOfType if we added it to the lostChainTxs
	CBAssociativeArrayDelete(&self->unconfirmedTxs, CBAssociativeArrayFind(&self->unconfirmedTxs, tx).position, !tx->ours || now);
	return true;
}
bool CBNodeFullNoNewBranches(void * vpeer, CBBlock * block) {
	CBPeer * peer = vpeer;
	CBNodeFull * self = peer->nodeObj;
	CBLogVerbose("No new branches");
	// Can't accept blocks from this peer at the moment
	CBMutexUnlock(CBGetNode(self)->blockAndTxMutex);
	CBNodeFullPeerDownloadEnd(self, peer);
	// Remove this block from blockPeers and askedForBlocks
	CBNodeFullFinishedWithBlock(self, block);
	return true;
}
void CBNodeFullOnNetworkError(CBNetworkCommunicator * comm, CBErrorReason reason){
	CBNode * node = CBGetNode(comm);
	node->callbacks.onFatalNodeError(node, reason);
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
	CBLogVerbose("Ending download from %s", peer->peerStr);
	CBMutexLock(self->numberPeersDownloadMutex);
	self->numberPeersDownload--;
	CBMutexUnlock(self->numberPeersDownloadMutex);
	peer->expectBlock = false;
	peer->downloading = false;
	// Now not working on any branches
	if (peer->branchWorkingOn != CB_NO_BRANCH) {
		CBMutexLock(self->workingMutex);
		CBGetNode(self)->validator->branches[peer->branchWorkingOn].working--;
		CBMutexUnlock(self->workingMutex);
		peer->branchWorkingOn = CB_NO_BRANCH;
		peer->allowNewBranch = true;
	}
	// See if we can download from any other peers
	CBNodeFullDownloadFromSomePeer(self, peer);
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
	CBFreeMutex(peer->requestedDataMutex);
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
		// Remove peer from blockPeers.
		CBMutexLock(fullNode->pendingBlockDataMutex);
		for (; peer->reqBlockCursor != CB_END_REQ_BLOCKS; peer->reqBlockCursor = peer->reqBlocks[peer->reqBlockCursor].next) {
			uint8_t * blockHash = peer->reqBlocks[peer->reqBlockCursor].reqBlock;
			CBFindResult res = CBAssociativeArrayFind(&fullNode->blockPeers, blockHash);
			if (!res.found)
				// Must have been deleted from blockPeers in CBNodeFullCancelBlock
				continue;
			CBBlockPeers * blockPeers = CBFindResultToPointer(res);
			if (blockPeers->peers.root->numElements == 1 && !blockPeers->peers.root->children[0])
				// Only have one more element, so delete block from blockPeers.
				CBAssociativeArrayDelete(&fullNode->blockPeers, res.position, true);
			else{
				// Else only delete peer from blockPeers
				res = CBAssociativeArrayFind(&fullNode->blockPeers, peer);
				if (!res.found)
					// Must have been deleted from blockPeers in CBNodeFullCancelBlock
					continue;
				CBAssociativeArrayDelete(&blockPeers->peers, res.position, false);
			}
		}
		CBMutexUnlock(fullNode->pendingBlockDataMutex);
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
	CBNodeFullDownloadFromSomePeer(fullNode, peer);
	CBFreePeer(peer);
}
void CBNodeFullPeerSetup(CBNetworkCommunicator * self, CBPeer * peer){
	CBGetObject(peer)->free = CBNodeFullPeerFree;
	CBInitAssociativeArray(&peer->expectedTxs, CBHashCompare, NULL, free);
	peer->downloading = false;
	peer->upload = false;
	peer->expectBlock = false;
	peer->requestedData = NULL;
	peer->nodeObj = self;
	peer->upToDate = false;
	peer->invResponseTimerStarted = false;
	peer->reqBlockCursor = CB_END_REQ_BLOCKS;
	CBNewMutex(&peer->invResponseMutex);
	CBNewMutex(&peer->requestedDataMutex);
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
bool CBNodeFullProcessNewTransaction(CBNodeFull * self, CBTransaction * tx, uint64_t time, uint32_t blockHeight, CBTransactionAccountDetailList * list, bool processDependants, bool ours){
	// Call newTransaction if ours
	if (ours)
		CBGetNode(self)->callbacks.newTransaction(CBGetNode(self), tx, time, blockHeight, list);
	// Check orphans for chain dependencies.
	if (processDependants && !CBNodeFullProcessNewTransactionProcessDependants(self, (CBNewTransactionType){true, {.chainTx = tx}})) {
		CBLogError("Failed to process the dependants of a new transaction found on the block chain.");
		return false;
	}
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
			}else if (self->numCanDownload < CB_MIN_DOWNLOAD){
				CBMutexUnlock(self->numCanDownloadMutex);
				return CB_MESSAGE_ACTION_DISCONNECT;
			}
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
				|| self->numberPeersDownload == CB_MAX_BLOCK_QUEUE){
				CBMutexUnlock(self->numberPeersDownloadMutex);
				break;
			}
			self->numberPeersDownload++;
			CBMutexUnlock(self->numberPeersDownloadMutex);
			// Send getblocks
			if (!CBNodeFullSendGetBlocks(self, peer, NULL, NULL))
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
			// Begin sending requested data, or add the requested data to the existing requested data.
			char hashStr[41];
			CBByteArrayToString(getData->itemFront->hash, 0, 20, hashStr, true);
			CBMutexLock(peer->requestedDataMutex);
			if (peer->requestedData == NULL) {
				peer->requestedData = getData;
				CBRetainObject(peer->requestedData);
				CBLogVerbose("New getdata with first hash %s from %s", hashStr, peer->peerStr);
				CBNodeFullSendRequestedData(self, peer);
			}else{
				// Add to existing requested data
				uint16_t addNum = getData->itemNum;
				if (addNum + peer->requestedData->itemNum > 50000)
					// Adjust to give no more than 50000
					addNum = 50000 - peer->requestedData->itemNum;
				for (uint16_t x = 0; x < addNum; x++)
					CBInventoryTakeInventoryItem(peer->requestedData, CBInventoryPopInventoryItem(getData));
				CBLogVerbose("Apended getdata with first hash %s from %s", hashStr, peer->peerStr);
			}
			CBMutexUnlock(peer->requestedDataMutex);
			break;
		}
		case CB_MESSAGE_TYPE_INV:{
			// The peer has sent us an inventory broadcast
			CBInventory * inv = CBGetInventory(message);
			// Get data request for objects we want.
			CBInventory * getData = NULL;
			uint16_t reqBlockNum = 0;
			for (CBInventoryItem * item; (item = CBInventoryPopInventoryItem(inv)) != NULL;) {
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
					char blockStr[CB_BLOCK_HASH_STR_SIZE];
					CBByteArrayToString(item->hash, 0, CB_BLOCK_HASH_STR_BYTES, blockStr, true);
					if (exists == CB_TRUE) {
						// Ignore this block
						// If the last block is not on the same branch as we are downloading from this peer, then resend getblocks for the other branch. This is needed in the case of very large reorgs.
						if (item == inv->itemBack){
							// Only allow for ignored blocks when working on another branch.
							uint8_t branch;
							uint32_t index;
							if (CBBlockChainStorageGetBlockLocation(CBGetNode(self)->validator, hash, &branch, &index) != CB_TRUE)
								return CBNodeReturnError(CBGetNode(self), "Couldn't get location of last block in an inventory to see if the peer has changed to another branch.");
							if (peer->branchWorkingOn != branch) {
								// Working on new branch
								CBMutexLock(self->workingMutex);
								if (peer->branchWorkingOn != CB_NO_BRANCH)
									CBGetNode(self)->validator->branches[peer->branchWorkingOn].working--;
								CBGetNode(self)->validator->branches[branch].working++;
								CBMutexUnlock(self->workingMutex);
								peer->branchWorkingOn = branch;
								peer->allowNewBranch = true;
								CBNodeFullSendGetBlocks(self, peer, NULL, NULL);
								return CB_MESSAGE_ACTION_CONTINUE;
							}
							// All blocks ignored but not resending getblocks, so uptodate
							CBNodeFullPeerDownloadEnd(self, peer);
							peer->upToDate = true;
						}
						continue;
					}
					// Ensure we have not asked for this block yet.
					CBMutexLock(self->pendingBlockDataMutex);
					CBFindResult res = CBAssociativeArrayFind(&self->askedForBlocks, hash);
					if (res.found){
						CBLogVerbose("Already asked for the block %s from the inv from %s.", blockStr, peer->peerStr);
						CBMutexUnlock(self->pendingBlockDataMutex);
						continue;
					}
					// Add block to required blocks from peer
					memcpy(peer->reqBlocks[reqBlockNum].reqBlock, hash, 32);
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
					// Ensure the peer has not given us this block yet
					CBFindResult blockPeersRes = CBAssociativeArrayFind(&blockPeers->peers, peer);
					if (blockPeersRes.found) {
						CBLogVerbose("Already got the block %s from %s.", blockStr, peer->peerStr);
						CBMutexUnlock(self->pendingBlockDataMutex);
						continue;
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
						CBAssociativeArrayInsert(&blockPeers->peers, peer, blockPeersRes.position, NULL);
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
					char txStr[41];
					CBByteArrayToString(item->hash, 0, 20, txStr, true);
					CBLogVerbose("%s gave us hash of transaction %s", peer->peerStr, txStr);
					// We do not have the transaction, so add it to getdata
					if (getData == NULL)
						getData = CBNewInventory();
					CBInventoryAddInventoryItem(getData, item);
					// Also add it to expectedTxs
					uint8_t * insertHash = malloc(20);
					memcpy(insertHash, hash, 20);
					CBAssociativeArrayInsert(&peer->expectedTxs, insertHash, res.position, NULL);
				}
			}
			if (reqBlockNum != 0) {
				// Has blocks in inventory
				if (peer->invResponseTimerStarted) {
					CBEndTimer(peer->invResponseTimer);
					peer->invResponseTimerStarted = false;
				}
				// Add CB_END_REQ_BLOCKS to last required block
				peer->reqBlocks[reqBlockNum-1].next = CB_END_REQ_BLOCKS;
			}
			// Send getdata if not NULL
			if (getData != NULL){
				CBGetMessage(getData)->type = CB_MESSAGE_TYPE_GETDATA;
				CBNodeSendMessageOnNetworkThread(CBGetNetworkCommunicator(self), peer, CBGetMessage(getData), NULL);
				CBReleaseObject(getData);
			}
			break;
		}
		case CB_MESSAGE_TYPE_TX:{
			CBTransaction * tx = CBGetTransaction(message);
			// Ensure we expected this transaction
			CBFindResult res = CBAssociativeArrayFind(&peer->expectedTxs, CBTransactionGetHash(tx));
			if (!res.found){
				// Did not expect this transaction
				char txStr[CB_TX_HASH_STR_SIZE];
				CBTransactionHashToString(tx, txStr);
				CBLogWarning("%s gave us an unexpected transaction %s.", peer->peerStr, txStr);
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
			CBErrBool result = CBNodeFullNewUnconfirmedTransaction(self, peer, tx);
			CBMutexUnlock(node->blockAndTxMutex);
			if (result == CB_ERROR)
				return CBNodeReturnError(node, "Failed to process a new unconfirmed transaction.");
			if (result == CB_FALSE)
				return CB_MESSAGE_ACTION_DISCONNECT;
			break;
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
bool CBNodeFullRemoveBlock(void * vpeer, uint8_t branch, uint32_t blockHeight, CBBlock * block){
	CBNodeFull * self = CBGetPeer(vpeer)->nodeObj;
	// Loop through and process transactions
	for (uint32_t x = 1; x < block->transactionNum; x++) {
		CBTransaction * tx = block->transactions[x];
		// Add transaction to unconfirmedTxs
		if (! self->initialisedUnconfirmedTxs) {
			// Initialise the lost transactions array
			CBInitAssociativeArray(&self->unconfirmedTxs, CBTransactionPtrCompare, NULL, CBFreeChainTransaction);
			// Initialise the array for the dependencies of the lost transactions.
			CBInitAssociativeArray(&self->unconfirmedTxDependencies, CBHashCompare, NULL, CBFreeTransactionDependency);
			self->initialisedUnconfirmedTxs = true;
		}
		// Make transaction of type
		CBChainTransaction * chainTx = malloc(sizeof(*chainTx));
		chainTx->tx = tx;
		chainTx->height = blockHeight;
		// Add this transaction to the lostTxs array
		CBAssociativeArrayInsert(&self->unconfirmedTxs, chainTx, CBAssociativeArrayFind(&self->unconfirmedTxs, chainTx).position, NULL);
		CBRetainObject(tx);
		// Add dependencies
		for (uint32_t y = 0; y < tx->inputNum; y++)
			CBNodeFullAddToDependency(&self->unconfirmedTxDependencies, CBByteArrayGetData(tx->inputs[y]->prevOut.hash), chainTx, CBTransactionPtrCompare);
		// See if this transaction is ours or not.
		CBErrBool ours = CBAccounterIsOurs(CBGetNode(self)->accounterStorage, CBTransactionGetHash(tx));
		if (ours == CB_ERROR) {
			CBLogError("Could not determine if a transaction is ours.");
			return false;
		}
		chainTx->ours = ours == CB_TRUE;
	}
	return true;
}
void CBNodeFullRemoveFoundTransactionFromDependencies(CBNodeFull * self, CBUnconfTransaction * uTx){
	for (uint32_t x = 0; x < uTx->tx->inputNum; x++) {
		uint8_t * prevHash = CBByteArrayGetData(uTx->tx->inputs[x]->prevOut.hash);
		// See if the previous hash is of a found transaction
		CBFoundTransaction * fndTx = CBNodeFullGetFoundTransaction(self, prevHash);
		if (fndTx)
			CBAssociativeArrayDelete(&fndTx->dependants, CBAssociativeArrayFind(&fndTx->dependants, uTx).position, false);
		else
			// Remove from chain dependency if it exists in one.
			CBNodeFullRemoveFromDependency(&self->chainDependencies, prevHash, uTx);
	}
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
	CBAssociativeArrayDelete(&dep->dependants, res2.position, false);
	// See if the array is empty. If it is then we can remove this dependant
	if (CBAssociativeArrayIsEmpty(&dep->dependants))
		CBAssociativeArrayDelete(deps, res.position, true);
}
bool CBNodeFullRemoveLostUnconfTxs(CBNodeFull * self){
	if (self->initialisedLostUnconfTxs) {
		CBAssociativeArrayForEach(CBUnconfTransaction * tx, &self->lostUnconfTxs)
			if (!CBNodeFullRemoveUnconfTx(self, tx)) {
				CBLogError("Could not process an unconfirmed transaction being lost.");
				return false;
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
		if (uTx->type == CB_TX_ORPHAN){
			CBAssociativeArrayDelete(&self->orphanTxs, CBAssociativeArrayFind(&self->orphanTxs, uTx->tx).position, false);
			CBNodeFullDeleteOrphanFromDependencies(self, (CBOrphan *)uTx);
			CBFreeOrphan(uTx);
		}else{ // CB_TX_OURS or CB_TX_OTHER
			CBFoundTransaction * fndTx = (CBFoundTransaction *)uTx;
			// Add dependants to queue
			CBAssociativeArrayForEach(CBUnconfTransaction * dependant, &fndTx->dependants) {
				last->next = malloc(sizeof(*last->next));
				last = last->next;
				last->item = dependant;
				last->next = NULL;
			}
			// Remove from storage
			if (uTx->type == CB_TX_OURS) {
				if (!CBNodeStorageRemoveOurTx(CBGetNode(self)->nodeStorage, uTx->tx)) {
					CBLogError("Could not remove our lost transaction.");
					return false;
				}
			}else if (uTx->type == CB_TX_OTHER && !CBNodeStorageRemoveOtherTx(CBGetNode(self)->nodeStorage, uTx->tx)) {
				CBLogError("Could not remove other lost transaction.");
				return false;
			}
			// Remove from ours or other array
			CBAssociativeArray * arr = (uTx->type == CB_TX_OURS) ? &self->ourTxs : &self->otherTxs;
			CBAssociativeArrayDelete(arr, CBAssociativeArrayFind(arr, fndTx).position, false);
			// Also delete from allChainFoundTxs if in there
			CBFindResult res = CBAssociativeArrayFind(&self->allChainFoundTxs, fndTx);
			if (res.found)
				CBAssociativeArrayDelete(&self->allChainFoundTxs, res.position, false);
			// Remove from dependencies
			CBNodeFullRemoveFoundTransactionFromDependencies(self, uTx);
			if (uTx->type == CB_TX_OURS){
				// Remove from accounter
				if (! CBAccounterLostTransaction(CBGetNode(self)->accounterStorage, uTx->tx, CB_UNCONFIRMED)) {
					CBLogError("Could not lose unconfirmed transaction from the accounter.");
					return false;
				}
				// Call callback
				CBGetNode(self)->callbacks.doubleSpend(CBGetNode(self), CBTransactionGetHash(uTx->tx));
			}
			// Delete unconfirmed transaction
			CBFreeFoundTransaction(uTx);
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
bool CBNodeFullSendGetBlocks(CBNodeFull * self, CBPeer * peer, uint8_t * extraBlock, CBByteArray * stopAtHash){
	// a5923556af
	// Lock for access to block chain.
	CBMutexLock(CBGetNode(self)->blockAndTxMutex);
	// We need to make sure we get the chain descriptor for the branch we are working on with the peer.
	uint8_t branch = peer->branchWorkingOn == CB_NO_BRANCH ? CBGetNode(self) ->validator->mainBranch : peer->branchWorkingOn;
	CBChainDescriptor * chainDesc = CBValidatorGetChainDescriptor(CBGetNode(self)->validator, branch, extraBlock);
	CBMutexUnlock(CBGetNode(self)->blockAndTxMutex);
	if (chainDesc == NULL){
		CBLogError("There was an error when trying to retrieve the block-chain descriptor.");
		return false;
	}
	char blockStr[CB_BLOCK_HASH_STR_SIZE];
	if (chainDesc->hashNum == 1)
		strcpy(blockStr, "genesis");
	else
		CBByteArrayToString(chainDesc->hashes[0], 0, CB_BLOCK_HASH_STR_BYTES, blockStr, true);
	CBLogVerbose("Sending getblocks to %s with block %s as the latest.", peer->peerStr, blockStr);
	CBGetBlocks * getBlocks = CBNewGetBlocks(CB_MIN_PROTO_VERSION, chainDesc, stopAtHash);
	CBReleaseObject(chainDesc);
	CBGetMessage(getBlocks)->type = CB_MESSAGE_TYPE_GETBLOCKS;
	CBNodeSendMessageOnNetworkThread(CBGetNetworkCommunicator(self), peer, CBGetMessage(getBlocks), NULL);
	CBReleaseObject(getBlocks);
	// Check in the future that the peer did send us an inv of blocks
	// If responseTimeout is 0 then wait indefinitely. 0 should only be set when debugging, as there should be timeouts.
	if (CBGetNetworkCommunicator(self)->responseTimeOut != 0) {
		CBStartTimer(CBGetNetworkCommunicator(self)->eventLoop, &peer->invResponseTimer, CBGetNetworkCommunicator(self)->responseTimeOut, CBNodeFullHasNotGivenBlockInv, peer);
		peer->invResponseTimerStarted = true;
	}
	return true;
}
void CBNodeFullSendRequestedData(void * vself, void * vpeer){
	CBNodeFull * self = vself;
	CBPeer * peer = vpeer;
	CBMutexLock(peer->requestedDataMutex);
	CBLogVerbose("Sending data to %s", peer->peerStr);
	for (;;) {
		if (peer->requestedData == NULL) {
			// Done already
			CBMutexUnlock(peer->requestedDataMutex);
			return;
		}
		// Pop next item
		CBInventoryItem * item = CBInventoryPopInventoryItem(peer->requestedData);
		if (item == NULL) {
			// We have sent everything
			CBReleaseObject(peer->requestedData);
			peer->requestedData = NULL;
			CBMutexUnlock(peer->requestedDataMutex);
			CBLogVerbose("Ended send data to %s", peer->peerStr);
			return;
		}
		// Lock for block or transaction access
		CBMutexLock(CBGetNode(self)->blockAndTxMutex);
		if (item->type == CB_INVENTORY_ITEM_TX) {
			// Look at all unconfirmed transactions
			// Change the comparison function to accept a hash in place of a transaction object.
			CBFoundTransaction * fndTx = CBNodeFullGetFoundTransaction(self, CBByteArrayGetData(item->hash));
			if (fndTx != NULL){
				// The peer was requesting an unconfirmed transaction
				CBGetMessage(fndTx->utx.tx)->type = CB_MESSAGE_TYPE_TX;
				char txStr[CB_TX_HASH_STR_SIZE];
				CBTransactionHashToString(fndTx->utx.tx, txStr);
				CBLogVerbose("Sending transaction %s to %s", txStr, peer->peerStr);
				CBMutexUnlock(CBGetNode(self)->blockAndTxMutex);
				CBMutexUnlock(peer->requestedDataMutex);
				CBNodeSendMessageOnNetworkThread(CBGetNetworkCommunicator(self), peer, CBGetMessage(fndTx->utx.tx), CBNodeFullSendRequestedData);
				CBReleaseObject(item);
				return;
			}
			// Else the peer was requesting something we do not have as unconfirmed so ignore.
			// Loop around to next...
		}else if (item->type == CB_INVENTORY_ITEM_BLOCK) {
			// Look for the block to give
			uint8_t branch;
			uint32_t index;
			CBErrBool exists = CBBlockChainStorageGetBlockLocation(CBGetNode(self)->validator, CBByteArrayGetData(item->hash), &branch, &index);
			if (exists == CB_ERROR) {
				CBLogError("Could not get the location for a block when a peer requested it of us.");
				CBGetNode(self)->callbacks.onFatalNodeError(CBGetNode(self), CB_ERROR_GENERAL);
				CBFreeNodeFull(self);
				CBMutexUnlock(CBGetNode(self)->blockAndTxMutex);
				CBMutexUnlock(peer->requestedDataMutex);
				CBReleaseObject(item);
				return;
			}
			char blkStr[CB_BLOCK_HASH_STR_SIZE];
			CBByteArrayToString(item->hash, 0, CB_BLOCK_HASH_STR_BYTES, blkStr, true);
			if (exists == CB_FALSE) {
				// The peer is requesting a block we do not have. Obviously we may have removed the block since, but this is unlikely
				CBMutexUnlock(peer->requestedDataMutex);
				CBMutexUnlock(CBGetNode(self)->blockAndTxMutex);
				CBLogWarning("Peer %s is requesting the block %s which we do not have.", peer->peerStr, blkStr);
				CBRunOnEventLoop(CBGetNetworkCommunicator(self)->eventLoop, CBNodeDisconnectPeer, peer, false);
				CBReleaseObject(item);
				return;
			}
			// Load the block from storage
			CBBlock * block = CBBlockChainStorageLoadBlock(CBGetNode(self)->validator, index, branch);
			if (block == NULL) {
				CBLogError("Could not load a block from storage.");
				CBGetNode(self)->callbacks.onFatalNodeError(CBGetNode(self), CB_ERROR_GENERAL);
				CBFreeNodeFull(self);
				CBMutexUnlock(CBGetNode(self)->blockAndTxMutex);
				CBMutexUnlock(peer->requestedDataMutex);
				CBReleaseObject(item);
				return;
			}
			CBLogVerbose("Sending block %s to %s", blkStr, peer->peerStr);
			CBGetMessage(block)->type = CB_MESSAGE_TYPE_BLOCK;
			CBMutexUnlock(CBGetNode(self)->blockAndTxMutex);
			CBMutexUnlock(peer->requestedDataMutex);
			CBNodeSendMessageOnNetworkThread(CBGetNetworkCommunicator(self), peer, CBGetMessage(block), CBNodeFullSendRequestedData);
			CBReleaseObject(block);
			CBReleaseObject(item);
			return;
		}
		CBMutexUnlock(CBGetNode(self)->blockAndTxMutex);
		// Unknown, loop around.
		CBReleaseObject(item);
	}
}
bool CBNodeFullStartValidation(void * vpeer){
	CBMutexLock(CBGetNode(CBGetPeer(vpeer)->nodeObj)->blockAndTxMutex);
	CBGetNodeFull(CBGetPeer(vpeer)->nodeObj)->forkPoint = CB_NO_FORK;
	return true;
}
bool CBNodeFullUnconfToChain(CBNodeFull * self, CBUnconfTransaction * uTx, uint32_t blockHeight, uint32_t time){
	// ??? Remove redundant code with CBNodeFullRemoveUnconfTx?
	if (uTx->type == CB_TX_ORPHAN){
		CBAssociativeArrayDelete(&self->orphanTxs, CBAssociativeArrayFind(&self->orphanTxs, uTx).position, false);
		// Try adding transaction to accounter
		CBTransactionAccountDetailList * list;
		if (! CBAccounterFoundTransaction(CBGetNode(self)->accounterStorage, uTx->tx, blockHeight, time, &list)) {
			CBLogError("Could not process orphan to chain with the accounter.");
			return false;
		}
		// Process dependants for this orphan
		if (!CBNodeFullProcessNewTransactionProcessDependants(self, (CBNewTransactionType){true, {.chainTx = uTx->tx}})){
			CBLogError("Could not process dependants for an orphan found on the block-chain");
			return false;
		}
		// Remove from dependencies
		CBNodeFullDeleteOrphanFromDependencies(self, (CBOrphan *) uTx);
		// Call newTransaction
		CBGetNode(self)->callbacks.newTransaction(CBGetNode(self), uTx->tx, time, blockHeight, list);
		CBFreeOrphan(uTx);
		return true;
	}
	// CB_TX_OURS or CB_TX_OTHER
	CBFoundTransaction * fndTx = (CBFoundTransaction *)uTx;
	// Change to chain dependency
	CBTransactionDependency * dep = malloc(sizeof(*dep));
	memcpy(dep->hash, CBTransactionGetHash(uTx->tx), 32);
	dep->dependants = fndTx->dependants;
	CBAssociativeArrayInsert(&self->chainDependencies, dep, CBAssociativeArrayFind(&self->chainDependencies, dep).position, NULL);
	// Remove from ours or other array
	CBAssociativeArray * arr = (uTx->type == CB_TX_OURS) ? &self->ourTxs : &self->otherTxs;
	CBAssociativeArrayDelete(arr, CBAssociativeArrayFind(arr, fndTx).position, false);
	// Also delete from allChainFoundTxs if in there
	CBFindResult res = CBAssociativeArrayFind(&self->allChainFoundTxs, fndTx);
	if (res.found)
		CBAssociativeArrayDelete(&self->allChainFoundTxs, res.position, false);
	// Remove from dependencies
	CBNodeFullRemoveFoundTransactionFromDependencies(self, uTx);
	// Decrement the number of unconfirmed dependencies for all dependants.
	CBAssociativeArrayForEach(CBUnconfTransaction * dependant, &dep->dependants){
		if (--dependant->numUnconfDeps == 0 && dependant->type != CB_TX_ORPHAN)
			// Add this to the allChainFoundTxs
			CBAssociativeArrayInsert(&self->allChainFoundTxs, dependant, CBAssociativeArrayFind(&self->allChainFoundTxs, dependant).position, NULL);
	}
	// Move to chain on accounter
	if (uTx->type == CB_TX_OURS){
		if (! CBAccounterTransactionChangeHeight(CBGetNode(self)->accounterStorage, uTx->tx, CB_UNCONFIRMED, blockHeight)) {
			CBLogError("Could not make unconf transaction a confirmed one on the accounter.");
			return false;
		}
		// Call transactionConfirmed if ours
		CBGetNode(self)->callbacks.transactionConfirmed(CBGetNode(self), CBTransactionGetHash(uTx->tx), blockHeight);
	}
	// Now delete as unconfirmed, but keep dependants which the chain dependency now has.
	CBReleaseObject(fndTx->utx.tx);
	free(fndTx);
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
	// Remove this block from blockPeers and askedForBlocks
	CBNodeFullFinishedWithBlock(self, block);
	// Get the next block or ask for new blocks only if not up to date
	if (peer->upToDate){
		peer->expectBlock = false;
		peer->downloading = false;
		return true;
	}
	// Get the next required block
	CBMutexLock(self->pendingBlockDataMutex);
	// By default this is the last block and will be included in the chain descriptor.
	uint8_t * lastBlock = NULL;
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
		lastBlock = hash;
	}
	CBMutexUnlock(self->pendingBlockDataMutex);
	// The node is allowed to give a new branch with a new inventory
	peer->allowNewBranch = true;
	// No next required blocks for this node, now allowing for inv response.
	peer->expectBlock = false;
	// Ask for new block inventory.
	if (!CBNodeFullSendGetBlocks(self, peer, lastBlock, NULL))
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
