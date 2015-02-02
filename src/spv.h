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
#include <sys/types.h>
#include <mqueue.h>
#include <netdb.h>
#include <pthread.h>
#include <sys/time.h>
#include <pwd.h>
#include "common.h"
#include "CBDependencies.h"
#include "CBNodeFull.h"
#include "CBRPCServer.h"
#include "CBVersion.h"
#include "CBPeer.h"

// Macros


#define CB_DEFUALT_DATA_DIR "/.cbitcoin-server"


#define CB_CLIENT_USER_AGENT CB_USER_AGENT_SEGMENT "client-server"
#define CBInvalidArg(str) {printf("Invalid argument. " str ": %s %s\n", argv[x], argv[x+1]); return EXIT_FAILURE;}

/*
 * Message Handling
 */
bool SPVsendMessage(CBNetworkCommunicator * self, CBPeer * peer, CBMessage * message);
bool SPVsendMessageViaPeer(CBNetworkCommunicator *self,CBPeer *peer, CBMessage *toSend);
bool SPVreceiveMessageHeader(CBNetworkCommunicator * self, CBPeer * peer);
bool SPVreadHeader(CBNetworkCommunicator *self, CBPeer * peer);


CBNetworkAddress * CBReadNetworkAddress(char * ip, bool isPublic);

// call backs
CBOnMessageReceivedAction onMessageReceived(CBNetworkCommunicator * comm, CBPeer * peer, CBMessage * theMessage);

bool acceptType(CBNetworkCommunicator *, CBPeer *, CBMessageType);

void onBadTime(void * foo);

void onNetworkError(CBNetworkCommunicator * comm, CBErrorReason reason);

void CBNetworkCommunicatorTryConnectionsVoid(void * comm);

void onPeerWhatever(CBNetworkCommunicator * foo, CBPeer * bar);


