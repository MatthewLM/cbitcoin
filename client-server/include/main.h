//
//  main.h
//  cbitcoin
//
//  Created by Matthew Mitchell on 11/12/2013.
//  Copyright (c) 2013 Matthew Mitchell
//
//  This file is part of cbitcoin. It is subject to the license terms
//  in the LICENSE file found in the top-level directory of this
//  distribution and at http://www.cbitcoin.com/license.html. No part of
//  cbitcoin, including this file, may be copied, modified, propagated,
//  or distributed except according to the terms contained in the
//  LICENSE file.

#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <pthread.h>
#include <sys/time.h>
#include <pwd.h>
#include <uuid/uuid.h>
#include "CBNodeFull.h"

// Macros

#if defined(CB_MACOSX)
	#define CB_DEFUALT_DATA_DIR "/Library/Application Support/cbitcoin-server"
#elif defined(CB_LINUX)
	#define CB_DEFUALT_DATA_DIR "/.cbitcoin-server"
#endif

#define CB_CLIENT_USER_AGENT CB_USER_AGENT_SEGMENT "client-server"
#define CB_SEED_DOMAINS (char *[]){"seed.bitcoin.sipa.be", "dnsseed.bluematt.me", "dnsseed.bitcoin.dashjr.org", "bitseed.xf2.org"}
#define CB_SEED_DOMAIN_NUM 4
#define CBInvalidArg(str) {printf("Invalid argument. " str ": %s %s\n", argv[x], argv[x+1]); return EXIT_FAILURE;}

// Structures

typedef struct CBNetworkAddressLinkedList CBNetworkAddressLinkedList;

struct CBNetworkAddressLinkedList{
	CBNetworkAddress * addr;
	CBNetworkAddressLinkedList * next;
};

// Functions

bool CBCheckNumber(uint32_t num, size_t lineNum);
void CBOnDoubleSpend(CBNode * node, uint8_t * txHash);
void CBOnNewBlock(CBNode * node, CBBlock * block, uint32_t forkPoint);
void CBOnNewTransaction(CBNode * node, CBTransaction * tx, uint64_t timestamp, uint32_t blockHeight, CBTransactionAccountDetailList * details);
void CBOnFatalNodeError(CBNode * node);
void CBOnTransactionConfirmed(CBNode * node, uint8_t * txHash, uint32_t blockHeight);
void CBOnTransactionUnconfirmed(CBNode * node, uint8_t * txHash);

CBNetworkAddress * CBReadNetworkAddress(char * ip, size_t lineNum, bool isPublic);
void CBStartNode(void * comm);
