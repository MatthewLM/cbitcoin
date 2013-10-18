//
//  testCBNodeFull.c
//  cbitcoin
//
//  Created by Matthew Mitchell on 30/09/2013.
//  Copyright (c) 2012 Matthew Mitchell
//
//  This file is part of cbitcoin. It is subject to the license terms
//  in the LICENSE file found in the top-level directory of this
//  distribution and at http://www.cbitcoin.com/license.html. No part of
//  cbitcoin, including this file, may be copied, modified, propagated,
//  or distributed except according to the terms contained in the
//  LICENSE file.

#include <stdio.h>
#include "CBNodeFull.h"
#include "CBLibEventSockets.h"
#include <event2/thread.h>
#include <time.h>
#include "stdarg.h"
#include <sys/stat.h>
#include "CBDependencies.h"

enum{
	CREATING_INITIAL_BLOCKS,
	RECEIVE_INITIAL_BLOCKS_AND_TXS
} testPhase = CREATING_INITIAL_BLOCKS;

enum{
	NODE1_GOT = 1,
	NODE2_GOT = 2,
	NODE1_FORK = 4,
	COMPLETE_RECEIVE_INITAL = 7
} receiveInitialBlocksAndTxs = 0;
int gotTxNum = 0;

CBNodeFull * nodes[3];
CBDepObject keyPairs[3];
uint8_t addrs[3][20];
CBTransaction * initialTxs[14];

pthread_mutex_t completeProcessMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t completeProcessCond = PTHREAD_COND_INITIALIZER;

