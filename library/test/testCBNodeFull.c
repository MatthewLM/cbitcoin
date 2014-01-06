//
//  testCBNodeFull.c
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

// ??? Add tests for invalid blocks, perhaps seperately.

#include <stdio.h>
#include "CBNodeFull.h"
#include "CBLibEventSockets.h"
#include <event2/thread.h>
#include <time.h>
#include "stdarg.h"
#include <sys/stat.h>
#include "CBDependencies.h"
#include "CBHDKeys.h"
#include "checkTransactions.h"

enum{
	CREATING_BLOCKS,
	RECEIVE_INITIAL_BLOCKS_AND_TXS,
	CHAIN_REORGANISATION,
	LOSE_CHAIN_AND_RELAY,
	ORPHAN_TO_CHAIN
} testPhase = CREATING_BLOCKS;

enum{
	NODE1_GOT = 1,
	NODE2_GOT = 2,
	NODE1_FORK = 4,
	COMPLETE_RECEIVE_INITAL = 7
} receiveInitialBlocksAndTxs = 0;

enum{
	NODE0_REORG = 1,
	NODE1_REORG = 2,
	NODE2_REORG = 4,
	COMPLETE_CHAIN_REORGANISATION = 7
} chainReorg = 0;

int gotTxNum = 0;
int doubleSpendNum = 0;
int confirmedNum = 0;
int unconfirmedNum = 0;

bool nodeOwns[3][14] = {
	{0, 0, 1, 0, 1, 0, 1, 1, 0, 0, 1, 0, 1, 1},
	{1, 0, 0, 1, 1, 1, 0, 1, 1, 0, 0, 1, 1, 0},
	{0, 1, 0, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 1},
};

CBNodeFull * nodes[3];
CBKeyPair keys[3];
CBTransaction * initialTxs[14];
CBTransaction * doubleSpends[6]; // Inc. dependants of double spend
CBTransaction * chainDoubleSpend;
CBTransaction * orphanSpendOtherBranch;

CBBlock * lastInitialBlock;
CBByteArray * lastNode1BlockHash;
CBByteArray * reorgTestBlockHash;
CBDepObject testMutex;
CBPeer * dummyPeer;

