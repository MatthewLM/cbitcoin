//
//  CBNodeStorage.c
//  cbitcoin
//
//  Created by Matthew Mitchell on 30/09/2013.
//  Copyright (c) 2013 Matthew Mitchell
//
//  This file is part of cbitcoin. It is subject to the license terms
//  in the LICENSE file found in the top-level directory of this
//  distribution and at http://www.cbitcoin.com/license.html. No part of
//  cbitcoin, including this file, may be copied, modified, propagated,
//  or distributed except according to the terms contained in the
//  LICENSE file.

//  SEE HEADER FILE FOR DOCUMENTATION

#include "CBNodeStorage.h"
#include "CBNodeFull.h"

bool CBNewNodeStorage(CBDepObject * storage, CBDepObject database){
	CBNodeStorage * self = malloc(sizeof(*self));
	self->database = database.ptr;
	self->ourTxs = CBLoadIndex(self->database, CB_INDEX_OUR_TXS, 32, 10000);
	if (self->ourTxs) {
		self->otherTxs = CBLoadIndex(self->database, CB_INDEX_OTHER_TXS, 32, 10000);
		if ( self->otherTxs) {
			storage->ptr = self;
			return true;
		}else
			CBLogError("Could not load other unconfirmed transactions index.");
	}else
		CBLogError("Could not load our unconfirmed transactions index.");
	return false;
}
void CBFreeNodeStorage(CBDepObject storage){
	CBFreeIndex(((CBNodeStorage *)storage.ptr)->ourTxs);
	free(storage.ptr);
}
bool CBNodeStorageGetStartScanningTime(CBDepObject vstorage, uint64_t * startTime){
	CBNodeStorage * storage = vstorage.ptr;
	*startTime = CBArrayToInt64(storage->database->current.extraData, CB_START_SCANNING_TIME);
	if (*startTime == 0) {
		// Set start time to now
		*startTime = time(NULL);
		CBInt64ToArray(storage->database->current.extraData, CB_START_SCANNING_TIME, *startTime);
	}
	return true;
}
bool CBNodeStorageLoadUnconfTxs(void * vnode){
	// ??? Split some code over to CBNodeFull
	CBNodeFull * node = vnode;
	CBNodeStorage * storage = CBGetNode(node)->nodeStorage.ptr;
	for (CBDatabaseIndex * index = storage->ourTxs; index == storage->ourTxs; index = storage->otherTxs) {
		CBDatabaseRangeIterator it;
		CBInitDatabaseRangeIterator(&it, (uint8_t [32]){0}, (uint8_t [32]){0xFF}, index);
		CBIndexFindStatus res = CBDatabaseRangeIteratorFirst(&it);
		if (res == CB_DATABASE_INDEX_ERROR) {
			CBLogError("Could not find the first unconfirmed transaction to load");
			CBFreeDatabaseRangeIterator(&it);
			return false;
		}
		while (res == CB_DATABASE_INDEX_FOUND) {
			uint8_t key[32];
			CBDatabaseRangeIteratorGetKey(&it, key);
			// Get the transaction size
			uint32_t len;
			if (! CBDatabaseRangeIteratorGetLength(&it, &len)) {
				CBLogError("Could not get the length of an unconfirmed transaction.");
				CBFreeDatabaseRangeIterator(&it);
				return false;
			}
			CBByteArray * data = CBNewByteArrayOfSize(len);
			if (! CBDatabaseRangeIteratorRead(&it, CBByteArrayGetData(data), len, 0)) {
				CBLogError("Could not read data of an unconfirmed transaction from the database.");
				CBFreeDatabaseRangeIterator(&it);
				return false;
			}
			CBTransaction * tx = CBNewTransactionFromData(data);
			CBReleaseObject(data);
			// Set hash now
			memcpy(tx->hash, key, 32);
			tx->hashSet = true;
			// Add found transaction data to node
			CBFoundTransaction * fndTx = malloc(sizeof(*fndTx));
			fndTx->utx.tx = tx;
			fndTx->utx.type = (index == storage->ourTxs) ? CB_TX_OURS : CB_TX_OTHER;
			fndTx->utx.numUnconfDeps = 0;
			fndTx->timeFound = fndTx->nextRelay = CBGetMilliseconds();
			CBInitAssociativeArray(&fndTx->dependants, CBTransactionPtrCompare, NULL, NULL);
			CBAssociativeArray * array = (index == storage->ourTxs) ? &node->ourTxs : &node->otherTxs;
			CBAssociativeArrayInsert(array, fndTx, CBAssociativeArrayFind(array, fndTx).position, NULL);
			// Get next
			res = CBDatabaseRangeIteratorNext(&it);
			if (res == CB_DATABASE_INDEX_ERROR) {
				CBLogError("Could not iterate to the next unconfirmed transaction of ours in the database.");
				CBFreeDatabaseRangeIterator(&it);
				return false;
			}
		}
		CBFreeDatabaseRangeIterator(&it);
	}
	// Loop through transactions and check dependencies
	for (CBAssociativeArray * array = &node->ourTxs;; array = &node->otherTxs) {
		CBAssociativeArrayForEach(CBFoundTransaction * fndTx, array){
			// Loop through inputs, searching for dependencies
			for (uint32_t x = 0; x < fndTx->utx.tx->inputNum; x++) {
				uint8_t * prevHash = CBByteArrayGetData(fndTx->utx.tx->inputs[x]->prevOut.hash);
				CBFoundTransaction * dep = CBNodeFullGetFoundTransaction(node, prevHash);
				if (dep != NULL) {
					// Have an unconfirmed dependency
					CBFindResult res = CBAssociativeArrayFind(&dep->dependants, fndTx);
					if (!res.found)
						// Not found, therefore add this as a dependant.
						CBAssociativeArrayInsert(&dep->dependants, fndTx, res.position, NULL);
				}else
					// Must be a chain dependency
					CBNodeFullAddToDependency(&node->chainDependencies, prevHash, fndTx, CBTransactionPtrCompare);
			}
			// Add to allChainFoundTxs if no unconfirmed dependencies
			if (fndTx->utx.numUnconfDeps == 0)
				CBAssociativeArrayInsert(&node->allChainFoundTxs, fndTx, CBAssociativeArrayFind(&node->allChainFoundTxs, fndTx).position, NULL);
		}
		if (array == &node->otherTxs)
			break;
	}
	return true;
}
bool CBNodeStorageRemoveOurTx(CBDepObject ustorage, void * tx){
	CBNodeStorage * storage = ustorage.ptr;
	if (!CBDatabaseRemoveValue(storage->ourTxs, CBTransactionGetHash((CBTransaction *)tx), false)){
		CBLogError("Could not remove our transaction from the database");
		return false;
	}
	return true;
}
bool CBNodeStorageAddOurTx(CBDepObject ustorage, void * vtx){
	CBNodeStorage * storage = ustorage.ptr;
	CBTransaction * tx = vtx;
	CBDatabaseWriteValue(storage->ourTxs, CBTransactionGetHash(tx), CBByteArrayGetData(CBGetMessage(tx)->bytes), CBGetMessage(tx)->bytes->length);
	return true;
}
bool CBNodeStorageRemoveOtherTx(CBDepObject ustorage, void * tx){
	CBNodeStorage * storage = ustorage.ptr;
	if (!CBDatabaseRemoveValue(storage->otherTxs, CBTransactionGetHash((CBTransaction *)tx), false)){
		CBLogError("Could not remove other transaction from the database");
		return false;
	}
	return true;
}
bool CBNodeStorageAddOtherTx(CBDepObject ustorage, void * vtx){
	CBNodeStorage * storage = ustorage.ptr;
	CBTransaction * tx = vtx;
	CBDatabaseWriteValue(storage->otherTxs, CBTransactionGetHash(tx), CBByteArrayGetData(CBGetMessage(tx)->bytes), CBGetMessage(tx)->bytes->length);
	return true;
}