uint64_t CBGetMilliseconds(void){
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

void onFatalNodeError(CBNode * node);
void onFatalNodeError(CBNode * node){
	CBLogError("ON FATAL NODE ERROR\n");
	exit(EXIT_FAILURE);
}

void onBadTime(void * foo);
void onBadTime(void * foo){
	CBLogError("BAD TIME FAIL\n");
	exit(EXIT_FAILURE);
}

void finishReceiveInitialTest(void);
void finishReceiveInitialTest(void){
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
	bool nodeOwns[3][14] = {
		{0, 0, 1, 0, 1, 0, 1, 1, 0, 0, 1, 0, 1, 1},
		{1, 0, 0, 1, 1, 1, 0, 1, 1, 0, 0, 1, 1, 0},
		{0, 1, 0, 0, 0, 0, 1, 0, 0, 1, 0, 1, 1, 1}
	};
	for (uint8_t x = 0; x < 3; x++) {
		CBNodeFull * node = nodes[x];
		if (CBGetNetworkCommunicator(node)->blockHeight != 1001) {
			CBLogError("RECEIVE INITIAL BLOCKS AND TXS FINISH BLOCK HEIGHT FAIL\n");
			exit(EXIT_FAILURE);
		}
		for (CBAssociativeArray * array = &node->ourTxs;; array = &node->otherTxs) {
			CBAssociativeArrayForEach(CBFoundTransaction * fndTx, array){
				if (fndTx->utx.type != (array == &node->ourTxs) ? CB_TX_OURS : CB_TX_OTHER) {
					CBLogError("RECEIVE INITIAL BLOCKS AND TXS FINISH UTX TYPE FAIL\n");
					exit(EXIT_FAILURE);
				}
				for (uint8_t y = 4;; y++) {
					if (memcmp(CBTransactionGetHash(fndTx->utx.tx), CBTransactionGetHash(initialTxs[y]), 32) == 0) {
						if (!((y > 10) ^ (fndTx->utx.numUnconfDeps != 0))) {
							CBLogError("RECEIVE INITIAL BLOCKS AND TXS FINISH NUM UNCONF DEPS FAIL\n");
							exit(EXIT_FAILURE);
						}
						if (!((array == &node->ourTxs) ^ nodeOwns[x][y])) {
							CBLogError("RECEIVE INITIAL BLOCKS AND TXS FINISH OWNERSHIP FAIL\n");
							exit(EXIT_FAILURE);
						}
					}
					if (y == 13) {
						CBLogError("RECEIVE INITIAL BLOCKS AND TXS FINISH UNKNOWN TX\n");
						exit(EXIT_FAILURE);
					}
				}
			}
			if (array == &node->otherTxs)
				break;
		}
	}
}

void newBlock(CBNode *, CBBlock * block, uint32_t height, uint32_t forkPoint);
void newBlock(CBNode * node, CBBlock * block, uint32_t height, uint32_t forkPoint){
	CBNodeFull * nodeFull = CBGetNodeFull(node);
	int nodeNum = (nodes[0] == nodeFull)? 0 : ((nodes[1] == nodeFull)? 1 : 2);
	if (testPhase == RECEIVE_INITIAL_BLOCKS_AND_TXS) {
		if (nodeNum == 0) {
			CBLogError("RECEIVE INITIAL BLOCKS AND TXS NEW BLOCK SENDING NODE\n");
			exit(EXIT_FAILURE);
		}
		if (forkPoint == 500) {
			if (nodeNum != 1) {
				CBLogError("RECEIVE INITIAL BLOCKS AND TXS NEW BLOCK FORK NOT IN NODE 1\n");
				exit(EXIT_FAILURE);
			}
			receiveInitialBlocksAndTxs |= NODE1_FORK;
		}else if (forkPoint != CB_NO_FORK) {
			CBLogError("RECEIVE INITIAL BLOCKS AND TXS NEW BLOCK FORK\n");
			exit(EXIT_FAILURE);
		}
		if (receiveInitialBlocksAndTxs | nodeNum) {
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
			if (receiveInitialBlocksAndTxs == COMPLETE_RECEIVE_INITAL && gotTxNum == 14) {
				// Got the blocks and transactions, check node data
				finishReceiveInitialTest();
			}
		}
	}
}

void newTransaction(CBNode *, CBTransaction * tx, uint64_t timestamp, uint32_t blockHeight, CBTransactionAccountDetailList * details);
void newTransaction(CBNode * node, CBTransaction * tx, uint64_t timestamp, uint32_t blockHeight, CBTransactionAccountDetailList * details){
	CBNodeFull * nodeFull = CBGetNodeFull(node);
	int nodeNum = (nodes[0] == nodeFull)? 0 : ((nodes[1] == nodeFull)? 1 : 2);
	if (testPhase == RECEIVE_INITIAL_BLOCKS_AND_TXS) {
		if (gotTxNum == 14) {
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
			if (timestamp != 1231470665) {
				CBLogError("RECEIVE INITIAL BLOCKS AND TXS NEW TX TIMESTAMP 1231470665 FAIL\n");
				exit(EXIT_FAILURE);
			}
			// Should be got by node (txNum + 1) % 3
			uint8_t expectedNode = (txNum + 1) % 3;
			if (nodeNum != expectedNode) {
				CBLogError("RECEIVE INITIAL BLOCKS AND TXS NEW TX %u BAD NODE NUM\n", txNum);
				exit(EXIT_FAILURE);
			}
			if (details->accountID != expectedNode + 1){
				CBLogError("RECEIVE INITIAL BLOCKS AND TXS NEW TX %u BAD ACCOUNT ID NUM\n", txNum);
				exit(EXIT_FAILURE);
			}
			if (memcmp(details->accountTxDetails.addrHash, addrs[expectedNode], 20)){
				CBLogError("RECEIVE INITIAL BLOCKS AND TXS NEW TX %u BAD ADDR HASH\n",txNum);
				exit(EXIT_FAILURE);
			}
			if (details->accountTxDetails.amount != 625000000) {
				CBLogError("RECEIVE INITIAL BLOCKS AND TXS NEW TX %u BAD AMOUNT\n", txNum);
				exit(EXIT_FAILURE);
			}
			if (details->next != NULL) {
				CBLogError("RECEIVE INITIAL BLOCKS AND TXS NEW TX %u END FAIL\n", txNum);
				exit(EXIT_FAILURE);
			}
		}else{
			if (blockHeight != CB_NO_BRANCH) {
				CBLogError("RECEIVE INITIAL BLOCKS AND TXS NEW TX BLOCKHEIGHT UNCONF FAIL\n");
				exit(EXIT_FAILURE);
			}
			uint8_t expectedNodes[2] = {(((txNum - 4) % 4) + 1) % 3, (txNum - 4) % 3};
			bool checkUnconf = false;
			for (uint8_t x = 0; x < 2; x++) {
				if (nodeNum != expectedNodes[x]) {
					if (x == 1) {
						if (txNum < 6) {
							CBLogError("RECEIVE INITIAL BLOCKS AND TXS NEW TX %u NOT IN EXPECTED NODES\n", txNum);
							exit(EXIT_FAILURE);
						}
						// Check also the unconf dependency
						checkUnconf = true;
					}
					continue;
				}
				if (details->accountID != expectedNodes[x] + 1){
					CBLogError("RECEIVE INITIAL BLOCKS AND TXS NEW TX %u BAD ACCOUNT ID NUM\n", txNum);
					exit(EXIT_FAILURE);
				}
				if (memcmp(details->accountTxDetails.addrHash, addrs[expectedNodes[x]], 20)){
					CBLogError("RECEIVE INITIAL BLOCKS AND TXS NEW TX %u BAD ADDR HASH\n",txNum);
					exit(EXIT_FAILURE);
				}
				if (details->accountTxDetails.amount != 312500000*(1-(x == 0)*2)) {
					CBLogError("RECEIVE INITIAL BLOCKS AND TXS NEW TX %u BAD AMOUNT\n", txNum);
					exit(EXIT_FAILURE);
				}
				if (x == 0 && txNum >= 4 && txNum <= 7) {
					if (details->next == NULL) {
						CBLogError("RECEIVE INITIAL BLOCKS AND TXS NEW TX %u NOT NEXT IN DETAILS\n", txNum);
						exit(EXIT_FAILURE);
					}
					details = details->next;
				}else{
					if (txNum != 6) {
						if (details->next != NULL) {
							CBLogError("RECEIVE INITIAL BLOCKS AND TXS NEW TX %u END FAIL\n", txNum);
							exit(EXIT_FAILURE);
						}
					}else{
						if (details->next == NULL) {
							CBLogError("RECEIVE INITIAL BLOCKS AND TXS NEW TX %u NOT NEXT IN DETAILS\n", txNum);
							exit(EXIT_FAILURE);
						}
						details = details->next;
						checkUnconf = true;
					}
				}
			}
			if (checkUnconf) {
				uint8_t expectedNode = (txNum % 2)*2 + 3;
				if (details->accountID != expectedNode + 1){
					CBLogError("RECEIVE INITIAL BLOCKS AND TXS NEW TX %u UNCONF BAD ACCOUNT ID NUM\n", txNum);
					exit(EXIT_FAILURE);
				}
				if (memcmp(details->accountTxDetails.addrHash, addrs[expectedNode], 20)){
					CBLogError("RECEIVE INITIAL BLOCKS AND TXS NEW TX %u UNCONF BAD ADDR HASH\n",txNum);
					exit(EXIT_FAILURE);
				}
				if (details->accountTxDetails.amount != -156250000) {
					CBLogError("RECEIVE INITIAL BLOCKS AND TXS NEW TX %u UNCONF BAD AMOUNT\n", txNum);
					exit(EXIT_FAILURE);
				}
				if (details->next != NULL) {
					CBLogError("RECEIVE INITIAL BLOCKS AND TXS NEW TX %u UNCONF END FAIL\n", txNum);
					exit(EXIT_FAILURE);
				}
			}
		}
		gotTxNum++;
		if (gotTxNum == 14 && receiveInitialBlocksAndTxs == COMPLETE_RECEIVE_INITAL)
			finishReceiveInitialTest();
	}
}

void transactionConfirmed(CBNode *, uint8_t * txHash, uint32_t blockHeight);
void transactionConfirmed(CBNode * node, uint8_t * txHash, uint32_t blockHeight){
	
}

void doubleSpend(CBNode *, uint8_t * txHash);
void doubleSpend(CBNode * node, uint8_t * txHash){
	
}

void transactionUnconfirmed(CBNode *, uint8_t * txHash);
void transactionUnconfirmed(CBNode * node, uint8_t * txHash){
	
}

int main(){
	puts("You may need to move your mouse around if this test stalls.");
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
		CBNetworkAddress * addr = CBNewNetworkAddress(0, loopBack, 45562 + x, 0, false);
		CBReleaseObject(loopBack);
		CBByteArray * userAgent = CBNewByteArrayFromString(CB_USER_AGENT_SEGMENT, false);
		CBNetworkCommunicator * comm = CBGetNetworkCommunicator(nodes[x]);
		comm->networkID = CB_PRODUCTION_NETWORK_BYTES;
		comm->maxConnections = 3;
		comm->maxIncommingConnections = 3;
		comm->heartBeat = 1000;
		comm->timeOut = 2000;
		CBNetworkCommunicatorSetUserAgent(comm, userAgent);
		CBNetworkCommunicatorSetOurIPv4(comm, addr);
		CBNetworkCommunicatorSetReachability(comm, CB_IP_IP4 | CB_IP_LOCAL, true);
		// Disbable POW check
		CBGetNode(nodes[x])->validator->flags |= CB_VALIDATOR_DISABLE_POW_CHECK;
	}
	// Give node 0 the addresses for node 1 and 2
	CBByteArray * loopBack = CBNewByteArrayWithDataCopy((uint8_t [16]){0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xFF, 0xFF, 127, 0, 0, 1}, 16);
	CBNetworkAddress * addr = CBNewNetworkAddress(0, loopBack, 45563, 0, true);
	CBNetworkAddress * addr2 = CBNewNetworkAddress(0, loopBack, 45564, 0, true);
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
	script = CBNewScriptWithDataCopy((uint8_t []){CB_SCRIPT_TRUE}, 1);
	CBTransactionTakeOutput(block->transactions[0], CBNewTransactionOutput(1250000000, script));
	CBTransactionTakeOutput(block->transactions[0], CBNewTransactionOutput(1250000000, script));
	CBTransactionTakeOutput(block->transactions[0], CBNewTransactionOutput(1250000000, script));
	CBTransactionTakeOutput(block->transactions[0], CBNewTransactionOutput(1250000000, script));
	CBReleaseObject(script);
	CBGetMessage(block->transactions[0])->bytes = CBNewByteArrayOfSize(CBTransactionCalculateLength(block->transactions[0]));
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
	}
	// Create three keys and add watched hash to the accounters
	for (uint8_t x = 0; x < 3; x++) {
		CBNewKeyPair(keyPairs + x);
		uint8_t keySize = CBKeyPairGetPublicKeySize(keyPairs[x]);
		uint8_t pubKey[keySize];
		CBKeyPairGetPublicKey(keyPairs[x], pubKey);
		uint8_t hash[32];
		CBSha256(pubKey, keySize, hash);
		CBSha160(hash, 32, addrs[x]);
		CBAccounterAddWatchedOutputToAccount(CBGetNode(nodes[x])->accounterStorage, addrs[x], CBAccounterNewAccount(CBGetNode(nodes[x])->accounterStorage));
	}
	// Add block with fours additional transactions for node 0
	block->prevBlockHash = lastNode0BlockHash;
	block->time++;
	CBByteArraySetInt16(block->transactions[0]->inputs[0]->scriptObject, 0, 1500);
	CBTransactionSerialise(block->transactions[0], true);
	block->transactions = realloc(block->transactions, sizeof(*block->transactions)*5);
	for (uint8_t x = 1; x < 5; x++) {
		block->transactions[x] = CBNewTransaction(0, 1);
		CBScript * script = CBNewScriptWithDataCopy((uint8_t []){0}, 1);
		CBTransactionTakeInput(block->transactions[x], CBNewTransactionInput(script, CB_TX_INPUT_FINAL, firstCoinbase, x-1));
		CBReleaseObject(script);
		script = CBNewScriptPubKeyHash(addrs[x % 3]);
		for (uint8_t y = 0; y < 4; y++)
			CBTransactionTakeOutput(block->transactions[x], CBNewTransactionOutput(312500000, script));
		CBReleaseObject(script);
		CBGetMessage(block->transactions[x])->bytes = CBNewByteArrayOfSize(CBTransactionCalculateLength(block->transactions[x]));
		CBTransactionSerialise(block->transactions[x], true);
		initialTxs[x-1] = block->transactions[x];
	}
	CBBlockCalculateAndSetMerkleRoot(block);
	CBGetMessage(block)->bytes = CBNewByteArrayOfSize(CBBlockCalculateLength(block, true));
	CBBlockSerialise(block, true, true);
	CBNodeFullAddBlockDirectly(nodes[0], block);
	CBGetNetworkCommunicator(nodes[0])->blockHeight = 1001;
	CBGetNetworkCommunicator(nodes[1])->blockHeight = 1000;
	// Broadcast transactions
	CBTransaction * deps[2];
	for (uint8_t x = 0; x < 10; x++) {
		CBTransaction * tx = CBNewTransaction(0, 1);
		// Transaction 0 and 1 will be lost
		CBByteArray * prev = CBNewByteArrayWithDataCopy(CBTransactionGetHash(block->transactions[x % 4 + 1]), 32);
		CBTransactionTakeInput(tx, CBNewTransactionInput(NULL, CB_TX_INPUT_FINAL, prev, x/4));
		CBReleaseObject(prev);
		// Record one that will be lost and one that wont be lost
		if (x == 3 || x == 5)
			deps[x/2-1] = tx;
		if (x >= 6){
			// Odd ones will be lost
			CBByteArray * prev = CBNewByteArrayWithDataCopy(CBTransactionGetHash(deps[x % 2]), 32);
			CBTransactionTakeInput(tx, CBNewTransactionInput(NULL, CB_TX_INPUT_FINAL, prev, x/2));
			CBReleaseObject(prev);
		}
		script = CBNewScriptPubKeyHash(addrs[x % 3]);
		CBTransactionTakeOutput(tx, CBNewTransactionOutput(156250000*(1 + (x >= 6)), script));
		CBTransactionTakeOutput(tx, CBNewTransactionOutput(156250000*(1 + (x >= 6)), script));
		CBReleaseObject(script);
		// Sign transaction
		CBTransactionSignInput(tx, keyPairs[(x % 4 + 1) % 3], block->transactions[x % 4 + 1]->outputs[x/4]->scriptObject, 0, CB_SIGHASH_ALL);
		if (x >= 6)
			CBTransactionSignInput(tx, keyPairs[(x % 2)*2], deps[x % 2]->outputs[(x-6)/2]->scriptObject, 1, CB_SIGHASH_ALL);
		CBGetMessage(tx)->bytes = CBNewByteArrayOfSize(CBTransactionCalculateLength(tx));
		CBTransactionSerialise(tx, false);
		CBNodeFullBroadcastTransaction(nodes[0], tx, x >= 6, 312500000 + (x >= 6)*156250000);
		initialTxs[x+4] = tx;
	}
	testPhase = RECEIVE_INITIAL_BLOCKS_AND_TXS;
	// Start nodes and listen on node 1 and 2
	for (uint8_t x = 3; x--;) {
		CBNetworkCommunicatorStart(CBGetNetworkCommunicator(nodes[x]));
		if (x > 0)
			CBNetworkCommunicatorStartListening(CBGetNetworkCommunicator(nodes[x]));
		else
			CBNetworkCommunicatorTryConnections(CBGetNetworkCommunicator(nodes[x]));
	}
	pthread_exit(NULL);
}
