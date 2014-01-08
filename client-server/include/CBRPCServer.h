//
//  CBRPCServer.h
//  cbitcoin
//
//  Created by Matthew Mitchell on 20/12/2013.
//  Copyright (c) 2013 Matthew Mitchell
//
//  This file is part of cbitcoin. It is subject to the license terms
//  in the LICENSE file found in the top-level directory of this
//  distribution and at http://www.cbitcoin.com/license.html. No part of
//  cbitcoin, including this file, may be copied, modified, propagated,
//  or distributed except according to the terms contained in the
//  LICENSE file.

#include "CBNodeFull.h"
#include "asprintf.h"
#include <arpa/inet.h>
#include <stdio.h>

#define CB_RPC_VERSION_STRING "1.0-pre-alpha"

typedef enum{
	CB_HTTP_OK                    = 200,
    CB_HTTP_BAD_REQUEST           = 400,
    CB_HTTP_UNAUTHORIZED          = 401,
    CB_HTTP_FORBIDDEN             = 403,
    CB_HTTP_NOT_FOUND             = 404,
    CB_HTTP_INTERNAL_SERVER_ERROR = 500,
} CBRPCServerStatus;

typedef struct{
	CBNodeFull node;
	CBDepObject eventLoop;
	CBDepObject socket;
	CBDepObject acceptEvent;
	CBDepObject connSocket;
	CBDepObject sendEvent;
	char * resp;
	void (*callback)(void *);
	uint32_t bytesSent;
	uint32_t respSize;
} CBRPCServer;

bool CBStartRPCServer(CBRPCServer * self, CBNodeFull * node, uint16_t port);

void CBRPCServerAcceptConnection(void * self, CBDepObject socket);
void CBRPCServerCanSend(void * self, void * foo);
void CBRPCServerDisconnect(void * self);
void CBRPCServerOnError(void * self);
void CBRPCServerOnTimeout(void * self, void *, CBTimeOutType type);
bool CBRPCServerRespond(CBRPCServer * self, char * resp, CBRPCServerStatus status, bool keepAlive, void (*callback)(void *));
