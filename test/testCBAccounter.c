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
	// Find the transaction branchless
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
	CBTransactionDetails txDetails;
	CBDepObject cursor;
	if (! CBNewAccounterStorageTransactionCursor(&cursor, storage, CB_NO_BRANCH, account1, 1000, 1000)) {
		printf("CREATE CURSOR FAIL\n");
		return 1;
	}
	if (CBAccounterGetNextTransaction(cursor, &txDetails) != CB_TRUE) {
		printf("GET FIRST TX FAIL\n");
		return 1;
	}
	if (memcmp(txDetails.accountTxDetails.addrHash, hash1, 20)) {
		printf("FOUND TX ADDR HASH FAIL\n");
		return 1;
	}
	if (txDetails.height != 0) {
		printf("FOUND TX HEIGHT FAIL\n");
		return 1;
	}
	if (txDetails.accountTxDetails.amount != 400) {
		printf("FOUND TX AMOUNT FAIL\n");
		return 1;
	}
	if (txDetails.timestamp != 1000){
		printf("FOUND TX TIMESTAMP FAIL\n");
		return 1;
	}
	if (memcmp(txDetails.txHash, CBTransactionGetHash(tx1), 32)){
		printf("FOUND TX TX HASH FAIL\n");
		return 1;
	}
	CBFreeAccounterStorageTransactionCursor(cursor);
	// Check obtaining the unconf balance
	int64_t balance;
	if (! CBAccounterGetBranchAccountBalance(storage, CB_NO_BRANCH, account1, &balance)){
		printf("GET UNCONF BAL FAIL\n");
		return 1;
	}
	if (balance != 400) {
		printf("UNCONF BAL NUM FAIL\n");
		return 1;
	}
	// Check obtaining the unspent outputs
	if (! CBNewAccounterStorageUnspentOutputCursor(&cursor, storage, CB_NO_BRANCH, account1)) {
		printf("CREATE UNSPENT OUTPUT CURSOR FAIL\n");
		return 1;
	}
	CBUnspentOutputDetails uoDetails;
	if (CBAccounterGetNextUnspentOutput(cursor, &uoDetails) != CB_TRUE) {
		printf("GET FIRST UNSPENT OUTPUT FAIL\n");
		return 1;
	}
	if (uoDetails.value != 100) {
		printf("FIRST UNSPENT OUTPUT VALUE FAIL\n");
		return 1;
	}
	if (memcmp(uoDetails.txHash, CBTransactionGetHash(tx1), 32)) {
		printf("FIRST UNSPENT OUTPUT TX FAIL\n");
		return 1;
	}
	if (uoDetails.index != 0) {
		printf("FIRST UNSPENT OUTPUT INDEX FAIL\n");
		return 1;
	}
	if (memcmp(uoDetails.watchedHash, hash1, 20)) {
		printf("FIRST UNSPENT OUTPUT ID HASH FAIL\n");
		return 1;
	}
	if (CBAccounterGetNextUnspentOutput(cursor, &uoDetails) != CB_TRUE) {
		CBLogError("GET SECOND UNSPENT OUTPUT FAIL");
		return 1;
	}
	if (uoDetails.value != 300) {
		printf("SECOND UNSPENT OUTPUT VALUE FAIL\n");
		return 1;
	}
	if (memcmp(uoDetails.txHash, CBTransactionGetHash(tx1), 32)) {
		printf("SECOND UNSPENT OUTPUT TX FAIL\n");
		return 1;
	}
	if (uoDetails.index != 1) {
		printf("SECOND UNSPENT OUTPUT INDEX FAIL\n");
		return 1;
	}
	if (memcmp(uoDetails.watchedHash, hash1, 20)) {
		printf("SECOND UNSPENT OUTPUT ID HASH FAIL\n");
		return 1;
	}
	if (CBAccounterGetNextUnspentOutput(cursor, &uoDetails) != CB_FALSE) {
		CBLogError("NO MORE UNSPENT OUTPUTS FAIL");
		return 1;
	}
	CBFreeAccounterStorageUnspentOutputCursor(cursor);
	// Create first branch
	if (! CBAccounterNewBranch(storage, 0, CB_NO_PARENT, 0)) {
		printf("CREATE FIRST BRANCH FAIL\n");
		return 1;
	}
	// Move transaction into branch
	if (! CBAccounterBranchlessTransactionToBranch(storage, tx1, 1000, 0)) {
		printf("MOVE TX TO BRANCH FAIL\n");
		return 1;
	}
	// Check balances
	if (! CBAccounterGetBranchAccountBalance(storage, CB_NO_BRANCH, account1, &balance)){
		printf("GET UNCONF BAL AFTER MOVE FAIL\n");
		return 1;
	}
	if (balance != 0) {
		printf("UNCONF BAL NUM AFTER MOVE FAIL\n");
		return 1;
	}
	if (! CBAccounterGetBranchAccountBalance(storage, 0, account1, &balance)){
		printf("GET BRANCH 0 BAL AFTER MOVE FAIL\n");
		return 1;
	}
	if (balance != 400) {
		printf("UNCONF BRANCH 0 NUM AFTER MOVE FAIL\n");
		return 1;
	}
	// Check we can't get transaction as branchless
	CBNewAccounterStorageTransactionCursor(&cursor, storage, CB_NO_BRANCH, account1, 1000, 1000);
	if (CBAccounterGetNextTransaction(cursor, &txDetails) != CB_FALSE) {
		printf("GET FIRST TX IN BRANCHLESS AFTER MOVE FAIL\n");
		return 1;
	}
	CBFreeAccounterStorageTransactionCursor(cursor);
	// Check we can get transaction on branch
	CBNewAccounterStorageTransactionCursor(&cursor, storage, 0, account1, 1000, 1000);
	if (CBAccounterGetNextTransaction(cursor, &txDetails) != CB_TRUE) {
		printf("GET FIRST TX IN BRANCH 0 AFTER MOVE FAIL\n");
		return 1;
	}
	if (memcmp(txDetails.accountTxDetails.addrHash, hash1, 20)) {
		printf("FOUND TX ADDR HASH AFTER MOVE FAIL\n");
		return 1;
	}
	if (txDetails.height != 1000) {
		printf("FOUND TX HEIGHT AFTER MOVE FAIL\n");
		return 1;
	}
	if (txDetails.accountTxDetails.amount != 400) {
		printf("FOUND TX AMOUNT AFTER MOVE FAIL\n");
		return 1;
	}
	if (txDetails.timestamp != 1000){
		printf("FOUND TX TIMESTAMP AFTER MOVE FAIL\n");
		return 1;
	}
	if (memcmp(txDetails.txHash, CBTransactionGetHash(tx1), 32)){
		printf("FOUND TX TX HASH AFTER MOVE FAIL\n");
		return 1;
	}
	CBFreeAccounterStorageTransactionCursor(cursor);
	// Check cannot get unspent output as unconfirmed
	CBNewAccounterStorageUnspentOutputCursor(&cursor, storage, CB_NO_BRANCH, account1);
	if (CBAccounterGetNextUnspentOutput(cursor, &uoDetails) != CB_FALSE) {
		printf("GET FIRST UNSPENT OUTPUT IN BRANCHLESS AFTER MOVE FAIL\n");
		return 1;
	}
	CBFreeAccounterStorageUnspentOutputCursor(cursor);
	// Check unspent output in branch 0
	CBNewAccounterStorageUnspentOutputCursor(&cursor, storage, 0, account1);
	if (CBAccounterGetNextUnspentOutput(cursor, &uoDetails) != CB_TRUE) {
		printf("GET FIRST UNSPENT OUTPUT IN BRANCH 0 AFTER MOVE FAIL\n");
		return 1;
	}
	if (uoDetails.value != 100) {
		printf("FIRST UNSPENT OUTPUT VALUE AFTER MOVE FAIL\n");
		return 1;
	}
	if (memcmp(uoDetails.txHash, CBTransactionGetHash(tx1), 32)) {
		printf("FIRST UNSPENT OUTPUT TX AFTER MOVE FAIL\n");
		return 1;
	}
	if (uoDetails.index != 0) {
		printf("FIRST UNSPENT OUTPUT INDEX AFTER MOVE FAIL\n");
		return 1;
	}
	if (memcmp(uoDetails.watchedHash, hash1, 20)) {
		printf("FIRST UNSPENT OUTPUT ID HASH AFTER MOVE FAIL\n");
		return 1;
	}
	if (CBAccounterGetNextUnspentOutput(cursor, &uoDetails) != CB_TRUE) {
		CBLogError("GET SECOND UNSPENT OUTPUT AFTER MOVE FAIL");
		return 1;
	}
	if (uoDetails.value != 300) {
		printf("SECOND UNSPENT OUTPUT VALUE AFTER MOVE FAIL\n");
		return 1;
	}
	if (memcmp(uoDetails.txHash, CBTransactionGetHash(tx1), 32)) {
		printf("SECOND UNSPENT OUTPUT TX AFTER MOVE FAIL\n");
		return 1;
	}
	if (uoDetails.index != 1) {
		printf("SECOND UNSPENT OUTPUT INDEX AFTER MOVE FAIL\n");
		return 1;
	}
	if (memcmp(uoDetails.watchedHash, hash1, 20)) {
		printf("SECOND UNSPENT OUTPUT ID HASH AFTER MOVE FAIL\n");
		return 1;
	}
	if (CBAccounterGetNextUnspentOutput(cursor, &uoDetails) != CB_FALSE) {
		CBLogError("NO MORE UNSPENT OUTPUTS AFTER MOVE FAIL");
		return 1;
	}
	CBFreeAccounterStorageUnspentOutputCursor(cursor);
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
	CBTransactionTakeInput(tx2, CBNewTransactionInput(inScript, CB_TX_INPUT_FINAL, prevOutTx1, 0));
	// Not owned by us
	CBTransactionTakeInput(tx2, CBNewTransactionInput(inScript, CB_TX_INPUT_FINAL, prevOutTx1, 2));
	CBTransactionTakeOutput(tx2, CBNewTransactionOutput(50, outScript));
	CBTransactionTakeOutput(tx2, CBNewTransactionOutput(100, outScript2));
	// Serialise transaction
	CBGetMessage(tx2)->bytes = CBNewByteArrayOfSize(CBTransactionCalculateLength(tx2));
	if (CBTransactionSerialise(tx2, false) != CBTransactionCalculateLength(tx2)){
		printf("TX2 SERAILISE FAIL\n");
		return 1;
	}
	// Find transaction on branchless
	if (! CBAccounterFoundTransaction(storage, tx2, 0, 2000, CB_NO_BRANCH, &details)) {
		printf("FOUND TX2 FAIL\n");
		return 1;
	}
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
	// Check branchless balance
	if (! CBAccounterGetBranchAccountBalance(storage, CB_NO_BRANCH, account1, &balance)){
		printf("GET UNCONF BAL FOR TX2 FAIL\n");
		return 1;
	}
	if (balance != 50) {
		printf("UNCONF BAL NUM FOR TX2 FAIL\n");
		return 1;
	}
	// Check unspent outputs for branch 0
	CBNewAccounterStorageUnspentOutputCursor(&cursor, storage, 0, account1);
	if (CBAccounterGetNextUnspentOutput(cursor, &uoDetails) != CB_TRUE) {
		printf("GET FIRST UNSPENT OUTPUT IN BRANCH 0 FOR TX2 FAIL\n");
		return 1;
	}
	if (uoDetails.value != 300) {
		printf("FIRST UNSPENT OUTPUT VALUE FOR TX2 FAIL\n");
		return 1;
	}
	if (memcmp(uoDetails.txHash, CBTransactionGetHash(tx1), 32)) {
		printf("FIRST UNSPENT OUTPUT TX FOR TX2 FAIL\n");
		return 1;
	}
	if (uoDetails.index != 1) {
		printf("FIRST UNSPENT OUTPUT INDEX FOR TX2 FAIL\n");
		return 1;
	}
	if (memcmp(uoDetails.watchedHash, hash1, 20)) {
		printf("FIRST UNSPENT OUTPUT ID HASH FOR TX2 FAIL\n");
		return 1;
	}
	if (CBAccounterGetNextUnspentOutput(cursor, &uoDetails) != CB_FALSE) {
		CBLogError("NO MORE UNSPENT OUTPUTS FOR TX2 FAIL");
		return 1;
	}
	CBFreeAccounterStorageUnspentOutputCursor(cursor);
	// Check unspent outputs for branchless
	CBNewAccounterStorageUnspentOutputCursor(&cursor, storage, CB_NO_BRANCH, account1);
	if (CBAccounterGetNextUnspentOutput(cursor, &uoDetails) != CB_TRUE) {
		printf("GET FIRST UNSPENT OUTPUT IN BRANCHLESS FOR TX2 FAIL\n");
		return 1;
	}
	if (uoDetails.value != 50) {
		printf("FIRST UNSPENT OUTPUT VALUE FOR TX2 IN BRANCHLESS FAIL\n");
		return 1;
	}
	if (memcmp(uoDetails.txHash, CBTransactionGetHash(tx2), 32)) {
		printf("FIRST UNSPENT OUTPUT TX FOR TX2 IN BRANCHLESS FAIL\n");
		return 1;
	}
	if (uoDetails.index != 0) {
		printf("FIRST UNSPENT OUTPUT INDEX FOR TX2 IN BRANCHLESS FAIL\n");
		return 1;
	}
	if (memcmp(uoDetails.watchedHash, hash1, 20)) {
		printf("FIRST UNSPENT OUTPUT ID HASH FOR TX2 IN BRANCHLESS FAIL\n");
		return 1;
	}
	if (CBAccounterGetNextUnspentOutput(cursor, &uoDetails) != CB_TRUE) {
		printf("GET SECOND UNSPENT OUTPUT IN BRANCHLESS FOR TX2 FAIL\n");
		return 1;
	}
	if (uoDetails.value != 100) {
		printf("SECOND UNSPENT OUTPUT VALUE FOR TX2 IN BRANCHLESS FAIL\n");
		return 1;
	}
	if (memcmp(uoDetails.txHash, CBTransactionGetHash(tx2), 32)) {
		printf("SECOND UNSPENT OUTPUT TX FOR TX2 IN BRANCHLESS FAIL\n");
		return 1;
	}
	if (uoDetails.index != 1) {
		printf("SECOND UNSPENT OUTPUT INDEX FOR TX2 IN BRANCHLESS FAIL\n");
		return 1;
	}
	if (memcmp(uoDetails.watchedHash, hash2, 20)) {
		printf("SECOND UNSPENT OUTPUT ID HASH FOR TX2 IN BRANCHLESS FAIL\n");
		return 1;
	}
	if (CBAccounterGetNextUnspentOutput(cursor, &uoDetails) != CB_FALSE) {
		printf("NO MORE UNSPENT OUTPUTS FOR TX2 IN BRANCHLESS FAIL\n");
		return 1;
	}
	CBFreeAccounterStorageUnspentOutputCursor(cursor);
	// Test loosing branchless transaction
	if (! CBAccounterLostBranchlessTransaction(storage, tx2)) {
		printf("LOOSE BRANCHLESS TX FAIL\n");
		return 1;
	}
	// Check balances
	if (! CBAccounterGetBranchAccountBalance(storage, CB_NO_BRANCH, account1, &balance)){
		printf("GET UNCONF BAL AFTER LOOSE FAIL\n");
		return 1;
	}
	if (balance != 0) {
		printf("UNCONF BAL NUM AFTER LOOSE FAIL\n");
		return 1;
	}
	if (! CBAccounterGetBranchAccountBalance(storage, 0, account1, &balance)){
		printf("GET BRANCH 0 BAL AFTER LOOSE FAIL\n");
		return 1;
	}
	if (balance != 400) {
		printf("UNCONF BRANCH 0 NUM AFTER LOOSE FAIL\n");
		return 1;
	}
	// Check we can't get transaction as branchless
	CBNewAccounterStorageTransactionCursor(&cursor, storage, CB_NO_BRANCH, account1, 1000, 2000);
	if (CBAccounterGetNextTransaction(cursor, &txDetails) != CB_FALSE) {
		printf("GET FIRST TX IN BRANCHLESS AFTER LOOSE FAIL\n");
		return 1;
	}
	CBFreeAccounterStorageTransactionCursor(cursor);
	// Check we can get transaction on branch
	CBNewAccounterStorageTransactionCursor(&cursor, storage, 0, account1, 1000, 2000);
	if (CBAccounterGetNextTransaction(cursor, &txDetails) != CB_TRUE) {
		printf("GET FIRST TX IN BRANCH 0 AFTER LOOSE FAIL\n");
		return 1;
	}
	if (memcmp(txDetails.accountTxDetails.addrHash, hash1, 20)) {
		printf("FOUND TX ADDR HASH AFTER LOOSE FAIL\n");
		return 1;
	}
	if (txDetails.height != 1000) {
		printf("FOUND TX HEIGHT AFTER LOOSE FAIL\n");
		return 1;
	}
	if (txDetails.accountTxDetails.amount != 400) {
		printf("FOUND TX AMOUNT AFTER LOOSE FAIL\n");
		return 1;
	}
	if (txDetails.timestamp != 1000){
		printf("FOUND TX TIMESTAMP AFTER LOOSE FAIL\n");
		return 1;
	}
	if (memcmp(txDetails.txHash, CBTransactionGetHash(tx1), 32)){
		printf("FOUND TX TX HASH AFTER LOOSE FAIL\n");
		return 1;
	}
	CBFreeAccounterStorageTransactionCursor(cursor);
	// Check cannot get unspent output as unconfirmed
	CBNewAccounterStorageUnspentOutputCursor(&cursor, storage, CB_NO_BRANCH, account1);
	if (CBAccounterGetNextUnspentOutput(cursor, &uoDetails) != CB_FALSE) {
		printf("GET FIRST UNSPENT OUTPUT IN BRANCHLESS AFTER LOOSE FAIL\n");
		return 1;
	}
	CBFreeAccounterStorageUnspentOutputCursor(cursor);
	// Check unspent output in branch 0
	CBNewAccounterStorageUnspentOutputCursor(&cursor, storage, 0, account1);
	if (CBAccounterGetNextUnspentOutput(cursor, &uoDetails) != CB_TRUE) {
		printf("GET FIRST UNSPENT OUTPUT IN BRANCH 0 AFTER LOOSE FAIL\n");
		return 1;
	}
	if (uoDetails.value != 100) {
		printf("FIRST UNSPENT OUTPUT VALUE AFTER LOOSE FAIL\n");
		return 1;
	}
	if (memcmp(uoDetails.txHash, CBTransactionGetHash(tx1), 32)) {
		printf("FIRST UNSPENT OUTPUT TX AFTER LOOSE FAIL\n");
		return 1;
	}
	if (uoDetails.index != 0) {
		printf("FIRST UNSPENT OUTPUT INDEX AFTER LOOSE FAIL\n");
		return 1;
	}
	if (memcmp(uoDetails.watchedHash, hash1, 20)) {
		printf("FIRST UNSPENT OUTPUT ID HASH AFTER LOOSE FAIL\n");
		return 1;
	}
	if (CBAccounterGetNextUnspentOutput(cursor, &uoDetails) != CB_TRUE) {
		CBLogError("GET SECOND UNSPENT OUTPUT AFTER LOOSE FAIL");
		return 1;
	}
	if (uoDetails.value != 300) {
		printf("SECOND UNSPENT OUTPUT VALUE AFTER LOOSE FAIL\n");
		return 1;
	}
	if (memcmp(uoDetails.txHash, CBTransactionGetHash(tx1), 32)) {
		printf("SECOND UNSPENT OUTPUT TX AFTER LOOSE FAIL\n");
		return 1;
	}
	if (uoDetails.index != 1) {
		printf("SECOND UNSPENT OUTPUT INDEX AFTER LOOSE FAIL\n");
		return 1;
	}
	if (memcmp(uoDetails.watchedHash, hash1, 20)) {
		printf("SECOND UNSPENT OUTPUT ID HASH AFTER LOOSE FAIL\n");
		return 1;
	}
	if (CBAccounterGetNextUnspentOutput(cursor, &uoDetails) != CB_FALSE) {
		CBLogError("NO MORE UNSPENT OUTPUTS AFTER LOOSE FAIL");
		return 1;
	}
	CBFreeAccounterStorageUnspentOutputCursor(cursor);
	// Add directly to branch
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
	if (! CBAccounterGetBranchAccountBalance(storage, CB_NO_BRANCH, account1, &balance)){
		printf("GET UNCONF BAL AFTER DIRECT FAIL\n");
		return 1;
	}
	if (balance != 0) {
		printf("UNCONF BAL NUM AFTER DIRECT FAIL\n");
		return 1;
	}
	if (! CBAccounterGetBranchAccountBalance(storage, 0, account1, &balance)){
		printf("GET BRANCH 0 BAL AFTER DIRECT FAIL\n");
		return 1;
	}
	if (balance != 450) {
		printf("BRANCH 0 BAL NUM AFTER DIRECT FAIL\n");
		return 1;
	}
	// Check transactions
	CBNewAccounterStorageTransactionCursor(&cursor, storage, 0, account1, 1000, 3000);
	if (CBAccounterGetNextTransaction(cursor, &txDetails) != CB_TRUE) {
		printf("GET FIRST TX IN BRANCH 0 AFTER DIRECT FAIL\n");
		return 1;
	}
	if (memcmp(txDetails.accountTxDetails.addrHash, hash1, 20)) {
		printf("FIRST TX ADDR HASH AFTER DIRECT FAIL\n");
		return 1;
	}
	if (txDetails.height != 1000) {
		printf("FIRST TX HEIGHT AFTER DIRECT FAIL\n");
		return 1;
	}
	if (txDetails.accountTxDetails.amount != 400) {
		printf("FIRST TX AMOUNT AFTER DIRECT FAIL\n");
		return 1;
	}
	if (txDetails.timestamp != 1000){
		printf("FIRST TX TIMESTAMP AFTER DIRECT FAIL\n");
		return 1;
	}
	if (memcmp(txDetails.txHash, CBTransactionGetHash(tx1), 32)){
		printf("FIRST TX TX HASH AFTER DIRECT FAIL\n");
		return 1;
	}
	if (CBAccounterGetNextTransaction(cursor, &txDetails) != CB_TRUE) {
		printf("GET SECOND TX IN BRANCH 0 AFTER DIRECT FAIL\n");
		return 1;
	}
	if (memcmp(txDetails.accountTxDetails.addrHash, hash1, 20)) {
		printf("SECOND TX ADDR HASH AFTER DIRECT FAIL\n");
		return 1;
	}
	if (txDetails.height != 2000) {
		printf("SECOND TX HEIGHT AFTER DIRECT FAIL\n");
		return 1;
	}
	if (txDetails.accountTxDetails.amount != 50) {
		printf("SECOND TX AMOUNT AFTER DIRECT FAIL\n");
		return 1;
	}
	if (txDetails.timestamp != 3000){
		printf("SECOND TX TIMESTAMP AFTER DIRECT FAIL\n");
		return 1;
	}
	if (memcmp(txDetails.txHash, CBTransactionGetHash(tx2), 32)){
		printf("SECOND TX TX HASH AFTER DIRECT FAIL\n");
		return 1;
	}
	if (CBAccounterGetNextTransaction(cursor, &txDetails) != CB_FALSE) {
		printf("NO MORE TX IN BRANCH 0 AFTER DIRECT FAIL\n");
		return 1;
	}
	// Check unspent outputs for branch 0
	CBNewAccounterStorageUnspentOutputCursor(&cursor, storage, 0, account1);
	if (CBAccounterGetNextUnspentOutput(cursor, &uoDetails) != CB_TRUE) {
		printf("GET FIRST UNSPENT OUTPUT IN BRANCH 0 AFTER DIRECT FAIL\n");
		return 1;
	}
	if (uoDetails.value != 50) {
		printf("FIRST UNSPENT OUTPUT VALUE AFTER DIRECT FAIL\n");
		return 1;
	}
	if (memcmp(uoDetails.txHash, CBTransactionGetHash(tx2), 32)) {
		printf("FIRST UNSPENT OUTPUT TX AFTER DIRECT FAIL\n");
		return 1;
	}
	if (uoDetails.index != 0) {
		printf("FIRST UNSPENT OUTPUT INDEX AFTER DIRECT2 FAIL\n");
		return 1;
	}
	if (memcmp(uoDetails.watchedHash, hash1, 20)) {
		printf("FIRST UNSPENT OUTPUT ID HASH AFTER DIRECT FAIL\n");
		return 1;
	}
	if (CBAccounterGetNextUnspentOutput(cursor, &uoDetails) != CB_TRUE) {
		printf("GET SECOND UNSPENT OUTPUT AFTER DIRECT FAIL\n");
		return 1;
	}
	if (uoDetails.value != 100) {
		printf("SECOND UNSPENT OUTPUT VALUE AFTER DIRECT FAIL\n");
		return 1;
	}
	if (memcmp(uoDetails.txHash, CBTransactionGetHash(tx2), 32)) {
		printf("SECOND UNSPENT OUTPUT TX AFTER DIRECT FAIL\n");
		return 1;
	}
	if (uoDetails.index != 1) {
		printf("SECOND UNSPENT OUTPUT INDEX AFTER DIRECT FAIL\n");
		return 1;
	}
	if (memcmp(uoDetails.watchedHash, hash2, 20)) {
		printf("SECOND UNSPENT OUTPUT ID HASH AFTER DIRECT FAIL\n");
		return 1;
	}
	if (CBAccounterGetNextUnspentOutput(cursor, &uoDetails) != CB_TRUE) {
		printf("GET THIRD UNSPENT OUTPUT AFTER DIRECT FAIL\n");
		return 1;
	}
	if (uoDetails.value != 300) {
		printf("THIRD UNSPENT OUTPUT VALUE AFTER DIRECT FAIL\n");
		return 1;
	}
	if (memcmp(uoDetails.txHash, CBTransactionGetHash(tx1), 32)) {
		printf("THIRD UNSPENT OUTPUT TX AFTER DIRECT FAIL\n");
		return 1;
	}
	if (uoDetails.index != 1) {
		printf("THIRD UNSPENT OUTPUT INDEX AFTER DIRECT FAIL\n");
		return 1;
	}
	if (memcmp(uoDetails.watchedHash, hash1, 20)) {
		printf("THIRD UNSPENT OUTPUT ID HASH AFTER DIRECT FAIL\n");
		return 1;
	}
	if (CBAccounterGetNextUnspentOutput(cursor, &uoDetails) != CB_FALSE) {
		CBLogError("NO MORE UNSPENT OUTPUTS AFTER DIRECT FAIL");
		return 1;
	}
	CBFreeAccounterStorageUnspentOutputCursor(cursor);
	// Test making branchless transaction, then adding another transaction with the same timestamp to the branch and then finally adding the branchless transaction. Test also negative branchless balance.
	// Make next transaction
	CBTransaction * tx3 = CBNewTransaction(0, 1);
	CBTransactionTakeInput(tx3, CBNewTransactionInput(inScript, CB_TX_INPUT_FINAL, prevOutTx1, 1));
	CBTransactionTakeOutput(tx3, CBNewTransactionOutput(100, outScript));
	// Serialise transaction
	CBGetMessage(tx3)->bytes = CBNewByteArrayOfSize(CBTransactionCalculateLength(tx3));
	if (CBTransactionSerialise(tx3, false) != CBTransactionCalculateLength(tx3)){
		printf("TX3 SERAILISE FAIL\n");
		return 1;
	}
	// Find transaction on branchless
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
	// Check negative balance
	if (! CBAccounterGetBranchAccountBalance(storage, CB_NO_BRANCH, account1, &balance)){
		printf("GET UNCONF BAL FOR TX3 FAIL\n");
		return 1;
	}
	if (balance != -200) {
		printf("UNCONF BAL NUM FOR TX3 FAIL\n");
		return 1;
	}
	// Add next transaction directly to branch
	CBTransaction * tx4 = CBNewTransaction(0, 1);
	CBByteArray * prevOutTx2 = CBNewScriptOfSize(32);
	memcpy(CBByteArrayGetData(prevOutTx2), CBTransactionGetHash(tx2), 32);
	CBTransactionTakeInput(tx4, CBNewTransactionInput(inScript, CB_TX_INPUT_FINAL, prevOutTx2, 0));
	CBTransactionTakeInput(tx4, CBNewTransactionInput(inScript, CB_TX_INPUT_FINAL, prevOutTx2, 1));
	CBTransactionTakeOutput(tx4, CBNewTransactionOutput(100, outScript2));
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
	if (! CBAccounterBranchlessTransactionToBranch(storage, tx3, 4000, 0)) {
		printf("MOVE TX3 INTO BRANCH 0\n");
		return 1;
	}
	// Now check balances
	if (! CBAccounterGetBranchAccountBalance(storage, CB_NO_BRANCH, account1, &balance)){
		printf("GET UNCONF BAL AFTER TX3 MOVE FAIL\n");
		return 1;
	}
	if (balance != 0) {
		printf("UNCONF BAL NUM AFTER TX3 MOVE FAIL\n");
		return 1;
	}
	if (! CBAccounterGetBranchAccountBalance(storage, 0, account1, &balance)){
		printf("GET BRANCH 0 BAL AFTER TX3 MOVE FAIL\n");
		return 1;
	}
	if (balance != 200) {
		printf("BRANCH 0 BAL NUM AFTER TX3 MOVE FAIL\n");
		return 1;
	}
	// Check transactions
	CBNewAccounterStorageTransactionCursor(&cursor, storage, 0, account1, 1000, 4000);
	if (CBAccounterGetNextTransaction(cursor, &txDetails) != CB_TRUE) {
		printf("GET FIRST TX IN BRANCH 0 AFTER TX3 MOVE FAIL\n");
		return 1;
	}
	if (memcmp(txDetails.accountTxDetails.addrHash, hash1, 20)) {
		printf("FIRST TX ADDR HASH AFTER TX3 MOVE FAIL\n");
		return 1;
	}
	if (txDetails.height != 1000) {
		printf("FIRST TX HEIGHT AFTER TX3 MOVE FAIL\n");
		return 1;
	}
	if (txDetails.accountTxDetails.amount != 400) {
		printf("FIRST TX AMOUNT AFTER TX3 MOVE FAIL\n");
		return 1;
	}
	if (txDetails.timestamp != 1000){
		printf("FIRST TX TIMESTAMP AFTER TX3 MOVE FAIL\n");
		return 1;
	}
	if (memcmp(txDetails.txHash, CBTransactionGetHash(tx1), 32)){
		printf("FIRST TX TX HASH AFTER TX3 MOVE FAIL\n");
		return 1;
	}
	if (CBAccounterGetNextTransaction(cursor, &txDetails) != CB_TRUE) {
		printf("GET SECOND TX IN BRANCH 0 AFTER TX3 MOVE FAIL\n");
		return 1;
	}
	if (memcmp(txDetails.accountTxDetails.addrHash, hash1, 20)) {
		printf("SECOND TX ADDR HASH AFTER TX3 MOVE FAIL\n");
		return 1;
	}
	if (txDetails.height != 2000) {
		printf("SECOND TX HEIGHT AFTER TX3 MOVE FAIL\n");
		return 1;
	}
	if (txDetails.accountTxDetails.amount != 50) {
		printf("SECOND TX AMOUNT AFTER TX3 MOVE FAIL\n");
		return 1;
	}
	if (txDetails.timestamp != 3000){
		printf("SECOND TX TIMESTAMP AFTER TX3 MOVE FAIL\n");
		return 1;
	}
	if (memcmp(txDetails.txHash, CBTransactionGetHash(tx2), 32)){
		printf("SECOND TX TX HASH AFTER TX3 MOVE FAIL\n");
		return 1;
	}
	// This should be tx4 followed by tx3
	if (CBAccounterGetNextTransaction(cursor, &txDetails) != CB_TRUE) {
		printf("GET THIRD TX IN BRANCH 0 AFTER TX3 MOVE FAIL\n");
		return 1;
	}
	if (memcmp(txDetails.accountTxDetails.addrHash, CBByteArrayGetData(prevOutHash), 20)) {
		printf("THIRD TX ADDR HASH AFTER TX3 MOVE FAIL\n");
		return 1;
	}
	if (txDetails.height != 4000) {
		printf("THIRD TX HEIGHT AFTER TX3 MOVE FAIL\n");
		return 1;
	}
	if (txDetails.accountTxDetails.amount != -50) {
		printf("THIRD TX AMOUNT AFTER TX3 MOVE FAIL\n");
		return 1;
	}
	if (txDetails.timestamp != 4000){
		printf("THIRD TX TIMESTAMP AFTER TX3 MOVE FAIL\n");
		return 1;
	}
	if (memcmp(txDetails.txHash, CBTransactionGetHash(tx4), 32)){
		printf("THIRD TX TX HASH AFTER TX3 MOVE FAIL\n");
		return 1;
	}
	if (CBAccounterGetNextTransaction(cursor, &txDetails) != CB_TRUE) {
		printf("GET FORTH TX IN BRANCH 0 AFTER TX3 MOVE FAIL\n");
		return 1;
	}
	if (memcmp(txDetails.accountTxDetails.addrHash, CBByteArrayGetData(prevOutHash), 20)) {
		printf("FORTH TX ADDR HASH AFTER TX3 MOVE FAIL\n");
		return 1;
	}
	if (txDetails.height != 4000) {
		printf("FORTH TX HEIGHT AFTER TX3 MOVE FAIL\n");
		return 1;
	}
	if (txDetails.accountTxDetails.amount != -200) {
		printf("FORTH TX AMOUNT AFTER TX3 MOVE FAIL\n");
		return 1;
	}
	if (txDetails.timestamp != 4000){
		printf("FORTH TX TIMESTAMP AFTER TX3 MOVE FAIL\n");
		return 1;
	}
	if (memcmp(txDetails.txHash, CBTransactionGetHash(tx3), 32)){
		printf("FORTH TX TX HASH AFTER TX3 MOVE FAIL\n");
		return 1;
	}
	if (CBAccounterGetNextTransaction(cursor, &txDetails) != CB_FALSE) {
		printf("NO MORE TX IN BRANCH 0 AFTER TX3 MOVE FAIL\n");
		return 1;
	}
	CBFreeAccounterStorageTransactionCursor(cursor);
	// Check outputs
	CBNewAccounterStorageUnspentOutputCursor(&cursor, storage, 0, account1);
	if (CBAccounterGetNextUnspentOutput(cursor, &uoDetails) != CB_TRUE) {
		printf("GET FIRST UNSPENT OUTPUT IN BRANCH 0 AFTER DIRECT FAIL\n");
		return 1;
	}
	if (uoDetails.value != 100) {
		printf("FIRST UNSPENT OUTPUT VALUE AFTER DIRECT FAIL\n");
		return 1;
	}
	if (memcmp(uoDetails.txHash, CBTransactionGetHash(tx3), 32)) {
		printf("FIRST UNSPENT OUTPUT TX AFTER DIRECT FAIL\n");
		return 1;
	}
	if (uoDetails.index != 0) {
		printf("FIRST UNSPENT OUTPUT INDEX AFTER DIRECT2 FAIL\n");
		return 1;
	}
	if (memcmp(uoDetails.watchedHash, hash1, 20)) {
		printf("FIRST UNSPENT OUTPUT ID HASH AFTER DIRECT FAIL\n");
		return 1;
	}
	if (CBAccounterGetNextUnspentOutput(cursor, &uoDetails) != CB_TRUE) {
		printf("GET SECOND UNSPENT OUTPUT AFTER DIRECT FAIL\n");
		return 1;
	}
	if (uoDetails.value != 100) {
		printf("SECOND UNSPENT OUTPUT VALUE AFTER DIRECT FAIL\n");
		return 1;
	}
	if (memcmp(uoDetails.txHash, CBTransactionGetHash(tx4), 32)) {
		printf("SECOND UNSPENT OUTPUT TX AFTER DIRECT FAIL\n");
		return 1;
	}
	if (uoDetails.index != 0) {
		printf("SECOND UNSPENT OUTPUT INDEX AFTER DIRECT FAIL\n");
		return 1;
	}
	if (memcmp(uoDetails.watchedHash, hash2, 20)) {
		printf("SECOND UNSPENT OUTPUT ID HASH AFTER DIRECT FAIL\n");
		return 1;
	}
	if (CBAccounterGetNextUnspentOutput(cursor, &uoDetails) != CB_FALSE) {
		CBLogError("NO MORE UNSPENT OUTPUTS AFTER DIRECT FAIL");
		return 1;
	}
	CBFreeAccounterStorageUnspentOutputCursor(cursor);
	// Check that no unspent outputs can be gotten from CB_NO_BRANCH
	CBNewAccounterStorageUnspentOutputCursor(&cursor, storage, CB_NO_BRANCH, account1);
	if (CBAccounterGetNextUnspentOutput(cursor, &uoDetails) != CB_FALSE) {
		CBLogError("NO BRANCHLESS UNSPENT OUTPUTS AFTER DIRECT FAIL");
		return 1;
	}
	CBFreeAccounterStorageUnspentOutputCursor(cursor);
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
	// Test adding to branchless
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
	if (! CBAccounterGetBranchAccountBalance(storage, CB_NO_BRANCH, account1, &balance)){
		printf("GET UNCONF BAL FOR TX5 ACCOUNT 1 FAIL\n");
		return 1;
	}
	if (balance != -100) {
		printf("UNCONF BAL NUM FOR TX5 ACCOUNT 1 FAIL\n");
		return 1;
	}
	// Check unconf balance for account 2
	if (! CBAccounterGetBranchAccountBalance(storage, CB_NO_BRANCH, account2, &balance)){
		printf("GET UNCONF BAL FOR TX5 ACCOUNT 2 FAIL\n");
		return 1;
	}
	if (balance != 100) {
		printf("UNCONF BAL NUM FOR TX5 ACCOUNT 2 FAIL\n");
		return 1;
	}
	// Check unconf transaction for account 1
	CBNewAccounterStorageTransactionCursor(&cursor, storage, CB_NO_BRANCH, account1, 1000, 5000);
	if (CBAccounterGetNextTransaction(cursor, &txDetails) != CB_TRUE) {
		printf("GET FIRST TX IN BRANCHLESS FOR TX5 ACCOUNT 1 FAIL\n");
		return 1;
	}
	if (memcmp(txDetails.accountTxDetails.addrHash, hash3, 20)) {
		printf("FIRST TX ADDR HASH FOR TX5 ACCOUNT 1 FAIL\n");
		return 1;
	}
	if (txDetails.height != 0) {
		printf("FIRST TX HEIGHT FOR TX5 ACCOUNT 1 FAIL\n");
		return 1;
	}
	if (txDetails.accountTxDetails.amount != -100) {
		printf("FIRST TX AMOUNT FOR TX5 ACCOUNT 1 FAIL\n");
		return 1;
	}
	if (txDetails.timestamp != 5000){
		printf("FIRST TX TIMESTAMP FOR TX5 ACCOUNT 1 FAIL\n");
		return 1;
	}
	if (memcmp(txDetails.txHash, CBTransactionGetHash(tx5), 32)){
		printf("FIRST TX TX HASH FOR TX5 ACCOUNT 1 FAIL\n");
		return 1;
	}
	if (CBAccounterGetNextTransaction(cursor, &txDetails) != CB_FALSE) {
		printf("NO MORE TX IN BRANCHLESS FOR TX5 ACCOUNT 1 FAIL\n");
		return 1;
	}
	CBFreeAccounterStorageTransactionCursor(cursor);
	// Check unconf transaction for account 2
	CBNewAccounterStorageTransactionCursor(&cursor, storage, CB_NO_BRANCH, account2, 1000, 5000);
	if (CBAccounterGetNextTransaction(cursor, &txDetails) != CB_TRUE) {
		printf("GET FIRST TX IN BRANCHLESS FOR TX5 ACCOUNT 2 FAIL\n");
		return 1;
	}
	if (memcmp(txDetails.accountTxDetails.addrHash, hash3, 20)) {
		printf("FIRST TX ADDR HASH FOR TX5 ACCOUNT 2 FAIL\n");
		return 1;
	}
	if (txDetails.accountTxDetails.amount != 100) {
		printf("FIRST TX AMOUNT FOR TX5 ACCOUNT 2 FAIL\n");
		return 1;
	}
	if (txDetails.timestamp != 5000){
		printf("FIRST TX TIMESTAMP FOR TX5 ACCOUNT 2 FAIL\n");
		return 1;
	}
	if (memcmp(txDetails.txHash, CBTransactionGetHash(tx5), 32)){
		printf("FIRST TX TX HASH FOR TX5 ACCOUNT 2 FAIL\n");
		return 1;
	}
	if (CBAccounterGetNextTransaction(cursor, &txDetails) != CB_FALSE) {
		printf("NO MORE TX IN BRANCHLESS FOR TX5 ACCOUNT 2 FAIL\n");
		return 1;
	}
	CBFreeAccounterStorageTransactionCursor(cursor);
	// Check cannot get any on branch 0 for account 2
	CBNewAccounterStorageTransactionCursor(&cursor, storage, 0, account2, 1000, 5000);
	if (CBAccounterGetNextTransaction(cursor, &txDetails) != CB_FALSE) {
		printf("NO MORE TX IN BRANCH 0 FOR TX5 ACCOUNT 2 FAIL\n");
		return 1;
	}
	CBFreeAccounterStorageTransactionCursor(cursor);
	// Check cannot get any unconf unspent outputs for account 1
	CBNewAccounterStorageUnspentOutputCursor(&cursor, storage, CB_NO_BRANCH, account1);
	if (CBAccounterGetNextUnspentOutput(cursor, &uoDetails) != CB_FALSE) {
		CBLogError("NO MORE UNSPENT OUTPUTS FOR TX5 ACCOUNT 1 FAIL");
		return 1;
	}
	CBFreeAccounterStorageUnspentOutputCursor(cursor);
	// Check unspent output for account 2
	CBNewAccounterStorageUnspentOutputCursor(&cursor, storage, CB_NO_BRANCH, account2);
	if (CBAccounterGetNextUnspentOutput(cursor, &uoDetails) != CB_TRUE) {
		printf("GET FIRST UNSPENT OUTPUT IN BRANCHLESS FOR TX5 ACCOUNT 2 FAIL\n");
		return 1;
	}
	if (uoDetails.value != 100) {
		printf("FIRST UNSPENT OUTPUT VALUE FOR TX5 ACCOUNT 2 FAIL\n");
		return 1;
	}
	if (memcmp(uoDetails.txHash, CBTransactionGetHash(tx5), 32)) {
		printf("FIRST UNSPENT OUTPUT TX FOR TX5 ACCOUNT 2 FAIL\n");
		return 1;
	}
	if (uoDetails.index != 0) {
		printf("FIRST UNSPENT OUTPUT INDEX FOR TX5 ACCOUNT 2 FAIL\n");
		return 1;
	}
	if (memcmp(uoDetails.watchedHash, hash3, 20)) {
		printf("FIRST UNSPENT OUTPUT ID HASH FOR TX5 ACCOUNT 2 FAIL\n");
		return 1;
	}
	if (CBAccounterGetNextUnspentOutput(cursor, &uoDetails) != CB_FALSE) {
		CBLogError("NO MORE UNSPENT OUTPUTS FOR TX5 ACCOUNT 2 FAIL\n");
		return 1;
	}
	CBFreeAccounterStorageUnspentOutputCursor(cursor);
	// Now test adding to branch
	if (! CBAccounterBranchlessTransactionToBranch(storage, tx5, 5000, 0)) {
		CBLogError("TX5 TO BRANCH 0 FAIL\n");
		return 1;
	}
	// Check balances for account 1
	if (! CBAccounterGetBranchAccountBalance(storage, CB_NO_BRANCH, account1, &balance)){
		printf("GET UNCONF BAL AFTER TX5 MOVE ACCOUNT 1 FAIL\n");
		return 1;
	}
	if (balance != 0) {
		printf("UNCONF BAL NUM AFTER TX5 MOVE ACCOUNT 1 FAIL\n");
		return 1;
	}
	if (! CBAccounterGetBranchAccountBalance(storage, 0, account1, &balance)){
		printf("GET BRANCH 0 BAL AFTER TX5 MOVE ACCOUNT 1 FAIL\n");
		return 1;
	}
	if (balance != 100) {
		printf("BRANCH 0 BAL NUM AFTER TX5 MOVE ACCOUNT 1 FAIL\n");
		return 1;
	}
	// Check balances for account 2
	if (! CBAccounterGetBranchAccountBalance(storage, CB_NO_BRANCH, account2, &balance)){
		printf("GET UNCONF BAL AFTER TX5 MOVE ACCOUNT 2 FAIL\n");
		return 1;
	}
	if (balance != 0) {
		printf("UNCONF BAL NUM AFTER TX5 MOVE ACCOUNT 2 FAIL\n");
		return 1;
	}
	if (! CBAccounterGetBranchAccountBalance(storage, 0, account2, &balance)){
		printf("GET BRANCH 0 BAL AFTER TX5 MOVE ACCOUNT 2 FAIL\n");
		return 1;
	}
	if (balance != 100) {
		printf("BRANCH 0 BAL NUM AFTER TX5 MOVE ACCOUNT 2 FAIL\n");
		return 1;
	}
	// Check getting only last transaction
	CBNewAccounterStorageTransactionCursor(&cursor, storage, 0, account1, 5000, 5000);
	if (CBAccounterGetNextTransaction(cursor, &txDetails) != CB_TRUE) {
		printf("GET ONLY LAST TX IN BRANCH 0 AFTER TX5 MOVE ACCOUNT 1 FAIL\n");
		return 1;
	}
	if (memcmp(txDetails.accountTxDetails.addrHash, hash3, 20)) {
		printf("LAST ONLY TX ADDR HASH AFTER TX5 MOVE ACCOUNT 1 FAIL\n");
		return 1;
	}
	if (txDetails.height != 5000) {
		printf("LAST ONLY TX HEIGHT AFTER TX5 MOVE ACCOUNT 1 FAIL\n");
		return 1;
	}
	if (txDetails.accountTxDetails.amount != -100) {
		printf("LAST ONLY TX AMOUNT AFTER TX5 MOVE ACCOUNT 1 FAIL\n");
		return 1;
	}
	if (txDetails.timestamp != 5000){
		printf("LAST ONLY TX TIMESTAMP AFTER TX5 MOVE ACCOUNT 1 FAIL\n");
		return 1;
	}
	if (memcmp(txDetails.txHash, CBTransactionGetHash(tx5), 32)){
		printf("LAST ONLY TX TX HASH AFTER TX5 MOVE ACCOUNT 1 FAIL\n");
		return 1;
	}
	if (CBAccounterGetNextTransaction(cursor, &txDetails) != CB_FALSE) {
		printf("NO MORE TX IN BRANCH 0 AFTER TX5 MOVE ACCOUNT 1 FAIL\n");
		return 1;
	}
	// Check getting tx for account 2
	CBNewAccounterStorageTransactionCursor(&cursor, storage, 0, account2, 5000, 5000);
	if (CBAccounterGetNextTransaction(cursor, &txDetails) != CB_TRUE) {
		printf("GET TX IN BRANCH 0 AFTER TX5 MOVE ACCOUNT 2 FAIL\n");
		return 1;
	}
	if (memcmp(txDetails.accountTxDetails.addrHash, hash3, 20)) {
		printf("TX ADDR HASH AFTER TX5 MOVE ACCOUNT 2 FAIL\n");
		return 1;
	}
	if (txDetails.height != 5000) {
		printf("LAST ONLY TX HEIGHT AFTER TX5 MOVE ACCOUNT 2 FAIL\n");
		return 1;
	}
	if (txDetails.accountTxDetails.amount != 100) {
		printf("TX AMOUNT AFTER TX5 MOVE ACCOUNT 2 FAIL\n");
		return 1;
	}
	if (txDetails.timestamp != 5000){
		printf("TX TIMESTAMP AFTER TX5 MOVE ACCOUNT 2 FAIL\n");
		return 1;
	}
	if (memcmp(txDetails.txHash, CBTransactionGetHash(tx5), 32)){
		printf("TX HASH AFTER TX5 MOVE ACCOUNT 2 FAIL\n");
		return 1;
	}
	if (CBAccounterGetNextTransaction(cursor, &txDetails) != CB_FALSE) {
		printf("NO MORE TX IN BRANCH 0 AFTER TX5 MOVE ACCOUNT 2 FAIL\n");
		return 1;
	}
	// Check getting unspent output for account 1
	CBNewAccounterStorageUnspentOutputCursor(&cursor, storage, 0, account1);
	if (CBAccounterGetNextUnspentOutput(cursor, &uoDetails) != CB_TRUE) {
		printf("GET FIRST UNSPENT OUTPUT IN BRANCH 0 AFTER TX5 MOVE ACCOUNT 1 FAIL\n");
		return 1;
	}
	if (uoDetails.value != 100) {
		printf("FIRST UNSPENT OUTPUT VALUE AFTER TX5 MOVE ACCOUNT 1 FAIL\n");
		return 1;
	}
	if (memcmp(uoDetails.txHash, CBTransactionGetHash(tx4), 32)) {
		printf("FIRST UNSPENT OUTPUT TX AFTER TX5 MOVE ACCOUNT 1 FAIL\n");
		return 1;
	}
	if (uoDetails.index != 0) {
		printf("FIRST UNSPENT OUTPUT INDEX AFTER TX5 MOVE ACCOUNT 1 FAIL\n");
		return 1;
	}
	if (memcmp(uoDetails.watchedHash, hash2, 20)) {
		printf("FIRST UNSPENT OUTPUT ID HASH AFTER TX5 MOVE ACCOUNT 1 FAIL\n");
		return 1;
	}
	if (CBAccounterGetNextUnspentOutput(cursor, &uoDetails) != CB_FALSE) {
		CBLogError("NO MORE UNSPENT OUTPUTS AFTER TX5 MOVE ACCOUNT 1 FAIL\n");
		return 1;
	}
	// Check unspent output for account 2
	CBNewAccounterStorageUnspentOutputCursor(&cursor, storage, 0, account2);
	if (CBAccounterGetNextUnspentOutput(cursor, &uoDetails) != CB_TRUE) {
		printf("GET FIRST UNSPENT OUTPUT IN BRANCH 0 AFTER TX5 MOVE ACCOUNT 2 FAIL\n");
		return 1;
	}
	if (uoDetails.value != 100) {
		printf("FIRST UNSPENT OUTPUT VALUE AFTER TX5 MOVE ACCOUNT 2 FAIL\n");
		return 1;
	}
	if (memcmp(uoDetails.txHash, CBTransactionGetHash(tx5), 32)) {
		printf("FIRST UNSPENT OUTPUT TX AFTER TX5 MOVE ACCOUNT 2 FAIL\n");
		return 1;
	}
	if (uoDetails.index != 0) {
		printf("FIRST UNSPENT OUTPUT INDEX AFTER TX5 MOVE ACCOUNT 2 FAIL\n");
		return 1;
	}
	if (memcmp(uoDetails.watchedHash, hash3, 20)) {
		printf("FIRST UNSPENT OUTPUT ID HASH AFTER TX5 MOVE ACCOUNT 2 FAIL\n");
		return 1;
	}
	if (CBAccounterGetNextUnspentOutput(cursor, &uoDetails) != CB_FALSE) {
		printf("NO MORE UNSPENT OUTPUTS AFTER TX5 MOVE ACCOUNT 2 FAIL\n");
		return 1;
	}
	CBFreeAccounterStorageUnspentOutputCursor(cursor);
	// Test creating new branch fork at height 2000 leaving only the first transaction in new branch
	if (! CBAccounterNewBranch(storage, 1, 0, 2000)) {
		printf("NEW BRANCH 1 FAIL\n");
		return 1;
	}
	// Test balance for branch 0 for account 1
	if (! CBAccounterGetBranchAccountBalance(storage, 0, account1, &balance)){
		printf("GET BRANCH 0 BAL AFTER NEW BRANCH ACCOUNT 1 FAIL\n");
		return 1;
	}
	if (balance != 100) {
		printf("BRANCH 0 BAL NUM AFTER NEW BRANCH ACCOUNT 1 FAIL\n");
		return 1;
	}
	// Test balance for branch 0 for account 2
	if (! CBAccounterGetBranchAccountBalance(storage, 0, account2, &balance)){
		printf("GET BRANCH 0 BAL AFTER NEW BRANCH ACCOUNT 2 FAIL\n");
		return 1;
	}
	if (balance != 100) {
		printf("BRANCH 0 BAL NUM AFTER NEW BRANCH ACCOUNT 2 FAIL\n");
		return 1;
	}
	// Test balance for branch 1 for account 1
	if (! CBAccounterGetBranchAccountBalance(storage, 1, account1, &balance)){
		printf("GET BRANCH 0 BAL AFTER NEW BRANCH ACCOUNT 1 FAIL\n");
		return 1;
	}
	if (balance != 400) {
		printf("BRANCH 0 BAL NUM AFTER NEW BRANCH ACCOUNT 1 FAIL\n");
		return 1;
	}
	// Test transactions remain intact in branch 0 for account 1
	CBNewAccounterStorageTransactionCursor(&cursor, storage, 0, account1, 1000, 5000);
	if (CBAccounterGetNextTransaction(cursor, &txDetails) != CB_TRUE) {
		printf("GET FIRST TX IN BRANCH 0 AFTER NEW BRANCH FAIL\n");
		return 1;
	}
	if (memcmp(txDetails.accountTxDetails.addrHash, hash1, 20)) {
		printf("FIRST TX ADDR HASH AFTER NEW BRANCH FAIL\n");
		return 1;
	}
	if (txDetails.height != 1000) {
		printf("FIRST TX HEIGHT AFTER NEW BRANCH FAIL\n");
		return 1;
	}
	if (txDetails.accountTxDetails.amount != 400) {
		printf("FIRST TX AMOUNT AFTER NEW BRANCH FAIL\n");
		return 1;
	}
	if (txDetails.timestamp != 1000){
		printf("FIRST TX TIMESTAMP AFTER NEW BRANCH FAIL\n");
		return 1;
	}
	if (memcmp(txDetails.txHash, CBTransactionGetHash(tx1), 32)){
		printf("FIRST TX TX HASH AFTER NEW BRANCH FAIL\n");
		return 1;
	}
	if (CBAccounterGetNextTransaction(cursor, &txDetails) != CB_TRUE) {
		printf("GET SECOND TX IN BRANCH 0 AFTER NEW BRANCH FAIL\n");
		return 1;
	}
	if (memcmp(txDetails.accountTxDetails.addrHash, hash1, 20)) {
		printf("SECOND TX ADDR HASH AFTER NEW BRANCH FAIL\n");
		return 1;
	}
	if (txDetails.height != 2000) {
		printf("SECOND TX HEIGHT AFTER NEW BRANCH FAIL\n");
		return 1;
	}
	if (txDetails.accountTxDetails.amount != 50) {
		printf("SECOND TX AMOUNT AFTER NEW BRANCH FAIL\n");
		return 1;
	}
	if (txDetails.timestamp != 3000){
		printf("SECOND TX TIMESTAMP AFTER NEW BRANCH FAIL\n");
		return 1;
	}
	if (memcmp(txDetails.txHash, CBTransactionGetHash(tx2), 32)){
		printf("SECOND TX TX HASH AFTER NEW BRANCH FAIL\n");
		return 1;
	}
	if (CBAccounterGetNextTransaction(cursor, &txDetails) != CB_TRUE) {
		printf("GET THIRD TX IN BRANCH 0 AFTER NEW BRANCH FAIL\n");
		return 1;
	}
	if (memcmp(txDetails.accountTxDetails.addrHash, CBByteArrayGetData(prevOutHash), 20)) {
		printf("THIRD TX ADDR HASH AFTER NEW BRANCH FAIL\n");
		return 1;
	}
	if (txDetails.height != 4000) {
		printf("THIRD TX HEIGHT AFTER NEW BRANCH FAIL\n");
		return 1;
	}
	if (txDetails.accountTxDetails.amount != -50) {
		printf("THIRD TX AMOUNT AFTER NEW BRANCH FAIL\n");
		return 1;
	}
	if (txDetails.timestamp != 4000){
		printf("THIRD TX TIMESTAMP AFTER NEW BRANCH FAIL\n");
		return 1;
	}
	if (memcmp(txDetails.txHash, CBTransactionGetHash(tx4), 32)){
		printf("THIRD TX TX HASH AFTER NEW BRANCH FAIL\n");
		return 1;
	}
	if (CBAccounterGetNextTransaction(cursor, &txDetails) != CB_TRUE) {
		printf("GET FORTH TX IN BRANCH 0 AFTER NEW BRANCH FAIL\n");
		return 1;
	}
	if (memcmp(txDetails.accountTxDetails.addrHash, CBByteArrayGetData(prevOutHash), 20)) {
		printf("FORTH TX ADDR HASH AFTER NEW BRANCH FAIL\n");
		return 1;
	}
	if (txDetails.height != 4000) {
		printf("FORTH TX HEIGHT AFTER NEW BRANCH FAIL\n");
		return 1;
	}
	if (txDetails.accountTxDetails.amount != -200) {
		printf("FORTH TX AMOUNT AFTER NEW BRANCH FAIL\n");
		return 1;
	}
	if (txDetails.timestamp != 4000){
		printf("FORTH TX TIMESTAMP AFTER NEW BRANCH FAIL\n");
		return 1;
	}
	if (memcmp(txDetails.txHash, CBTransactionGetHash(tx3), 32)){
		printf("FORTH TX TX HASH AFTER NEW BRANCH FAIL\n");
		return 1;
	}
	if (CBAccounterGetNextTransaction(cursor, &txDetails) != CB_TRUE) {
		printf("GET FIFTH TX IN BRANCH 0 AFTER NEW BRANCH FAIL\n");
		return 1;
	}
	if (memcmp(txDetails.accountTxDetails.addrHash, hash3, 20)) {
		printf("FIFTH TX ADDR HASH AFTER NEW BRANCH FAIL\n");
		return 1;
	}
	if (txDetails.height != 5000) {
		printf("FIFTH TX HEIGHT AFTER NEW BRANCH FAIL\n");
		return 1;
	}
	if (txDetails.accountTxDetails.amount != -100) {
		printf("FIFTH TX AMOUNT AFTER NEW BRANCH FAIL\n");
		return 1;
	}
	if (txDetails.timestamp != 5000){
		printf("FIFTH TX TIMESTAMP AFTER NEW BRANCH FAIL\n");
		return 1;
	}
	if (memcmp(txDetails.txHash, CBTransactionGetHash(tx5), 32)){
		printf("FIFTH TX TX HASH AFTER NEW BRANCH FAIL\n");
		return 1;
	}
	if (CBAccounterGetNextTransaction(cursor, &txDetails) != CB_FALSE) {
		printf("NO MORE TX IN BRANCH 0 AFTER NEW BRANCH FAIL\n");
		return 1;
	}
	CBFreeAccounterStorageTransactionCursor(cursor);
	// Test transactions remain intact in branch 0 for account 2
	CBNewAccounterStorageTransactionCursor(&cursor, storage, 0, account2, 1000, 5000);
	if (CBAccounterGetNextTransaction(cursor, &txDetails) != CB_TRUE) {
		printf("GET TX IN BRANCH 0 AFTER NEW BRANCH ACCOUNT 2 FAIL\n");
		return 1;
	}
	if (memcmp(txDetails.accountTxDetails.addrHash, hash3, 20)) {
		printf("TX ADDR HASH AFTER NEW BRANCH ACCOUNT 2 FAIL\n");
		return 1;
	}
	if (txDetails.height != 5000) {
		printf("TX HEIGHT AFTER NEW BRANCH ACCOUNT 2 FAIL\n");
		return 1;
	}
	if (txDetails.accountTxDetails.amount != 100) {
		printf("TX AMOUNT AFTER NEW BRANCH ACCOUNT 2 FAIL\n");
		return 1;
	}
	if (txDetails.timestamp != 5000){
		printf("TX TIMESTAMP AFTER NEW BRANCH ACCOUNT 2 FAIL\n");
		return 1;
	}
	if (memcmp(txDetails.txHash, CBTransactionGetHash(tx5), 32)){
		printf("TX TX HASH AFTER NEW BRANCH ACCOUNT 2 FAIL\n");
		return 1;
	}
	if (CBAccounterGetNextTransaction(cursor, &txDetails) != CB_FALSE) {
		printf("NO MORE TX IN BRANCH 0 AFTER NEW BRANCH ACCOUNT 2 FAIL\n");
		return 1;
	}
	CBFreeAccounterStorageTransactionCursor(cursor);
	// Test unspent output remain intact in branch 0 for account 1
	CBNewAccounterStorageUnspentOutputCursor(&cursor, storage, 0, account1);
	if (CBAccounterGetNextUnspentOutput(cursor, &uoDetails) != CB_TRUE) {
		printf("GET FIRST UNSPENT OUTPUT IN BRANCH 0 AFTER NEW BRANCH ACCOUNT 1 FAIL\n");
		return 1;
	}
	if (uoDetails.value != 100) {
		printf("FIRST UNSPENT OUTPUT VALUE AFTER NEW BRANCH ACCOUNT 1 FAIL\n");
		return 1;
	}
	if (memcmp(uoDetails.txHash, CBTransactionGetHash(tx4), 32)) {
		printf("FIRST UNSPENT OUTPUT TX AFTER NEW BRANCH ACCOUNT 1 FAIL\n");
		return 1;
	}
	if (uoDetails.index != 0) {
		printf("FIRST UNSPENT OUTPUT INDEX AFTER NEW BRANCH ACCOUNT 1 FAIL\n");
		return 1;
	}
	if (memcmp(uoDetails.watchedHash, hash2, 20)) {
		printf("FIRST UNSPENT OUTPUT ID HASH AFTER NEW BRANCH ACCOUNT 1 FAIL\n");
		return 1;
	}
	if (CBAccounterGetNextUnspentOutput(cursor, &uoDetails) != CB_FALSE) {
		CBLogError("NO MORE UNSPENT OUTPUTS AFTER NEW BRANCH ACCOUNT 1 FAIL\n");
		return 1;
	}
	// Test unspent outputs remain intact in branch 0 for account 2
	CBNewAccounterStorageUnspentOutputCursor(&cursor, storage, 0, account2);
	if (CBAccounterGetNextUnspentOutput(cursor, &uoDetails) != CB_TRUE) {
		printf("GET FIRST UNSPENT OUTPUT IN BRANCH 0 AFTER NEW BRANCH ACCOUNT 2 FAIL\n");
		return 1;
	}
	if (uoDetails.value != 100) {
		printf("FIRST UNSPENT OUTPUT VALUE AFTER NEW BRANCH ACCOUNT 2 FAIL\n");
		return 1;
	}
	if (memcmp(uoDetails.txHash, CBTransactionGetHash(tx5), 32)) {
		printf("FIRST UNSPENT OUTPUT TX AFTER NEW BRANCH ACCOUNT 2 FAIL\n");
		return 1;
	}
	if (uoDetails.index != 0) {
		printf("FIRST UNSPENT OUTPUT INDEX AFTER NEW BRANCH ACCOUNT 2 FAIL\n");
		return 1;
	}
	if (memcmp(uoDetails.watchedHash, hash3, 20)) {
		printf("FIRST UNSPENT OUTPUT ID HASH AFTER NEW BRANCH ACCOUNT 2 FAIL\n");
		return 1;
	}
	if (CBAccounterGetNextUnspentOutput(cursor, &uoDetails) != CB_FALSE) {
		printf("NO MORE UNSPENT OUTPUTS AFTER NEW BRANCH ACCOUNT 2 FAIL\n");
		return 1;
	}
	CBFreeAccounterStorageUnspentOutputCursor(cursor);
	// Test account 1 transaction in new branch.
	CBNewAccounterStorageTransactionCursor(&cursor, storage, 1, account1, 1000, 5000);
	if (CBAccounterGetNextTransaction(cursor, &txDetails) != CB_TRUE) {
		printf("GET FIRST TX IN BRANCH 1 AFTER NEW BRANCH FAIL\n");
		return 1;
	}
	if (memcmp(txDetails.accountTxDetails.addrHash, hash1, 20)) {
		printf("FIRST TX IN BRANCH 1 ADDR HASH AFTER NEW BRANCH FAIL\n");
		return 1;
	}
	if (txDetails.height != 1000) {
		printf("FIRST TX IN BRANCH 1 HEIGHT AFTER NEW BRANCH FAIL\n");
		return 1;
	}
	if (txDetails.accountTxDetails.amount != 400) {
		printf("FIRST TX IN BRANCH 1 AMOUNT AFTER NEW BRANCH FAIL\n");
		return 1;
	}
	if (txDetails.timestamp != 1000){
		printf("FIRST TX IN BRANCH 1 TIMESTAMP AFTER NEW BRANCH FAIL\n");
		return 1;
	}
	if (memcmp(txDetails.txHash, CBTransactionGetHash(tx1), 32)){
		printf("FIRST TX IN BRANCH 1 TX HASH AFTER NEW BRANCH FAIL\n");
		return 1;
	}
	if (CBAccounterGetNextTransaction(cursor, &txDetails) != CB_FALSE) {
		printf("NO MORE TX IN BRANCH 1 AFTER NEW BRANCH FAIL\n");
		return 1;
	}
	// Test account 1 unspent outputs in new branch.
	CBNewAccounterStorageUnspentOutputCursor(&cursor, storage, 1, account1);
	if (CBAccounterGetNextUnspentOutput(cursor, &uoDetails) != CB_TRUE) {
		printf("GET FIRST UNSPENT OUTPUT IN BRANCH 1 AFTER NEW BRANCH FAIL\n");
		return 1;
	}
	if (uoDetails.value != 100) {
		printf("FIRST UNSPENT OUTPUT IN BRANCH 1 VALUE AFTER NEW BRANCH FAIL\n");
		return 1;
	}
	if (memcmp(uoDetails.txHash, CBTransactionGetHash(tx1), 32)) {
		printf("FIRST UNSPENT OUTPUT IN BRANCH 1 TX AFTER NEW BRANCH FAIL\n");
		return 1;
	}
	if (uoDetails.index != 0) {
		printf("FIRST UNSPENT OUTPUT IN BRANCH 1 INDEX AFTER NEW BRANCH FAIL\n");
		return 1;
	}
	if (memcmp(uoDetails.watchedHash, hash1, 20)) {
		printf("FIRST UNSPENT OUTPUT IN BRANCH 1 ID HASH AFTER NEW BRANCH FAIL\n");
		return 1;
	}
	if (CBAccounterGetNextUnspentOutput(cursor, &uoDetails) != CB_TRUE) {
		printf("GET SECOND UNSPENT OUTPUT IN BRANCH 1 AFTER NEW BRANCH FAIL");
		return 1;
	}
	if (uoDetails.value != 300) {
		printf("SECOND UNSPENT OUTPUT IN BRANCH 1 VALUE AFTER NEW BRANCH FAIL\n");
		return 1;
	}
	if (memcmp(uoDetails.txHash, CBTransactionGetHash(tx1), 32)) {
		printf("SECOND UNSPENT OUTPUT IN BRANCH 1 TX AFTER NEW BRANCH FAIL\n");
		return 1;
	}
	if (uoDetails.index != 1) {
		printf("SECOND UNSPENT OUTPUT IN BRANCH 1 INDEX AFTER NEW BRANCH FAIL\n");
		return 1;
	}
	if (memcmp(uoDetails.watchedHash, hash1, 20)) {
		printf("SECOND UNSPENT OUTPUT IN BRANCH 1 ID HASH AFTER NEW BRANCH FAIL\n");
		return 1;
	}
	if (CBAccounterGetNextUnspentOutput(cursor, &uoDetails) != CB_FALSE) {
		CBLogError("NO MORE UNSPENT OUTPUTS IN BRANCH 1 AFTER NEW BRANCH FAIL");
		return 1;
	}
	CBFreeAccounterStorageUnspentOutputCursor(cursor);
	// Test adding two transactions to branchless
	CBTransaction * tx6 = CBNewTransaction(0, 1);
	CBTransactionTakeInput(tx6, CBNewTransactionInput(inScript, CB_TX_INPUT_FINAL, prevOutHash, 0));
	CBTransactionTakeOutput(tx6, CBNewTransactionOutput(300, outScript3));
	// Serialise transaction
	CBGetMessage(tx6)->bytes = CBNewByteArrayOfSize(CBTransactionCalculateLength(tx6));
	if (CBTransactionSerialise(tx6, false) != CBTransactionCalculateLength(tx6)){
		printf("TX6 SERAILISE FAIL\n");
		return 1;
	}
	// Find tx as branchless
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
	// Check branchless balance for account 2
	if (! CBAccounterGetBranchAccountBalance(storage, CB_NO_BRANCH, account2, &balance)){
		printf("GET UNCONF BAL AFTER FIRST OF TWO TXS ACCOUNT 2 FAIL\n");
		return 1;
	}
	if (balance != 300) {
		printf("UNCONF BAL NUM AFTER FIRST OF TWO TXS ACCOUNT 2 FAIL\n");
		return 1;
	}
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
	// Find tx as branchless
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
	// Check branchless balance for account 1
	if (! CBAccounterGetBranchAccountBalance(storage, CB_NO_BRANCH, account1, &balance)){
		printf("GET UNCONF BAL AFTER TWO TXS ACCOUNT 1 FAIL\n");
		return 1;
	}
	if (balance != 100) {
		printf("UNCONF BAL NUM AFTER TWO TXS ACCOUNT 1 FAIL\n");
		return 1;
	}
	// Check branchless balance for account 2
	if (! CBAccounterGetBranchAccountBalance(storage, CB_NO_BRANCH, account2, &balance)){
		printf("GET UNCONF BAL AFTER TWO TXS ACCOUNT 2 FAIL\n");
		return 1;
	}
	if (balance != 0) {
		printf("UNCONF BAL NUM AFTER TWO TXS ACCOUNT 2 FAIL\n");
		return 1;
	}
	// Check getting transaction for account 1
	CBNewAccounterStorageTransactionCursor(&cursor, storage, CB_NO_BRANCH, account1, 1000, 7000);
	if (CBAccounterGetNextTransaction(cursor, &txDetails) != CB_TRUE) {
		printf("GET TX IN BRANCHLESS AFTER TWO TXS FAIL\n");
		return 1;
	}
	if (memcmp(txDetails.accountTxDetails.addrHash, hash1, 20)) {
		printf("TX IN BRANCHLESS ADDR HASH AFTER TWO TXS FAIL\n");
		return 1;
	}
	if (txDetails.height != 0) {
		printf("TX IN BRANCHLESS HEIGHT AFTER TWO TXS FAIL\n");
		return 1;
	}
	if (txDetails.accountTxDetails.amount != 100) {
		printf("TX IN BRANCHLESS AMOUNT AFTER TWO TXS FAIL\n");
		return 1;
	}
	if (txDetails.timestamp != 7000){
		printf("TX IN BRANCHLESS TIMESTAMP AFTER TWO TXS FAIL\n");
		return 1;
	}
	if (memcmp(txDetails.txHash, CBTransactionGetHash(tx7), 32)){
		printf("TX IN BRANCHLESS TX HASH AFTER TWO TXS FAIL\n");
		return 1;
	}
	if (CBAccounterGetNextTransaction(cursor, &txDetails) != CB_FALSE) {
		printf("NO MORE TX IN BRANCHLESS AFTER TWO TXS FAIL\n");
		return 1;
	}
	// Check getting transaction for account 2
	CBNewAccounterStorageTransactionCursor(&cursor, storage, CB_NO_BRANCH, account2, 1000, 7000);
	if (CBAccounterGetNextTransaction(cursor, &txDetails) != CB_TRUE) {
		printf("GET FIRST TX IN BRANCHLESS AFTER TWO TXS ACCOUNT 2 FAIL\n");
		return 1;
	}
	if (memcmp(txDetails.accountTxDetails.addrHash, hash3, 20)) {
		printf("FIRST TX IN BRANCHLESS ADDR HASH AFTER TWO TXS ACCOUNT 2 FAIL\n");
		return 1;
	}
	if (txDetails.height != 0) {
		printf("FIRST TX IN BRANCHLESS HEIGHT AFTER TWO TXS ACCOUNT 2 FAIL\n");
		return 1;
	}
	if (txDetails.accountTxDetails.amount != 300) {
		printf("FIRST TX IN BRANCHLESS AMOUNT AFTER TWO TXS ACCOUNT 2 FAIL\n");
		return 1;
	}
	if (txDetails.timestamp != 6000){
		printf("FIRST TX IN BRANCHLESS TIMESTAMP AFTER TWO TXS ACCOUNT 2 FAIL\n");
		return 1;
	}
	if (memcmp(txDetails.txHash, CBTransactionGetHash(tx6), 32)){
		printf("FIRST TX IN BRANCHLESS TX HASH AFTER TWO TXS ACCOUNT 2 FAIL\n");
		return 1;
	}
	if (CBAccounterGetNextTransaction(cursor, &txDetails) != CB_TRUE) {
		printf("GET SECOND TX IN BRANCHLESS AFTER TWO TXS ACCOUNT 2 FAIL\n");
		return 1;
	}
	if (memcmp(txDetails.accountTxDetails.addrHash, hash1, 20)) {
		printf("SECOND TX IN BRANCHLESS ADDR HASH AFTER TWO TXS ACCOUNT 2 FAIL\n");
		return 1;
	}
	if (txDetails.height != 0) {
		printf("SECOND TX IN BRANCHLESS HEIGHT AFTER TWO TXS ACCOUNT 2 FAIL\n");
		return 1;
	}
	if (txDetails.accountTxDetails.amount != -300) {
		printf("SECOND TX IN BRANCHLESS AMOUNT AFTER TWO TXS ACCOUNT 2 FAIL\n");
		return 1;
	}
	if (txDetails.timestamp != 7000){
		printf("SECOND TX IN BRANCHLESS TIMESTAMP AFTER TWO TXS ACCOUNT 2 FAIL\n");
		return 1;
	}
	if (memcmp(txDetails.txHash, CBTransactionGetHash(tx7), 32)){
		printf("SECOND TX IN BRANCHLESS TX HASH AFTER TWO TXS ACCOUNT 2 FAIL\n");
		return 1;
	}
	if (CBAccounterGetNextTransaction(cursor, &txDetails) != CB_FALSE) {
		printf("NO MORE TX IN BRANCHLESS AFTER TWO TXS ACCOUNT 2 FAIL\n");
		return 1;
	}
	// Check getting unspent output
	CBNewAccounterStorageUnspentOutputCursor(&cursor, storage, CB_NO_BRANCH, account1);
	if (CBAccounterGetNextUnspentOutput(cursor, &uoDetails) != CB_TRUE) {
		printf("GET UNSPENT OUTPUT IN BRANCHLESS AFTER TWO TXS FAIL\n");
		return 1;
	}
	if (uoDetails.value != 500) {
		printf("UNSPENT OUTPUT VALUE AFTER TWO TXS FAIL\n");
		return 1;
	}
	if (memcmp(uoDetails.txHash, CBTransactionGetHash(tx7), 32)) {
		printf("UNSPENT OUTPUT TX AFTER TWO TXS FAIL\n");
		return 1;
	}
	if (uoDetails.index != 0) {
		printf("UNSPENT OUTPUT INDEX AFTER TWO TXS FAIL\n");
		return 1;
	}
	if (memcmp(uoDetails.watchedHash, hash1, 20)) {
		printf("UNSPENT OUTPUT ID HASH AFTER TWO TXS FAIL\n");
		return 1;
	}
	if (CBAccounterGetNextUnspentOutput(cursor, &uoDetails) != CB_FALSE) {
		printf("NO MORE UNSPENT OUTPUTS AFTER TWO TXS FAIL\n");
		return 1;
	}
	// Test adding the two transactions to branch 1
	if (! CBAccounterBranchlessTransactionToBranch(storage, tx6, 6000, 1)) {
		printf("TX6 TO BRANCH 1 FAIL\n");
		return 1;
	}
	if (! CBAccounterBranchlessTransactionToBranch(storage, tx7, 7000, 1)) {
		printf("TX7 TO BRANCH 1 FAIL\n");
		return 1;
	}
	for (uint8_t x = 0; x < 2; x++) {
		// Check balances
		if (! CBAccounterGetBranchAccountBalance(storage, CB_NO_BRANCH, account1, &balance)){
			printf("GET UNCONF BAL AFTER %s ACCOUNT 1 FAIL\n", (x == 0) ? "TWO TXS MOVE" : "DELETE BRANCH 0");
			return 1;
		}
		if (balance != 0) {
			printf("UNCONF BAL NUM AFTER %s ACCOUNT 1 FAIL\n", (x == 0) ? "TWO TXS MOVE" : "DELETE BRANCH 0");
			return 1;
		}
		// Check branchless balance for account 2
		if (! CBAccounterGetBranchAccountBalance(storage, CB_NO_BRANCH, account2, &balance)){
			printf("GET UNCONF BAL AFTER %s ACCOUNT 2 FAIL\n", (x == 0) ? "TWO TXS MOVE" : "DELETE BRANCH 0");
			return 1;
		}
		if (balance != 0) {
			printf("UNCONF BAL NUM AFTER %s ACCOUNT 2 FAIL\n", (x == 0) ? "TWO TXS MOVE" : "DELETE BRANCH 0");
			return 1;
		}
		if (! CBAccounterGetBranchAccountBalance(storage, 1, account1, &balance)){
			printf("GET BRANCH 1 BAL AFTER %s ACCOUNT 1 FAIL\n", (x == 0) ? "TWO TXS MOVE" : "DELETE BRANCH 0");
			return 1;
		}
		if (balance != 500) {
			printf("BRANCH 1 BAL NUM AFTER %s ACCOUNT 1 FAIL\n", (x == 0) ? "TWO TXS MOVE" : "DELETE BRANCH 0");
			return 1;
		}
		if (! CBAccounterGetBranchAccountBalance(storage, 1, account2, &balance)){
			printf("GET BRANCH 1 BAL AFTER %s ACCOUNT 2 FAIL\n", (x == 0) ? "TWO TXS MOVE" : "DELETE BRANCH 0");
			return 1;
		}
		if (balance != 0) {
			printf("BRANCH 1 BAL NUM AFTER %s ACCOUNT 2 FAIL\n", (x == 0) ? "TWO TXS MOVE" : "DELETE BRANCH 0");
			return 1;
		}
		// Check getting transactions for account 1
		CBNewAccounterStorageTransactionCursor(&cursor, storage, 1, account1, 1000, 7000);
		if (CBAccounterGetNextTransaction(cursor, &txDetails) != CB_TRUE) {
			printf("GET FIRST TX IN BRANCH 1 AFTER %s FAIL\n", (x == 0) ? "TWO TXS MOVE" : "DELETE BRANCH 0");
			return 1;
		}
		if (memcmp(txDetails.accountTxDetails.addrHash, hash1, 20)) {
			printf("FIRST TX IN BRANCH 1 ADDR HASH AFTER %s FAIL\n", (x == 0) ? "TWO TXS MOVE" : "DELETE BRANCH 0");
			return 1;
		}
		if (txDetails.height != 1000) {
			printf("FIRST TX IN BRANCH 1 HEIGHT AFTER %s FAIL\n", (x == 0) ? "TWO TXS MOVE" : "DELETE BRANCH 0");
			return 1;
		}
		if (txDetails.accountTxDetails.amount != 400) {
			printf("FIRST TX IN BRANCH 1 AMOUNT AFTER %s FAIL\n", (x == 0) ? "TWO TXS MOVE" : "DELETE BRANCH 0");
			return 1;
		}
		if (txDetails.timestamp != 1000){
			printf("FIRST TX IN BRANCH 1 TIMESTAMP AFTER %s FAIL\n", (x == 0) ? "TWO TXS MOVE" : "DELETE BRANCH 0");
			return 1;
		}
		if (memcmp(txDetails.txHash, CBTransactionGetHash(tx1), 32)){
			printf("FIRST TX IN BRANCH 1 TX HASH AFTER %s FAIL\n", (x == 0) ? "TWO TXS MOVE" : "DELETE BRANCH 0");
			return 1;
		}
		if (CBAccounterGetNextTransaction(cursor, &txDetails) != CB_TRUE) {
			printf("GET SECOND TX IN BRANCH 1 AFTER %s FAIL\n", (x == 0) ? "TWO TXS MOVE" : "DELETE BRANCH 0");
			return 1;
		}
		if (memcmp(txDetails.accountTxDetails.addrHash, hash1, 20)) {
			printf("SECOND TX IN BRANCH 1 ADDR HASH AFTER %s FAIL\n", (x == 0) ? "TWO TXS MOVE" : "DELETE BRANCH 0");
			return 1;
		}
		if (txDetails.height != 7000) {
			printf("SECOND TX IN BRANCH 1 HEIGHT AFTER %s FAIL\n", (x == 0) ? "TWO TXS MOVE" : "DELETE BRANCH 0");
			return 1;
		}
		if (txDetails.accountTxDetails.amount != 100) {
			printf("SECOND TX IN BRANCH 1 AMOUNT AFTER %s FAIL\n", (x == 0) ? "TWO TXS MOVE" : "DELETE BRANCH 0");
			return 1;
		}
		if (txDetails.timestamp != 7000){
			printf("SECOND TX IN BRANCH 1 TIMESTAMP AFTER %s FAIL\n", (x == 0) ? "TWO TXS MOVE" : "DELETE BRANCH 0");
			return 1;
		}
		if (memcmp(txDetails.txHash, CBTransactionGetHash(tx7), 32)){
			printf("SECOND TX IN BRANCH 1 TX HASH AFTER %s FAIL\n", (x == 0) ? "TWO TXS MOVE" : "DELETE BRANCH 0");
			return 1;
		}
		if (CBAccounterGetNextTransaction(cursor, &txDetails) != CB_FALSE) {
			printf("NO MORE TX IN BRANCH 1 AFTER %s FAIL\n", (x == 0) ? "TWO TXS MOVE" : "DELETE BRANCH 0");
			return 1;
		}
		// Check getting transactions for account 2
		CBNewAccounterStorageTransactionCursor(&cursor, storage, 1, account2, 1000, 7000);
		if (CBAccounterGetNextTransaction(cursor, &txDetails) != CB_TRUE) {
			printf("GET FIRST TX IN BRANCH 1 AFTER %s ACCOUNT 2 FAIL\n", (x == 0) ? "TWO TXS MOVE" : "DELETE BRANCH 0");
			return 1;
		}
		if (memcmp(txDetails.accountTxDetails.addrHash, hash3, 20)) {
			printf("FIRST TX IN BRANCH 1 ADDR HASH AFTER %s ACCOUNT 2 FAIL\n", (x == 0) ? "TWO TXS MOVE" : "DELETE BRANCH 0");
			return 1;
		}
		if (txDetails.height != 6000) {
			printf("FIRST TX IN BRANCH 1 HEIGHT AFTER %s ACCOUNT 2 FAIL\n", (x == 0) ? "TWO TXS MOVE" : "DELETE BRANCH 0");
			return 1;
		}
		if (txDetails.accountTxDetails.amount != 300) {
			printf("FIRST TX IN BRANCH 1 AMOUNT AFTER %s ACCOUNT 2 FAIL\n", (x == 0) ? "TWO TXS MOVE" : "DELETE BRANCH 0");
			return 1;
		}
		if (txDetails.timestamp != 6000){
			printf("FIRST TX IN BRANCH 1 TIMESTAMP AFTER %s ACCOUNT 2 FAIL\n", (x == 0) ? "TWO TXS MOVE" : "DELETE BRANCH 0");
			return 1;
		}
		if (memcmp(txDetails.txHash, CBTransactionGetHash(tx6), 32)){
			printf("FIRST TX IN BRANCH 1 TX HASH AFTER %s ACCOUNT 2 FAIL\n", (x == 0) ? "TWO TXS MOVE" : "DELETE BRANCH 0");
			return 1;
		}
		if (CBAccounterGetNextTransaction(cursor, &txDetails) != CB_TRUE) {
			printf("GET SECOND TX IN BRANCH 1 AFTER %s ACCOUNT 2 FAIL\n", (x == 0) ? "TWO TXS MOVE" : "DELETE BRANCH 0");
			return 1;
		}
		if (memcmp(txDetails.accountTxDetails.addrHash, hash1, 20)) {
			printf("SECOND TX IN BRANCH 1 ADDR HASH AFTER %s ACCOUNT 2 FAIL\n", (x == 0) ? "TWO TXS MOVE" : "DELETE BRANCH 0");
			return 1;
		}
		if (txDetails.height != 7000) {
			printf("SECOND TX IN BRANCH 1 HEIGHT AFTER %s ACCOUNT 2 FAIL\n", (x == 0) ? "TWO TXS MOVE" : "DELETE BRANCH 0");
			return 1;
		}
		if (txDetails.accountTxDetails.amount != -300) {
			printf("SECOND TX IN BRANCH 1 AMOUNT AFTER %s ACCOUNT 2 FAIL\n", (x == 0) ? "TWO TXS MOVE" : "DELETE BRANCH 0");
			return 1;
		}
		if (txDetails.timestamp != 7000){
			printf("SECOND TX IN BRANCH 1 TIMESTAMP AFTER %s ACCOUNT 2 FAIL\n", (x == 0) ? "TWO TXS MOVE" : "DELETE BRANCH 0");
			return 1;
		}
		if (memcmp(txDetails.txHash, CBTransactionGetHash(tx7), 32)){
			printf("SECOND TX IN BRANCH 1 TX HASH AFTER %s ACCOUNT 2 FAIL\n", (x == 0) ? "TWO TXS MOVE" : "DELETE BRANCH 0");
			return 1;
		}
		if (CBAccounterGetNextTransaction(cursor, &txDetails) != CB_FALSE) {
			printf("NO MORE TX IN BRANCH 1 AFTER %s ACCOUNT 2 FAIL\n", (x == 0) ? "TWO TXS MOVE" : "DELETE BRANCH 0");
			return 1;
		}
		// Test getting unspent output
		CBNewAccounterStorageUnspentOutputCursor(&cursor, storage, 1, account1);
		if (CBAccounterGetNextUnspentOutput(cursor, &uoDetails) != CB_TRUE) {
			printf("GET UNSPENT OUTPUT IN BRANCHLESS AFTER %s FAIL\n", (x == 0) ? "TWO TXS MOVE" : "DELETE BRANCH 0");
			return 1;
		}
		if (uoDetails.value != 500) {
			printf("UNSPENT OUTPUT VALUE AFTER %s FAIL\n", (x == 0) ? "TWO TXS MOVE" : "DELETE BRANCH 0");
			return 1;
		}
		if (memcmp(uoDetails.txHash, CBTransactionGetHash(tx7), 32)) {
			printf("UNSPENT OUTPUT TX AFTER %s FAIL\n", (x == 0) ? "TWO TXS MOVE" : "DELETE BRANCH 0");
			return 1;
		}
		if (uoDetails.index != 0) {
			printf("UNSPENT OUTPUT INDEX AFTER %s FAIL\n", (x == 0) ? "TWO TXS MOVE" : "DELETE BRANCH 0");
			return 1;
		}
		if (memcmp(uoDetails.watchedHash, hash1, 20)) {
			printf("UNSPENT OUTPUT ID HASH AFTER %s FAIL\n", (x == 0) ? "TWO TXS MOVE" : "DELETE BRANCH 0");
			return 1;
		}
		if (CBAccounterGetNextUnspentOutput(cursor, &uoDetails) != CB_FALSE) {
			printf("NO MORE UNSPENT OUTPUTS AFTER %s FAIL\n", (x == 0) ? "TWO TXS MOVE" : "DELETE BRANCH 0");
			return 1;
		}
		if (x == 0) {
			// Test deleting branch 0
			if (! CBAccounterDeleteBranch(storage, 0)) {
				printf("DELETE BRANCH 0 FAIL\n");
				return 1;
			}
		}
		// Now check branch 1 again in loop
	}
	// Test duplicate tx6 transaction (these can exist)
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
	// Test new branch seperating tx7 and second tx6 and check first tx6
	if (! CBAccounterNewBranch(storage, 0, 1, 7000)) {
		printf("NEW BRANCH TO FIRST TX6\n");
		return 1;
	}
	// Check new branch balance
	if (! CBAccounterGetBranchAccountBalance(storage, 0, account1, &balance)){
		printf("GET BRANCH 0 BAL AFTER DUP ACCOUNT 1 FAIL\n");
		return 1;
	}
	if (balance != 400) {
		printf("BRANCH 0 BAL NUM AFTER DUP ACCOUNT 1 FAIL\n");
		return 1;
	}
	if (! CBAccounterGetBranchAccountBalance(storage, 0, account2, &balance)){
		printf("GET BRANCH 0 BAL AFTER DUP ACCOUNT 2 FAIL\n");
		return 1;
	}
	if (balance != 300) {
		printf("BRANCH 0 BAL NUM AFTER DUP ACCOUNT 2 FAIL\n");
		return 1;
	}
	// Check can get duplicate and old tx6 in branch 1
	CBNewAccounterStorageTransactionCursor(&cursor, storage, 1, account2, 6000, 6000);
	if (CBAccounterGetNextTransaction(cursor, &txDetails) != CB_TRUE) {
		printf("GET FIRST TX6 IN BRANCH 1 AFTER DUP FAIL\n");
		return 1;
	}
	if (memcmp(txDetails.accountTxDetails.addrHash, hash3, 20)) {
		printf("FIRST TX6 IN BRANCH 1 ADDR HASH AFTER DUP FAIL\n");
		return 1;
	}
	if (txDetails.height != 6000) {
		printf("FIRST TX6 IN BRANCH 1 HEIGHT AFTER DUP FAIL\n");
		return 1;
	}
	if (txDetails.accountTxDetails.amount != 300) {
		printf("FIRST TX6 IN BRANCH 1 AMOUNT AFTER DUP FAIL\n");
		return 1;
	}
	if (txDetails.timestamp != 6000){
		printf("FIRST TX6 IN BRANCH 1 TIMESTAMP AFTER DUP FAIL\n");
		return 1;
	}
	if (memcmp(txDetails.txHash, CBTransactionGetHash(tx6), 32)){
		printf("FIRST TX6 IN BRANCH 1 TX HASH AFTER DUP FAIL\n");
		return 1;
	}
	if (CBAccounterGetNextTransaction(cursor, &txDetails) != CB_TRUE) {
		printf("GET SECOND TX6 IN BRANCH 1 AFTER DUP FAIL\n");
		return 1;
	}
	if (memcmp(txDetails.accountTxDetails.addrHash, hash3, 20)) {
		printf("SECOND TX6 IN BRANCH 1 ADDR HASH AFTER DUP FAIL\n");
		return 1;
	}
	if (txDetails.height != 8000) {
		printf("SECOND TX6 IN BRANCH 1 HEIGHT AFTER DUP FAIL\n");
		return 1;
	}
	if (txDetails.accountTxDetails.amount != 300) {
		printf("SECOND TX6 IN BRANCH 1 AMOUNT AFTER DUP FAIL\n");
		return 1;
	}
	if (txDetails.timestamp != 6000){
		printf("SECOND TX6 IN BRANCH 1 TIMESTAMP AFTER DUP FAIL\n");
		return 1;
	}
	if (memcmp(txDetails.txHash, CBTransactionGetHash(tx6), 32)){
		printf("SECOND TX6 IN BRANCH 1 TX HASH AFTER DUP FAIL\n");
		return 1;
	}
	if (CBAccounterGetNextTransaction(cursor, &txDetails) != CB_FALSE) {
		printf("NO MORE SECOND TX6 IN BRANCH 1 AFTER DUP FAIL\n");
		return 1;
	}
	// Check transaction for branch 0 for account 1
	CBNewAccounterStorageTransactionCursor(&cursor, storage, 0, account1, 1000, 8000);
	if (CBAccounterGetNextTransaction(cursor, &txDetails) != CB_TRUE) {
		printf("GET TX IN BRANCH 0 AFTER DUP ACCOUNT 1 FAIL\n");
		return 1;
	}
	if (memcmp(txDetails.accountTxDetails.addrHash, hash1, 20)) {
		printf("TX IN BRANCH 0 ADDR HASH AFTER DUP ACCOUNT 1 FAIL\n");
		return 1;
	}
	if (txDetails.height != 1000) {
		printf("TX IN BRANCH 0 HEIGHT AFTER DUP ACCOUNT 1 FAIL\n");
		return 1;
	}
	if (txDetails.accountTxDetails.amount != 400) {
		printf("TX IN BRANCH 0 AMOUNT AFTER DUP ACCOUNT 1 FAIL\n");
		return 1;
	}
	if (txDetails.timestamp != 1000){
		printf("TX IN BRANCH 0 TIMESTAMP AFTER DUP ACCOUNT 1 FAIL\n");
		return 1;
	}
	if (memcmp(txDetails.txHash, CBTransactionGetHash(tx1), 32)){
		printf("TX IN BRANCH 0 TX HASH AFTER DUP ACCOUNT 1 FAIL\n");
		return 1;
	}
	if (CBAccounterGetNextTransaction(cursor, &txDetails) != CB_FALSE) {
		printf("NO MORE TX IN BRANCH 0 AFTER DUP ACCOUNT 1 FAIL\n");
		return 1;
	}
	// Check transaction for branch 0 for account 2
	CBNewAccounterStorageTransactionCursor(&cursor, storage, 0, account2, 1000, 8000);
	if (CBAccounterGetNextTransaction(cursor, &txDetails) != CB_TRUE) {
		printf("GET TX IN BRANCH 0 AFTER DUP ACCOUNT 2 FAIL\n");
		return 1;
	}
	if (memcmp(txDetails.accountTxDetails.addrHash, hash3, 20)) {
		printf("TX IN BRANCH 0 ADDR HASH AFTER DUP ACCOUNT 2 FAIL\n");
		return 1;
	}
	if (txDetails.height != 6000) {
		printf("TX IN BRANCH 0 HEIGHT AFTER DUP ACCOUNT 2 FAIL\n");
		return 1;
	}
	if (txDetails.accountTxDetails.amount != 300) {
		printf("TX IN BRANCH 0 AMOUNT AFTER DUP ACCOUNT 2 FAIL\n");
		return 1;
	}
	if (txDetails.timestamp != 6000){
		printf("TX IN BRANCH 0 TIMESTAMP AFTER DUP ACCOUNT 2 FAIL\n");
		return 1;
	}
	if (memcmp(txDetails.txHash, CBTransactionGetHash(tx6), 32)){
		printf("TX IN BRANCH 0 TX HASH AFTER DUP ACCOUNT 2 FAIL\n");
		return 1;
	}
	if (CBAccounterGetNextTransaction(cursor, &txDetails) != CB_FALSE) {
		printf("NO MORE TX IN BRANCH 0 AFTER DUP ACCOUNT 2 FAIL\n");
		return 1;
	}
	// Check unspent outputs for account 1
	CBNewAccounterStorageUnspentOutputCursor(&cursor, storage, 0, account1);
	if (CBAccounterGetNextUnspentOutput(cursor, &uoDetails) != CB_TRUE) {
		printf("GET FIRST UNSPENT OUTPUT IN BRANCH 0 AFTER DUP ACCOUNT 1 FAIL\n");
		return 1;
	}
	if (uoDetails.value != 100) {
		printf("FIRST UNSPENT OUTPUT VALUE AFTER DUP ACCOUNT 1 FAIL\n");
		return 1;
	}
	if (memcmp(uoDetails.txHash, CBTransactionGetHash(tx1), 32)) {
		printf("FIRST UNSPENT OUTPUT TX AFTER DUP ACCOUNT 1 FAIL\n");
		return 1;
	}
	if (uoDetails.index != 0) {
		printf("FIRST UNSPENT OUTPUT INDEX AFTER DUP ACCOUNT 1 FAIL\n");
		return 1;
	}
	if (memcmp(uoDetails.watchedHash, hash1, 20)) {
		printf("FIRST UNSPENT OUTPUT ID HASH AFTER DUP ACCOUNT 1 FAIL\n");
		return 1;
	}
	if (CBAccounterGetNextUnspentOutput(cursor, &uoDetails) != CB_TRUE) {
		CBLogError("GET SECOND UNSPENT OUTPUT AFTER DUP ACCOUNT 1 FAIL");
		return 1;
	}
	if (uoDetails.value != 300) {
		printf("SECOND UNSPENT OUTPUT VALUE AFTER DUP ACCOUNT 1 FAIL\n");
		return 1;
	}
	if (memcmp(uoDetails.txHash, CBTransactionGetHash(tx1), 32)) {
		printf("SECOND UNSPENT OUTPUT TX AFTER DUP ACCOUNT 1 FAIL\n");
		return 1;
	}
	if (uoDetails.index != 1) {
		printf("SECOND UNSPENT OUTPUT INDEX AFTER DUP ACCOUNT 1 FAIL\n");
		return 1;
	}
	if (memcmp(uoDetails.watchedHash, hash1, 20)) {
		printf("SECOND UNSPENT OUTPUT ID HASH AFTER DUP ACCOUNT 1 FAIL\n");
		return 1;
	}
	if (CBAccounterGetNextUnspentOutput(cursor, &uoDetails) != CB_FALSE) {
		CBLogError("NO MORE UNSPENT OUTPUTS AFTER DUP ACCOUNT 1 FAIL");
		return 1;
	}
	// Get the unspent output for account 2
	CBNewAccounterStorageUnspentOutputCursor(&cursor, storage, 0, account2);
	if (CBAccounterGetNextUnspentOutput(cursor, &uoDetails) != CB_TRUE) {
		printf("GET UNSPENT OUTPUT IN BRANCH 0 AFTER DUP ACCOUNT 2 FAIL\n");
		return 1;
	}
	if (uoDetails.value != 300) {
		printf("UNSPENT OUTPUT VALUE AFTER DUP ACCOUNT 2 FAIL\n");
		return 1;
	}
	if (memcmp(uoDetails.txHash, CBTransactionGetHash(tx6), 32)) {
		printf("UNSPENT OUTPUT TX AFTER DUP ACCOUNT 2 FAIL\n");
		return 1;
	}
	if (uoDetails.index != 0) {
		printf("UNSPENT OUTPUT INDEX AFTER DUP ACCOUNT 2 FAIL\n");
		return 1;
	}
	if (memcmp(uoDetails.watchedHash, hash3, 20)) {
		printf("UNSPENT OUTPUT ID HASH AFTER DUP ACCOUNT 2 FAIL\n");
		return 1;
	}
	if (CBAccounterGetNextUnspentOutput(cursor, &uoDetails) != CB_FALSE) {
		printf("NO MORE UNSPENT OUTPUTS AFTER DUP ACCOUNT 2 FAIL\n");
		return 1;
	}
	CBFreeAccounterStorageUnspentOutputCursor(cursor);
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
	// ??? Add tests for duplicate transactions added and then removed as unconfimed? Also duplicate txs moved from branchless to a branch.
	CBReleaseObject(tx1);
	CBReleaseObject(tx2);
	CBReleaseObject(tx3);
	CBReleaseObject(tx4);
	CBReleaseObject(tx5);
	CBReleaseObject(tx6);
	CBReleaseObject(tx7);
	CBFreeAccounterStorage(storage);
	return 0;
}
