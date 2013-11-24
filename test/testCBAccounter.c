//
//  testCBAccounter.c
//  cbitcoin
//
//  Created by Matthew Mitchell on 16/02/2013.
//  Copyright (c) 2012 Matthew Mitchell
//
//  This file is part of cbitcoin. It is subject to the license terms
//  in the LICENSE file found in the top-level directory of this
//  distribution and at http://www.cbitcoin.com/license.html. No part of
//  cbitcoin, including this file, may be copied, modified, propagated,
//  or distributed except according to the terms contained in the
//  LICENSE file.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "CBDependencies.h"
#include "CBTransaction.h"
#include <time.h>
#include <sys/time.h>

void CBLogError(char * b, ...);
void CBLogError(char * b, ...){
	printf("FAIL -> %s\n", b);
	exit(1);
}

uint64_t CBGetMilliseconds(void){
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

typedef struct{
	uint8_t * addr;
	uint32_t height;
	int64_t amount;
	uint64_t timestamp;
	uint8_t * txHash;
} CBTestTxDetails;

void checkTransactions(CBDepObject cursor, CBTestTxDetails * txTestDetails, uint8_t numTxs);
void checkTransactions(CBDepObject cursor, CBTestTxDetails * txTestDetails, uint8_t numTxs){
	CBTransactionDetails txDetails;
	for (uint8_t x = 0; x < numTxs; x++) {
		if (CBAccounterGetNextTransaction(cursor, &txDetails) != CB_TRUE) {
			printf("TX %u FAIL\n", x);
			exit(EXIT_FAILURE);
		}
		if (memcmp(txDetails.accountTxDetails.addrHash, txTestDetails[x].addr, 20)) {
			printf("TX %u ADDR HASH FAIL\n", x);
			exit(EXIT_FAILURE);
		}
		if (txDetails.height !=  txTestDetails[x].height) {
			printf("TX %u HEIGHT FAIL\n", x);
			exit(EXIT_FAILURE);
		}
		if (txDetails.accountTxDetails.amount != txTestDetails[x].amount) {
			printf("TX %u AMOUNT FAIL\n", x);
			exit(EXIT_FAILURE);
		}
		if (txDetails.timestamp !=  txTestDetails[x].timestamp){
			printf("TX %u TIMESTAMP FAIL\n", x);
			exit(EXIT_FAILURE);
		}
		if (memcmp(txDetails.txHash,  txTestDetails[x].txHash, 32)){
			printf("TX %u TX HASH FAIL\n", x);
			exit(EXIT_FAILURE);
		}
	}
	if (CBAccounterGetNextTransaction(cursor, &txDetails) != CB_FALSE) {
		printf("TX END FAIL\n");
		exit(EXIT_FAILURE);
	}
	CBFreeAccounterStorageTransactionCursor(cursor);
}

typedef struct{
	uint64_t value;
	uint8_t * txHash;
	uint32_t index;
	uint8_t * watchHash;
} CBTestUOutDetails;

void checkUnspentOutputs(CBDepObject cursor, CBTestUOutDetails * outDetails, uint8_t num);
void checkUnspentOutputs(CBDepObject cursor, CBTestUOutDetails * outDetails, uint8_t num){
	CBUnspentOutputDetails uoDetails;
	for (uint8_t x = 0; x < num; x++) {
		if (CBAccounterGetNextUnspentOutput(cursor, &uoDetails) != CB_TRUE) {
			printf("GET UNSPENT OUTPUT %u FAIL\n", x);
			exit(EXIT_FAILURE);
		}
		if (uoDetails.value != outDetails[x].value) {
			printf("UNSPENT OUTPUT %u VALUE FAIL\n", x);
			exit(EXIT_FAILURE);
		}
		if (memcmp(uoDetails.txHash, outDetails[x].txHash, 32)) {
			printf("UNSPENT OUTPUT %u TX FAIL\n", x);
			exit(EXIT_FAILURE);
		}
		if (uoDetails.index != outDetails[x].index) {
			printf("UNSPENT OUTPUT %u INDEX FAIL\n", x);
			exit(EXIT_FAILURE);
		}
		if (memcmp(uoDetails.watchedHash, outDetails[x].watchHash, 20)) {
			printf("UNSPENT OUTPUT %u ID HASH FAIL\n", x);
			exit(EXIT_FAILURE);
		}
	}
	if (CBAccounterGetNextUnspentOutput(cursor, &uoDetails) != CB_FALSE) {
		printf("UNSPENT OUTPUT END FAIL\n");
		exit(EXIT_FAILURE);
	}
	CBFreeAccounterStorageUnspentOutputCursor(cursor);
}

void checkBalance(CBDepObject storage, uint8_t branch, uint64_t account, int64_t exp);
void checkBalance(CBDepObject storage, uint8_t branch, uint64_t account, int64_t exp){
	int64_t balance;
	if (! CBAccounterGetBranchAccountBalance(storage, branch, account, &balance)){
		printf("GET BALANCE FAIL\n");
		exit(EXIT_FAILURE);
	}
	if (balance != exp) {
		printf("BALANCE NUM FAIL\n");
		exit(EXIT_FAILURE);
	}
}

int main(){
	// ??? For all tests, should change return statuses to EXIT_SUCCESS and EXIT_FAILURE
	remove("./cbitcoin/log.dat");
	remove("./cbitcoin/del.dat");
	remove("./cbitcoin/idx_8_0.dat");
	remove("./cbitcoin/idx_9_0.dat");
	remove("./cbitcoin/idx_10_0.dat");
	remove("./cbitcoin/idx_11_0.dat");
	remove("./cbitcoin/idx_12_0.dat");
	remove("./cbitcoin/idx_13_0.dat");
	remove("./cbitcoin/idx_14_0.dat");
	remove("./cbitcoin/idx_15_0.dat");
	remove("./cbitcoin/idx_16_0.dat");
	remove("./cbitcoin/idx_17_0.dat");
	remove("./cbitcoin/idx_18_0.dat");
	remove("./cbitcoin/idx_19_0.dat");
	remove("./cbitcoin/idx_20_0.dat");
	remove("./cbitcoin/idx_21_0.dat");
	remove("./cbitcoin/idx_22_0.dat");
	remove("./cbitcoin/val_0.dat");
	CBDepObject storage;
	CBDepObject database;
	CBNewStorageDatabase(&database, "./", 10000000, 10000000);
	if (! CBNewAccounterStorage(&storage, database)){
		printf("NEW ACCOUNTER STORAGE FAIL\n");
		return 1;
	}
	// Get an account
	uint64_t account1 = CBAccounterNewAccount(storage);
	// Add a watched hash
	uint8_t hash1[20];
	memset(hash1, 1, 20);
	if (! CBAccounterAddWatchedOutputToAccount(storage, hash1, account1)) {
		printf("ADD WATCHED OUTPUT FAIL\n");
		return 1;
	}
	// Create output for first watched output
	CBScript * outScript = CBNewScriptOfSize(25);
	CBByteArraySetByte(outScript, 0, CB_SCRIPT_OP_DUP);
	CBByteArraySetByte(outScript, 1, CB_SCRIPT_OP_HASH160);
	CBByteArraySetByte(outScript, 2, 0x14);
	memset(CBByteArrayGetData(outScript) + 3, 1, 20);
	CBByteArraySetByte(outScript, 23, CB_SCRIPT_OP_EQUALVERIFY);
	CBByteArraySetByte(outScript, 24, CB_SCRIPT_OP_CHECKSIG);
	// Create a transaction with two said watched hash and an output without it.
	CBTransaction * tx1 = CBNewTransaction(0, 1);
	CBScript * inScript = CBNewScriptWithDataCopy((uint8_t []){CB_SCRIPT_OP_TRUE}, 1);
	CBByteArray * prevOutHash = CBNewScriptOfSize(32);
	memset(CBByteArrayGetData(prevOutHash), 0, 32);
	CBTransactionTakeInput(tx1, CBNewTransactionInput(inScript, CB_TX_INPUT_FINAL, prevOutHash, 0));
	CBTransactionTakeOutput(tx1, CBNewTransactionOutput(100, outScript));
	CBTransactionTakeOutput(tx1, CBNewTransactionOutput(300, outScript));
	CBTransactionTakeOutput(tx1, CBNewTransactionOutputTakeScript(200, CBNewScriptWithDataCopy((uint8_t []){CB_SCRIPT_OP_TRUE}, 1)));
	// Serialise transaction
	CBGetMessage(tx1)->bytes = CBNewByteArrayOfSize(CBTransactionCalculateLength(tx1));
	if (CBTransactionSerialise(tx1, false) != CBTransactionCalculateLength(tx1)){
		printf("TX1 SERAILISE FAIL\n");
		return 1;
	}
	// Find the transaction unconf
	CBTransactionAccountDetailList * details;
	if (! CBAccounterFoundTransaction(storage, tx1, 0, 1000, CB_NO_BRANCH, &details)) {
		printf("FOUND TX FAIL\n");
		return 1;
	}
	// Check details
	if (details->accountID != account1) {
		printf("TX1 ACCOUNT 1 DETAILS ACCOUNT ID FAIL\n");
		return 1;
	}
	if (memcmp(details->accountTxDetails.addrHash, hash1, 20)) {
		printf("TX1 ACCOUNT 1 DETAILS ADDR FAIL\n");
		return 1;
	}
	if (details->accountTxDetails.amount != 400) {
		printf("TX1 ACCOUNT 1 DETAILS AMOUNT FAIL\n");
		return 1;
	}
	if (details->next != NULL) {
		printf("TX1 DIRECT END FAIL\n");
		return 1;
	}
	// Free details
	CBFreeTransactionAccountDetailList(details);
	// Check obtaining the transaction details
	CBDepObject cursor;
	if (! CBNewAccounterStorageTransactionCursor(&cursor, storage, CB_NO_BRANCH, account1, 1000, 1000)) {
		printf("CREATE CURSOR FAIL\n");
		return 1;
	}
	printf("CHECKING FOUND TX1\n");
	checkTransactions(cursor, (CBTestTxDetails [1]){
		{hash1,0,400,1000,CBTransactionGetHash(tx1)}
	}, 1);
	// Check obtaining the unconf balance
	checkBalance(storage, CB_NO_BRANCH, account1, 400);
	// Check obtaining the unspent outputs
	if (! CBNewAccounterStorageUnspentOutputCursor(&cursor, storage, CB_NO_BRANCH, account1)) {
		printf("CREATE UNSPENT OUTPUT CURSOR FAIL\n");
		return 1;
	}
	checkUnspentOutputs(cursor, (CBTestUOutDetails [2]){
		{100, CBTransactionGetHash(tx1), 0, hash1},
		{300, CBTransactionGetHash(tx1), 1, hash1}
	}, 2);
	// Create first branch
	if (! CBAccounterNewBranch(storage, 0, CB_NO_PARENT, 0)) {
		printf("CREATE FIRST BRANCH FAIL\n");
		return 1;
	}
	// Move transaction into branch
	if (! CBAccounterUnconfirmedTransactionToBranch(storage, tx1, 1000, 0)) {
		printf("MOVE TX TO BRANCH FAIL\n");
		return 1;
	}
	printf("CHECKING TX1 MOVE TO BRANCH\n");
	// Check balances
	checkBalance(storage, CB_NO_BRANCH, account1, 0);
	checkBalance(storage, 0, account1, 400);
	// Check we can't get transaction as UNCONF
	CBNewAccounterStorageTransactionCursor(&cursor, storage, CB_NO_BRANCH, account1, 1000, 1000);
	printf("CHECKING TRANSACTIONS NOT AS UNCONF\n");
	checkTransactions(cursor, NULL, 0);
	// Check we can get transaction on branch
	printf("CHECKING TRANSACTIONS ON BRANCH\n");
	CBNewAccounterStorageTransactionCursor(&cursor, storage, 0, account1, 1000, 1000);
	checkTransactions(cursor, (CBTestTxDetails [1]){
		{hash1,1000,400,1000,CBTransactionGetHash(tx1)}
	}, 1);
	// Check cannot get unspent output as unconfirmed
	printf("CHECKING UNSPENT OUTPUTS NOT AS UNCONF\n");
	CBNewAccounterStorageUnspentOutputCursor(&cursor, storage, CB_NO_BRANCH, account1);
	checkUnspentOutputs(cursor, NULL, 0);
	// Check unspent output in branch 0
	printf("CHECKING UNSPENT OUTPUTS ON BRANCH\n");
	CBNewAccounterStorageUnspentOutputCursor(&cursor, storage, 0, account1);
	checkUnspentOutputs(cursor, (CBTestUOutDetails [2]){
		{100, CBTransactionGetHash(tx1), 0, hash1},
		{300, CBTransactionGetHash(tx1), 1, hash1}
	}, 2);
	// Make new watched output
	uint8_t hash2[20];
	memset(hash2, 2, 20);
	if (! CBAccounterAddWatchedOutputToAccount(storage, hash2, account1)) {
		printf("ADD 2ND WATCHED OUTPUT FAIL\n");
		return 1;
	}
	CBScript * outScript2 = CBNewScriptOfSize(25);
	CBByteArraySetByte(outScript2, 0, CB_SCRIPT_OP_DUP);
	CBByteArraySetByte(outScript2, 1, CB_SCRIPT_OP_HASH160);
	CBByteArraySetByte(outScript2, 2, 0x14);
	memset(CBByteArrayGetData(outScript2) + 3, 2, 20);
	CBByteArraySetByte(outScript2, 23, CB_SCRIPT_OP_EQUALVERIFY);
	CBByteArraySetByte(outScript2, 24, CB_SCRIPT_OP_CHECKSIG);
	// Make next transaction
	CBTransaction * tx2 = CBNewTransaction(0, 1);
	CBByteArray * prevOutTx1 = CBNewScriptOfSize(32);
	memcpy(CBByteArrayGetData(prevOutTx1), CBTransactionGetHash(tx1), 32);
	CBTransactionTakeInput(tx2, CBNewTransactionInput(inScript, CB_TX_INPUT_FINAL, prevOutTx1, 0)); // -100
	// Not owned by us
	CBTransactionTakeInput(tx2, CBNewTransactionInput(inScript, CB_TX_INPUT_FINAL, prevOutTx1, 2));
	CBTransactionTakeOutput(tx2, CBNewTransactionOutput(50, outScript)); // +50
	CBTransactionTakeOutput(tx2, CBNewTransactionOutput(100, outScript2)); // +100
	// Serialise transaction
	CBGetMessage(tx2)->bytes = CBNewByteArrayOfSize(CBTransactionCalculateLength(tx2));
	if (CBTransactionSerialise(tx2, false) != CBTransactionCalculateLength(tx2)){
		printf("TX2 SERAILISE FAIL\n");
		return 1;
	}
	// Find transaction on unconfirmed
	if (! CBAccounterFoundTransaction(storage, tx2, 0, 2000, CB_NO_BRANCH, &details)) {
		printf("FOUND TX2 FAIL\n");
		return 1;
	}
	printf("CHECKING FOUND TX2\n");
	// Check details
	if (details->accountID != account1) {
		printf("TX2 ACCOUNT 1 DETAILS ACCOUNT ID FAIL\n");
		return 1;
	}
	if (memcmp(details->accountTxDetails.addrHash, hash1, 20)) {
		printf("TX2 ACCOUNT 1 DETAILS ADDR FAIL\n");
		return 1;
	}
	if (details->accountTxDetails.amount != 50) {
		printf("TX2 ACCOUNT 1 DETAILS AMOUNT FAIL\n");
		return 1;
	}
	if (details->next != NULL) {
		printf("TX2 END FAIL\n");
		return 1;
	}
	// Free details
	CBFreeTransactionAccountDetailList(details);
	// Check unconfirmed balance
	checkBalance(storage, CB_NO_BRANCH, account1, 50);
	printf("CHECKING BRANCH 0 OUTPUTS\n");
	// Check unspent outputs for branch 0
	CBNewAccounterStorageUnspentOutputCursor(&cursor, storage, 0, account1);
	checkUnspentOutputs(cursor, (CBTestUOutDetails [1]){
		{300, CBTransactionGetHash(tx1), 1, hash1}
	}, 1);
	// Check unspent outputs for unconfirmed
	printf("CHECKING UNCONF OUTPUTS\n");
	CBNewAccounterStorageUnspentOutputCursor(&cursor, storage, CB_NO_BRANCH, account1);
	checkUnspentOutputs(cursor, (CBTestUOutDetails [2]){
		{50, CBTransactionGetHash(tx2), 0, hash1},
		{100, CBTransactionGetHash(tx2), 1, hash2},
	}, 2);
	// Test loosing unconfirmed transaction
	printf("CHECKING LOST TX2\n");
	if (! CBAccounterLostUnconfirmedTransaction(storage, tx2)) {
		printf("LOOSE UNCONF TX FAIL\n");
		return 1;
	}
	// Check balances
	printf("CHECKING UNCONF BALANCE\n");
	checkBalance(storage, CB_NO_BRANCH, account1, 0);
	printf("CHECKING BRANCH 0 BALANCE\n");
	checkBalance(storage, 0, account1, 400);
	// Check we can't get transaction as unconfirmed
	printf("CHECKING UNCONF TRANSACTIONS\n");
	CBNewAccounterStorageTransactionCursor(&cursor, storage, CB_NO_BRANCH, account1, 1000, 2000);
	checkTransactions(cursor, NULL, 0);
	// Check we can get transaction on branch
	printf("CHECKING BRANCH 0 TRANSACTIONS\n");
	CBNewAccounterStorageTransactionCursor(&cursor, storage, 0, account1, 1000, 2000);
	checkTransactions(cursor, (CBTestTxDetails [1]){
		{hash1,1000,400,1000,CBTransactionGetHash(tx1)}
	}, 1);
	// Check cannot get unspent output as unconfirmed
	printf("CHECKING UNCONF OUTPUTS\n");
	CBNewAccounterStorageUnspentOutputCursor(&cursor, storage, CB_NO_BRANCH, account1);
	checkUnspentOutputs(cursor, NULL, 0);
	// Check unspent output in branch 0
	printf("CHECKING BRANCH 0 OUTPUTS\n");
	CBNewAccounterStorageUnspentOutputCursor(&cursor, storage, 0, account1);
	checkUnspentOutputs(cursor, (CBTestUOutDetails [2]){
		{100, CBTransactionGetHash(tx1), 0, hash1},
		{300, CBTransactionGetHash(tx1), 1, hash1},
	}, 2);
	// Add directly to branch
	printf("CHECKING DIRECT TX2 OUTPUTS\n");
	if (! CBAccounterFoundTransaction(storage, tx2, 2000, 3000, 0, &details)) {
		CBLogError("DIRECT ADD TO BRANCH FAIL\n");
		return 1;
	}
	// Check details
	if (details->accountID != account1) {
		printf("TX2 DIRECT ACCOUNT 1 DETAILS ACCOUNT ID FAIL\n");
		return 1;
	}
	if (memcmp(details->accountTxDetails.addrHash, hash1, 20)) {
		printf("TX2 DIRECT ACCOUNT 1 DETAILS ADDR FAIL\n");
		return 1;
	}
	if (details->accountTxDetails.amount != 50) {
		printf("TX2 DIRECT ACCOUNT 1 DETAILS AMOUNT FAIL\n");
		return 1;
	}
	if (details->next != NULL) {
		printf("TX2 DIRECT END FAIL\n");
		return 1;
	}
	// Free details
	CBFreeTransactionAccountDetailList(details);
	// Check balances
	printf("CHECKING UNCONF BALANCE\n");
	checkBalance(storage, CB_NO_BRANCH, account1, 0);
	printf("CHECKING BRANCH 0 BALANCE\n");
	checkBalance(storage, 0, account1, 450);
	// Check transactions
	CBNewAccounterStorageTransactionCursor(&cursor, storage, 0, account1, 1000, 3000);
	checkTransactions(cursor, (CBTestTxDetails [2]){
		{hash1,1000,400,1000,CBTransactionGetHash(tx1)},
		{hash1,2000,50,3000,CBTransactionGetHash(tx2)}
	}, 2);
	// Check unspent outputs for branch 0
	CBNewAccounterStorageUnspentOutputCursor(&cursor, storage, 0, account1);
	checkUnspentOutputs(cursor, (CBTestUOutDetails [3]){
		{50, CBTransactionGetHash(tx2), 0, hash1},
		{100, CBTransactionGetHash(tx2), 1, hash2},
		{300, CBTransactionGetHash(tx1), 1, hash1},
	}, 3);
	// Test making UNCONF transaction, then adding another transaction with the same timestamp to the branch and then finally adding the UNCONF transaction. Test also negative UNCONF balance.
	// Make next transaction
	CBTransaction * tx3 = CBNewTransaction(0, 1);
	CBTransactionTakeInput(tx3, CBNewTransactionInput(inScript, CB_TX_INPUT_FINAL, prevOutTx1, 1)); // -300
	CBTransactionTakeOutput(tx3, CBNewTransactionOutput(100, outScript)); // +100
	// Serialise transaction
	CBGetMessage(tx3)->bytes = CBNewByteArrayOfSize(CBTransactionCalculateLength(tx3));
	if (CBTransactionSerialise(tx3, false) != CBTransactionCalculateLength(tx3)){
		printf("TX3 SERAILISE FAIL\n");
		return 1;
	}
	// Find transaction on UNCONF
	if (! CBAccounterFoundTransaction(storage, tx3, 0, 4000, CB_NO_BRANCH, &details)) {
		printf("FOUND TX3 FAIL\n");
		return 1;
	}
	// Check details
	if (details->accountID != account1) {
		printf("TX3 ACCOUNT 1 DETAILS ACCOUNT ID FAIL\n");
		return 1;
	}
	if (memcmp(details->accountTxDetails.addrHash, CBByteArrayGetData(prevOutHash), 20)) {
		printf("TX3 ACCOUNT 1 DETAILS ADDR FAIL\n");
		return 1;
	}
	if (details->accountTxDetails.amount != -200) {
		printf("TX3 ACCOUNT 1 DETAILS AMOUNT FAIL\n");
		return 1;
	}
	if (details->next != NULL) {
		printf("TX3 END FAIL\n");
		return 1;
	}
	// Free details
	CBFreeTransactionAccountDetailList(details);
	printf("CHECK TX3 NEGATIVE BLANCE\n");
	// Check negative balance
	checkBalance(storage, CB_NO_BRANCH, account1, -200);
	// Add next transaction directly to branch
	CBTransaction * tx4 = CBNewTransaction(0, 1);
	CBByteArray * prevOutTx2 = CBNewScriptOfSize(32);
	memcpy(CBByteArrayGetData(prevOutTx2), CBTransactionGetHash(tx2), 32);
	CBTransactionTakeInput(tx4, CBNewTransactionInput(inScript, CB_TX_INPUT_FINAL, prevOutTx2, 0)); // -50
	CBTransactionTakeInput(tx4, CBNewTransactionInput(inScript, CB_TX_INPUT_FINAL, prevOutTx2, 1)); // -100
	CBTransactionTakeOutput(tx4, CBNewTransactionOutput(100, outScript2)); // +100
	// Serialise transaction
	CBGetMessage(tx4)->bytes = CBNewByteArrayOfSize(CBTransactionCalculateLength(tx4));
	if (CBTransactionSerialise(tx4, false) != CBTransactionCalculateLength(tx4)){
		printf("TX4 SERAILISE FAIL\n");
		return 1;
	}
	if (! CBAccounterFoundTransaction(storage, tx4, 4000, 4000, 0, &details)) {
		printf("FOUND TX4 FAIL\n");
		return 1;
	}
	// Check details
	if (details->accountID != account1) {
		printf("TX4 ACCOUNT 1 DETAILS ACCOUNT ID FAIL\n");
		return 1;
	}
	if (memcmp(details->accountTxDetails.addrHash, CBByteArrayGetData(prevOutHash), 20)) {
		printf("TX4 ACCOUNT 1 DETAILS ADDR FAIL\n");
		return 1;
	}
	if (details->accountTxDetails.amount != -50) {
		printf("TX4 ACCOUNT 1 DETAILS AMOUNT FAIL\n");
		return 1;
	}
	if (details->next != NULL) {
		printf("TX4 END FAIL\n");
		return 1;
	}
	// Free details
	CBFreeTransactionAccountDetailList(details);
	// Move over tx3
	if (! CBAccounterUnconfirmedTransactionToBranch(storage, tx3, 4000, 0)) {
		printf("MOVE TX3 INTO BRANCH 0\n");
		return 1;
	}
	printf("CHECK DIRECT TX4 AND MOVE TX3\n");
	// Now check balances
	printf("CHECK UNCONF BALANCE\n");
	checkBalance(storage, CB_NO_BRANCH, account1, 0);
	printf("CHECK BRANCH 0 BALANCE\n");
	checkBalance(storage, 0, account1, 200);
	// Check transactions
	CBNewAccounterStorageTransactionCursor(&cursor, storage, 0, account1, 1000, 4000);
	checkTransactions(cursor, (CBTestTxDetails [4]){
		{hash1,1000,400,1000,CBTransactionGetHash(tx1)}, // +400
		{hash1,2000,50,3000,CBTransactionGetHash(tx2)}, // +50
		// This should be tx4 followed by tx3
		{CBByteArrayGetData(prevOutHash),4000,-50,4000,CBTransactionGetHash(tx4)}, // -50
		{CBByteArrayGetData(prevOutHash),4000,-200,4000,CBTransactionGetHash(tx3)}, // -200
	}, 4);
	// Check outputs
	printf("CHECKING BRANCH 0 OUTPUS\n");
	CBNewAccounterStorageUnspentOutputCursor(&cursor, storage, 0, account1);
	checkUnspentOutputs(cursor, (CBTestUOutDetails [2]){
		{100, CBTransactionGetHash(tx3), 0, hash1},
		{100, CBTransactionGetHash(tx4), 0, hash2},
	}, 2);
	// Check that no unspent outputs can be gotten from CB_NO_BRANCH
	CBNewAccounterStorageUnspentOutputCursor(&cursor, storage, CB_NO_BRANCH, account1);
	printf("CHECKING UNCONF OUTPUS\n");
	checkUnspentOutputs(cursor, NULL, 0);
	// Create new account with new watched hash
	uint64_t account2 = CBAccounterNewAccount(storage);
	uint8_t hash3[20];
	memset(hash3, 3, 20);
	if (! CBAccounterAddWatchedOutputToAccount(storage, hash3, account2)) {
		printf("ADD WATCHED OUTPUT FOR SECOND ACCOUNT FAIL\n");
		return 1;
	}
	// Make output script for new watched hash
	CBScript * outScript3 = CBNewScriptOfSize(25);
	CBByteArraySetByte(outScript3, 0, CB_SCRIPT_OP_DUP);
	CBByteArraySetByte(outScript3, 1, CB_SCRIPT_OP_HASH160);
	CBByteArraySetByte(outScript3, 2, 0x14);
	memset(CBByteArrayGetData(outScript3) + 3, 3, 20);
	CBByteArraySetByte(outScript3, 23, CB_SCRIPT_OP_EQUALVERIFY);
	CBByteArraySetByte(outScript3, 24, CB_SCRIPT_OP_CHECKSIG);
	// Create transaction moving funds from account 1 to account 2
	CBTransaction * tx5 = CBNewTransaction(0, 1);
	CBByteArray * prevOutTx3 = CBNewScriptOfSize(32);
	memcpy(CBByteArrayGetData(prevOutTx3), CBTransactionGetHash(tx3), 32);
	CBTransactionTakeInput(tx5, CBNewTransactionInput(inScript, CB_TX_INPUT_FINAL, prevOutTx3, 0));
	CBTransactionTakeOutput(tx5, CBNewTransactionOutput(100, outScript3));
	// Serialise transaction
	CBGetMessage(tx5)->bytes = CBNewByteArrayOfSize(CBTransactionCalculateLength(tx5));
	if (CBTransactionSerialise(tx5, false) != CBTransactionCalculateLength(tx5)){
		printf("TX5 SERAILISE FAIL\n");
		return 1;
	}
	// Test adding to UNCONF
	printf("CHECKING FOUND TX5\n");
	if (! CBAccounterFoundTransaction(storage, tx5, 0, 5000, CB_NO_BRANCH, &details)) {
		printf("FOUND TX5 FAIL\n");
		return 1;
	}
	// Check details
	if (details->accountID != account1) {
		printf("TX5 ACCOUNT 1 DETAILS ACCOUNT ID FAIL\n");
		return 1;
	}
	if (memcmp(details->accountTxDetails.addrHash, hash3, 20)) {
		printf("TX5 ACCOUNT 1 DETAILS ADDR FAIL\n");
		return 1;
	}
	if (details->accountTxDetails.amount != -100) {
		printf("TX5 ACCOUNT 1 DETAILS AMOUNT FAIL\n");
		return 1;
	}
	details = details->next;
	if (details->accountID != account2) {
		printf("TX5 ACCOUNT 2 DETAILS ACCOUNT ID FAIL\n");
		return 1;
	}
	if (memcmp(details->accountTxDetails.addrHash, hash3, 20)) {
		printf("TX5 ACCOUNT 2 DETAILS ADDR FAIL\n");
		return 1;
	}
	if (details->accountTxDetails.amount != 100) {
		printf("TX5 ACCOUNT 2 DETAILS AMOUNT FAIL\n");
		return 1;
	}
	if (details->next != NULL) {
		printf("TX5 END FAIL\n");
		return 1;
	}
	// Free details
	CBFreeTransactionAccountDetailList(details);
	// Check unconf balance for account 1
	printf("CHECKING UNCONF BALANACE ACCOUNT 1\n");
	checkBalance(storage, CB_NO_BRANCH, account1, -100);
	// Check unconf balance for account 2
	printf("CHECKING UNCONF BALANACE ACCOUNT 2\n");
	checkBalance(storage, CB_NO_BRANCH, account2, 100);
	// Check unconf transaction for account 1
	printf("CHECKING UNCONF TRANSACTIONS ACCOUNT 1\n");
	CBNewAccounterStorageTransactionCursor(&cursor, storage, CB_NO_BRANCH, account1, 1000, 5000);
	checkTransactions(cursor, (CBTestTxDetails [1]){
		{hash3,0,-100,5000,CBTransactionGetHash(tx5)}
	}, 1);
	// Check unconf transaction for account 2
	printf("CHECKING UNCONF TRANSACTIONS ACCOUNT 2\n");
	CBNewAccounterStorageTransactionCursor(&cursor, storage, CB_NO_BRANCH, account2, 1000, 5000);
	checkTransactions(cursor, (CBTestTxDetails [1]){
		{hash3,0,100,5000,CBTransactionGetHash(tx5)}
	}, 1);
	printf("CHECKING BRANCH 0 TRANSACTIONS ACCOUNT 2\n");
	// Check cannot get any on branch 0 for account 2
	CBNewAccounterStorageTransactionCursor(&cursor, storage, 0, account2, 1000, 5000);
	checkTransactions(cursor, NULL, 0);
	printf("CHECKING UNCONF OUTPUTS ACCOUNT 1\n");
	// Check cannot get any unconf unspent outputs for account 1
	CBNewAccounterStorageUnspentOutputCursor(&cursor, storage, CB_NO_BRANCH, account1);
	checkUnspentOutputs(cursor, NULL, 0);
	// Check unspent output for account 2
	printf("CHECKING UNCONF OUTPUTS ACCOUNT 2\n");
	CBNewAccounterStorageUnspentOutputCursor(&cursor, storage, CB_NO_BRANCH, account2);
	checkUnspentOutputs(cursor, (CBTestUOutDetails [1]){
		{100, CBTransactionGetHash(tx5), 0, hash3}
	}, 1);
	// Now test adding to branch
	printf("CHECKING TX5 TO BRANCH\n");
	if (! CBAccounterUnconfirmedTransactionToBranch(storage, tx5, 5000, 0)) {
		CBLogError("TX5 TO BRANCH 0 FAIL\n");
		return 1;
	}
	// Check balances for account 1
	printf("CHECKING UNCONF BALANCE ACCOUNT 1\n");
	checkBalance(storage, CB_NO_BRANCH, account1, 0);
	printf("CHECKING BRANCH 0 BALANCE ACCOUNT 1\n");
	checkBalance(storage, 0, account1, 100);
	// Check balances for account 2
	printf("CHECKING UNCONF BALANCE ACCOUNT 2\n");
	checkBalance(storage, CB_NO_BRANCH, account2, 0);
	printf("CHECKING BRANCH 0 BALANCE ACCOUNT 2\n");
	checkBalance(storage, 0, account2, 100);
	// Check getting only last transaction
	printf("CHECKING BRANCH 0 LAST TRANSACTION ACCOUNT 1\n");
	CBNewAccounterStorageTransactionCursor(&cursor, storage, 0, account1, 5000, 5000);
	checkTransactions(cursor, (CBTestTxDetails [1]){
		{hash3,5000,-100,5000,CBTransactionGetHash(tx5)}
	}, 1);
	// Check getting tx for account 2
	printf("CHECKING BRANCH 0 LAST TRANSACTION ACCOUNT 2\n");
	CBNewAccounterStorageTransactionCursor(&cursor, storage, 0, account2, 5000, 5000);
	checkTransactions(cursor, (CBTestTxDetails [1]){
		{hash3,5000,100,5000,CBTransactionGetHash(tx5)}
	}, 1);
	// Check getting unspent output for account 1
	printf("CHECKING BRANCH 0 OUTPUTS ACCOUNT 1\n");
	CBNewAccounterStorageUnspentOutputCursor(&cursor, storage, 0, account1);
	checkUnspentOutputs(cursor, (CBTestUOutDetails [1]){
		{100, CBTransactionGetHash(tx4), 0, hash2}
	}, 1);
	// Check unspent output for account 2
	printf("CHECKING BRANCH 0 OUTPUTS ACCOUNT 2\n");
	CBNewAccounterStorageUnspentOutputCursor(&cursor, storage, 0, account2);
	checkUnspentOutputs(cursor, (CBTestUOutDetails [1]){
		{100, CBTransactionGetHash(tx5), 0, hash3}
	}, 1);
	// Test creating new branch fork at height 2000 leaving only the first transaction in new branch
	printf("CHECKING NEW BRANCH FORK AT 2000\n");
	if (! CBAccounterNewBranch(storage, 1, 0, 2000)) {
		printf("NEW BRANCH 1 FAIL\n");
		return 1;
	}
	// Test balance for branch 0 for account 1
	printf("CHECKING BRANCH 0 BALANCE ACCOUNT 1\n");
	checkBalance(storage, 0, account1, 100);
	// Test balance for branch 0 for account 2
	printf("CHECKING BRANCH 0 BALANCE ACCOUNT 2\n");
	checkBalance(storage, 0, account1, 100);
	// Test balance for branch 1 for account 1
	printf("CHECKING BRANCH 1 BALANCE ACCOUNT 1\n");
	checkBalance(storage, 1, account1, 400);
	// Test transactions remain intact in branch 0 for account 1
	printf("CHECKING BRANCH 0 TRANSACTIONS ACCOUNT 1\n");
	CBNewAccounterStorageTransactionCursor(&cursor, storage, 0, account1, 1000, 5000);
	checkTransactions(cursor, (CBTestTxDetails [5]){
		{hash1,1000,400,1000,CBTransactionGetHash(tx1)},
		{hash1,2000,50,3000,CBTransactionGetHash(tx2)},
		{CBByteArrayGetData(prevOutHash),4000,-50,4000,CBTransactionGetHash(tx4)},
		{CBByteArrayGetData(prevOutHash),4000,-200,4000,CBTransactionGetHash(tx3)},
		{hash3,5000,-100,5000,CBTransactionGetHash(tx5)},
	}, 5);
	// Test transactions remain intact in branch 0 for account 2
	printf("CHECKING BRANCH 0 TRANSACTIONS ACCOUNT 2\n");
	CBNewAccounterStorageTransactionCursor(&cursor, storage, 0, account2, 1000, 5000);
	checkTransactions(cursor, (CBTestTxDetails [1]){
		{hash3,5000,100,5000,CBTransactionGetHash(tx5)},
	}, 1);
	// Test unspent output remain intact in branch 0 for account 1
	printf("CHECKING BRANCH 0 OUTPUTS ACCOUNT 1\n");
	CBNewAccounterStorageUnspentOutputCursor(&cursor, storage, 0, account1);
	checkUnspentOutputs(cursor, (CBTestUOutDetails [1]){
		{100, CBTransactionGetHash(tx4), 0, hash2}
	}, 1);
	// Test unspent outputs remain intact in branch 0 for account 2
	printf("CHECKING BRANCH 0 OUTPUTS ACCOUNT 2\n");
	CBNewAccounterStorageUnspentOutputCursor(&cursor, storage, 0, account2);
	checkUnspentOutputs(cursor, (CBTestUOutDetails [1]){
		{100, CBTransactionGetHash(tx5), 0, hash3}
	}, 1);
	// Test account 1 transaction in new branch.
	printf("CHECKING BRANCH 1 TRANSACTIONS ACCOUNT 1\n");
	CBNewAccounterStorageTransactionCursor(&cursor, storage, 1, account1, 1000, 5000);
	checkTransactions(cursor, (CBTestTxDetails [1]){
		{hash1,1000,400,1000,CBTransactionGetHash(tx1)},
	}, 1);
	// Test account 1 unspent outputs in new branch.
	printf("CHECKING BRANCH 1 OUTPUTS ACCOUNT 1\n");
	CBNewAccounterStorageUnspentOutputCursor(&cursor, storage, 1, account1);
	checkUnspentOutputs(cursor, (CBTestUOutDetails [2]){
		{100, CBTransactionGetHash(tx1), 0, hash1},
		{300, CBTransactionGetHash(tx1), 1, hash1},
	}, 2);
	// Test adding two transactions as unconf
	CBTransaction * tx6 = CBNewTransaction(0, 1);
	CBTransactionTakeInput(tx6, CBNewTransactionInput(inScript, CB_TX_INPUT_FINAL, prevOutHash, 0));
	CBTransactionTakeOutput(tx6, CBNewTransactionOutput(300, outScript3));
	// Serialise transaction
	CBGetMessage(tx6)->bytes = CBNewByteArrayOfSize(CBTransactionCalculateLength(tx6));
	if (CBTransactionSerialise(tx6, false) != CBTransactionCalculateLength(tx6)){
		printf("TX6 SERAILISE FAIL\n");
		return 1;
	}
	// Find tx as unconf
	printf("CHECKING TX6 FOUND\n");
	if (! CBAccounterFoundTransaction(storage, tx6, 0, 6000, CB_NO_BRANCH, &details)) {
		printf("FOUND TX6 FAIL\n");
		return 1;
	}
	// Check details
	if (details->accountID != account2) {
		printf("TX6 ACCOUNT 2 DETAILS ACCOUNT ID FAIL\n");
		return 1;
	}
	if (memcmp(details->accountTxDetails.addrHash, hash3, 20)) {
		printf("TX6 ACCOUNT 2 DETAILS ADDR FAIL\n");
		return 1;
	}
	if (details->accountTxDetails.amount != 300) {
		printf("TX6 ACCOUNT 2 DETAILS AMOUNT FAIL\n");
		return 1;
	}
	if (details->next != NULL) {
		printf("TX6 END FAIL\n");
		return 1;
	}
	// Free details
	CBFreeTransactionAccountDetailList(details);
	// Check unconf balance for account 2
	printf("CHECKING UNCONF BALANCE ACCOUNT 2\n");
	checkBalance(storage, CB_NO_BRANCH, account2, 300);
	// Second tx
	CBTransaction * tx7 = CBNewTransaction(0, 1);
	CBByteArray * prevOutTx6 = CBNewScriptOfSize(32);
	memcpy(CBByteArrayGetData(prevOutTx6), CBTransactionGetHash(tx6), 32);
	CBTransactionTakeInput(tx7, CBNewTransactionInput(inScript, CB_TX_INPUT_FINAL, prevOutTx1, 0));
	CBTransactionTakeInput(tx7, CBNewTransactionInput(inScript, CB_TX_INPUT_FINAL, prevOutTx1, 1));
	CBTransactionTakeInput(tx7, CBNewTransactionInput(inScript, CB_TX_INPUT_FINAL, prevOutTx6, 0));
	CBTransactionTakeOutput(tx7, CBNewTransactionOutput(500, outScript));
	// Serialise transaction
	CBGetMessage(tx7)->bytes = CBNewByteArrayOfSize(CBTransactionCalculateLength(tx7));
	if (CBTransactionSerialise(tx7, false) != CBTransactionCalculateLength(tx7)){
		printf("TX7 SERAILISE FAIL\n");
		return 1;
	}
	// Find tx as unconf
	printf("CHECKING TX7 FOUND\n");
	if (! CBAccounterFoundTransaction(storage, tx7, 0, 7000, CB_NO_BRANCH, &details)) {
		printf("FOUND TX7 FAIL\n");
		return 1;
	}
	// Check details
	if (details->accountID != account1) {
		printf("TX7 ACCOUNT 1 DETAILS ACCOUNT ID FAIL\n");
		return 1;
	}
	if (memcmp(details->accountTxDetails.addrHash, hash1, 20)) {
		printf("TX7 ACCOUNT 1 DETAILS ADDR FAIL\n");
		return 1;
	}
	if (details->accountTxDetails.amount != 100) {
		printf("TX7 ACCOUNT 1 DETAILS AMOUNT FAIL\n");
		return 1;
	}
	details = details->next;
	if (details->accountID != account2) {
		printf("TX7 ACCOUNT 2 DETAILS ACCOUNT ID FAIL\n");
		return 1;
	}
	if (memcmp(details->accountTxDetails.addrHash, hash1, 20)) {
		printf("TX7 ACCOUNT 2 DETAILS ADDR FAIL\n");
		return 1;
	}
	if (details->accountTxDetails.amount != -300) {
		printf("TX7 ACCOUNT 2 DETAILS AMOUNT FAIL\n");
		return 1;
	}
	if (details->next != NULL) {
		printf("TX7 END FAIL\n");
		return 1;
	}
	// Free details
	CBFreeTransactionAccountDetailList(details);
	// Check unconf balance for account 1
	printf("CHECKING UNCONF BALANCE ACCOUNT 1\n");
	checkBalance(storage, CB_NO_BRANCH, account1, 100);
	// Check unconf balance for account 2
	printf("CHECKING UNCONF BALANCE ACCOUNT 2\n");
	checkBalance(storage, CB_NO_BRANCH, account2, 0);
	// Check getting transaction for account 1
	printf("CHECKING UNCONF TRANSACTIONS ACCOUNT 1\n");
	CBNewAccounterStorageTransactionCursor(&cursor, storage, CB_NO_BRANCH, account1, 1000, 7000);
	checkTransactions(cursor, (CBTestTxDetails [1]){
		{hash1,0,100,7000,CBTransactionGetHash(tx7)},
	}, 1);
	// Check getting transaction for account 2
	printf("CHECKING UNCONF TRANSACTIONS ACCOUNT 2\n");
	CBNewAccounterStorageTransactionCursor(&cursor, storage, CB_NO_BRANCH, account2, 1000, 7000);
	checkTransactions(cursor, (CBTestTxDetails [2]){
		{hash3,0,300,6000,CBTransactionGetHash(tx6)},
		{hash1,0,-300,7000,CBTransactionGetHash(tx7)},
	}, 2);
	// Check getting unspent output
	printf("CHECKING UNCONF OUTPUTS ACCOUNT 1\n");
	CBNewAccounterStorageUnspentOutputCursor(&cursor, storage, CB_NO_BRANCH, account1);
	checkUnspentOutputs(cursor, (CBTestUOutDetails [1]){
		{500, CBTransactionGetHash(tx7), 0, hash1},
	}, 1);
	// Test adding the two transactions to branch 1
	printf("CHECKING TX6 AND TX7 TO BRANCH 1\n");
	if (! CBAccounterUnconfirmedTransactionToBranch(storage, tx6, 6000, 1)) {
		printf("TX6 TO BRANCH 1 FAIL\n");
		return 1;
	}
	if (! CBAccounterUnconfirmedTransactionToBranch(storage, tx7, 7000, 1)) {
		printf("TX7 TO BRANCH 1 FAIL\n");
		return 1;
	}
	for (uint8_t x = 0; x < 2; x++) {
		// Check balances
		printf("CHECKING UNCONF BALANCE ACCOUNT 1\n");
		checkBalance(storage, CB_NO_BRANCH, account1, 0);
		// Check unconf balance for account 2
		printf("CHECKING UNCONF BALANCE ACCOUNT 2\n");
		checkBalance(storage, CB_NO_BRANCH, account2, 0);
		// Check branch 1 balance for account 1
		printf("CHECKING BRANCH 1 BALANCE ACCOUNT 1\n");
		checkBalance(storage, 1, account1, 500);
		// Check branch 1 balance for account 2
		printf("CHECKING BRANCH 1 BALANCE ACCOUNT 2\n");
		checkBalance(storage, 1, account2, 0);
		// Check getting transactions for account 1
		printf("CHECKING BRANCH 1 TRANSACTIONS ACCOUNT 1\n");
		CBNewAccounterStorageTransactionCursor(&cursor, storage, 1, account1, 1000, 7000);
		checkTransactions(cursor, (CBTestTxDetails [2]){
			{hash1,1000,400,1000,CBTransactionGetHash(tx1)},
			{hash1,7000,100,7000,CBTransactionGetHash(tx7)},
		}, 2);
		// Check getting transactions for account 2
		printf("CHECKING BRANCH 1 TRANSACTIONS ACCOUNT 2\n");
		CBNewAccounterStorageTransactionCursor(&cursor, storage, 1, account2, 1000, 7000);
		checkTransactions(cursor, (CBTestTxDetails [2]){
			{hash3,6000,300,6000,CBTransactionGetHash(tx6)},
			{hash1,7000,-300,7000,CBTransactionGetHash(tx7)},
		}, 2);
		// Test getting unspent output
		printf("CHECKING BRANCH 1 OUTPUTS ACCOUNT 1\n");
		CBNewAccounterStorageUnspentOutputCursor(&cursor, storage, 1, account1);
		checkUnspentOutputs(cursor, (CBTestUOutDetails [1]){
			{500, CBTransactionGetHash(tx7), 0, hash1},
		}, 1);
		if (x == 0) {
			// Test deleting branch 0
			printf("CHECKING DELETE BRANCH 0\n");
			if (! CBAccounterDeleteBranch(storage, 0)) {
				printf("DELETE BRANCH 0 FAIL\n");
				return 1;
			}
		}
		// Now check branch 1 again in loop
	}
	// Test duplicate tx6 transaction (these can exist)
	printf("CHECKING DUPLICATE TX6\n");
	if (! CBAccounterFoundTransaction(storage, tx6, 8000, 0, 1, &details)) {
		printf("FOUND DUPLICATE TX FAIL\n");
		return 1;
	}
	// Check details
	if (details->accountID != account2) {
		printf("TX6 DUPLICATE ACCOUNT 2 DETAILS ACCOUNT ID FAIL\n");
		return 1;
	}
	if (memcmp(details->accountTxDetails.addrHash, hash3, 20)) {
		printf("TX6 DUPLICATE DIRECT ACCOUNT 2 DETAILS ADDR FAIL\n");
		return 1;
	}
	if (details->accountTxDetails.amount != 300) {
		printf("TX6 DUPLICATE DIRECT ACCOUNT 2 DETAILS AMOUNT FAIL\n");
		return 1;
	}
	if (details->next != NULL) {
		printf("TX6 DUPLICATE END FAIL\n");
		return 1;
	}
	// Free details
	CBFreeTransactionAccountDetailList(details);
	// Check balances
	printf("CHECKING UNCONF BALANCE ACCOUNT 1\n");
	checkBalance(storage, CB_NO_BRANCH, account1, 0);
	// Check unconf balance for account 2
	printf("CHECKING UNCONF BALANCE ACCOUNT 2\n");
	checkBalance(storage, CB_NO_BRANCH, account2, 0);
	// Check branch 1 balance for account 1
	printf("CHECKING BRANCH 1 BALANCE ACCOUNT 1\n");
	checkBalance(storage, 1, account1, 500);
	// Check branch 1 balance for account 2
	printf("CHECKING BRANCH 1 BALANCE ACCOUNT 2\n");
	checkBalance(storage, 1, account2, 300);
	// Check getting transactions for account 1
	printf("CHECKING BRANCH 1 TRANSACTIONS ACCOUNT 1\n");
	CBNewAccounterStorageTransactionCursor(&cursor, storage, 1, account1, 1000, 8000);
	checkTransactions(cursor, (CBTestTxDetails [2]){
		{hash1,1000,400,1000,CBTransactionGetHash(tx1)},
		{hash1,7000,100,7000,CBTransactionGetHash(tx7)},
	}, 2);
	// Check getting transactions for account 2
	printf("CHECKING BRANCH 1 TRANSACTIONS ACCOUNT 2\n");
	CBNewAccounterStorageTransactionCursor(&cursor, storage, 1, account2, 1000, 8000);
	checkTransactions(cursor, (CBTestTxDetails [3]){
		{hash3,6000,300,6000,CBTransactionGetHash(tx6)}, // +300
		{hash3,8000,300,6000,CBTransactionGetHash(tx6)}, // +300
		{hash1,7000,-300,7000,CBTransactionGetHash(tx7)}, // -300
	}, 3);
	// Test getting unspent outputs
	printf("CHECKING BRANCH 1 OUTPUTS ACCOUNT 1\n");
	CBNewAccounterStorageUnspentOutputCursor(&cursor, storage, 1, account1);
	checkUnspentOutputs(cursor, (CBTestUOutDetails [1]){
		{500, CBTransactionGetHash(tx7), 0, hash1},
	}, 1);
	printf("CHECKING BRANCH 1 OUTPUTS ACCOUNT 2\n");
	CBNewAccounterStorageUnspentOutputCursor(&cursor, storage, 1, account2);
	checkUnspentOutputs(cursor, (CBTestUOutDetails [1]){
		{300, CBTransactionGetHash(tx6), 0, hash3},
	}, 1);
	// Test new branch seperating tx7 and second tx6 and check first tx6
	printf("CHECKING NEW BRANCH SEPERATING TX7 AND SECOND TX6\n");
	if (! CBAccounterNewBranch(storage, 0, 1, 7000)) {
		printf("NEW BRANCH TO FIRST TX6\n");
		return 1;
	}
	// Check new branch balance
	printf("CHECKING BRANCH 0 BALANCE ACCOUNT 1\n");
	checkBalance(storage, 0, account1, 400);
	printf("CHECKING BRANCH 0 BALANCE ACCOUNT 2\n");
	checkBalance(storage, 0, account2, 300);
	// Check can get duplicate and old tx6 in branch 1
	printf("CHECKING BRANCH 1 TX6S ACCOUNT 2\n");
	CBNewAccounterStorageTransactionCursor(&cursor, storage, 1, account2, 6000, 6000);
	checkTransactions(cursor, (CBTestTxDetails [2]){
		{hash3,6000,300,6000,CBTransactionGetHash(tx6)},
		{hash3,8000,300,6000,CBTransactionGetHash(tx6)},
	}, 2);
	// Check transaction for branch 0 for account 1
	printf("CHECKING BRANCH 0 TRANSACTIONS ACCOUNT 1\n");
	CBNewAccounterStorageTransactionCursor(&cursor, storage, 0, account1, 1000, 8000);
	checkTransactions(cursor, (CBTestTxDetails [1]){
		{hash1,1000,400,1000,CBTransactionGetHash(tx1)},
	}, 1);
	// Check transaction for branch 0 for account 2
	printf("CHECKING BRANCH 0 TRANSACTIONS ACCOUNT 2\n");
	CBNewAccounterStorageTransactionCursor(&cursor, storage, 0, account2, 1000, 8000);
	checkTransactions(cursor, (CBTestTxDetails [1]){
		{hash3,6000,300,6000,CBTransactionGetHash(tx6)},
	}, 1);
	// Check unspent outputs for branch 0 for account 1
	printf("CHECKING BRANCH 0 OUTPUTS ACCOUNT 1\n");
	CBNewAccounterStorageUnspentOutputCursor(&cursor, storage, 0, account1);
	checkUnspentOutputs(cursor, (CBTestUOutDetails [2]){
		{100, CBTransactionGetHash(tx1), 0, hash1},
		{300, CBTransactionGetHash(tx1), 1, hash1},
	}, 2);
	// Get the unspent output for branch 0 for account 2
	printf("CHECKING BRANCH 0 OUTPUTS ACCOUNT 2\n");
	CBNewAccounterStorageUnspentOutputCursor(&cursor, storage, 0, account2);
	checkUnspentOutputs(cursor, (CBTestUOutDetails [1]){
		{300, CBTransactionGetHash(tx6), 0, hash3},
	}, 1);
	// Check balances
	printf("CHECKING UNCONF BALANCE ACCOUNT 1\n");
	checkBalance(storage, CB_NO_BRANCH, account1, 0);
	// Check unconf balance for account 2
	printf("CHECKING UNCONF BALANCE ACCOUNT 2\n");
	checkBalance(storage, CB_NO_BRANCH, account2, 0);
	// Check branch 1 balance for account 1
	printf("CHECKING BRANCH 1 BALANCE ACCOUNT 1\n");
	checkBalance(storage, 1, account1, 500);
	// Check branch 1 balance for account 2
	printf("CHECKING BRANCH 1 BALANCE ACCOUNT 2\n");
	checkBalance(storage, 1, account2, 300);
	// Check getting transactions for account 1
	printf("CHECKING BRANCH 1 TRANSACTIONS ACCOUNT 1\n");
	CBNewAccounterStorageTransactionCursor(&cursor, storage, 1, account1, 1000, 8000);
	checkTransactions(cursor, (CBTestTxDetails [2]){
		{hash1,1000,400,1000,CBTransactionGetHash(tx1)},
		{hash1,7000,100,7000,CBTransactionGetHash(tx7)},
	}, 2);
	// Check getting transactions for account 2
	printf("CHECKING BRANCH 1 TRANSACTIONS ACCOUNT 2\n");
	CBNewAccounterStorageTransactionCursor(&cursor, storage, 1, account2, 1000, 8000);
	checkTransactions(cursor, (CBTestTxDetails [3]){
		{hash3,6000,300,6000,CBTransactionGetHash(tx6)}, // +300
		{hash3,8000,300,6000,CBTransactionGetHash(tx6)}, // +300
		{hash1,7000,-300,7000,CBTransactionGetHash(tx7)}, // -300
	}, 3);
	// Test getting unspent outputs
	printf("CHECKING BRANCH 1 OUTPUTS ACCOUNT 1\n");
	CBNewAccounterStorageUnspentOutputCursor(&cursor, storage, 1, account1);
	checkUnspentOutputs(cursor, (CBTestUOutDetails [1]){
		{500, CBTransactionGetHash(tx7), 0, hash1},
	}, 1);
	printf("CHECKING BRANCH 1 OUTPUTS ACCOUNT 2\n");
	CBNewAccounterStorageUnspentOutputCursor(&cursor, storage, 1, account2);
	checkUnspentOutputs(cursor, (CBTestUOutDetails [1]){
		{300, CBTransactionGetHash(tx6), 0, hash3},
	}, 1);
	// Test CBAccounterIsOurs
	if (CBAccounterIsOurs(storage, CBTransactionGetHash(tx1)) != CB_TRUE) {
		printf("IS OURS TX1 FAIL\n");
		return 1;
	}
	if (CBAccounterIsOurs(storage, CBTransactionGetHash(tx6)) != CB_TRUE) {
		printf("IS OURS TX6 FAIL\n");
		return 1;
	}
	// Test CBAccounterGetTransactionTime
	uint64_t time;
	if (!CBAccounterGetTransactionTime(storage, CBTransactionGetHash(tx1), &time)) {
		printf("GET TX1 TIME FAIL\n");
		return 1;
	}
	if (time != 1000) {
		printf("TX1 TIME FAIL\n");
		return 1;
	}
	if (!CBAccounterGetTransactionTime(storage, CBTransactionGetHash(tx6), &time)) {
		printf("GET TX2 TIME FAIL\n");
		return 1;
	}
	if (time != 6000) {
		printf("TX6 TIME FAIL\n");
		return 1;
	}
	// Give unconfirmed transaction to account 1 and 2 to make unconfirmed balance
	CBTransaction * tx8 = CBNewTransaction(0, 1);
	CBByteArray * prevOutTx7 = CBNewScriptOfSize(32);
	memcpy(CBByteArrayGetData(prevOutTx7), CBTransactionGetHash(tx7), 32);
	CBTransactionTakeInput(tx8, CBNewTransactionInput(inScript, CB_TX_INPUT_FINAL, prevOutTx7, 0)); // -500 from account 1
	CBTransactionTakeOutput(tx8, CBNewTransactionOutput(400, outScript3)); // 400 for account 2
	// Serialise transaction
	CBGetMessage(tx8)->bytes = CBNewByteArrayOfSize(CBTransactionCalculateLength(tx8));
	if (CBTransactionSerialise(tx8, false) != CBTransactionCalculateLength(tx8)){
		printf("TX8 SERAILISE FAIL\n");
		return 1;
	}
	// Find tx as unconf
	if (! CBAccounterFoundTransaction(storage, tx8, 0, 8000, CB_NO_BRANCH, &details)) {
		printf("FOUND TX8 FAIL\n");
		return 1;
	}
	printf("CHECKING MERGE\n");
	// Test merging account 1 into account 2
	if (!CBAccounterMergeAccountIntoAccount(storage, account2, account1)) {
		printf("MERGE FAIL\n");
		return 1;
	}
	// Check unconfirmed balance
	printf("CHECKING UNCONF BALANCE ACCOUNT 2\n");
	checkBalance(storage, CB_NO_BRANCH, account2, -100);
	printf("CHECKING BRANCH 0 BALANCE ACCOUNT 2\n");
	checkBalance(storage, 0, account2, 700);
	printf("CHECKING BRANCH 1 BALANCE ACCOUNT 2\n");
	checkBalance(storage, 1, account2, 800);
	// Check getting unconf transactions for account 2
	printf("CHECKING UNCONF TRANSACTIONS ACCOUNT 2\n");
	CBNewAccounterStorageTransactionCursor(&cursor, storage, CB_NO_BRANCH, account2, 1000, 8000);
	checkTransactions(cursor, (CBTestTxDetails [1]){
		{hash3,0,-100,8000,CBTransactionGetHash(tx8)},
	}, 1);
	// Check getting branch 0 transactions for account 2
	printf("CHECKING BRANCH 0 TRANSACTIONS ACCOUNT 2\n");
	CBNewAccounterStorageTransactionCursor(&cursor, storage, 0, account2, 1000, 8000);
	checkTransactions(cursor, (CBTestTxDetails [2]){
		{hash1,1000,400,1000,CBTransactionGetHash(tx1)},
		{hash3,6000,300,6000,CBTransactionGetHash(tx6)},
	}, 2);
	// Check getting branch 1 transactions for account 2
	printf("CHECKING BRANCH 1 TRANSACTIONS ACCOUNT 2\n");
	CBNewAccounterStorageTransactionCursor(&cursor, storage, 1, account2, 1000, 8000);
	checkTransactions(cursor, (CBTestTxDetails [4]){
		{hash1,1000,400,1000,CBTransactionGetHash(tx1)}, // +400
		{hash3,6000,300,6000,CBTransactionGetHash(tx6)}, // +300
		{hash3,8000,300,6000,CBTransactionGetHash(tx6)}, // +300
		{hash1,7000,-200,7000,CBTransactionGetHash(tx7)}, // -200
	}, 4);
	// Check getting unconf outputs for account 2
	printf("CHECKING UNCONF OUTPUTS ACCOUNT 2\n");
	CBNewAccounterStorageUnspentOutputCursor(&cursor, storage, CB_NO_BRANCH, account2);
	checkUnspentOutputs(cursor, (CBTestUOutDetails [1]){
		{400, CBTransactionGetHash(tx8), 0, hash3},
	}, 1);
	// Check getting branch 0 outputs for account 2
	printf("CHECKING BRANCH 0 OUTPUTS ACCOUNT 2\n");
	CBNewAccounterStorageUnspentOutputCursor(&cursor, storage, 0, account2);
	checkUnspentOutputs(cursor, (CBTestUOutDetails [3]){
		{300, CBTransactionGetHash(tx6), 0, hash3},
		{100, CBTransactionGetHash(tx1), 0, hash1},
		{300, CBTransactionGetHash(tx1), 1, hash1},
	}, 3);
	// Check getting branch 1 outputs for account 2
	printf("CHECKING BRANCH 1 OUTPUTS ACCOUNT 2\n");
	CBNewAccounterStorageUnspentOutputCursor(&cursor, storage, 1, account2);
	checkUnspentOutputs(cursor, (CBTestUOutDetails [2]){
		{300, CBTransactionGetHash(tx6), 0, hash3},
		{500, CBTransactionGetHash(tx7), 0, hash1},
	}, 2);
	// Check account 2 obtained watched hash of account 1 with new transaction
	CBTransaction * tx9 = CBNewTransaction(0, 1);
	CBTransactionTakeInput(tx9, CBNewTransactionInput(inScript, CB_TX_INPUT_FINAL, prevOutTx1, 0)); // -100 from account 2
	CBTransactionTakeOutput(tx9, CBNewTransactionOutput(50, outScript)); // 50 for account 2
	// Serialise transaction
	CBGetMessage(tx9)->bytes = CBNewByteArrayOfSize(CBTransactionCalculateLength(tx9));
	if (CBTransactionSerialise(tx9, false) != CBTransactionCalculateLength(tx9)){
		printf("TX9 SERAILISE FAIL\n");
		return 1;
	}
	// Find tx as unconf
	if (! CBAccounterFoundTransaction(storage, tx9, 0, 9000, CB_NO_BRANCH, &details)) {
		printf("FOUND TX9 FAIL\n");
		return 1;
	}
	// Verify account 2 had change in balance
	printf("CHECKING ACCOUNT 2 GOT WATCHED HASH FROM ACCOUNT 1\n");
	checkBalance(storage, CB_NO_BRANCH, account2, -150);
	// Add another transaction as unconfirmed spending tx9, then remove it. Does the tx9 outputs then exist as unspent?
	CBTransaction * tx10 = CBNewTransaction(0, 1);
	CBByteArray * prevOutTx9 = CBNewScriptOfSize(32);
	memcpy(CBByteArrayGetData(prevOutTx9), CBTransactionGetHash(tx9), 32);
	CBTransactionTakeInput(tx10, CBNewTransactionInput(inScript, CB_TX_INPUT_FINAL, prevOutTx9, 0)); // -50 from account 2
	CBTransactionTakeOutput(tx10, CBNewTransactionOutput(50, outScript3)); // 50 for account 2
	// Serialise transaction
	CBGetMessage(tx10)->bytes = CBNewByteArrayOfSize(CBTransactionCalculateLength(tx10));
	if (CBTransactionSerialise(tx10, false) != CBTransactionCalculateLength(tx10)){
		printf("TX10 SERAILISE FAIL\n");
		return 1;
	}
	// Find tx as unconf
	if (! CBAccounterFoundTransaction(storage, tx10, 0, 10000, CB_NO_BRANCH, &details)) {
		printf("FOUND TX10 FAIL\n");
		return 1;
	}
	// Remove
	if (! CBAccounterLostUnconfirmedTransaction(storage, tx10)){
		printf("LOST TX10 FAIL\n");
		return 1;
	}
	// Check we can get tx9 output
	CBNewAccounterStorageUnspentOutputCursor(&cursor, storage, CB_NO_BRANCH, account2);
	checkUnspentOutputs(cursor, (CBTestUOutDetails [2]){
		{400, CBTransactionGetHash(tx8), 0, hash3},
		{50, CBTransactionGetHash(tx9), 0, hash1},
	}, 2);
	// ??? Add tests for duplicate transactions added and then removed as unconfimed? Also duplicate txs moved from unconfirmed to a branch.
	CBReleaseObject(tx1);
	CBReleaseObject(tx2);
	CBReleaseObject(tx3);
	CBReleaseObject(tx4);
	CBReleaseObject(tx5);
	CBReleaseObject(tx6);
	CBReleaseObject(tx7);
	CBReleaseObject(tx8);
	CBReleaseObject(tx9);
	CBFreeAccounterStorage(storage);
	return 0;
}