uint64_t CBGetMilliseconds(void){
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

void onFatalNodeError(CBNode * node, CBErrorReason reason);
void onFatalNodeError(CBNode * node, CBErrorReason reason){
	CBLogError("ON FATAL NODE ERROR %u\n", reason);
	exit(EXIT_FAILURE);
}

void onBadTime(void * foo);
void onBadTime(void * foo){
	CBLogError("BAD TIME FAIL\n");
	exit(EXIT_FAILURE);
}

void stop(void * comm);
void stop(void * comm){
	CBNetworkCommunicatorStop(comm);
	CBReleaseObject(comm);
}

void maybeFinishOrphanTest(void * foo);
void maybeFinishOrphanTest(void * foo){
	if (chainReorg == COMPLETE_CHAIN_REORGANISATION && gotTxNum == 5 && doubleSpendNum == 2 && confirmedNum == 4) {
		// Ensure validation is complete for second node, so that all double spends are processed
		CBMutexLock(CBGetNode(nodes[1])->blockAndTxMutex);
		CBMutexUnlock(CBGetNode(nodes[1])->blockAndTxMutex);
		for (uint8_t x = 0; x < 3; x++) {
			CBNodeFull * node = nodes[x];
			if (CBGetNetworkCommunicator(node)->blockHeight != 1004) {
				CBLogError("OPRHAN TO CHAIN FINISH BLOCK HEIGHT FAIL\n");
				exit(EXIT_FAILURE);
			}
			for (uint8_t y = 4; y < 14; y++) {
				CBFoundTransaction * fndTx = CBNodeFullGetFoundTransaction(node,  CBTransactionGetHash(initialTxs[y]));
				if ((y == 4 && x == 2) || y == 5 || y == 7 || y == 11 || y == 13){
					if (fndTx != NULL) {
						CBLogError("OPRHAN TO CHAIN CONFIRMED TX %u FOUND AS UNCONFIRMED NODE %u", y, x);
						exit(EXIT_FAILURE);
					}
					continue;
				}
				if (fndTx == NULL) {
					if (nodeOwns[x][y]) {
						CBLogError("OPRHAN TO CHAIN UNCONFIRMED TX NOT FOUND");
						exit(EXIT_FAILURE);
					}
					break;
				}
				if (fndTx->utx.numUnconfDeps != (y == 11 || y == 13)) {
					CBLogError("OPRHAN TO CHAIN FINISH NUM UNCONF DEPS FAIL\n");
					exit(EXIT_FAILURE);
				}
				if ((fndTx->utx.type == CB_TX_OTHER) == nodeOwns[x][y]) {
					CBLogError("OPRHAN TO CHAIN FINISH OWNERSHIP FAIL\n");
					exit(EXIT_FAILURE);
				}
			}
			for (uint8_t y = 0; y < 6; y++) {
				if (CBNodeFullGetFoundTransaction(node,  CBTransactionGetHash(doubleSpends[y])) != NULL) {
					CBLogError("OPRHAN TO CHAIN FINISH TX DOUBLE SPEND IS UNCONFIRMED FAIL\n");
					exit(EXIT_FAILURE);
				}
			}
			// Make sure orphan no longer exists as unconfirmed
			if (CBNodeFullGetFoundTransaction(node, CBTransactionGetHash(orphanSpendOtherBranch)) != NULL) {
				CBLogError("OPRHAN TO CHAIN STILL UNCONFIRMED FAIL");
				exit(EXIT_FAILURE);
			}
		}
		// Check accounter information (has all initial transactions except for 11 and 13, double spends and the orphan we added) Only bother with node 0
		CBDepObject cursor;
		if (! CBNewAccounterStorageTransactionCursor(&cursor, CBGetNode(nodes[0])->accounterStorage, 1, 0, UINT64_MAX)) {
			printf("CREATE CURSOR FAIL\n");
			exit(EXIT_FAILURE);
		}
		uint8_t zeroHash[20];
		memset(zeroHash, 0, 20);
		uint64_t times[5];
		for (uint8_t x = 0; x < 5; x++)
			CBAccounterGetTransactionTime(CBGetNode(nodes[0])->accounterStorage, CBTransactionGetHash(initialTxs[(int []){4,6,7,10,12}[x]]), times + x);
		checkTransactions(cursor, (CBTestTxDetails [13]){
			/* 0 */ {CBKeyPairGetHash(keys), 1003, 1250000000, 1231471165, CBTransactionGetHash(initialTxs[2]),1250000000,0},
			/* 1 */ {CBKeyPairGetHash(keys), 1001, 312500000, 1231471166, CBTransactionGetHash(doubleSpends[0]),1562500000,0},
			/* 2 */ {zeroHash, 1001, 0, 1231471166, CBTransactionGetHash(doubleSpends[1]),1562500000,0},
			/* 3 */ {zeroHash, 1001, 0, 1231471166, CBTransactionGetHash(doubleSpends[2]),1562500000,0},
			/* 4 */ {CBKeyPairGetHash(keys), 1002, 312500000, 1231471167, CBTransactionGetHash(doubleSpends[3]),1875000000,0},
			/* 5 */ {zeroHash, 1002, 0, 1231471167, CBTransactionGetHash(doubleSpends[4]),1875000000,0},
			/* 6 */ {zeroHash, 1002, 0, 1231471167, CBTransactionGetHash(doubleSpends[5]),1875000000,0},
			/* 7 */ {zeroHash, 1004, 0, 1231471169, CBTransactionGetHash(orphanSpendOtherBranch),1875000000,0},
			// Timestamps should be after the fixed timestamps.
			/* 8 */ {CBKeyPairGetHash(keys), CB_UNCONFIRMED, 312500000, times[0], CBTransactionGetHash(initialTxs[4]),2187500000,312500000},
			/* 9 */ {CBKeyPairGetHash(keys + 2), CB_UNCONFIRMED, -312500000, times[1], CBTransactionGetHash(initialTxs[6]),1875000000,0},
			/* 10 */ {CBKeyPairGetHash(keys), 1002, 312500000, times[2], CBTransactionGetHash(initialTxs[7]),2187500000,0},
			/* 11 */ {CBKeyPairGetHash(keys), CB_UNCONFIRMED, 0, times[3], CBTransactionGetHash(initialTxs[10]),2187500000,0},
			/* 12 */ {CBKeyPairGetHash(keys + 2), CB_UNCONFIRMED, -156250000, times[4], CBTransactionGetHash(initialTxs[12]),2031250000,-156250000},
		}, 13);
		int64_t ubalance;
		uint64_t cbalance;
		if (! CBAccounterGetAccountBalance(CBGetNode(nodes[0])->accounterStorage, 1, &cbalance, &ubalance)){
			printf("GET UNCONF BALANCE FAIL\n");
			exit(EXIT_FAILURE);
		}
		if (ubalance != -156250000) {
			printf("UNCONF BALANCE FAIL %lli != -156250000\n", ubalance);
			exit(EXIT_FAILURE);
		}
		if (cbalance != 2031250000) {
			printf("ALL BALANCE FAIL %llu != 312500000\n", cbalance);
			exit(EXIT_FAILURE);
		}
		CBLogVerbose("OPRHAN TO CHAIN complete.");
		// Disconnect nodes
		CBLogVerbose("STOPPING NODES");
		CBRunOnEventLoop(CBGetNetworkCommunicator(nodes[0])->eventLoop, stop, nodes[0], false);
		CBRunOnEventLoop(CBGetNetworkCommunicator(nodes[1])->eventLoop, stop, nodes[1], false);
		CBRunOnEventLoop(CBGetNetworkCommunicator(nodes[2])->eventLoop, stop, nodes[2], false);
	}
}

void maybeFinishLoseTest(void);
void maybeFinishLoseTest(void){
	if (chainReorg == COMPLETE_CHAIN_REORGANISATION && gotTxNum == 2 && doubleSpendNum == 4 && unconfirmedNum == 4 && confirmedNum == 2) {
		for (uint8_t x = 0; x < 3; x++) {
			CBNodeFull * node = nodes[x];
			if (CBGetNetworkCommunicator(node)->blockHeight != 1003) {
				CBLogError("LOSE CHAIN AND RELAY FINISH BLOCK HEIGHT FAIL\n");
				exit(EXIT_FAILURE);
			}
			for (uint8_t y = 4; y < 14; y++) {
				CBFoundTransaction * fndTx = CBNodeFullGetFoundTransaction(node,  CBTransactionGetHash(initialTxs[y]));
				if ((y == 4 && x == 2) || y == 5 || y == 7 || y == 11 || y == 13){
					if (fndTx != NULL) {
						CBLogError("LOSE CHAIN AND RELAY CONFIRMED TX %u FOUND AS UNCONFIRMED NODE %u", y, x);
						exit(EXIT_FAILURE);
					}
					continue;
				}
				if (fndTx == NULL) {
					if (nodeOwns[x][y]) {
						CBLogError("LOSE CHAIN AND RELAY UNCONFIRMED TX NOT FOUND");
						exit(EXIT_FAILURE);
					}
					break;
				}
				if (fndTx->utx.numUnconfDeps != (y == 11 || y == 13)) {
					CBLogError("LOSE CHAIN AND RELAY FINISH NUM UNCONF DEPS FAIL\n");
					exit(EXIT_FAILURE);
				}
				if ((fndTx->utx.type == CB_TX_OTHER) == nodeOwns[x][y]) {
					CBLogError("LOSE CHAIN AND RELAY FINISH OWNERSHIP FAIL\n");
					exit(EXIT_FAILURE);
				}
			}
			for (uint8_t y = 0; y < 6; y++) {
				CBFoundTransaction * fndTx = CBNodeFullGetFoundTransaction(node,  CBTransactionGetHash(doubleSpends[y]));
				if ((fndTx == NULL) == (y > 2)) {
					CBLogError("LOSE CHAIN AND RELAY FINISH TX DOUBLE SPEND IS UNCONFIRMED FAIL\n");
					exit(EXIT_FAILURE);
				}
				if (fndTx != NULL){
					// These are the unconfirmed ones
					if (fndTx->utx.numUnconfDeps != y > 3) {
						CBLogError("LOSE CHAIN AND RELAY FINISH DOUBLE SPEND NUM UNCONF DEPS FAIL %u != %u > 3 (node = %u)\n", fndTx->utx.numUnconfDeps, y, x);
						exit(EXIT_FAILURE);
					}
					if ((fndTx->utx.type == CB_TX_OTHER) != (x == 2 || (x == 1 && y > 3))) {
						CBLogError("LOSE CHAIN AND RELAY FINISH DOUBLE SPEND OWNERSHIP FAIL\n");
						exit(EXIT_FAILURE);
					}
				}
			}
		}
		// Check accounter information. Only bother with node 0
		CBDepObject cursor;
		if (! CBNewAccounterStorageTransactionCursor(&cursor, CBGetNode(nodes[0])->accounterStorage, 1, 0, UINT64_MAX)) {
			printf("LOSE CHAIN AND RELAY CREATE CURSOR FAIL\n");
			exit(EXIT_FAILURE);
		}
		uint8_t zeroHash[20];
		memset(zeroHash, 0, 20);
		uint64_t times[5];
		for (uint8_t x = 0; x < 5; x++)
			CBAccounterGetTransactionTime(CBGetNode(nodes[0])->accounterStorage, CBTransactionGetHash(initialTxs[(int []){4,6,7,10,12}[x]]), times + x);
		checkTransactions(cursor, (CBTestTxDetails [9]){
			/* 0 */ {CBKeyPairGetHash(keys), 1001, 1250000000, 1231471165, CBTransactionGetHash(initialTxs[2]),1250000000,0},
			/* 1 */ {CBKeyPairGetHash(keys), CB_UNCONFIRMED, 312500000, 1231471167, CBTransactionGetHash(doubleSpends[3]),1562500000,312500000},
			/* 2 */ {zeroHash, CB_UNCONFIRMED, 0, 1231471167, CBTransactionGetHash(doubleSpends[4]),1562500000,312500000},
			/* 3 */ {zeroHash, CB_UNCONFIRMED, 0, 1231471167, CBTransactionGetHash(doubleSpends[5]),1562500000,312500000},
			/* 4 */ {CBKeyPairGetHash(keys), CB_UNCONFIRMED, 312500000, times[0], CBTransactionGetHash(initialTxs[4]),1875000000,625000000},
			/* 5 */ {CBKeyPairGetHash(keys + 2), CB_UNCONFIRMED, -312500000, times[1], CBTransactionGetHash(initialTxs[6]),1562500000,312500000},
			/* 6 */ {CBKeyPairGetHash(keys), 1003, 312500000, times[2], CBTransactionGetHash(initialTxs[7]),1875000000,312500000},
			/* 7 */ {CBKeyPairGetHash(keys), CB_UNCONFIRMED, 0, times[3], CBTransactionGetHash(initialTxs[10]),1875000000,312500000},
			/* 8 */ {CBKeyPairGetHash(keys + 2), CB_UNCONFIRMED, -156250000, times[4], CBTransactionGetHash(initialTxs[12]),1718750000,156250000},
		}, 9);
		int64_t ubalance;
		uint64_t cbalance;
		if (! CBAccounterGetAccountBalance(CBGetNode(nodes[0])->accounterStorage, 1, &cbalance, &ubalance)){
			printf("LOSE CHAIN AND RELAY GET UNCONF BALANCE FAIL\n");
			exit(EXIT_FAILURE);
		}
		if (ubalance != 156250000) {
			printf("INITIAL UNCONF BALANCE FAIL %lli != 625000000\n", ubalance);
			exit(EXIT_FAILURE);
		}
		if (cbalance != 1718750000) {
			printf("INITIAL ALL BALANCE FAIL %llu != 1875000000\n", cbalance);
			exit(EXIT_FAILURE);
		}
		CBLogVerbose("LOSE_CHAIN_AND_RELAY complete.");
		gotTxNum = 0;
		chainReorg = 0;
		confirmedNum = 0;
		doubleSpendNum = 0;
		dummyPeer->nodeObj = nodes[0];
		// Test giving orphan transaction to node 0, and then giving a false dependency.
		CBTransaction * falseTx = CBNewTransaction(19, 1);
		CBGetMessage(falseTx)->type = CB_MESSAGE_TYPE_TX;
		CBByteArray * prev = CBNewByteArrayWithDataCopy(CBTransactionGetHash(initialTxs[0]), 32);
		CBTransactionTakeInput(falseTx, CBNewTransactionInput(NULL, CB_TX_INPUT_FINAL, prev, 0));
		CBReleaseObject(prev);
		CBScript * script = CBNewScriptPubKeyHashOutput(CBKeyPairGetHash(keys + 1));
		CBTransactionTakeOutput(falseTx, CBNewTransactionOutput(312500000, script));
		CBTransactionSignPubKeyHashInput(falseTx, keys + 1, initialTxs[0]->outputs[0]->scriptObject, 0, CB_SIGHASH_ALL);
		CBTransactionMakeBytes(falseTx);
		CBTransactionSerialise(falseTx, true);
		CBTransaction * orphan = CBNewTransaction(20, 1);
		CBGetMessage(orphan)->type = CB_MESSAGE_TYPE_TX;
		prev = CBNewByteArrayWithDataCopy(CBTransactionGetHash(initialTxs[4]), 32);
		CBTransactionTakeInput(orphan, CBNewTransactionInput(NULL, CB_TX_INPUT_FINAL, prev, 1));
		CBReleaseObject(prev);
		prev = CBNewByteArrayWithDataCopy(CBTransactionGetHash(falseTx), 32);
		CBTransactionTakeInput(orphan, CBNewTransactionInput(NULL, CB_TX_INPUT_FINAL, prev, 0));
		CBReleaseObject(prev);
		CBTransactionTakeOutput(orphan, CBNewTransactionOutput(312500000, script));
		CBReleaseObject(script);
		CBTransactionSignPubKeyHashInput(orphan, keys, initialTxs[4]->outputs[0]->scriptObject, 0, CB_SIGHASH_ALL);
		CBTransactionSignPubKeyHashInput(orphan, keys + 1, falseTx->outputs[0]->scriptObject, 1, CB_SIGHASH_ALL);
		CBTransactionMakeBytes(orphan);
		CBTransactionSerialise(orphan, true);
		// Get some peer for node 0
		CBPosition pos;
		CBAssociativeArrayGetFirst(&CBGetNetworkCommunicator(nodes[0])->addresses->peers, &pos);
		CBPeer * peer = pos.node->elements[pos.index];
		if (CBNodeFullNewUnconfirmedTransaction(nodes[0], peer, orphan) != CB_TRUE) {
			CBLogError("PROCESS ORPHAN ERROR");
			exit(EXIT_FAILURE);
		}
		if (CBNodeFullNewUnconfirmedTransaction(nodes[0], peer, falseTx) != CB_TRUE){
			CBLogError("PROCESS FALSE TX ERROR");
			exit(EXIT_FAILURE);
		}
		// Test both transactions not being stored as they are invalid.
		if (CBNodeFullGetAnyTransaction(nodes[0], CBTransactionGetHash(falseTx)) != NULL) {
			CBLogError("HAVE FALSE TX ERROR");
			exit(EXIT_FAILURE);
		}
		if (CBNodeFullGetAnyTransaction(nodes[0], CBTransactionGetHash(orphan)) != NULL) {
			CBLogError("HAVE ORPHAN TX ERROR");
			exit(EXIT_FAILURE);
		}
		CBReleaseObject(orphan);
		CBReleaseObject(falseTx);
		// Test processing transaction spending transaction (3) on the chain which has been spent on the chain (by 7)
		falseTx = CBNewTransaction(21, 1);
		CBGetMessage(falseTx)->type = CB_MESSAGE_TYPE_TX;
		prev = CBNewByteArrayWithDataCopy(CBTransactionGetHash(initialTxs[3]), 32);
		CBTransactionTakeInput(falseTx, CBNewTransactionInput(NULL, CB_TX_INPUT_FINAL, prev, 1));
		CBReleaseObject(prev);
		script = CBNewScriptPubKeyHashOutput(CBKeyPairGetHash(keys));
		CBTransactionTakeOutput(falseTx, CBNewTransactionOutput(312500000, script));
		CBTransactionSignPubKeyHashInput(falseTx, keys + 1, initialTxs[0]->outputs[0]->scriptObject, 0, CB_SIGHASH_ALL);
		CBTransactionMakeBytes(falseTx);
		CBTransactionSerialise(falseTx, true);
		if (CBNodeFullNewUnconfirmedTransaction(nodes[0], peer, falseTx) != CB_TRUE){
			CBLogError("PROCESS FALSE TX 2 ERROR");
			exit(EXIT_FAILURE);
		}
		if (CBNodeFullGetAnyTransaction(nodes[0], CBTransactionGetHash(falseTx)) != NULL) {
			CBLogError("HAVE FALSE TX 2 ERROR");
			exit(EXIT_FAILURE);
		}
		// Test a transaction which spends an output on the other branch.
		orphanSpendOtherBranch = CBNewTransaction(21, 1);
		prev = CBNewByteArrayWithDataCopy(CBTransactionGetHash(doubleSpends[2]), 32);
		CBTransactionTakeInput(orphanSpendOtherBranch, CBNewTransactionInput(NULL, CB_TX_INPUT_FINAL, prev, 0));
		CBReleaseObject(prev);
		script = CBNewScriptPubKeyHashOutput(CBKeyPairGetHash(keys));
		CBTransactionTakeOutput(orphanSpendOtherBranch, CBNewTransactionOutput(312500000, script));
		CBTransactionSignPubKeyHashInput(orphanSpendOtherBranch, keys, doubleSpends[2]->outputs[0]->scriptObject, 0, CB_SIGHASH_ALL);
		CBTransactionMakeBytes(orphanSpendOtherBranch);
		CBTransactionSerialise(orphanSpendOtherBranch, true);
		CBGetMessage(orphanSpendOtherBranch)->type = CB_MESSAGE_TYPE_TX;
		if (CBNodeFullNewUnconfirmedTransaction(nodes[0], peer, orphanSpendOtherBranch) != CB_TRUE){
			CBLogError("PROCESS TX SPENDING OTHER BRANCH ERROR");
			exit(EXIT_FAILURE);
		}
		// Check it is orphan
		if (CBNodeFullGetAnyTransaction(nodes[0], CBTransactionGetHash(orphanSpendOtherBranch))->type != CB_TX_ORPHAN) {
			CBLogError("TX SPENDING OTHER BRANCH NOT ORPHAN");
			exit(EXIT_FAILURE);
		}
		// Now create reorg with the orphan on the other branch
		testPhase = ORPHAN_TO_CHAIN;
		CBBlock * block = CBNewBlock();
		block->prevBlockHash = reorgTestBlockHash;
		block->time = 1231471168;
		block->target = CB_MAX_TARGET;
		block->nonce = 2573394689;
		block->transactionNum = 3;
		block->transactions = malloc(sizeof(*block->transactions) * 3);
		// Coinbase
		block->transactions[0] = CBNewTransaction(0, 1);
		CBByteArray * nullHash = CBNewByteArrayOfSize(32);
		memset(CBByteArrayGetData(nullHash), 0, 32);
		script = CBNewScriptWithDataCopy((uint8_t []){0xdf,5}, 2);
		CBTransactionTakeInput(block->transactions[0], CBNewTransactionInput(script, CB_TX_INPUT_FINAL, nullHash, 0xFFFFFFFF));
		CBReleaseObject(script);
		script = CBNewScriptWithDataCopy((uint8_t []){CB_SCRIPT_OP_TRUE}, 1);
		CBTransactionTakeOutput(block->transactions[0], CBNewTransactionOutput(1250000000, script));
		CBReleaseObject(script);
		CBTransactionMakeBytes(block->transactions[0]);
		CBTransactionSerialise(block->transactions[0], false);
		// Add transactions 0 and 2
		block->transactions[1] = initialTxs[0];
		CBRetainObject(initialTxs[0]);
		block->transactions[2] = initialTxs[2];
		CBRetainObject(initialTxs[2]);
		CBBlockCalculateAndSetMerkleRoot(block);
		CBGetMessage(block)->bytes = CBNewByteArrayOfSize(CBBlockCalculateLength(block, true));
		CBBlockSerialise(block, true, false);
		// Add to node 0
		CBBlockPeers * blockPeers = malloc(sizeof(*blockPeers));
		memcpy(blockPeers->hash, CBBlockGetHash(block), 20);
		CBInitAssociativeArray(&blockPeers->peers, CBPtrCompare, NULL, NULL);
		CBAssociativeArrayInsert(&nodes[0]->blockPeers, blockPeers, CBAssociativeArrayFind(&nodes[0]->blockPeers, blockPeers).position, NULL);
		CBAssociativeArrayInsert(&nodes[0]->askedForBlocks, blockPeers, CBAssociativeArrayFind(&nodes[0]->askedForBlocks, blockPeers).position, NULL);
		CBValidatorQueueBlock(CBGetNode(nodes[0])->validator, block, dummyPeer);
		// The next block will contain the orphan
		CBBlock * block2 = CBNewBlock();
		block2->prevBlockHash = CBNewByteArrayWithDataCopy(CBBlockGetHash(block), 32);
		CBReleaseObject(block);
		block2->time = 1231471169;
		block2->target = CB_MAX_TARGET;
		block2->nonce = 2573394690;
		block2->transactionNum = 2;
		block2->transactions = malloc(sizeof(*block2->transactions) * 2);
		// Coinbase
		block2->transactions[0] = CBNewTransaction(0, 1);
		memset(CBByteArrayGetData(nullHash), 0, 32);
		script = CBNewScriptWithDataCopy((uint8_t []){0xe0,5}, 2);
		CBTransactionTakeInput(block2->transactions[0], CBNewTransactionInput(script, CB_TX_INPUT_FINAL, nullHash, 0xFFFFFFFF));
		CBReleaseObject(script);
		CBReleaseObject(nullHash);
		script = CBNewScriptWithDataCopy((uint8_t []){CB_SCRIPT_OP_TRUE}, 1);
		CBTransactionTakeOutput(block2->transactions[0], CBNewTransactionOutput(1250000000, script));
		CBReleaseObject(script);
		CBTransactionMakeBytes(block2->transactions[0]);
		CBTransactionSerialise(block2->transactions[0], false);
		// Add orphan
		block2->transactions[1] = orphanSpendOtherBranch;
		CBRetainObject(orphanSpendOtherBranch);
		CBBlockCalculateAndSetMerkleRoot(block2);
		CBGetMessage(block2)->bytes = CBNewByteArrayOfSize(CBBlockCalculateLength(block2, true));
		CBBlockSerialise(block2, true, true);
		// Add to node 0
		blockPeers = malloc(sizeof(*blockPeers));
		memcpy(blockPeers->hash, CBBlockGetHash(block2), 20);
		CBInitAssociativeArray(&blockPeers->peers, CBPtrCompare, NULL, NULL);
		CBAssociativeArrayInsert(&nodes[0]->blockPeers, blockPeers, CBAssociativeArrayFind(&nodes[0]->blockPeers, blockPeers).position, NULL);
		CBAssociativeArrayInsert(&nodes[0]->askedForBlocks, blockPeers, CBAssociativeArrayFind(&nodes[0]->askedForBlocks, blockPeers).position, NULL);
		CBValidatorQueueBlock(CBGetNode(nodes[0])->validator, block2, dummyPeer);
		CBReleaseObject(block2);
	}
}

void maybeFinishReorgTest(void);
void maybeFinishReorgTest(void){
	if (chainReorg == COMPLETE_CHAIN_REORGANISATION && gotTxNum == 8 && doubleSpendNum == 4 && confirmedNum == 4 && unconfirmedNum == 2) {
		for (uint8_t x = 0; x < 3; x++) {
			CBNodeFull * node = nodes[x];
			if (CBGetNetworkCommunicator(node)->blockHeight != 1002) {
				CBLogError("CHAIN REORGANISATION FINISH BLOCK HEIGHT FAIL\n");
				exit(EXIT_FAILURE);
			}
			for (uint8_t y = 0; y < 14; y++) {
				CBFoundTransaction * fndTx = CBNodeFullGetFoundTransaction(node,  CBTransactionGetHash(initialTxs[y]));
				if (y == 1 || y == 3 || y == 5 || y == 7 || y == 11 || y == 13) {
					if (fndTx != NULL) {
						CBLogError("CHAIN REORGANISATION CONFIRMED TX FOUND AS UNCONFIRMED");
						exit(EXIT_FAILURE);
					}
					continue;
				}
				if (fndTx == NULL) {
					if (nodeOwns[x][y]) {
						CBLogError("CHAIN REORGANISATION UNCONFIRMED TX NOT FOUND");
						exit(EXIT_FAILURE);
					}
					continue;
				}
				if (fndTx->utx.numUnconfDeps != (y % 2 == 0 && y > 3) + (y == 11 || y == 13)) {
					CBLogError("CHAIN REORGANISATION FINISH NUM UNCONF DEPS FAIL\n");
					exit(EXIT_FAILURE);
				}
				if ((fndTx->utx.type == CB_TX_OURS) != nodeOwns[x][y]) {
					CBLogError("CHAIN REORGANISATION FINISH OWNERSHIP FAIL\n");
					exit(EXIT_FAILURE);
				}
			}
			for (uint8_t y = 0; y < 6; y++) {
				CBFoundTransaction * fndTx = CBNodeFullGetFoundTransaction(node,  CBTransactionGetHash(doubleSpends[y]));
				if (fndTx != NULL) {
					CBLogError("CHAIN REORGANISATION FINISH TX DOUBLE SPEND IS UNCONFIRMED FAIL\n");
					exit(EXIT_FAILURE);
				}
			}
		}
		// Check accounter information (has all initial transactions except for 11 and 13 and double spends) Only bother with node 0
		CBDepObject cursor;
		if (! CBNewAccounterStorageTransactionCursor(&cursor, CBGetNode(nodes[0])->accounterStorage, 1, 0, UINT64_MAX)) {
			printf("CHAIN REORGANISATION CREATE CURSOR FAIL\n");
			exit(EXIT_FAILURE);
		}
		uint8_t zeroHash[20];
		memset(zeroHash, 0, 20);
		uint64_t times[5];
		for (uint8_t x = 0; x < 5; x++)
			CBAccounterGetTransactionTime(CBGetNode(nodes[0])->accounterStorage, CBTransactionGetHash(initialTxs[(int []){4,6,7,10,12}[x]]), times + x);
		checkTransactions(cursor, (CBTestTxDetails [12]){
			/* 0 */ {CBKeyPairGetHash(keys), CB_UNCONFIRMED, 1250000000, 1231471165, CBTransactionGetHash(initialTxs[2]),1250000000,1250000000},
			/* 1 */ {CBKeyPairGetHash(keys), 1001, 312500000, 1231471166, CBTransactionGetHash(doubleSpends[0]),1562500000,1250000000},
			/* 2 */ {zeroHash, 1001, 0, 1231471166, CBTransactionGetHash(doubleSpends[1]),1562500000,1250000000},
			/* 3 */ {zeroHash, 1001, 0, 1231471166, CBTransactionGetHash(doubleSpends[2]),1562500000,1250000000},
			/* 4 */ {CBKeyPairGetHash(keys), 1002, 312500000, 1231471167, CBTransactionGetHash(doubleSpends[3]),1875000000,1250000000},
			/* 5 */ {zeroHash, 1002, 0, 1231471167, CBTransactionGetHash(doubleSpends[4]),1875000000,1250000000},
			/* 6 */ {zeroHash, 1002, 0, 1231471167, CBTransactionGetHash(doubleSpends[5]),1875000000,1250000000},
			// Timestamps should be after the fixed timestamps.
			/* 7 */ {CBKeyPairGetHash(keys), CB_UNCONFIRMED, 312500000, times[0], CBTransactionGetHash(initialTxs[4]),2187500000,1562500000},
			/* 8 */ {CBKeyPairGetHash(keys + 2), CB_UNCONFIRMED, -312500000, times[1], CBTransactionGetHash(initialTxs[6]),1875000000,1250000000},
			/* 9 */ {CBKeyPairGetHash(keys), 1002, 312500000, times[2], CBTransactionGetHash(initialTxs[7]),2187500000,1250000000},
			/* 10 */ {CBKeyPairGetHash(keys), CB_UNCONFIRMED, 0, times[3], CBTransactionGetHash(initialTxs[10]),2187500000,1250000000},
			/* 11 */ {CBKeyPairGetHash(keys + 2), CB_UNCONFIRMED, -156250000, times[4], CBTransactionGetHash(initialTxs[12]),2031250000,1093750000},
		}, 12);
		int64_t ubalance;
		uint64_t cbalance;
		if (! CBAccounterGetAccountBalance(CBGetNode(nodes[0])->accounterStorage, 1, &cbalance, &ubalance)) {
			printf("CHAIN REORGANISATION GET CONF BALANCE FAIL\n");
			exit(EXIT_FAILURE);
		}
		if (ubalance != 1093750000) {
			printf("CHAIN REORGANISATION UNCONF BALANCE FAIL %lli != 1093750000\n", ubalance);
			exit(EXIT_FAILURE);
		}
		if (cbalance != 2031250000) {
			printf("CHAIN REORGANISATION ALL BALANCE FAIL %llu != 2031250000\n", cbalance);
			exit(EXIT_FAILURE);
		}
		CBLogVerbose("CHAIN_REORGANISATION complete.");
		testPhase = LOSE_CHAIN_AND_RELAY;
		chainReorg = 0;
		gotTxNum = 0;
		unconfirmedNum = 0;
		confirmedNum = 0;
		doubleSpendNum = 0;
		dummyPeer->nodeObj = nodes[2];
		// Test double spending one of the transactions on the chain with two dependants on the chain.
		// Test unconfirming a transaction with two dependants, and that are not dependencies of any unconfirmed transactions
		// Test re-relaying unconfirmed transactions and losing some due to oldness.
		// Relay transaction 8 again
		CBFoundTransaction * tx8 = CBNodeFullGetFoundTransaction(nodes[2], CBTransactionGetHash(initialTxs[8]));
		tx8->nextRelay = CBGetMilliseconds() - 1;
		// Lose tx 4 to oldness
		CBFoundTransaction * tx4 = CBNodeFullGetFoundTransaction(nodes[2], CBTransactionGetHash(initialTxs[4]));
		tx4->timeFound = CBGetMilliseconds() - 7200001;
		// First block
		CBBlock * block = CBNewBlock();
		block->prevBlockHash = CBNewByteArrayWithDataCopy(CBBlockGetHash(lastInitialBlock), 32);
		block->time = 1231471166;
		block->target = CB_MAX_TARGET;
		block->nonce = 2573394689;
		block->transactionNum = 2;
		block->transactions = malloc(sizeof(*block->transactions) * 2);
		// Coinbase
		block->transactions[0] = CBNewTransaction(0, 1);
		CBByteArray * nullHash = CBNewByteArrayOfSize(32);
		memset(CBByteArrayGetData(nullHash), 0, 32);
		CBScript * script = CBNewScriptWithDataCopy((uint8_t []){0xdd,5}, 2);
		CBTransactionTakeInput(block->transactions[0], CBNewTransactionInput(script, CB_TX_INPUT_FINAL, nullHash, 0xFFFFFFFF));
		CBReleaseObject(script);
		script = CBNewScriptWithDataCopy((uint8_t []){CB_SCRIPT_OP_TRUE}, 1);
		CBTransactionTakeOutput(block->transactions[0], CBNewTransactionOutput(1250000000, script));
		CBReleaseObject(script);
		CBTransactionMakeBytes(block->transactions[0]);
		CBTransactionSerialise(block->transactions[0], false);
		// Double spend the first double spend on the other chain
		block->transactions[1] = CBNewTransaction(14, 1);
		CBByteArray * prev = CBNewByteArrayWithDataCopy(CBTransactionGetHash(initialTxs[1]), 32);
		CBTransactionTakeInput(block->transactions[1], CBNewTransactionInput(NULL, CB_TX_INPUT_FINAL, prev, 2));
		CBReleaseObject(prev);
		script = CBNewScriptPubKeyHashOutput(CBKeyPairGetHash(keys + 1)); // This transaction differs by giving to node 1
		CBTransactionTakeOutput(block->transactions[1], CBNewTransactionOutput(312500000, script));
		CBReleaseObject(script);
		CBTransactionSignPubKeyHashInput(block->transactions[1], keys + 2, initialTxs[1]->outputs[3]->scriptObject, 0, CB_SIGHASH_ALL);
		CBTransactionMakeBytes(block->transactions[1]);
		CBTransactionSerialise(block->transactions[1], true);
		chainDoubleSpend = block->transactions[1];
		CBRetainObject(chainDoubleSpend);
		CBBlockCalculateAndSetMerkleRoot(block);
		CBGetMessage(block)->bytes = CBNewByteArrayOfSize(CBBlockCalculateLength(block, true));
		CBBlockSerialise(block, true, false);
		// Add to node 2
		CBBlockPeers * blockPeers = malloc(sizeof(*blockPeers));
		memcpy(blockPeers->hash, CBBlockGetHash(block), 20);
		CBInitAssociativeArray(&blockPeers->peers, CBPtrCompare, NULL, NULL);
		CBAssociativeArrayInsert(&nodes[2]->blockPeers, blockPeers, CBAssociativeArrayFind(&nodes[2]->blockPeers, blockPeers).position, NULL);
		CBAssociativeArrayInsert(&nodes[2]->askedForBlocks, blockPeers, CBAssociativeArrayFind(&nodes[2]->askedForBlocks, blockPeers).position, NULL);
		CBValidatorQueueBlock(CBGetNode(nodes[2])->validator, block, dummyPeer);
		// The next block will contain all the other transactions except those of the second double spend in the last test
		CBBlock * block2 = CBNewBlock();
		block2->prevBlockHash = CBNewByteArrayWithDataCopy(CBBlockGetHash(block), 32);
		CBReleaseObject(block);
		block2->time = 1231471167;
		block2->target = CB_MAX_TARGET;
		block2->nonce = 2573394690;
		block2->transactionNum = 3;
		block2->transactions = malloc(sizeof(*block2->transactions) * 3);
		// Coinbase
		block2->transactions[0] = CBNewTransaction(0, 1);
		memset(CBByteArrayGetData(nullHash), 0, 32);
		script = CBNewScriptWithDataCopy((uint8_t []){0xde,5}, 2);
		CBTransactionTakeInput(block2->transactions[0], CBNewTransactionInput(script, CB_TX_INPUT_FINAL, nullHash, 0xFFFFFFFF));
		CBReleaseObject(script);
		CBReleaseObject(nullHash);
		script = CBNewScriptWithDataCopy((uint8_t []){CB_SCRIPT_OP_TRUE}, 1);
		CBTransactionTakeOutput(block2->transactions[0], CBNewTransactionOutput(1250000000, script));
		CBReleaseObject(script);
		CBTransactionMakeBytes(block2->transactions[0]);
		CBTransactionSerialise(block2->transactions[0], false);
		// Transaction 5
		block2->transactions[1] = initialTxs[5];
		CBRetainObject(initialTxs[5]);
		// Transaction 7
		block2->transactions[2] = initialTxs[7];
		CBRetainObject(initialTxs[7]);
		CBBlockCalculateAndSetMerkleRoot(block2);
		CBGetMessage(block2)->bytes = CBNewByteArrayOfSize(CBBlockCalculateLength(block2, true));
		CBBlockSerialise(block2, true, true);
		// Add to node 2
		blockPeers = malloc(sizeof(*blockPeers));
		memcpy(blockPeers->hash, CBBlockGetHash(block2), 20);
		CBInitAssociativeArray(&blockPeers->peers, CBPtrCompare, NULL, NULL);
		CBAssociativeArrayInsert(&nodes[2]->blockPeers, blockPeers, CBAssociativeArrayFind(&nodes[2]->blockPeers, blockPeers).position, NULL);
		CBAssociativeArrayInsert(&nodes[2]->askedForBlocks, blockPeers, CBAssociativeArrayFind(&nodes[2]->askedForBlocks, blockPeers).position, NULL);
		CBValidatorQueueBlock(CBGetNode(nodes[2])->validator, block2, dummyPeer);
		CBReleaseObject(block2);
	}
}

void finishReceiveInitialTest(void * foo);
void finishReceiveInitialTest(void * foo){
	// Ensure nodes finished processing transactions
	for (uint8_t x = 0; x < 3; x++) {
		CBMutexLock(CBGetNode(nodes[x])->blockAndTxMutex);
		CBMutexUnlock(CBGetNode(nodes[x])->blockAndTxMutex);
	}
	// Transactions look like this
	// [  Block  ]
	// ---------------------
	// [         ] [#0 -> 0]
	// [ #1 -> 1 ] [#4 -> 1]
	// [         ] [#8 -> 2]
	// ---------------------
	// [         ] [#1 -> 1]
	// [ #2 -> 2 ] [#5 -> 2] -> #7,#9
	// [         ] [#9 -> 0]
	// ---------------------
	// [         ] [#2 -> 2]
	// [ #3 -> 0 ] [#6 -> 0]
	// [         ]
	// ---------------------
	// [         ] [#3 -> 0] -> #6,#8
	// [ #4 -> 1 ] [#7 -> 1]
	// [         ]
	for (uint8_t x = 0; x < 3; x++) {
		CBNodeFull * node = nodes[x];
		if (CBGetNetworkCommunicator(node)->blockHeight != 1001) {
			CBLogError("RECEIVE INITIAL BLOCKS AND TXS FINISH BLOCK HEIGHT FAIL\n");
			exit(EXIT_FAILURE);
		}
		for (uint8_t y = 4; y < 14; y++) {
			CBFoundTransaction * fndTx = CBNodeFullGetFoundTransaction(node, CBTransactionGetHash(initialTxs[y]));
			if (fndTx == NULL) {
				if (nodeOwns[x][y]) {
					CBLogError("RECEIVE INITIAL BLOCKS AND TXS FINISH NOT FOUND");
					exit(EXIT_FAILURE);
				}
				continue;
			}
			if ((y >= 10) != (fndTx->utx.numUnconfDeps != 0)) {
				CBLogError("RECEIVE INITIAL BLOCKS AND TXS FINISH NUM UNCONF DEPS FAIL\n");
				exit(EXIT_FAILURE);
			}
			if ((fndTx->utx.type == CB_TX_OURS) != nodeOwns[x][y]) {
				CBLogError("RECEIVE INITIAL BLOCKS AND TXS FINISH OWNERSHIP FAIL\n");
				exit(EXIT_FAILURE);
			}
		}
	}
	// Check accounter information. Only bother with node 0
	CBDepObject cursor;
	if (! CBNewAccounterStorageTransactionCursor(&cursor, CBGetNode(nodes[0])->accounterStorage, 1, 0, UINT64_MAX)) {
		printf("INITIAL CREATE CURSOR FAIL\n");
		exit(EXIT_FAILURE);
	}
	uint8_t zeroHash[20];
	memset(zeroHash, 0, 20);
	uint64_t times[5];
	for (uint8_t x = 0; x < 6; x++)
		CBAccounterGetTransactionTime(CBGetNode(nodes[0])->accounterStorage, CBTransactionGetHash(initialTxs[(int []){4,6,7,10,12,13}[x]]), times + x);
	checkTransactions(cursor, (CBTestTxDetails [7]){
		/* 0 */ {CBKeyPairGetHash(keys), 1001, 1250000000, 1231471165, CBTransactionGetHash(initialTxs[2]), 1250000000,0},
		/* 1 */ {CBKeyPairGetHash(keys), CB_UNCONFIRMED, 312500000, times[0], CBTransactionGetHash(initialTxs[4]), 1562500000,312500000},
		/* 2 */ {CBKeyPairGetHash(keys + 2), CB_UNCONFIRMED, -312500000, times[1], CBTransactionGetHash(initialTxs[6]), 1250000000,0},
		/* 3 */ {CBKeyPairGetHash(keys), CB_UNCONFIRMED, 312500000, times[2], CBTransactionGetHash(initialTxs[7]), 1562500000,312500000},
		/* 4 */ {CBKeyPairGetHash(keys), CB_UNCONFIRMED, 0, times[3], CBTransactionGetHash(initialTxs[10]), 1562500000,312500000},
		/* 5 */ {CBKeyPairGetHash(keys + 2), CB_UNCONFIRMED, -156250000, times[4], CBTransactionGetHash(initialTxs[12]), 1406250000, 156250000},
		/* 6 */ {CBKeyPairGetHash(keys), CB_UNCONFIRMED, 468750000, times[5], CBTransactionGetHash(initialTxs[13]),1875000000,625000000},
	}, 7);
	int64_t ubalance;
	uint64_t cbalance;
	if (! CBAccounterGetAccountBalance(CBGetNode(nodes[0])->accounterStorage, 1, &cbalance, &ubalance)){
		printf("INITIAL GET UNCONF BALANCE FAIL\n");
		exit(EXIT_FAILURE);
	}
	if (ubalance != 625000000) {
		printf("INITIAL UNCONF BALANCE FAIL %lli != 625000000\n", ubalance);
		exit(EXIT_FAILURE);
	}
	if (cbalance != 1875000000) {
		printf("INITIAL ALL BALANCE FAIL %llu != 1875000000\n", cbalance);
		exit(EXIT_FAILURE);
	}
	CBLogVerbose("RECEIVE INITIAL BLOCKS complete.");
	// Test processing transaction spending unconf transaction which has been spent
	CBTransaction * falseTx = CBNewTransaction(22, 1);
	CBGetMessage(falseTx)->type = CB_MESSAGE_TYPE_TX;
	CBByteArray * prev = CBNewByteArrayWithDataCopy(CBTransactionGetHash(initialTxs[9]), 32);
	CBTransactionTakeInput(falseTx, CBNewTransactionInput(NULL, CB_TX_INPUT_FINAL, prev, 0));
	CBReleaseObject(prev);
	CBScript * script = CBNewScriptPubKeyHashOutput(CBKeyPairGetHash(keys));
	CBTransactionTakeOutput(falseTx, CBNewTransactionOutput(312500000, script));
	CBTransactionSignPubKeyHashInput(falseTx, keys, initialTxs[0]->outputs[0]->scriptObject, 0, CB_SIGHASH_ALL);
	CBTransactionMakeBytes(falseTx);
	CBTransactionSerialise(falseTx, true);
	// Get some peer for node 0
	CBPosition pos;
	CBAssociativeArrayGetFirst(&CBGetNetworkCommunicator(nodes[0])->addresses->peers, &pos);
	CBPeer * peer = pos.node->elements[pos.index];
	if (CBNodeFullNewUnconfirmedTransaction(nodes[0], peer, falseTx) != CB_TRUE){
		CBLogError("PROCESS FALSE TX 3 ERROR");
		exit(EXIT_FAILURE);
	}
	if (CBNodeFullGetAnyTransaction(nodes[0], CBTransactionGetHash(falseTx)) != NULL) {
		CBLogError("HAVE FALSE TX 3 ERROR");
		exit(EXIT_FAILURE);
	}
	// Test adding blocks to the smaller chain on 45563 for fork.
	// For first block
	// Add transaction 1 and 5
	// Add double spend of 13
	// For the second
	// Add transaction 3 and 7
	// Add double spend of 11
	testPhase = CHAIN_REORGANISATION;
	gotTxNum = 0;
	dummyPeer->nodeObj = nodes[1];
	CBBlock * block = CBNewBlock();
	block->prevBlockHash = lastNode1BlockHash;
	block->time = 1231471166;
	block->target = CB_MAX_TARGET;
	block->nonce = 2573394689;
	block->transactionNum = 6;
	block->transactions = malloc(sizeof(*block->transactions) * 6);
	// Coinbase
	block->transactions[0] = CBNewTransaction(0, 1);
	CBByteArray * nullHash = CBNewByteArrayOfSize(32);
	memset(CBByteArrayGetData(nullHash), 0, 32);
	script = CBNewScriptWithDataCopy((uint8_t []){0xdc,5}, 2);
	CBTransactionTakeInput(block->transactions[0], CBNewTransactionInput(script, CB_TX_INPUT_FINAL, nullHash, 0xFFFFFFFF));
	CBReleaseObject(script);
	script = CBNewScriptWithDataCopy((uint8_t []){CB_SCRIPT_OP_TRUE}, 1);
	CBTransactionTakeOutput(block->transactions[0], CBNewTransactionOutput(1250000000, script));
	CBReleaseObject(script);
	CBTransactionMakeBytes(block->transactions[0]);
	CBTransactionSerialise(block->transactions[0], false);
	// Transaction 1
	block->transactions[1] = initialTxs[1];
	CBRetainObject(initialTxs[1]);
	// Transaction 5
	block->transactions[2] = initialTxs[5];
	CBRetainObject(initialTxs[5]);
	// Double spend of 13
	block->transactions[3] = CBNewTransaction(14, 1);
	prev = CBNewByteArrayWithDataCopy(CBTransactionGetHash(initialTxs[1]), 32);
	CBTransactionTakeInput(block->transactions[3], CBNewTransactionInput(NULL, CB_TX_INPUT_FINAL, prev, 2));
	CBReleaseObject(prev);
	script = CBNewScriptPubKeyHashOutput(CBKeyPairGetHash(keys));
	CBTransactionTakeOutput(block->transactions[3], CBNewTransactionOutput(312500000, script));
	CBReleaseObject(script);
	CBTransactionSignPubKeyHashInput(block->transactions[3], keys + 2, initialTxs[1]->outputs[3]->scriptObject, 0, CB_SIGHASH_ALL);
	CBTransactionMakeBytes(block->transactions[3]);
	CBTransactionSerialise(block->transactions[3], true);
	doubleSpends[0] = block->transactions[3];
	CBRetainObject(doubleSpends[0]);
	// Now give two dependants to the double spend of 13
	for (uint8_t x = 0; x < 2; x++) {
		block->transactions[4 + x] = CBNewTransaction(15 + x, 1);
		prev = CBNewByteArrayWithDataCopy(CBTransactionGetHash(block->transactions[3 + x]), 32);
		CBTransactionTakeInput(block->transactions[4 + x], CBNewTransactionInput(NULL, CB_TX_INPUT_FINAL, prev, 0));
		CBReleaseObject(prev);
		script = CBNewScriptPubKeyHashOutput(CBKeyPairGetHash(keys));
		CBTransactionTakeOutput(block->transactions[4 + x], CBNewTransactionOutput(312500000, script));
		CBReleaseObject(script);
		CBTransactionSignPubKeyHashInput(block->transactions[4 + x], keys, block->transactions[3 + x]->outputs[0]->scriptObject, 0, CB_SIGHASH_ALL);
		CBTransactionMakeBytes(block->transactions[4 + x]);
		CBTransactionSerialise(block->transactions[4 + x], true);
		doubleSpends[1 + x] = block->transactions[4 + x];
		CBRetainObject(doubleSpends[1 + x]);
	}
	CBBlockCalculateAndSetMerkleRoot(block);
	CBGetMessage(block)->bytes = CBNewByteArrayOfSize(CBBlockCalculateLength(block, true));
	CBBlockSerialise(block, true, false);
	// Add to node 1
	CBBlockPeers *  blockPeers = malloc(sizeof(*blockPeers));
	memcpy(blockPeers->hash, CBBlockGetHash(block), 20);
	CBInitAssociativeArray(&blockPeers->peers, CBPtrCompare, NULL, NULL);
	CBAssociativeArrayInsert(&nodes[1]->blockPeers, blockPeers, CBAssociativeArrayFind(&nodes[1]->blockPeers, blockPeers).position, NULL);
	CBAssociativeArrayInsert(&nodes[1]->askedForBlocks, blockPeers, CBAssociativeArrayFind(&nodes[1]->askedForBlocks, blockPeers).position, NULL);
	CBValidatorQueueBlock(CBGetNode(nodes[1])->validator, block, dummyPeer);
	// Second block
	CBBlock * block2 = CBNewBlock();
	block2->prevBlockHash = CBNewByteArrayWithDataCopy(CBBlockGetHash(block), 32);
	CBReleaseObject(block);
	block2->time = 1231471167;
	block2->target = CB_MAX_TARGET;
	block2->nonce = 2573394690;
	block2->transactionNum = 6;
	block2->transactions = malloc(sizeof(*block2->transactions) * 6);
	// Coinbase
	block2->transactions[0] = CBNewTransaction(0, 1);
	memset(CBByteArrayGetData(nullHash), 0, 32);
	script = CBNewScriptWithDataCopy((uint8_t []){0xdd,5}, 2);
	CBTransactionTakeInput(block2->transactions[0], CBNewTransactionInput(script, CB_TX_INPUT_FINAL, nullHash, 0xFFFFFFFF));
	CBReleaseObject(script);
	CBReleaseObject(nullHash);
	script = CBNewScriptWithDataCopy((uint8_t []){CB_SCRIPT_OP_TRUE}, 1);
	CBTransactionTakeOutput(block2->transactions[0], CBNewTransactionOutput(1250000000, script));
	CBReleaseObject(script);
	CBTransactionMakeBytes(block2->transactions[0]);
	CBTransactionSerialise(block2->transactions[0], false);
	// Transaction 3
	block2->transactions[1] = initialTxs[3];
	CBRetainObject(initialTxs[1]);
	// Transaction 7
	block2->transactions[2] = initialTxs[7];
	CBRetainObject(initialTxs[7]);
	// Double spend of 11
	block2->transactions[3] = CBNewTransaction(17, 1);
	prev = CBNewByteArrayWithDataCopy(CBTransactionGetHash(initialTxs[3]), 32);
	CBTransactionTakeInput(block2->transactions[3], CBNewTransactionInput(NULL, CB_TX_INPUT_FINAL, prev, 1));
	CBReleaseObject(prev);
	script = CBNewScriptPubKeyHashOutput(CBKeyPairGetHash(keys));
	CBTransactionTakeOutput(block2->transactions[3], CBNewTransactionOutput(312500000, script));
	CBReleaseObject(script);
	CBTransactionSignPubKeyHashInput(block2->transactions[3], keys + 1, initialTxs[3]->outputs[1]->scriptObject, 0, CB_SIGHASH_ALL);
	CBTransactionMakeBytes(block2->transactions[3]);
	CBTransactionSerialise(block2->transactions[3], true);
	doubleSpends[3] = block2->transactions[3];
	CBRetainObject(doubleSpends[3]);
	// Now give two dependants to the double spend of 11
	for (uint8_t x = 0; x < 2; x++) {
		block2->transactions[4 + x] = CBNewTransaction(18 + x, 1);
		prev = CBNewByteArrayWithDataCopy(CBTransactionGetHash(block2->transactions[3 + x]), 32);
		CBTransactionTakeInput(block2->transactions[4 + x], CBNewTransactionInput(NULL, CB_TX_INPUT_FINAL, prev, 0));
		CBReleaseObject(prev);
		script = CBNewScriptPubKeyHashOutput(CBKeyPairGetHash(keys));
		CBTransactionTakeOutput(block2->transactions[4 + x], CBNewTransactionOutput(312500000, script));
		CBReleaseObject(script);
		CBTransactionSignPubKeyHashInput(block2->transactions[4 + x], keys, block2->transactions[3 + x]->outputs[0]->scriptObject, 0, CB_SIGHASH_ALL);
		CBTransactionMakeBytes(block2->transactions[4 + x]);
		CBTransactionSerialise(block2->transactions[4 + x], true);
		doubleSpends[4 + x] = block2->transactions[4 + x];
		CBRetainObject(doubleSpends[4 + x]);
	}
	CBBlockCalculateAndSetMerkleRoot(block2);
	CBGetMessage(block2)->bytes = CBNewByteArrayOfSize(CBBlockCalculateLength(block2, true));
	CBBlockSerialise(block2, true, true);
	reorgTestBlockHash = CBNewByteArrayWithDataCopy(CBBlockGetHash(block2), 32);
	// Add to node 1
	// Add block to block peers and asked for blocks
	blockPeers = malloc(sizeof(*blockPeers));
	memcpy(blockPeers->hash, CBBlockGetHash(block2), 20);
	CBInitAssociativeArray(&blockPeers->peers, CBPtrCompare, NULL, NULL);
	CBAssociativeArrayInsert(&nodes[1]->blockPeers, blockPeers, CBAssociativeArrayFind(&nodes[1]->blockPeers, blockPeers).position, NULL);
	CBAssociativeArrayInsert(&nodes[1]->askedForBlocks, blockPeers, CBAssociativeArrayFind(&nodes[1]->askedForBlocks, blockPeers).position, NULL);
	CBValidatorQueueBlock(CBGetNode(nodes[1])->validator, block2, dummyPeer);
	CBReleaseObject(block2);
}

void newBlock(CBNode *, CBBlock * block, uint32_t forkPoint);
void newBlock(CBNode * node, CBBlock * block, uint32_t forkPoint){
	CBNodeFull * nodeFull = CBGetNodeFull(node);
	uint32_t height = CBGetNetworkCommunicator(node)->blockHeight;
	int nodeNum = (nodes[0] == nodeFull) ? 0 : ((nodes[1] == nodeFull)? 1 : 2);
	CBMutexLock(testMutex);
	if (testPhase == RECEIVE_INITIAL_BLOCKS_AND_TXS) {
		if (nodeNum == 0) {
			CBLogError("RECEIVE INITIAL BLOCKS AND TXS NEW BLOCK SENDING NODE\n");
			exit(EXIT_FAILURE);
		}
		if (forkPoint == 501) {
			if (nodeNum == 0) {
				CBLogError("RECEIVE INITIAL BLOCKS AND TXS NEW BLOCK FORK NOT IN NODE 1\n");
				exit(EXIT_FAILURE);
			}else if (nodeNum == 1)
				receiveInitialBlocksAndTxs |= NODE1_FORK;
			// Node 2 might fork if got blocks from node 1 first
		}else if (forkPoint != CB_NO_FORK) {
			CBLogError("RECEIVE INITIAL BLOCKS AND TXS NEW BLOCK FORK\n");
			exit(EXIT_FAILURE);
		}
		if (receiveInitialBlocksAndTxs & nodeNum) {
			CBLogError("RECEIVE INITIAL BLOCKS AND TXS NEW BLOCK AFTER GOT INITAL BLOCKS\n");
			exit(EXIT_FAILURE);
		}
		if (height == 1001) {
			if (nodeNum == 1) {
				if (! (receiveInitialBlocksAndTxs | NODE1_FORK)) {
					CBLogError("RECEIVE INITIAL BLOCKS AND TXS NEW BLOCK NODE 1 GOT BEFORE FORK\n");
					exit(EXIT_FAILURE);
				}
				receiveInitialBlocksAndTxs |= NODE1_GOT;
			}else
				receiveInitialBlocksAndTxs |= NODE2_GOT;
		}
	}else if (testPhase == CHAIN_REORGANISATION) {
		if (height != 1002) {
			CBLogError("CHAIN REORGANISATION BLOCK HEIGHT FAIL\n");
			exit(EXIT_FAILURE);
		}
		if (forkPoint != 501) {
			CBLogError("CHAIN REORGANISATION FORK POINT FAIL\n");
			exit(EXIT_FAILURE);
		}
		if (chainReorg & (1 << nodeNum)) {
			CBLogError("CHAIN REORGANISATION NEW BLOCK AFTER GOT BLOCKS\n");
			exit(EXIT_FAILURE);
		}
		chainReorg |= 1 << nodeNum;
	}else if (testPhase == LOSE_CHAIN_AND_RELAY) {
		if (height != 1003) {
			CBLogError("CHAIN REORGANISATION BLOCK HEIGHT FAIL\n");
			exit(EXIT_FAILURE);
		}
		if (forkPoint != 501) {
			CBLogError("CHAIN REORGANISATION FORK POINT FAIL\n");
			exit(EXIT_FAILURE);
		}
		if (chainReorg & (1 << nodeNum)) {
			CBLogError("CHAIN REORGANISATION NEW BLOCK AFTER GOT BLOCKS\n");
			exit(EXIT_FAILURE);
		}
		chainReorg |= 1 << nodeNum;
	}else if (testPhase == ORPHAN_TO_CHAIN) {
		if (height != 1004) {
			CBLogError("ORPHAN TO CHAIN BLOCK HEIGHT FAIL\n");
			exit(EXIT_FAILURE);
		}
		if (forkPoint != 501) {
			CBLogError("ORPHAN TO CHAIN FORK POINT FAIL\n");
			exit(EXIT_FAILURE);
		}
		if (chainReorg & (1 << nodeNum)) {
			CBLogError("ORPHAN TO CHAIN NEW BLOCK AFTER GOT BLOCKS\n");
			exit(EXIT_FAILURE);
		}
		chainReorg |= 1 << nodeNum;
	}
	CBMutexUnlock(testMutex);
}

void newTransaction(CBNode *, CBTransaction * tx, uint64_t timestamp, uint32_t blockHeight, CBTransactionAccountDetailList * details);
void newTransaction(CBNode * node, CBTransaction * tx, uint64_t timestamp, uint32_t blockHeight, CBTransactionAccountDetailList * details){
	CBNodeFull * nodeFull = CBGetNodeFull(node);
	int nodeNum = (nodes[0] == nodeFull) ? 0 : ((nodes[1] == nodeFull)? 1 : 2);
	CBMutexLock(testMutex);
	if (details->accountID != 1){
		CBLogError("NEW TX BAD ACCOUNT ID NUM\n");
		exit(EXIT_FAILURE);
	}
	if (testPhase == RECEIVE_INITIAL_BLOCKS_AND_TXS) {
		if (gotTxNum == 15) {
			CBLogError("RECEIVE INITIAL BLOCKS AND TXS NEW TX TOO MANY TXS\n");
			exit(EXIT_FAILURE);
		}
		uint8_t txNum;
		for (uint8_t x = 0;; x++) {
			if (memcmp(CBTransactionGetHash(tx), CBTransactionGetHash(initialTxs[x]), 32) == 0){
				txNum = x;
				break;
			}
			if (x == 13) {
				CBLogError("RECEIVE INITIAL BLOCKS AND TXS NEW TX UNKNOWN TX\n");
				exit(EXIT_FAILURE);
			}
		}
		if (txNum < 4) {
			if (blockHeight != 1001) {
				CBLogError("RECEIVE INITIAL BLOCKS AND TXS NEW TX BLOCKHEIGHT 1001 FAIL\n");
				exit(EXIT_FAILURE);
			}
			if (timestamp != 1231471165) {
				CBLogError("RECEIVE INITIAL BLOCKS AND TXS NEW TX TIMESTAMP 1231471165 FAIL\n");
				exit(EXIT_FAILURE);
			}
			// Should be got by node (txNum + 1) % 3
			uint8_t expectedNode = (txNum + 1) % 3;
			if (nodeNum != expectedNode) {
				CBLogError("RECEIVE INITIAL BLOCKS AND TXS NEW TX %u BAD NODE NUM\n", txNum);
				exit(EXIT_FAILURE);
			}
			if (memcmp(details->accountTxDetails.addrHash, CBKeyPairGetHash(keys + nodeNum), 20)){
				CBLogError("RECEIVE INITIAL BLOCKS AND TXS NEW TX BAD ADDR HASH\n");
				exit(EXIT_FAILURE);
			}
			if (details->accountTxDetails.amount != 1250000000) {
				CBLogError("RECEIVE INITIAL BLOCKS AND TXS NEW TX %u BAD AMOUNT\n", txNum);
				exit(EXIT_FAILURE);
			}
			if (details->next != NULL) {
				CBLogError("RECEIVE INITIAL BLOCKS AND TXS NEW TX %u END FAIL\n", txNum);
				exit(EXIT_FAILURE);
			}
		}else{
			if (blockHeight != CB_UNCONFIRMED) {
				CBLogError("RECEIVE INITIAL BLOCKS AND TXS NEW TX BLOCKHEIGHT UNCONF FAIL\n");
				exit(EXIT_FAILURE);
			}
			if (!nodeOwns[nodeNum][txNum]) {
				CBLogError("RECEIVE INITIAL BLOCKS AND TXS NEW TX %u DOESN'T OWN\n",txNum);
				exit(EXIT_FAILURE);
			}
			uint8_t adjInd = txNum - 4;
			bool takeInput = (adjInd % 4 + 1) % 3 == nodeNum;
			bool gotOutput = adjInd % 3 == nodeNum;
			bool secondInput = (adjInd >= 6)*(adjInd % 2)*2 == nodeNum;
			if (details->accountTxDetails.amount
				!= -takeInput*312500000 - secondInput*156250000 + gotOutput*(312500000 + (adjInd >= 6)*156250000)) {
				CBLogError("RECEIVE INITIAL BLOCKS AND TXS NEW TX %u BAD AMOUNT\n", txNum);
				exit(EXIT_FAILURE);
			}
			if (memcmp(details->accountTxDetails.addrHash, CBKeyPairGetHash(keys + adjInd % 3), 20)){
				CBLogError("RECEIVE INITIAL BLOCKS AND TXS NEW TX %u BAD ADDR HASH\n",txNum);
				exit(EXIT_FAILURE);
			}
		}
		gotTxNum++;
		if (gotTxNum == 15 && receiveInitialBlocksAndTxs == COMPLETE_RECEIVE_INITAL)
			CBRunOnEventLoop(CBGetNetworkCommunicator(nodes[0])->eventLoop, finishReceiveInitialTest, NULL, false);
	}else if (testPhase == CHAIN_REORGANISATION) {
		uint8_t txNum;
		for (uint8_t x = 0;; x++) {
			if (memcmp(CBTransactionGetHash(tx), CBTransactionGetHash(doubleSpends[x]), 32) == 0) {
				txNum = x;
				break;
			}
			if (x == 5) {
				CBLogError("CHAIN REORGANISATION NEW TX UNKNOWN TX\n");
				exit(EXIT_FAILURE);
			}
		}
		if (blockHeight != 1001 + (txNum > 2)) {
			CBLogError("CHAIN REORGANISATION NEW TX BLOCKHEIGHT 1001 FAIL\n");
			exit(EXIT_FAILURE);
		}
		if (timestamp != 1231471166 + (txNum > 2)) {
			CBLogError("CHAIN REORGANISATION NEW TX TIMESTAMP FAIL\n");
			exit(EXIT_FAILURE);
		}
		// Node 1 has the second double spend, and node 2 the first only. The dependants are for node 0
		if ((txNum != 3 && nodeNum == 1) || (txNum != 0 && nodeNum == 2)) {
			CBLogError("CHAIN REORGANISATION NEW TX %u BAD NODE NUM\n", txNum);
			exit(EXIT_FAILURE);
		}
		// Addr is zero when transferring money to self.
		if ((txNum % 3 == 0 && memcmp(details->accountTxDetails.addrHash, CBKeyPairGetHash(keys), 20))
			|| (txNum % 3 != 0 && memcmp(details->accountTxDetails.addrHash, (uint8_t [20]){0}, 20))){
			CBLogError("CHAIN REORGANISATION NEW TX %u BAD ADDR HASH\n", txNum);
			exit(EXIT_FAILURE);
		}
		if (details->accountTxDetails.amount != (nodeNum == 1 || nodeNum == 2 ? -312500000 : (txNum % 3 == 0 ? 312500000 : 0))) {
			CBLogError("CHAIN REORGANISATION NEW TX %u BAD AMOUNT\n", txNum);
			exit(EXIT_FAILURE);
		}
		if (details->next != NULL) {
			CBLogError("CHAIN REORGANISATION NEW TX %u END FAIL", txNum);
			exit(EXIT_FAILURE);
		}
		gotTxNum++;
		maybeFinishReorgTest();
	}else if (testPhase == LOSE_CHAIN_AND_RELAY) {
		if (memcmp(CBTransactionGetHash(tx), CBTransactionGetHash(chainDoubleSpend), 32)) {
			CBLogError("LOSE CHAIN AND RELAY NEW TX HASH FAIL");
			exit(EXIT_FAILURE);
		}
		if (timestamp != 1231471166) {
			CBLogError("LOSE CHAIN AND RELAY NEW TX TIMESTAMP FAIL\n");
			exit(EXIT_FAILURE);
		}
		if (nodeNum == 0) {
			CBLogError("LOSE CHAIN AND RELAY NEW TX BAD NODE\n");
			exit(EXIT_FAILURE);
		}
		if (memcmp(details->accountTxDetails.addrHash, CBKeyPairGetHash(keys + 1), 20)) {
			CBLogError("LOSE CHAIN AND RELAY NEW TX BAD ADDR HASH\n");
			exit(EXIT_FAILURE);
		}
		if (details->accountTxDetails.amount != (nodeNum == 2 ? -312500000 : 312500000)) {
			CBLogError("LOSE CHAIN AND RELAY NEW TX BAD AMOUNT\n");
			exit(EXIT_FAILURE);
		}
		if (details->next != NULL) {
			CBLogError("LOSE CHAIN AND RELAY NEW TX END FAIL");
			exit(EXIT_FAILURE);
		}
		gotTxNum++;
		maybeFinishLoseTest();
	}else if (testPhase == ORPHAN_TO_CHAIN){
		for (uint8_t x = 0;; x++) {
			if (memcmp(CBTransactionGetHash(tx), CBTransactionGetHash(doubleSpends[x]), 32) == 0) {
				if (blockHeight != 1001) {
					CBLogError("ORPHAN TO CHAIN NEW TX BLOCKHEIGHT 1001 FAIL\n");
					exit(EXIT_FAILURE);
				}
				if (timestamp != 1231471166) {
					CBLogError("ORPHAN TO CHAIN NEW TX TIMESTAMP FAIL\n");
					exit(EXIT_FAILURE);
				}
				// Node 1 has the second double spend, and node 2 the first only. The dependants are for node 0
				if (nodeNum == 1 || (x != 0 && nodeNum == 2)) {
					CBLogError("ORPHAN TO CHAIN NEW TX %u BAD NODE NUM\n", x);
					exit(EXIT_FAILURE);
				}
				// Addr is zero when transferring money to self.
				if ((x == 0 && memcmp(details->accountTxDetails.addrHash, CBKeyPairGetHash(keys), 20))
					|| (x != 0 && memcmp(details->accountTxDetails.addrHash, (uint8_t [20]){0}, 20))){
					CBLogError("ORPHAN TO CHAIN NEW TX %u BAD ADDR HASH\n", x);
					exit(EXIT_FAILURE);
				}
				if (details->accountTxDetails.amount != (nodeNum == 2 ? -312500000 : (x == 0 ? 312500000 : 0))) {
					CBLogError("ORPHAN TO CHAIN NEW TX %u BAD AMOUNT\n", x);
					exit(EXIT_FAILURE);
				}
				break;
			}
			if (x == 2) {
				if (memcmp(CBTransactionGetHash(tx), CBTransactionGetHash(orphanSpendOtherBranch), 32)) {
					CBLogError("ORPHAN TO CHAIN NEW TX UNKNOWN TX\n");
					exit(EXIT_FAILURE);
				}
				if (blockHeight != 1004) {
					CBLogError("ORPHAN TO CHAIN NEW TX ORPHAN BLOCKHEIGHT 1001 FAIL\n");
					exit(EXIT_FAILURE);
				}
				if (timestamp != 1231471169) {
					CBLogError("ORPHAN TO CHAIN NEW TX ORPHAN TIMESTAMP FAIL\n");
					exit(EXIT_FAILURE);
				}
				if (nodeNum != 0) {
					CBLogError("ORPHAN TO CHAIN NEW TX ORPHAN BAD NODE NUM\n");
					exit(EXIT_FAILURE);
				}
				// Addr is zero when transferring money to self.
				if (memcmp(details->accountTxDetails.addrHash, (uint8_t [20]){0}, 20)){
					CBLogError("ORPHAN TO CHAIN NEW TX ORPHAN BAD ADDR HASH\n");
					exit(EXIT_FAILURE);
				}
				if (details->accountTxDetails.amount != 0) {
					CBLogError("ORPHAN TO CHAIN NEW TX OPRHAN BAD AMOUNT\n");
					exit(EXIT_FAILURE);
				}
				break;
			}
		}
		if (details->next != NULL) {
			CBLogError("ORPHAN TO CHAIN NEW TX END FAIL");
			exit(EXIT_FAILURE);
		}
		gotTxNum++;
		CBRunOnEventLoop(CBGetNetworkCommunicator(nodes[0])->eventLoop, maybeFinishOrphanTest, NULL, false);
	}
	CBMutexUnlock(testMutex);
}

void transactionConfirmed(CBNode *, uint8_t * txHash, uint32_t blockHeight);
void transactionConfirmed(CBNode * node, uint8_t * txHash, uint32_t blockHeight){
	CBMutexLock(testMutex);
	if (testPhase == CHAIN_REORGANISATION) {
		if (blockHeight == 1001) {
			if (nodes[0] == CBGetNodeFull(node)) {
				CBLogError("CHAIN REORGANISATION TX CONFIRMED BLOCK 1001 NODE FAIL\n");
				exit(EXIT_FAILURE);
			}
			if (memcmp(txHash, CBTransactionGetHash(initialTxs[5]), 32) != 0) {
				CBLogError("CHAIN REORGANISATION TX CONFIRMED BLOCK 1001 HASH FAIL\n");
				exit(EXIT_FAILURE);
			}
		}else if (blockHeight == 1002){
			if (nodes[2] == CBGetNodeFull(node)) {
				CBLogError("CHAIN REORGANISATION TX CONFIRMED BLOCK 1002 NODE FAIL\n");
				exit(EXIT_FAILURE);
			}
			if (memcmp(txHash, CBTransactionGetHash(initialTxs[7]), 32) != 0) {
				CBLogError("CHAIN REORGANISATION TX CONFIRMED BLOCK 1002 HASH FAIL\n");
				exit(EXIT_FAILURE);
			}
		}else{
			CBLogError("CHAIN REORGANISATION TX CONFIRMED BLOCK HEIGHT FAIL\n");
			exit(EXIT_FAILURE);
		}
		confirmedNum++;
		maybeFinishReorgTest();
	}else if (testPhase == LOSE_CHAIN_AND_RELAY){
		for (uint8_t x = 0;; x += 2) { // Only txs 0 and 2 were added onto the other branch, so we have all of them back
			if (memcmp(txHash, CBTransactionGetHash(initialTxs[x]), 32) == 0)
				break;
			if (x == 2) {
				CBLogError("LOSE CHAIN AND RELAY TX CONFIRMED BAD HASH\n");
				exit(EXIT_FAILURE);
			}
		}
		confirmedNum++;
		maybeFinishLoseTest();
	}else if (testPhase == ORPHAN_TO_CHAIN){
		for (uint8_t x = 3;; x++) {
			if (memcmp(txHash, CBTransactionGetHash(doubleSpends[x]), 32) == 0) {
				if (nodes[0] != CBGetNodeFull(node) && (x != 3 || nodes[1] != CBGetNodeFull(node))) {
					CBLogError("ORPHAN TO CHAIN TX CONFIRMED NODE NUM FAIL\n");
					exit(EXIT_FAILURE);
				}
				if (CBNodeFullGetFoundTransaction(CBGetNodeFull(node),  CBTransactionGetHash(doubleSpends[x])) != NULL) {
					CBLogError("OPRHAN TO CHAIN TX CONFIRMED STILL UNCONFIRMED FAIL\n");
					exit(EXIT_FAILURE);
				}
				break;
			}
			if (x == 5) {
				CBLogError("ORPHAN TO CHAIN TX CONFIRMED HASH FAIL\n");
				exit(EXIT_FAILURE);
			}
		}
		confirmedNum++;
		CBRunOnEventLoop(CBGetNetworkCommunicator(nodes[0])->eventLoop, maybeFinishOrphanTest, NULL, false);
	}else{
		CBLogError("TX CONFIRMED BAD TEST PHASE\n");
		exit(EXIT_FAILURE);
	}
	CBMutexUnlock(testMutex);
}

void doubleSpend(CBNode *, uint8_t * txHash);
void doubleSpend(CBNode * node, uint8_t * txHash){
	CBNodeFull * nodeFull = CBGetNodeFull(node);
	int nodeNum = (nodes[0] == nodeFull)? 0 : ((nodes[1] == nodeFull)? 1 : 2);
	CBMutexLock(testMutex);
	if (testPhase == CHAIN_REORGANISATION) {
		if ((memcmp(txHash, CBTransactionGetHash(initialTxs[11]), 32) != 0 || !nodeOwns[nodeNum][11])
			&& (memcmp(txHash, CBTransactionGetHash(initialTxs[13]), 32) != 0 || !nodeOwns[nodeNum][13])) {
			CBLogError("CHAIN REORGANISATION TX DOUBLE SPEND HASH FAIL\n");
			exit(EXIT_FAILURE);
		}
		doubleSpendNum++;
		maybeFinishReorgTest();
	}else if (testPhase == LOSE_CHAIN_AND_RELAY){
		for (uint8_t x = 0;; x++) {
			if (memcmp(txHash, CBTransactionGetHash(doubleSpends[x]), 32) == 0) {
				if ((nodeNum == 2 && x != 0) || nodeNum == 1) {
					CBLogError("LOSE CHAIN AND RELAY TX DOUBLE SPEND NODE NUM FAIL\n");
					exit(EXIT_FAILURE);
				}
				break;
			}
			if (x == 2) {
				CBLogError("LOSE CHAIN AND RELAY TX DOUBLE SPEND HASH FAIL\n");
				exit(EXIT_FAILURE);
			}
		}
		doubleSpendNum++;
		maybeFinishLoseTest();
	}else if (testPhase == ORPHAN_TO_CHAIN){
		if (memcmp(txHash, CBTransactionGetHash(chainDoubleSpend), 32) != 0) {
			CBLogError("OPRHAN TO CHAIN TX DOUBLE SPEND HASH FAIL\n");
			exit(EXIT_FAILURE);
		}
		if (nodeNum == 0) {
			CBLogError("OPRHAN TO CHAIN TX DOUBLE SPEND BAD NODE\n");
			exit(EXIT_FAILURE);
		}
		doubleSpendNum++;
		maybeFinishReorgTest();
	}else{
		CBLogError("DOUBLE SPEND BAD TEST PHASE\n");
		exit(EXIT_FAILURE);
	}
	CBMutexUnlock(testMutex);
}

void transactionUnconfirmed(CBNode *, uint8_t * txHash);
void transactionUnconfirmed(CBNode * node, uint8_t * txHash){
	CBNodeFull * nodeFull = CBGetNodeFull(node);
	int nodeNum = (nodes[0] == nodeFull)? 0 : ((nodes[1] == nodeFull)? 1 : 2);
	CBMutexLock(testMutex);
	if (testPhase == CHAIN_REORGANISATION) {
		for (uint8_t x = 0;; x++) {
			if (memcmp(txHash, CBTransactionGetHash(initialTxs[x]), 32) == 0){
				if (x == 1) {
					CBLogError("CHAIN REORGANISATION TX UNCONFIRMED TX BAD TX 1\n");
					exit(EXIT_FAILURE);
				}
				if (nodeNum != (x + 1) % 3) {
					CBLogError("CHAIN REORGANISATION NEW TX %u BAD NODE NUM\n", x);
					exit(EXIT_FAILURE);
				}
				unconfirmedNum++;
				maybeFinishReorgTest();
				break;
			}
			if (x == 2) {
				CBLogError("CHAIN REORGANISATION TX UNCONFIRMED UNKNOWN TX\n");
				exit(EXIT_FAILURE);
			}
		}
	}else if (testPhase == LOSE_CHAIN_AND_RELAY){
		for (uint8_t x = 0;; x++) {
			if (memcmp(txHash, CBTransactionGetHash(doubleSpends[3 + x]), 32) == 0) {
				if ((nodeNum == 1 && x != 0) || nodeNum == 2) {
					CBLogError("LOSE CHAIN AND RELAY TX UNCONFIRMED NODE NUM FAIL\n");
					exit(EXIT_FAILURE);
				}
				break;
			}
			if (x == 2) {
				CBLogError("LOSE CHAIN AND RELAY TX UNCONFIRMED HASH FAIL\n");
				exit(EXIT_FAILURE);
			}
		}
		unconfirmedNum++;
		maybeFinishLoseTest();
	}else{
		CBLogError("TX UNCONFIRMED BAD TEST PHASE\n");
		exit(EXIT_FAILURE);
	}
	CBMutexUnlock(testMutex);
}

void CBNetworkCommunicatorTryConnectionsVoid(void * comm);
void CBNetworkCommunicatorTryConnectionsVoid(void * comm){
	CBNetworkCommunicatorTryConnections(comm, false);
}

void CBNetworkCommunicatorStartListeningVoid(void * comm);
void CBNetworkCommunicatorStartListeningVoid(void * comm){
	CBNetworkCommunicatorStartListening(comm);
}

int main(){
	puts("You may need to move your mouse around if this test stalls.");
	CBNewMutex(&testMutex);
	// Make dummy peer for callback object
	CBObject * dummyAddr = malloc(sizeof(*dummyAddr));
	CBInitObject(dummyAddr, false);
	dummyPeer = CBNewPeer(CBGetNetworkAddress(dummyAddr));
	dummyPeer->downloading = false;
	dummyPeer->allowNewBranch = true;
	dummyPeer->branchWorkingOn = CB_NO_BRANCH;
	dummyPeer->upToDate = true;
	CBReleaseObject(dummyAddr);
	// Create three nodes to talk to each other
	CBDepObject databases[3];
	CBNodeCallbacks callbacks = {
		onFatalNodeError,
		newBlock,
		newTransaction,
		transactionConfirmed,
		doubleSpend,
		transactionUnconfirmed
	};
	for (uint8_t x = 0; x < 3; x++) {
		char directory[5], filename[26];
		sprintf(directory, "./%u", x);
		mkdir(directory, S_IRWXU | S_IRWXG);
		chmod(directory, S_IRWXU | S_IRWXG);
		// Delete data
		sprintf(filename, "%s/cbitcoin/log.dat", directory);
		remove(filename);
		sprintf(filename, "%s/cbitcoin/del.dat", directory);
		remove(filename);
		sprintf(filename, "%s/cbitcoin/val_0.dat", directory);
		remove(filename);
		for (uint8_t x = 0; x < 22; x++) {
			sprintf(filename, "%s/cbitcoin/idx_%u_0.dat", directory, x);
			remove(filename);
		}
		CBNewStorageDatabase(&databases[x], directory, 10000000, 10000000);
		nodes[x] = CBNewNodeFull(databases[x], CB_NODE_CHECK_STANDARD, 100000, callbacks);
		CBByteArray * loopBack = CBNewByteArrayWithDataCopy((uint8_t [16]){0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xFF, 0xFF, 127, 0, 0, 1}, 16);
		CBNetworkAddress * addr = CBNewNetworkAddress(0, (CBSocketAddress){loopBack, 45562 + x}, 0, false);
		CBReleaseObject(loopBack);
		CBByteArray * userAgent = CBNewByteArrayFromString(CB_USER_AGENT_SEGMENT, false);
		CBNetworkCommunicator * comm = CBGetNetworkCommunicator(nodes[x]);
		comm->maxConnections = 3;
		comm->maxIncommingConnections = 3;
		comm->recvTimeOut = 0;
		comm->responseTimeOut = 0;
		comm->sendTimeOut = 0;
		comm->timeOut = 0;
		comm->heartBeat = 0;
		comm->connectionTimeOut = 0;
		CBNetworkCommunicatorSetUserAgent(comm, userAgent);
		CBNetworkCommunicatorSetOurIPv4(comm, addr);
		CBNetworkCommunicatorSetReachability(comm, CB_IP_IP4 | CB_IP_LOCAL, true);
		// Disable POW check
		CBGetNode(nodes[x])->validator->flags |= CB_VALIDATOR_DISABLE_POW_CHECK;
	}
	// Give node 0 the addresses for node 1 and 2
	CBByteArray * loopBack = CBNewByteArrayWithDataCopy((uint8_t [16]){0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xFF, 0xFF, 127, 0, 0, 1}, 16);
	CBNetworkAddress * addr = CBNewNetworkAddress(0, (CBSocketAddress){loopBack, 45563}, 0, true);
	CBNetworkAddress * addr2 = CBNewNetworkAddress(0, (CBSocketAddress){loopBack, 45564}, 0, true);
	CBReleaseObject(loopBack);
	CBNetworkAddressManagerAddAddress(CBGetNetworkCommunicator(nodes[0])->addresses, addr);
	CBNetworkAddressManagerAddAddress(CBGetNetworkCommunicator(nodes[0])->addresses, addr2);
	CBReleaseObject(addr);
	CBReleaseObject(addr2);
	// Give 1001 blocks to node 0 with last block containing four transactions. Also give 500 of them to node 1 and then give 500 other blocks to node 1.
	CBBlock * block = CBNewBlock();
	block->prevBlockHash = CBNewByteArrayWithDataCopy((uint8_t []){0x6F, 0xE2, 0x8C, 0x0A, 0xB6, 0xF1, 0xB3, 0x72, 0xC1, 0xA6, 0xA2, 0x46, 0xAE, 0x63, 0xF7, 0x4F, 0x93, 0x1E, 0x83, 0x65, 0xE1, 0x5A, 0x08, 0x9C, 0x68, 0xD6, 0x19, 0x00, 0x00, 0x00, 0x00, 0x00}, 32);
	block->time = 1231469665;
	block->target = CB_MAX_TARGET;
	block->nonce = 2573394689;
	block->transactionNum = 1;
	block->transactions = malloc(sizeof(*block->transactions));
	block->transactions[0] = CBNewTransaction(0, 1);
	CBByteArray * nullHash = CBNewByteArrayOfSize(32);
	memset(CBByteArrayGetData(nullHash), 0, 32);
	CBScript * script = CBNewScriptWithDataCopy((uint8_t []){0,0}, 2);
	CBTransactionTakeInput(block->transactions[0], CBNewTransactionInput(script, CB_TX_INPUT_FINAL, nullHash, 0xFFFFFFFF));
	CBReleaseObject(script);
	CBReleaseObject(nullHash);
	script = CBNewScriptWithDataCopy((uint8_t []){CB_SCRIPT_OP_TRUE}, 1);
	CBTransactionTakeOutput(block->transactions[0], CBNewTransactionOutput(1250000000, script));
	CBTransactionTakeOutput(block->transactions[0], CBNewTransactionOutput(1250000000, script));
	CBTransactionTakeOutput(block->transactions[0], CBNewTransactionOutput(1250000000, script));
	CBTransactionTakeOutput(block->transactions[0], CBNewTransactionOutput(1250000000, script));
	CBReleaseObject(script);
	CBTransactionMakeBytes(block->transactions[0]);
	CBTransactionSerialise(block->transactions[0], false);
	CBBlockCalculateAndSetMerkleRoot(block);
	CBGetMessage(block)->bytes = CBNewByteArrayOfSize(CBBlockCalculateLength(block, true));
	CBBlockSerialise(block, true, false);
	// Add blocks
	CBNodeFullAddBlockDirectly(nodes[0], block);
	CBNodeFullAddBlockDirectly(nodes[1], block);
	CBByteArray * firstCoinbase = CBNewByteArrayWithDataCopy(CBTransactionGetHash(block->transactions[0]), 32);
	// Add 999 more
	CBByteArray * prevToFork, * lastNode0BlockHash;
	for (uint16_t x = 0; x < 1499; x++) {
		if (x == 999)
			block->prevBlockHash = prevToFork;
		else
			block->prevBlockHash = CBNewByteArrayWithDataCopy(CBBlockGetHash(block), 32);
		block->time++;
		CBByteArraySetInt16(block->transactions[0]->inputs[0]->scriptObject, 0, x+1);
		CBTransactionSerialise(block->transactions[0], true);
		CBBlockCalculateAndSetMerkleRoot(block);
		CBBlockSerialise(block, true, true);
		if (x < 999)
			CBNodeFullAddBlockDirectly(nodes[0], block);
		if (x < 499 || x >= 999){
			CBNodeFullAddBlockDirectly(nodes[1], block);
			if (x == 498)
				prevToFork = CBNewByteArrayWithDataCopy(CBBlockGetHash(block), 32);
		}
		if (x == 998) 
			lastNode0BlockHash = CBNewByteArrayWithDataCopy(CBBlockGetHash(block), 32);
		else if (x == 1498)
			lastNode1BlockHash = CBNewByteArrayWithDataCopy(CBBlockGetHash(block), 32);
	}
	// Create three keys and add watched hash to the accounters
	for (uint8_t x = 0; x < 3; x++) {
		CBInitKeyPair(keys + x);
		CBKeyPairGenerate(keys + x);
		CBAccounterAddWatchedOutputToAccount(CBGetNode(nodes[x])->accounterStorage, CBKeyPairGetHash(keys + x), CBAccounterNewAccount(CBGetNode(nodes[x])->accounterStorage));
	}
	// Add block with fours additional transactions for node 0
	block->prevBlockHash = lastNode0BlockHash;
	block->time++;
	CBByteArraySetInt16(block->transactions[0]->inputs[0]->scriptObject, 0, 1500);
	CBTransactionSerialise(block->transactions[0], true);
	block->transactions = realloc(block->transactions, sizeof(*block->transactions)*5);
	block->transactionNum = 5;
	for (uint8_t x = 1; x < 5; x++) {
		block->transactions[x] = CBNewTransaction(0, 1);
		CBScript * script = CBNewScriptWithDataCopy((uint8_t []){0}, 1);
		CBTransactionTakeInput(block->transactions[x], CBNewTransactionInput(script, CB_TX_INPUT_FINAL, firstCoinbase, x-1));
		CBReleaseObject(script);
		script = CBNewScriptPubKeyHashOutput(CBKeyPairGetHash(keys + x % 3));
		for (uint8_t y = 0; y < 4; y++)
			CBTransactionTakeOutput(block->transactions[x], CBNewTransactionOutput(312500000, script));
		CBReleaseObject(script);
		CBTransactionMakeBytes(block->transactions[x]);
		CBTransactionSerialise(block->transactions[x], true);
		initialTxs[x-1] = block->transactions[x];
		CBRetainObject(block->transactions[x]);
	}
	CBBlockCalculateAndSetMerkleRoot(block);
	CBGetMessage(block)->bytes = CBNewByteArrayOfSize(CBBlockCalculateLength(block, true));
	CBBlockSerialise(block, true, true);
	CBNodeFullAddBlockDirectly(nodes[0], block);
	lastInitialBlock = block;
	// Broadcast transactions
	CBTransaction * deps[2];
	for (uint8_t x = 0; x < 10; x++) {
		CBTransaction * tx = CBNewTransaction(x, 1);
		CBByteArray * prev = CBNewByteArrayWithDataCopy(CBTransactionGetHash(block->transactions[x % 4 + 1]), 32);
		CBTransactionTakeInput(tx, CBNewTransactionInput(NULL, CB_TX_INPUT_FINAL, prev, x/4));
		CBReleaseObject(prev);
		// Record one that will be lost and one that wont be lost
		if (x == 3 || x == 5)
			deps[x/2-1] = tx;
		if (x >= 6){
			// Odd ones will be lost
			CBByteArray * prev = CBNewByteArrayWithDataCopy(CBTransactionGetHash(deps[x % 2]), 32);
			CBTransactionTakeInput(tx, CBNewTransactionInput(NULL, CB_TX_INPUT_FINAL, prev, (x-6)/2));
			CBReleaseObject(prev);
		}
		script = CBNewScriptPubKeyHashOutput(CBKeyPairGetHash(keys + x % 3));
		CBTransactionTakeOutput(tx, CBNewTransactionOutput(156250000 + (x >= 6)*156250000, script));
		CBTransactionTakeOutput(tx, CBNewTransactionOutput(156250000, script));
		CBReleaseObject(script);
		// Sign transaction
		CBTransactionSignPubKeyHashInput(tx, keys + (x % 4 + 1) % 3, block->transactions[x % 4 + 1]->outputs[x/4]->scriptObject, 0, CB_SIGHASH_ALL);
		if (x >= 6)
			CBTransactionSignPubKeyHashInput(tx, keys + (x % 2)*2, deps[x % 2]->outputs[(x-6)/2]->scriptObject, 1, CB_SIGHASH_ALL);
		CBTransactionMakeBytes(tx);
		CBTransactionSerialise(tx, true);
		CBNodeFullNewUnconfirmedTransaction(nodes[0], NULL, tx);
		initialTxs[x+4] = tx;
	}
	testPhase = RECEIVE_INITIAL_BLOCKS_AND_TXS;
	// Start nodes and listen on node 1 and 2
	for (uint8_t x = 3; x--;) {
		if (x > 0)
			// Block, thus ensuring nodes are listening before we try to connect.
			CBRunOnEventLoop(CBGetNetworkCommunicator(nodes[x])->eventLoop, CBNetworkCommunicatorStartListeningVoid, nodes[x], true);
		else
			CBRunOnEventLoop(CBGetNetworkCommunicator(nodes[x])->eventLoop, CBNetworkCommunicatorTryConnectionsVoid, nodes[x], false);
	}
	CBThreadJoin(((CBEventLoop *)CBGetNetworkCommunicator(nodes[0])->eventLoop.ptr)->loopThread);
	return EXIT_SUCCESS;
}
