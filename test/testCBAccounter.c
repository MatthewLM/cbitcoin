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
	uint64_t storage = CBNewAccounterStorage("./");
	CBAccounter * accounter = CBNewAccounter(storage);
	// Add 6 watched output scripts to two different accounts
	for (uint8_t x = 0; x < 6; x++) {
		CBScript * script = CBNewScriptOfSize(1);
		CBByteArraySetByte(script, 0, CB_SCRIPT_OP_1 + x);
		if (NOT CBAccounterAddWatchedOutputScript(accounter, x % 2, script)) {
			printf("ADD OUTPUT SCRIPT %u FAIL\n",x);
			return 1;
		}
		CBReleaseObject(script);
	}
	// Scan three transactions, the second referencing the first and the third referencing the second which make use of the outputs
	CBTransaction * tx = CBNewTransaction(0, 1);
	CBScript * inScript = CBNewScriptWithDataCopy((uint8_t []){CB_SCRIPT_OP_1}, 1);
	CBByteArray * prevOutHash = CBNewByteArrayOfSize(32);
	memset(CBByteArrayGetData(prevOutHash), 0, 32);
	CBTransactionTakeInput(tx, CBNewTransactionInput(inScript, CB_TX_INPUT_FINAL, prevOutHash, 0));
	CBReleaseObject(prevOutHash);
	CBScript * outScript = CBNewScriptWithDataCopy((uint8_t []){CB_SCRIPT_OP_1}, 1);
	CBTransactionTakeOutput(tx, CBNewTransactionOutput(120, outScript));
	CBReleaseObject(outScript);
	outScript = CBNewScriptWithDataCopy((uint8_t []){CB_SCRIPT_OP_2}, 1);
	CBTransactionTakeOutput(tx, CBNewTransactionOutput(1089, outScript));
	CBReleaseObject(outScript);
	CBGetMessage(tx)->bytes = CBNewByteArrayOfSize(CBTransactionCalculateLength(tx));
	CBTransactionSerialise(tx, false);
	if (NOT CBAccounterFoundTransaction(accounter, tx, 1, 1361056241)) {
		printf("FOUND TX ONE FAIL\n");
		return 1;
	}
	prevOutHash = CBNewByteArrayWithDataCopy(CBTransactionGetHash(tx), 32);
	CBReleaseObject(tx);
	tx = CBNewTransaction(0, 1);
	CBTransactionTakeInput(tx, CBNewTransactionInput(inScript, CB_TX_INPUT_FINAL, prevOutHash, 0));
	outScript = CBNewScriptWithDataCopy((uint8_t []){CB_SCRIPT_OP_3}, 1);
	CBTransactionTakeOutput(tx, CBNewTransactionOutput(10, outScript));
	CBReleaseObject(outScript);
	outScript = CBNewScriptWithDataCopy((uint8_t []){CB_SCRIPT_OP_1}, 1);
	CBTransactionTakeOutput(tx, CBNewTransactionOutput(50, outScript));
	CBReleaseObject(outScript);
	CBGetMessage(tx)->bytes = CBNewByteArrayOfSize(CBTransactionCalculateLength(tx));
	CBTransactionSerialise(tx, false);
	if (NOT CBAccounterFoundTransaction(accounter, tx, 1, 1361056417)) {
		printf("FOUND TX TWO FAIL\n");
		return 1;
	}
	CBByteArray * prevOutHash2 = CBNewByteArrayWithDataCopy(CBTransactionGetHash(tx), 32);
	CBTransaction * tx3 = CBNewTransaction(0, 1);
	CBTransactionTakeInput(tx3, CBNewTransactionInput(inScript, CB_TX_INPUT_FINAL, prevOutHash, 1));
	CBTransactionTakeInput(tx3, CBNewTransactionInput(inScript, CB_TX_INPUT_FINAL, prevOutHash2, 0));
	outScript = CBNewScriptWithDataCopy((uint8_t []){CB_SCRIPT_OP_4}, 1);
	CBTransactionTakeOutput(tx3, CBNewTransactionOutput(12, outScript));
	CBReleaseObject(outScript);
	outScript = CBNewScriptWithDataCopy((uint8_t []){CB_SCRIPT_OP_5}, 1);
	CBTransactionTakeOutput(tx3, CBNewTransactionOutput(9, outScript));
	CBReleaseObject(outScript);
	CBGetMessage(tx3)->bytes = CBNewByteArrayOfSize(CBTransactionCalculateLength(tx3));
	CBTransactionSerialise(tx3, false);
	if (NOT CBAccounterFoundTransaction(accounter, tx3, 2, 1361056547)) {
		printf("FOUND TX THREE FAIL\n");
		return 1;
	}
	// Lose the last transaction
	if (NOT CBAccounterLostTransaction(accounter, tx3)) {
		printf("LOSE LAST TX FAIL\n");
		return 1;
	}
	CBReleaseObject(tx3);
	// Change the status of the second transaction
	if (NOT CBAccounterTransactionChangeStatus(accounter, tx, CB_TX_UNCONFIRMED)) {
		printf("CHANGE SECOND TX STATUS FAIL\n");
		return 1;
	}
	CBReleaseObject(tx);
	// Close accounter and storage.
	CBReleaseObject(accounter);
	CBFreeAccounterStorage(storage);
	// Reopen
	storage = CBNewAccounterStorage("./");
	accounter = CBNewAccounter(storage);
	// Check data is correct
	CBPosition pos;
	if (CBAssociativeArrayGetFirst(&accounter->accounts, &pos)) for (uint8_t x = 0;;x++) {
		CBAccount * account = pos.node->elements[pos.index];
		if (account->accountID != x) {
			printf("ACCOUNT ID FAIL\n");
			return 1;
		}
		if (account->balance != ((x == 0) ? 60 : 1089)){
			printf("ACCOUNT BALANCE FAIL\n");
			return 1;
		}
		if (account->numWatchedOutputScripts != 3) {
			printf("ACCOUNT NUM WATCHED OUTPUT SCRIPTS FAIL\n");
			return 1;
		}
		CBPosition pos2;
		if (CBAssociativeArrayGetFirst(&account->orderedTransactionsByTime, &pos2)) for (uint8_t y = 0;;y++) {
			CBTransactionDetails * details = pos2.node->elements[pos2.index];
			if (details->accountID != x) {
				printf("TIME ORDERED TX DETAILS ACCOUNT ID FAIL\n");
				return 1;
			}
			if (details->blockHeight != ((y == 0) ? 1 : CB_TX_UNCONFIRMED)) {
				printf("TIME ORDERED TX DETAILS BLOCK HEIGHT FAIL\n");
				return 1;
			}
			if (memcmp(details->hash, CBByteArrayGetData((y == 0) ? prevOutHash : prevOutHash2), 32)) {
				printf("TIME ORDERED TX DETAILS HASH FAIL\n");
				return 1;
			}
			if (details->timestamp != ((y == 0) ? 1361056241 : 1361056417)) {
				printf("TIME ORDERED TX DETAILS TIMESTAMP FAIL\n");
				return 1;
			}
			if (details->value != ((x == 0) ? ((y == 0) ? 120 : -60) : 1089)) {
				printf("TIME ORDERED TX DETAILS VALUE FAIL\n");
				return 1;
			}
			if (CBAssociativeArrayIterate(&account->orderedTransactionsByTime, &pos2)){
				if (y != 1 - x) {
					printf("NUM TIME ORDERED TXS FAIL\n");
					return 1;
				}
				break;
			}
		}
		if (CBAssociativeArrayGetFirst(&account->unspentOutputs, &pos2)) for (uint8_t y = 0;;y++) {
			CBOutputRef * out = pos2.node->elements[pos2.index];
			if (out->accountID != x) {
				printf("UNSPENT OUTPUT ACCOUNT ID FAIL\n");
				return 1;
			}
			if (memcmp(out->txHash, CBByteArrayGetData((x == 1) ? prevOutHash : prevOutHash2), 32)) {
				printf("UNSPENT OUTPUT HASH FAIL\n");
				return 1;
			}
			if (out->txIndex != ((x == 0) ? y : 1)) {
				printf("UNSPENT OUTPUT INDEX FAIL\n");
				return 1;
			}
			if (out->value != ((x == 0) ? ((y == 0) ? 10 : 50) : 1089)) {
				printf("UNSPENT OUTPUT VALUE FAIL\n");
				return 1;
			}
			if (CBAssociativeArrayIterate(&account->orderedTransactionsByTime, &pos2)){
				if (y != 1 - x) {
					printf("NUM TIME ORDERED TXS FAIL\n");
					return 1;
				}
				break;
			}
		}
		if (CBAssociativeArrayIterate(&accounter->accounts, &pos)){
			if (x != 1) {
				printf("NUM ACCOUNTS FAIL\n");
				return 1;
			}
			break;
		}
	}
	if (CBAssociativeArrayGetFirst(&accounter->orderedSpentOutputsByHashAndIndex, &pos)) for (uint8_t x = 0;;x++) {
		CBOutputRef * outRef = pos.node->elements[pos.index];
		if (outRef->accountID != 0) {
			printf("SPENT OUTPUT ACCOUNT ID FAIL\n");
			return 1;
		}
		if (memcmp(outRef->txHash, CBByteArrayGetData(prevOutHash), 32)) {
			printf("SPENT OUTPUT TX HASH FAIL\n");
			return 1;
		}
		if (outRef->txIndex != 0) {
			printf("SPENT OUTPUT TX INDEX FAIL\n");
			return 1;
		}
		if (outRef->value != 120) {
			printf("SPENT OUTPUT VALUE FAIL\n");
			return 1;
		}
		if (CBAssociativeArrayIterate(&accounter->orderedSpentOutputsByHashAndIndex, &pos)){
			if (x != 0) {
				printf("NUM SPENT OUTPUTS FAIL\n");
				return 1;
			}
			break;
		}
	}
	if (CBAssociativeArrayGetFirst(&accounter->orderedUnspentOutputsByHashAndIndex, &pos)) for (uint8_t x = 0;;x++) {
		CBOutputRef * outRef = pos.node->elements[pos.index];
		if (outRef->accountID != ((x == 2) ? 1 : 0)) {
			printf("UNSPENT OUTPUT ACCOUNT ID FAIL\n");
			return 1;
		}
		if (memcmp(outRef->txHash, CBByteArrayGetData((x == 2) ? prevOutHash : prevOutHash2), 32)) {
			printf("UNSPENT OUTPUT TX HASH FAIL\n");
			return 1;
		}
		if (outRef->txIndex != ((x == 0) ? 0 : 1 )) {
			printf("UNSPENT OUTPUT TX INDEX FAIL\n");
			return 1;
		}
		if (outRef->value != ((x == 0) ? 10 : ((x == 1) ? 50 : 1089))) {
			printf("UNSPENT OUTPUT VALUE FAIL\n");
			return 1;
		}
		if (CBAssociativeArrayIterate(&accounter->orderedSpentOutputsByHashAndIndex, &pos)){
			if (x != 2) {
				printf("NUM UNSPENT OUTPUTS FAIL\n");
				return 1;
			}
			break;
		}
	}
	if (CBAssociativeArrayGetFirst(&accounter->orderedTransactionsByHash, &pos)) for (uint8_t x = 0;;x++) {
		CBTransactionDetails * details = pos.node->elements[pos.index];
		if (details->accountID != ((x > 1) ? 1 : 0)) {
			printf("TX BY HASH ACCOUNT ID FAIL\n");
			return 1;
		}
		if (details->blockHeight != ((x == 0) ? CB_TX_UNCONFIRMED : 1)) {
			printf("TX BY HASH BLOCK HEIGHT FAIL\n");
			return 1;
		}
		if (memcmp(details->hash, CBByteArrayGetData((x == 0) ? prevOutHash2 : prevOutHash), 32)) {
			printf("TX BY HASH HASH FAIL\n");
			return 1;
		}
		if (details->timestamp != ((x == 0) ? 1361056417 : 1361056241)) {
			printf("TX BY HASH TIMESTAMP FAIL\n");
			return 1;
		}
		if (details->value != ((x == 0) ? -60 : ((x == 1) ? 120 : 1089))) {
			printf("TX BY HASH TIMESTAMP FAIL\n");
			return 1;
		}
		if (CBAssociativeArrayIterate(&accounter->orderedSpentOutputsByHashAndIndex, &pos)){
			if (x != 2) {
				printf("NUM TXS BY HASH FAIL\n");
				return 1;
			}
			break;
		}
	}
	if (CBAssociativeArrayGetFirst(&accounter->orderedWatchedOutputScripts, &pos)) for (uint8_t x = 0;;x++) {
		CBWatchedOutputScripts * outScript = pos.node->elements[pos.index];
		if (outScript->accountID != x % 2) {
			printf("WATCHED OUTPUT ACCOUNT ID FAIL\n");
			return 1;
		}
		if (CBByteArrayGetByte(outScript->script, 0) != CB_SCRIPT_OP_1 + x) {
			printf("WATCHED OUTPUT SCRIPT FAIL\n");
			return 1;
		}
		if (CBAssociativeArrayIterate(&accounter->orderedSpentOutputsByHashAndIndex, &pos)){
			if (x != 5) {
				printf("NUM WATCHED OUTPUTS FAIL\n");
				return 1;
			}
			break;
		}
	}
	// Free objects
	CBReleaseObject(prevOutHash);
	CBReleaseObject(prevOutHash2);
	CBReleaseObject(accounter);
	CBFreeAccounterStorage(storage);
	// Now test two accounts sharing same output
	remove("./acnt_log.dat");
	remove("./acnt_0.dat");
	remove("./acnt_1.dat");
	remove("./acnt_2.dat");
	storage = CBNewAccounterStorage("./");
	accounter = CBNewAccounter(storage);
	CBScript * script = CBNewScriptOfSize(1);
	CBByteArraySetByte(script, 0, CB_SCRIPT_OP_TRUE);
	if (NOT CBAccounterAddWatchedOutputScript(accounter, 0, script)) {
		printf("ADD OUTPUT SCRIPT SHARE 0 FAIL\n");
		return 1;
	}
	if (NOT CBAccounterAddWatchedOutputScript(accounter, 1, script)) {
		printf("ADD OUTPUT SCRIPT SHARE 1 FAIL\n");
		return 1;
	}
	tx = CBNewTransaction(0, 1);
	inScript = CBNewScriptWithDataCopy((uint8_t []){CB_SCRIPT_OP_1}, 1);
	prevOutHash = CBNewByteArrayOfSize(32);
	memset(CBByteArrayGetData(prevOutHash), 0, 32);
	CBTransactionTakeInput(tx, CBNewTransactionInput(inScript, CB_TX_INPUT_FINAL, prevOutHash, 0));
	CBReleaseObject(prevOutHash);
	outScript = CBNewScriptWithDataCopy((uint8_t []){CB_SCRIPT_OP_TRUE}, 1);
	CBTransactionTakeOutput(tx, CBNewTransactionOutput(666, outScript));
	CBReleaseObject(outScript);
	outScript = CBNewScriptWithDataCopy((uint8_t []){CB_SCRIPT_OP_TRUE}, 1);
	CBTransactionTakeOutput(tx, CBNewTransactionOutput(999, outScript));
	CBReleaseObject(outScript);
	CBGetMessage(tx)->bytes = CBNewByteArrayOfSize(CBTransactionCalculateLength(tx));
	CBTransactionSerialise(tx, false);
	if (NOT CBAccounterFoundTransaction(accounter, tx, 2, 1361315893)) {
		printf("FOUND TX SHARE ONE FAIL\n");
		return 1;
	}
	prevOutHash = CBNewByteArrayWithDataCopy(CBTransactionGetHash(tx), 32);
	CBReleaseObject(tx);
	tx = CBNewTransaction(0, 1);
	CBTransactionTakeInput(tx, CBNewTransactionInput(inScript, CB_TX_INPUT_FINAL, prevOutHash, 1));
	outScript = CBNewScriptWithDataCopy((uint8_t []){CB_SCRIPT_OP_TRUE}, 1);
	CBTransactionTakeOutput(tx, CBNewTransactionOutput(333, outScript));
	CBReleaseObject(outScript);
	CBGetMessage(tx)->bytes = CBNewByteArrayOfSize(CBTransactionCalculateLength(tx));
	CBTransactionSerialise(tx, false);
	if (NOT CBAccounterFoundTransaction(accounter, tx, 5, 1361316135)) {
		printf("FOUND TX SHARE TWO FAIL\n");
		return 1;
	}
	prevOutHash2 = CBNewByteArrayWithDataCopy(CBTransactionGetHash(tx), 32);
	// Close accounter and storage.
	CBReleaseObject(accounter);
	CBFreeAccounterStorage(storage);
	// Reopen
	storage = CBNewAccounterStorage("./");
	accounter = CBNewAccounter(storage);
	// Check data is correct
	if (CBAssociativeArrayGetFirst(&accounter->accounts, &pos)) for (uint8_t x = 0;;x++) {
		CBAccount * account = pos.node->elements[pos.index];
		if (account->accountID != x) {
			printf("ACCOUNT ID SHARE FAIL\n");
			return 1;
		}
		if (account->balance != 999){
			printf("ACCOUNT BALANCE SHARE FAIL\n");
			return 1;
		}
		if (account->numWatchedOutputScripts != 1) {
			printf("ACCOUNT NUM WATCHED OUTPUT SCRIPTS SHARE FAIL\n");
			return 1;
		}
		CBPosition pos2;
		if (CBAssociativeArrayGetFirst(&account->orderedTransactionsByTime, &pos2)) for (uint8_t y = 0;;y++) {
			CBTransactionDetails * details = pos2.node->elements[pos2.index];
			if (details->accountID != x) {
				printf("TIME ORDERED TX DETAILS ACCOUNT ID SHARE FAIL\n");
				return 1;
			}
			if (details->blockHeight != ((y == 0) ? 2 : 5)) {
				printf("TIME ORDERED TX DETAILS BLOCK HEIGHT SHARE FAIL\n");
				return 1;
			}
			if (memcmp(details->hash, CBByteArrayGetData((y == 0) ? prevOutHash : prevOutHash2), 32)) {
				printf("TIME ORDERED TX DETAILS HASH SHARE FAIL\n");
				return 1;
			}
			if (details->timestamp != ((y == 0) ? 1361315893 : 1361316135)) {
				printf("TIME ORDERED TX DETAILS TIMESTAMP SHARE FAIL\n");
				return 1;
			}
			if (details->value != ((y == 0) ? 1665 : -666)) {
				printf("TIME ORDERED TX DETAILS VALUE SHARE FAIL\n");
				return 1;
			}
			if (CBAssociativeArrayIterate(&account->orderedTransactionsByTime, &pos2)){
				if (y != 1) {
					printf("NUM TIME ORDERED TXS SHARE FAIL\n");
					return 1;
				}
				break;
			}
		}
		if (CBAssociativeArrayGetFirst(&account->unspentOutputs, &pos2)) for (uint8_t y = 0;;y++) {
			CBOutputRef * out = pos2.node->elements[pos2.index];
			if (out->accountID != x) {
				printf("UNSPENT OUTPUT ACCOUNT ID SHARE FAIL\n");
				return 1;
			}
			if (memcmp(out->txHash, CBByteArrayGetData((y == 1) ? prevOutHash : prevOutHash2), 32)) {
				printf("UNSPENT OUTPUT HASH SHARE FAIL\n");
				return 1;
			}
			if (out->txIndex != 0) {
				printf("UNSPENT OUTPUT INDEX SHARE FAIL\n");
				return 1;
			}
			if (out->value != ((y == 0) ? 333 : 666)) {
				printf("UNSPENT OUTPUT VALUE SHARE FAIL\n");
				return 1;
			}
			if (CBAssociativeArrayIterate(&account->orderedTransactionsByTime, &pos2)){
				if (y != 1) {
					printf("NUM TIME ORDERED TXS SHARE FAIL\n");
					return 1;
				}
				break;
			}
		}
		if (CBAssociativeArrayIterate(&accounter->accounts, &pos)){
			if (x != 1) {
				printf("NUM ACCOUNTS SHARE FAIL\n");
				return 1;
			}
			break;
		}
	}
	if (CBAssociativeArrayGetFirst(&accounter->orderedSpentOutputsByHashAndIndex, &pos)) for (uint8_t x = 0;;x++) {
		CBOutputRef * outRef = pos.node->elements[pos.index];
		if (outRef->accountID != x) {
			printf("SPENT OUTPUT ACCOUNT ID SHARE FAIL\n");
			return 1;
		}
		if (memcmp(outRef->txHash, CBByteArrayGetData(prevOutHash), 32)) {
			printf("SPENT OUTPUT TX HASH SHARE FAIL\n");
			return 1;
		}
		if (outRef->txIndex != 1) {
			printf("SPENT OUTPUT TX INDEX SHARE FAIL\n");
			return 1;
		}
		if (outRef->value != 999) {
			printf("SPENT OUTPUT VALUE SHARE FAIL\n");
			return 1;
		}
		if (CBAssociativeArrayIterate(&accounter->orderedSpentOutputsByHashAndIndex, &pos)){
			if (x != 1) {
				printf("NUM SPENT OUTPUTS SHARE FAIL\n");
				return 1;
			}
			break;
		}
	}
	if (CBAssociativeArrayGetFirst(&accounter->orderedUnspentOutputsByHashAndIndex, &pos)) for (uint8_t x = 0;;x++) {
		CBOutputRef * outRef = pos.node->elements[pos.index];
		if (outRef->accountID != x % 2) {
			printf("UNSPENT OUTPUT ACCOUNT ID SHARE FAIL\n");
			return 1;
		}
		if (memcmp(outRef->txHash, CBByteArrayGetData((x > 1) ? prevOutHash : prevOutHash2), 32)) {
			printf("UNSPENT OUTPUT TX HASH SHARE FAIL\n");
			return 1;
		}
		if (outRef->txIndex != 0) {
			printf("UNSPENT OUTPUT TX INDEX SHARE FAIL\n");
			return 1;
		}
		if (outRef->value != ((x > 1) ? 666 : 333)) {
			printf("UNSPENT OUTPUT VALUE SHARE FAIL\n");
			return 1;
		}
		if (CBAssociativeArrayIterate(&accounter->orderedSpentOutputsByHashAndIndex, &pos)){
			if (x != 3) {
				printf("NUM UNSPENT OUTPUTS SHARE FAIL\n");
				return 1;
			}
			break;
		}
	}
	if (CBAssociativeArrayGetFirst(&accounter->orderedTransactionsByHash, &pos)) for (uint8_t x = 0;;x++) {
		CBTransactionDetails * details = pos.node->elements[pos.index];
		if (details->accountID != x % 2) {
			printf("TX BY HASH ACCOUNT ID SHARE FAIL\n");
			return 1;
		}
		if (details->blockHeight != ((x > 1) ? 2 : 5)) {
			printf("TX BY HASH BLOCK HEIGHT SHARE FAIL\n");
			return 1;
		}
		if (memcmp(details->hash, CBByteArrayGetData((x > 1) ? prevOutHash : prevOutHash2), 32)) {
			printf("TX BY HASH HASH SHARE FAIL\n");
			return 1;
		}
		if (details->timestamp != ((x > 1) ? 1361315893 : 1361316135)) {
			printf("TX BY HASH TIMESTAMP SHARE FAIL\n");
			return 1;
		}
		if (details->value != ((x > 1) ? 1665 : -666)) {
			printf("TX BY HASH VALUE SHARE FAIL\n");
			return 1;
		}
		if (CBAssociativeArrayIterate(&accounter->orderedSpentOutputsByHashAndIndex, &pos)){
			if (x != 3) {
				printf("NUM TXS BY HASH SHARE FAIL\n");
				return 1;
			}
			break;
		}
	}
	if (CBAssociativeArrayGetFirst(&accounter->orderedWatchedOutputScripts, &pos)) for (uint8_t x = 0;;x++) {
		CBWatchedOutputScripts * outScript = pos.node->elements[pos.index];
		if (outScript->accountID != x % 2) {
			printf("WATCHED OUTPUT ACCOUNT ID SHARE FAIL\n");
			return 1;
		}
		if (CBByteArrayGetByte(outScript->script, 0) != CB_SCRIPT_OP_TRUE) {
			printf("WATCHED OUTPUT SCRIPT SHARE FAIL\n");
			return 1;
		}
		if (CBAssociativeArrayIterate(&accounter->orderedSpentOutputsByHashAndIndex, &pos)){
			if (x != 1) {
				printf("NUM WATCHED OUTPUTS SHARE FAIL\n");
				return 1;
			}
			break;
		}
	}
	// Test losing a transaction
	if (NOT CBAccounterLostTransaction(accounter, tx)) {
		printf("LOST SECOND SHARE FAIL\n");
		return 1;
	}
	// Close accounter and storage.
	CBReleaseObject(accounter);
	CBFreeAccounterStorage(storage);
	// Reopen
	storage = CBNewAccounterStorage("./");
	accounter = CBNewAccounter(storage);
	// Check data is correct
	if (CBAssociativeArrayGetFirst(&accounter->accounts, &pos)) for (uint8_t x = 0;;x++) {
		CBAccount * account = pos.node->elements[pos.index];
		if (account->accountID != x) {
			printf("ACCOUNT ID SHARE LOST FAIL\n");
			return 1;
		}
		if (account->balance != 1665){
			printf("ACCOUNT BALANCE SHARE LOST FAIL\n");
			return 1;
		}
		if (account->numWatchedOutputScripts != 1) {
			printf("ACCOUNT NUM WATCHED OUTPUT SCRIPTS SHARE LOST FAIL\n");
			return 1;
		}
		CBPosition pos2;
		if (CBAssociativeArrayGetFirst(&account->orderedTransactionsByTime, &pos2)) for (uint8_t y = 0;;y++) {
			CBTransactionDetails * details = pos2.node->elements[pos2.index];
			if (details->accountID != x) {
				printf("TIME ORDERED TX DETAILS ACCOUNT ID SHARE LOST FAIL\n");
				return 1;
			}
			if (details->blockHeight != 2) {
				printf("TIME ORDERED TX DETAILS BLOCK HEIGHT SHARE LOST FAIL\n");
				return 1;
			}
			if (memcmp(details->hash, CBByteArrayGetData(prevOutHash), 32)) {
				printf("TIME ORDERED TX DETAILS HASH SHARE LOST FAIL\n");
				return 1;
			}
			if (details->timestamp != 1361315893) {
				printf("TIME ORDERED TX DETAILS TIMESTAMP SHARE LOST FAIL\n");
				return 1;
			}
			if (details->value != 1665) {
				printf("TIME ORDERED TX DETAILS VALUE SHARE LOST FAIL\n");
				return 1;
			}
			if (CBAssociativeArrayIterate(&account->orderedTransactionsByTime, &pos2)){
				if (y != 0) {
					printf("NUM TIME ORDERED TXS SHARE LOST FAIL\n");
					return 1;
				}
				break;
			}
		}
		if (CBAssociativeArrayGetFirst(&account->unspentOutputs, &pos2)) for (uint8_t y = 0;;y++) {
			CBOutputRef * out = pos2.node->elements[pos2.index];
			if (out->accountID != x) {
				printf("UNSPENT OUTPUT ACCOUNT ID SHARE LOST FAIL\n");
				return 1;
			}
			if (memcmp(out->txHash, CBByteArrayGetData(prevOutHash), 32)) {
				printf("UNSPENT OUTPUT HASH SHARE LOST FAIL\n");
				return 1;
			}
			if (out->txIndex != y) {
				printf("UNSPENT OUTPUT INDEX SHARE LOST FAIL\n");
				return 1;
			}
			if (out->value != ((y == 0) ? 666 : 999)) {
				printf("UNSPENT OUTPUT VALUE SHARE LOST FAIL\n");
				return 1;
			}
			if (CBAssociativeArrayIterate(&account->orderedTransactionsByTime, &pos2)){
				if (y != 1) {
					printf("NUM TIME ORDERED TXS SHARE LOST FAIL\n");
					return 1;
				}
				break;
			}
		}
		if (CBAssociativeArrayIterate(&accounter->accounts, &pos)){
			if (x != 1) {
				printf("NUM ACCOUNTS SHARE LOST FAIL\n");
				return 1;
			}
			break;
		}
	}
	if (CBAssociativeArrayGetFirst(&accounter->orderedUnspentOutputsByHashAndIndex, &pos)) for (uint8_t x = 0;;x++) {
		CBOutputRef * outRef = pos.node->elements[pos.index];
		if (outRef->accountID != x % 2) {
			printf("UNSPENT OUTPUT ACCOUNT ID SHARE LOST FAIL\n");
			return 1;
		}
		if (memcmp(outRef->txHash, CBByteArrayGetData(prevOutHash), 32)) {
			printf("UNSPENT OUTPUT TX HASH SHARE LOST FAIL\n");
			return 1;
		}
		if (outRef->txIndex != x / 2) {
			printf("UNSPENT OUTPUT TX INDEX SHARE LOST FAIL\n");
			return 1;
		}
		if (outRef->value != ((x > 1) ? 999 : 666)) {
			printf("UNSPENT OUTPUT VALUE SHARE LOST FAIL\n");
			return 1;
		}
		if (CBAssociativeArrayIterate(&accounter->orderedSpentOutputsByHashAndIndex, &pos)){
			if (x != 3) {
				printf("NUM UNSPENT OUTPUTS SHARE LOST FAIL\n");
				return 1;
			}
			break;
		}
	}
	if (CBAssociativeArrayGetFirst(&accounter->orderedTransactionsByHash, &pos)) for (uint8_t x = 0;;x++) {
		CBTransactionDetails * details = pos.node->elements[pos.index];
		if (details->accountID != x) {
			printf("TX BY HASH ACCOUNT ID SHARE LOST FAIL\n");
			return 1;
		}
		if (details->blockHeight != 2) {
			printf("TX BY HASH BLOCK HEIGHT SHARE LOST FAIL\n");
			return 1;
		}
		if (memcmp(details->hash, CBByteArrayGetData(prevOutHash), 32)) {
			printf("TX BY HASH HASH SHARE LOST FAIL\n");
			return 1;
		}
		if (details->timestamp != 1361315893) {
			printf("TX BY HASH TIMESTAMP SHARE LOST FAIL\n");
			return 1;
		}
		if (details->value != 1665) {
			printf("TX BY HASH VALUE SHARE LOST FAIL\n");
			return 1;
		}
		if (CBAssociativeArrayIterate(&accounter->orderedSpentOutputsByHashAndIndex, &pos)){
			if (x != 1) {
				printf("NUM TXS BY HASH SHARE LOST FAIL\n");
				return 1;
			}
			break;
		}
	}
	CBReleaseObject(tx);
	// Free objects
	CBReleaseObject(prevOutHash);
	CBReleaseObject(prevOutHash2);
	CBReleaseObject(accounter);
	CBFreeAccounterStorage(storage);
	CBReleaseObject(script);
	return 0;
}
