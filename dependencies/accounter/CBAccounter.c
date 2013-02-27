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

uint64_t CBNewAccounter(char * dataDir){
	CBAccounter * self = malloc(sizeof(*self));
	if (NOT self) {
		CBLogError("Could not create the accounter object.");
		return 0;
	}
	if (NOT CBInitDatabase(CBGetDatabase(self), dataDir, "acnt")) {
		CBLogError("Could not create a database object for an accounter.");
		free(self);
		return 0;
	}
	uint8_t data[24];
	CBDecKey(key, CB_TYPE_ACCOUNTER_DETAILS);
	if (CBDatabaseGetLength(CBGetDatabase(self), CB_TYPE_ACCOUNTER_DETAILS) == CB_DOESNT_EXIST) {
		// Write the initial data
		self->nextAccountID = 0;
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
		self->nextAccountID = CBArrayToInt64(data, CB_ACCOUNTER_DETAILS_ACCOUNT_ID);
		self->nextOutputRefID = CBArrayToInt64(data, CB_ACCOUNTER_DETAILS_OUTPUT_ID);
		self->nextTxID = CBArrayToInt64(data, CB_ACCOUNTER_DETAILS_TX_ID);
	}
	return (uint64_t) self;
}
void CBFreeAccounter(uint64_t iself){
	CBFreeDatabase((CBDatabase *)iself);
}
bool CBAccounterCommit(uint64_t self){
	return CBDatabaseCommit((CBDatabase *) self);
}
bool CBAccounterChangeOutputReferenceSpentStatus(CBDatabase * self, CBPrevOut * prevOut, uint8_t branch, bool spent, CBAssociativeArray * txInfo){
	// Get the key
	CBDecKey(hashAndIndexToIDKey, CB_TYPE_OUTPUT_HASH_AND_INDEX_TO_ID);
	CBDecKey(outputBranchDetailsKey, CB_TYPE_OUTPUT_BRANCH_DETAILS);
	memcpy(hashAndIndexToIDKey + 1, CBByteArrayGetData(prevOut->hash), 32);
	CBInt32ToArray(hashAndIndexToIDKey, 33, prevOut->index);
	if (CBDatabaseGetLength(self, hashAndIndexToIDKey) == CB_DOESNT_EXIST)
		// The ouptut reference does not exist, so we can end here.
		return true;
	if (NOT CBDatabaseReadValue(self, hashAndIndexToIDKey, outputBranchDetailsKey + 1, 8, 0)) {
		CBLogError("Could not read the index of an output reference.");
		return false;
	}
	outputBranchDetailsKey[9] = branch;
	// Save the data
	if (NOT CBDatabaseWriteValueSubSection(self, outputBranchDetailsKey, (uint8_t *)&spent, 1, CB_OUTPUT_REF_BRANCH_DATA_SPENT)){
		CBLogError("Could not write the new spent status for an output reference.");
		return false;
	}
	// For each account that owns the output remove or add from/to the unspent outputs.
	CBDecKey(unspentOutKey, CB_TYPE_ACCOUNT_UNSPENT_OUTPUTS);
	unspentOutKey[9] = branch;
	memcpy(unspentOutKey + 10, outputBranchDetailsKey + 1, 8);
	// Get the first account
	CBRangeIterator it;
	CBDecKey(minOutputAccountKey, CB_TYPE_OUTPUT_ACCOUNTS);
	CBDecKey(maxOutputAccountKey, CB_TYPE_OUTPUT_ACCOUNTS);
	memcpy(minOutputAccountKey + 1, outputBranchDetailsKey + 1, 8);
	CBInt64ToArray(minOutputAccountKey, 9, 0);
	memcpy(maxOutputAccountKey + 1, outputBranchDetailsKey + 1, 8);
	CBInt64ToArray(maxOutputAccountKey, 9, UINT32_MAX);
	CBAssociativeArrayRangeIteratorStart(&self->index, minOutputAccountKey, maxOutputAccountKey, &it);
	for (;;) {
		// Add account ID
		uint8_t * outputAccountKey = CBRangeIteratorGetPointer(&it);
		memcpy(unspentOutKey + 1, outputAccountKey + 9, 8);
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
			memcpy(outDetailsKey + 1, outputBranchDetailsKey + 1, 8);
			if (NOT CBDatabaseReadValue(self, outDetailsKey, data, 8, CB_OUTPUT_REF_DATA_VALUE)) {
				CBLogError("Could not read the value of an output.");
				return false;
			}
			uint64_t value = CBArrayToInt64(data, 0);
			// First look to see if the accountID exists
			uint32_t accountID = CBArrayToInt32(unspentOutKey, 1);
			CBFindResult res = CBAssociativeArrayFind(txInfo, &accountID);
			if (res.found) {
				// Add to the debit amount
				CBTransactionAccountCreditDebit * info = CBFindResultToPointer(res);
				info->debitAmount += value;
			}else{
				// Add new element
				CBTransactionAccountCreditDebit * info = malloc(sizeof(*info));
				if (NOT info) {
					CBLogError("Could not allocate memory for account credit/debit information for a transaction.");
					return false;
				}
				info->accountID = accountID;
				info->creditAmount = 0;
				info->debitAmount = value;
				info->foundCreditAddr = false;
			}
		}
	}
	return true;
}
bool CBAccounterAdjustAccountBalanceByTx(CBDatabase * database, uint8_t * accountTxDetails, uint8_t * accountDetailsKey){
	uint8_t data[8];
	if (NOT CBDatabaseReadValue(database, accountTxDetails, data, 8, CB_TX_ACCOUNT_DETAILS_VALUE)) {
		CBLogError("Could not read an accounts transaction value.");
		return false;
	}
	uint64_t txValueRaw = CBInt64ToArray(data, 0, 8);
	int64_t txValue = txValueRaw & 0x7FFFFFFFFFFFFFFF;
	if (NOT (txValueRaw & 0x8000000000000000)) {
		txValue = -txValue;
	}
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
bool CBAccounterFoundTransaction(uint32_t iself, void * vtx, uint32_t blockHeight, uint32_t time, uint8_t branch){
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
	CBDecKey(outputBranchDetailsKey, CB_TYPE_OUTPUT_BRANCH_DETAILS);
	CBDecKey(outputAccountsKey, CB_TYPE_OUTPUT_ACCOUNTS);
	CBDecKey(unspentOutputsKey, CB_TYPE_ACCOUNT_UNSPENT_OUTPUTS);
	CBDecKey(outputHashAndIndexToIDKey, CB_TYPE_OUTPUT_HASH_AND_INDEX_TO_ID);
	CBDecKey(accounterDetailsKey, CB_TYPE_ACCOUNTER_DETAILS);
	// Set the transaction hash into the hash and index to ID key.
	memcpy(outputHashAndIndexToIDKey + 1, CBTransactionGetHash(tx), 32);
	for (uint32_t x = 0; x < tx->outputNum; x++) {
		// Try to obtain an address (or multisig script hash) from this output
		// Get the identifying hash of the output of output
		uint8_t hash[32]; // 32 for mutlisig hashing
		if (CBAccounterGetOutputHash(tx->outputs[x], hash))
			continue;
		// Now look through accounts for this output.
		memcpy(minHashKey + 1, hash, 20);
		memcpy(maxHashKey + 1, hash, 20);
		CBInt64ToArray(minHashKey, 21, 0);
		CBInt64ToArray(maxHashKey, 21, UINT32_MAX);
		CBRangeIterator it;
		if (CBAssociativeArrayRangeIteratorStart(&database->index, minHashKey, maxHashKey, &it)){
			uint64_t outputID;
			uint8_t data[8];
			// Add the index ot the hash and index to ID key
			CBInt32ToArray(outputHashAndIndexToIDKey, 33, x);
			// Detemine if the output exists already
			if (CBDatabaseGetLength(database, outputHashAndIndexToIDKey) == CB_DOESNT_EXIST) {
				// Doesn't exist, get the next ID
				outputID = self->nextOutputRefID;
				// Save the output details
				CBInt64ToArray(outputDetailsKey, 1, outputID);
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
			CBInt64ToArray(outputBranchDetailsKey, 1, self->nextOutputRefID);
			outputBranchDetailsKey[9] = branch;
			data[0] = false;
			if (NOT CBDatabaseWriteValue(database, outputBranchDetailsKey, data, 1)) {
				CBLogError("Could not write the branch details for a new output reference.");
				return false;
			}
			// Set the output ID to the output accounts key.
			CBInt64ToArray(outputAccountsKey, 1, self->nextOutputRefID);
			// Set the output ID and branch to the unspent outputs key.
			unspentOutputsKey[9] = branch;
			CBInt64ToArray(unspentOutputsKey, 10, self->nextOutputRefID);
			for (;;) {
				// Found an account that is watching this output
				uint8_t * foundKey = CBRangeIteratorGetPointer(&it);
				uint32_t accountID = CBArrayToInt64(foundKey, 21);
				// Detemine if the output account details exist alredy or not.
				CBInt64ToArray(outputAccountsKey, 9, accountID);
				if (CBDatabaseGetLength(database, outputAccountsKey) == CB_DOESNT_EXIST) {
					// Add an entry to the output account information.
					if (NOT CBDatabaseWriteValue(database, outputAccountsKey, NULL, 0)) {
						CBLogError("Could not write an output account entry.");
						return false;
					}
				}
				// Add an entry to the unspent outputs for the account
				CBInt64ToArray(unspentOutputsKey, 1, accountID);
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
					if (NOT details) {
						CBLogError("Could not allocate memory for credit/debit information for an account's transaction.");
						return false;
					}
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
		memcpy(txHashToIDKey + 1, CBTransactionGetHash(tx), 32);
		uint64_t txID;
		uint8_t data[37];
		// Determine if the transaction exists already.
		if (CBDatabaseGetLength(database, txHashToIDKey) == CB_DOESNT_EXIST) { // ??? Use Read here and return CB_DOESNT_EXIST or similar?
			// It doesn't exist, get the next ID.
			txID = self->nextTxID;
			// Create the transaction details
			CBInt64ToArray(txDetailsKey, 1, txID);
			CBInt32ToArray(data, CB_TX_DETAILS_TIMESTAMP, time);
			data[CB_TX_DETAILS_BRANCH_INSTANCES] = 1;
			memcpy(data + CB_TX_DETAILS_HASH, CBTransactionGetHash(tx), 32);
			if (NOT CBDatabaseWriteValue(database, txDetailsKey, data, 37)) {
				CBLogError("Couldn't write the details for a new transaction for the accounter.");
				return false;
			}
			// Create the transaction hash to ID entry
			// Use txDetailsKey which has the ID
			if (NOT CBDatabaseWriteValue(database, txHashToIDKey, txDetailsKey + 1, 8)) {
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
			CBInt64ToArray(txDetailsKey, 1, txID);
			if (NOT CBDatabaseReadValue(database, txDetailsKey, data, 1, 1)) {
				CBLogError("Couldn't read the number of branches that own a transaction.");
				return false;
			}
			data[0]++;
			if (NOT CBDatabaseWriteValueSubSection(database, txDetailsKey, data, 1, CB_TX_DETAILS_BRANCH_INSTANCES)) {
				CBLogError("Couldn't write the incremented number of branches that own a transaction.");
				return false;
			}
		}
		if (blockHeight != CB_NO_BRANCH) {
			// Set the block height for the branch and this transaction
			CBDecKey(branchTXDetailsKey, CB_TYPE_BRANCH_TX_DETAILS);
			branchTXDetailsKey[1] = branch;
			CBInt64ToArray(branchTXDetailsKey, 2, txID);
			CBInt32ToArray(data, 0, blockHeight);
			if (NOT CBDatabaseWriteValue(database, branchTXDetailsKey, data, 4)) {
				CBLogError("Couldn't write the block height for a transaction on a branch.");
				return false;
			}
			// Create the branch and block height entry for the transaction
			CBDecKey(txHeightBranchIDKey, CB_TYPE_TX_HEIGHT_BRANCH_AND_ID);
			txHeightBranchIDKey[1] = branch;
			CBInt32ToArray(txHeightBranchIDKey, 2, blockHeight);
			CBInt64ToArray(txHeightBranchIDKey, 6, txID);
			if (NOT CBDatabaseWriteValue(database, txHeightBranchIDKey, NULL, 0)) {
				CBLogError("Couldn't write the branch and height to the transaction ID entry.");
				return false;
			}
		}
		CBDecKey(txAccountKey, CB_TYPE_TX_ACCOUNTS);
		CBInt64ToArray(txAccountKey, 1, txID);
		CBDecKey(accountTxDetailsKey, CB_TYPE_ACCOUNT_TX_DETAILS);
		for (;;) {
			// Get the transaction information for an account
			CBTransactionAccountCreditDebit * info = pos.node->elements[pos.index];
			// Add the account ID to the key
			CBInt64ToArray(txAccountKey, 9, info->accountID);
			// Set the account's transaction details key account
			CBInt64ToArray(accountTxDetailsKey, 1, info->accountID);
			// Determine if the transaction account details exist already.
			if (CBDatabaseGetLength(database, txAccountKey) == CB_DOESNT_EXIST) {
				// Create the transaction account entry
				if (NOT CBDatabaseWriteValue(database, txAccountKey, NULL, 0)) {
					CBLogError("Could not create a transaction account entry.");
					return false;
				}
				// Set the value
				int64_t valueInt = info->creditAmount - info->debitAmount;
				CBInt64ToArray(data, CB_TX_ACCOUNT_DETAILS_VALUE, labs(valueInt) | ((valueInt > 0) ? 0x8000000000000000 : 0));
				// Set the address.
				if (valueInt > 0)
					// Use credit address
					memcpy(data + CB_TX_ACCOUNT_DETAILS_ADDR, info->creditAddr, 20);
				else{
					// Find a debit address if there is one we can use.
					if (NOT info->foundCreditAddr || NOT info->creditAddrIndexIsZero) {
						// Try index 0.
						if (CBAccounterGetOutputHash(tx->outputs[0], data + CB_TX_ACCOUNT_DETAILS_ADDR))
							// The output is not supported, use zero to represent this.
							memset(data + CB_TX_ACCOUNT_DETAILS_ADDR, 0, 20);
					}else{
						// Else we have found a credit address and it is at the zero index
						if (tx->outputNum == 1)
							// No other outputs, must be wasting money to miners.
							memset(data + CB_TX_ACCOUNT_DETAILS_ADDR, 0, 20);
						// Try index 1
						else if (CBAccounterGetOutputHash(tx->outputs[1], data + CB_TX_ACCOUNT_DETAILS_ADDR))
							memset(data + CB_TX_ACCOUNT_DETAILS_ADDR, 0, 20);
					}
				}
				// Now save the account's transaction details
				if (NOT CBDatabaseWriteValue(database, accountTxDetailsKey, data, 28)) {
					CBLogError("Could not write the transaction details for an account.");
					return false;
				}
			}
			// Adjust the account's branch balance, increasing the balance for this transaction.
			CBDecKey(accountBranchDetailsKey, CB_TYPE_ACCOUNT_BRANCH_DETAILS);
			CBInt64ToArray(accountBranchDetailsKey, 1, info->accountID);
			accountBranchDetailsKey[9] = branch;
			if (NOT CBAccounterAdjustAccountBalanceByTx(database, accountTxDetailsKey, accountBranchDetailsKey)) {
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
bool CBAccounterGetOutputHash(CBTransactionOutput * output, uint8_t hash[32]){
	switch (CBTransactionOutputGetType(output)) {
		case CB_TX_OUTPUT_TYPE_KEYHASH:{
			// See if the public-key hash is one we are watching.
			uint32_t cursor = 2;
			CBScriptGetPushAmount(output->scriptObject, &cursor);
			memcpy(hash, CBByteArrayGetData(output->scriptObject) + cursor, 20);
			break;
		}
		case CB_TX_OUTPUT_TYPE_MULTISIG:
			// Hash the entire script and then see if we are watching that script.
			CBSha256(CBByteArrayGetData(output->scriptObject), output->scriptObject->length, hash);
			break;
		case CB_TX_OUTPUT_TYPE_P2SH:
			// See if the script hash is one we are watching.
			memcpy(hash, CBByteArrayGetData(output->scriptObject) + 2, 20);
			break;
		default:
			// Unsupported
			return true;
	}
	return false;
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
		branchTxDetailsKey[1] = branch;
		memcpy(branchTxDetailsKey + 2, txDetailsKey + 1, 8);
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
		branchHeightAndIDKey[1] = branch;
		memcpy(branchHeightAndIDKey + 2, data, 4);
		memcpy(branchHeightAndIDKey + 6, txDetailsKey + 1, 8);
		if (NOT CBDatabaseRemoveValue(database, branchHeightAndIDKey)) {
			CBLogError("Couldn't remove the branch height and transaction ID entry.");
			return false;
		}
	}
	// Loop through the transaction accounts
	CBDecKey(txAccountsMin, CB_TYPE_TX_ACCOUNTS);
	CBDecKey(txAccountsMax, CB_TYPE_TX_ACCOUNTS);
	memcpy(txAccountsMin + 1, txDetailsKey + 1, 8);
	memcpy(txAccountsMax + 1, txDetailsKey + 1, 8);
	memset(txAccountsMin + 9, 0, 8);
	memset(txAccountsMax + 9, 0xFF, 8);
	CBRangeIterator it;
	CBAssociativeArrayRangeIteratorStart(&database->index, txAccountsMin, txAccountsMax, &it);
	CBDecKey(accountTxDetailsKey, CB_TYPE_ACCOUNT_TX_DETAILS);
	CBDecKey(accountBranchDetailsKey, CB_TYPE_ACCOUNT_BRANCH_DETAILS);
	for (;;) {
		// Get the specific key
		uint8_t * key = CBRangeIteratorGetPointer(&it);
		// Create the key for the account transaction and account branch details.
		memcpy(accountTxDetailsKey + 1, key + 9, 8);
		memcpy(accountTxDetailsKey + 1, txDetailsKey + 1, 8);
		memcpy(accountBranchDetailsKey + 1, key + 9, 8);
		accountBranchDetailsKey[9] = branch;
		// Adjust balance, lowing balance of branchless transactions.
		if (NOT CBAccounterAdjustAccountBalanceByTx(database, accountTxDetailsKey, accountBranchDetailsKey)) {
			CBLogError("Could not adjust the accounts balance for branchless transactions.");
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
	memcpy(outputHashAndIndexToIDMin + 1, txHashToIDKey + 1, 32);
	memcpy(outputHashAndIndexToIDMax + 1, txHashToIDKey + 1, 32);
	memset(outputHashAndIndexToIDMin + 33, 0, 4);
	memset(outputHashAndIndexToIDMax + 33, 0xFF, 4);
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
		CBDecKey(outputBranchDetailsKey, CB_TYPE_OUTPUT_BRANCH_DETAILS);
		memcpy(outputBranchDetailsKey + 1, data, 8);
		outputBranchDetailsKey[9] = branch;
		// Remove the output branch information.
		if (NOT CBDatabaseRemoveValue(database, outputBranchDetailsKey)) {
			CBLogError("Could not remove the output branch details.");
			return false;
		}
		// Loop through the output accounts
		CBDecKey(outputAccountsMin, CB_TYPE_OUTPUT_ACCOUNTS);
		CBDecKey(outputAccountsMax, CB_TYPE_OUTPUT_ACCOUNTS);
		memcpy(outputAccountsMin + 1, data, 8);
		memcpy(outputAccountsMax + 1, data, 8);
		memset(outputAccountsMin + 9, 0, 8);
		memset(outputAccountsMax + 9, 0xFF, 8);
		CBRangeIterator accountIt;
		CBAssociativeArrayRangeIteratorStart(&database->index, outputAccountsMin, outputAccountsMax, &accountIt);
		for (;;) {
			// Get the key for this account
			uint8_t * outputAccountKey = CBRangeIteratorGetPointer(&accountIt);
			// Remove the output as unspent from the account for this branch, if it exists as so.
			CBDecKey(accountUnspentOuputKey, CB_TYPE_ACCOUNT_UNSPENT_OUTPUTS);
			memcpy(accountUnspentOuputKey + 1, outputAccountKey + 9, 8);
			accountUnspentOuputKey[9] = branch;
			CBInt64ToArray(accountUnspentOuputKey, 10, data);
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
			memcpy(outputDetailsKey + 1, data, 8);
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
	memcpy(txHashToIDKey, CBTransactionGetHash(tx), 32);
	if (NOT CBDatabaseReadValue(database, txHashToIDKey, data, 8, 0)) {
		CBLogError("Could not read a transaction ID for a hash.");
		return false;
	}
	uint64_t txID = CBArrayToInt64(data, 0);
	// Remove the transaction from the branch
	CBDecKey(txDetailsKey, CB_TYPE_TX_DETAILS);
	CBInt64ToArray(txDetailsKey, 1, txID);
	if (NOT CBAccounterRemoveTransactionFromBranch(database, txDetailsKey, txHashToIDKey, CB_NO_BRANCH)) {
		CBLogError("Could not remove a transaction as branchless.");
		return false;
	}
	return true;
}
bool CBAccounterBranchlessTransactionToBranch(uint64_t iself, void * vtx, uint32_t blockHeight, uint8_t branch){
	CBDatabase * database = (CBDatabase *)iself;
	CBTransaction * tx = vtx;
	uint8_t data[8];
	// A branchless transaction is found in a branch.
	// Loop through outputs and change references to the new branch
	CBDecKey(outputHashAndIndexToIDKey, CB_TYPE_OUTPUT_HASH_AND_INDEX_TO_ID);
	CBDecKey(outputBranchDetails, CB_TYPE_OUTPUT_BRANCH_DETAILS);
	CBDecKey(newOutputBranchDetails, CB_TYPE_OUTPUT_BRANCH_DETAILS);
	for (uint32_t x = 0; x < tx->outputNum; x++) {
		// Get the output ID of this output, if it exists.
		memcpy(outputHashAndIndexToIDKey + 1, CBTransactionGetHash(tx), 32);
		CBInt32ToArray(outputHashAndIndexToIDKey, 33, x);
		if (CBDatabaseGetLength(database, outputHashAndIndexToIDKey) != CB_DOESNT_EXIST) {
			// The output ID exists so get it
			if (NOT CBDatabaseReadValue(database, outputHashAndIndexToIDKey, data, 8, 0)) {
				CBLogError("Could not get the ID of a branchless output so we can move the spent status to a branch");
				return false;
			}
			uint64_t outputID = CBArrayToInt64(data, 0);
			// Create key for the output branch information.
			memcpy(outputBranchDetails + 1, data, 8);
			outputBranchDetails[9] = CB_NO_BRANCH;
			// Check the spent status of this output
			if (NOT CBDatabaseReadValue(database, outputBranchDetails, data, 1, CB_OUTPUT_REF_BRANCH_DATA_SPENT)) {
				CBLogError("Could not get the spent status of a branchless output.");
				return false;
			}
			if (NOT data[0]) {
				// Unspent
				// Loop through the output's accounts and change the unspent output references to the branch the output is being added to.
				CBDecKey(outputAccountsMin, CB_TYPE_OUTPUT_ACCOUNTS);
				CBDecKey(outputAccountsMax, CB_TYPE_OUTPUT_ACCOUNTS);
				CBInt64ToArray(outputAccountsMin, 1, outputID);
				CBInt64ToArray(outputAccountsMax, 1, outputID);
				memset(outputAccountsMin + 9, 0, 8);
				memset(outputAccountsMax + 9, 0xFF, 8);
				CBRangeIterator it;
				CBAssociativeArrayRangeIteratorStart(&database->index, outputAccountsMin, outputAccountsMax, &it);
				CBDecKey(accountUnspentOutputKey, CB_TYPE_ACCOUNT_UNSPENT_OUTPUTS);
				CBDecKey(newAccountUnspentOutputKey, CB_TYPE_ACCOUNT_UNSPENT_OUTPUTS);
				for (;;) {
					// Get the account key
					uint8_t * key = CBRangeIteratorGetPointer(&it);
					// Make the unspent output key
					memcpy(accountUnspentOutputKey + 1, key + 9, 8);
					accountUnspentOutputKey[9] = CB_NO_BRANCH;
					CBInt64ToArray(accountUnspentOutputKey, 10, outputID);
					// Make the new key for the branch
					memcpy(newAccountUnspentOutputKey + 1, key + 9, 8);
					newAccountUnspentOutputKey[9] = branch;
					CBInt64ToArray(newAccountUnspentOutputKey, 10, outputID);
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
			memcpy(newOutputBranchDetails + 1, data, 8);
			newOutputBranchDetails[9] = branch;
			// Change the key
			if (NOT CBDatabaseChangeKey(database, outputBranchDetails, newOutputBranchDetails)) {
				CBLogError("Could not change the key of a branchless output's branch information to include the branch it is changed to.");
				return false;
			}
		}
	}
	// Get the transaction ID
	CBDecKey(txHashToIDKey, CB_TYPE_TX_HASH_TO_ID);
	memcpy(txHashToIDKey + 1, CBTransactionGetHash(tx), 32);
	if (NOT CBDatabaseReadValue(database, txHashToIDKey, data, 8, 0)) {
		CBLogError("Could not get a transaction ID for adding to a branch.");
		return false;
	}
	uint64_t txID = CBArrayToInt64(data, 8);
	// Add branch transaction details
	CBDecKey(branchTxDetailsKey, CB_TYPE_BRANCH_TX_DETAILS);
	branchTxDetailsKey[1] = branch;
	CBInt64ToArray(branchTxDetailsKey, 2, txID);
	CBInt32ToArray(data, CB_TX_BRANCH_DETAILS_BLOCK_HEIGHT, blockHeight);
	if (NOT CBDatabaseWriteValue(database, branchTxDetailsKey, data, 4)) {
		CBLogError("Could not write the branch transaction details for a branchless transaction added to a branch.");
		return false;
	}
	// Add branch, height and ID entry.
	CBDecKey(branchHeightAndIDKey, CB_TYPE_TX_HEIGHT_BRANCH_AND_ID);
	branchHeightAndIDKey[1] = branch;
	CBInt32ToArray(branchHeightAndIDKey, 5, blockHeight);
	CBInt64ToArray(branchHeightAndIDKey, 9, txID);
	if (NOT CBDatabaseWriteValue(database, branchHeightAndIDKey, NULL, 0)) {
		CBLogError("Could not create a branch height and transaction ID entry for a branchless transaction added to a branch.");
		return false;
	}
	// Loop through transaction accounts
	CBDecKey(txAccountMin, CB_TYPE_TX_ACCOUNTS);
	CBDecKey(txAccountMax, CB_TYPE_TX_ACCOUNTS);
	CBInt64ToArray(txAccountMin, 1, txID);
	CBInt64ToArray(txAccountMax, 1, txID);
	memset(txAccountMin + 9, 0, 8);
	memset(txAccountMax + 9, 0xFF, 8);
	CBRangeIterator it;
	CBDecKey(accountDetailsKey, CB_TYPE_ACCOUNT_BRANCH_DETAILS);
	CBDecKey(accountTxDetails, CB_TYPE_ACCOUNT_TX_DETAILS);
	CBAssociativeArrayRangeIteratorStart(&database->index, txAccountMin, txAccountMax, &it);
	for (;;) {
		// Get the transaction account key
		uint8_t * key = CBRangeIteratorGetPointer(&it);
		// Get the account Tx key and account details key.
		memcpy(accountTxDetails + 1, key + 9, 8);
		memcpy(accountTxDetails + 9, key + 1, 8);
		memcpy(accountDetailsKey + 1, key + 9, 8);
		accountDetailsKey[9] = CB_NO_BRANCH;
		// Adjust balance of branchless transactions
		if (NOT CBAccounterAdjustAccountBalanceByTx(database, accountTxDetails, accountDetailsKey)) {
			CBLogError("Could not adjust the accounts balance for branchless transactions.");
			return false;
		}
		// Adjust balance of branch we are adding to
		accountDetailsKey[9] = branch;
		if (NOT CBAccounterAdjustAccountBalanceByTx(database, accountTxDetails, accountDetailsKey)) {
			CBLogError("Could not adjust the accounts balance for branchless transactions.");
			return false;
		}
		// Iterate to the next transaction account if there is any.
		if (CBAssociativeArrayRangeIteratorNext(&database->index, *it))
			break;
	}
	return true;
}
bool CBAccounterDeleteBranch(uint64_t iself, uint8_t branch){
	CBDatabase * database = (CBDatabase *)iself;
	uint8_t data[32];
	// Loop through transactions
	CBDecKey(txBranchHeightAndIDMin, CB_TYPE_TX_HEIGHT_BRANCH_AND_ID);
	CBDecKey(txBranchHeightAndIDMax, CB_TYPE_TX_HEIGHT_BRANCH_AND_ID);
	txBranchHeightAndIDMin[1] = branch;
	txBranchHeightAndIDMax[1] = branch;
	memset(txBranchHeightAndIDMin + 2, 0xFF, 4);
	memset(txBranchHeightAndIDMax + 2, 0xFF, 4);
	memset(txBranchHeightAndIDMin + 6, 0, 8);
	memset(txBranchHeightAndIDMax + 6, 0xFF, 8);
	CBRangeIterator it;
	if (CBAssociativeArrayRangeIteratorStart(&database->index, txBranchHeightAndIDMin, txBranchHeightAndIDMax, &it)) for (;;) {
		// Get the key
		uint8_t * key = CBRangeIteratorGetPointer(&it);
		// Remove the transaction from the branch
		CBDecKey(txDetailsKey, CB_TYPE_TX_DETAILS);
		memcpy(txDetailsKey + 1, key + 6, 8);
		CBDecKey(txHashToIDKey, CB_TYPE_TX_HASH_TO_ID);
		// Get the transaction hash
		if (NOT CBDatabaseReadValue(database, txDetailsKey, data, 32, CB_TX_DETAILS_HASH)) {
			CBLogError("Could not get a transaction hash details.");
			return false;
		}
		// Add hash to the hash to ID key.
		memcpy(txHashToIDKey + 1, data, 32);
		if (NOT CBAccounterRemoveTransactionFromBranch(database, txDetailsKey, txHashToIDKey, branch)) {
			CBLogError("Could not remove a transaction from a branch.");
			return false;
		}
		// Iterate to next transaction
		if (CBAssociativeArrayRangeIteratorNext(&database->index, &it))
			break;
	}
}
bool CBAccounterNewBranch(uint64_t iself, uint8_t newBranch, uint8_t inherit){
	CBDatabase * database = (CBDatabase *)iself;
	// Make new branch identical to inherited branch
	// Loop through transactions
	CBDecKey(branchTxDetailsMin, CB_TYPE_BRANCH_TX_DETAILS);
	CBDecKey(branchTxDetailsMax, CB_TYPE_BRANCH_TX_DETAILS);
	branchTxDetailsMin[1] = inherit;
	branchTxDetailsMax[1] = inherit;
	memset(branchTxDetailsMin + 2, 0, 8);
	memset(branchTxDetailsMax + 2, 0xFF, 8);
	CBRangeIterator it;
	if (CBAssociativeArrayRangeIteratorStart(&database->index, branchTxDetailsMin, branchTxDetailsMax, &it)) for (;;) {
		// Get the key
		uint8_t * key = CBRangeIteratorGetPointer(&it);
		
		// Iterate to the next transaction
		if (CBAssociativeArrayRangeIteratorNext(&database->index, &it)) {
			<#statements#>
		}
	}
}
CBCompare CBCompareUInt32(void * vint1, void * vint2){
	uint32_t int1 = vint1;
	uint32_t int2 = vint2;
	if (int1 > int2)
		return CB_COMPARE_MORE_THAN;
	if (int1 < int2)
		return CB_COMPARE_LESS_THAN;
	return CB_COMPARE_EQUAL;
}
