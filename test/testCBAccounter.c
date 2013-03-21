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
#include "CBAccounter.h"
#include <time.h>

void CBLogError(char * b, ...);
void CBLogError(char * b, ...){
	printf("%s\n", b);
	exit(EXIT_FAILURE);
}

int main(){
	remove("./acnt_log.dat");
	remove("./acnt_0.dat");
	remove("./acnt_1.dat");
	remove("./acnt_2.dat");
	uint64_t accounter = CBNewAccounter("./");
	// Get an account
	uint64_t account1 = CBAccounterNewAccount(accounter);
	if (account1 != 1) {
		printf("NEW ACCOUNT FAIL\n");
		return 1;
	}
	// Add a watched hash
	uint8_t hash[20];
	memset(hash, 1, 20);
	if (NOT CBAccounterAddWatchedOutputToAccount(accounter, hash, account1)) {
		printf("ADD WATCHED OUTPUT FAIL");
		return 1;
	}
	// Create a transaction with two said watched hash and an output without it.
	CBTransaction * tx1 = CBNewTransaction(0, 1);
	CBScript * inScript = CBNewScriptWithDataCopy((uint8_t []){CB_SCRIPT_OP_TRUE}, 1);
	CBByteArray * prevOutHash = CBNewScriptOfSize(32);
	memset(CBByteArrayGetData(prevOutHash), 0, 32);
	CBTransactionTakeInput(tx1, CBNewTransactionInput(inScript, CB_TX_INPUT_FINAL, prevOutHash, 0));
	CBScript * outScript = CBNewScriptOfSize(25);
	CBByteArraySetByte(outScript, 0, CB_SCRIPT_OP_DUP);
	CBByteArraySetByte(outScript, 1, CB_SCRIPT_OP_HASH160);
	CBByteArraySetByte(outScript, 2, 0x14);
	memset(CBByteArrayGetData(outScript) + 3, 1, 20);
	CBByteArraySetByte(outScript, 23, CB_SCRIPT_OP_EQUALVERIFY);
	CBByteArraySetByte(outScript, 24, CB_SCRIPT_OP_CHECKSIG);
	CBTransactionTakeOutput(tx1, CBNewTransactionOutput(100, outScript));
	CBTransactionTakeOutput(tx1, CBNewTransactionOutput(300, outScript));
	CBTransactionTakeOutput(tx1, CBNewTransactionOutputTakeScript(200, CBNewScriptWithDataCopy((uint8_t []){CB_SCRIPT_OP_TRUE}, 1)));
	// Serialise transaction
	CBGetMessage(tx1)->bytes = CBNewByteArrayOfSize(CBTransactionCalculateLength(tx1));
	if (CBTransactionSerialise(tx1, false) != CBTransactionCalculateLength(tx1)){
		printf("TX1 SERAILISE FAIL\n");
		return 1;
	}
	// Find the transaction in branch index 0
	//WATCHED OUTPUT FOUND IN WRITE VALUES ARRAY. ADD DATABASE FUNCTIONS TO SEARCH DATABASE. MIGHT AS WELL AS DISK INDEXING BEFOREHAND;
	if (NOT CBAccounterFoundTransaction(accounter, tx1, 1000, 1362264283, 0)) {
		printf("FOUND TX FAIL\n");
		return 1;
	}
	// Check obtaining the transaction details
	CBTransactionDetails txDetails;
	uint64_t txCursor = 0;
	if (CBAccounterGetFirstTransactionBetween(accounter, 0, account1, 1362264283, 1362264283, &txCursor, &txDetails) != CB_GET_TX_OK) {
		printf("GET FIRST TX FAIL\n");
		return 1;
	}
	if (memcmp(txDetails.addrHash, hash, 20)) {
		printf("FOUND TX ADDR HASH FAIL\n");
		return 1;
	}
	if (txDetails.amount != 400) {
		printf("FOUND TX AMOUNT FAIL\n");
		return 1;
	}
	// Check obtaining the account balance
	// Check obtaining the unspent output
	return 0;
}
