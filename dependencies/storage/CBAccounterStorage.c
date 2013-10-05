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

uint8_t CB_KEY_ARRAY[36];
uint8_t CB_NEW_KEY_ARRAY[36];
uint8_t CB_DATA_ARRAY[64];

bool CBNewAccounterStorage(CBDepObject * storage, CBDepObject database) {
	CBAccounterStorage * self = malloc(sizeof(*self));
	self->database = database.ptr;
	uint8_t indexID = CB_INDEX_ACCOUNTER_START;
	// ??? These nested ifs were simple at first, trust me...
	self->accountUnconfBalance = CBLoadIndex(self->database, indexID++, 8, 10000);
	if (self->accountUnconfBalance) {
		self->accountTxDetails = CBLoadIndex(self->database, indexID++, 16, 10000);
		if (self->accountTxDetails) {
			self->branchSectionAccountOutputs = CBLoadIndex(self->database, indexID++, 21, 10000);
			if (self->branchSectionAccountOutputs) {
				self->branchSectionAccountTimeTx = CBLoadIndex(self->database, indexID++, 33, 10000);
				if (self->branchSectionAccountTimeTx) {
					self->branchSectionTxDetails = CBLoadIndex(self->database, indexID++, 17, 10000);
					if (self->branchSectionTxDetails) {
						self->outputAccounts = CBLoadIndex(self->database, indexID++, 16, 10000);
						if (self->outputAccounts) {
							self->outputDetails = CBLoadIndex(self->database, indexID++, 8, 10000);
							if (self->outputDetails) {
								self->outputHashAndIndexToID = CBLoadIndex(self->database, indexID++, 36, 10000);
								if (self->outputHashAndIndexToID) {
									self->txAccounts = CBLoadIndex(self->database, indexID++, 16, 10000);
									if (self->txAccounts) {
										self->txDetails = CBLoadIndex(self->database, indexID++, 8, 10000);
										if (self->txDetails) {
											self->txHashToID = CBLoadIndex(self->database, indexID++, 32, 10000);
											if (self->txHashToID) {
												self->txBranchSectionHeightAndID = CBLoadIndex(self->database, indexID++, 21, 10000);
												if (self->txBranchSectionHeightAndID) {
													self->watchedHashes = CBLoadIndex(self->database, indexID++, 28, 10000);
													if (self->watchedHashes) {
														// Load data
														self->lastAccountID = CBArrayToInt64(self->database->current.extraData, CB_ACCOUNTER_DETAILS_ACCOUNT_ID);
														self->nextOutputRefID = CBArrayToInt64(self->database->current.extraData, CB_ACCOUNTER_DETAILS_OUTPUT_ID);
														self->nextTxID = CBArrayToInt64(self->database->current.extraData, CB_ACCOUNTER_DETAILS_TX_ID);
														storage->ptr = self;
														return true;
													}
													CBFreeIndex(self->txBranchSectionHeightAndID);
												}
												CBFreeIndex(self->txHashToID);
											}
											CBFreeIndex(self->txDetails);
										}
										CBFreeIndex(self->txAccounts);
									}
									CBFreeIndex(self->outputHashAndIndexToID);
								}
								CBFreeIndex(self->outputDetails);
							}
							CBFreeIndex(self->outputAccounts);
						}
						CBFreeIndex(self->branchSectionTxDetails);
					}
					CBFreeIndex(self->branchSectionAccountTimeTx);
				}
				CBFreeIndex(self->branchSectionAccountOutputs);
			}
			CBFreeIndex(self->accountTxDetails);
		}
		CBFreeIndex(self->accountUnconfBalance);
	}
	return false;
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
	CBInitAssociativeArray(&self->spentOutputs, CBTransactionKeysCompare, storage->outputDetails, NULL);
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
bool CBAccounterAddWatchedOutputToAccount(CBDepObject self, uint8_t * hash, uint64_t accountID){
	CBAccounterStorage * storage = self.ptr;
	memcpy(CB_KEY_ARRAY + CB_WATCHED_HASHES_HASH, hash, 20);
	CBInt64ToArray(CB_KEY_ARRAY, CB_WATCHED_HASHES_ACCOUNT_ID, accountID);
	CBDatabaseWriteValue(storage->watchedHashes, CB_KEY_ARRAY, NULL, 0);
	return true;
}
bool CBAccounterAdjustUnconfBalance(CBAccounterStorage * self, uint64_t accountID, uint64_t value){
	int64_t balance;
	if (! CBAccounterGetBranchAccountBalance((CBDepObject){.ptr=self}, CB_NO_BRANCH, accountID, &balance)) {
		CBLogError("Could not get the previous balance for unconfirmed transactions for adjustment.");
		return false;
	}
	balance += value;
	CBInt64ToArray(CB_DATA_ARRAY, 0, labs(balance) | ((balance > 0) ? 0x8000000000000000 : 0));
	CBDatabaseWriteValue(self->accountUnconfBalance, CB_KEY_ARRAY, CB_DATA_ARRAY, 8);
	return true;
}
bool CBAccounterBranchlessTransactionToBranch(CBDepObject self, void * vtx, uint32_t blockHeight, uint8_t branch){
	CBAccounterStorage * storage = self.ptr;
	CBTransaction * tx = vtx;
	uint8_t branchSectionID = CBAccounterGetLastBranchSectionID(storage, branch);
	// A previously branchless transaction has been found in a branch.
	// Loop through inputs and change the spent status key for the branch.
	for (uint32_t x = 0; x < tx->inputNum; x++) {
		// Get the output ID
		memcpy(CB_KEY_ARRAY + CB_OUTPUT_HASH_AND_INDEX_TO_ID_HASH, CBByteArrayGetData(tx->inputs[x]->prevOut.hash), 32);
		CBInt32ToArray(CB_KEY_ARRAY, CB_OUTPUT_HASH_AND_INDEX_TO_ID_INDEX, tx->inputs[x]->prevOut.index);
		CBIndexFindStatus stat = CBDatabaseReadValue(storage->outputHashAndIndexToID, CB_KEY_ARRAY, CB_DATA_ARRAY, 8, 0, false);
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
		memcpy(outputAccountsMin + CB_OUTPUT_ACCOUNTS_OUTPUT_ID, CB_DATA_ARRAY, 8);
		memcpy(outputAccountsMax + CB_OUTPUT_ACCOUNTS_OUTPUT_ID, CB_DATA_ARRAY, 8);
		memset(outputAccountsMin + CB_OUTPUT_ACCOUNTS_ACCOUNTS_ID, 0, 8);
		memset(outputAccountsMax + CB_OUTPUT_ACCOUNTS_ACCOUNTS_ID, 0xFF, 8);
		CBDatabaseRangeIterator it = {outputAccountsMin, outputAccountsMax, storage->outputAccounts};
		if (CBDatabaseRangeIteratorFirst(&it) != CB_DATABASE_INDEX_FOUND) {
			CBLogError("Could not get the first account for an output when removing an output spent status from branchless.");
			return false;
		}
		// Prepare account output status keys with the branch, zero height and output ID.
		CB_KEY_ARRAY[CB_ACCOUNT_OUTPUTS_BRANCH] = CB_NO_BRANCH;
		memset(CB_KEY_ARRAY + CB_ACCOUNT_OUTPUTS_HEIGHT, 0xFF, 4);
		memcpy(CB_KEY_ARRAY + CB_ACCOUNT_OUTPUTS_OUTPUT_ID, CB_DATA_ARRAY, 8);
		CB_NEW_KEY_ARRAY[CB_ACCOUNT_OUTPUTS_BRANCH] = branch;
		CBInt32ToArrayBigEndian(CB_NEW_KEY_ARRAY, CB_ACCOUNT_OUTPUTS_HEIGHT, UINT32_MAX - blockHeight);
		memcpy(CB_NEW_KEY_ARRAY + CB_ACCOUNT_OUTPUTS_OUTPUT_ID, CB_DATA_ARRAY, 8);
		for (;;) {
			// Add the account ID into the keys
			uint8_t * key = CBDatabaseRangeIteratorGetKey(&it);
			memcpy(CB_KEY_ARRAY + CB_ACCOUNT_OUTPUTS_ACCOUNT_ID, key + CB_OUTPUT_ACCOUNTS_ACCOUNTS_ID, 8);
			memcpy(CB_NEW_KEY_ARRAY + CB_ACCOUNT_OUTPUTS_ACCOUNT_ID, key + CB_OUTPUT_ACCOUNTS_ACCOUNTS_ID, 8);
			// Now change the key
			if (! CBDatabaseChangeKey(storage->branchSectionAccountOutputs, CB_KEY_ARRAY, CB_NEW_KEY_ARRAY, false)) {
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
		memcpy(CB_KEY_ARRAY + CB_OUTPUT_HASH_AND_INDEX_TO_ID_HASH, CBTransactionGetHash(tx), 32);
		CBInt32ToArray(CB_KEY_ARRAY, CB_OUTPUT_HASH_AND_INDEX_TO_ID_INDEX, x);
		uint32_t len;
		if (! CBDatabaseGetLength(storage->outputHashAndIndexToID, CB_KEY_ARRAY, &len)) {
			CBLogError("Unable to obtain length for an outputHashAndIndexToID entry when a tx enters a branch.");
			return false;
		}
		if (len != CB_DOESNT_EXIST) {
			// The output ID exists so get it
			if (CBDatabaseReadValue(storage->outputHashAndIndexToID, CB_KEY_ARRAY, CB_DATA_ARRAY, 8, 0, false) != CB_DATABASE_INDEX_FOUND) {
				CBLogError("Could not get the ID of a branchless output so we can move the spent status to a branch");
				return false;
			}
			uint64_t outputID = CBArrayToInt64(CB_DATA_ARRAY, 0);
			// Loop through the output's accounts and change the unspent output references to the branch the output is being added to.
			uint8_t outputAccountsMin[16];
			uint8_t outputAccountsMax[16];
			CBInt64ToArray(outputAccountsMin, CB_OUTPUT_ACCOUNTS_OUTPUT_ID, outputID);
			CBInt64ToArray(outputAccountsMax, CB_OUTPUT_ACCOUNTS_OUTPUT_ID, outputID);
			memset(outputAccountsMin + CB_OUTPUT_ACCOUNTS_ACCOUNTS_ID, 0, 8);
			memset(outputAccountsMax + CB_OUTPUT_ACCOUNTS_ACCOUNTS_ID, 0xFF, 8);
			CBDatabaseRangeIterator it = {outputAccountsMin, outputAccountsMax, storage->outputAccounts};
			if (CBDatabaseRangeIteratorFirst(&it) != CB_DATABASE_INDEX_FOUND){
				CBLogError("Couldn't find outputAccount data for an output that has entered a branch");
				return false;
			}
			for (;;) {
				// Get the output account key
				uint8_t * key = CBDatabaseRangeIteratorGetKey(&it);
				// Delete the branchless unspent output entry
				CB_KEY_ARRAY[CB_ACCOUNT_OUTPUTS_BRANCH] = CB_NO_BRANCH;
				memcpy(CB_KEY_ARRAY + CB_ACCOUNT_OUTPUTS_ACCOUNT_ID, key + CB_OUTPUT_ACCOUNTS_ACCOUNTS_ID, 8);
				memset(CB_KEY_ARRAY + CB_ACCOUNT_OUTPUTS_HEIGHT, 0xFF, 4); // Height is zero for branchless outputs thus UINT32_MAX minus zero gives 0xFFFFFFFF
				memcpy(CB_KEY_ARRAY + CB_ACCOUNT_OUTPUTS_OUTPUT_ID, CB_DATA_ARRAY, 8);
				// Delete branchless entry.
				if (! CBDatabaseRemoveValue(storage->branchSectionAccountOutputs, CB_KEY_ARRAY, false)){
					CBLogError("Could not delete the branchless unspent output when moving to a branch.");
					return false;
				}
				// Make the account outputs key for the branch
				CB_KEY_ARRAY[CB_ACCOUNT_OUTPUTS_BRANCH] = branchSectionID;
				// Add block height in descending order.
				CBInt32ToArrayBigEndian(CB_KEY_ARRAY, CB_ACCOUNT_OUTPUTS_HEIGHT, UINT32_MAX - blockHeight);
				// Write unspent output
				CBDatabaseWriteValue(storage->branchSectionAccountOutputs, CB_KEY_ARRAY, (uint8_t []){true}, 1);
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
	memcpy(CB_KEY_ARRAY, CBTransactionGetHash(tx), 32);
	if (CBDatabaseReadValue(storage->txHashToID, CB_KEY_ARRAY, CB_DATA_ARRAY, 8, 0, false) != CB_DATABASE_INDEX_FOUND) {
		CBLogError("Could not get a transaction ID for adding to a branch.");
		return false;
	}
	uint64_t txID = CBArrayToInt64(CB_DATA_ARRAY, 0);
	// Get the next order num
	uint8_t * nextOrderNumData = storage->database->current.extraData + CB_ACCOUNTER_DETAILS_BRANCH_SECTION_NEXT_ORDER_NUM + branch*8;
	// Increment the next order
	CBInt64ToArrayBigEndian(storage->database->current.extraData, CB_ACCOUNTER_DETAILS_BRANCH_SECTION_NEXT_ORDER_NUM + branch*8, CBArrayToInt64BigEndian(nextOrderNumData, 0) + 1);
	// Get the timestamp of the transaction
	uint8_t timestamp[8];
	CBInt64ToArray(CB_KEY_ARRAY, 0, txID);
	if (CBDatabaseReadValue(storage->txDetails, CB_KEY_ARRAY, timestamp, 8, CB_TX_DETAILS_TIMESTAMP, false) != CB_DATABASE_INDEX_FOUND) {
		CBLogError("Could not read a transactions timestamp for moving a branchless transaction into a branch.");
		return false;
	}
	// Add branch transaction details
	CB_KEY_ARRAY[CB_BRANCH_TX_DETAILS_BRANCH] = branchSectionID;
	CBInt64ToArray(CB_KEY_ARRAY, CB_BRANCH_TX_DETAILS_TX_ID, txID);
	memcpy(CB_KEY_ARRAY + CB_BRANCH_TX_DETAILS_ORDER_NUM, nextOrderNumData, 8);
	CBInt32ToArrayBigEndian(CB_DATA_ARRAY, CB_BRANCH_TX_DETAILS_HEIGHT, blockHeight);
	CBDatabaseWriteValue(storage->branchSectionTxDetails, CB_KEY_ARRAY, CB_DATA_ARRAY, 12);
	// Add branch, height and ID entry.
	CB_KEY_ARRAY[CB_TX_HEIGHT_BRANCH_AND_ID_BRANCH] = branchSectionID;
	CBInt32ToArrayBigEndian(CB_KEY_ARRAY, CB_TX_HEIGHT_BRANCH_AND_ID_HEIGHT, blockHeight);
	CBInt64ToArray(CB_KEY_ARRAY, CB_TX_HEIGHT_BRANCH_AND_ID_TX_ID, txID);
	memcpy(CB_KEY_ARRAY + CB_TX_HEIGHT_BRANCH_AND_ID_ORDER_NUM, nextOrderNumData, 8);
	// Write the txBranchSectionHeightAndID entry
	CBDatabaseWriteValue(storage->txBranchSectionHeightAndID, CB_KEY_ARRAY, NULL, 0);
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
		uint8_t * key = CBDatabaseRangeIteratorGetKey(&it);
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
		CBAccounterGetLastAccountBranchSectionBalance(storage, accountID, branchSectionID, &balance);
		balance += value;
		// Change the branch account time transaction entry, for the new branch.
		CB_KEY_ARRAY[CB_BRANCH_ACCOUNT_TIME_TX_BRANCH] = CB_NO_BRANCH;
		CBInt64ToArray(CB_KEY_ARRAY, CB_BRANCH_ACCOUNT_TIME_TX_ACCOUNT_ID, accountID);
		memcpy(CB_KEY_ARRAY + CB_BRANCH_ACCOUNT_TIME_TX_TIMESTAMP, timestamp, 8);
		memset(CB_KEY_ARRAY + CB_BRANCH_ACCOUNT_TIME_TX_ORDERNUM, 0, 8);
		CBInt64ToArray(CB_KEY_ARRAY, CB_BRANCH_ACCOUNT_TIME_TX_TX_ID, txID);
		// Make new key
		CB_NEW_KEY_ARRAY[CB_BRANCH_ACCOUNT_TIME_TX_BRANCH] = branchSectionID;
		CBInt64ToArray(CB_NEW_KEY_ARRAY, CB_BRANCH_ACCOUNT_TIME_TX_ACCOUNT_ID, accountID);
		memcpy(CB_NEW_KEY_ARRAY + CB_BRANCH_ACCOUNT_TIME_TX_TIMESTAMP, timestamp, 8);
		memcpy(CB_NEW_KEY_ARRAY + CB_BRANCH_ACCOUNT_TIME_TX_ORDERNUM, nextOrderNumData, 8);
		CBInt64ToArray(CB_NEW_KEY_ARRAY, CB_BRANCH_ACCOUNT_TIME_TX_TX_ID, txID);
		if (! CBDatabaseChangeKey(storage->branchSectionAccountTimeTx, CB_KEY_ARRAY, CB_NEW_KEY_ARRAY, false)){
			CBLogError("Could not change key for the branch account time transaction entry for a transaction entering a branch.");
			return false;
		}
		// Now write balance
		CBInt64ToArray(CB_DATA_ARRAY, 0, balance);
		CBDatabaseWriteValue(storage->branchSectionAccountTimeTx, CB_NEW_KEY_ARRAY, CB_DATA_ARRAY, 8);
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
	// Make the output hash and index to ID key
	memcpy(CB_KEY_ARRAY + CB_OUTPUT_HASH_AND_INDEX_TO_ID_HASH, CBByteArrayGetData(prevOut->hash), 32);
	CBInt32ToArray(CB_KEY_ARRAY, CB_OUTPUT_HASH_AND_INDEX_TO_ID_INDEX, prevOut->index);
	CBIndexFindStatus stat = CBDatabaseReadValue(self->outputHashAndIndexToID, CB_KEY_ARRAY, CB_DATA_ARRAY, 8, 0, false);
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
	memcpy(minOutputAccountKey + CB_OUTPUT_ACCOUNTS_OUTPUT_ID, CB_DATA_ARRAY, 8);
	memcpy(maxOutputAccountKey + CB_OUTPUT_ACCOUNTS_OUTPUT_ID, CB_DATA_ARRAY, 8);
	memset(minOutputAccountKey + CB_OUTPUT_ACCOUNTS_ACCOUNTS_ID, 0, 8);
	memset(maxOutputAccountKey + CB_OUTPUT_ACCOUNTS_ACCOUNTS_ID, 0xFF, 8);
	CBDatabaseRangeIterator it = {minOutputAccountKey, maxOutputAccountKey, self->outputAccounts};
	if (CBDatabaseRangeIteratorFirst(&it) != CB_DATABASE_INDEX_FOUND) {
		CBLogError("Couldn't get the first output account entry");
		return false;
	}
	uint8_t outputIDBuf[8];
	memcpy(outputIDBuf, CB_DATA_ARRAY, 8);
	for (;;) {
		// Get the ouput account key
		uint8_t * outputAccountKey = CBDatabaseRangeIteratorGetKey(&it);
		// Create the account output key
		CB_KEY_ARRAY[CB_ACCOUNT_OUTPUTS_BRANCH] = branchSectionID;
		memcpy(CB_KEY_ARRAY + CB_ACCOUNT_OUTPUTS_ACCOUNT_ID, outputAccountKey + CB_OUTPUT_ACCOUNTS_ACCOUNTS_ID, 8);
		CBInt32ToArrayBigEndian(CB_KEY_ARRAY, CB_ACCOUNT_OUTPUTS_HEIGHT, UINT32_MAX - blockHeight);
		memcpy(CB_KEY_ARRAY + CB_ACCOUNT_OUTPUTS_OUTPUT_ID, outputIDBuf, 8);
		// Write output as spent.
		CBDatabaseWriteValue(self->branchSectionAccountOutputs, CB_KEY_ARRAY, (uint8_t []){false}, 1);
		// Add txInfo if not NULL
		if (txInfo){
			// Add the account outflow information
			// Get the output value
			memcpy(CB_KEY_ARRAY, outputIDBuf, 8);
			if (CBDatabaseReadValue(self->outputDetails, CB_KEY_ARRAY, CB_DATA_ARRAY, 8, CB_OUTPUT_DETAILS_VALUE, false) != CB_DATABASE_INDEX_FOUND) {
				CBLogError("Could not read the value of an output.");
				CBFreeDatabaseRangeIterator(&it);
				return false;
			}
			uint64_t value = CBArrayToInt64(CB_DATA_ARRAY, 0);
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
		uint8_t * key = CBDatabaseRangeIteratorGetKey(&it);
		// Remove the tx from the branch
		memcpy(CB_KEY_ARRAY, key + CB_TX_HEIGHT_BRANCH_AND_ID_TX_ID, 8);
		// Get the transaction hash
		uint8_t txHash[32];
		if (CBDatabaseReadValue(storage->txDetails, CB_KEY_ARRAY, txHash, 32, CB_TX_DETAILS_HASH, false) != CB_DATABASE_INDEX_FOUND) {
			CBLogError("Could not get a transaction hash details.");
			CBFreeDatabaseRangeIterator(&it);
			return false;
		}
		uint8_t orderNumData[8];
		uint8_t heightData[4];
		memcpy(orderNumData, key + CB_TX_HEIGHT_BRANCH_AND_ID_ORDER_NUM, 8);
		memcpy(heightData, key + CB_TX_HEIGHT_BRANCH_AND_ID_HEIGHT, 4);
		if (! CBAccounterRemoveTransactionFromBranch(storage, CBArrayToInt64(CB_KEY_ARRAY, 0), txHash, branch, orderNumData, heightData)) {
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
				CBInt64ToArray(CB_KEY_ARRAY, 0, outputID);
				CBInt64ToArray(CB_DATA_ARRAY, CB_OUTPUT_DETAILS_VALUE, tx->outputs[x]->value);
				memcpy(CB_DATA_ARRAY + CB_OUTPUT_DETAILS_TX_HASH, CBTransactionGetHash(tx), 32);
				CBInt32ToArray(CB_DATA_ARRAY, CB_OUTPUT_DETAILS_INDEX, x);
				memcpy(CB_DATA_ARRAY + CB_OUTPUT_DETAILS_ID_HASH, hash, 20);
				CBDatabaseWriteValue(storage->outputDetails, CB_KEY_ARRAY, CB_DATA_ARRAY, 64);
				// Save the output hash and index to ID entry
				CBInt64ToArray(CB_DATA_ARRAY, 0, outputID);
				CBDatabaseWriteValue(storage->outputHashAndIndexToID, outputHashAndIndexToIDKey, CB_DATA_ARRAY, 8);
				// Increment the next output reference ID and save it.
				storage->nextOutputRefID++;
				CBInt64ToArray(storage->database->current.extraData, CB_ACCOUNTER_DETAILS_OUTPUT_ID, storage->nextOutputRefID);
			}else{
				// Get the ID of this output
				if (CBDatabaseReadValue(storage->outputHashAndIndexToID, outputHashAndIndexToIDKey, CB_DATA_ARRAY, 8, 0, false) != CB_DATABASE_INDEX_FOUND) {
					CBLogError("Could not read the output hash and index to ID entry.");
					CBFreeDatabaseRangeIterator(&it);
					return CB_ERROR;
				}
				outputID = CBArrayToInt64(CB_DATA_ARRAY, 0);
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
				uint8_t * key = CBDatabaseRangeIteratorGetKey(&it);
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
		memcpy(CB_KEY_ARRAY, CBTransactionGetHash(tx), 32);
		// Determine if the transaction exists already.
		uint32_t len;
		if (! CBDatabaseGetLength(storage->txHashToID, CB_KEY_ARRAY, &len)) {
			CBLogError("Could not get length of txHashToID to see if transaction extists.");
			return CB_ERROR;
		}
		if (len == CB_DOESNT_EXIST) {
			// It doesn't exist, get the next ID.
			txID = storage->nextTxID;
			// Create the transaction hash to ID entry
			CBInt64ToArray(CB_DATA_ARRAY, 0, txID);
			CBDatabaseWriteValue(storage->txHashToID, CB_KEY_ARRAY, CB_DATA_ARRAY, 8);
			// Create the transaction details. Copy the txID from CB_DATA_ARRAY.
			memcpy(CB_KEY_ARRAY, CB_DATA_ARRAY, 8);
			CB_DATA_ARRAY[CB_TX_DETAILS_BRANCH_INSTANCES] = 1;
			memcpy(CB_DATA_ARRAY + CB_TX_DETAILS_HASH, CBTransactionGetHash(tx), 32);
			CBInt64ToArrayBigEndian(CB_DATA_ARRAY, CB_TX_DETAILS_TIMESTAMP, time);
			CBDatabaseWriteValue(storage->txDetails, CB_KEY_ARRAY, CB_DATA_ARRAY, 41);
			// Increment the next ID
			storage->nextTxID++;
			CBInt64ToArray(storage->database->current.extraData, CB_ACCOUNTER_DETAILS_TX_ID, storage->nextTxID);
		}else{
			// Obtain the transaction ID.
			if (CBDatabaseReadValue(storage->txHashToID, CB_KEY_ARRAY, CB_DATA_ARRAY, 8, 0, false) != CB_DATABASE_INDEX_FOUND) {
				CBLogError("Couldn't read a transaction ID.");
				return CB_ERROR;
			}
			txID = CBArrayToInt64(CB_DATA_ARRAY, 0);
			// Increase the number of branches that own this transaction
			CBInt64ToArray(CB_KEY_ARRAY, 0, txID);
			if (CBDatabaseReadValue(storage->txDetails, CB_KEY_ARRAY, CB_DATA_ARRAY, 1, CB_TX_DETAILS_BRANCH_INSTANCES, false) != CB_DATABASE_INDEX_FOUND) {
				CBLogError("Couldn't read the branch instance count for a transaction.");
				return CB_ERROR;
			}
			CB_DATA_ARRAY[0]++;
			CBDatabaseWriteValueSubSection(storage->txDetails, CB_KEY_ARRAY, CB_DATA_ARRAY, 1, CB_TX_DETAILS_BRANCH_INSTANCES);
			// Get time
			if (CBDatabaseReadValue(storage->txDetails, CB_KEY_ARRAY, CB_DATA_ARRAY, 8, CB_TX_DETAILS_TIMESTAMP, false) != CB_DATABASE_INDEX_FOUND) {
				CBLogError("Could not read a transaction's timestamp when getting a transaction time.");
				return false;
			}
			time = CBArrayToInt64BigEndian(CB_DATA_ARRAY, 0);
		}
		uint8_t * nextOrderNumData;
		if (branch != CB_NO_BRANCH) {
			// Get the next order num
			nextOrderNumData = storage->database->current.extraData + CB_ACCOUNTER_DETAILS_BRANCH_SECTION_NEXT_ORDER_NUM + branch*8;
			// Increment the next order num
			CBInt64ToArrayBigEndian(storage->database->current.extraData, CB_ACCOUNTER_DETAILS_BRANCH_SECTION_NEXT_ORDER_NUM + branch*8,  CBArrayToInt64BigEndian(nextOrderNumData, 0) + 1);
			// Write the value
			// Set the block height and timestamp for the branch and this transaction
			CB_KEY_ARRAY[CB_BRANCH_TX_DETAILS_BRANCH] = branchSectionID;
			CBInt64ToArray(CB_KEY_ARRAY, CB_BRANCH_TX_DETAILS_TX_ID, txID);
			memcpy(CB_KEY_ARRAY + CB_BRANCH_TX_DETAILS_ORDER_NUM, nextOrderNumData, 8);
			CBInt32ToArrayBigEndian(CB_DATA_ARRAY, CB_BRANCH_TX_DETAILS_HEIGHT, blockHeight);
			CBDatabaseWriteValue(storage->branchSectionTxDetails, CB_KEY_ARRAY, CB_DATA_ARRAY, 12);
			// Create the branch and block height entry for the transaction
			CB_KEY_ARRAY[CB_TX_HEIGHT_BRANCH_AND_ID_BRANCH] = branchSectionID;
			CBInt32ToArrayBigEndian(CB_KEY_ARRAY, CB_TX_HEIGHT_BRANCH_AND_ID_HEIGHT, blockHeight);
			memcpy(CB_KEY_ARRAY + CB_TX_HEIGHT_BRANCH_AND_ID_ORDER_NUM, nextOrderNumData, 8);
			CBInt64ToArray(CB_KEY_ARRAY, CB_TX_HEIGHT_BRANCH_AND_ID_TX_ID, txID);
			CBDatabaseWriteValue(storage->txBranchSectionHeightAndID, CB_KEY_ARRAY, NULL, 0);
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
			// Write details to the linked list
			*curDetails = malloc(sizeof(**curDetails));
			(*curDetails)->accountID = info->accountID;
			(*curDetails)->next = NULL;
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
				CBInt64ToArray(CB_DATA_ARRAY, CB_ACCOUNT_TX_DETAILS_VALUE, labs(valueInt) | ((valueInt > 0) ? 0x8000000000000000 : 0));
				// Set the address.
				if (valueInt > 0)
					// Use inflow address
					memcpy(CB_DATA_ARRAY + CB_ACCOUNT_TX_DETAILS_ADDR, info->inAddr, 20);
				else{
					// Find a outflow address if there is one we can use.
					if (! info->foundInAddr || ! info->inAddrIndexIsZero) {
						// Try index 0.
						if (CBTransactionOuputGetHash(tx->outputs[0], CB_DATA_ARRAY + CB_ACCOUNT_TX_DETAILS_ADDR))
							// The output is not supported, use zero to represent this.
							memset(CB_DATA_ARRAY + CB_ACCOUNT_TX_DETAILS_ADDR, 0, 20);
					}else{
						// Else we have found a inflow address and it is at the zero index
						if (tx->outputNum == 1)
							// No other outputs, must be wasting money to miners.
							memset(CB_DATA_ARRAY + CB_ACCOUNT_TX_DETAILS_ADDR, 0, 20);
						// Try index 1
						else if (CBTransactionOuputGetHash(tx->outputs[1], CB_DATA_ARRAY + CB_ACCOUNT_TX_DETAILS_ADDR))
							memset(CB_DATA_ARRAY + CB_ACCOUNT_TX_DETAILS_ADDR, 0, 20);
					}
				}
				// Now save the account's transaction details
				CBDatabaseWriteValue(storage->accountTxDetails, accountTxDetailsKey, CB_DATA_ARRAY, 28);
				// Set the address in the account transaction details list.
				memcpy((*curDetails)->accountTxDetails.addrHash, CB_DATA_ARRAY + CB_ACCOUNT_TX_DETAILS_ADDR, 20);
			}else{
				// Get the value of the transaction
				if (! CBAccounterGetTxAccountValue(storage, txID, info->accountID, &valueInt)){
					CBLogError("Could not obtain the value of a transaction.");
					CBFoundTransactionReturnError
				}
				// Get the address
				if (CBDatabaseReadValue(storage->accountTxDetails, accountTxDetailsKey, (*curDetails)->accountTxDetails.addrHash, 20, CB_ACCOUNT_TX_DETAILS_ADDR, false) != CB_DATABASE_INDEX_FOUND) {
					CBLogError("Could not read the address of a transaction for an account.");
					CBFoundTransactionReturnError
				}
			}
			(*curDetails)->accountTxDetails.amount = valueInt;
			// Finished adding data for the account.
			curDetails = &(*curDetails)->next;
			// Write the balance
			if (branch == CB_NO_BRANCH) {
				// Adust unconf balance
				if (! CBAccounterAdjustUnconfBalance(storage, info->accountID, valueInt)) {
					CBLogError("Could not adjust the balance of unconfirmed transactions for an account when a new transaction has been found.");
					CBFoundTransactionReturnError
				}
				// Set a zero order num for the key array for the branch account time transaction entry.
				memset(CB_KEY_ARRAY + CB_BRANCH_ACCOUNT_TIME_TX_ORDERNUM, 0, 8);
				// Do not set cumulative balance. Not needed.
			}else{
				// Else write the cumulative balance for the transaction and get the next order number
				uint64_t balance;
				if (! CBAccounterGetLastAccountBranchSectionBalance(storage, info->accountID, branchSectionID, &balance)) {
					CBLogError("Could not read the cumulative balance for a transaction.");
					CBFoundTransactionReturnError
				}
				// Set the order num into the key array for the branch account time transaction entry.
				memcpy(CB_KEY_ARRAY + CB_BRANCH_ACCOUNT_TIME_TX_ORDERNUM, nextOrderNumData, 8);
				// Set balance into data array
				balance += valueInt;
				CBInt64ToArray(CB_DATA_ARRAY, 0, balance);
			}
			// Create the branch account time transaction entry.
			CB_KEY_ARRAY[CB_BRANCH_ACCOUNT_TIME_TX_BRANCH] = branchSectionID;
			CBInt64ToArray(CB_KEY_ARRAY, CB_BRANCH_ACCOUNT_TIME_TX_ACCOUNT_ID, info->accountID);
			CBInt64ToArrayBigEndian(CB_KEY_ARRAY, CB_BRANCH_ACCOUNT_TIME_TX_TIMESTAMP, time);
			CBInt64ToArray(CB_KEY_ARRAY, CB_BRANCH_ACCOUNT_TIME_TX_TX_ID, txID);
			CBDatabaseWriteValue(storage->branchSectionAccountTimeTx, CB_KEY_ARRAY, CB_DATA_ARRAY, 8);
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
		CBInt64ToArray(CB_KEY_ARRAY, 0, accountID);
		CBIndexFindStatus res = CBDatabaseReadValue(storage->accountUnconfBalance, CB_KEY_ARRAY, CB_DATA_ARRAY, 8, 0, false);
		if (res == CB_DATABASE_INDEX_ERROR) {
			CBLogError("There was an error trying to get the balance of unconfirmed transactions.");
			return false;
		}
		if (res == CB_DATABASE_INDEX_FOUND) {
			uint64_t txBalanceRaw = CBArrayToInt64(CB_DATA_ARRAY, 0);
			*balance = txBalanceRaw & 0x7FFFFFFFFFFFFFFF;
			if (! (txBalanceRaw & 0x8000000000000000))
				*balance = -*balance;
		}else
			*balance = 0;
		return true;
	}
	uint64_t ubalance;
	if (! CBAccounterGetLastAccountBranchSectionBalance(storage, accountID, CBAccounterGetLastBranchSectionID(storage, branch), &ubalance)) {
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
	uint8_t * key = CBDatabaseRangeIteratorGetKey(&cursor->it);
	// Get the transaction hash
	memcpy(CB_KEY_ARRAY, key + CB_BRANCH_ACCOUNT_TIME_TX_TX_ID, 8);
	if (CBDatabaseReadValue(cursor->storage->txDetails, CB_KEY_ARRAY, details->txHash, 32, CB_TX_DETAILS_HASH, false) != CB_DATABASE_INDEX_FOUND) {
		CBLogError("Could not read a transaction's hash.");
		return CB_ERROR;
	}
	// Get the address and value
	memcpy(CB_KEY_ARRAY + CB_ACCOUNT_TX_DETAILS_ACCOUNT_ID, txCursor->branchSectionAccountTimeTxMin + CB_BRANCH_ACCOUNT_TIME_TX_ACCOUNT_ID, 8);
	memcpy(CB_KEY_ARRAY + CB_ACCOUNT_TX_DETAILS_TX_ID, key + CB_BRANCH_ACCOUNT_TIME_TX_TX_ID, 8);
	if (CBDatabaseReadValue(cursor->storage->accountTxDetails, CB_KEY_ARRAY, details->accountTxDetails.addrHash, 20, CB_ACCOUNT_TX_DETAILS_ADDR, false) != CB_DATABASE_INDEX_FOUND) {
		CBLogError("Could not read a transaction's source/destination address.");
		return CB_ERROR;
	}
	// ??? Here the read is being done on the same key twice. Cache the index search result?
	if (CBDatabaseReadValue(cursor->storage->accountTxDetails, CB_KEY_ARRAY, CB_DATA_ARRAY, 8, CB_ACCOUNT_TX_DETAILS_VALUE, false) != CB_DATABASE_INDEX_FOUND) {
		CBLogError("Could not read a transaction's value.");
		return CB_ERROR;
	}
	uint64_t txValueRaw = CBArrayToInt64(CB_DATA_ARRAY, 0);
	details->accountTxDetails.amount = txValueRaw & 0x7FFFFFFFFFFFFFFF;
	if (! (txValueRaw & 0x8000000000000000))
		details->accountTxDetails.amount = -details->accountTxDetails.amount;
	// Read height
	if (key[CB_BRANCH_ACCOUNT_TIME_TX_BRANCH] == CB_NO_BRANCH)
		details->height = 0;
	else{
		CB_KEY_ARRAY[CB_BRANCH_TX_DETAILS_BRANCH] = key[CB_BRANCH_ACCOUNT_TIME_TX_BRANCH];
		memcpy(CB_KEY_ARRAY + CB_BRANCH_TX_DETAILS_TX_ID, key + CB_BRANCH_ACCOUNT_TIME_TX_TX_ID, 8);
		memcpy(CB_KEY_ARRAY + CB_BRANCH_TX_DETAILS_ORDER_NUM, key + CB_BRANCH_ACCOUNT_TIME_TX_ORDERNUM, 8);
		if (CBDatabaseReadValue(cursor->storage->branchSectionTxDetails, CB_KEY_ARRAY, CB_DATA_ARRAY, 4, CB_BRANCH_TX_DETAILS_HEIGHT, false) != CB_DATABASE_INDEX_FOUND) {
			CBLogError("Could not read the height of a transaction in a branch");
			return false;
		}
		details->height = CBArrayToInt32BigEndian(CB_DATA_ARRAY, 0);
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
			uint8_t * key = CBDatabaseRangeIteratorGetKey(&cursor->it);
			// Ensure unspent
			bool unspent = true;
			// See if we have figured this as spent already.
			uint8_t * outputKey = key + CB_ACCOUNT_OUTPUTS_OUTPUT_ID;
			CBFindResult res = CBAssociativeArrayFind(&uoCursor->spentOutputs, outputKey);
			if (res.found)
				unspent = false;
			// Read the spent status
			if (unspent && ! CBDatabaseRangeIteratorRead(&cursor->it, CB_DATA_ARRAY, 1, 0)){
				CBLogError("There was an error trying to read if an unspent output is active or not for an account.");
				return CB_ERROR;
			}
			if (unspent && ! CB_DATA_ARRAY[0])
				unspent = false;
			if (unspent && cursor->branchSections[cursor->currentSection] != CB_NO_BRANCH) {
				// Check that this output is not spent in a branchless transaction.
				uint32_t len;
				CB_KEY_ARRAY[0] = CB_NO_BRANCH;
				memcpy(CB_KEY_ARRAY + CB_ACCOUNT_OUTPUTS_ACCOUNT_ID, key + CB_ACCOUNT_OUTPUTS_ACCOUNT_ID, 8);
				memset(CB_KEY_ARRAY + CB_ACCOUNT_OUTPUTS_HEIGHT, 0xFF, 4); // Block height is made zero on branchless therefore UINT32_MAX minus zero gives 0xFFFFFFFF
				memcpy(CB_KEY_ARRAY + CB_ACCOUNT_OUTPUTS_OUTPUT_ID, key + CB_ACCOUNT_OUTPUTS_OUTPUT_ID, 8);
				if (! CBDatabaseGetLength(cursor->storage->branchSectionAccountOutputs, CB_KEY_ARRAY, &len)) {
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
			if (CBDatabaseReadValue(cursor->storage->outputDetails, outputKey, CB_DATA_ARRAY, 64, 0, false)!= CB_DATABASE_INDEX_FOUND) {
				CBLogError("Couldn't get the details for an output.");
				return CB_ERROR;
			}
			details->value = CBArrayToInt64(CB_DATA_ARRAY, CB_OUTPUT_DETAILS_VALUE);
			memcpy(details->txHash, CB_DATA_ARRAY + CB_OUTPUT_DETAILS_TX_HASH, 32);
			details->index = CBArrayToInt32(CB_DATA_ARRAY, CB_OUTPUT_DETAILS_INDEX);
			memcpy(details->watchedHash, CB_DATA_ARRAY + CB_OUTPUT_DETAILS_ID_HASH, 20);
			return CB_TRUE;
		}
		if (cursor->currentSection == CB_MAX_BRANCH_CACHE - uoCursor->numSections)
			// No more to try
			return CB_FALSE;
		cursor->started = false;
	}
}
bool CBAccounterGetLastAccountBranchSectionBalance(CBAccounterStorage * storage, uint64_t accountID, uint8_t branchSectionID, uint64_t * balance){
	uint8_t branchSectionAccountTimeTxMin[33];
	uint8_t branchSectionAccountTimeTxMax[33];
	CBInt64ToArray(branchSectionAccountTimeTxMin, CB_BRANCH_ACCOUNT_TIME_TX_ACCOUNT_ID, accountID);
	CBInt64ToArray(branchSectionAccountTimeTxMax, CB_BRANCH_ACCOUNT_TIME_TX_ACCOUNT_ID, accountID);
	memset(branchSectionAccountTimeTxMin + CB_BRANCH_ACCOUNT_TIME_TX_TIMESTAMP, 0, 24);
	memset(branchSectionAccountTimeTxMax + CB_BRANCH_ACCOUNT_TIME_TX_TIMESTAMP, 0xFF, 24);
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
			if (! CBDatabaseRangeIteratorRead(&lastTxIt, CB_DATA_ARRAY, 8, 0)) {
				CBLogError("Could not read the transaction cumulative balance for an account.");
				return false;
			}
			*balance = CBArrayToInt64(CB_DATA_ARRAY, 0);
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
	memcpy(CB_KEY_ARRAY, txHash, 32);
	if (CBDatabaseReadValue(storage->txHashToID, CB_KEY_ARRAY, CB_DATA_ARRAY, 8, 0, false) != CB_DATABASE_INDEX_FOUND) {
		CBLogError("Could not read a transaction ID for a hash when getting a transaction time.");
		return false;
	}
	// Read timestamp
	uint8_t timestamp[8];
	if (CBDatabaseReadValue(storage->txDetails, CB_DATA_ARRAY, timestamp, 8, CB_TX_DETAILS_TIMESTAMP, false) != CB_DATABASE_INDEX_FOUND) {
		CBLogError("Could not read a transaction's timestamp when getting a transaction time.");
		return false;
	}
	*time = CBArrayToInt64BigEndian(timestamp, 0);
	return true;
}
bool CBAccounterGetTxAccountValue(CBAccounterStorage * self, uint64_t txID, uint64_t accountID, int64_t * value){
	CBInt64ToArray(CB_KEY_ARRAY, CB_ACCOUNT_TX_DETAILS_ACCOUNT_ID, accountID);
	CBInt64ToArray(CB_KEY_ARRAY, CB_ACCOUNT_TX_DETAILS_TX_ID, txID);
	if (CBDatabaseReadValue(self->accountTxDetails, CB_KEY_ARRAY, CB_DATA_ARRAY, 8, CB_ACCOUNT_TX_DETAILS_VALUE, false) != CB_DATABASE_INDEX_FOUND) {
		CBLogError("Could not read an accounts transaction value.");
		return false;
	}
	uint64_t txValueRaw = CBArrayToInt64(CB_DATA_ARRAY, 0);
	*value = txValueRaw & 0x7FFFFFFFFFFFFFFF;
	if (! (txValueRaw & 0x8000000000000000))
		*value = -*value;
	return true;
}
CBErrBool CBAccounterIsOurs(CBDepObject uself, uint8_t * txHash){
	CBAccounterStorage * self = uself.ptr;
	memcpy(CB_KEY_ARRAY, txHash, 32);
	uint32_t length;
	if (!CBDatabaseGetLength(self->txHashToID, CB_KEY_ARRAY, &length))
		return CB_ERROR;
	return (length == CB_DOESNT_EXIST) ? CB_FALSE : CB_TRUE;
}
bool CBAccounterLostBranchlessTransaction(CBDepObject self, void * vtx){
	CBAccounterStorage * storage = self.ptr;
	CBTransaction * tx = vtx;
	// Lost a transaction not on any branch.
	// Loop through inputs looking for spent outputs that would therefore become unspent outputs.
	for (uint32_t x = 0; x < tx->inputNum; x++) {
		// Get the output ID
		memcpy(CB_KEY_ARRAY + CB_OUTPUT_HASH_AND_INDEX_TO_ID_HASH, CBByteArrayGetData(tx->inputs[x]->prevOut.hash), 32);
		CBInt32ToArray(CB_KEY_ARRAY, CB_OUTPUT_HASH_AND_INDEX_TO_ID_INDEX, tx->inputs[x]->prevOut.index);
		CBIndexFindStatus stat = CBDatabaseReadValue(storage->outputHashAndIndexToID, CB_KEY_ARRAY, CB_DATA_ARRAY, 8, 0, false);
		if (stat == CB_DATABASE_INDEX_ERROR) {
			CBLogError("Could not read the ID of an output reference.");
			return false;
		}
		if (stat == CB_DATABASE_INDEX_NOT_FOUND)
			// The output reference does not exist, so we can end here and continue to the next input.
			continue;
		// Remove output spent status for all accounts
		uint8_t outputAccountsMin[21];
		uint8_t outputAccountsMax[21];
		memcpy(outputAccountsMin + CB_OUTPUT_ACCOUNTS_OUTPUT_ID, CB_DATA_ARRAY, 8);
		memcpy(outputAccountsMax + CB_OUTPUT_ACCOUNTS_OUTPUT_ID, CB_DATA_ARRAY, 8);
		memset(outputAccountsMin + CB_OUTPUT_ACCOUNTS_ACCOUNTS_ID, 0, 8);
		memset(outputAccountsMax + CB_OUTPUT_ACCOUNTS_ACCOUNTS_ID, 0xFF, 8);
		CBDatabaseRangeIterator it = {outputAccountsMin, outputAccountsMax, storage->outputAccounts};
		if (CBDatabaseRangeIteratorFirst(&it) != CB_DATABASE_INDEX_FOUND) {
			CBLogError("Could not get the first account for an output when removing an output spent status from branchless.");
			return false;
		}
		// Prepare account output status key with the branch, zero height and output ID.
		CB_KEY_ARRAY[CB_ACCOUNT_OUTPUTS_BRANCH] = CB_NO_BRANCH;
		memset(CB_KEY_ARRAY + CB_ACCOUNT_OUTPUTS_HEIGHT, 0xFF, 4);
		memcpy(CB_KEY_ARRAY + CB_ACCOUNT_OUTPUTS_OUTPUT_ID, CB_DATA_ARRAY, 8);
		for (;;) {
			// Add the account ID into the key
			memcpy(CB_KEY_ARRAY + CB_ACCOUNT_OUTPUTS_ACCOUNT_ID, CBDatabaseRangeIteratorGetKey(&it) + CB_OUTPUT_ACCOUNTS_ACCOUNTS_ID, 8);
			// Now delete
			if (! CBDatabaseRemoveValue(storage->branchSectionAccountOutputs, CB_KEY_ARRAY, false)) {
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
	// Remove branch details for the transaction and lower the number of branches owning the transaction by one. If no more ownership remove the transaction details.
	// First get the transaction ID
	memcpy(CB_KEY_ARRAY, CBTransactionGetHash(tx), 32);
	if (CBDatabaseReadValue(storage->txHashToID, CB_KEY_ARRAY, CB_DATA_ARRAY, 8, 0, false) != CB_DATABASE_INDEX_FOUND) {
		CBLogError("Could not read a transaction ID for a hash.");
		return false;
	}
	// Get the transaction ID
	uint64_t txID = CBArrayToInt64(CB_DATA_ARRAY, 0);
	// Remove the transaction from branchless
	if (! CBAccounterRemoveTransactionFromBranch(storage, txID, CBTransactionGetHash(tx), CB_NO_BRANCH, NULL, NULL)) {
		CBLogError("Could not remove a transaction as branchless.");
		return false;
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
		// Get the key
		uint8_t * key = CBDatabaseRangeIteratorGetKey(&it);
		// Get the transaction hash
		uint8_t txHash[32];
		memcpy(CB_KEY_ARRAY, key + CB_TX_HEIGHT_BRANCH_AND_ID_TX_ID, 8);
		if (CBDatabaseReadValue(storage->txDetails, CB_KEY_ARRAY, txHash, 32, CB_TX_DETAILS_HASH, false) != CB_DATABASE_INDEX_FOUND) {
			CBLogError("Could not get the transaction hash.");
			CBFreeDatabaseRangeIterator(&it);
			return false;
		}
		// Get the transaction timestamp
		uint8_t timestamp[8];
		if (CBDatabaseReadValue(storage->txDetails, CB_KEY_ARRAY, timestamp, 8, CB_TX_DETAILS_TIMESTAMP, false) != CB_DATABASE_INDEX_FOUND) {
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
			uint8_t * txAccountKey = CBDatabaseRangeIteratorGetKey(&it2);
			// Change the branchSectionAccountTimeTx
			CB_KEY_ARRAY[CB_BRANCH_ACCOUNT_TIME_TX_BRANCH] = parentLastSectionIndex;
			memcpy(CB_KEY_ARRAY + CB_BRANCH_ACCOUNT_TIME_TX_ACCOUNT_ID, txAccountKey + CB_TX_ACCOUNTS_ACCOUNT_ID, 8);
			memcpy(CB_KEY_ARRAY + CB_BRANCH_ACCOUNT_TIME_TX_ORDERNUM, orderNum, 8);
			memcpy(CB_KEY_ARRAY + CB_BRANCH_ACCOUNT_TIME_TX_TIMESTAMP, timestamp, 8);
			memcpy(CB_KEY_ARRAY + CB_BRANCH_ACCOUNT_TIME_TX_TX_ID, txAccountKey + CB_TX_ACCOUNTS_TX_ID, 8);
			CB_NEW_KEY_ARRAY[CB_BRANCH_ACCOUNT_TIME_TX_BRANCH] = branchSectionIndex;
			memcpy(CB_NEW_KEY_ARRAY + CB_BRANCH_ACCOUNT_TIME_TX_ACCOUNT_ID, CB_KEY_ARRAY + CB_BRANCH_ACCOUNT_TIME_TX_ACCOUNT_ID, 32);
			if (! CBDatabaseChangeKey(storage->branchSectionAccountTimeTx, CB_KEY_ARRAY, CB_NEW_KEY_ARRAY, false)){
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
				uint8_t * branchSectionAccountOutputsKey = CBDatabaseRangeIteratorGetKey(&it3);
				// Change key
				CB_NEW_KEY_ARRAY[CB_ACCOUNT_OUTPUTS_BRANCH] = branchSectionIndex;
				memcpy(CB_NEW_KEY_ARRAY + CB_ACCOUNT_OUTPUTS_ACCOUNT_ID, branchSectionAccountOutputsKey + CB_ACCOUNT_OUTPUTS_ACCOUNT_ID, 20);
				if (! CBDatabaseChangeKey(storage->branchSectionAccountOutputs, branchSectionAccountOutputsKey, CB_NEW_KEY_ARRAY, false)){
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
		CB_KEY_ARRAY[CB_BRANCH_TX_DETAILS_BRANCH] = parentLastSectionIndex;
		memcpy(CB_KEY_ARRAY + CB_BRANCH_TX_DETAILS_ORDER_NUM, key + CB_TX_HEIGHT_BRANCH_AND_ID_ORDER_NUM, 8);
		memcpy(CB_KEY_ARRAY + CB_BRANCH_TX_DETAILS_TX_ID, key + CB_TX_HEIGHT_BRANCH_AND_ID_TX_ID, 8);
		CB_NEW_KEY_ARRAY[CB_BRANCH_TX_DETAILS_BRANCH] = branchSectionIndex;
		memcpy(CB_NEW_KEY_ARRAY + CB_BRANCH_TX_DETAILS_ORDER_NUM, CB_KEY_ARRAY + CB_BRANCH_TX_DETAILS_ORDER_NUM, 16);
		if (! CBDatabaseChangeKey(storage->branchSectionTxDetails, CB_KEY_ARRAY, CB_NEW_KEY_ARRAY, false)){
			CBLogError("Could not change the branch section tx height key to a new branch section when creating a new branch.");
			return false;
		}
		// Change the branch height and ID entry branch
		CB_NEW_KEY_ARRAY[CB_TX_HEIGHT_BRANCH_AND_ID_BRANCH] = branchSectionIndex;
		memcpy(CB_NEW_KEY_ARRAY + CB_TX_HEIGHT_BRANCH_AND_ID_HEIGHT, key + CB_TX_HEIGHT_BRANCH_AND_ID_HEIGHT, 20);
		if (! CBDatabaseChangeKey(storage->txBranchSectionHeightAndID, key, CB_NEW_KEY_ARRAY, false)){
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
	CBInt64ToArray(CB_KEY_ARRAY, 0, txID);
	if (CBDatabaseReadValue(storage->txDetails, CB_KEY_ARRAY, &branchOwners, 1, CB_TX_DETAILS_BRANCH_INSTANCES, false) != CB_DATABASE_INDEX_FOUND) {
		CBLogError("Could not read the number of branches that own a transaction.");
		return false;
	}
	// Also read timestamp for later
	CBInt64ToArray(CB_KEY_ARRAY, 0, txID);
	if (CBDatabaseReadValue(storage->txDetails, CB_KEY_ARRAY, timestampData, 8, CB_TX_DETAILS_TIMESTAMP, false) != CB_DATABASE_INDEX_FOUND) {
		CBLogError("Could not read a transactions timestamp.");
		return false;
	}
	// Lower the number of branch owners
	branchOwners--;
	if (branchOwners == 0) {
		// No more owners, delete transaction information.
		if (! CBDatabaseRemoveValue(storage->txDetails, CB_KEY_ARRAY, false)) {
			CBLogError("Could not remove transaction details.");
			return false;
		}
		if (! CBDatabaseRemoveValue(storage->txHashToID, txHash, false)) {
			CBLogError("Could not remove a transaction hash to ID entry.");
			return false;
		}
	}else
		CBDatabaseWriteValueSubSection(storage->txDetails, CB_KEY_ARRAY, &branchOwners, 1, CB_TX_DETAILS_BRANCH_INSTANCES);
	// If in a block-chain branch, remove transaction branch data
	if (branch != CB_NO_BRANCH) {
		// Only need to delete from last branch section as this only happens when deleting branches
		CB_KEY_ARRAY[CB_BRANCH_TX_DETAILS_BRANCH] = branchSectionIndex;
		memcpy(CB_KEY_ARRAY + CB_BRANCH_TX_DETAILS_ORDER_NUM, orderNum, 8);
		CBInt64ToArray(CB_KEY_ARRAY, CB_BRANCH_TX_DETAILS_TX_ID, txID);
		if (! CBDatabaseRemoveValue(storage->branchSectionTxDetails, CB_KEY_ARRAY, false)) {
			CBLogError("Couldn't remove the branch transaction details");
			return false;
		}
		// Remove txBranchSectionHeightAndID
		CB_KEY_ARRAY[CB_TX_HEIGHT_BRANCH_AND_ID_BRANCH] = branchSectionIndex;
		memcpy(CB_KEY_ARRAY + CB_TX_HEIGHT_BRANCH_AND_ID_HEIGHT, height, 4);
		CBInt64ToArray(CB_KEY_ARRAY, CB_TX_HEIGHT_BRANCH_AND_ID_TX_ID, txID);
		memcpy(CB_KEY_ARRAY + CB_TX_HEIGHT_BRANCH_AND_ID_ORDER_NUM, orderNum, 8);
		if (! CBDatabaseRemoveValue(storage->txBranchSectionHeightAndID, CB_KEY_ARRAY, false)) {
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
		uint8_t * key = CBDatabaseRangeIteratorGetKey(&it);
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
		CB_KEY_ARRAY[CB_BRANCH_ACCOUNT_TIME_TX_BRANCH] = branchSectionIndex;
		memcpy(CB_KEY_ARRAY + CB_BRANCH_ACCOUNT_TIME_TX_ACCOUNT_ID, key + CB_TX_ACCOUNTS_ACCOUNT_ID, 8);
		memcpy(CB_KEY_ARRAY + CB_BRANCH_ACCOUNT_TIME_TX_TIMESTAMP, timestampData, 8);
		if (branch == CB_NO_BRANCH)
			memset(CB_KEY_ARRAY + CB_BRANCH_ACCOUNT_TIME_TX_ORDERNUM, 0, 8);
		else
			memcpy(CB_KEY_ARRAY + CB_BRANCH_ACCOUNT_TIME_TX_ORDERNUM, orderNum, 8);
		CBInt64ToArray(CB_KEY_ARRAY, CB_BRANCH_ACCOUNT_TIME_TX_TX_ID, txID);
		if (! CBDatabaseRemoveValue(storage->branchSectionAccountTimeTx, CB_KEY_ARRAY, false)) {
			CBLogError("Could not remove an account transaction entry for a branch ordered by time.");
			CBFreeDatabaseRangeIterator(&it);
			return false;
		}
		CBIndexFindStatus status;
		// Remove non-branch-specific details if no more bramches own the transaction
		if (branchOwners == 0) {
			// Remove the account transaction details
			memcpy(CB_KEY_ARRAY + CB_ACCOUNT_TX_DETAILS_ACCOUNT_ID, key + CB_TX_ACCOUNTS_ACCOUNT_ID, 8);
			memcpy(CB_KEY_ARRAY + CB_ACCOUNT_TX_DETAILS_TX_ID, key + CB_TX_ACCOUNTS_TX_ID, 8);
			if (! CBDatabaseRemoveValue(storage->accountTxDetails, CB_KEY_ARRAY, false)) {
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
		uint8_t * key = CBDatabaseRangeIteratorGetKey(&it);
		// Get the output ID
		if (CBDatabaseReadValue(storage->outputHashAndIndexToID, key, CB_DATA_ARRAY, 8, 0, false) != CB_DATABASE_INDEX_FOUND) {
			CBLogError("Could not read the ID of an output.");
			CBFreeDatabaseRangeIterator(&it);
			return false;
		}
		// Loop through the output accounts
		uint8_t outputAccountsMin[16];
		uint8_t outputAccountsMax[16];
		memcpy(outputAccountsMin + CB_OUTPUT_ACCOUNTS_OUTPUT_ID, CB_DATA_ARRAY, 8);
		memcpy(outputAccountsMax + CB_OUTPUT_ACCOUNTS_OUTPUT_ID, CB_DATA_ARRAY, 8);
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
			uint8_t * outputAccountKey = CBDatabaseRangeIteratorGetKey(&accountIt);
			// Remove the output as unspent from the account for this branch, if it exists as so.
			CB_KEY_ARRAY[CB_ACCOUNT_OUTPUTS_BRANCH] = branchSectionIndex;
			memcpy(CB_KEY_ARRAY + CB_ACCOUNT_OUTPUTS_ACCOUNT_ID, outputAccountKey + CB_OUTPUT_ACCOUNTS_ACCOUNTS_ID, 8);
			if (branch != CB_NO_BRANCH)
				memcpy(CB_KEY_ARRAY + CB_ACCOUNT_OUTPUTS_HEIGHT, height, 4);
			else
				memset(CB_KEY_ARRAY + CB_ACCOUNT_OUTPUTS_HEIGHT, 0, 4);
			memcpy(CB_KEY_ARRAY + CB_ACCOUNT_OUTPUTS_OUTPUT_ID, CB_DATA_ARRAY, 8);
			if (! CBDatabaseRemoveValue(storage->branchSectionAccountOutputs, CB_KEY_ARRAY, false)) {
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
			memcpy(CB_KEY_ARRAY, CB_DATA_ARRAY, 8);
			if (! CBDatabaseRemoveValue(storage->outputDetails, CB_KEY_ARRAY, false)) {
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
	memcpy(CB_KEY_ARRAY, hash, 32);
	uint32_t len;
	if (! CBDatabaseGetLength(storageObj->txHashToID, CB_KEY_ARRAY, &len)) {
		CBLogError("There was an error trying to determine is a transaction exists.");
		return CB_ERROR;
	}
	return len != CB_DOESNT_EXIST;
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
