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
		{&self->accountUnconfBalance, 8, 10000},
		{&self->accountTxDetails, 16, 10000},
		{&self->branchSectionAccountOutputs, 21, 10000},
		{&self->branchSectionAccountTimeTx, 33, 10000},
		{&self->branchSectionTxDetails, 17, 10000},
		{&self->outputAccounts, 16, 10000},
		{&self->outputDetails, 8, 10000},
		{&self->outputHashAndIndexToID, 36, 10000},
		{&self->txAccounts, 16, 10000},
		{&self->txDetails, 8, 10000},
		{&self->txHashToID, 32, 10000},
		{&self->txBranchSectionHeightAndID, 21, 10000},
		{&self->watchedHashes, 28, 10000},
		{&self->accountWatchedHashes, 28, 10000},
	};
	for (uint8_t x = 0; x < 14; x++) {
		*indexes[x].index = CBLoadIndex(self->database, indexID + x, indexes[x].keySize, indexes[x].cacheLimit);
		if (indexes[x].index == NULL) {
			for (uint8_t y = 0; y < x; y++)
				CBFreeIndex(*indexes[y].index);
			CBLogError("There was an error loading the accounter index %u", x);
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
bool CBNewAccounterStorageTransactionCursor(CBDepObject * ucursor, CBDepObject accounter, uint8_t branch, uint64_t accountID, uint64_t timeMin, uint64_t timeMax){
	CBAccounterStorageTxCursor * self = malloc(sizeof(*self));
	CBAccounterCursorInit(CBGetAccounterCursor(self), accounter.ptr, branch);
	CBInt64ToArray(self->branchSectionAccountTimeTxMin, CB_BRANCH_ACCOUNT_TIME_TX_ACCOUNT_ID, accountID);
	CBInt64ToArray(self->branchSectionAccountTimeTxMax, CB_BRANCH_ACCOUNT_TIME_TX_ACCOUNT_ID, accountID);
	CBInt64ToArrayBigEndian(self->branchSectionAccountTimeTxMin, CB_BRANCH_ACCOUNT_TIME_TX_TIMESTAMP, timeMin);
	CBInt64ToArrayBigEndian(self->branchSectionAccountTimeTxMax, CB_BRANCH_ACCOUNT_TIME_TX_TIMESTAMP, timeMax);
	memset(self->branchSectionAccountTimeTxMin + CB_BRANCH_ACCOUNT_TIME_TX_ORDERNUM, 0, 16);
	memset(self->branchSectionAccountTimeTxMax + CB_BRANCH_ACCOUNT_TIME_TX_ORDERNUM, 0xFF, 16);
	ucursor->ptr = self;
	return true;
}
bool CBNewAccounterStorageUnspentOutputCursor(CBDepObject * ucursor, CBDepObject accounter, uint8_t branch, uint64_t accountID){
	CBAccounterStorageUnspentOutputCursor * self = malloc(sizeof(*self));
	CBAccounterStorage * storage = accounter.ptr;
	CBAccounterCursorInit(CBGetAccounterCursor(self), storage, branch);
	self->numSections = CB_MAX_BRANCH_CACHE - CBGetAccounterCursor(self)->currentSection;
	// We need to go through outputs in reverse so set the current section to the top index.
	CBGetAccounterCursor(self)->currentSection = CB_MAX_BRANCH_CACHE - 1;
	CBInt64ToArray(self->branchSectionAccountOutputsMin, CB_ACCOUNT_OUTPUTS_ACCOUNT_ID, accountID);
	CBInt64ToArray(self->branchSectionAccountOutputsMax, CB_ACCOUNT_OUTPUTS_ACCOUNT_ID, accountID);
	memset(self->branchSectionAccountOutputsMin + CB_ACCOUNT_OUTPUTS_HEIGHT, 0, 12);
	memset(self->branchSectionAccountOutputsMax + CB_ACCOUNT_OUTPUTS_HEIGHT, 0xFF, 12);
	CBInitAssociativeArray(&self->spentOutputs, CBTransactionKeysCompare, storage->outputDetails, CBFreeSpentOutputKeyPointer);
	ucursor->ptr = self;
	return true;
}
void CBFreeAccounterStorage(CBDepObject self){
	CBAccounterStorage * storage = self.ptr;
	CBFreeIndex(storage->accountTxDetails);
	CBFreeIndex(storage->branchSectionTxDetails);
	CBFreeIndex(storage->outputAccounts);
	CBFreeIndex(storage->outputDetails);
	CBFreeIndex(storage->outputHashAndIndexToID);
	CBFreeIndex(storage->txAccounts);
	CBFreeIndex(storage->txDetails);
	CBFreeIndex(storage->txHashToID);
	CBFreeIndex(storage->txBranchSectionHeightAndID);
	CBFreeIndex(storage->watchedHashes);
	CBFreeIndex(storage->accountWatchedHashes);
	CBFreeIndex(storage->branchSectionAccountOutputs);
	CBFreeIndex(storage->branchSectionAccountTimeTx);
	free(self.ptr);
}
void CBFreeAccounterStorageTransactionCursor(CBDepObject self){
	CBFreeDatabaseRangeIterator(&CBGetAccounterCursor(self.ptr)->it);
	free(self.ptr);
}
void CBFreeAccounterStorageUnspentOutputCursor(CBDepObject self){
	CBFreeAssociativeArray(&((CBAccounterStorageUnspentOutputCursor *)self.ptr)->spentOutputs);
	CBFreeDatabaseRangeIterator(&CBGetAccounterCursor(self.ptr)->it);
	free(self.ptr);
}
void CBFreeSpentOutputKeyPointer(void * key){
	free((uint8_t *)key - CB_ACCOUNT_OUTPUTS_OUTPUT_ID);
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
bool CBAccounterAdjustBalances(CBAccounterStorage * storage, uint8_t * low, uint8_t * high, uint8_t section, uint64_t accountID, int64_t adjustment, uint64_t * last){
	uint8_t branchSectionAccountTimeTxMin[33];
	uint8_t branchSectionAccountTimeTxMax[33];
	branchSectionAccountTimeTxMin[CB_BRANCH_ACCOUNT_TIME_TX_BRANCH] = section;
	branchSectionAccountTimeTxMax[CB_BRANCH_ACCOUNT_TIME_TX_BRANCH] = section;
	CBInt64ToArray(branchSectionAccountTimeTxMin, CB_BRANCH_ACCOUNT_TIME_TX_ACCOUNT_ID, accountID);
	CBInt64ToArray(branchSectionAccountTimeTxMax, CB_BRANCH_ACCOUNT_TIME_TX_ACCOUNT_ID, accountID);
	memcpy(branchSectionAccountTimeTxMin + CB_BRANCH_ACCOUNT_TIME_TX_TIMESTAMP, low, 16);
	memcpy(branchSectionAccountTimeTxMax + CB_BRANCH_ACCOUNT_TIME_TX_TIMESTAMP, high, 16);
	memset(branchSectionAccountTimeTxMin + CB_BRANCH_ACCOUNT_TIME_TX_TX_ID, 0, 8);
	memset(branchSectionAccountTimeTxMax + CB_BRANCH_ACCOUNT_TIME_TX_TX_ID, 0xFF, 8);
	CBDatabaseRangeIterator it = {branchSectionAccountTimeTxMin, branchSectionAccountTimeTxMax, storage->branchSectionAccountTimeTx};
	if (adjustment != 0) {
		CBIndexFindStatus status = CBDatabaseRangeIteratorFirst(&it);
		if (status == CB_DATABASE_INDEX_ERROR) {
			CBLogError("There was a problem finding the first branch section account time tx details.");
			return false;
		}
		if (status == CB_DATABASE_INDEX_FOUND) for (;;) {
			// Get the data
			uint8_t data[8];
			if (!CBDatabaseRangeIteratorRead(&it, data, 8, 0)) {
				CBLogError("Could not read the cumulative balance for adjustment");
				return false;
			}
			// Adjust balance
			uint64_t adjustedBal = CBArrayToInt64(data, 0) + adjustment;
			CBInt64ToArray(data, 0, adjustedBal);
			if (last)
				*last = adjustedBal;
			// Write adjusted balance
			uint8_t key[33];
			CBDatabaseRangeIteratorGetKey(&it, key);
			CBDatabaseWriteValue(storage->branchSectionAccountTimeTx, key, data, 8);
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
		// Adjustment is zero so do not adjust but get the last balance as it is
		CBIndexFindStatus status = CBDatabaseRangeIteratorLast(&it);
		if (status == CB_DATABASE_INDEX_ERROR) {
			CBLogError("There was a problem finding the first branch section account time tx details.");
			return false;
		}
		if (status == CB_DATABASE_INDEX_FOUND){
			uint8_t data[8];
			if (!CBDatabaseRangeIteratorRead(&it, data, 8, 0)) {
				CBLogError("Could not read the cumulative balance for adjustment");
				return false;
			}
			// Adjust balance
			if (last)
				*last = CBArrayToInt64(data, 0);
		}
		// Else do not change last
	}
	CBFreeDatabaseRangeIterator(&it);
	return true;
}
bool CBAccounterAdjustUnconfBalance(CBAccounterStorage * self, uint64_t accountID, uint64_t value){
	int64_t balance;
	if (! CBAccounterGetBranchAccountBalance((CBDepObject){.ptr=self}, CB_NO_BRANCH, accountID, &balance)) {
		CBLogError("Could not get the previous balance for unconfirmed transactions for adjustment.");
		return false;
	}
	balance += value;
	uint8_t accountUnconfBalanceKey[8], data[8];
	CBInt64ToArray(accountUnconfBalanceKey, 0, accountID);
	CBInt64ToArray(data, 0, labs(balance) | ((balance > 0) ? 0x8000000000000000 : 0));
	CBDatabaseWriteValue(self->accountUnconfBalance, accountUnconfBalanceKey, data, 8);
	return true;
}
bool CBAccounterUnconfirmedTransactionToBranch(CBDepObject self, void * vtx, uint32_t blockHeight, uint8_t branch){
	CBAccounterStorage * storage = self.ptr;
	CBTransaction * tx = vtx;
	uint8_t keyArr[36], newKey[33], data[8];
	uint8_t branchSectionID = CBAccounterGetLastBranchSectionID(storage, branch);
	// A previously branchless transaction has been found in a branch.
	// Loop through inputs and change the spent status key for the branch.
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
		// Loop through accounts
		uint8_t outputAccountsMin[21];
		uint8_t outputAccountsMax[21];
		memcpy(outputAccountsMin + CB_OUTPUT_ACCOUNTS_OUTPUT_ID, data, 8);
		memcpy(outputAccountsMax + CB_OUTPUT_ACCOUNTS_OUTPUT_ID, data, 8);
		memset(outputAccountsMin + CB_OUTPUT_ACCOUNTS_ACCOUNTS_ID, 0, 8);
		memset(outputAccountsMax + CB_OUTPUT_ACCOUNTS_ACCOUNTS_ID, 0xFF, 8);
		CBDatabaseRangeIterator it = {outputAccountsMin, outputAccountsMax, storage->outputAccounts};
		if (CBDatabaseRangeIteratorFirst(&it) != CB_DATABASE_INDEX_FOUND) {
			CBLogError("Could not get the first account for an output when removing an output spent status from branchless.");
			return false;
		}
		// Prepare account output status keys with the branch, zero height and output ID.
		keyArr[CB_ACCOUNT_OUTPUTS_BRANCH] = CB_NO_BRANCH;
		memset(keyArr + CB_ACCOUNT_OUTPUTS_HEIGHT, 0xFF, 4);
		memcpy(keyArr + CB_ACCOUNT_OUTPUTS_OUTPUT_ID, data, 8);
		newKey[CB_ACCOUNT_OUTPUTS_BRANCH] = branch;
		CBInt32ToArrayBigEndian(newKey, CB_ACCOUNT_OUTPUTS_HEIGHT, UINT32_MAX - blockHeight);
		memcpy(newKey + CB_ACCOUNT_OUTPUTS_OUTPUT_ID, data, 8);
		for (;;) {
			// Add the account ID into the keys
			uint8_t key[16];
			CBDatabaseRangeIteratorGetKey(&it, key);
			memcpy(keyArr + CB_ACCOUNT_OUTPUTS_ACCOUNT_ID, key + CB_OUTPUT_ACCOUNTS_ACCOUNTS_ID, 8);
			memcpy(newKey + CB_ACCOUNT_OUTPUTS_ACCOUNT_ID, key + CB_OUTPUT_ACCOUNTS_ACCOUNTS_ID, 8);
			// Now change the key
			if (! CBDatabaseChangeKey(storage->branchSectionAccountOutputs, keyArr, newKey, false)) {
				CBLogError("Could not remove a spent status for removing a branchless transaction.");
				return false;
			}
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
	// Loop through outputs and change references to the new branch
	for (uint32_t x = 0; x < tx->outputNum; x++) {
		// Get the output ID of this output, if it exists.
		memcpy(keyArr + CB_OUTPUT_HASH_AND_INDEX_TO_ID_HASH, CBTransactionGetHash(tx), 32);
		CBInt32ToArray(keyArr, CB_OUTPUT_HASH_AND_INDEX_TO_ID_INDEX, x);
		uint32_t len;
		if (! CBDatabaseGetLength(storage->outputHashAndIndexToID, keyArr, &len)) {
			CBLogError("Unable to obtain length for an outputHashAndIndexToID entry when a tx enters a branch.");
			return false;
		}
		if (len != CB_DOESNT_EXIST) {
			// The output ID exists so get it
			if (CBDatabaseReadValue(storage->outputHashAndIndexToID, keyArr, data, 8, 0, false) != CB_DATABASE_INDEX_FOUND) {
				CBLogError("Could not get the ID of a branchless output so we can move the spent status to a branch");
				return false;
			}
			// Loop through the output's accounts and change the unspent output references to the branch the output is being added to.
			uint8_t outputAccountsMin[16];
			uint8_t outputAccountsMax[16];
			memcpy(outputAccountsMin + CB_OUTPUT_ACCOUNTS_OUTPUT_ID, data, 8);
			memcpy(outputAccountsMax + CB_OUTPUT_ACCOUNTS_OUTPUT_ID, data, 8);
			memset(outputAccountsMin + CB_OUTPUT_ACCOUNTS_ACCOUNTS_ID, 0, 8);
			memset(outputAccountsMax + CB_OUTPUT_ACCOUNTS_ACCOUNTS_ID, 0xFF, 8);
			CBDatabaseRangeIterator it = {outputAccountsMin, outputAccountsMax, storage->outputAccounts};
			if (CBDatabaseRangeIteratorFirst(&it) != CB_DATABASE_INDEX_FOUND){
				CBLogError("Couldn't find outputAccount data for an output that has entered a branch");
				return false;
			}
			for (;;) {
				// Get the output account key
				uint8_t key[16];
				CBDatabaseRangeIteratorGetKey(&it, key);
				// Create key for unconfirmed output
				keyArr[CB_ACCOUNT_OUTPUTS_BRANCH] = CB_NO_BRANCH;
				memcpy(keyArr + CB_ACCOUNT_OUTPUTS_ACCOUNT_ID, key + CB_OUTPUT_ACCOUNTS_ACCOUNTS_ID, 8);
				memset(keyArr + CB_ACCOUNT_OUTPUTS_HEIGHT, 0xFF, 4); // Height is zero for branchless outputs thus UINT32_MAX minus zero gives 0xFFFFFFFF
				memcpy(keyArr + CB_ACCOUNT_OUTPUTS_OUTPUT_ID, data, 8); // Output Id is in data and was read above
				// Get spend status
				uint8_t unspent;
				if (CBDatabaseReadValue(storage->branchSectionAccountOutputs, keyArr, &unspent, 1, 0, false) != CB_DATABASE_INDEX_FOUND) {
					CBLogError("Could not get the spend status of an output from an unconfirmed transaction.");
					return false;
				}
				// Create key for new output entry in branch
				newKey[CB_ACCOUNT_OUTPUTS_BRANCH] = branchSectionID;
				memcpy(newKey + CB_ACCOUNT_OUTPUTS_ACCOUNT_ID, key + CB_OUTPUT_ACCOUNTS_ACCOUNTS_ID, 8);
				CBInt32ToArrayBigEndian(newKey, CB_ACCOUNT_OUTPUTS_HEIGHT, UINT32_MAX - blockHeight);
				memcpy(newKey + CB_ACCOUNT_OUTPUTS_OUTPUT_ID, data, 8);
				if (unspent) {
					// The output is unspent. We should move it to the branch as unspent.
					// Change key
					if (! CBDatabaseChangeKey(storage->branchSectionAccountOutputs, keyArr, newKey, false)) {
						CBLogError("Could not change an unspent output entry to be on a branch.");
						return false;
					}
				}else
					// The output was spent by another transaction, so we keep this entry but add an unspent entry to the branch
					CBDatabaseWriteValue(storage->branchSectionAccountOutputs, newKey, (uint8_t []){true}, 1);
				// Iterate to the next account if available
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
		}
	}
	// Get the transaction ID
	memcpy(keyArr, CBTransactionGetHash(tx), 32);
	if (CBDatabaseReadValue(storage->txHashToID, keyArr, data, 8, 0, false) != CB_DATABASE_INDEX_FOUND) {
		CBLogError("Could not get a transaction ID for adding to a branch.");
		return false;
	}
	uint64_t txID = CBArrayToInt64(data, 0);
	// Get the next order num
	uint8_t * nextOrderNumData = storage->database->current.extraData + CB_ACCOUNTER_DETAILS_BRANCH_SECTION_NEXT_ORDER_NUM + branch*8;
	// Increment the next order
	CBInt64ToArrayBigEndian(storage->database->current.extraData, CB_ACCOUNTER_DETAILS_BRANCH_SECTION_NEXT_ORDER_NUM + branch*8, CBArrayToInt64BigEndian(nextOrderNumData, 0) + 1);
	// Get the timestamp of the transaction
	uint8_t timestamp[8];
	CBInt64ToArray(keyArr, 0, txID);
	if (CBDatabaseReadValue(storage->txDetails, keyArr, timestamp, 8, CB_TX_DETAILS_TIMESTAMP, false) != CB_DATABASE_INDEX_FOUND) {
		CBLogError("Could not read a transactions timestamp for moving a branchless transaction into a branch.");
		return false;
	}
	// Record transaction as not being unconfirmed
	CBDatabaseWriteValueSubSection(storage->txDetails, keyArr, (uint8_t []){false}, 1, CB_TX_DETAILS_IS_UNCONF);
	// Add branch transaction details
	keyArr[CB_BRANCH_TX_DETAILS_BRANCH] = branchSectionID;
	CBInt64ToArray(keyArr, CB_BRANCH_TX_DETAILS_TX_ID, txID);
	memcpy(keyArr + CB_BRANCH_TX_DETAILS_ORDER_NUM, nextOrderNumData, 8);
	CBInt32ToArrayBigEndian(data, CB_BRANCH_TX_DETAILS_HEIGHT, blockHeight);
	CBDatabaseWriteValue(storage->branchSectionTxDetails, keyArr, data, 4);
	// Add branch, height and ID entry.
	keyArr[CB_TX_HEIGHT_BRANCH_AND_ID_BRANCH] = branchSectionID;
	CBInt32ToArrayBigEndian(keyArr, CB_TX_HEIGHT_BRANCH_AND_ID_HEIGHT, blockHeight);
	CBInt64ToArray(keyArr, CB_TX_HEIGHT_BRANCH_AND_ID_TX_ID, txID);
	memcpy(keyArr + CB_TX_HEIGHT_BRANCH_AND_ID_ORDER_NUM, nextOrderNumData, 8);
	// Write the txBranchSectionHeightAndID entry
	CBDatabaseWriteValue(storage->txBranchSectionHeightAndID, keyArr, NULL, 0);
	// Loop through transaction accounts
	uint8_t txAccountMin[16];
	uint8_t txAccountMax[16];
	CBInt64ToArray(txAccountMin, CB_TX_ACCOUNTS_TX_ID, txID);
	CBInt64ToArray(txAccountMax, CB_TX_ACCOUNTS_TX_ID, txID);
	memset(txAccountMin + CB_TX_ACCOUNTS_ACCOUNT_ID, 0, 8);
	memset(txAccountMax + CB_TX_ACCOUNTS_ACCOUNT_ID, 0xFF, 8);
	CBDatabaseRangeIterator it = {txAccountMin, txAccountMax, storage->txAccounts};
	if (CBDatabaseRangeIteratorFirst(&it) != CB_DATABASE_INDEX_FOUND) {
		CBLogError("Couldn't find first transaction account data when looping through for a transaction being added to a branch.");
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
		// Adjust the unconf balance
		if (! CBAccounterAdjustUnconfBalance(storage, accountID, -value)) {
			CBLogError("Could not adjust the balance of unconfirmed transactions for an account when removing one that enters a branch.");
			CBFreeDatabaseRangeIterator(&it);
			return false;
		}
		// Get cumultive balance
		uint64_t balance;
		CBAccounterGetLastAccountBranchSectionBalance(storage, accountID, branchSectionID, UINT64_MAX, &balance);
		balance += value;
		// Change the branch account time transaction entry, for the new branch.
		keyArr[CB_BRANCH_ACCOUNT_TIME_TX_BRANCH] = CB_NO_BRANCH;
		CBInt64ToArray(keyArr, CB_BRANCH_ACCOUNT_TIME_TX_ACCOUNT_ID, accountID);
		memcpy(keyArr + CB_BRANCH_ACCOUNT_TIME_TX_TIMESTAMP, timestamp, 8);
		memset(keyArr + CB_BRANCH_ACCOUNT_TIME_TX_ORDERNUM, 0, 8);
		CBInt64ToArray(keyArr, CB_BRANCH_ACCOUNT_TIME_TX_TX_ID, txID);
		// Make new key
		newKey[CB_BRANCH_ACCOUNT_TIME_TX_BRANCH] = branchSectionID;
		CBInt64ToArray(newKey, CB_BRANCH_ACCOUNT_TIME_TX_ACCOUNT_ID, accountID);
		memcpy(newKey + CB_BRANCH_ACCOUNT_TIME_TX_TIMESTAMP, timestamp, 8);
		memcpy(newKey + CB_BRANCH_ACCOUNT_TIME_TX_ORDERNUM, nextOrderNumData, 8);
		CBInt64ToArray(newKey, CB_BRANCH_ACCOUNT_TIME_TX_TX_ID, txID);
		if (! CBDatabaseChangeKey(storage->branchSectionAccountTimeTx, keyArr, newKey, false)){
			CBLogError("Could not change key for the branch account time transaction entry for a transaction entering a branch.");
			return false;
		}
		// Now write balance
		CBInt64ToArray(data, 0, balance);
		CBDatabaseWriteValue(storage->branchSectionAccountTimeTx, newKey, data, 8);
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
	return true;
}
bool CBAccounterMakeOutputSpent(CBAccounterStorage * self, CBPrevOut * prevOut, uint8_t branch, uint32_t blockHeight, CBAssociativeArray * txInfo){
	uint8_t branchSectionID = CBAccounterGetLastBranchSectionID(self, branch);
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
	// For each account that owns the output remove or add from/to the unspent outputs.
	uint8_t minOutputAccountKey[16];
	uint8_t maxOutputAccountKey[16];
	memcpy(minOutputAccountKey + CB_OUTPUT_ACCOUNTS_OUTPUT_ID, data, 8);
	memcpy(maxOutputAccountKey + CB_OUTPUT_ACCOUNTS_OUTPUT_ID, data, 8);
	memset(minOutputAccountKey + CB_OUTPUT_ACCOUNTS_ACCOUNTS_ID, 0, 8);
	memset(maxOutputAccountKey + CB_OUTPUT_ACCOUNTS_ACCOUNTS_ID, 0xFF, 8);
	CBDatabaseRangeIterator it = {minOutputAccountKey, maxOutputAccountKey, self->outputAccounts};
	if (CBDatabaseRangeIteratorFirst(&it) != CB_DATABASE_INDEX_FOUND) {
		CBLogError("Couldn't get the first output account entry");
		return false;
	}
	uint8_t outputIDBuf[8];
	memcpy(outputIDBuf, data, 8);
	for (;;) {
		// Get the ouput account key
		uint8_t outputAccountKey[16];
		CBDatabaseRangeIteratorGetKey(&it, outputAccountKey);
		// Create the account output key
		keyArr[CB_ACCOUNT_OUTPUTS_BRANCH] = branchSectionID;
		memcpy(keyArr + CB_ACCOUNT_OUTPUTS_ACCOUNT_ID, outputAccountKey + CB_OUTPUT_ACCOUNTS_ACCOUNTS_ID, 8);
		CBInt32ToArrayBigEndian(keyArr, CB_ACCOUNT_OUTPUTS_HEIGHT, UINT32_MAX - blockHeight);
		memcpy(keyArr + CB_ACCOUNT_OUTPUTS_OUTPUT_ID, outputIDBuf, 8);
		// Write output as spent.
		CBDatabaseWriteValue(self->branchSectionAccountOutputs, keyArr, (uint8_t []){false}, 1);
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
void CBAccounterCursorInit(CBAccounterStorageCursor * cursor, CBAccounterStorage * storage, uint8_t branch){
	uint8_t x = CB_MAX_BRANCH_CACHE - 1;
	cursor->storage = storage;
	cursor->branchSections[x] = CBAccounterGetLastBranchSectionID(storage, branch);
	if (cursor->branchSections[x] != CB_NO_BRANCH) {
		for (;x--;) {
			cursor->branchSections[x] = CBAccounterGetParentBranchSection(storage, cursor->branchSections[x + 1]);
			if (cursor->branchSections[x] == CB_NO_PARENT) {
				x++;
				break;
			}
		}
	}
	cursor->currentSection = x;
	cursor->started = false;
}
bool CBAccounterDeleteBranch(CBDepObject self, uint8_t branch){
	// Delete last section data.
	CBAccounterStorage * storage = self.ptr;
	uint8_t branchSectionID = CBAccounterGetLastBranchSectionID(storage, branch);
	uint8_t keyArr[8];
	// Loop through transactions
	uint8_t txBranchSectionHeightAndIDMin[21];
	uint8_t txBranchSectionHeightAndIDMax[21];
	txBranchSectionHeightAndIDMin[CB_TX_HEIGHT_BRANCH_AND_ID_BRANCH] = branchSectionID;
	txBranchSectionHeightAndIDMax[CB_TX_HEIGHT_BRANCH_AND_ID_BRANCH] = branchSectionID;
	memset(txBranchSectionHeightAndIDMin + CB_TX_HEIGHT_BRANCH_AND_ID_HEIGHT, 0, 20);
	memset(txBranchSectionHeightAndIDMax + CB_TX_HEIGHT_BRANCH_AND_ID_HEIGHT, 0xFF, 20);
	CBDatabaseRangeIterator it = {txBranchSectionHeightAndIDMin, txBranchSectionHeightAndIDMax, storage->txBranchSectionHeightAndID};
	CBIndexFindStatus res = CBDatabaseRangeIteratorFirst(&it);
	if (res == CB_DATABASE_INDEX_ERROR) {
		CBLogError("There was an error whilst attempting to find the first tx branch height and ID entry for deleting a branch.");
		return false;
	}
	if (res == CB_DATABASE_INDEX_FOUND) for(;;) {
		// Get the key for the tx branch height and ID
		uint8_t key[21];
		CBDatabaseRangeIteratorGetKey(&it, key);
		// Remove the tx from the branch
		memcpy(keyArr, key + CB_TX_HEIGHT_BRANCH_AND_ID_TX_ID, 8);
		// Get the transaction hash
		uint8_t txHash[32];
		if (CBDatabaseReadValue(storage->txDetails, keyArr, txHash, 32, CB_TX_DETAILS_HASH, false) != CB_DATABASE_INDEX_FOUND) {
			CBLogError("Could not get a transaction hash details.");
			CBFreeDatabaseRangeIterator(&it);
			return false;
		}
		uint8_t orderNumData[8];
		uint8_t heightData[4];
		memcpy(orderNumData, key + CB_TX_HEIGHT_BRANCH_AND_ID_ORDER_NUM, 8);
		memcpy(heightData, key + CB_TX_HEIGHT_BRANCH_AND_ID_HEIGHT, 4);
		if (! CBAccounterRemoveTransactionFromBranch(storage, CBArrayToInt64(keyArr, 0), txHash, branch, orderNumData, heightData)) {
			CBLogError("Could not remove a transaction from a branch.");
			CBFreeDatabaseRangeIterator(&it);
			return false;
		}
		// Iterate to the next tx if possible
		CBIndexFindStatus status = CBDatabaseRangeIteratorNext(&it);
		if (status == CB_DATABASE_INDEX_ERROR) {
			CBLogError("Error whilst iterating through branch transactions.");
			CBFreeDatabaseRangeIterator(&it);
			return false;
		}
		if (status == CB_DATABASE_INDEX_NOT_FOUND)
			break;
	}
	CBFreeDatabaseRangeIterator(&it);
	// Make parent last section and mark this section as deleted
	storage->database->current.extraData[25 + branch] = storage->database->current.extraData[25 + CB_MAX_BRANCH_CACHE + branchSectionID];
	storage->database->current.extraData[25 + CB_MAX_BRANCH_CACHE + branchSectionID] = CB_UNUSED_SECTION;
	return true;
}
CBErrBool CBAccounterFoundTransaction(CBDepObject self, void * vtx, uint32_t blockHeight, uint64_t time, uint8_t branch, CBTransactionAccountDetailList ** details){
	CBAccounterStorage * storage = self.ptr;
	CBTransaction * tx = vtx;
	CBTransactionAccountDetailList ** curDetails = details;
	uint8_t branchSectionID = CBAccounterGetLastBranchSectionID(storage, branch);
	uint8_t keyArr[33], data[64];
	// Create transaction flow information array.
	CBAssociativeArray txInfo;
	CBInitAssociativeArray(&txInfo, CBCompareUInt32, NULL, free);
	// Look through inputs for any unspent outputs being spent.
	for (uint32_t x = 0; x < tx->inputNum; x++) {
		// Make previous output spent for this branch.
		if (! CBAccounterMakeOutputSpent(storage, &tx->inputs[x]->prevOut, branch, blockHeight, &txInfo)) {
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
		CBDatabaseRangeIterator it = {minWatchedHashKey, maxWatchedHashKey, storage->watchedHashes};
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
			// Now loop through accounts. Set outputID to the output accounts key and the outputId and branch/height to the account outputs key so that we don't keep doing it.
			uint8_t outputAccountsKey[16];
			uint8_t accountOutputsKey[21];
			CBInt64ToArray(outputAccountsKey, CB_OUTPUT_ACCOUNTS_OUTPUT_ID, outputID);
			accountOutputsKey[CB_ACCOUNT_OUTPUTS_BRANCH] = branchSectionID;
			CBInt32ToArrayBigEndian(accountOutputsKey, CB_ACCOUNT_OUTPUTS_HEIGHT, UINT32_MAX - blockHeight);
			CBInt64ToArray(accountOutputsKey, CB_ACCOUNT_OUTPUTS_OUTPUT_ID, outputID);
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
				// Add an entry to the outputs for the account
				CBInt64ToArray(accountOutputsKey, CB_ACCOUNT_OUTPUTS_ACCOUNT_ID, accountID);
				CBDatabaseWriteValue(storage->branchSectionAccountOutputs, accountOutputsKey, (uint8_t []){true}, 1);
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
			CBFreeDatabaseRangeIterator(&it);
		}
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
			data[CB_TX_DETAILS_BRANCH_INSTANCES] = 1;
			memcpy(data + CB_TX_DETAILS_HASH, CBTransactionGetHash(tx), 32);
			CBInt64ToArrayBigEndian(data, CB_TX_DETAILS_TIMESTAMP, time);
			data[CB_TX_DETAILS_IS_UNCONF] = branch == CB_NO_BRANCH;
			CBDatabaseWriteValue(storage->txDetails, keyArr, data, 42);
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
			// Increase the number of branches that own this transaction
			CBInt64ToArray(keyArr, 0, txID);
			if (CBDatabaseReadValue(storage->txDetails, keyArr, data, 1, CB_TX_DETAILS_BRANCH_INSTANCES, false) != CB_DATABASE_INDEX_FOUND) {
				CBLogError("Couldn't read the branch instance count for a transaction.");
				return CB_ERROR;
			}
			data[0]++;
			CBDatabaseWriteValueSubSection(storage->txDetails, keyArr, data, 1, CB_TX_DETAILS_BRANCH_INSTANCES);
			// Get time
			if (CBDatabaseReadValue(storage->txDetails, keyArr, data, 8, CB_TX_DETAILS_TIMESTAMP, false) != CB_DATABASE_INDEX_FOUND) {
				CBLogError("Could not read a transaction's timestamp when getting a transaction time.");
				return false;
			}
			time = CBArrayToInt64BigEndian(data, 0);
			if (branch == CB_NO_BRANCH)
				// Write as unconfirmed
				CBDatabaseWriteValueSubSection(storage->txDetails, keyArr, (uint8_t []){true}, 1, CB_TX_DETAILS_IS_UNCONF);
		}
		uint8_t * nextOrderNumData;
		if (branch != CB_NO_BRANCH) {
			// Get the next order num
			nextOrderNumData = storage->database->current.extraData + CB_ACCOUNTER_DETAILS_BRANCH_SECTION_NEXT_ORDER_NUM + branch*8;
			// Increment the next order num
			CBInt64ToArrayBigEndian(storage->database->current.extraData, CB_ACCOUNTER_DETAILS_BRANCH_SECTION_NEXT_ORDER_NUM + branch*8,  CBArrayToInt64BigEndian(nextOrderNumData, 0) + 1);
			// Write the value
			// Set the block height and timestamp for the branch and this transaction
			keyArr[CB_BRANCH_TX_DETAILS_BRANCH] = branchSectionID;
			CBInt64ToArray(keyArr, CB_BRANCH_TX_DETAILS_TX_ID, txID);
			memcpy(keyArr + CB_BRANCH_TX_DETAILS_ORDER_NUM, nextOrderNumData, 8);
			CBInt32ToArrayBigEndian(data, CB_BRANCH_TX_DETAILS_HEIGHT, blockHeight);
			CBDatabaseWriteValue(storage->branchSectionTxDetails, keyArr, data, 4);
			// Create the branch and block height entry for the transaction
			keyArr[CB_TX_HEIGHT_BRANCH_AND_ID_BRANCH] = branchSectionID;
			CBInt32ToArrayBigEndian(keyArr, CB_TX_HEIGHT_BRANCH_AND_ID_HEIGHT, blockHeight);
			memcpy(keyArr + CB_TX_HEIGHT_BRANCH_AND_ID_ORDER_NUM, nextOrderNumData, 8);
			CBInt64ToArray(keyArr, CB_TX_HEIGHT_BRANCH_AND_ID_TX_ID, txID);
			CBDatabaseWriteValue(storage->txBranchSectionHeightAndID, keyArr, NULL, 0);
		}
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
						else if (CBTransactionOuputGetHash(tx->outputs[1], data+ CB_ACCOUNT_TX_DETAILS_ADDR))
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
			// Write the balance
			if (branch == CB_NO_BRANCH) {
				// Adust unconf balance
				if (! CBAccounterAdjustUnconfBalance(storage, info->accountID, valueInt)) {
					CBLogError("Could not adjust the balance of unconfirmed transactions for an account when a new transaction has been found.");
					CBFoundTransactionReturnError
				}
				// Set a zero order num for the key array for the branch account time transaction entry.
				memset(keyArr + CB_BRANCH_ACCOUNT_TIME_TX_ORDERNUM, 0, 8);
				// Do not set cumulative balance. Not needed.
			}else{
				// Else write the cumulative balance for the transaction and get the next order number
				uint64_t balance;
				// Get balance upto this transaction
				if (! CBAccounterGetLastAccountBranchSectionBalance(storage, info->accountID, branchSectionID, time, &balance)) {
					CBLogError("Could not read the cumulative balance for a transaction.");
					CBFoundTransactionReturnError
				}
				// Set the order num into the key array for the branch account time transaction entry.
				memcpy(keyArr + CB_BRANCH_ACCOUNT_TIME_TX_ORDERNUM, nextOrderNumData, 8);
				// Set balance into data array
				balance += valueInt;
				CBInt64ToArray(data, 0, balance);
			}
			// Create the branch account time transaction entry.
			keyArr[CB_BRANCH_ACCOUNT_TIME_TX_BRANCH] = branchSectionID;
			CBInt64ToArray(keyArr, CB_BRANCH_ACCOUNT_TIME_TX_ACCOUNT_ID, info->accountID);
			CBInt64ToArrayBigEndian(keyArr, CB_BRANCH_ACCOUNT_TIME_TX_TIMESTAMP, time);
			CBInt64ToArray(keyArr, CB_BRANCH_ACCOUNT_TIME_TX_TX_ID, txID);
			CBDatabaseWriteValue(storage->branchSectionAccountTimeTx, keyArr, data, 8);
			// Adjust the balances of any future transactions already stored
			CBInt64ToArrayBigEndian(keyArr, CB_BRANCH_ACCOUNT_TIME_TX_ORDERNUM, CBArrayToInt64BigEndian(keyArr, CB_BRANCH_ACCOUNT_TIME_TX_ORDERNUM) + 1);
			uint8_t high[16];
			memset(high, 0xFF, 16);
			if (!CBAccounterAdjustBalances(storage, keyArr + CB_BRANCH_ACCOUNT_TIME_TX_TIMESTAMP, high, branchSectionID, info->accountID, valueInt, NULL)) {
				CBLogError("Could not adjust the balances of transactions after one inserted.");
				return false;
			}
			// Get the next transaction account information.
			if (CBAssociativeArrayIterate(&txInfo, &pos))
				break;
		}
	}else
		// Not got any transactions for us.
		return CB_FALSE;
	return CB_TRUE;
}
bool CBAccounterGetBranchAccountBalance(CBDepObject self, uint8_t branch, uint64_t accountID, int64_t * balance){
	CBAccounterStorage * storage = self.ptr;
	if (branch == CB_NO_BRANCH) {
		uint8_t key[8], data[8];
		CBInt64ToArray(key, 0, accountID);
		CBIndexFindStatus res = CBDatabaseReadValue(storage->accountUnconfBalance, key, data, 8, 0, false);
		if (res == CB_DATABASE_INDEX_ERROR) {
			CBLogError("There was an error trying to get the balance of unconfirmed transactions.");
			return false;
		}
		if (res == CB_DATABASE_INDEX_FOUND) {
			uint64_t txBalanceRaw = CBArrayToInt64(data, 0);
			*balance = txBalanceRaw & 0x7FFFFFFFFFFFFFFF;
			if (! (txBalanceRaw & 0x8000000000000000))
				*balance = -*balance;
		}else
			*balance = 0;
		return true;
	}
	uint64_t ubalance;
	if (! CBAccounterGetLastAccountBranchSectionBalance(storage, accountID, CBAccounterGetLastBranchSectionID(storage, branch), UINT64_MAX, &ubalance)) {
		CBLogError("Could not get the balance for a branch.");
		return false;
	}
	*balance = ubalance;
	return true;
}
CBErrBool CBAccounterGetNextTransaction(CBDepObject cursoru, CBTransactionDetails * details){
	CBAccounterStorageTxCursor * txCursor = cursoru.ptr;
	CBAccounterStorageCursor * cursor = CBGetAccounterCursor(txCursor);
	CBIndexFindStatus res;
	for (;; cursor->currentSection++) {
		if (cursor->started)
			res = CBDatabaseRangeIteratorNext(&cursor->it);
		else{
			txCursor->branchSectionAccountTimeTxMin[CB_BRANCH_ACCOUNT_TIME_TX_BRANCH] = cursor->branchSections[cursor->currentSection];
			txCursor->branchSectionAccountTimeTxMax[CB_BRANCH_ACCOUNT_TIME_TX_BRANCH] = cursor->branchSections[cursor->currentSection];
			cursor->it = (CBDatabaseRangeIterator){txCursor->branchSectionAccountTimeTxMin, txCursor->branchSectionAccountTimeTxMax, cursor->storage->branchSectionAccountTimeTx};
			res = CBDatabaseRangeIteratorFirst(&cursor->it);
			cursor->started = true;
		}
		if (res == CB_DATABASE_INDEX_ERROR) {
			CBLogError("There was an error trying to find the first transaction for a transaction cursor.");
			return CB_ERROR;
		}
		if (res == CB_DATABASE_INDEX_FOUND)
			break;
		if (cursor->currentSection == CB_MAX_BRANCH_CACHE - 1)
			return CB_FALSE;
		cursor->started = false;
	}
	// Get the key
	uint8_t key[33];
	CBDatabaseRangeIteratorGetKey(&cursor->it, key);
	// Get the transaction hash
	uint8_t keyArr[17], data[8];
	memcpy(keyArr, key + CB_BRANCH_ACCOUNT_TIME_TX_TX_ID, 8);
	if (CBDatabaseReadValue(cursor->storage->txDetails, keyArr, details->txHash, 32, CB_TX_DETAILS_HASH, false) != CB_DATABASE_INDEX_FOUND) {
		CBLogError("Could not read a transaction's hash.");
		return CB_ERROR;
	}
	// Get the address and value
	memcpy(keyArr + CB_ACCOUNT_TX_DETAILS_ACCOUNT_ID, txCursor->branchSectionAccountTimeTxMin + CB_BRANCH_ACCOUNT_TIME_TX_ACCOUNT_ID, 8);
	memcpy(keyArr + CB_ACCOUNT_TX_DETAILS_TX_ID, key + CB_BRANCH_ACCOUNT_TIME_TX_TX_ID, 8);
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
	if (key[CB_BRANCH_ACCOUNT_TIME_TX_BRANCH] == CB_NO_BRANCH)
		details->height = 0;
	else{
		keyArr[CB_BRANCH_TX_DETAILS_BRANCH] = key[CB_BRANCH_ACCOUNT_TIME_TX_BRANCH];
		memcpy(keyArr + CB_BRANCH_TX_DETAILS_ORDER_NUM, key + CB_BRANCH_ACCOUNT_TIME_TX_ORDERNUM, 8);
		memcpy(keyArr + CB_BRANCH_TX_DETAILS_TX_ID, key + CB_BRANCH_ACCOUNT_TIME_TX_TX_ID, 8);
		if (CBDatabaseReadValue(cursor->storage->branchSectionTxDetails, keyArr, data, 4, CB_BRANCH_TX_DETAILS_HEIGHT, false) != CB_DATABASE_INDEX_FOUND) {
			CBLogError("Could not read the height of a transaction in a branch");
			return false;
		}
		details->height = CBArrayToInt32BigEndian(data, 0);
	}
	// Add the timestamp
	details->timestamp = CBArrayToInt64BigEndian(key, CB_BRANCH_ACCOUNT_TIME_TX_TIMESTAMP);
	return CB_TRUE;
}
CBErrBool CBAccounterGetNextUnspentOutput(CBDepObject cursoru, CBUnspentOutputDetails * details){
	CBAccounterStorageUnspentOutputCursor * uoCursor = cursoru.ptr;
	CBAccounterStorageCursor * cursor = CBGetAccounterCursor(uoCursor);
	CBIndexFindStatus res;
	for (;; cursor->currentSection--) {
		if (cursor->started)
			res = CBDatabaseRangeIteratorNext(&cursor->it);
		else{
			uoCursor->branchSectionAccountOutputsMin[CB_ACCOUNT_OUTPUTS_BRANCH] = cursor->branchSections[cursor->currentSection];
			uoCursor->branchSectionAccountOutputsMax[CB_ACCOUNT_OUTPUTS_BRANCH] = cursor->branchSections[cursor->currentSection];
			cursor->it = (CBDatabaseRangeIterator){uoCursor->branchSectionAccountOutputsMin, uoCursor->branchSectionAccountOutputsMax, cursor->storage->branchSectionAccountOutputs};
			res = CBDatabaseRangeIteratorFirst(&cursor->it);
			cursor->started = true;
		}
		if (res == CB_DATABASE_INDEX_ERROR) {
			CBLogError("There was an error trying to find an unspent output for an account.");
			return CB_ERROR;
		}
		if (res == CB_DATABASE_INDEX_FOUND){
			// Get the key
			uint8_t * key = malloc(21);
			CBDatabaseRangeIteratorGetKey(&cursor->it, key);
			// Ensure unspent
			bool unspent = true;
			// See if we have figured this as spent already.
			uint8_t * outputKey = key + CB_ACCOUNT_OUTPUTS_OUTPUT_ID;
			CBFindResult res = CBAssociativeArrayFind(&uoCursor->spentOutputs, outputKey);
			if (res.found)
				unspent = false;
			// Read the spent status
			uint8_t spendStatus;
			if (unspent && ! CBDatabaseRangeIteratorRead(&cursor->it, &spendStatus, 1, 0)){
				CBLogError("There was an error trying to read if an unspent output is active or not for an account.");
				return CB_ERROR;
			}
			if (unspent && ! spendStatus)
				unspent = false;
			if (unspent && cursor->branchSections[cursor->currentSection] != CB_NO_BRANCH) {
				// Check that this output is not spent in a branchless transaction.
				uint32_t len;
				uint8_t keyArr[21];
				keyArr[0] = CB_NO_BRANCH;
				memcpy(keyArr + CB_ACCOUNT_OUTPUTS_ACCOUNT_ID, key + CB_ACCOUNT_OUTPUTS_ACCOUNT_ID, 8);
				memset(keyArr + CB_ACCOUNT_OUTPUTS_HEIGHT, 0xFF, 4); // Block height is made zero on branchless therefore UINT32_MAX minus zero gives 0xFFFFFFFF
				memcpy(keyArr + CB_ACCOUNT_OUTPUTS_OUTPUT_ID, key + CB_ACCOUNT_OUTPUTS_OUTPUT_ID, 8);
				if (! CBDatabaseGetLength(cursor->storage->branchSectionAccountOutputs, keyArr, &len)) {
					CBLogError("Couldn't get the branchless spent status for an output.");
					return CB_ERROR;
				}
				if (len != CB_DOESNT_EXIST)
					// It exists so therefore it has been spent as it can't be another output of the same.
					unspent = false;
			}
			if (! unspent){
				// Spent, try next. Also record that this output is spent if we have not already.
				if (! res.found)
					CBAssociativeArrayInsert(&uoCursor->spentOutputs, outputKey, res.position, NULL);
				return CBAccounterGetNextUnspentOutput(cursoru, details);
			}
			// Unspent. Get output details
			uint8_t data[64];
			if (CBDatabaseReadValue(cursor->storage->outputDetails, outputKey, data, 64, 0, false)!= CB_DATABASE_INDEX_FOUND) {
				CBLogError("Couldn't get the details for an output.");
				return CB_ERROR;
			}
			// Now free the key
			free(key);
			details->value = CBArrayToInt64(data, CB_OUTPUT_DETAILS_VALUE);
			memcpy(details->txHash, data + CB_OUTPUT_DETAILS_TX_HASH, 32);
			details->index = CBArrayToInt32(data, CB_OUTPUT_DETAILS_INDEX);
			memcpy(details->watchedHash, data + CB_OUTPUT_DETAILS_ID_HASH, 20);
			return CB_TRUE;
		}
		if (cursor->currentSection == CB_MAX_BRANCH_CACHE - uoCursor->numSections)
			// No more to try
			return CB_FALSE;
		cursor->started = false;
	}
}
bool CBAccounterGetLastAccountBranchSectionBalance(CBAccounterStorage * storage, uint64_t accountID, uint8_t branchSectionID, uint64_t maxTime, uint64_t * balance){
	uint8_t branchSectionAccountTimeTxMin[33];
	uint8_t branchSectionAccountTimeTxMax[33];
	CBInt64ToArray(branchSectionAccountTimeTxMin, CB_BRANCH_ACCOUNT_TIME_TX_ACCOUNT_ID, accountID);
	CBInt64ToArray(branchSectionAccountTimeTxMax, CB_BRANCH_ACCOUNT_TIME_TX_ACCOUNT_ID, accountID);
	memset(branchSectionAccountTimeTxMin + CB_BRANCH_ACCOUNT_TIME_TX_TIMESTAMP, 0, 24);
	CBInt64ToArrayBigEndian(branchSectionAccountTimeTxMax, CB_BRANCH_ACCOUNT_TIME_TX_TIMESTAMP, maxTime);
	CBDatabaseRangeIterator lastTxIt = {branchSectionAccountTimeTxMin, branchSectionAccountTimeTxMax, storage->branchSectionAccountTimeTx};
	for (uint8_t tryBranchSectionID = branchSectionID;
		 tryBranchSectionID != CB_NO_PARENT;
		 tryBranchSectionID = CBAccounterGetParentBranchSection(storage, tryBranchSectionID)) {
		branchSectionAccountTimeTxMin[CB_BRANCH_ACCOUNT_TIME_TX_BRANCH] = tryBranchSectionID;
		branchSectionAccountTimeTxMax[CB_BRANCH_ACCOUNT_TIME_TX_BRANCH] = tryBranchSectionID;
		CBIndexFindStatus res = CBDatabaseRangeIteratorLast(&lastTxIt);
		if (res == CB_DATABASE_INDEX_ERROR) {
			CBLogError("Could not get the last transaction in a branch section to get the last balance for calculating a new balance.");
			return false;
		}
		if (res == CB_DATABASE_INDEX_FOUND){
			uint8_t data[8];
			if (! CBDatabaseRangeIteratorRead(&lastTxIt, data, 8, 0)) {
				CBLogError("Could not read the transaction cumulative balance for an account.");
				return false;
			}
			*balance = CBArrayToInt64(data, 0);
			return true;
		}
		CBFreeDatabaseRangeIterator(&lastTxIt);
	}
	*balance = 0;
	return true;
}
uint8_t CBAccounterGetLastBranchSectionID(CBAccounterStorage * self, uint8_t branch){
	if (branch == CB_NO_BRANCH || branch == CB_NO_PARENT)
		return branch;
	return self->database->current.extraData[CB_ACCOUNTER_DETAILS_BRANCH_LAST_SECTION_IDS + branch];
}
uint8_t CBAccounterGetParentBranchSection(CBAccounterStorage * self, uint8_t branchSectionID){
	if (branchSectionID == CB_NO_BRANCH)
		return CB_NO_PARENT;
	return self->database->current.extraData[CB_ACCOUNTER_DETAILS_BRANCH_SECTION_PARENTS + branchSectionID] - 1;
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
bool CBAccounterLostUnconfirmedTransaction(CBDepObject self, void * vtx){
	CBAccounterStorage * storage = self.ptr;
	CBTransaction * tx = vtx;
	uint8_t keyArr[36], data[8], txID[8];
	// Lost a transaction not on any branch.
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
		uint8_t unconf;
		// We have the tx ID, so determine if it is unconfirmed.
		if (CBDatabaseReadValue(storage->txDetails, txID, &unconf, 1, CB_TX_DETAILS_IS_UNCONF, false) != CB_DATABASE_INDEX_FOUND){
			CBLogError("Could not determine if a transaction appears as unconfirmed.");
			return false;
		}
		// Loop through account outputs
		uint8_t outputAccountsMin[21];
		uint8_t outputAccountsMax[21];
		memcpy(outputAccountsMin + CB_OUTPUT_ACCOUNTS_OUTPUT_ID, data, 8);
		memcpy(outputAccountsMax + CB_OUTPUT_ACCOUNTS_OUTPUT_ID, data, 8);
		memset(outputAccountsMin + CB_OUTPUT_ACCOUNTS_ACCOUNTS_ID, 0, 8);
		memset(outputAccountsMax + CB_OUTPUT_ACCOUNTS_ACCOUNTS_ID, 0xFF, 8);
		CBDatabaseRangeIterator it = (CBDatabaseRangeIterator){outputAccountsMin, outputAccountsMax, storage->outputAccounts};
		if (CBDatabaseRangeIteratorFirst(&it) != CB_DATABASE_INDEX_FOUND) {
			CBLogError("Could not get the first account for an output when removing an output spent status from branchless.");
			return false;
		}
		// Prepare account output status key with the branch, zero height and output ID.
		keyArr[CB_ACCOUNT_OUTPUTS_BRANCH] = CB_NO_BRANCH;
		memset(keyArr + CB_ACCOUNT_OUTPUTS_HEIGHT, 0xFF, 4);
		memcpy(keyArr + CB_ACCOUNT_OUTPUTS_OUTPUT_ID, data, 8);
		for (;;) {
			// Add the account ID into the key
			uint8_t key[16];
			CBDatabaseRangeIteratorGetKey(&it, key);
			memcpy(keyArr + CB_ACCOUNT_OUTPUTS_ACCOUNT_ID, key + CB_OUTPUT_ACCOUNTS_ACCOUNTS_ID, 8);
			// Now delete or change to unspent
			if (!unconf) {
				// Delete
				if (! CBDatabaseRemoveValue(storage->branchSectionAccountOutputs, keyArr, false)) {
					CBLogError("Could not remove a spent status for removing a branchless transaction.");
					return false;
				}
			}else
				// Change to unspent
				CBDatabaseWriteValue(storage->branchSectionAccountOutputs, keyArr, (uint8_t []){true}, 1);
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
	// Remove branch details for the transaction and lower the number of branches owning the transaction by one. If no more ownership remove the transaction details.
	// First get the transaction ID
	memcpy(keyArr, CBTransactionGetHash(tx), 32);
	if (CBDatabaseReadValue(storage->txHashToID, keyArr, data, 8, 0, false) != CB_DATABASE_INDEX_FOUND) {
		CBLogError("Could not read a transaction ID for a hash.");
		return false;
	}
	// Get the transaction ID
	uint64_t txIDInt = CBArrayToInt64(data, 0);
	// Remove the transaction from branchless
	if (! CBAccounterRemoveTransactionFromBranch(storage, txIDInt, CBTransactionGetHash(tx), CB_NO_BRANCH, NULL, NULL)) {
		CBLogError("Could not remove a transaction as branchless.");
		return false;
	}
	return true;
}
bool CBAccounterMergeAccountIntoAccount(CBDepObject self, uint64_t accountDest, uint64_t accountSrc){
	CBAccounterStorage * storage = self.ptr;
	// Add unconfirmed balance from dest to src
	int64_t balance;
	if (!CBAccounterGetBranchAccountBalance(self, CB_NO_BRANCH, accountSrc, &balance)) {
		CBLogError("Unable to get the balance of the source account when merging.");
		return false;
	}
	if (!CBAccounterAdjustUnconfBalance(storage, accountDest, balance)){
		CBLogError("Unable to adjust the balance of the destination account when merging.");
		return false;
	}
	// Loop through watched hashes of source account
	uint8_t accountWatchedHashesMin[28];
	uint8_t accountWatchedHashesMax[28];
	CBInt64ToArray(accountWatchedHashesMin, CB_ACCOUNT_WATCHED_HASHES_ACCOUNT_ID, accountSrc);
	CBInt64ToArray(accountWatchedHashesMax, CB_ACCOUNT_WATCHED_HASHES_ACCOUNT_ID, accountSrc);
	memset(accountWatchedHashesMin + CB_ACCOUNT_WATCHED_HASHES_HASH, 0, 20);
	memset(accountWatchedHashesMax + CB_ACCOUNT_WATCHED_HASHES_HASH, 0xFF, 20);
	CBDatabaseRangeIterator it = {accountWatchedHashesMin, accountWatchedHashesMax, storage->accountWatchedHashes};
	CBIndexFindStatus status = CBDatabaseRangeIteratorFirst(&it);
	if (status == CB_DATABASE_INDEX_ERROR) {
		CBLogError("There was a problem finding the first account transaction details entry.");
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
	it = (CBDatabaseRangeIterator){accountTxDetailsMin, accountTxDetailsMax, storage->accountTxDetails};
	status = CBDatabaseRangeIteratorFirst(&it);
	if (status == CB_DATABASE_INDEX_ERROR) {
		CBLogError("There was a problem finding the first account transaction details entry.");
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
	// For each branch section : 1. loop through branchSectionAccountTimeTx 2. loop through account outputs
	struct {
		int64_t addedBalance;
		uint8_t childSections[2];
		uint64_t last;
	} sectionQueue[CB_MAX_BRANCH_CACHE-1];
	uint8_t queueBack = 0;
	for (uint8_t currentSection = 0;;currentSection++) {
		// Get section
		if (currentSection != 0 && (currentSection-1)/2 == queueBack)
			break;
		int64_t addedBalance;
		uint8_t section;
		uint64_t last;
		if (currentSection == 0) {
			section = 0;
			addedBalance = 0;
			last = 0;
		}else{
			section = sectionQueue[(currentSection-1)/2].childSections[((currentSection-1) % 2)];
			addedBalance = sectionQueue[(currentSection-1)/2].addedBalance;
			last = sectionQueue[(currentSection-1)/2].last;
		}
		// branchSectionAccountTimeTx
		uint8_t branchSectionAccountTimeTxMin[33];
		uint8_t branchSectionAccountTimeTxMax[33];
		branchSectionAccountTimeTxMin[CB_BRANCH_ACCOUNT_TIME_TX_BRANCH] = section;
		branchSectionAccountTimeTxMax[CB_BRANCH_ACCOUNT_TIME_TX_BRANCH] = section;
		CBInt64ToArray(branchSectionAccountTimeTxMin, CB_BRANCH_ACCOUNT_TIME_TX_ACCOUNT_ID, accountSrc);
		CBInt64ToArray(branchSectionAccountTimeTxMax, CB_BRANCH_ACCOUNT_TIME_TX_ACCOUNT_ID, accountSrc);
		memset(branchSectionAccountTimeTxMin + CB_BRANCH_ACCOUNT_TIME_TX_TIMESTAMP, 0, 24);
		memset(branchSectionAccountTimeTxMax + CB_BRANCH_ACCOUNT_TIME_TX_TIMESTAMP, 0xFF, 24);
		CBDatabaseRangeIterator it = {branchSectionAccountTimeTxMin, branchSectionAccountTimeTxMax, storage->branchSectionAccountTimeTx};
		CBIndexFindStatus status = CBDatabaseRangeIteratorFirst(&it);
		if (status == CB_DATABASE_INDEX_ERROR) {
			CBLogError("There was a problem finding the first branch section account time tx details.");
			return false;
		}
		// Set last key as zero
		memset(keyArr + CB_BRANCH_ACCOUNT_TIME_TX_TIMESTAMP, 0, 16);
		if (status == CB_DATABASE_INDEX_FOUND) for (;;) {
			// Get the key
			uint8_t key[33];
			CBDatabaseRangeIteratorGetKey(&it, key);
			// Adjust balances for everything between last and this key if addedBalance is not 0
			if (!CBAccounterAdjustBalances(storage, keyArr + CB_BRANCH_ACCOUNT_TIME_TX_TIMESTAMP, key + CB_BRANCH_ACCOUNT_TIME_TX_TIMESTAMP, section, accountDest, addedBalance, &last)) {
				CBLogError("Could not adjust the cumulative balanaces.");
				return false;
			}
			// Adjust addedBalance by the value of the source account transaction details.
			memcpy(keyArr2 + CB_ACCOUNT_TX_DETAILS_TX_ID, key + CB_BRANCH_ACCOUNT_TIME_TX_TX_ID, 8);
			CBInt64ToArray(keyArr2, CB_ACCOUNT_TX_DETAILS_ACCOUNT_ID, accountSrc);
			uint8_t data[8];
			if (CBDatabaseReadValue(storage->accountTxDetails, keyArr2, data, 8, CB_ACCOUNT_TX_DETAILS_VALUE, false) != CB_DATABASE_INDEX_FOUND) {
				CBLogError("Couldn't read the value of a transaction for the source account.");
				return false;
			}
			int64_t value;
			CBAccounterUInt64ToInt64(CBArrayToInt64(data, 0), &value);
			addedBalance += value;
			last += value;
			CBInt64ToArray(data, 0, last);
			// Copy everything from the key except the account ID
			keyArr[CB_BRANCH_ACCOUNT_TIME_TX_BRANCH] = section;
			memcpy(keyArr + CB_BRANCH_ACCOUNT_TIME_TX_TIMESTAMP, key + CB_BRANCH_ACCOUNT_TIME_TX_TIMESTAMP, 24);
			// Set the destination account ID
			CBInt64ToArray(keyArr, CB_BRANCH_ACCOUNT_TIME_TX_ACCOUNT_ID, accountDest);
			// Write data for destination account
			CBDatabaseWriteValue(storage->branchSectionAccountTimeTx, keyArr, data, 8);
			// Increase key by one since we do not want to adjust the value added. Do this with the ordernum
			CBInt64ToArrayBigEndian(keyArr, CB_BRANCH_ACCOUNT_TIME_TX_ORDERNUM, CBArrayToInt64BigEndian(keyArr, CB_BRANCH_ACCOUNT_TIME_TX_ORDERNUM) + 1);
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
		if (!CBAccounterAdjustBalances(storage, keyArr + CB_BRANCH_ACCOUNT_TIME_TX_TIMESTAMP, max, section, accountDest, addedBalance, &last)) {
			CBLogError("Could not adjust the remaining cumulative balanaces.");
			return false;
		}
		// branchSectionAccountOutputs
		uint8_t branchSectionAccountOutputsMin[21];
		uint8_t branchSectionAccountOutputsMax[21];
		branchSectionAccountOutputsMin[CB_ACCOUNT_OUTPUTS_BRANCH] = section;
		branchSectionAccountOutputsMax[CB_ACCOUNT_OUTPUTS_BRANCH] = section;
		CBInt64ToArray(branchSectionAccountOutputsMin, CB_ACCOUNT_OUTPUTS_ACCOUNT_ID, accountSrc);
		CBInt64ToArray(branchSectionAccountOutputsMax, CB_ACCOUNT_OUTPUTS_ACCOUNT_ID, accountSrc);
		memset(branchSectionAccountOutputsMin + CB_ACCOUNT_OUTPUTS_HEIGHT, 0, 12);
		memset(branchSectionAccountOutputsMax + CB_ACCOUNT_OUTPUTS_HEIGHT, 0xFF, 12);
		it = (CBDatabaseRangeIterator){branchSectionAccountOutputsMin, branchSectionAccountOutputsMax, storage->branchSectionAccountOutputs};
		status = CBDatabaseRangeIteratorFirst(&it);
		if (status == CB_DATABASE_INDEX_ERROR) {
			CBLogError("There was a problem finding the first branch section account output details.");
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
			// Copy everything from the key except the account ID
			keyArr[CB_ACCOUNT_OUTPUTS_BRANCH] = key[CB_ACCOUNT_OUTPUTS_BRANCH];
			memcpy(keyArr + CB_ACCOUNT_OUTPUTS_HEIGHT, key + CB_ACCOUNT_OUTPUTS_HEIGHT, 12);
			// Set the destination account ID
			CBInt64ToArray(keyArr, CB_ACCOUNT_OUTPUTS_ACCOUNT_ID, accountDest);
			// See if it exists already. If it is spent then do not write
			uint8_t unspent;
			CBIndexFindStatus status = CBDatabaseReadValue(storage->branchSectionAccountOutputs, keyArr, &unspent, 1, 0, false);
			if (status == CB_DATABASE_INDEX_ERROR) {
				CBLogError("Could not determine the spent status of an output for seeing if we should write from another account during a merge.");
				return false;
			}
			if (status != CB_DATABASE_INDEX_FOUND || (unspent && !data))
				// Write data for destination account
				CBDatabaseWriteValue(storage->branchSectionAccountOutputs, keyArr, &data, 1);
			if (status != CB_DATABASE_INDEX_FOUND) {
				// The destination account will need the outputAccount entry
				memcpy(keyArr + CB_OUTPUT_ACCOUNTS_OUTPUT_ID, key + CB_ACCOUNT_OUTPUTS_OUTPUT_ID, 8);
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
		// Get next sections if any children
		bool doneFirst = false;
		for (uint8_t x = 0; x < CB_MAX_BRANCH_SECTIONS; x++) {
			if (CBAccounterGetParentBranchSection(storage, x) == section) {
				sectionQueue[queueBack].childSections[doneFirst] = x;
				if (doneFirst) {
					sectionQueue[queueBack].addedBalance = addedBalance;
					sectionQueue[queueBack].last = last;
					queueBack++;
					break;
				}else
					doneFirst = true;
			}
		}
	}
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
bool CBAccounterNewBranch(CBDepObject self, uint8_t newBranch, uint8_t parent, uint32_t blockHeight){
	CBAccounterStorage * storage = self.ptr;
	if (parent != CB_NO_PARENT)
		// Make new branch inherit order num count from parent ??? Inherit from fork point or don't bother?
		memcpy(storage->database->current.extraData + CB_ACCOUNTER_DETAILS_BRANCH_SECTION_NEXT_ORDER_NUM + newBranch*8,
			   storage->database->current.extraData + CB_ACCOUNTER_DETAILS_BRANCH_SECTION_NEXT_ORDER_NUM + parent*8, 8);
	// Find first CB_UNUSED_SECTION
	uint8_t branchSectionIndex = 0;
	for (;storage->database->current.extraData[CB_ACCOUNTER_DETAILS_BRANCH_SECTION_PARENTS + branchSectionIndex] != CB_UNUSED_SECTION; branchSectionIndex++);
	// Set this as the last section for the new branch.
	storage->database->current.extraData[CB_ACCOUNTER_DETAILS_BRANCH_LAST_SECTION_IDS + newBranch] = branchSectionIndex;
	// Set the block height
	CBInt32ToArrayBigEndian(storage->database->current.extraData, CB_ACCOUNTER_DETAILS_BRANCH_SECTION_HEIGHT_START + branchSectionIndex, blockHeight);
	// Set parent section
	uint8_t parentLastSectionIndex = CBAccounterGetLastBranchSectionID(storage, parent);
	storage->database->current.extraData[CB_ACCOUNTER_DETAILS_BRANCH_SECTION_PARENTS + branchSectionIndex] = parentLastSectionIndex + 1;
	// If we have no parent we can end here
	if (parent == CB_NO_PARENT)
		return true;
	// Give parent a new section after the fork.
	for (branchSectionIndex++;storage->database->current.extraData[CB_ACCOUNTER_DETAILS_BRANCH_SECTION_PARENTS + branchSectionIndex] != CB_UNUSED_SECTION; branchSectionIndex++);
	storage->database->current.extraData[CB_ACCOUNTER_DETAILS_BRANCH_SECTION_PARENTS + branchSectionIndex] = parentLastSectionIndex + 1;
	CBInt32ToArrayBigEndian(storage->database->current.extraData, CB_ACCOUNTER_DETAILS_BRANCH_SECTION_HEIGHT_START + branchSectionIndex, blockHeight);
	storage->database->current.extraData[CB_ACCOUNTER_DETAILS_BRANCH_LAST_SECTION_IDS + parent] = branchSectionIndex;
	// Now transfer all of the transactions in the parent branch after the fork to the new section
	// parentLastSectionIndex is old section
	// branchSectionIndex is new section
	// Use txBranchSectionHeightAndID to iterate through transactions to move
	uint8_t txBranchSectionHeightAndIDMin[21];
	uint8_t txBranchSectionHeightAndIDMax[21];
	txBranchSectionHeightAndIDMin[CB_TX_HEIGHT_BRANCH_AND_ID_BRANCH] = parentLastSectionIndex;
	CBInt32ToArrayBigEndian(txBranchSectionHeightAndIDMin, CB_TX_HEIGHT_BRANCH_AND_ID_HEIGHT, blockHeight);
	memset(txBranchSectionHeightAndIDMin + CB_TX_HEIGHT_BRANCH_AND_ID_ORDER_NUM, 0, 16);
	txBranchSectionHeightAndIDMax[CB_TX_HEIGHT_BRANCH_AND_ID_BRANCH] = parentLastSectionIndex;
	memset(txBranchSectionHeightAndIDMax + CB_TX_HEIGHT_BRANCH_AND_ID_HEIGHT, 0xFF, 20);
	CBDatabaseRangeIterator it = {txBranchSectionHeightAndIDMin, txBranchSectionHeightAndIDMax, storage->txBranchSectionHeightAndID};
	CBIndexFindStatus res = CBDatabaseRangeIteratorFirst(&it);
	if (res == CB_DATABASE_INDEX_ERROR) {
		CBLogError("There was an error when attempting to find the first transaction that should be moved to a new branch section for a fork.");
		return false;
	}
	if (res == CB_DATABASE_INDEX_FOUND) for(;;) {
		uint8_t keyArr[33], newKey[33];
		// Get the key
		uint8_t key[21];
		CBDatabaseRangeIteratorGetKey(&it, key);
		// Get the transaction hash
		uint8_t txHash[32];
		memcpy(keyArr, key + CB_TX_HEIGHT_BRANCH_AND_ID_TX_ID, 8);
		if (CBDatabaseReadValue(storage->txDetails, keyArr, txHash, 32, CB_TX_DETAILS_HASH, false) != CB_DATABASE_INDEX_FOUND) {
			CBLogError("Could not get the transaction hash.");
			CBFreeDatabaseRangeIterator(&it);
			return false;
		}
		// Get the transaction timestamp
		uint8_t timestamp[8];
		if (CBDatabaseReadValue(storage->txDetails, keyArr, timestamp, 8, CB_TX_DETAILS_TIMESTAMP, false) != CB_DATABASE_INDEX_FOUND) {
			CBLogError("Could not get the transaction timestamp.");
			CBFreeDatabaseRangeIterator(&it);
			return false;
		}
		// Get the order num from txBranchSectionHeightAndID
		uint8_t * orderNum = key + CB_TX_HEIGHT_BRANCH_AND_ID_ORDER_NUM;
		// Loop through accounts owning this transaction
		uint8_t txAccountsMin[16];
		uint8_t txAccountsMax[16];
		memcpy(txAccountsMin + CB_TX_ACCOUNTS_TX_ID, key + CB_TX_HEIGHT_BRANCH_AND_ID_TX_ID, 8);
		memcpy(txAccountsMax + CB_TX_ACCOUNTS_TX_ID, key + CB_TX_HEIGHT_BRANCH_AND_ID_TX_ID, 8);
		memset(txAccountsMin + CB_TX_ACCOUNTS_ACCOUNT_ID, 0, 8);
		memset(txAccountsMax + CB_TX_ACCOUNTS_ACCOUNT_ID, 0xFF, 8);
		CBDatabaseRangeIterator it2 = (CBDatabaseRangeIterator){txAccountsMin, txAccountsMax, storage->txAccounts};
		if (CBDatabaseRangeIteratorFirst(&it2) != CB_DATABASE_INDEX_FOUND) {
			CBLogError("Could not get the first account for a transaction.");
			CBFreeDatabaseRangeIterator(&it);
			return false;
		}
		for (;;) {
			// Get the key
			uint8_t txAccountKey[16];
			CBDatabaseRangeIteratorGetKey(&it2, txAccountKey);
			// Change the branchSectionAccountTimeTx
			keyArr[CB_BRANCH_ACCOUNT_TIME_TX_BRANCH] = parentLastSectionIndex;
			memcpy(keyArr + CB_BRANCH_ACCOUNT_TIME_TX_ACCOUNT_ID, txAccountKey + CB_TX_ACCOUNTS_ACCOUNT_ID, 8);
			memcpy(keyArr + CB_BRANCH_ACCOUNT_TIME_TX_ORDERNUM, orderNum, 8);
			memcpy(keyArr + CB_BRANCH_ACCOUNT_TIME_TX_TIMESTAMP, timestamp, 8);
			memcpy(keyArr + CB_BRANCH_ACCOUNT_TIME_TX_TX_ID, txAccountKey + CB_TX_ACCOUNTS_TX_ID, 8);
			newKey[CB_BRANCH_ACCOUNT_TIME_TX_BRANCH] = branchSectionIndex;
			memcpy(newKey + CB_BRANCH_ACCOUNT_TIME_TX_ACCOUNT_ID, keyArr + CB_BRANCH_ACCOUNT_TIME_TX_ACCOUNT_ID, 32);
			if (! CBDatabaseChangeKey(storage->branchSectionAccountTimeTx, keyArr, newKey, false)){
				CBLogError("Could not change the key for an account time transaction entry to a new branch section when creating a new branch.");
				return false;
			}
			// Iterate through branchSectionAccountOutputs for moving to new section
			uint8_t branchSectionAccountOutputsMin[21];
			uint8_t branchSectionAccountOutputsMax[21];
			branchSectionAccountOutputsMin[CB_ACCOUNT_OUTPUTS_BRANCH] = parentLastSectionIndex;
			memcpy(branchSectionAccountOutputsMin + CB_ACCOUNT_OUTPUTS_ACCOUNT_ID, txAccountKey + CB_TX_ACCOUNTS_ACCOUNT_ID, 8);
			memset(branchSectionAccountOutputsMin + CB_ACCOUNT_OUTPUTS_HEIGHT, 0, 12);
			memcpy(branchSectionAccountOutputsMax + CB_ACCOUNT_OUTPUTS_BRANCH, branchSectionAccountOutputsMin + CB_ACCOUNT_OUTPUTS_BRANCH, 7);
			memcpy(branchSectionAccountOutputsMax + CB_ACCOUNT_OUTPUTS_ACCOUNT_ID, txAccountKey + CB_TX_ACCOUNTS_ACCOUNT_ID, 8);
			CBInt32ToArrayBigEndian(branchSectionAccountOutputsMax, CB_ACCOUNT_OUTPUTS_HEIGHT, UINT32_MAX - blockHeight);
			memset(branchSectionAccountOutputsMax + CB_ACCOUNT_OUTPUTS_OUTPUT_ID, 0xFF, 8);
			CBDatabaseRangeIterator it3 = {branchSectionAccountOutputsMin, branchSectionAccountOutputsMax, storage->branchSectionAccountOutputs};
			CBIndexFindStatus res = CBDatabaseRangeIteratorFirst(&it3);
			if (res == CB_DATABASE_INDEX_ERROR) {
				CBLogError("Could not get the first unspent output for an account.");
				CBFreeDatabaseRangeIterator(&it);
				CBFreeDatabaseRangeIterator(&it2);
				return false;
			}
			if (res == CB_DATABASE_INDEX_FOUND) for (;;) {
				// Get key
				uint8_t branchSectionAccountOutputsKey[21];
				CBDatabaseRangeIteratorGetKey(&it3, branchSectionAccountOutputsKey);
				// Change key
				newKey[CB_ACCOUNT_OUTPUTS_BRANCH] = branchSectionIndex;
				memcpy(newKey + CB_ACCOUNT_OUTPUTS_ACCOUNT_ID, branchSectionAccountOutputsKey + CB_ACCOUNT_OUTPUTS_ACCOUNT_ID, 20);
				if (! CBDatabaseChangeKey(storage->branchSectionAccountOutputs, branchSectionAccountOutputsKey, newKey, false)){
					CBLogError("Could not change the account output key to a new branch section when creating a new branch.");
					return false;
				}
				// Go to next first if possible
				CBIndexFindStatus status = CBDatabaseRangeIteratorFirst(&it3);
				if (status == CB_DATABASE_INDEX_ERROR) {
					CBLogError("Error whilst iterating through an account's unspent outputs.");
					CBFreeDatabaseRangeIterator(&it);
					CBFreeDatabaseRangeIterator(&it2);
					CBFreeDatabaseRangeIterator(&it3);
					return false;
				}
				if (status == CB_DATABASE_INDEX_NOT_FOUND)
					break;
			}
			CBFreeDatabaseRangeIterator(&it3);
			// Iterate if possible
			CBIndexFindStatus status = CBDatabaseRangeIteratorNext(&it2);
			if (status == CB_DATABASE_INDEX_ERROR) {
				CBLogError("Error whilst iterating through transaction accounts.");
				CBFreeDatabaseRangeIterator(&it);
				CBFreeDatabaseRangeIterator(&it2);
				return false;
			}
			if (status == CB_DATABASE_INDEX_NOT_FOUND)
				break;
		}
		CBFreeDatabaseRangeIterator(&it2);
		// Change the key for the branch section tx details
		keyArr[CB_BRANCH_TX_DETAILS_BRANCH] = parentLastSectionIndex;
		memcpy(keyArr + CB_BRANCH_TX_DETAILS_ORDER_NUM, key + CB_TX_HEIGHT_BRANCH_AND_ID_ORDER_NUM, 8);
		memcpy(keyArr + CB_BRANCH_TX_DETAILS_TX_ID, key + CB_TX_HEIGHT_BRANCH_AND_ID_TX_ID, 8);
		newKey[CB_BRANCH_TX_DETAILS_BRANCH] = branchSectionIndex;
		memcpy(newKey + CB_BRANCH_TX_DETAILS_ORDER_NUM, keyArr + CB_BRANCH_TX_DETAILS_ORDER_NUM, 16);
		if (! CBDatabaseChangeKey(storage->branchSectionTxDetails, keyArr, newKey, false)){
			CBLogError("Could not change the branch section tx height key to a new branch section when creating a new branch.");
			return false;
		}
		// Change the branch height and ID entry branch
		newKey[CB_TX_HEIGHT_BRANCH_AND_ID_BRANCH] = branchSectionIndex;
		memcpy(newKey + CB_TX_HEIGHT_BRANCH_AND_ID_HEIGHT, key + CB_TX_HEIGHT_BRANCH_AND_ID_HEIGHT, 20);
		if (! CBDatabaseChangeKey(storage->txBranchSectionHeightAndID, key, newKey, false)){
			CBLogError("Could not change the branch section height ID entry to a new branch section when creating a new branch.");
			return false;
		}
		// Get next first as we have removed the txBranchSectionHeightAndID
		CBIndexFindStatus status = CBDatabaseRangeIteratorFirst(&it);
		if (status == CB_DATABASE_INDEX_ERROR) {
			CBLogError("Error whilst iterating through transactions to move to another branch section.");
			CBFreeDatabaseRangeIterator(&it);
			return false;
		}
		if (status == CB_DATABASE_INDEX_NOT_FOUND)
			break;
	}
	CBFreeDatabaseRangeIterator(&it);
	// Find branch sections which are pointing to old section, and make them point to the new section if their starting block height is equal or beyond the fork.
	for (uint8_t x = 0; x < CB_MAX_BRANCH_SECTIONS; x++) {
		if (x != branchSectionIndex // Do not make the new section point to itself, duh.
			&& CBAccounterGetParentBranchSection(storage, x) == parentLastSectionIndex
			&& CBArrayToInt32BigEndian(storage->database->current.extraData, CB_ACCOUNTER_DETAILS_BRANCH_SECTION_HEIGHT_START + x) >= blockHeight) {
			storage->database->current.extraData[CB_ACCOUNTER_DETAILS_BRANCH_SECTION_PARENTS + x] = branchSectionIndex + 1;
		}
	}
	return true;
}
bool CBAccounterRemoveTransactionFromBranch(CBAccounterStorage * storage, uint64_t txID, uint8_t * txHash, uint8_t branch, uint8_t * orderNum, uint8_t * height){
	// Read the branch ownership number
	uint8_t branchOwners;
	uint8_t timestampData[8];
	uint8_t branchSectionIndex = CBAccounterGetLastBranchSectionID(storage, branch);
	uint8_t keyArr[33];
	CBInt64ToArray(keyArr, 0, txID);
	if (CBDatabaseReadValue(storage->txDetails, keyArr, &branchOwners, 1, CB_TX_DETAILS_BRANCH_INSTANCES, false) != CB_DATABASE_INDEX_FOUND) {
		CBLogError("Could not read the number of branches that own a transaction.");
		return false;
	}
	// Also read timestamp for later
	if (CBDatabaseReadValue(storage->txDetails, keyArr, timestampData, 8, CB_TX_DETAILS_TIMESTAMP, false) != CB_DATABASE_INDEX_FOUND) {
		CBLogError("Could not read a transactions timestamp.");
		return false;
	}
	// Lower the number of branch owners
	branchOwners--;
	if (branchOwners == 0) {
		// No more owners, delete transaction information.
		if (! CBDatabaseRemoveValue(storage->txDetails, keyArr, false)) {
			CBLogError("Could not remove transaction details.");
			return false;
		}
		if (! CBDatabaseRemoveValue(storage->txHashToID, txHash, false)) {
			CBLogError("Could not remove a transaction hash to ID entry.");
			return false;
		}
	}else{
		CBDatabaseWriteValueSubSection(storage->txDetails, keyArr, &branchOwners, 1, CB_TX_DETAILS_BRANCH_INSTANCES);
		if (branch == CB_NO_BRANCH)
			// Make not as unconfirmed
			CBDatabaseWriteValueSubSection(storage->txDetails, keyArr, (uint8_t []){false}, 1, CB_TX_DETAILS_IS_UNCONF);
	}
	// If in a block-chain branch, remove transaction branch data
	if (branch != CB_NO_BRANCH) {
		// Only need to delete from last branch section as this only happens when deleting branches
		keyArr[CB_BRANCH_TX_DETAILS_BRANCH] = branchSectionIndex;
		memcpy(keyArr + CB_BRANCH_TX_DETAILS_ORDER_NUM, orderNum, 8);
		CBInt64ToArray(keyArr, CB_BRANCH_TX_DETAILS_TX_ID, txID);
		if (! CBDatabaseRemoveValue(storage->branchSectionTxDetails, keyArr, false)) {
			CBLogError("Couldn't remove the branch transaction details");
			return false;
		}
		// Remove txBranchSectionHeightAndID
		keyArr[CB_TX_HEIGHT_BRANCH_AND_ID_BRANCH] = branchSectionIndex;
		memcpy(keyArr + CB_TX_HEIGHT_BRANCH_AND_ID_HEIGHT, height, 4);
		CBInt64ToArray(keyArr, CB_TX_HEIGHT_BRANCH_AND_ID_TX_ID, txID);
		memcpy(keyArr + CB_TX_HEIGHT_BRANCH_AND_ID_ORDER_NUM, orderNum, 8);
		if (! CBDatabaseRemoveValue(storage->txBranchSectionHeightAndID, keyArr, false)) {
			CBLogError("Couldn't remove the branch height and transaction ID entry.");
			return false;
		}
	}
	// Loop through the transaction accounts
	uint8_t txAccountsMin[16];
	uint8_t txAccountsMax[16];
	CBInt64ToArray(txAccountsMin, CB_TX_ACCOUNTS_TX_ID, txID);
	CBInt64ToArray(txAccountsMax, CB_TX_ACCOUNTS_TX_ID, txID);
	memset(txAccountsMin + CB_TX_ACCOUNTS_ACCOUNT_ID, 0, 8);
	memset(txAccountsMax + CB_TX_ACCOUNTS_ACCOUNT_ID, 0xFF, 8);
	CBDatabaseRangeIterator it = {txAccountsMin, txAccountsMax, storage->txAccounts};
	if (CBDatabaseRangeIteratorFirst(&it) != CB_DATABASE_INDEX_FOUND) {
		CBLogError("There was a problem finding the first transaction account entry.");
		return false;
	}
	for (;;) {
		// Get the tx account key
		uint8_t key[16];
		CBDatabaseRangeIteratorGetKey(&it, key);
		uint64_t accountID = CBArrayToInt64(key, CB_TX_ACCOUNTS_ACCOUNT_ID);
		// Adjust balance for branchless transactions only as transactions are removed from branches which are being deleted completely.
		int64_t value;
		if (! CBAccounterGetTxAccountValue(storage, txID, accountID, &value)) {
			CBLogError("Could not get the value of a transaction for an account.");
			CBFreeDatabaseRangeIterator(&it);
			return false;
		}
		if (branch == CB_NO_BRANCH){
			if (! CBAccounterAdjustUnconfBalance(storage, accountID, -value)) {
				CBLogError("Could not adjust an account balance for branchless transactions when one is being removed.");
				CBFreeDatabaseRangeIterator(&it);
				return false;
			}
		}
		// Remove the branch account time transaction entry
		keyArr[CB_BRANCH_ACCOUNT_TIME_TX_BRANCH] = branchSectionIndex;
		memcpy(keyArr + CB_BRANCH_ACCOUNT_TIME_TX_ACCOUNT_ID, key + CB_TX_ACCOUNTS_ACCOUNT_ID, 8);
		memcpy(keyArr + CB_BRANCH_ACCOUNT_TIME_TX_TIMESTAMP, timestampData, 8);
		if (branch == CB_NO_BRANCH)
			memset(keyArr + CB_BRANCH_ACCOUNT_TIME_TX_ORDERNUM, 0, 8);
		else
			memcpy(keyArr + CB_BRANCH_ACCOUNT_TIME_TX_ORDERNUM, orderNum, 8);
		CBInt64ToArray(keyArr, CB_BRANCH_ACCOUNT_TIME_TX_TX_ID, txID);
		if (! CBDatabaseRemoveValue(storage->branchSectionAccountTimeTx, keyArr, false)) {
			CBLogError("Could not remove an account transaction entry for a branch ordered by time.");
			CBFreeDatabaseRangeIterator(&it);
			return false;
		}
		CBIndexFindStatus status;
		// Remove non-branch-specific details if no more bramches own the transaction
		if (branchOwners == 0) {
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
	memcpy(outputHashAndIndexToIDMin + CB_OUTPUT_HASH_AND_INDEX_TO_ID_HASH, txHash, 32);
	memcpy(outputHashAndIndexToIDMax + CB_OUTPUT_HASH_AND_INDEX_TO_ID_HASH, txHash, 32);
	memset(outputHashAndIndexToIDMin + CB_OUTPUT_HASH_AND_INDEX_TO_ID_INDEX, 0, 4);
	memset(outputHashAndIndexToIDMax + CB_OUTPUT_HASH_AND_INDEX_TO_ID_INDEX, 0xFF, 4);
	// Loop through the range of potential outputs
	it = (CBDatabaseRangeIterator){outputHashAndIndexToIDMin, outputHashAndIndexToIDMax, storage->outputHashAndIndexToID};
	CBIndexFindStatus res = CBDatabaseRangeIteratorFirst(&it);
	if (res == CB_DATABASE_INDEX_ERROR) {
		CBLogError("There was an error whilst attemping to find the first account output information for a transaction.");
		return false;
	}
	if (res == CB_DATABASE_INDEX_FOUND) for (;;){
		// Get the key for this output ID
		uint8_t key[36];
		CBDatabaseRangeIteratorGetKey(&it, key);
		// Get the output ID
		uint8_t outputId[8];
		if (CBDatabaseReadValue(storage->outputHashAndIndexToID, key, outputId, 8, 0, false) != CB_DATABASE_INDEX_FOUND) {
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
		CBDatabaseRangeIterator accountIt = {outputAccountsMin, outputAccountsMax, storage->outputAccounts};
		if (CBDatabaseRangeIteratorFirst(&accountIt) != CB_DATABASE_INDEX_FOUND) {
			CBLogError("There was a problem finding the first output account entry.");
			CBFreeDatabaseRangeIterator(&it);
			return false;
		}
		for (;;) {
			// Get the key for this account
			uint8_t outputAccountKey[16];
			CBDatabaseRangeIteratorGetKey(&accountIt, outputAccountKey);
			// Remove the output as unspent from the account for this branch, if it exists as so.
			keyArr[CB_ACCOUNT_OUTPUTS_BRANCH] = branchSectionIndex;
			memcpy(keyArr + CB_ACCOUNT_OUTPUTS_ACCOUNT_ID, outputAccountKey + CB_OUTPUT_ACCOUNTS_ACCOUNTS_ID, 8);
			if (branch != CB_NO_BRANCH){
				// We need to ake the height from UINT32_MAX for CB_ACCOUNT_OUTPUTS_HEIGHT due to reverse ordering
				CBInt32ToArrayBigEndian(keyArr, CB_ACCOUNT_OUTPUTS_HEIGHT, UINT32_MAX - CBArrayToInt32BigEndian(height, 0));
			}else
				memset(keyArr + CB_ACCOUNT_OUTPUTS_HEIGHT, 0xFF, 4); // Unconfirmed transactions have block height of zero.
			memcpy(keyArr + CB_ACCOUNT_OUTPUTS_OUTPUT_ID, outputId, 8);
			if (! CBDatabaseRemoveValue(storage->branchSectionAccountOutputs, keyArr, false)) {
				CBLogError("Couldn't remove an output as unspent for an account.");
				CBFreeDatabaseRangeIterator(&it);
				CBFreeDatabaseRangeIterator(&accountIt);
				return false;
			}
			// If no more branches own the transaction, completely remove all output account information
			CBIndexFindStatus status;
			if (branchOwners == 0){
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
		// If no more branches own the transaction, completely remove all output information
		CBIndexFindStatus status;
		if (branchOwners == 0) {
			// Remove the output details
			memcpy(keyArr, outputId, 8);
			if (! CBDatabaseRemoveValue(storage->outputDetails, keyArr, false)) {
				CBLogError("Could not remove the output details.");
				CBFreeDatabaseRangeIterator(&it);
				return false;
			}
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
