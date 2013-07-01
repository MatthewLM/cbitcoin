//
//  CBAccounter.c
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

#include "CBAccounter.h"

uint8_t cbKeySizes[] = {
	0, /**< CB_TYPE_ACCOUNTER_DETAILS : Null - Details for the accounter. */
	8, /**< CB_TYPE_TX_DETAILS : The 8 byte transaction ID - Details for a transaction independant of the account. */
	9, /**< CB_TYPE_BRANCH_ACCOUNT_DETAILS : 1 byte branch number and 8 byte account ID (CB_NO_BRANCH for unconfirmed) - Stores the number of transaction in the account and the balance */
	8, /**< CB_TYPE_OUTPUT_DETAILS : 8 byte output ID - Stores the value. */
	9, /**< CB_TYPE_BRANCH_OUTPUT_DETAILS : 1 byte branch number and 8 byte output ID - Stores the spent status */
	16, /**< CB_TYPE_ACCOUNT_TX_DETAILS : 8 byte account ID and 8 byte transaction ID - Transaction details for this account */
	16, /**< CB_TYPE_BRANCH_ACCOUNT_TIME_TX : 1 byte branch number, 8 byte account ID, 8 byte transaction timestamp and 8 byte transaction ID - Null */
	9, /**< CB_TYPE_BRANCH_TX_DETAILS : 1 byte branch number and 8 byte transaction ID - Details for a transaction on a branch, no information for unconfirmed transactions. (Not used for CB_NO_BRANCH) */
	16, /**< CB_TYPE_OUTPUT_ACCOUNTS : 8 byte output ID and 8 byte account ID - Null */
	17, /**< CB_TYPE_ACCOUNT_UNSPENT_OUTPUTS : 1 byte branch number, 8 byte account ID, 8 byte output ID - Null */
	16, /**< CB_TYPE_TX_ACCOUNTS : 8 byte transaction ID and 8 byte account ID - Null */
	32, /**< CB_TYPE_TX_HASH_TO_ID : 32 byte transaction hash - The transaction ID */
	13, /**< CB_TYPE_TX_HEIGHT_BRANCH_AND_ID : 1 byte branch number, 4 byte block height and 8 byte transaction ID - Null (Not used for CB_NO_BRANCH) */
	36, /**< CB_TYPE_OUTPUT_HASH_AND_INDEX_TO_ID : 32 byte transaction hash and 4 byte output transaction index - The output reference ID */
	28, /**< CB_TYPE_WATCHED_HASHES : 20 byte hash and 8 byte account ID - Null */
};

