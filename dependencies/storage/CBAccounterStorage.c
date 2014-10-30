//
//  CBAccounterStorage.c
//  cbitcoin
//
//  Created by Matthew Mitchell on 08/02/2013.
//  Copyright (c) 2013 Matthew Mitchell
//
//  This file is part of cbitcoin. It is subject to the license terms
//  in the LICENSE file found in the top-level directory of this
//  distribution and at http://www.cbitcoin.com/license.html. No part of
//  cbitcoin, including this file, may be copied, modified, propagated,
//  or distributed except according to the terms contained in the
//  LICENSE file.

// ??? Seperate some of this code into the core library?

#include "CBAccounterStorage.h"

bool CBNewAccounterStorage(CBDepObject * storage, CBDepObject database) {
	CBAccounterStorage * self = malloc(sizeof(*self));
	self->database = database.ptr;
	uint8_t indexID = CB_INDEX_ACCOUNTER_START;
	struct{
		CBDatabaseIndex ** index; // To set
		uint8_t keySize;
		uint32_t cacheLimit;
	} indexes[14] = {
		// ??? Decide on cache limits
		{&self->accountTxDetails, 16, 10000},
		{&self->accountUnspentOutputs, 16, 10000},
		{&self->accountTimeTx, 32, 10000},
		{&self->orderNumTxDetails, 16, 10000},
		{&self->outputAccounts, 16, 10000},
		{&self->outputDetails, 8, 10000},
		{&self->outputHashAndIndexToID, 36, 10000},
		{&self->txAccounts, 16, 10000},
		{&self->txDetails, 8, 10000},
		{&self->txHashToID, 32, 10000},
		{&self->watchedHashes, 28, 10000},
		{&self->accountWatchedHashes, 28, 10000},
	};
	for (uint8_t x = 0; x < 12; x++) {
		*indexes[x].index = CBLoadIndex(self->database, indexID + x, indexes[x].keySize, indexes[x].cacheLimit);
		if (indexes[x].index == NULL) {
			for (uint8_t y = 0; y < x; y++)
				CBFreeIndex(*indexes[y].index);
			CBLogError("There was an error loading the accounter index %" PRIu8, x);
			return false;
		}
	}
	// Load data
	self->lastAccountID = CBArrayToInt64(self->database->current.extraData, CB_ACCOUNTER_DETAILS_ACCOUNT_ID);
	self->nextOutputRefID = CBArrayToInt64(self->database->current.extraData, CB_ACCOUNTER_DETAILS_OUTPUT_ID);
	self->nextTxID = CBArrayToInt64(self->database->current.extraData, CB_ACCOUNTER_DETAILS_TX_ID);
	storage->ptr = self;
	return true;
}
bool CBNewAccounterStorageTransactionCursor(CBDepObject * ucursor, CBDepObject accounter, uint64_t accountID, uint64_t timeMin, uint64_t timeMax){
	CBAccounterStorageTxCursor * self = malloc(sizeof(*self));
	CBGetAccounterCursor(self)->storage = accounter.ptr;
	CBGetAccounterCursor(self)->started = false;
	CBInt64ToArray(self->accountTimeTxMin, CB_ACCOUNT_TIME_TX_ACCOUNT_ID, accountID);
	CBInt64ToArray(self->accountTimeTxMax, CB_ACCOUNT_TIME_TX_ACCOUNT_ID, accountID);
	CBInt64ToArrayBigEndian(self->accountTimeTxMin, CB_ACCOUNT_TIME_TX_TIMESTAMP, timeMin);
	CBInt64ToArrayBigEndian(self->accountTimeTxMax, CB_ACCOUNT_TIME_TX_TIMESTAMP, timeMax);
	memset(self->accountTimeTxMin + CB_ACCOUNT_TIME_TX_ORDERNUM, 0, 16);
	memset(self->accountTimeTxMax + CB_ACCOUNT_TIME_TX_ORDERNUM, 0xFF, 16);
	ucursor->ptr = self;
	return true;
}
bool CBNewAccounterStorageUnspentOutputCursor(CBDepObject * ucursor, CBDepObject accounter, uint64_t accountID){
	CBAccounterStorageUnspentOutputCursor * self = malloc(sizeof(*self));
	CBGetAccounterCursor(self)->storage = accounter.ptr;
	CBGetAccounterCursor(self)->started = false;
	CBInt64ToArray(self->accountUnspentOutputsMin, CB_ACCOUNT_UNSPENT_OUTPUTS_ACCOUNT_ID, accountID);
	CBInt64ToArray(self->accountUnspentOutputsMax, CB_ACCOUNT_UNSPENT_OUTPUTS_ACCOUNT_ID, accountID);
	memset(self->accountUnspentOutputsMin + CB_ACCOUNT_UNSPENT_OUTPUTS_OUTPUT_ID, 0, 12);
	memset(self->accountUnspentOutputsMax + CB_ACCOUNT_UNSPENT_OUTPUTS_OUTPUT_ID, 0xFF, 12);
	ucursor->ptr = self;
	return true;
}
void CBFreeAccounterStorage(CBDepObject self){
	CBAccounterStorage * storage = self.ptr;
	CBFreeIndex(storage->accountTxDetails);
	CBFreeIndex(storage->orderNumTxDetails);
	CBFreeIndex(storage->outputAccounts);
	CBFreeIndex(storage->outputDetails);
	CBFreeIndex(storage->outputHashAndIndexToID);
	CBFreeIndex(storage->txAccounts);
	CBFreeIndex(storage->txDetails);
	CBFreeIndex(storage->txHashToID);
	CBFreeIndex(storage->watchedHashes);
	CBFreeIndex(storage->accountWatchedHashes);
	CBFreeIndex(storage->accountUnspentOutputs);
	CBFreeIndex(storage->accountTimeTx);
	free(self.ptr);
}
void CBFreeAccounterCursor(CBDepObject self){
	CBFreeDatabaseRangeIterator(&CBGetAccounterCursor(self.ptr)->it);
	free(self.ptr);
}
bool CBAccounterAddWatchedOutputToAccount(CBDepObject self, uint8_t * hash, uint64_t accountID){
	CBAccounterStorage * storage = self.ptr;
	uint8_t watchedHashKey[28];
	memcpy(watchedHashKey + CB_WATCHED_HASHES_HASH, hash, 20);
	CBInt64ToArray(watchedHashKey, CB_WATCHED_HASHES_ACCOUNT_ID, accountID);
	CBDatabaseWriteValue(storage->watchedHashes, watchedHashKey, NULL, 0);
	memcpy(watchedHashKey + CB_ACCOUNT_WATCHED_HASHES_HASH, hash, 20);
	CBInt64ToArray(watchedHashKey, CB_ACCOUNT_WATCHED_HASHES_ACCOUNT_ID, accountID);
	CBDatabaseWriteValue(storage->accountWatchedHashes, watchedHashKey, NULL, 0);
	return true;
}
bool CBAccounterAdjustBalances(CBAccounterStorage * storage, uint8_t * low, uint8_t * high, uint64_t accountID, int64_t adjustment, int64_t unconfAdjustment, uint64_t * last, int64_t * unconfLast){
	uint8_t accountTimeTxMin[32];
	uint8_t accountTimeTxMax[32];
	CBInt64ToArray(accountTimeTxMin, CB_ACCOUNT_TIME_TX_ACCOUNT_ID, accountID);
	CBInt64ToArray(accountTimeTxMax, CB_ACCOUNT_TIME_TX_ACCOUNT_ID, accountID);
	memcpy(accountTimeTxMin + CB_ACCOUNT_TIME_TX_TIMESTAMP, low, 16);
	memcpy(accountTimeTxMax + CB_ACCOUNT_TIME_TX_TIMESTAMP, high, 16);
	memset(accountTimeTxMin + CB_ACCOUNT_TIME_TX_TX_ID, 0, 8);
	memset(accountTimeTxMax + CB_ACCOUNT_TIME_TX_TX_ID, 0xFF, 8);
	CBDatabaseRangeIterator it;
	CBInitDatabaseRangeIterator(&it, accountTimeTxMin, accountTimeTxMax, storage->accountTimeTx);
	if (adjustment != 0 || unconfAdjustment != 0) {
		CBIndexFindStatus status = CBDatabaseRangeIteratorFirst(&it);
		if (status == CB_DATABASE_INDEX_ERROR) {
			CBLogError("There was a problem finding the first branch section account time tx details.");
			CBFreeDatabaseRangeIterator(&it);
			return false;
		}
		if (status == CB_DATABASE_INDEX_FOUND) for (;;) {
			// Get the data
			uint8_t data[16];
			if (!CBDatabaseRangeIteratorRead(&it, data, 16, 0)) {
				CBLogError("Could not read the cumulative balance for adjustment");
				CBFreeDatabaseRangeIterator(&it);
				return false;
			}
			// Adjust balances
			uint64_t adjustedBal = CBArrayToInt64(data, CB_ACCOUNT_TIME_TX_BALANCE) + adjustment;
			CBInt64ToArray(data, CB_ACCOUNT_TIME_TX_BALANCE, adjustedBal);
			if (last)
				*last = adjustedBal;
			int64_t unconfAdjustedBal;
			CBAccounterUInt64ToInt64(CBArrayToInt64(data, CB_ACCOUNT_TIME_TX_UNCONF_BALANCE), &unconfAdjustedBal);
			unconfAdjustedBal += unconfAdjustment;
			CBInt64ToArray(data, CB_ACCOUNT_TIME_TX_UNCONF_BALANCE, CBAccounterInt64ToUInt64(unconfAdjustedBal));
			if (unconfLast)
				*unconfLast = unconfAdjustedBal;
			// Write adjusted balances
			uint8_t key[33];
			CBDatabaseRangeIteratorGetKey(&it, key);
			CBDatabaseWriteValue(storage->accountTimeTx, key, data, 16);
			// Go to next if possible.
			CBIndexFindStatus status = CBDatabaseRangeIteratorNext(&it);
			if (status == CB_DATABASE_INDEX_ERROR) {
				CBLogError("Error whilst iterating through branch section account time tx details.");
				CBFreeDatabaseRangeIterator(&it);
				return false;
			}
			if (status == CB_DATABASE_INDEX_NOT_FOUND)
				break;
		}
	}else{
		// Adjustment is zero so do not adjust but get the last balances as they are
		CBIndexFindStatus status = CBDatabaseRangeIteratorLast(&it);
		if (status == CB_DATABASE_INDEX_ERROR) {
			CBLogError("There was a problem finding the first branch section account time tx details.");
			CBFreeDatabaseRangeIterator(&it);
			return false;
		}
		if (status == CB_DATABASE_INDEX_FOUND){
			uint8_t data[16];
			if (!CBDatabaseRangeIteratorRead(&it, data, 16, 0)) {
				CBLogError("Could not read the cumulative balance for adjustment");
				CBFreeDatabaseRangeIterator(&it);
				return false;
			}
			if (last)
				*last = CBArrayToInt64(data, CB_ACCOUNT_TIME_TX_BALANCE);
			if (unconfLast)
				*unconfLast = CBArrayToInt64(data, CB_ACCOUNT_TIME_TX_UNCONF_BALANCE);
		}
		// Else do not change last
	}
	CBFreeDatabaseRangeIterator(&it);
	return true;
}
bool CBAccounterTransactionChangeHeight(CBDepObject self, void * vtx, uint32_t oldHeight, uint32_t newHeight){
	CBAccounterStorage * storage = self.ptr;
	CBTransaction * tx = vtx;
	uint8_t data[8];
	// A previously unconfirmed transaction has been found on the chain
	if (CBDatabaseReadValue(storage->txHashToID, CBTransactionGetHash(tx), data, 8, 0, false) != CB_DATABASE_INDEX_FOUND) {
		CBLogError("Could not get a transaction ID for adding to a branch.");
		return false;
	}
	uint64_t txID = CBArrayToInt64(data, 0);
	// Get the last order num key
	uint8_t orderNumTxDetailsMin[16], orderNumTxDetailsMax[16];
	memcpy(orderNumTxDetailsMin + CB_ORDER_NUM_TX_DETAILS_TX_ID, data, 8);
	memcpy(orderNumTxDetailsMax + CB_ORDER_NUM_TX_DETAILS_TX_ID, data, 8);
	memset(orderNumTxDetailsMin + CB_ORDER_NUM_TX_DETAILS_ORDER_NUM, 0, 8);
	memset(orderNumTxDetailsMax + CB_ORDER_NUM_TX_DETAILS_ORDER_NUM, 0xFF, 8);
	CBDatabaseRangeIterator it;
	CBInitDatabaseRangeIterator(&it, orderNumTxDetailsMin, orderNumTxDetailsMax, storage->orderNumTxDetails);
	if (CBDatabaseRangeIteratorLast(&it) != CB_DATABASE_INDEX_FOUND){
		CBLogError("Could not get the last order number for a transaction.");
		CBFreeDatabaseRangeIterator(&it);
		return false;
	}
	// Get the key
	CBDatabaseRangeIteratorGetKey(&it, orderNumTxDetailsMax);
	CBFreeDatabaseRangeIterator(&it);
	// Change the height in the ordered details.
	CBInt32ToArrayBigEndian(data, CB_ORDER_NUM_TX_DETAILS_HEIGHT, newHeight);
	CBDatabaseWriteValue(storage->orderNumTxDetails, orderNumTxDetailsMax, data, 4);
	// Loop through transaction accounts and adjust unconfirmed balance if we are moving from or to unconfirmed
	if (oldHeight == CB_UNCONFIRMED || newHeight == CB_UNCONFIRMED) {
		uint8_t txAccountMin[16];
		uint8_t txAccountMax[16];
		CBInt64ToArray(txAccountMin, CB_TX_ACCOUNTS_TX_ID, txID);
		CBInt64ToArray(txAccountMax, CB_TX_ACCOUNTS_TX_ID, txID);
		memset(txAccountMin + CB_TX_ACCOUNTS_ACCOUNT_ID, 0, 8);
		memset(txAccountMax + CB_TX_ACCOUNTS_ACCOUNT_ID, 0xFF, 8);
		CBInitDatabaseRangeIterator(&it, txAccountMin, txAccountMax, storage->txAccounts);
		if (CBDatabaseRangeIteratorFirst(&it) != CB_DATABASE_INDEX_FOUND) {
			CBLogError("Couldn't find first transaction account data when looping through for a transaction being added to a branch.");
			CBFreeDatabaseRangeIterator(&it);
			return false;
		}
		for (;;) {
			// Get the transaction account key
			uint8_t key[16];
			CBDatabaseRangeIteratorGetKey(&it, key);
			// Get account ID
			uint64_t accountID = CBArrayToInt64(key, CB_TX_ACCOUNTS_ACCOUNT_ID);
			// Get the transaction value
			int64_t value;
			if (! CBAccounterGetTxAccountValue(storage, txID, accountID, &value)) {
				CBLogError("Could not get the value of a transaction for an account.");
				CBFreeDatabaseRangeIterator(&it);
				return false;
			}
			// Get the timestamp and orderNum
			uint8_t timestampAndOrderNum[16];
			if (CBDatabaseReadValue(storage->txDetails, key + CB_TX_ACCOUNTS_TX_ID, timestampAndOrderNum, 8, CB_TX_DETAILS_TIMESTAMP, false) != CB_DATABASE_INDEX_FOUND) {
				CBLogError("Could not read a transaction's timestamp when changing the height");
				CBFreeDatabaseRangeIterator(&it);
				return false;
			}
			// Get the last order number of the transaction
			uint8_t orderNumTxDetailsMin[16], orderNumTxDetailsMax[16];
			memcpy(orderNumTxDetailsMin + CB_ORDER_NUM_TX_DETAILS_TX_ID, key + CB_TX_ACCOUNTS_TX_ID, 8);
			memcpy(orderNumTxDetailsMax + CB_ORDER_NUM_TX_DETAILS_TX_ID, key + CB_TX_ACCOUNTS_TX_ID, 8);
			memset(orderNumTxDetailsMin + CB_ORDER_NUM_TX_DETAILS_ORDER_NUM, 0, 8);
			memset(orderNumTxDetailsMax + CB_ORDER_NUM_TX_DETAILS_ORDER_NUM, 0xFF, 8);
			CBDatabaseRangeIterator it2;
			CBInitDatabaseRangeIterator(&it2, orderNumTxDetailsMin, orderNumTxDetailsMax, storage->orderNumTxDetails);
			if (CBDatabaseRangeIteratorLast(&it2) != CB_DATABASE_INDEX_FOUND){
				CBLogError("Could not get the last order number for a transaction.");
				CBFreeDatabaseRangeIterator(&it);
				CBFreeDatabaseRangeIterator(&it2);
				return false;
			}
			// Get the key and thus the order number
			CBDatabaseRangeIteratorGetKey(&it2, orderNumTxDetailsMax);
			CBFreeDatabaseRangeIterator(&it2);
			memcpy(timestampAndOrderNum + 8, orderNumTxDetailsMax + CB_ORDER_NUM_TX_DETAILS_ORDER_NUM, 8);

			// Adjust the unconf balances.

			uint8_t high[16];
			memset(high, 0xFF, 16);

			if (!CBAccounterAdjustBalances(storage, timestampAndOrderNum, high, accountID, 0, oldHeight == CB_UNCONFIRMED ? -value : value, NULL, NULL)) {
				CBLogError("Unable to adjust unconfirmed cumulative balances.");
				CBFreeDatabaseRangeIterator(&it);
				return false;
			}

			// Iterate if possible
			CBIndexFindStatus status = CBDatabaseRangeIteratorNext(&it);
			if (status == CB_DATABASE_INDEX_ERROR) {
				CBLogError("Error whilst iterating through transaction accounts.");
				CBFreeDatabaseRangeIterator(&it);
				return false;
			}
			if (status == CB_DATABASE_INDEX_NOT_FOUND)
				break;
		}
		CBFreeDatabaseRangeIterator(&it);
	}
	return true;
}
bool CBAccounterMakeOutputSpent(CBAccounterStorage * self, CBPrevOut * prevOut, uint32_t blockHeight, CBAssociativeArray * txInfo){
	uint8_t keyArr[36], data[8];
	// Make the output hash and index to ID key
	memcpy(keyArr + CB_OUTPUT_HASH_AND_INDEX_TO_ID_HASH, CBByteArrayGetData(prevOut->hash), 32);
	CBInt32ToArray(keyArr, CB_OUTPUT_HASH_AND_INDEX_TO_ID_INDEX, prevOut->index);
	CBIndexFindStatus stat = CBDatabaseReadValue(self->outputHashAndIndexToID, keyArr, data, 8, 0, false);
	if (stat == CB_DATABASE_INDEX_ERROR) {
		CBLogError("Could not read the ID of an output reference.");
		return false;
	}
	if (stat == CB_DATABASE_INDEX_NOT_FOUND)
		// The output reference does not exist, so we can end here.
		return true;
	// For each account that owns the output remove from the unspent outputs.
	uint8_t minOutputAccountKey[16];
	uint8_t maxOutputAccountKey[16];
	memcpy(minOutputAccountKey + CB_OUTPUT_ACCOUNTS_OUTPUT_ID, data, 8);
	memcpy(maxOutputAccountKey + CB_OUTPUT_ACCOUNTS_OUTPUT_ID, data, 8);
	memset(minOutputAccountKey + CB_OUTPUT_ACCOUNTS_ACCOUNTS_ID, 0, 8);
	memset(maxOutputAccountKey + CB_OUTPUT_ACCOUNTS_ACCOUNTS_ID, 0xFF, 8);
	CBDatabaseRangeIterator it;
	CBInitDatabaseRangeIterator(&it, minOutputAccountKey, maxOutputAccountKey, self->outputAccounts);
	if (CBDatabaseRangeIteratorFirst(&it) != CB_DATABASE_INDEX_FOUND) {
		CBLogError("Couldn't get the first output account entry");
		CBFreeDatabaseRangeIterator(&it);
		return false;
	}
	uint8_t outputIDBuf[8];
	memcpy(outputIDBuf, data, 8);
	for (;;) {
		// Get the ouput account key
		uint8_t outputAccountKey[16];
		CBDatabaseRangeIteratorGetKey(&it, outputAccountKey);
		// Create the account unspent output key
		memcpy(keyArr + CB_ACCOUNT_UNSPENT_OUTPUTS_ACCOUNT_ID, outputAccountKey + CB_OUTPUT_ACCOUNTS_ACCOUNTS_ID, 8);
		memcpy(keyArr + CB_ACCOUNT_UNSPENT_OUTPUTS_OUTPUT_ID, outputIDBuf, 8);
		// Delete unspent output
		if (!CBDatabaseRemoveValue(self->accountUnspentOutputs, keyArr, false)){
			CBLogError("Could not remove unspent output from accounter.");
			CBFreeDatabaseRangeIterator(&it);
			return false;
		}
		// Add txInfo if not NULL
		if (txInfo){
			// Add the account outflow information
			// Get the output value
			memcpy(keyArr, outputIDBuf, 8);
			if (CBDatabaseReadValue(self->outputDetails, keyArr, data, 8, CB_OUTPUT_DETAILS_VALUE, false) != CB_DATABASE_INDEX_FOUND) {
				CBLogError("Could not read the value of an output.");
				CBFreeDatabaseRangeIterator(&it);
				return false;
			}
			uint64_t value = CBArrayToInt64(data, 0);
			// First look to see if the accountID exists in the transaction flow information
			uint64_t accountID = CBArrayToInt64(outputAccountKey, CB_OUTPUT_ACCOUNTS_ACCOUNTS_ID);
			CBFindResult res = CBAssociativeArrayFind(txInfo, &accountID);
			if (res.found) {
				// Add to the outflow amount
				CBTransactionAccountFlow * info = CBFindResultToPointer(res);
				info->outAmount += value;
			}else{
				// Add new element
				CBTransactionAccountFlow * info = malloc(sizeof(*info));
				info->accountID = accountID;
				info->inAmount = 0;
				info->outAmount = value;
				info->foundInAddr = false;
				// Insert new details
				CBAssociativeArrayInsert(txInfo, info, CBAssociativeArrayFind(txInfo, info).position, NULL);
			}
		}
		// Iterate if possible
		CBIndexFindStatus status = CBDatabaseRangeIteratorNext(&it);
		if (status == CB_DATABASE_INDEX_ERROR) {
			CBLogError("Error whilst iterating through output accounts.");
			CBFreeDatabaseRangeIterator(&it);
			return false;
		}
		if (status == CB_DATABASE_INDEX_NOT_FOUND)
			break;
	}
	CBFreeDatabaseRangeIterator(&it);
	return true;
}
CBErrBool CBAccounterFoundTransaction(CBDepObject self, void * vtx, uint32_t blockHeight, uint64_t time, CBTransactionAccountDetailList ** details){
	CBAccounterStorage * storage = self.ptr;
	CBTransaction * tx = vtx;
	CBTransactionAccountDetailList ** curDetails = details;
	uint8_t keyArr[33], data[64];
	// Create transaction flow information array.
	CBAssociativeArray txInfo;
	CBInitAssociativeArray(&txInfo, CBCompareUInt32, NULL, free);
	// Look through inputs for any unspent outputs being spent.
	for (uint32_t x = 0; x < tx->inputNum; x++) {
		// Make previous output spent for this branch.
		if (! CBAccounterMakeOutputSpent(storage, &tx->inputs[x]->prevOut, blockHeight, &txInfo)) {
			CBLogError("Could not change the status of an output to spent.");
			return CB_ERROR;
		}
	}
	// Scan for watched addresses.
	// Set the transaction hash into the "hash and index to ID key" now to prevent redoing it.
	uint8_t outputHashAndIndexToIDKey[36];
	memcpy(outputHashAndIndexToIDKey + CB_OUTPUT_HASH_AND_INDEX_TO_ID_HASH, CBTransactionGetHash(tx), 32);
	for (uint32_t x = 0; x < tx->outputNum; x++) {
		// Try to obtain an address (or multisig script hash) from this output
		// Get the identifying hash of the output of output
		uint8_t hash[32]; // 32 for mutlisig hashing
		if (CBTransactionOuputGetHash(tx->outputs[x], hash))
			continue;
		// Now look through accounts watching this output.
		uint8_t minWatchedHashKey[28];
		uint8_t maxWatchedHashKey[28];
		memcpy(minWatchedHashKey + CB_WATCHED_HASHES_HASH, hash, 20);
		memcpy(maxWatchedHashKey + CB_WATCHED_HASHES_HASH, hash, 20);
		memset(minWatchedHashKey + CB_WATCHED_HASHES_ACCOUNT_ID, 0, 8);
		memset(maxWatchedHashKey + CB_WATCHED_HASHES_ACCOUNT_ID, 0xFF, 8);
		CBDatabaseRangeIterator it;
		CBInitDatabaseRangeIterator(&it, minWatchedHashKey, maxWatchedHashKey, storage->watchedHashes);
		CBIndexFindStatus res = CBDatabaseRangeIteratorFirst(&it);
		if (res == CB_DATABASE_INDEX_ERROR) {
			CBLogError("There was an error whilst attempting to find the first account watching an output hash.");
			CBFreeDatabaseRangeIterator(&it);
			return CB_ERROR;
		}
		if (res == CB_DATABASE_INDEX_FOUND){
			uint64_t outputID;
			// Add the index to the "hash and index to ID" key
			CBInt32ToArray(outputHashAndIndexToIDKey, CB_OUTPUT_HASH_AND_INDEX_TO_ID_INDEX, x);
			// Detemine if the output exists already
			uint32_t len;
			if (! CBDatabaseGetLength(storage->outputHashAndIndexToID, outputHashAndIndexToIDKey, &len)) {
				CBLogError("Unable to obtain the length for a outputHashAndIndexToID entry when found a watched hash.");
				CBFreeDatabaseRangeIterator(&it);
				return CB_ERROR;
			}
			if (len == CB_DOESNT_EXIST) {
				// Doesn't exist, get the next ID
				outputID = storage->nextOutputRefID;
				// Save the output details
				CBInt64ToArray(keyArr, 0, outputID);
				CBInt64ToArray(data, CB_OUTPUT_DETAILS_VALUE, tx->outputs[x]->value);
				memcpy(data + CB_OUTPUT_DETAILS_TX_HASH, CBTransactionGetHash(tx), 32);
				CBInt32ToArray(data, CB_OUTPUT_DETAILS_INDEX, x);
				memcpy(data + CB_OUTPUT_DETAILS_ID_HASH, hash, 20);
				CBDatabaseWriteValue(storage->outputDetails, keyArr, data, 64);
				// Save the output hash and index to ID entry
				CBInt64ToArray(data, 0, outputID);
				CBDatabaseWriteValue(storage->outputHashAndIndexToID, outputHashAndIndexToIDKey, data, 8);
				// Increment the next output reference ID and save it.
				storage->nextOutputRefID++;
				CBInt64ToArray(storage->database->current.extraData, CB_ACCOUNTER_DETAILS_OUTPUT_ID, storage->nextOutputRefID);
			}else{
				// Get the ID of this output
				if (CBDatabaseReadValue(storage->outputHashAndIndexToID, outputHashAndIndexToIDKey, data, 8, 0, false) != CB_DATABASE_INDEX_FOUND) {
					CBLogError("Could not read the output hash and index to ID entry.");
					CBFreeDatabaseRangeIterator(&it);
					return CB_ERROR;
				}
				outputID = CBArrayToInt64(data, 0);
			}
			// Now loop through accounts. Set outputID to the output accounts key and the outputId to the account unspent outputs key so that we don't keep doing it.
			uint8_t outputAccountsKey[16];
			uint8_t accountUnspentOutputsKey[20];
			CBInt64ToArray(outputAccountsKey, CB_OUTPUT_ACCOUNTS_OUTPUT_ID, outputID);
			CBInt64ToArray(accountUnspentOutputsKey, CB_ACCOUNT_UNSPENT_OUTPUTS_OUTPUT_ID, outputID);
			for (;;) {
				// Get accountID
				uint8_t key[28];
				CBDatabaseRangeIteratorGetKey(&it, key);
				uint64_t accountID = CBArrayToInt64(key, CB_WATCHED_HASHES_ACCOUNT_ID);
				// Detemine if the output account details exist alredy or not.
				CBInt64ToArray(outputAccountsKey, CB_OUTPUT_ACCOUNTS_ACCOUNTS_ID, accountID);
				if (! CBDatabaseGetLength(storage->outputAccounts, outputAccountsKey, &len)) {
					CBLogError("Unable to obtain the length for an outputAccounts entry to see if one should be made.");
					CBFreeDatabaseRangeIterator(&it);
					return CB_ERROR;
				}
				if (len == CB_DOESNT_EXIST)
					// Add an entry to the output account information.
					CBDatabaseWriteValue(storage->outputAccounts, outputAccountsKey, NULL, 0);
				// Add an entry to the unspent outputs for the account
				CBInt64ToArray(accountUnspentOutputsKey, CB_ACCOUNT_UNSPENT_OUTPUTS_ACCOUNT_ID, accountID);
				CBDatabaseWriteValue(storage->accountUnspentOutputs, accountUnspentOutputsKey, NULL, 0);
				// Add the output value to the inflow amount for this transaction
				CBFindResult res = CBAssociativeArrayFind(&txInfo, &accountID);
				CBTransactionAccountFlow * details;
				if (res.found) {
					details = CBFindResultToPointer(res);
					details->inAmount += tx->outputs[x]->value;
				}else{
					// Create new details
					details = malloc(sizeof(*details));
					details->accountID = accountID;
					details->inAmount = tx->outputs[x]->value;
					details->outAmount = 0;
					details->foundInAddr = false;
					// Insert new details
					CBAssociativeArrayInsert(&txInfo, details, CBAssociativeArrayFind(&txInfo, details).position, NULL);
				}
				if (! details->foundInAddr) {
					// Add this address as an inflow address
					details->foundInAddr = true;
					details->inAddrIndexIsZero = ! x;
					memcpy(details->inAddr, hash, 20);
				}
				// Iterate if possible
				CBIndexFindStatus status = CBDatabaseRangeIteratorNext(&it);
				if (status == CB_DATABASE_INDEX_ERROR) {
					CBLogError("Error whilst iterating through watched hashes.");
					CBFreeDatabaseRangeIterator(&it);
					return CB_ERROR;
				}
				if (status == CB_DATABASE_INDEX_NOT_FOUND)
					break;
			}
		}
		CBFreeDatabaseRangeIterator(&it);
	}
	// Loop through all accounts seeing what flows have been made
	CBPosition pos;
	if (CBAssociativeArrayGetFirst(&txInfo, &pos)){
		uint64_t txID;
		memcpy(keyArr, CBTransactionGetHash(tx), 32);
		// Determine if the transaction exists already.
		uint32_t len;
		if (! CBDatabaseGetLength(storage->txHashToID, keyArr, &len)) {
			CBLogError("Could not get length of txHashToID to see if transaction extists.");
			return CB_ERROR;
		}
		if (len == CB_DOESNT_EXIST) {
			// It doesn't exist, get the next ID.
			txID = storage->nextTxID;
			// Create the transaction hash to ID entry
			CBInt64ToArray(data, 0, txID);
			CBDatabaseWriteValue(storage->txHashToID, keyArr, data, 8);
			// Create the transaction details. Copy the txID from CB_DATA_ARRAY.
			memcpy(keyArr, data, 8);
			data[CB_TX_DETAILS_INSTANCES] = 1;
			memcpy(data + CB_TX_DETAILS_HASH, CBTransactionGetHash(tx), 32);
			CBInt64ToArrayBigEndian(data, CB_TX_DETAILS_TIMESTAMP, time);
			CBDatabaseWriteValue(storage->txDetails, keyArr, data, 41);
			// Increment the next ID
			storage->nextTxID++;
			CBInt64ToArray(storage->database->current.extraData, CB_ACCOUNTER_DETAILS_TX_ID, storage->nextTxID);
		}else{
			// Obtain the transaction ID.
			if (CBDatabaseReadValue(storage->txHashToID, keyArr, data, 8, 0, false) != CB_DATABASE_INDEX_FOUND) {
				CBLogError("Couldn't read a transaction ID.");
				return CB_ERROR;
			}
			txID = CBArrayToInt64(data, 0);
			// Increase the number of instances
			CBInt64ToArray(keyArr, 0, txID);
			if (CBDatabaseReadValue(storage->txDetails, keyArr, data, 1, CB_TX_DETAILS_INSTANCES, false) != CB_DATABASE_INDEX_FOUND) {
				CBLogError("Couldn't read the instance count for a transaction.");
				return CB_ERROR;
			}
			data[0]++;
			CBDatabaseWriteValueSubSection(storage->txDetails, keyArr, data, 1, CB_TX_DETAILS_INSTANCES);
			// Get time
			if (CBDatabaseReadValue(storage->txDetails, keyArr, data, 8, CB_TX_DETAILS_TIMESTAMP, false) != CB_DATABASE_INDEX_FOUND) {
				CBLogError("Could not read a transaction's timestamp when getting a transaction time.");
				return false;
			}
			time = CBArrayToInt64BigEndian(data, 0);
		}
		uint8_t * nextOrderNumData;
		// Get the next order num
		nextOrderNumData = storage->database->current.extraData + CB_ACCOUNTER_DETAILS_ORDER_NUM;
		// Increment the next order num
		CBInt64ToArrayBigEndian(storage->database->current.extraData, CB_ACCOUNTER_DETAILS_ORDER_NUM,  CBArrayToInt64BigEndian(nextOrderNumData, 0) + 1);
		// Write the value
		// Set the block height for this ordered transaction
		CBInt64ToArray(keyArr, CB_ORDER_NUM_TX_DETAILS_TX_ID, txID);
		memcpy(keyArr + CB_ORDER_NUM_TX_DETAILS_ORDER_NUM, nextOrderNumData, 8);
		CBInt32ToArrayBigEndian(data, CB_ORDER_NUM_TX_DETAILS_HEIGHT, blockHeight);
		CBDatabaseWriteValue(storage->orderNumTxDetails, keyArr, data, 4);
		// Add txID to the txAccount and accountTxDetails keys in advance
		uint8_t txAccountKey[16];
		uint8_t accountTxDetailsKey[16];
		CBInt64ToArray(txAccountKey, CB_TX_ACCOUNTS_TX_ID, txID);
		CBInt64ToArray(accountTxDetailsKey, CB_ACCOUNT_TX_DETAILS_TX_ID, txID);
		#define CBFoundTransactionReturnError \
			for (CBTransactionAccountDetailList ** x = details, ** temp; *x != NULL; x = temp) { \
				temp = &(*x)->next; \
				free(x); \
			}\
			return CB_ERROR;
		for (;;) {
			// Get the transaction information for an account
			CBTransactionAccountFlow * info = pos.node->elements[pos.index];
			if (curDetails) {
				// Write details to the linked list
				*curDetails = malloc(sizeof(**curDetails));
				(*curDetails)->accountID = info->accountID;
				(*curDetails)->next = NULL;
			}
			// Add the account ID to the txAccount key
			CBInt64ToArray(txAccountKey, CB_TX_ACCOUNTS_ACCOUNT_ID, info->accountID);
			CBInt64ToArray(accountTxDetailsKey, CB_ACCOUNT_TX_DETAILS_ACCOUNT_ID, info->accountID);
			// Determine if the transaction account details exist already.
			uint32_t len;
			if (! CBDatabaseGetLength(storage->txAccounts, txAccountKey, &len)) {
				CBLogError("Unable to get the length of a txAccount entry to see if it exists");
				CBFoundTransactionReturnError
			}
			int64_t valueInt;
			if (len == CB_DOESNT_EXIST){
				// Create the transaction account entry
				CBDatabaseWriteValue(storage->txAccounts, txAccountKey, NULL, 0);
				// Set the value
				valueInt = info->inAmount - info->outAmount;
				CBInt64ToArray(data, CB_ACCOUNT_TX_DETAILS_VALUE, CBAccounterInt64ToUInt64(valueInt));
				// Set the address.
				if (valueInt > 0)
					// Use inflow address
					memcpy(data + CB_ACCOUNT_TX_DETAILS_ADDR, info->inAddr, 20);
				else{
					// Find a outflow address if there is one we can use.
					if (! info->foundInAddr || ! info->inAddrIndexIsZero) {
						// Try index 0, which should be another address.
						if (CBTransactionOuputGetHash(tx->outputs[0], data + CB_ACCOUNT_TX_DETAILS_ADDR))
							// The output is not supported, use zero to represent this.
							memset(data + CB_ACCOUNT_TX_DETAILS_ADDR, 0, 20);
					}else{
						// Else we have found a inflow address of ours and it is at the zero index
						if (tx->outputNum == 1)
							// No other outputs, must be wasting money to miners, or transferring money to ourself.
							memset(data + CB_ACCOUNT_TX_DETAILS_ADDR, 0, 20);
						// Try index 1 for another address
						else if (CBTransactionOuputGetHash(tx->outputs[1], data + CB_ACCOUNT_TX_DETAILS_ADDR))
							memset(data + CB_ACCOUNT_TX_DETAILS_ADDR, 0, 20);
					}
				}
				// Now save the account's transaction details
				CBDatabaseWriteValue(storage->accountTxDetails, accountTxDetailsKey, data, 28);
				if (curDetails)
					// Set the address in the account transaction details list.
					memcpy((*curDetails)->accountTxDetails.addrHash, data + CB_ACCOUNT_TX_DETAILS_ADDR, 20);
			}else{
				// Get the value of the transaction
				if (! CBAccounterGetTxAccountValue(storage, txID, info->accountID, &valueInt)){
					CBLogError("Could not obtain the value of a transaction.");
					CBFoundTransactionReturnError
				}
				// Get the address for the details
				if (curDetails && CBDatabaseReadValue(storage->accountTxDetails, accountTxDetailsKey, (*curDetails)->accountTxDetails.addrHash, 20, CB_ACCOUNT_TX_DETAILS_ADDR, false) != CB_DATABASE_INDEX_FOUND) {
					CBLogError("Could not read the address of a transaction for an account.");
					CBFoundTransactionReturnError
				}
			}
			if (curDetails) {
				(*curDetails)->accountTxDetails.amount = valueInt;
				// Finished adding data for the account.
				curDetails = &(*curDetails)->next;
			}
			// Else write the cumulative balances for the transaction and get the next order number
			uint64_t balance;
			int64_t unconfBalance;
			// Get balance upto this transaction
			if (! CBAccounterGetLastAccountBalance(storage, info->accountID, time, &balance, &unconfBalance)) {
				CBLogError("Could not read the cumulative balance for a transaction.");
				CBFoundTransactionReturnError
			}
			// Set the order num into the key array for the branch account time transaction entry.
			memcpy(keyArr + CB_ACCOUNT_TIME_TX_ORDERNUM, nextOrderNumData, 8);
			// Set balances into data array
			balance += valueInt;
			if (blockHeight == CB_UNCONFIRMED)
				unconfBalance += valueInt;
			CBInt64ToArray(data, CB_ACCOUNT_TIME_TX_BALANCE, balance);
			CBInt64ToArray(data, CB_ACCOUNT_TIME_TX_UNCONF_BALANCE, CBAccounterInt64ToUInt64(unconfBalance));
			// Create the branch account time transaction entry.
			CBInt64ToArray(keyArr, CB_ACCOUNT_TIME_TX_ACCOUNT_ID, info->accountID);
			CBInt64ToArrayBigEndian(keyArr, CB_ACCOUNT_TIME_TX_TIMESTAMP, time);
			CBInt64ToArray(keyArr, CB_ACCOUNT_TIME_TX_TX_ID, txID);
			CBDatabaseWriteValue(storage->accountTimeTx, keyArr, data, 16);
			// Adjust the balances of any future transactions already stored
			CBInt64ToArrayBigEndian(keyArr, CB_ACCOUNT_TIME_TX_ORDERNUM, CBArrayToInt64BigEndian(keyArr, CB_ACCOUNT_TIME_TX_ORDERNUM) + 1);
			uint8_t high[16];
			memset(high, 0xFF, 16);
			if (!CBAccounterAdjustBalances(storage, keyArr + CB_ACCOUNT_TIME_TX_TIMESTAMP, high, info->accountID, valueInt, (blockHeight == CB_UNCONFIRMED)*valueInt, NULL, NULL)) {
				CBLogError("Could not adjust the balances of transactions after one inserted.");
				return false;
			}
			// Get the next transaction account information.
			if (CBAssociativeArrayIterate(&txInfo, &pos))
				break;
		}
	}else{
		// Not got any transactions for us.
		CBFreeAssociativeArray(&txInfo);
		return CB_FALSE;
	}
	CBFreeAssociativeArray(&txInfo);
	return CB_TRUE;
}
bool CBAccounterGetAccountBalance(CBDepObject self, uint64_t accountID, uint64_t * balance, int64_t * unconfBalance){
	return CBAccounterGetLastAccountBalance(self.ptr, accountID, UINT64_MAX, balance, unconfBalance);
}
CBErrBool CBAccounterGetNextTransaction(CBDepObject cursoru, CBTransactionDetails * details){
	CBAccounterStorageTxCursor * txCursor = cursoru.ptr;
	CBAccounterStorageCursor * cursor = CBGetAccounterCursor(txCursor);
	CBIndexFindStatus res;
	if (cursor->started)
		res = CBDatabaseRangeIteratorNext(&cursor->it);
	else{
		CBInitDatabaseRangeIterator(&cursor->it, txCursor->accountTimeTxMin, txCursor->accountTimeTxMax, cursor->storage->accountTimeTx);
		res = CBDatabaseRangeIteratorFirst(&cursor->it);
		cursor->started = true;
	}
	if (res == CB_DATABASE_INDEX_ERROR) {
		CBLogError("There was an error trying to find the first transaction for a transaction cursor.");
		return CB_ERROR;
	}
	if (res != CB_DATABASE_INDEX_FOUND)
		return CB_FALSE;
	// Get the key
	uint8_t key[33], keyArr[17], data[16];
	CBDatabaseRangeIteratorGetKey(&cursor->it, key);
	// Read the cumulative balance
	if (! CBDatabaseRangeIteratorRead(&cursor->it, data, 16, 0)) {
		CBLogError("Could not read the cumulative balance of an account's transaction.");
		return CB_ERROR;
	}
	details->cumBalance = CBArrayToInt64(data, CB_ACCOUNT_TIME_TX_BALANCE);
	CBAccounterUInt64ToInt64(CBArrayToInt64(data, CB_ACCOUNT_TIME_TX_UNCONF_BALANCE), &details->cumUnconfBalance);
	// Get the transaction hash
	memcpy(keyArr, key + CB_ACCOUNT_TIME_TX_TX_ID, 8);
	if (CBDatabaseReadValue(cursor->storage->txDetails, keyArr, details->txHash, 32, CB_TX_DETAILS_HASH, false) != CB_DATABASE_INDEX_FOUND) {
		CBLogError("Could not read a transaction's hash.");
		return CB_ERROR;
	}
	// Get the address and value
	memcpy(keyArr + CB_ACCOUNT_TX_DETAILS_ACCOUNT_ID, txCursor->accountTimeTxMin + CB_ACCOUNT_TIME_TX_ACCOUNT_ID, 8);
	memcpy(keyArr + CB_ACCOUNT_TX_DETAILS_TX_ID, key + CB_ACCOUNT_TIME_TX_TX_ID, 8);
	if (CBDatabaseReadValue(cursor->storage->accountTxDetails, keyArr, details->accountTxDetails.addrHash, 20, CB_ACCOUNT_TX_DETAILS_ADDR, false) != CB_DATABASE_INDEX_FOUND) {
		CBLogError("Could not read a transaction's source/destination address.");
		return CB_ERROR;
	}
	// ??? Here the read is being done on the same key twice. Cache the index search result?
	if (CBDatabaseReadValue(cursor->storage->accountTxDetails, keyArr, data, 8, CB_ACCOUNT_TX_DETAILS_VALUE, false) != CB_DATABASE_INDEX_FOUND) {
		CBLogError("Could not read a transaction's value.");
		return CB_ERROR;
	}
	CBAccounterUInt64ToInt64(CBArrayToInt64(data, 0), &details->accountTxDetails.amount);
	// Read height
	memcpy(keyArr + CB_ORDER_NUM_TX_DETAILS_ORDER_NUM, key + CB_ACCOUNT_TIME_TX_ORDERNUM, 8);
	memcpy(keyArr + CB_ORDER_NUM_TX_DETAILS_TX_ID, key + CB_ACCOUNT_TIME_TX_TX_ID, 8);
	if (CBDatabaseReadValue(cursor->storage->orderNumTxDetails, keyArr, data, 4, CB_ORDER_NUM_TX_DETAILS_HEIGHT, false) != CB_DATABASE_INDEX_FOUND) {
		CBLogError("Could not read the height of a transaction in a branch");
		return false;
	}
	details->height = CBArrayToInt32BigEndian(data, 0);
	// Add the timestamp
	details->timestamp = CBArrayToInt64BigEndian(key, CB_ACCOUNT_TIME_TX_TIMESTAMP);
	return CB_TRUE;
}
CBErrBool CBAccounterGetNextUnspentOutput(CBDepObject cursoru, CBUnspentOutputDetails * details){
	CBAccounterStorageUnspentOutputCursor * uoCursor = cursoru.ptr;
	CBAccounterStorageCursor * cursor = CBGetAccounterCursor(uoCursor);
	CBIndexFindStatus res;
	for (;;) {
		if (cursor->started)
			res = CBDatabaseRangeIteratorNext(&cursor->it);
		else{
			CBInitDatabaseRangeIterator(&cursor->it, uoCursor->accountUnspentOutputsMin, uoCursor->accountUnspentOutputsMax, cursor->storage->accountUnspentOutputs);
			res = CBDatabaseRangeIteratorFirst(&cursor->it);
			cursor->started = true;
		}
		if (res == CB_DATABASE_INDEX_ERROR) {
			CBLogError("There was an error trying to find an unspent output for an account.");
			return CB_ERROR;
		}
		if (res != CB_DATABASE_INDEX_FOUND)
			return CB_FALSE;
		// Get the key
		uint8_t key[20];
		CBDatabaseRangeIteratorGetKey(&cursor->it, key);
		// Get output details
		uint8_t data[64];
		if (CBDatabaseReadValue(cursor->storage->outputDetails, key + CB_ACCOUNT_UNSPENT_OUTPUTS_OUTPUT_ID, data, 64, 0, false)!= CB_DATABASE_INDEX_FOUND) {
			CBLogError("Couldn't get the details for an output.");
			return CB_ERROR;
		}
		// Now free the key
		details->value = CBArrayToInt64(data, CB_OUTPUT_DETAILS_VALUE);
		memcpy(details->txHash, data + CB_OUTPUT_DETAILS_TX_HASH, 32);
		details->index = CBArrayToInt32(data, CB_OUTPUT_DETAILS_INDEX);
		memcpy(details->watchedHash, data + CB_OUTPUT_DETAILS_ID_HASH, 20);
		return CB_TRUE;
	}
}
bool CBAccounterGetLastAccountBalance(CBAccounterStorage * storage, uint64_t accountID, uint64_t maxTime, uint64_t * balance, int64_t * unconfBalance){
	uint8_t accountTimeTxMin[32];
	uint8_t accountTimeTxMax[32];
	CBInt64ToArray(accountTimeTxMin, CB_ACCOUNT_TIME_TX_ACCOUNT_ID, accountID);
	CBInt64ToArray(accountTimeTxMax, CB_ACCOUNT_TIME_TX_ACCOUNT_ID, accountID);
	memset(accountTimeTxMin + CB_ACCOUNT_TIME_TX_TIMESTAMP, 0, 24);
	CBInt64ToArrayBigEndian(accountTimeTxMax, CB_ACCOUNT_TIME_TX_TIMESTAMP, maxTime);
	memset(accountTimeTxMax + CB_ACCOUNT_TIME_TX_ORDERNUM, 0xFF, 16);
	CBDatabaseRangeIterator lastTxIt;
	CBInitDatabaseRangeIterator(&lastTxIt, accountTimeTxMin, accountTimeTxMax, storage->accountTimeTx);
	CBIndexFindStatus res = CBDatabaseRangeIteratorLast(&lastTxIt);
	if (res == CB_DATABASE_INDEX_ERROR) {
		CBLogError("Could not get the last transaction in a branch section to get the last balance for calculating a new balance.");
		CBFreeDatabaseRangeIterator(&lastTxIt);
		return false;
	}
	if (res == CB_DATABASE_INDEX_FOUND) {
		uint8_t data[16];
		if (! CBDatabaseRangeIteratorRead(&lastTxIt, data, 16, 0)) {
			CBLogError("Could not read the transaction cumulative balance for an account.");
			CBFreeDatabaseRangeIterator(&lastTxIt);
			return false;
		}
		*balance = CBArrayToInt64(data, CB_ACCOUNT_TIME_TX_BALANCE);
		CBAccounterUInt64ToInt64(CBArrayToInt64(data, CB_ACCOUNT_TIME_TX_UNCONF_BALANCE), unconfBalance);
	}else{
		*balance = 0;
		*unconfBalance = 0;
	}
	CBFreeDatabaseRangeIterator(&lastTxIt);
	return true;
}
bool CBAccounterGetTransactionTime(CBDepObject self, uint8_t * txHash, uint64_t * time){
	CBAccounterStorage * storage = self.ptr;
	// Get the transaction ID
	uint8_t key[32], data[8];
	memcpy(key, txHash, 32);
	if (CBDatabaseReadValue(storage->txHashToID, key, data, 8, 0, false) != CB_DATABASE_INDEX_FOUND) {
		CBLogError("Could not read a transaction ID for a hash when getting a transaction time.");
		return false;
	}
	// Read timestamp
	uint8_t timestamp[8];
	if (CBDatabaseReadValue(storage->txDetails, data, timestamp, 8, CB_TX_DETAILS_TIMESTAMP, false) != CB_DATABASE_INDEX_FOUND) {
		CBLogError("Could not read a transaction's timestamp when getting a transaction time.");
		return false;
	}
	*time = CBArrayToInt64BigEndian(timestamp, 0);
	return true;
}
bool CBAccounterGetTxAccountValue(CBAccounterStorage * self, uint64_t txID, uint64_t accountID, int64_t * value){
	uint8_t key[16], data[8];
	CBInt64ToArray(key, CB_ACCOUNT_TX_DETAILS_ACCOUNT_ID, accountID);
	CBInt64ToArray(key, CB_ACCOUNT_TX_DETAILS_TX_ID, txID);
	if (CBDatabaseReadValue(self->accountTxDetails, key, data, 8, CB_ACCOUNT_TX_DETAILS_VALUE, false) != CB_DATABASE_INDEX_FOUND) {
		CBLogError("Could not read an accounts transaction value.");
		return false;
	}
	CBAccounterUInt64ToInt64(CBArrayToInt64(data, 0), value);
	return true;
}
uint64_t CBAccounterInt64ToUInt64(int64_t value){
	return labs(value) | ((value > 0) ? 0x8000000000000000 : 0);
}
CBErrBool CBAccounterIsOurs(CBDepObject uself, uint8_t * txHash){
	CBAccounterStorage * self = uself.ptr;
	uint32_t length;
	if (!CBDatabaseGetLength(self->txHashToID, txHash, &length))
		return CB_ERROR;
	return (length == CB_DOESNT_EXIST) ? CB_FALSE : CB_TRUE;
}
bool CBAccounterLostTransaction(CBDepObject self, void * vtx, uint32_t height){
	CBAccounterStorage * storage = self.ptr;
	CBTransaction * tx = vtx;
	uint8_t keyArr[36], data[8], txID[8];
	// Lost a transaction
	// Loop through inputs looking for spent outputs that would therefore become unspent outputs.
	for (uint32_t x = 0; x < tx->inputNum; x++) {
		// Get the output ID
		memcpy(keyArr + CB_OUTPUT_HASH_AND_INDEX_TO_ID_HASH, CBByteArrayGetData(tx->inputs[x]->prevOut.hash), 32);
		CBInt32ToArray(keyArr, CB_OUTPUT_HASH_AND_INDEX_TO_ID_INDEX, tx->inputs[x]->prevOut.index);
		CBIndexFindStatus stat = CBDatabaseReadValue(storage->outputHashAndIndexToID, keyArr, data, 8, 0, false);
		if (stat == CB_DATABASE_INDEX_ERROR) {
			CBLogError("Could not read the ID of an output reference.");
			return false;
		}
		if (stat == CB_DATABASE_INDEX_NOT_FOUND)
			// The output reference does not exist, so we can end here and continue to the next input.
			continue;
		// Determine if the transaction is unconfirmed, if unconfirmed then we should change the spent status to unspent, otherwise we should remove the status.
		// First get transaction ID
		memcpy(keyArr, CBByteArrayGetData(tx->inputs[x]->prevOut.hash), 32);
		stat = CBDatabaseReadValue(storage->txHashToID, keyArr, txID, 8, 0, false);
		if (stat != CB_DATABASE_INDEX_FOUND) {
			CBLogError("Could not find the ID of a transaction.");
			return false;
		}
		// Loop through account outputs
		uint8_t outputAccountsMin[16];
		uint8_t outputAccountsMax[16];
		memcpy(outputAccountsMin + CB_OUTPUT_ACCOUNTS_OUTPUT_ID, data, 8);
		memcpy(outputAccountsMax + CB_OUTPUT_ACCOUNTS_OUTPUT_ID, data, 8);
		memset(outputAccountsMin + CB_OUTPUT_ACCOUNTS_ACCOUNTS_ID, 0, 8);
		memset(outputAccountsMax + CB_OUTPUT_ACCOUNTS_ACCOUNTS_ID, 0xFF, 8);
		CBDatabaseRangeIterator it;
		CBInitDatabaseRangeIterator(&it, outputAccountsMin, outputAccountsMax, storage->outputAccounts);
		if (CBDatabaseRangeIteratorFirst(&it) != CB_DATABASE_INDEX_FOUND) {
			CBLogError("Could not get the first account for an output when removing an output spent status from branchless.");
			CBFreeDatabaseRangeIterator(&it);
			return false;
		}
		// Prepare account unspent output key with  and output ID.
		memcpy(keyArr + CB_ACCOUNT_UNSPENT_OUTPUTS_OUTPUT_ID, data, 8);
		for (;;) {
			// Add the account ID into the key
			uint8_t key[16];
			CBDatabaseRangeIteratorGetKey(&it, key);
			memcpy(keyArr + CB_ACCOUNT_UNSPENT_OUTPUTS_ACCOUNT_ID, key + CB_OUTPUT_ACCOUNTS_ACCOUNTS_ID, 8);
			// Create unspent output entry
			CBDatabaseWriteValue(storage->accountUnspentOutputs, keyArr, NULL, 0);
			// Iterate if possible
			CBIndexFindStatus status = CBDatabaseRangeIteratorNext(&it);
			if (status == CB_DATABASE_INDEX_ERROR) {
				CBLogError("Error whilst iterating through account output statuses.");
				CBFreeDatabaseRangeIterator(&it);
				return false;
			}
			if (status == CB_DATABASE_INDEX_NOT_FOUND)
				break;
		}
		CBFreeDatabaseRangeIterator(&it);
	}
	// Get transaction ID
	memcpy(keyArr, CBTransactionGetHash(tx), 32);
	if (CBDatabaseReadValue(storage->txHashToID, keyArr, txID, 8, 0, false) != CB_DATABASE_INDEX_FOUND) {
		CBLogError("Could not read a transaction ID for a hash.");
		return false;
	}
	// Read the instance count
	uint8_t instances;
	uint8_t timestampData[8];
	if (CBDatabaseReadValue(storage->txDetails, txID, &instances, 1, CB_TX_DETAILS_INSTANCES, false) != CB_DATABASE_INDEX_FOUND) {
		CBLogError("Could not read the number of instances for a transaction.");
		return false;
	}
	// Also read timestamp for later
	if (CBDatabaseReadValue(storage->txDetails, txID, timestampData, 8, CB_TX_DETAILS_TIMESTAMP, false) != CB_DATABASE_INDEX_FOUND) {
		CBLogError("Could not read a transactions timestamp.");
		return false;
	}
	// Lower the number of instances
	if (--instances == 0) {
		// No more instances, delete transaction information.
		if (! CBDatabaseRemoveValue(storage->txDetails, keyArr, false)) {
			CBLogError("Could not remove transaction details.");
			return false;
		}
		if (! CBDatabaseRemoveValue(storage->txHashToID, CBTransactionGetHash(tx), false)) {
			CBLogError("Could not remove a transaction hash to ID entry.");
			return false;
		}
	}else
		CBDatabaseWriteValueSubSection(storage->txDetails, keyArr, &instances, 1, CB_TX_DETAILS_INSTANCES);
	// Get the last order number
	uint8_t orderNumTxDetailsMin[16], orderNumTxDetailsMax[16];
	memcpy(orderNumTxDetailsMin + CB_ORDER_NUM_TX_DETAILS_TX_ID, txID, 8);
	memcpy(orderNumTxDetailsMax + CB_ORDER_NUM_TX_DETAILS_TX_ID, txID, 8);
	memset(orderNumTxDetailsMin + CB_ORDER_NUM_TX_DETAILS_ORDER_NUM, 0, 8);
	memset(orderNumTxDetailsMax + CB_ORDER_NUM_TX_DETAILS_ORDER_NUM, 0xFF, 8);
	CBDatabaseRangeIterator it;
	CBInitDatabaseRangeIterator(&it, orderNumTxDetailsMin, orderNumTxDetailsMax, storage->orderNumTxDetails);
	if (CBDatabaseRangeIteratorLast(&it) != CB_DATABASE_INDEX_FOUND){
		CBLogError("Could not get the last order number for a transaction.");
		CBFreeDatabaseRangeIterator(&it);
		return false;
	}
	// Get the key and thus the order number, reuse orderNumTxDetailsMax
	CBDatabaseRangeIteratorGetKey(&it, orderNumTxDetailsMax);
	CBFreeDatabaseRangeIterator(&it);
	// Delete ordered transaction details.
	if (! CBDatabaseRemoveValue(storage->orderNumTxDetails, orderNumTxDetailsMax, false)) {
		CBLogError("Couldn't remove the ordered transaction details");
		return false;
	}
	uint8_t * orderNum = orderNumTxDetailsMax + CB_ORDER_NUM_TX_DETAILS_ORDER_NUM;
	// Loop through the transaction accounts
	uint8_t txAccountsMin[16];
	uint8_t txAccountsMax[16];
	memcpy(txAccountsMin + CB_TX_ACCOUNTS_TX_ID, txID, 8);
	memcpy(txAccountsMax + CB_TX_ACCOUNTS_TX_ID, txID, 8);
	memset(txAccountsMin + CB_TX_ACCOUNTS_ACCOUNT_ID, 0, 8);
	memset(txAccountsMax + CB_TX_ACCOUNTS_ACCOUNT_ID, 0xFF, 8);
	CBInitDatabaseRangeIterator(&it, txAccountsMin, txAccountsMax, storage->txAccounts);
	if (CBDatabaseRangeIteratorFirst(&it) != CB_DATABASE_INDEX_FOUND) {
		CBLogError("There was a problem finding the first transaction account entry.");
		CBFreeDatabaseRangeIterator(&it);
		return false;
	}
	for (;;) {
		// Get the tx account key
		uint8_t key[16];
		CBDatabaseRangeIteratorGetKey(&it, key);
		uint64_t accountID = CBArrayToInt64(key, CB_TX_ACCOUNTS_ACCOUNT_ID);
		// Adjust balances
		int64_t value;
		if (! CBAccounterGetTxAccountValue(storage, CBArrayToInt64(txID, 0), accountID, &value)) {
			CBLogError("Could not get the value of a transaction for an account.");
			CBFreeDatabaseRangeIterator(&it);
			return false;
		}
		// Remove the branch account time transaction entry
		memcpy(keyArr + CB_ACCOUNT_TIME_TX_ACCOUNT_ID, key + CB_TX_ACCOUNTS_ACCOUNT_ID, 8);
		memcpy(keyArr + CB_ACCOUNT_TIME_TX_TIMESTAMP, timestampData, 8);
		memcpy(keyArr + CB_ACCOUNT_TIME_TX_ORDERNUM, orderNum, 8);
		memcpy(keyArr + CB_ACCOUNT_TIME_TX_TX_ID, txID, 8);
		if (! CBDatabaseRemoveValue(storage->accountTimeTx, keyArr, false)) {
			CBLogError("Could not remove an account time ordered transaction entry.");
			CBFreeDatabaseRangeIterator(&it);
			return false;
		}
		// Can now adjust other time transaction entry balances...
		uint8_t high[16];
		memset(high, 0xFF, 16);
		if (! CBAccounterAdjustBalances(storage, keyArr + CB_ACCOUNT_TIME_TX_TIMESTAMP, high, accountID, -value, (height == CB_UNCONFIRMED)*(-value), NULL, NULL)){
			CBLogError("Could not adjust balances for transactions past a deleted one.");
			CBFreeDatabaseRangeIterator(&it);
			return false;
		}
		CBIndexFindStatus status;
		// Remove account transaction details if there are no more instances.
		if (instances == 0) {
			// Remove the account transaction details
			memcpy(keyArr + CB_ACCOUNT_TX_DETAILS_ACCOUNT_ID, key + CB_TX_ACCOUNTS_ACCOUNT_ID, 8);
			memcpy(keyArr + CB_ACCOUNT_TX_DETAILS_TX_ID, key + CB_TX_ACCOUNTS_TX_ID, 8);
			if (! CBDatabaseRemoveValue(storage->accountTxDetails, keyArr, false)) {
				CBLogError("Could not remove account transaction details.");
				CBFreeDatabaseRangeIterator(&it);
				return false;
			}
			// Remove the transaction account entry
			if (! CBDatabaseRemoveValue(storage->txAccounts, key, false)) {
				CBLogError("Could not remove a transaction account entry.");
				CBFreeDatabaseRangeIterator(&it);
				return false;
			}
			// Since we are removing the txAccounts, find first
			status = CBDatabaseRangeIteratorFirst(&it);
		}else
			// Else iterate if possible
			status = CBDatabaseRangeIteratorNext(&it);
		if (status == CB_DATABASE_INDEX_ERROR) {
			CBLogError("Error whilst iterating through transaction accounts.");
			CBFreeDatabaseRangeIterator(&it);
			return false;
		}
		if (status == CB_DATABASE_INDEX_NOT_FOUND)
			break;
	}
	CBFreeDatabaseRangeIterator(&it);
	// Look for outputs we have found.
	// Do this by finding outputs via the transaction hash and then go through all the outputs for that hash, if any.
	uint8_t outputHashAndIndexToIDMin[36];
	uint8_t outputHashAndIndexToIDMax[36];
	// Add the transaction hash to the keys
	memcpy(outputHashAndIndexToIDMin + CB_OUTPUT_HASH_AND_INDEX_TO_ID_HASH, CBTransactionGetHash(tx), 32);
	memcpy(outputHashAndIndexToIDMax + CB_OUTPUT_HASH_AND_INDEX_TO_ID_HASH, CBTransactionGetHash(tx), 32);
	memset(outputHashAndIndexToIDMin + CB_OUTPUT_HASH_AND_INDEX_TO_ID_INDEX, 0, 4);
	memset(outputHashAndIndexToIDMax + CB_OUTPUT_HASH_AND_INDEX_TO_ID_INDEX, 0xFF, 4);
	// Loop through the range of potential outputs
	CBInitDatabaseRangeIterator(&it, outputHashAndIndexToIDMin, outputHashAndIndexToIDMax, storage->outputHashAndIndexToID);
	CBIndexFindStatus res = CBDatabaseRangeIteratorFirst(&it);
	if (res == CB_DATABASE_INDEX_ERROR) {
		CBLogError("There was an error whilst attemping to find the first account output information for a transaction.");
		CBFreeDatabaseRangeIterator(&it);
		return false;
	}
	if (res == CB_DATABASE_INDEX_FOUND) for (;;) {
		// Get the output ID
		uint8_t outputId[8];
		if (! CBDatabaseRangeIteratorRead(&it, outputId, 8, 0)) {
			CBLogError("Could not read the ID of an output.");
			CBFreeDatabaseRangeIterator(&it);
			return false;
		}
		// Loop through the output accounts
		uint8_t outputAccountsMin[16];
		uint8_t outputAccountsMax[16];
		memcpy(outputAccountsMin + CB_OUTPUT_ACCOUNTS_OUTPUT_ID, outputId, 8);
		memcpy(outputAccountsMax + CB_OUTPUT_ACCOUNTS_OUTPUT_ID, outputId, 8);
		memset(outputAccountsMin + CB_OUTPUT_ACCOUNTS_ACCOUNTS_ID, 0, 8);
		memset(outputAccountsMax + CB_OUTPUT_ACCOUNTS_ACCOUNTS_ID, 0xFF, 8);
		CBDatabaseRangeIterator accountIt;
		CBInitDatabaseRangeIterator(&accountIt, outputAccountsMin, outputAccountsMax, storage->outputAccounts);
		if (CBDatabaseRangeIteratorFirst(&accountIt) != CB_DATABASE_INDEX_FOUND) {
			CBLogError("There was a problem finding the first output account entry.");
			CBFreeDatabaseRangeIterator(&it);
			CBFreeDatabaseRangeIterator(&accountIt);
			return false;
		}
		for (;;) {
			// Get the key for this account
			uint8_t outputAccountKey[16];
			CBDatabaseRangeIteratorGetKey(&accountIt, outputAccountKey);
			// Remove the account unspent output
			memcpy(keyArr + CB_ACCOUNT_UNSPENT_OUTPUTS_ACCOUNT_ID, outputAccountKey + CB_OUTPUT_ACCOUNTS_ACCOUNTS_ID, 8);
			memcpy(keyArr + CB_ACCOUNT_UNSPENT_OUTPUTS_OUTPUT_ID, outputId, 8);
			if (! CBDatabaseRemoveValue(storage->accountUnspentOutputs, keyArr, false)) {
				CBLogError("Couldn't remove an output as unspent for an account.");
				CBFreeDatabaseRangeIterator(&it);
				CBFreeDatabaseRangeIterator(&accountIt);
				return false;
			}
			// If there are no more instances of the transaction, completely remove all output account information
			CBIndexFindStatus status;
			if (instances == 0){
				// Remove the output account entry
				if (! CBDatabaseRemoveValue(storage->outputAccounts, outputAccountKey, false)) {
					CBLogError("Couldn't remove an output account entry");
					CBFreeDatabaseRangeIterator(&it);
					CBFreeDatabaseRangeIterator(&accountIt);
					return false;
				}
				// As we removed outputAccounts, find first.
				status = CBDatabaseRangeIteratorFirst(&accountIt);
			}else
				// Else iterate if possible
				status = CBDatabaseRangeIteratorNext(&accountIt);
			if (status == CB_DATABASE_INDEX_ERROR) {
				CBLogError("Error whilst iterating through output accounts.");
				CBFreeDatabaseRangeIterator(&it);
				CBFreeDatabaseRangeIterator(&accountIt);
				return false;
			}
			if (status == CB_DATABASE_INDEX_NOT_FOUND)
				break;
		}
		CBFreeDatabaseRangeIterator(&accountIt);
		// If there are no more instances of the transaction, completely remove all output information
		CBIndexFindStatus status;
		if (instances == 0) {
			// Remove the output details
			if (! CBDatabaseRemoveValue(storage->outputDetails, outputId, false)) {
				CBLogError("Could not remove the output details.");
				CBFreeDatabaseRangeIterator(&it);
				return false;
			}
			// Get the key
			uint8_t key[36];
			CBDatabaseRangeIteratorGetKey(&it, key);
			// Remove the hash and index to ID entry.
			if (! CBDatabaseRemoveValue(storage->outputHashAndIndexToID, key, false)) {
				CBLogError("Could not remove the output hash and index to ID entry.");
				CBFreeDatabaseRangeIterator(&it);
				return false;
			}
			// As we removed outputHashAndIndexToID find first.
			status = CBDatabaseRangeIteratorFirst(&it);
		}else
			// Else iterate if possible
			status = CBDatabaseRangeIteratorNext(&it);
		if (status == CB_DATABASE_INDEX_ERROR) {
			CBLogError("Error whilst iterating through tx outputs.");
			CBFreeDatabaseRangeIterator(&it);
			return false;
		}
		if (status == CB_DATABASE_INDEX_NOT_FOUND)
			break;
	}
	CBFreeDatabaseRangeIterator(&it);
	return true;
}
bool CBAccounterMergeAccountIntoAccount(CBDepObject self, uint64_t accountDest, uint64_t accountSrc){
	CBAccounterStorage * storage = self.ptr;
	// Loop through watched hashes of source account
	uint8_t accountWatchedHashesMin[28];
	uint8_t accountWatchedHashesMax[28];
	CBInt64ToArray(accountWatchedHashesMin, CB_ACCOUNT_WATCHED_HASHES_ACCOUNT_ID, accountSrc);
	CBInt64ToArray(accountWatchedHashesMax, CB_ACCOUNT_WATCHED_HASHES_ACCOUNT_ID, accountSrc);
	memset(accountWatchedHashesMin + CB_ACCOUNT_WATCHED_HASHES_HASH, 0, 20);
	memset(accountWatchedHashesMax + CB_ACCOUNT_WATCHED_HASHES_HASH, 0xFF, 20);
	CBDatabaseRangeIterator it;
	CBInitDatabaseRangeIterator(&it, accountWatchedHashesMin, accountWatchedHashesMax, storage->accountWatchedHashes);
	CBIndexFindStatus status = CBDatabaseRangeIteratorFirst(&it);
	if (status == CB_DATABASE_INDEX_ERROR) {
		CBLogError("There was a problem finding the first account transaction details entry.");
		CBFreeDatabaseRangeIterator(&it);
		return false;
	}
	uint8_t keyArr[33], keyArr2[16];
	if (status == CB_DATABASE_INDEX_FOUND) for (;;) {
		// Get key
		uint8_t key[28];
		CBDatabaseRangeIteratorGetKey(&it, key);
		// Copy hash for writing to destination account
		memcpy(keyArr + CB_ACCOUNT_WATCHED_HASHES_HASH, key + CB_ACCOUNT_WATCHED_HASHES_HASH, 20);
		// Insert destination account ID
		CBInt64ToArray(keyArr, CB_ACCOUNT_WATCHED_HASHES_ACCOUNT_ID, accountDest);
		// Write entry
		CBDatabaseWriteValue(storage->accountWatchedHashes, keyArr, NULL, 0);
		// Now do the same for watchedHashes
		memcpy(keyArr + CB_WATCHED_HASHES_HASH, key + CB_ACCOUNT_WATCHED_HASHES_HASH, 20);
		CBInt64ToArray(keyArr, CB_WATCHED_HASHES_ACCOUNT_ID, accountDest);
		CBDatabaseWriteValue(storage->watchedHashes, keyArr, NULL, 0);
		// Go to next first if possible. Make mininum same as key plus one
		CBDatabaseRangeIteratorNextMinimum(&it);
		status = CBDatabaseRangeIteratorFirst(&it);
		if (status == CB_DATABASE_INDEX_ERROR) {
			CBLogError("Error whilst iterating through account watched hashes.");
			CBFreeDatabaseRangeIterator(&it);
			return false;
		}
		if (status == CB_DATABASE_INDEX_NOT_FOUND)
			break;
	}
	CBFreeDatabaseRangeIterator(&it);
	// Loop through account transactions of source account
	uint8_t accountTxDetailsMin[16];
	uint8_t accountTxDetailsMax[16];
	CBInt64ToArray(accountTxDetailsMin, CB_ACCOUNT_TX_DETAILS_ACCOUNT_ID, accountSrc);
	CBInt64ToArray(accountTxDetailsMax, CB_ACCOUNT_TX_DETAILS_ACCOUNT_ID, accountSrc);
	memset(accountTxDetailsMin + CB_ACCOUNT_TX_DETAILS_TX_ID, 0, 8);
	memset(accountTxDetailsMax + CB_ACCOUNT_TX_DETAILS_TX_ID, 0xFF, 8);
	CBInitDatabaseRangeIterator(&it, accountTxDetailsMin, accountTxDetailsMax, storage->accountTxDetails);
	status = CBDatabaseRangeIteratorFirst(&it);
	if (status == CB_DATABASE_INDEX_ERROR) {
		CBLogError("There was a problem finding the first account transaction details entry.");
		CBFreeDatabaseRangeIterator(&it);
		return false;
	}
	if (status == CB_DATABASE_INDEX_FOUND) for (;;) {
		// Get the data
		uint8_t data[28];
		if (!CBDatabaseRangeIteratorRead(&it, data, 28, 0)) {
			CBLogError("There was an error trying to read transaction data for the source account during a merge.");
			CBFreeDatabaseRangeIterator(&it);
			return false;
		}
		// Get the key
		uint8_t key[16];
		CBDatabaseRangeIteratorGetKey(&it, key);
		// Copy transaction id for writing to destination account
		memcpy(keyArr + CB_ACCOUNT_TX_DETAILS_TX_ID, key + CB_ACCOUNT_TX_DETAILS_TX_ID, 8);
		// Copy destination account id
		CBInt64ToArray(keyArr, CB_ACCOUNT_TX_DETAILS_ACCOUNT_ID, accountDest);
		// See if transaction details for the destination account already exists
		uint8_t destData[28];
		CBIndexFindStatus status = CBDatabaseReadValue(storage->accountTxDetails, keyArr, destData, 28, 0, false);
		if (status == CB_DATABASE_INDEX_ERROR) {
			CBLogError("There was an error trying to determine if account transaction details exists.");
			CBFreeDatabaseRangeIterator(&it);
			return CB_ERROR;
		}
		if (status == CB_DATABASE_INDEX_FOUND) {
			// Exists
			// Get values
			int64_t srcVal, destVal, newVal;
			CBAccounterUInt64ToInt64(CBArrayToInt64(data, CB_ACCOUNT_TX_DETAILS_VALUE), &srcVal);
			CBAccounterUInt64ToInt64(CBArrayToInt64(destData, CB_ACCOUNT_TX_DETAILS_VALUE), &destVal);
			newVal = srcVal + destVal;
			// Adjust value
			CBInt64ToArray(destData, CB_ACCOUNT_TX_DETAILS_VALUE, CBAccounterInt64ToUInt64(newVal));
			// If source value absolute value is greater, use that address
			if (srcVal > destVal) {
				memcpy(destData + CB_ACCOUNT_TX_DETAILS_ADDR, data + CB_ACCOUNT_TX_DETAILS_ADDR, 20);
				// Write all data
				CBDatabaseWriteValue(storage->accountTxDetails, keyArr, destData, 28);
			}else if (srcVal != 0)
				// Write value only
				CBDatabaseWriteValueSubSection(storage->accountTxDetails, keyArr, destData, 8, CB_ACCOUNT_TX_DETAILS_VALUE);
		}else{
			// Doesn't exist, simply move details over
			// Write transaction details
			CBDatabaseWriteValue(storage->accountTxDetails, keyArr, data, 28);
			// Write transaction account entry
			memcpy(keyArr + CB_TX_ACCOUNTS_TX_ID, key + CB_ACCOUNT_TX_DETAILS_TX_ID, 8);
			CBInt64ToArray(keyArr, CB_TX_ACCOUNTS_ACCOUNT_ID, accountDest);
			CBDatabaseWriteValue(storage->txAccounts, keyArr, NULL, 0);
		}
		// Go to next first if possible. Make mininum same as key plus one
		CBDatabaseRangeIteratorNextMinimum(&it);
		status = CBDatabaseRangeIteratorFirst(&it);
		if (status == CB_DATABASE_INDEX_ERROR) {
			CBLogError("Error whilst iterating through account transaction details entries.");
			CBFreeDatabaseRangeIterator(&it);
			return false;
		}
		if (status == CB_DATABASE_INDEX_NOT_FOUND)
			break;
	}
	CBFreeDatabaseRangeIterator(&it);
	// 1. loop through branchSectionAccountTimeTx 2. loop through account outputs
	int64_t addedBalance = 0, addedUnconfBalance = 0;
	uint64_t last = 0;
	int64_t lastUnconf = 0;
	// branchSectionAccountTimeTx
	uint8_t accountTimeTxMin[32];
	uint8_t accountTimeTxMax[32];
	CBInt64ToArray(accountTimeTxMin, CB_ACCOUNT_TIME_TX_ACCOUNT_ID, accountSrc);
	CBInt64ToArray(accountTimeTxMax, CB_ACCOUNT_TIME_TX_ACCOUNT_ID, accountSrc);
	memset(accountTimeTxMin + CB_ACCOUNT_TIME_TX_TIMESTAMP, 0, 24);
	memset(accountTimeTxMax + CB_ACCOUNT_TIME_TX_TIMESTAMP, 0xFF, 24);
	CBInitDatabaseRangeIterator(&it, accountTimeTxMin, accountTimeTxMax, storage->accountTimeTx);
	status = CBDatabaseRangeIteratorFirst(&it);
	if (status == CB_DATABASE_INDEX_ERROR) {
		CBLogError("There was a problem finding the first account time tx details.");
		CBFreeDatabaseRangeIterator(&it);
		return false;
	}
	// Set last key as zero
	memset(keyArr + CB_ACCOUNT_TIME_TX_TIMESTAMP, 0, 16);
	if (status == CB_DATABASE_INDEX_FOUND) for (;;) {
		// Get the key
		uint8_t key[32], data[16];
		CBDatabaseRangeIteratorGetKey(&it, key);
		// Adjust balances for everything between last and this key if addedBalance is not 0
		if (!CBAccounterAdjustBalances(storage, keyArr + CB_ACCOUNT_TIME_TX_TIMESTAMP, key + CB_ACCOUNT_TIME_TX_TIMESTAMP, accountDest, addedBalance, addedUnconfBalance, &last, &lastUnconf)) {
			CBLogError("Could not adjust the cumulative balanaces.");
			CBFreeDatabaseRangeIterator(&it);
			return false;
		}
		// See if the transaction is unconfirmed. If it is then adjust the unconfirmed added balance and last balance.
		uint8_t orderNumTxDetailsKey[16];
		memcpy(orderNumTxDetailsKey + CB_ORDER_NUM_TX_DETAILS_TX_ID, key + CB_ACCOUNT_TIME_TX_TX_ID, 8);
		memcpy(orderNumTxDetailsKey + CB_ORDER_NUM_TX_DETAILS_ORDER_NUM, key + CB_ACCOUNT_TIME_TX_ORDERNUM, 8);
		if (CBDatabaseReadValue(storage->orderNumTxDetails, orderNumTxDetailsKey, data, 4, CB_ORDER_NUM_TX_DETAILS_HEIGHT, false) != CB_DATABASE_INDEX_FOUND) {
			CBLogError("Couldn't read the height of a transaction for the source account.");
			CBFreeDatabaseRangeIterator(&it);
			return false;
		}
		bool unconfirmed = CBArrayToInt32BigEndian(data, 0) == CB_UNCONFIRMED;
		// Adjust addedBalance by the value of the source account transaction details.
		memcpy(keyArr2 + CB_ACCOUNT_TX_DETAILS_TX_ID, key + CB_ACCOUNT_TIME_TX_TX_ID, 8);
		CBInt64ToArray(keyArr2, CB_ACCOUNT_TX_DETAILS_ACCOUNT_ID, accountSrc);
		if (CBDatabaseReadValue(storage->accountTxDetails, keyArr2, data, 8, CB_ACCOUNT_TX_DETAILS_VALUE, false) != CB_DATABASE_INDEX_FOUND) {
			CBLogError("Couldn't read the value of a transaction for the source account.");
			CBFreeDatabaseRangeIterator(&it);
			return false;
		}
		int64_t value;
		CBAccounterUInt64ToInt64(CBArrayToInt64(data, 0), &value);
		addedBalance += value;
		last += value;
		if (unconfirmed) {
			addedUnconfBalance += value;
			lastUnconf += value;
		}
		CBInt64ToArray(data, CB_ACCOUNT_TIME_TX_BALANCE, last);
		CBInt64ToArray(data, CB_ACCOUNT_TIME_TX_UNCONF_BALANCE, CBAccounterInt64ToUInt64(lastUnconf));
		// Copy everything from the key except the account ID
		memcpy(keyArr + CB_ACCOUNT_TIME_TX_TIMESTAMP, key + CB_ACCOUNT_TIME_TX_TIMESTAMP, 24);
		// Set the destination account ID
		CBInt64ToArray(keyArr, CB_ACCOUNT_TIME_TX_ACCOUNT_ID, accountDest);
		// Write data for destination account
		CBDatabaseWriteValue(storage->accountTimeTx, keyArr, data, 16);
		// Increase key by one since we do not want to adjust the value added. Do this with the ordernum
		CBInt64ToArrayBigEndian(keyArr, CB_ACCOUNT_TIME_TX_ORDERNUM, CBArrayToInt64BigEndian(keyArr, CB_ACCOUNT_TIME_TX_ORDERNUM) + 1);
		// Go to next first if possible. Make mininum same as key plus one
		CBDatabaseRangeIteratorNextMinimum(&it);
		CBIndexFindStatus status = CBDatabaseRangeIteratorFirst(&it);
		if (status == CB_DATABASE_INDEX_ERROR) {
			CBLogError("Error whilst iterating through branch section account time tx details.");
			CBFreeDatabaseRangeIterator(&it);
			return false;
		}
		if (status == CB_DATABASE_INDEX_NOT_FOUND)
			break;
	}
	CBFreeDatabaseRangeIterator(&it);
	// Adjust remaining balances.
	uint8_t max[16];
	memset(max, 0xFF, 16);
	if (!CBAccounterAdjustBalances(storage, keyArr + CB_ACCOUNT_TIME_TX_TIMESTAMP, max, accountDest, addedBalance, addedUnconfBalance, NULL, NULL)) {
		CBLogError("Could not adjust the remaining cumulative balanaces.");
		return false;
	}
	// branchSectionAccountOutputs
	uint8_t accountUnspentOutputsMin[20];
	uint8_t accountUnspentOutputsMax[20];
	CBInt64ToArray(accountUnspentOutputsMin, CB_ACCOUNT_UNSPENT_OUTPUTS_ACCOUNT_ID, accountSrc);
	CBInt64ToArray(accountUnspentOutputsMax, CB_ACCOUNT_UNSPENT_OUTPUTS_ACCOUNT_ID, accountSrc);
	CBInitDatabaseRangeIterator(&it, accountUnspentOutputsMin, accountUnspentOutputsMax, storage->accountUnspentOutputs);
	status = CBDatabaseRangeIteratorFirst(&it);
	if (status == CB_DATABASE_INDEX_ERROR) {
		CBLogError("There was a problem finding the first branch section account output details.");
		CBFreeDatabaseRangeIterator(&it);
		return false;
	}
	if (status == CB_DATABASE_INDEX_FOUND) for (;;) {
		// Get data
		uint8_t data;
		if (!CBDatabaseRangeIteratorRead(&it, &data, 1, 0)) {
			CBLogError("There was an error trying to read an output spend status for the source account during a merge.");
			CBFreeDatabaseRangeIterator(&it);
			return false;
		}
		// Get the key
		uint8_t key[21];
		CBDatabaseRangeIteratorGetKey(&it, key);
		// Copy the output id
		memcpy(keyArr + CB_ACCOUNT_UNSPENT_OUTPUTS_OUTPUT_ID, key + CB_ACCOUNT_UNSPENT_OUTPUTS_OUTPUT_ID, 8);
		// Set the destination account ID
		CBInt64ToArray(keyArr, CB_ACCOUNT_UNSPENT_OUTPUTS_ACCOUNT_ID, accountDest);
		// See if it exists already.
		uint32_t len;
		if (! CBDatabaseGetLength(storage->accountUnspentOutputs, keyArr, &len)) {
			CBLogError("Could not determine if the destination account owns an output during a merge.");
			CBFreeDatabaseRangeIterator(&it);
			return false;
		}
		if (len == CB_DOESNT_EXIST) {
			CBDatabaseWriteValue(storage->accountUnspentOutputs, keyArr, NULL, 0);
			memcpy(keyArr + CB_OUTPUT_ACCOUNTS_OUTPUT_ID, key + CB_ACCOUNT_UNSPENT_OUTPUTS_OUTPUT_ID, 8);
			CBInt64ToArray(keyArr, CB_OUTPUT_ACCOUNTS_ACCOUNTS_ID, accountDest);
			CBDatabaseWriteValue(storage->outputAccounts, keyArr, NULL, 0);
		}
		// Go to next first if possible. Make mininum same as key plus one
		CBDatabaseRangeIteratorNextMinimum(&it);
		status = CBDatabaseRangeIteratorFirst(&it);
		if (status == CB_DATABASE_INDEX_ERROR) {
			CBLogError("Error whilst iterating through branch section account output details.");
			CBFreeDatabaseRangeIterator(&it);
			return false;
		}
		if (status == CB_DATABASE_INDEX_NOT_FOUND)
			break;
	}
	CBFreeDatabaseRangeIterator(&it);
	return true;
}
uint64_t CBAccounterNewAccount(CBDepObject self){
	// Increment the next account ID and use it, thus avoiding 0 for errors
	CBAccounterStorage * storage = self.ptr;
	uint64_t nextAccountID = ++storage->lastAccountID;
	// Save last account ID
	CBInt64ToArray(storage->database->current.extraData, CB_ACCOUNTER_DETAILS_ACCOUNT_ID, storage->lastAccountID);
	// Return the account ID to use.
	return nextAccountID;
}
CBErrBool CBAccounterTransactionExists(CBDepObject self, uint8_t * hash){
	CBAccounterStorage * storageObj = self.ptr;
	uint32_t len;
	if (! CBDatabaseGetLength(storageObj->txHashToID, hash, &len)) {
		CBLogError("There was an error trying to determine is a transaction exists.");
		return CB_ERROR;
	}
	return len != CB_DOESNT_EXIST;
}
void CBAccounterUInt64ToInt64(uint64_t raw, int64_t * value){
	*value = raw & 0x7FFFFFFFFFFFFFFF;
	if (! (raw & 0x8000000000000000))
		*value = -*value;
}
CBCompare CBCompareUInt32(CBAssociativeArray * foo, void * vint1, void * vint2){
	uint32_t * int1 = vint1;
	uint32_t * int2 = vint2;
	if (*int1 > *int2)
		return CB_COMPARE_MORE_THAN;
	if (*int1 < *int2)
		return CB_COMPARE_LESS_THAN;
	return CB_COMPARE_EQUAL;
}
