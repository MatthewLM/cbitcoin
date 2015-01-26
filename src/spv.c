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
#include "nodeheadersonly.h"
#include "CBLibEventSockets.h"
#include <event2/thread.h>
#include <time.h>
#include "stdarg.h"
#include <sys/stat.h>
#include "CBDependencies.h"
#include "CBHDKeys.h"
#include "checkTransactions.h"

void onFatalNodeError(CBNode * node, CBErrorReason reason);
void onFatalNodeError(CBNode * node, CBErrorReason reason){
	CBLogError("ON FATAL NODE ERROR %u\n", reason);
	exit(EXIT_FAILURE);
}

void newBlock(CBNode *, CBBlock * block, uint32_t forkPoint);
void newBlock(CBNode * node, CBBlock * block, uint32_t forkPoint){
	CBLogError("New Block");
	exit(EXIT_FAILURE);
}
void newTransaction(CBNode *, CBTransaction * tx, uint64_t timestamp, uint32_t blockHeight, CBTransactionAccountDetailList * details);
void newTransaction(CBNode * node, CBTransaction * tx, uint64_t timestamp, uint32_t blockHeight, CBTransactionAccountDetailList * details){
	CBLogError("New Transaction");
	exit(EXIT_FAILURE);
}
void transactionConfirmed(CBNode *, uint8_t * txHash, uint32_t blockHeight);
void transactionConfirmed(CBNode * node, uint8_t * txHash, uint32_t blockHeight){
	CBLogError("Transaction Confirmed");
	exit(EXIT_FAILURE);
}

void doubleSpend(CBNode *, uint8_t * txHash);
void doubleSpend(CBNode * node, uint8_t * txHash){
	CBLogError("Double spend");
	exit(EXIT_FAILURE);
}

void transactionUnconfirmed(CBNode *, uint8_t * txHash);
void transactionUnconfirmed(CBNode * node, uint8_t * txHash){
	CBLogError("Transaction Unconfirmed");
	exit(EXIT_FAILURE);
}

void uptodate(CBNode * node, bool uptodate);
void uptodate(CBNode * node, bool uptodate){
	CBLogError("Up to date");
	exit(EXIT_FAILURE);
}

int main() {

	puts("You may need to move your mouse around if this test stalls.");
	CBDepObject databases[1];
	CBNodeCallbacks callbacks = {
		onFatalNodeError,
		newBlock,
		newTransaction,
		transactionConfirmed,
		doubleSpend,
		transactionUnconfirmed,
		uptodate
	};

	return EXIT_SUCCESS;
}