bool CBNewAccounter(CBDepObject * accounter, char * dataDir) {
	CBAccounter * self = malloc(sizeof(*self));
	if (NOT CBInitDatabase(CBGetDatabase(self), dataDir, "acnt")) {
		CBLogError("Could not create a database object for an accounter.");
		free(self);
		return 0;
	}
	CBLoadIndex(CBGetDatabase(self), 0, 1, <#uint32_t cacheLimit#>)
	if (<#condition#>) {
		<#statements#>
	}
	CBDatabaseIndex accounterDetails;
	CBDatabaseIndex txDetails;
	CBDatabaseIndex accountDetails;
	CBDatabaseIndex outputDetails;
	CBDatabaseIndex branchOutputDetails;
	CBDatabaseIndex accountTxDetails;
	CBDatabaseIndex branchAccountTimeTx;
	CBDatabaseIndex branchTxDetails;
	CBDatabaseIndex outputAccounts;
	CBDatabaseIndex accountUnspentOutputs;
	CBDatabaseIndex txAccounts;
	CBDatabaseIndex txHashToID;
	CBDatabaseIndex txHeightBranchAndID;
	CBDatabaseIndex outputHashAndIndexToID;
	CBDatabaseIndex watchedHashes;
	
	uint8_t data[24];
	CBDecKey(key, CB_TYPE_ACCOUNTER_DETAILS);
	if (CBDatabaseGetLength(CBGetDatabase(self), CB_TYPE_ACCOUNTER_DETAILS) == CB_DOESNT_EXIST) {
		// Write the initial data
		self->lastAccountID = 0;
		self->nextOutputRefID = 0;
		self->nextTxID = 0;
		memset(data, 0, 24);
		if (NOT CBDatabaseWriteValue(CBGetDatabase(self), key, data, 24)) {
			CBLogError("Could not write the initial data for an accounter.");
			return 0;
		}
		if (NOT CBDatabaseCommit(CBGetDatabase(self))) {
			CBLogError("Could not commit the initial data for an accounter.");
			return 0;
		}
	}else{
		// Load the ID information
		if (NOT CBDatabaseReadValue(CBGetDatabase(self), key, data, 24, 0)) {
			CBLogError("Could not read the next ID information for an accounter.");
			return 0;
		}
		self->lastAccountID = CBArrayToInt64(data, CB_ACCOUNTER_DETAILS_ACCOUNT_ID);
		self->nextOutputRefID = CBArrayToInt64(data, CB_ACCOUNTER_DETAILS_OUTPUT_ID);
		self->nextTxID = CBArrayToInt64(data, CB_ACCOUNTER_DETAILS_TX_ID);
	}
	return (uint64_t) self;
}
void CBFreeAccounter(uint64_t iself){
	CBFreeDatabase((CBDatabase *)iself);
}
bool CBAccounterAdjustAccountBalanceByTx(CBDatabase * database, uint8_t * accountTxDetails, uint8_t * accountDetailsKey){
	uint8_t data[8];
	if (NOT CBDatabaseReadValue(database, accountTxDetails, data, 8, CB_ACCOUNT_TX_DETAILS_VALUE)) {
		CBLogError("Could not read an accounts transaction value.");
		return false;
	}
	uint64_t txValueRaw = CBArrayToInt64(data, 0);
	int64_t txValue = txValueRaw & 0x7FFFFFFFFFFFFFFF;
	if (NOT (txValueRaw & 0x8000000000000000))
		txValue = -txValue;
	// Lower the transaction balance
	if (NOT CBDatabaseReadValue(database, accountDetailsKey, data, 8, CB_ACCOUNT_DETAILS_BALANCE)) {
		CBLogError("Could not read the balance for an account's branchless transactions.");
		return false;
	}
	CBInt64ToArray(data, 0, CBArrayToInt64(data, 0) + txValue);
	if (NOT CBDatabaseWriteValue(database, accountDetailsKey, data, 8)) {
		CBLogError("Could not write the adjusted balance for an account's branchless transactions.");
		return false;
	}
	return true;
}
bool CBAccounterAddWatchedOutputToAccount(uint64_t iself, uint8_t * hash, uint64_t accountID){
	CBDatabase * database = (CBDatabase *)iself;
	CBDecKey(watchedHashesKey, CB_TYPE_WATCHED_HASHES);
	memcpy(watchedHashesKey + CB_WATCHED_HASHES_HASH, hash, 20);
	CBInt64ToArray(watchedHashesKey, CB_WATCHED_HASHES_ACCOUNT_ID, accountID);
	return CBDatabaseWriteValue(database, watchedHashesKey, NULL, 0);
}
bool CBAccounterBranchlessTransactionToBranch(uint64_t iself, void * vtx, uint32_t blockHeight, uint8_t branch){
	CBDatabase * database = (CBDatabase *)iself;
	CBTransaction * tx = vtx;
	uint8_t data[8];
	// A branchless transaction is found in a branch.
	// Loop through outputs and change references to the new branch
	CBDecKey(outputHashAndIndexToIDKey, CB_TYPE_OUTPUT_HASH_AND_INDEX_TO_ID);
	CBDecKey(branchOutputDetails, CB_TYPE_BRANCH_OUTPUT_DETAILS);
	CBDecKey(newBranchOutputDetails, CB_TYPE_BRANCH_OUTPUT_DETAILS);
	for (uint32_t x = 0; x < tx->outputNum; x++) {
		// Get the output ID of this output, if it exists.
		memcpy(outputHashAndIndexToIDKey + CB_OUTPUT_HASH_AND_INDEX_TO_ID_HASH, CBTransactionGetHash(tx), 32);
		CBInt32ToArray(outputHashAndIndexToIDKey, CB_OUTPUT_HASH_AND_INDEX_TO_ID_INDEX, x);
		if (CBDatabaseGetLength(database, outputHashAndIndexToIDKey) != CB_DOESNT_EXIST) {
			// The output ID exists so get it
			if (NOT CBDatabaseReadValue(database, outputHashAndIndexToIDKey, data, 8, 0)) {
				CBLogError("Could not get the ID of a branchless output so we can move the spent status to a branch");
				return false;
			}
			uint64_t outputID = CBArrayToInt64(data, 0);
			// Create key for the output branch information.
			branchOutputDetails[CB_BRANCH_OUTPUT_DETAILS_BRANCH] = CB_NO_BRANCH;
			memcpy(branchOutputDetails + CB_BRANCH_OUTPUT_DETAILS_OUPUT_ID, data, 8);
			// Check the spent status of this output
			if (NOT CBDatabaseReadValue(database, branchOutputDetails, data, 1, CB_OUTPUT_REF_BRANCH_DATA_SPENT)) {
				CBLogError("Could not get the spent status of a branchless output.");
				return false;
			}
			if (NOT data[0]) {
				// Unspent
				// Loop through the output's accounts and change the unspent output references to the branch the output is being added to.
				CBDecKey(outputAccountsMin, CB_TYPE_OUTPUT_ACCOUNTS);
				CBDecKey(outputAccountsMax, CB_TYPE_OUTPUT_ACCOUNTS);
				CBInt64ToArray(outputAccountsMin, CB_OUTPUT_ACCOUNTS_OUTPUT_ID, outputID);
				CBInt64ToArray(outputAccountsMax, CB_OUTPUT_ACCOUNTS_OUTPUT_ID, outputID);
				memset(outputAccountsMin + CB_OUTPUT_ACCOUNTS_ACCOUNTS_ID, 0, 8);
				memset(outputAccountsMax + CB_OUTPUT_ACCOUNTS_ACCOUNTS_ID, 0xFF, 8);
				CBRangeIterator it;
				CBAssociativeArrayRangeIteratorStart(&database->index, outputAccountsMin, outputAccountsMax, &it);
				CBDecKey(accountUnspentOutputKey, CB_TYPE_ACCOUNT_UNSPENT_OUTPUTS);
				CBDecKey(newAccountUnspentOutputKey, CB_TYPE_ACCOUNT_UNSPENT_OUTPUTS);
				memcpy(accountUnspentOutputKey + CB_ACCOUNT_UNSPENT_OUTPUTS_OUTPUT_ID, data, 8);
				memcpy(newAccountUnspentOutputKey + CB_ACCOUNT_UNSPENT_OUTPUTS_OUTPUT_ID, data, 8);
				for (;;) {
					// Get the account key
					uint8_t * key = CBRangeIteratorGetPointer(&it);
					// Make the unspent output key
					accountUnspentOutputKey[CB_ACCOUNT_UNSPENT_OUTPUTS_BRANCH] = CB_NO_BRANCH;
					memcpy(accountUnspentOutputKey + CB_ACCOUNT_UNSPENT_OUTPUTS_ACCOUNT_ID, key + CB_OUTPUT_ACCOUNTS_ACCOUNTS_ID, 8);
					// Make the new key for the branch
					newAccountUnspentOutputKey[CB_ACCOUNT_UNSPENT_OUTPUTS_BRANCH] = branch;
					memcpy(newAccountUnspentOutputKey + CB_ACCOUNT_UNSPENT_OUTPUTS_ACCOUNT_ID, key + CB_OUTPUT_ACCOUNTS_ACCOUNTS_ID, 8);
					// Change the key
					if (NOT CBDatabaseChangeKey(database, accountUnspentOutputKey, newAccountUnspentOutputKey)) {
						CBLogError("Couldn't change the branch information for an unspent output from branchless to a branch the output has been added to.");
					}
					// Iterate to the next account if available
					if (CBAssociativeArrayRangeIteratorNext(&database->index, &it))
						break;
				}
			}
			// Create the new key
			newBranchOutputDetails[CB_BRANCH_OUTPUT_DETAILS_BRANCH] = branch;
			memcpy(newBranchOutputDetails + CB_BRANCH_OUTPUT_DETAILS_OUPUT_ID, data, 8);
			// Change the key
			if (NOT CBDatabaseChangeKey(database, branchOutputDetails, newBranchOutputDetails)) {
				CBLogError("Could not change the key of a branchless output's branch information to include the branch it is changed to.");
				return false;
			}
		}
	}
	// Get the transaction ID
	CBDecKey(txHashToIDKey, CB_TYPE_TX_HASH_TO_ID);
	memcpy(txHashToIDKey + CB_TX_HASH_TO_ID_HASH, CBTransactionGetHash(tx), 32);
	if (NOT CBDatabaseReadValue(database, txHashToIDKey, data, 8, 0)) {
		CBLogError("Could not get a transaction ID for adding to a branch.");
		return false;
	}
	uint64_t txID = CBArrayToInt64(data, 8);
	// Add branch transaction details
	CBDecKey(branchTxDetailsKey, CB_TYPE_BRANCH_TX_DETAILS);
	branchTxDetailsKey[CB_BRANCH_TX_DETAILS_BRANCH] = branch;
	CBInt64ToArray(branchTxDetailsKey, CB_BRANCH_TX_DETAILS_TX_ID, txID);
	CBInt32ToArray(data, CB_TX_BRANCH_DETAILS_BLOCK_HEIGHT, blockHeight);
	if (NOT CBDatabaseWriteValue(database, branchTxDetailsKey, data, 4)) {
		CBLogError("Could not write the branch transaction details for a branchless transaction added to a branch.");
		return false;
	}
	// Add branch, height and ID entry.
	CBDecKey(branchHeightAndIDKey, CB_TYPE_TX_HEIGHT_BRANCH_AND_ID);
	branchHeightAndIDKey[CB_TX_HEIGHT_BRANCH_AND_ID_BRANCH] = branch;
	CBInt32ToArray(branchHeightAndIDKey, CB_TX_HEIGHT_BRANCH_AND_ID_HEIGHT, blockHeight);
	CBInt64ToArray(branchHeightAndIDKey, CB_TX_HEIGHT_BRANCH_AND_ID_TX_ID, txID);
	if (NOT CBDatabaseWriteValue(database, branchHeightAndIDKey, NULL, 0)) {
		CBLogError("Could not create a branch height and transaction ID entry for a branchless transaction added to a branch.");
		return false;
	}
	// Loop through transaction accounts
	CBDecKey(txAccountMin, CB_TYPE_TX_ACCOUNTS);
	CBDecKey(txAccountMax, CB_TYPE_TX_ACCOUNTS);
	CBInt64ToArray(txAccountMin, CB_TX_ACCOUNTS_TX_ID, txID);
	CBInt64ToArray(txAccountMax, CB_TX_ACCOUNTS_TX_ID, txID);
	memset(txAccountMin + CB_TX_ACCOUNTS_ACCOUNT_ID, 0, 8);
	memset(txAccountMax + CB_TX_ACCOUNTS_ACCOUNT_ID, 0xFF, 8);
	CBRangeIterator it;
	CBDecKey(branchAccountDetailsKey, CB_TYPE_BRANCH_ACCOUNT_DETAILS);
	CBDecKey(accountTxDetails, CB_TYPE_ACCOUNT_TX_DETAILS);
	CBDecKey(branchAccountTimeTxKey, CB_TYPE_BRANCH_ACCOUNT_TIME_TX);
	CBDecKey(newBranchAccountTimeTxKey, CB_TYPE_BRANCH_ACCOUNT_TIME_TX);
	// Get the timestamp of the transaction
	CBDecKey(txDetailsKey, CB_TYPE_TX_DETAILS);
	CBInt64ToArray(txDetailsKey, CB_TX_DETAILS_TX_ID, txID);
	if (NOT CBDatabaseReadValue(database, txDetailsKey, data, 8, CB_TX_DETAILS_TIMESTAMP)) {
		CBLogError("Could not read a transactions timestampfor moving a branchless transaction into a branch.");
		return false;
	}
	uint64_t timestamp = CBArrayToInt64(data, 0);
	CBAssociativeArrayRangeIteratorStart(&database->index, txAccountMin, txAccountMax, &it);
	for (;;) {
		// Get the transaction account key
		uint8_t * key = CBRangeIteratorGetPointer(&it);
		// Create the account Tx key and branch account details keys.
		memcpy(accountTxDetails + CB_ACCOUNT_TX_DETAILS_ACCOUNT_ID, key + CB_TX_ACCOUNTS_ACCOUNT_ID, 8);
		memcpy(accountTxDetails + CB_ACCOUNT_TX_DETAILS_TX_ID, key + CB_TX_ACCOUNTS_TX_ID, 8);
		branchAccountDetailsKey[CB_BRANCH_ACCOUNT_DETAILS_BRANCH] = CB_NO_BRANCH;
		memcpy(branchAccountDetailsKey + CB_BRANCH_ACCOUNT_DETAILS_ACCOUNT_ID, key + CB_TX_ACCOUNTS_ACCOUNT_ID, 8);
		// Adjust balance of branchless transactions
		if (NOT CBAccounterAdjustAccountBalanceByTx(database, accountTxDetails, branchAccountDetailsKey)) {
			CBLogError("Could not adjust the accounts balance for branchless transactions.");
			return false;
		}
		// Adjust balance of branch we are adding to
		branchAccountDetailsKey[CB_BRANCH_ACCOUNT_DETAILS_BRANCH] = branch;
		if (NOT CBAccounterAdjustAccountBalanceByTx(database, accountTxDetails, branchAccountDetailsKey)) {
			CBLogError("Could not adjust the accounts balance for branchless transactions.");
			return false;
		}
		// Change the branch account time transaction entry, for the new branch.
		branchAccountTimeTxKey[CB_BRANCH_ACCOUNT_TIME_TX_BRANCH] = CB_NO_BRANCH;
		memcpy(branchAccountTimeTxKey + CB_BRANCH_ACCOUNT_TIME_TX_ACCOUNT_ID, key + CB_TX_ACCOUNTS_ACCOUNT_ID, 8);
		CBInt64ToArray(branchAccountTimeTxKey, CB_BRANCH_ACCOUNT_TIME_TX_TIMESTAMP, timestamp);
		memcpy(branchAccountTimeTxKey + CB_BRANCH_ACCOUNT_TIME_TX_TX_ID, key + CB_TX_ACCOUNTS_TX_ID, 8);
		memcpy(newBranchAccountTimeTxKey + CB_BRANCH_ACCOUNT_TIME_TX_ACCOUNT_ID, branchAccountTimeTxKey + CB_BRANCH_ACCOUNT_TIME_TX_ACCOUNT_ID, cbKeySizes[CB_TYPE_BRANCH_ACCOUNT_TIME_TX] - 1);
		newBranchAccountTimeTxKey[CB_BRANCH_ACCOUNT_TIME_TX_BRANCH] = branch;
		if (NOT CBDatabaseChangeKey(database, branchAccountTimeTxKey, newBranchAccountTimeTxKey)) {
			CBLogError("Could not change the account transaction entry for a branch ordered by time key, for moving a branchless transaction into a branch.");
			return false;
		}
		// Iterate to the next transaction account if there is any.
		if (CBAssociativeArrayRangeIteratorNext(&database->index, &it))
			break;
	}
	return true;
}
bool CBAccounterChangeOutputReferenceSpentStatus(CBDatabase * self, CBPrevOut * prevOut, uint8_t branch, bool spent, CBAssociativeArray * txInfo){
	// Get the key
	CBDecKey(hashAndIndexToIDKey, CB_TYPE_OUTPUT_HASH_AND_INDEX_TO_ID);
	CBDecKey(branchOutputDetailsKey, CB_TYPE_BRANCH_OUTPUT_DETAILS);
	memcpy(hashAndIndexToIDKey + CB_OUTPUT_HASH_AND_INDEX_TO_ID_HASH, CBByteArrayGetData(prevOut->hash), 32);
	CBInt32ToArray(hashAndIndexToIDKey, CB_OUTPUT_HASH_AND_INDEX_TO_ID_INDEX, prevOut->index);
	if (CBDatabaseGetLength(self, hashAndIndexToIDKey) == CB_DOESNT_EXIST)
		// The ouptut reference does not exist, so we can end here.
		return true;
	if (NOT CBDatabaseReadValue(self, hashAndIndexToIDKey, branchOutputDetailsKey + CB_BRANCH_OUTPUT_DETAILS_OUPUT_ID, 8, 0)) {
		CBLogError("Could not read the index of an output reference.");
		return false;
	}
	branchOutputDetailsKey[CB_BRANCH_OUTPUT_DETAILS_BRANCH] = branch;
	// Save the data
	if (NOT CBDatabaseWriteValueSubSection(self, branchOutputDetailsKey, (uint8_t *)&spent, 1, CB_OUTPUT_REF_BRANCH_DATA_SPENT)){
		CBLogError("Could not write the new spent status for an output reference.");
		return false;
	}
	// For each account that owns the output remove or add from/to the unspent outputs.
	CBDecKey(unspentOutKey, CB_TYPE_ACCOUNT_UNSPENT_OUTPUTS);
	unspentOutKey[CB_BRANCH_OUTPUT_DETAILS_BRANCH] = branch;
	memcpy(unspentOutKey + CB_ACCOUNT_UNSPENT_OUTPUTS_OUTPUT_ID, branchOutputDetailsKey + CB_BRANCH_OUTPUT_DETAILS_OUPUT_ID, 8);
	// Get the first account
	CBRangeIterator it;
	CBDecKey(minOutputAccountKey, CB_TYPE_OUTPUT_ACCOUNTS);
	CBDecKey(maxOutputAccountKey, CB_TYPE_OUTPUT_ACCOUNTS);
	memcpy(minOutputAccountKey + CB_OUTPUT_ACCOUNTS_OUTPUT_ID, branchOutputDetailsKey + CB_BRANCH_OUTPUT_DETAILS_OUPUT_ID, 8);
	memcpy(maxOutputAccountKey + CB_OUTPUT_ACCOUNTS_OUTPUT_ID, branchOutputDetailsKey + CB_BRANCH_OUTPUT_DETAILS_OUPUT_ID, 8);
	memset(minOutputAccountKey + CB_OUTPUT_ACCOUNTS_ACCOUNTS_ID, 0, 8);
	memset(maxOutputAccountKey + CB_OUTPUT_ACCOUNTS_ACCOUNTS_ID, 0xFF, 8);
	CBAssociativeArrayRangeIteratorStart(&self->index, minOutputAccountKey, maxOutputAccountKey, &it);
	for (;;) {
		// Add account ID
		uint8_t * outputAccountKey = CBRangeIteratorGetPointer(&it);
		memcpy(unspentOutKey + CB_ACCOUNT_UNSPENT_OUTPUTS_ACCOUNT_ID, outputAccountKey + CB_OUTPUT_ACCOUNTS_ACCOUNTS_ID, 8);
		if (spent) {
			// Remove entry from unspent outputs.
			if (NOT CBDatabaseRemoveValue(self, outputAccountKey)) {
				CBLogError("Could not remove an unspent output entry");
				return false;
			}
		}else{
			// Add to unspent outputs
			if (NOT CBDatabaseWriteValue(self, outputAccountKey, NULL, 0)) {
				CBLogError("Could not add an unspent output entry");
				return false;
			}
		}
		// If we have a txInfo array, add the account debit information
		if (txInfo) {
			// Get the output value
			uint8_t data[8];
			CBDecKey(outDetailsKey, CB_TYPE_OUTPUT_DETAILS);
			memcpy(outDetailsKey + CB_OUTPUT_DETAILS_OUTPUT_ID, branchOutputDetailsKey + CB_BRANCH_OUTPUT_DETAILS_OUPUT_ID, 8);
			if (NOT CBDatabaseReadValue(self, outDetailsKey, data, 8, CB_OUTPUT_REF_DATA_VALUE)) {
				CBLogError("Could not read the value of an output.");
				return false;
			}
			uint64_t value = CBArrayToInt64(data, 0);
			// First look to see if the accountID exists
			uint32_t accountID = CBArrayToInt32(unspentOutKey, CB_ACCOUNT_UNSPENT_OUTPUTS_ACCOUNT_ID);
			CBFindResult res = CBAssociativeArrayFind(txInfo, &accountID);
			if (res.found) {
				// Add to the debit amount
				CBTransactionAccountCreditDebit * info = CBFindResultToPointer(res);
				info->debitAmount += value;
			}else{
				// Add new element
				CBTransactionAccountCreditDebit * info = malloc(sizeof(*info));
				info->accountID = accountID;
				info->creditAmount = 0;
				info->debitAmount = value;
				info->foundCreditAddr = false;
			}
		}
	}
	return true;
}
bool CBAccounterCommit(uint64_t self){
	return CBDatabaseCommit((CBDatabase *) self);
}
CBCompare CBCompareUInt32(void * vint1, void * vint2){
	uint32_t * int1 = vint1;
	uint32_t * int2 = vint2;
	if (*int1 > *int2)
		return CB_COMPARE_MORE_THAN;
	if (*int1 < *int2)
		return CB_COMPARE_LESS_THAN;
	return CB_COMPARE_EQUAL;
}
bool CBAccounterDeleteBranch(uint64_t iself, uint8_t branch){
	CBDatabase * database = (CBDatabase *)iself;
	uint8_t data[32];
	// Loop through transactions
	CBDecKey(txBranchHeightAndIDMin, CB_TYPE_TX_HEIGHT_BRANCH_AND_ID);
	CBDecKey(txBranchHeightAndIDMax, CB_TYPE_TX_HEIGHT_BRANCH_AND_ID);
	txBranchHeightAndIDMin[CB_TX_HEIGHT_BRANCH_AND_ID_BRANCH] = branch;
	txBranchHeightAndIDMax[CB_TX_HEIGHT_BRANCH_AND_ID_BRANCH] = branch;
	memset(txBranchHeightAndIDMin + CB_TX_HEIGHT_BRANCH_AND_ID_HEIGHT, 0, 12);
	memset(txBranchHeightAndIDMax + CB_TX_HEIGHT_BRANCH_AND_ID_HEIGHT, 0xFF, 12);
	CBRangeIterator it;
	if (CBAssociativeArrayRangeIteratorStart(&database->index, txBranchHeightAndIDMin, txBranchHeightAndIDMax, &it)) for (;;) {
		// Get the key
		uint8_t * key = CBRangeIteratorGetPointer(&it);
		// Remove the transaction from the branch
		CBDecKey(txDetailsKey, CB_TYPE_TX_DETAILS);
		memcpy(txDetailsKey + CB_TX_DETAILS_TX_ID, key + CB_TX_HEIGHT_BRANCH_AND_ID_TX_ID, 8);
		CBDecKey(txHashToIDKey, CB_TYPE_TX_HASH_TO_ID);
		// Get the transaction hash
		if (NOT CBDatabaseReadValue(database, txDetailsKey, data, 32, CB_TX_DETAILS_HASH)) {
			CBLogError("Could not get a transaction hash details.");
			return false;
		}
		// Add hash to the hash to ID key.
		memcpy(txHashToIDKey + CB_TX_HASH_TO_ID_HASH, data, 32);
		if (NOT CBAccounterRemoveTransactionFromBranch(database, txDetailsKey, txHashToIDKey, branch)) {
			CBLogError("Could not remove a transaction from a branch.");
			return false;
		}
		// Iterate to next transaction
		if (CBAssociativeArrayRangeIteratorNext(&database->index, &it))
			break;
	}
}
bool CBAccounterFoundTransaction(uint64_t iself, void * vtx, uint32_t blockHeight, uint32_t time, uint8_t branch){
	CBAccounter * self = (CBAccounter *)iself;
	CBDatabase * database = (CBDatabase *)iself;
	CBTransaction * tx = vtx;
	// Create transaction credit-debit information array.
	CBAssociativeArray txInfo;
	if (NOT CBInitAssociativeArray(&txInfo, CBCompareUInt32, free)) {
		CBLogError("Could not create the transaction credit-debit information array");
		return false;
	}
	// Look through inputs for any unspent outputs being spent.
	for (uint32_t x = 0; x < tx->inputNum; x++) {
		// Make previous output spent for this branch.
		if (NOT CBAccounterChangeOutputReferenceSpentStatus(database, &tx->inputs[x]->prevOut, branch, true, &txInfo)) {
			CBLogError("Could not change the status of an output to spent.");
			return false;
		}
	}
	// Scan for watched addresses.
	CBDecKey(minHashKey, CB_TYPE_WATCHED_HASHES);
	CBDecKey(maxHashKey, CB_TYPE_WATCHED_HASHES);
	CBDecKey(outputDetailsKey, CB_TYPE_OUTPUT_DETAILS);
	CBDecKey(branchOutputDetailsKey, CB_TYPE_BRANCH_OUTPUT_DETAILS);
	CBDecKey(outputAccountsKey, CB_TYPE_OUTPUT_ACCOUNTS);
	CBDecKey(unspentOutputsKey, CB_TYPE_ACCOUNT_UNSPENT_OUTPUTS);
	CBDecKey(outputHashAndIndexToIDKey, CB_TYPE_OUTPUT_HASH_AND_INDEX_TO_ID);
	CBDecKey(accounterDetailsKey, CB_TYPE_ACCOUNTER_DETAILS);
	// Set the transaction hash into the hash and index to ID key.
	memcpy(outputHashAndIndexToIDKey + CB_OUTPUT_HASH_AND_INDEX_TO_ID_HASH, CBTransactionGetHash(tx), 32);
	for (uint32_t x = 0; x < tx->outputNum; x++) {
		// Try to obtain an address (or multisig script hash) from this output
		// Get the identifying hash of the output of output
		uint8_t hash[32]; // 32 for mutlisig hashing
		if (CBTransactionOuputGetHash(tx->outputs[x], hash))
			continue;
		// Now look through accounts for this output.
		memcpy(minHashKey + CB_WATCHED_HASHES_HASH, hash, 20);
		memcpy(maxHashKey + CB_WATCHED_HASHES_HASH, hash, 20);
		memset(minHashKey + CB_WATCHED_HASHES_ACCOUNT_ID, 0, 8);
		memset(maxHashKey + CB_WATCHED_HASHES_ACCOUNT_ID, 0xFF, 8);
		CBRangeIterator it;
		if (CBAssociativeArrayRangeIteratorStart(&database->index, minHashKey, maxHashKey, &it)){
			uint64_t outputID;
			uint8_t data[8];
			// Add the index ot the hash and index to ID key
			CBInt32ToArray(outputHashAndIndexToIDKey, CB_OUTPUT_HASH_AND_INDEX_TO_ID_INDEX, x);
			// Detemine if the output exists already
			if (CBDatabaseGetLength(database, outputHashAndIndexToIDKey) == CB_DOESNT_EXIST) {
				// Doesn't exist, get the next ID
				outputID = self->nextOutputRefID;
				// Save the output details
				CBInt64ToArray(outputDetailsKey, CB_OUTPUT_DETAILS_OUTPUT_ID, outputID);
				CBInt64ToArray(data, CB_OUTPUT_REF_DATA_VALUE, tx->outputs[x]->value);
				if (NOT CBDatabaseWriteValue(database, outputDetailsKey, data, 8)) {
					CBLogError("Could not write the details for a new output reference.");
					return false;
				}
				// Save the output hash and index to ID entry
				CBInt32ToArray(data, 0, outputID);
				if (NOT CBDatabaseWriteValue(database, outputHashAndIndexToIDKey, data, 4)) {
					CBLogError("Could not write the output hash and index to ID entry.");
					return false;
				}
				// Increment the next output reference ID and save it.
				CBInt32ToArray(data, 0, ++self->nextOutputRefID);
				if (NOT CBDatabaseWriteValueSubSection(database, accounterDetailsKey, data, 4, CB_ACCOUNTER_DETAILS_OUTPUT_ID)) {
					CBLogError("Could not write the next output reference ID for the accounter.");
					return false;
				}
			}else{
				// Get the ID of this output
				if (NOT CBDatabaseReadValue(database, outputHashAndIndexToIDKey, data, 8, 0)) {
					CBLogError("Could not read the output hash and index to ID entry.");
					return false;
				}
				outputID = CBArrayToInt64(data, 0);
			}
			// Save the branch details as unspent
			branchOutputDetailsKey[CB_BRANCH_OUTPUT_DETAILS_BRANCH] = branch;
			CBInt64ToArray(branchOutputDetailsKey, CB_BRANCH_OUTPUT_DETAILS_OUPUT_ID, outputID);
			data[0] = false;
			if (NOT CBDatabaseWriteValue(database, branchOutputDetailsKey, data, 1)) {
				CBLogError("Could not write the branch details for a new output reference.");
				return false;
			}
			// Set the output ID to the output accounts key.
			CBInt64ToArray(outputAccountsKey, CB_OUTPUT_ACCOUNTS_OUTPUT_ID, outputID);
			// Set the output ID and branch to the unspent outputs key.
			unspentOutputsKey[CB_ACCOUNT_UNSPENT_OUTPUTS_BRANCH] = branch;
			CBInt64ToArray(unspentOutputsKey, CB_ACCOUNT_UNSPENT_OUTPUTS_OUTPUT_ID, outputID);
			for (;;) {
				// Found an account that is watching this output
				uint8_t * foundKey = CBRangeIteratorGetPointer(&it);
				uint32_t accountID = CBArrayToInt64(foundKey, 21);
				// Detemine if the output account details exist alredy or not.
				CBInt64ToArray(outputAccountsKey, CB_OUTPUT_ACCOUNTS_ACCOUNTS_ID, accountID);
				if (CBDatabaseGetLength(database, outputAccountsKey) == CB_DOESNT_EXIST) {
					// Add an entry to the output account information.
					if (NOT CBDatabaseWriteValue(database, outputAccountsKey, NULL, 0)) {
						CBLogError("Could not write an output account entry.");
						return false;
					}
				}
				// Add an entry to the unspent outputs for the account
				CBInt64ToArray(unspentOutputsKey, CB_ACCOUNT_UNSPENT_OUTPUTS_ACCOUNT_ID, accountID);
				if (NOT CBDatabaseWriteValue(database, unspentOutputsKey, NULL, 0)) {
					CBLogError("Could not write an unspent output entry for an acount on a branch.");
					return false;
				}
				// Add the output value to the credit amount for this transaction
				CBFindResult res = CBAssociativeArrayFind(&txInfo, &accountID);
				CBTransactionAccountCreditDebit * details;
				if (res.found) {
					details = CBFindResultToPointer(res);
					details->creditAmount += tx->outputs[x]->value;
				}else{
					details = malloc(sizeof(*details));
					details->accountID = accountID;
					details->creditAmount = tx->outputs[x]->value;
					details->debitAmount = 0;
					details->foundCreditAddr = false;
				}
				if (NOT details->foundCreditAddr) {
					// Add this address as a credit address
					details->foundCreditAddr = true;
					details->creditAddrIndexIsZero = NOT x;
					memcpy(details->creditAddr, hash, 20);
				}
				// Iterate to the next account if exists
				if (CBAssociativeArrayRangeIteratorNext(&database->index, &it))
					break;
			}
		}
	}
	// Loop through all accounts seeing if any credits or debits have been made
	CBPosition pos;
	if (CBAssociativeArrayGetFirst(&txInfo, &pos)){
		// Create hash to id key
		CBDecKey(txDetailsKey, CB_TYPE_TX_DETAILS);
		CBDecKey(txHashToIDKey, CB_TYPE_TX_HASH_TO_ID);
		memcpy(txHashToIDKey + CB_TX_HASH_TO_ID_HASH, CBTransactionGetHash(tx), 32);
		uint64_t txID;
		uint8_t data[37];
		// Determine if the transaction exists already.
		if (CBDatabaseGetLength(database, txHashToIDKey) == CB_DOESNT_EXIST) { // ??? Use Read here and return CB_DOESNT_EXIST or similar?
			// It doesn't exist, get the next ID.
			txID = self->nextTxID;
			// Create the transaction details
			CBInt64ToArray(txDetailsKey, CB_TX_DETAILS_TX_ID, txID);
			CBInt32ToArray(data, CB_TX_DETAILS_TIMESTAMP, time);
			data[CB_TX_DETAILS_BRANCH_INSTANCES] = 1;
			memcpy(data + CB_TX_DETAILS_HASH, CBTransactionGetHash(tx), 32);
			if (NOT CBDatabaseWriteValue(database, txDetailsKey, data, 37)) {
				CBLogError("Couldn't write the details for a new transaction for the accounter.");
				return false;
			}
			// Create the transaction hash to ID entry
			// Use txDetailsKey which has the ID
			if (NOT CBDatabaseWriteValue(database, txHashToIDKey, txDetailsKey + CB_TX_DETAILS_TX_ID, 8)) {
				CBLogError("Couldn't write the hash to ID entry for a transaction.");
				return false;
			}
			// Increment the next ID
			CBInt64ToArray(data, 0, ++self->nextTxID);
			// Save to storage
			CBDecKey(accounterDetailsKey, CB_TYPE_ACCOUNTER_DETAILS);
			if (NOT CBDatabaseWriteValueSubSection(database, accounterDetailsKey, data, 8, CB_ACCOUNTER_DETAILS_TX_ID)) {
				CBLogError("Couldn't increment the next transaction ID.");
				return false;
			}
		}else{
			// Obtain the transaction ID.
			if (NOT CBDatabaseReadValue(database, txHashToIDKey, data, 8, 0)) {
				CBLogError("Couldn't read a transaction ID.");
				return false;
			}
			txID = CBArrayToInt64(data, 0);
			// Increase the number of branches that own this transaction
			CBInt64ToArray(txDetailsKey, CB_TX_DETAILS_TX_ID, txID);
			if (NOT CBDatabaseReadValue(database, txDetailsKey, data, 1, CB_TX_DETAILS_BRANCH_INSTANCES)) {
				CBLogError("Couldn't read the number of branches that own a transaction.");
				return false;
			}
			data[0]++;
			if (NOT CBDatabaseWriteValueSubSection(database, txDetailsKey, data, 1, CB_TX_DETAILS_BRANCH_INSTANCES)) {
				CBLogError("Couldn't write the incremented number of branches that own a transaction.");
				return false;
			}
			// Update the time to the first time found
			if (NOT CBDatabaseReadValue(database, txDetailsKey, data, 8, CB_TX_DETAILS_TIMESTAMP)) {
				CBLogError("Couldn't read the timestamp for a transaction.");
				return false;
			}
			time = CBArrayToInt64(data, 0);
		}
		if (blockHeight != CB_NO_BRANCH) {
			// Set the block height for the branch and this transaction
			CBDecKey(branchTXDetailsKey, CB_TYPE_BRANCH_TX_DETAILS);
			branchTXDetailsKey[CB_BRANCH_TX_DETAILS_BRANCH] = branch;
			CBInt64ToArray(branchTXDetailsKey, CB_BRANCH_TX_DETAILS_TX_ID, txID);
			CBInt32ToArray(data, 0, blockHeight);
			if (NOT CBDatabaseWriteValue(database, branchTXDetailsKey, data, 4)) {
				CBLogError("Couldn't write the block height for a transaction on a branch.");
				return false;
			}
			// Create the branch and block height entry for the transaction
			CBDecKey(txHeightBranchIDKey, CB_TYPE_TX_HEIGHT_BRANCH_AND_ID);
			txHeightBranchIDKey[CB_TX_HEIGHT_BRANCH_AND_ID_BRANCH] = branch;
			CBInt32ToArray(txHeightBranchIDKey, CB_TX_HEIGHT_BRANCH_AND_ID_HEIGHT, blockHeight);
			CBInt64ToArray(txHeightBranchIDKey, CB_TX_HEIGHT_BRANCH_AND_ID_TX_ID, txID);
			if (NOT CBDatabaseWriteValue(database, txHeightBranchIDKey, NULL, 0)) {
				CBLogError("Couldn't write the branch and height to the transaction ID entry.");
				return false;
			}
		}
		CBDecKey(txAccountKey, CB_TYPE_TX_ACCOUNTS);
		CBInt64ToArray(txAccountKey, CB_TX_ACCOUNTS_TX_ID, txID);
		CBDecKey(accountTxDetailsKey, CB_TYPE_ACCOUNT_TX_DETAILS);
		for (;;) {
			// Get the transaction information for an account
			CBTransactionAccountCreditDebit * info = pos.node->elements[pos.index];
			// Add the account ID to the key
			CBInt64ToArray(txAccountKey, CB_TX_ACCOUNTS_ACCOUNT_ID, info->accountID);
			// Set the account's transaction details key account
			CBInt64ToArray(accountTxDetailsKey, CB_ACCOUNT_TX_DETAILS_ACCOUNT_ID, info->accountID);
			// Determine if the transaction account details exist already.
			if (CBDatabaseGetLength(database, txAccountKey) == CB_DOESNT_EXIST) {
				// Create the transaction account entry
				if (NOT CBDatabaseWriteValue(database, txAccountKey, NULL, 0)) {
					CBLogError("Could not create a transaction account entry.");
					return false;
				}
				// Set the value
				int64_t valueInt = info->creditAmount - info->debitAmount;
				CBInt64ToArray(data, CB_ACCOUNT_TX_DETAILS_VALUE, labs(valueInt) | ((valueInt > 0) ? 0x8000000000000000 : 0));
				// Set the address.
				if (valueInt > 0)
					// Use credit address
					memcpy(data + CB_ACCOUNT_TX_DETAILS_ADDR, info->creditAddr, 20);
				else{
					// Find a debit address if there is one we can use.
					if (NOT info->foundCreditAddr || NOT info->creditAddrIndexIsZero) {
						// Try index 0.
						if (CBTransactionOuputGetHash(tx->outputs[0], data + CB_ACCOUNT_TX_DETAILS_ADDR))
							// The output is not supported, use zero to represent this.
							memset(data + CB_ACCOUNT_TX_DETAILS_ADDR, 0, 20);
					}else{
						// Else we have found a credit address and it is at the zero index
						if (tx->outputNum == 1)
							// No other outputs, must be wasting money to miners.
							memset(data + CB_ACCOUNT_TX_DETAILS_ADDR, 0, 20);
						// Try index 1
						else if (CBTransactionOuputGetHash(tx->outputs[1], data + CB_ACCOUNT_TX_DETAILS_ADDR))
							memset(data + CB_ACCOUNT_TX_DETAILS_ADDR, 0, 20);
					}
				}
				// Now save the account's transaction details
				if (NOT CBDatabaseWriteValue(database, accountTxDetailsKey, data, 28)) {
					CBLogError("Could not write the transaction details for an account.");
					return false;
				}
			}
			// Create the branch account time transaction entry.
			CBDecKey(branchAccountTimeTxKey, CB_TYPE_BRANCH_ACCOUNT_TIME_TX);
			branchAccountTimeTxKey[CB_BRANCH_ACCOUNT_TIME_TX_BRANCH] = branch;
			CBInt64ToArray(branchAccountTimeTxKey, CB_BRANCH_ACCOUNT_TIME_TX_ACCOUNT_ID, info->accountID);
			CBInt64ToArray(branchAccountTimeTxKey, CB_BRANCH_ACCOUNT_TIME_TX_TIMESTAMP, time);
			CBInt64ToArray(branchAccountTimeTxKey, CB_BRANCH_ACCOUNT_TIME_TX_TX_ID, txID);
			if (NOT CBDatabaseWriteValue(database, branchAccountTimeTxKey, NULL, 0)) {
				CBLogError("Could not write an entry for account transactions ordered by the transaction timestamp on a branch.");
				return false;
			}
			// Adjust the account's branch balance, increasing the balance for this transaction.
			CBDecKey(branchAccountDetailsKey, CB_TYPE_BRANCH_ACCOUNT_DETAILS);
			branchAccountDetailsKey[CB_BRANCH_ACCOUNT_DETAILS_BRANCH] = branch;
			CBInt64ToArray(branchAccountDetailsKey, CB_BRANCH_ACCOUNT_DETAILS_ACCOUNT_ID, info->accountID);
			if (NOT CBAccounterAdjustAccountBalanceByTx(database, accountTxDetailsKey, branchAccountDetailsKey)) {
				CBLogError("Could not adjust the accounts balance for branchless transactions.");
				return false;
			}
			// Get the next transaction account information.
			if (CBAssociativeArrayIterate(&txInfo, &pos))
				break;
		}
	}
	return true;
}
CBGetTxResult CBAccounterGetFirstTransactionBetween(uint64_t iself, uint8_t branch, uint64_t accountID, uint64_t timeMin, uint64_t timeMax, uint64_t * txIDCursor, CBTransactionDetails * details){
	CBDatabase * database = (CBDatabase *)iself;
	CBDecKey(branchAccountTimeMin, CB_TYPE_BRANCH_ACCOUNT_TIME_TX);
	CBDecKey(branchAccountTimeMax, CB_TYPE_BRANCH_ACCOUNT_TIME_TX);
	branchAccountTimeMin[CB_BRANCH_ACCOUNT_TIME_TX_BRANCH] = branch;
	branchAccountTimeMax[CB_BRANCH_ACCOUNT_TIME_TX_BRANCH] = branch;
	CBInt64ToArray(branchAccountTimeMin, CB_BRANCH_ACCOUNT_TIME_TX_ACCOUNT_ID, accountID);
	CBInt64ToArray(branchAccountTimeMax, CB_BRANCH_ACCOUNT_TIME_TX_ACCOUNT_ID, accountID);
	CBInt64ToArray(branchAccountTimeMin, CB_BRANCH_ACCOUNT_TIME_TX_TIMESTAMP, timeMin);
	CBInt64ToArray(branchAccountTimeMax, CB_BRANCH_ACCOUNT_TIME_TX_TIMESTAMP, timeMax);
	CBInt64ToArray(branchAccountTimeMin, CB_BRANCH_ACCOUNT_TIME_TX_TX_ID, *txIDCursor);
	memset(branchAccountTimeMax + CB_BRANCH_ACCOUNT_TIME_TX_TX_ID, 0xFF, 8);
	CBRangeIterator it;
	if (CBAssociativeArrayRangeIteratorStart(&database->index, branchAccountTimeMin, branchAccountTimeMax, &it)) {
		// Get the key
		uint8_t * key = CBRangeIteratorGetPointer(&it);
		// Get the transaction hash
		CBDecKey(txDetailsKey, CB_TYPE_TX_DETAILS);
		memcpy(txDetailsKey + CB_TX_DETAILS_TX_ID, key + CB_BRANCH_ACCOUNT_TIME_TX_TX_ID, 8);
		if (NOT CBDatabaseReadValue(database, txDetailsKey, details->txHash, 32, CB_TX_DETAILS_HASH)) {
			CBLogError("Could not read a transaction's hash.");
			return CB_GET_TX_ERROR;
		}
		// Get the address and value
		CBDecKey(accountTxDetailsKey, CB_TYPE_ACCOUNT_TX_DETAILS);
		CBInt64ToArray(accountTxDetailsKey, CB_ACCOUNT_TX_DETAILS_ACCOUNT_ID, accountID);
		memcpy(accountTxDetailsKey + CB_ACCOUNT_TX_DETAILS_TX_ID, key + CB_BRANCH_ACCOUNT_TIME_TX_TX_ID, 8);
		if (NOT CBDatabaseReadValue(database, accountTxDetailsKey, details->addrHash, 20, CB_ACCOUNT_TX_DETAILS_ADDR)) {
			CBLogError("Could not read a transaction's credit/debit address.");
			return CB_GET_TX_ERROR;
		}
		uint8_t data[8];
		if (NOT CBDatabaseReadValue(database, accountTxDetailsKey, data, 8, CB_ACCOUNT_TX_DETAILS_VALUE)) {
			CBLogError("Could not read a transaction's value.");
			return CB_GET_TX_ERROR;
		}
		uint64_t txValueRaw = CBArrayToInt64(data, 0);
		details->amount = txValueRaw & 0x7FFFFFFFFFFFFFFF;
		if (NOT (txValueRaw & 0x8000000000000000))
			details->amount = -details->amount;
		// Add the timestamp
		details->amount = CBArrayToInt64(key, CB_BRANCH_ACCOUNT_TIME_TX_TIMESTAMP);
		// Update the transaction ID cursor
		*txIDCursor = CBArrayToInt64(key, CB_BRANCH_ACCOUNT_TIME_TX_TX_ID) + 1;
		return CB_GET_TX_OK;
	}else return CB_GET_TX_NONE;
}
bool CBAccounterLostBranchlessTransaction(uint64_t iself, void * vtx){
	CBDatabase * database = (CBDatabase *)iself;
	CBTransaction * tx = vtx;
	// Lost a transaction not on any branch.
	// Loop through inputs looking for spent outputs that would therefore become unspent outputs.
	for (uint32_t x = 0; x < tx->inputNum; x++) {
		// Change all output references back to spent
		if (NOT CBAccounterChangeOutputReferenceSpentStatus(database, &tx->inputs[x]->prevOut, CB_NO_BRANCH, false, NULL)) {
			CBLogError("Could not change the status of an output to unspent.");
			return false;
		}
	}
	// Remove branch details for the transaction and lower the number of branches owning the transaction by one. If no more ownership remove the transaction details.
	// First get the transaction ID
	uint8_t data[8];
	CBDecKey(txHashToIDKey, CB_TYPE_TX_HASH_TO_ID);
	memcpy(txHashToIDKey + CB_TX_HASH_TO_ID_HASH, CBTransactionGetHash(tx), 32);
	if (NOT CBDatabaseReadValue(database, txHashToIDKey, data, 8, 0)) {
		CBLogError("Could not read a transaction ID for a hash.");
		return false;
	}
	uint64_t txID = CBArrayToInt64(data, 0);
	// Remove the transaction from the branch
	CBDecKey(txDetailsKey, CB_TYPE_TX_DETAILS);
	CBInt64ToArray(txDetailsKey, CB_TX_DETAILS_TX_ID, txID);
	if (NOT CBAccounterRemoveTransactionFromBranch(database, txDetailsKey, txHashToIDKey, CB_NO_BRANCH)) {
		CBLogError("Could not remove a transaction as branchless.");
		return false;
	}
	return true;
}
uint64_t CBAccounterNewAccount(uint64_t iself){
	// Increment the next account ID and use it, thus avoiding 0 for errors
	uint64_t nextAccountID = ++((CBAccounter *)iself)->lastAccountID;
	CBDatabase * database = (CBDatabase *)iself;
	// Save last account ID
	uint8_t data[8];
	CBInt64ToArray(data, 0, nextAccountID);
	CBDecKey(accounterDetailsKey, CB_TYPE_ACCOUNTER_DETAILS);
	if (NOT CBDatabaseWriteValueSubSection(database, accounterDetailsKey, data, 8, CB_ACCOUNTER_DETAILS_ACCOUNT_ID)) {
		CBLogError("Could not increment the account ID");
		return 0;
	}
	// Return the account ID to use.
	return nextAccountID;
}
bool CBAccounterNewBranch(uint64_t iself, uint8_t newBranch, uint8_t inherit){
	CBDatabase * database = (CBDatabase *)iself;
	// Make new branch identical to inherited branch
	// Loop through transactions
	CBDecKey(branchTxDetailsMin, CB_TYPE_BRANCH_TX_DETAILS);
	CBDecKey(branchTxDetailsMax, CB_TYPE_BRANCH_TX_DETAILS);
	branchTxDetailsMin[CB_BRANCH_TX_DETAILS_BRANCH] = inherit;
	branchTxDetailsMax[CB_BRANCH_TX_DETAILS_BRANCH] = inherit;
	memset(branchTxDetailsMin + CB_BRANCH_TX_DETAILS_TX_ID, 0, 8);
	memset(branchTxDetailsMax + CB_BRANCH_TX_DETAILS_TX_ID, 0xFF, 8);
	CBRangeIterator it;
	uint8_t data[32];
	if (CBAssociativeArrayRangeIteratorStart(&database->index, branchTxDetailsMin, branchTxDetailsMax, &it)) for (;;) {
		// Get the key
		uint8_t * key = CBRangeIteratorGetPointer(&it);
		// Get the branch transaction details
		if (NOT CBDatabaseReadValue(database, key, data, 4, 0)) {
			CBLogError("Could not read the block height of a transaction for a branch");
			return false;
		}
		// Save for the new branch
		key[CB_BRANCH_TX_DETAILS_BRANCH] = newBranch;
		if (NOT CBDatabaseWriteValue(database, key, data, 4)) {
			CBLogError("Could not write the block height of a transaction for the new branch.");
			return false;
		}
		// Now make the branch height and transaction ID entry.
		CBDecKey(branchHeightAndIDKey, CB_TYPE_TX_HEIGHT_BRANCH_AND_ID);
		branchHeightAndIDKey[CB_TX_HEIGHT_BRANCH_AND_ID_BRANCH] = newBranch;
		memcpy(branchHeightAndIDKey + CB_TX_HEIGHT_BRANCH_AND_ID_HEIGHT, data, 4);
		memcpy(branchHeightAndIDKey + CB_TX_HEIGHT_BRANCH_AND_ID_TX_ID, key + CB_BRANCH_TX_DETAILS_TX_ID, 8);
		if (NOT CBDatabaseWriteValue(database, key, NULL, 0)) {
			CBLogError("Could not write the branch height and transaction ID entry for a new branch.");
			return false;
		}
		// Increase branch ownership
		CBDecKey(txDetailsKey, CB_TYPE_TX_DETAILS);
		memcpy(txDetailsKey + CB_TX_DETAILS_TX_ID, key + CB_BRANCH_TX_DETAILS_TX_ID, 8);
		if (NOT CBDatabaseReadValue(database, txDetailsKey, data, 1, CB_TX_DETAILS_BRANCH_INSTANCES)) {
			CBLogError("Could not read the number of branches that own a transaction.");
			return false;
		}
		data[0]++;
		if (NOT CBDatabaseWriteValueSubSection(database, txDetailsKey, data, 1, CB_TX_DETAILS_BRANCH_INSTANCES)) {
			CBLogError("Could not save the incremented the number of branches that own a transaction.");
			return false;
		}
		// Iterate to the next transaction
		if (CBAssociativeArrayRangeIteratorNext(&database->index, &it))
			break;
	}
	// Loop through outputs
	CBDecKey(branchOutputDetailsMin, CB_TYPE_BRANCH_OUTPUT_DETAILS);
	CBDecKey(branchOutputDetailsMax, CB_TYPE_BRANCH_OUTPUT_DETAILS);
	branchOutputDetailsMin[CB_BRANCH_OUTPUT_DETAILS_BRANCH] = inherit;
	branchOutputDetailsMax[CB_BRANCH_OUTPUT_DETAILS_BRANCH] = inherit;
	memset(branchOutputDetailsMin + CB_BRANCH_OUTPUT_DETAILS_OUPUT_ID, 0, 8);
	memset(branchOutputDetailsMax + CB_BRANCH_OUTPUT_DETAILS_OUPUT_ID, 0xFF, 8);
	if (CBAssociativeArrayRangeIteratorStart(&database->index, branchOutputDetailsMin, branchOutputDetailsMax, &it)) for (;;) {
		// Get the output key
		uint8_t * branchOutputDetailsKey = CBRangeIteratorGetPointer(&it);
		// Get the branch output details
		if (NOT CBDatabaseReadValue(database, branchOutputDetailsKey, data, 1, CB_OUTPUT_REF_BRANCH_DATA_SPENT)) {
			CBLogError("Could not get the spent status of an output in an inherited branch.");
			return false;
		}
		branchOutputDetailsKey[CB_BRANCH_OUTPUT_DETAILS_BRANCH] = newBranch;
		if (NOT CBDatabaseWriteValue(database, branchOutputDetailsKey, data, 1)) {
			CBLogError("Could not write the spent status of an output in a new branch.");
			return false;
		}
		// Iterate to the next output
		if (CBAssociativeArrayRangeIteratorNext(&database->index, &it))
			break;
	}
	// Loop through accounts
	CBDecKey(branchAccountDetailsMin, CB_TYPE_BRANCH_ACCOUNT_DETAILS);
	CBDecKey(branchAccountDetailsMax, CB_TYPE_BRANCH_ACCOUNT_DETAILS);
	branchAccountDetailsMin[CB_BRANCH_ACCOUNT_DETAILS_BRANCH] = inherit;
	branchAccountDetailsMax[CB_BRANCH_ACCOUNT_DETAILS_BRANCH] = inherit;
	memset(branchAccountDetailsMin + CB_BRANCH_ACCOUNT_DETAILS_ACCOUNT_ID, 0, 8);
	memset(branchAccountDetailsMax + CB_BRANCH_ACCOUNT_DETAILS_ACCOUNT_ID, 0xFF, 8);
	if (CBAssociativeArrayRangeIteratorStart(&database->index, branchAccountDetailsMin, branchAccountDetailsMax, &it)) for (;;) {
		// Get the account key
		uint8_t * branchAccountDetailsKey = CBRangeIteratorGetPointer(&it);
		// Get the branch account details (balance)
		if (NOT CBDatabaseReadValue(database, branchAccountDetailsKey, data, 8, CB_ACCOUNT_DETAILS_BALANCE)) {
			CBLogError("Could not get the balance of an account in an inherited branch.");
			return false;
		}
		branchAccountDetailsKey[CB_BRANCH_ACCOUNT_DETAILS_BRANCH] = newBranch;
		if (NOT CBDatabaseWriteValue(database, branchAccountDetailsKey, data, 8)) {
			CBLogError("Could not write the balance of an account in a new branch.");
			return false;
		}
		// Iterate to the next output
		if (CBAssociativeArrayRangeIteratorNext(&database->index, &it))
			break;
	}
	// Loop through the unspent output information
	CBDecKey(accountUnspentOutputsMin, CB_TYPE_ACCOUNT_UNSPENT_OUTPUTS);
	CBDecKey(accountUnspentOutputsMax, CB_TYPE_ACCOUNT_UNSPENT_OUTPUTS);
	accountUnspentOutputsMin[CB_ACCOUNT_UNSPENT_OUTPUTS_BRANCH] = inherit;
	accountUnspentOutputsMax[CB_ACCOUNT_UNSPENT_OUTPUTS_BRANCH] = inherit;
	memset(accountUnspentOutputsMin + CB_ACCOUNT_UNSPENT_OUTPUTS_ACCOUNT_ID, 0, 16);
	memset(accountUnspentOutputsMax + CB_ACCOUNT_UNSPENT_OUTPUTS_ACCOUNT_ID, 0xFF, 16);
	if (CBAssociativeArrayRangeIteratorStart(&database->index, accountUnspentOutputsMin, accountUnspentOutputsMax, &it)) for (;;) {
		// Get the key
		uint8_t * key = CBRangeIteratorGetPointer(&it);
		// Write the entry for the new branch.
		key[CB_ACCOUNT_UNSPENT_OUTPUTS_BRANCH] = newBranch;
		if (NOT CBDatabaseWriteValue(database, key, NULL, 0)) {
			CBLogError("Could not write an unspent output entry for a new branch.");
			return false;
		}
		// Iterate to the next output
		if (CBAssociativeArrayRangeIteratorNext(&database->index, &it))
			break;
	}
	// Loop through branch account time transaction information
	CBDecKey(branchAccountTimeTxMin, CB_TYPE_BRANCH_ACCOUNT_TIME_TX);
	CBDecKey(branchAccountTimeTxMax, CB_TYPE_BRANCH_ACCOUNT_TIME_TX);
	branchAccountTimeTxMin[CB_BRANCH_ACCOUNT_TIME_TX_BRANCH] = inherit;
	branchAccountTimeTxMax[CB_BRANCH_ACCOUNT_TIME_TX_BRANCH] = inherit;
	memset(branchAccountTimeTxMin + CB_BRANCH_ACCOUNT_TIME_TX_ACCOUNT_ID, 0, cbKeySizes[CB_TYPE_BRANCH_ACCOUNT_TIME_TX] - 1);
	memset(branchAccountTimeTxMax + CB_BRANCH_ACCOUNT_TIME_TX_ACCOUNT_ID, 0xFF, cbKeySizes[CB_TYPE_BRANCH_ACCOUNT_TIME_TX] - 1);
	if (CBAssociativeArrayRangeIteratorStart(&database->index, branchAccountTimeTxMin, branchAccountTimeTxMax, &it)) for (;;) {
		// Get the key
		uint8_t * key = CBRangeIteratorGetPointer(&it);
		// Write the entry for the new branch.
		key[CB_BRANCH_ACCOUNT_TIME_TX_BRANCH] = newBranch;
		if (NOT CBDatabaseWriteValue(database, key, NULL, 0)) {
			CBLogError("Could not write a account transaction ordered by thime for a new branch.");
			return false;
		}
		// Iterate to the next key
		if (CBAssociativeArrayRangeIteratorNext(&database->index, &it))
			break;
	}
	return true;
}
bool CBAccounterRemoveTransactionFromBranch(CBDatabase * database, uint8_t * txDetailsKey, uint8_t * txHashToIDKey, uint8_t branch){
	// Get and lower the branch ownership number
	uint8_t branchOwners;
	if (NOT CBDatabaseReadValue(database, txDetailsKey, &branchOwners, 1, CB_TX_DETAILS_BRANCH_INSTANCES)) {
		CBLogError("Could not read the number of branches that own a transaction.");
		return false;
	}
	branchOwners--;
	if (NOT CBDatabaseWriteValueSubSection(database, txDetailsKey, &branchOwners, 1, CB_TX_DETAILS_BRANCH_INSTANCES)) {
		CBLogError("Could not write the deremented number of branches that own a transaction.");
		return false;
	}
	if (branchOwners == 0) {
		// No more owners, delete transaction information.
		if (NOT CBDatabaseRemoveValue(database, txDetailsKey)) {
			CBLogError("Could not remove transaction details.");
			return false;
		}
		if (NOT CBDatabaseRemoveValue(database, txHashToIDKey)) {
			CBLogError("Could not remove a transaction hash to ID entry.");
			return false;
		}
	}
	// If a branch, remove transaction branch data
	if (branch != CB_NO_BRANCH) {
		CBDecKey(branchTxDetailsKey, CB_TYPE_BRANCH_TX_DETAILS);
		branchTxDetailsKey[CB_BRANCH_TX_DETAILS_BRANCH] = branch;
		memcpy(branchTxDetailsKey + CB_BRANCH_TX_DETAILS_TX_ID, txDetailsKey + CB_TX_DETAILS_TX_ID, 8);
		// First get the height
		uint8_t data[4];
		if (NOT CBDatabaseReadValue(database, branchTxDetailsKey, data, 4, 0)) {
			CBLogError("Could not read the height of a transaction in a branch.");
			return false;
		}
		if (NOT CBDatabaseRemoveValue(database, branchTxDetailsKey)) {
			CBLogError("Couldn't remove the branch transaction details");
			return false;
		}
		CBDecKey(branchHeightAndIDKey, CB_TYPE_TX_HEIGHT_BRANCH_AND_ID);
		branchHeightAndIDKey[CB_TX_HEIGHT_BRANCH_AND_ID_BRANCH] = branch;
		memcpy(branchHeightAndIDKey + CB_TX_HEIGHT_BRANCH_AND_ID_HEIGHT, data, 4);
		memcpy(branchHeightAndIDKey + CB_TX_HEIGHT_BRANCH_AND_ID_TX_ID, txDetailsKey + CB_TX_DETAILS_TX_ID, 8);
		if (NOT CBDatabaseRemoveValue(database, branchHeightAndIDKey)) {
			CBLogError("Couldn't remove the branch height and transaction ID entry.");
			return false;
		}
	}
	// Loop through the transaction accounts
	CBDecKey(txAccountsMin, CB_TYPE_TX_ACCOUNTS);
	CBDecKey(txAccountsMax, CB_TYPE_TX_ACCOUNTS);
	memcpy(txAccountsMin + CB_TX_ACCOUNTS_TX_ID, txDetailsKey + CB_TX_DETAILS_TX_ID, 8);
	memcpy(txAccountsMax + CB_TX_ACCOUNTS_TX_ID, txDetailsKey + CB_TX_DETAILS_TX_ID, 8);
	memset(txAccountsMin + CB_TX_ACCOUNTS_ACCOUNT_ID, 0, 8);
	memset(txAccountsMax + CB_TX_ACCOUNTS_ACCOUNT_ID, 0xFF, 8);
	CBRangeIterator it;
	CBAssociativeArrayRangeIteratorStart(&database->index, txAccountsMin, txAccountsMax, &it);
	CBDecKey(accountTxDetailsKey, CB_TYPE_ACCOUNT_TX_DETAILS);
	memcpy(accountTxDetailsKey + CB_ACCOUNT_TX_DETAILS_TX_ID, txDetailsKey + CB_TX_DETAILS_TX_ID, 8);
	CBDecKey(branchAccountDetailsKey, CB_TYPE_BRANCH_ACCOUNT_DETAILS);
	for (;;) {
		// Get the specific key
		uint8_t * key = CBRangeIteratorGetPointer(&it);
		// Create the key for the account transaction and account branch details.
		memcpy(accountTxDetailsKey + CB_ACCOUNT_TX_DETAILS_ACCOUNT_ID, key + CB_TX_ACCOUNTS_ACCOUNT_ID, 8);
		branchAccountDetailsKey[CB_BRANCH_ACCOUNT_DETAILS_BRANCH] = branch;
		memcpy(branchAccountDetailsKey + CB_BRANCH_ACCOUNT_DETAILS_ACCOUNT_ID, key + 9, 8);
		// Adjust balance, lowing balance of branchless transactions.
		if (NOT CBAccounterAdjustAccountBalanceByTx(database, accountTxDetailsKey, branchAccountDetailsKey)) {
			CBLogError("Could not adjust the accounts balance for branchless transactions.");
			return false;
		}
		// Remove the branch account time transaction entry.
		CBDecKey(branchAccountTimeTxKey, CB_TYPE_BRANCH_ACCOUNT_TIME_TX);
		branchAccountTimeTxKey[CB_BRANCH_ACCOUNT_TIME_TX_BRANCH] = branch;
		memcpy(branchAccountTimeTxKey + CB_BRANCH_ACCOUNT_TIME_TX_ACCOUNT_ID, key + CB_TX_ACCOUNTS_ACCOUNT_ID, 8);
		memcpy(branchAccountTimeTxKey + CB_BRANCH_ACCOUNT_TIME_TX_TX_ID, txDetailsKey + CB_TX_DETAILS_TX_ID, 8);
		// Get the timestamp of the transaction
		if (NOT CBDatabaseReadValue(database, txDetailsKey, branchAccountTimeTxKey + CB_BRANCH_ACCOUNT_TIME_TX_TIMESTAMP, 8, CB_TX_DETAILS_TIMESTAMP)) {
			CBLogError("Could not read a transactions timestamp into an account's transactions for a branch ordered by time entry.");
			return false;
		}
		if (NOT CBDatabaseRemoveValue(database, branchAccountTimeTxKey)) {
			CBLogError("Could not remove an account transaction entry for a branch ordered by time.");
			return false;
		}
		// Remove non-branch-specific details if no more bramches own the transaction
		if (branchOwners == 0) {
			// Remove the transaction account entry
			if (NOT CBDatabaseRemoveValue(database, key)) {
				CBLogError("Could not remove a transaction account entry.");
				return false;
			}
			// Remove the account transaction details
			if (NOT CBDatabaseRemoveValue(database, accountTxDetailsKey)) {
				CBLogError("Could not remove account transaction details.");
				return false;
			}
		}
		if (CBAssociativeArrayRangeIteratorNext(&database->index, &it))
			break;
	}
	// Look for outputs we have found.
	// Do this by finding outputs via the transaction hash and then go through all the outputs for that hash, if any.
	CBDecKey(outputHashAndIndexToIDMin, CB_TYPE_OUTPUT_HASH_AND_INDEX_TO_ID);
	CBDecKey(outputHashAndIndexToIDMax, CB_TYPE_OUTPUT_HASH_AND_INDEX_TO_ID);
	// Add the transaction hash to the keys
	memcpy(outputHashAndIndexToIDMin + CB_OUTPUT_HASH_AND_INDEX_TO_ID_HASH, txHashToIDKey + CB_TX_HASH_TO_ID_HASH, 32);
	memcpy(outputHashAndIndexToIDMax + CB_OUTPUT_HASH_AND_INDEX_TO_ID_HASH, txHashToIDKey + CB_TX_HASH_TO_ID_HASH, 32);
	memset(outputHashAndIndexToIDMin + CB_OUTPUT_HASH_AND_INDEX_TO_ID_INDEX, 0, 4);
	memset(outputHashAndIndexToIDMax + CB_OUTPUT_HASH_AND_INDEX_TO_ID_INDEX, 0xFF, 4);
	// Loop through the range of potential outputs
	uint8_t data[8];
	if (CBAssociativeArrayRangeIteratorStart(&database->index, outputHashAndIndexToIDMin, outputHashAndIndexToIDMax, &it)) for (;;) {
		// Get the key for this output ID
		uint8_t * key = CBRangeIteratorGetPointer(&it);
		// Get the output ID
		if (NOT CBDatabaseReadValue(database, key, data, 8, 0)) {
			CBLogError("Could not read the ID of an output.");
			return false;
		}
		// Create the key for the output branch details
		CBDecKey(branchOutputDetailsKey, CB_TYPE_BRANCH_OUTPUT_DETAILS);
		branchOutputDetailsKey[CB_BRANCH_OUTPUT_DETAILS_BRANCH] = branch;
		memcpy(branchOutputDetailsKey + CB_BRANCH_OUTPUT_DETAILS_OUPUT_ID, data, 8);
		// Remove the output branch information.
		if (NOT CBDatabaseRemoveValue(database, branchOutputDetailsKey)) {
			CBLogError("Could not remove the output branch details.");
			return false;
		}
		// Loop through the output accounts
		CBDecKey(outputAccountsMin, CB_TYPE_OUTPUT_ACCOUNTS);
		CBDecKey(outputAccountsMax, CB_TYPE_OUTPUT_ACCOUNTS);
		memcpy(outputAccountsMin + CB_OUTPUT_ACCOUNTS_OUTPUT_ID, data, 8);
		memcpy(outputAccountsMax + CB_OUTPUT_ACCOUNTS_OUTPUT_ID, data, 8);
		memset(outputAccountsMin + CB_OUTPUT_ACCOUNTS_ACCOUNTS_ID, 0, 8);
		memset(outputAccountsMax + CB_OUTPUT_ACCOUNTS_ACCOUNTS_ID, 0xFF, 8);
		CBRangeIterator accountIt;
		CBAssociativeArrayRangeIteratorStart(&database->index, outputAccountsMin, outputAccountsMax, &accountIt);
		for (;;) {
			// Get the key for this account
			uint8_t * outputAccountKey = CBRangeIteratorGetPointer(&accountIt);
			// Remove the output as unspent from the account for this branch, if it exists as so.
			CBDecKey(accountUnspentOuputKey, CB_TYPE_ACCOUNT_UNSPENT_OUTPUTS);
			accountUnspentOuputKey[CB_ACCOUNT_UNSPENT_OUTPUTS_BRANCH] = branch;
			memcpy(accountUnspentOuputKey + CB_ACCOUNT_UNSPENT_OUTPUTS_ACCOUNT_ID, outputAccountKey + CB_OUTPUT_ACCOUNTS_ACCOUNTS_ID, 8);
			memcpy(accountUnspentOuputKey + CB_ACCOUNT_UNSPENT_OUTPUTS_OUTPUT_ID, data, 8);
			if (CBDatabaseGetLength(database, accountUnspentOuputKey) != CB_DOESNT_EXIST
				&& NOT CBDatabaseRemoveValue(database, accountUnspentOuputKey)) {
				CBLogError("Couldn't remove an unconfimed output as unspent for an account.");
				return false;
			}
			// If no more branches own the transaction, completely remove all output account information
			if (branchOwners == 0
				// Remove the output account entry
				&& NOT CBDatabaseRemoveValue(database, outputAccountKey)) {
				CBLogError("Couldn't remove an output account entry");
				return false;
			}
			if (CBAssociativeArrayRangeIteratorNext(&database->index, &accountIt))
				break;
		}
		// If no more branches own the transaction, completely remove all output information
		if (branchOwners == 0) {
			// Remove the output details
			CBDecKey(outputDetailsKey, CB_TYPE_OUTPUT_DETAILS);
			memcpy(outputDetailsKey + CB_OUTPUT_DETAILS_OUTPUT_ID, data, 8);
			if (NOT CBDatabaseRemoveValue(database, outputDetailsKey)) {
				CBLogError("Could not remove the output details.");
				return false;
			}
			// Remove the hash and index to ID entry.
			if (NOT CBDatabaseRemoveValue(database, key)) {
				CBLogError("Could not remove the output hash and index to ID entry.");
				return false;
			}
		}
		if (CBAssociativeArrayRangeIteratorNext(&database->index, &it))
			break;
	}
}