//
//  CBRPCServer.c
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

#include "CBRPCServer.h"

bool CBStartRPCServer(CBRPCServer * self, CBNodeFull * node, uint16_t port){
	// Create the event loop
	if (CBNewEventLoop(&self->eventLoop, CBRPCServerOnError, CBRPCServerOnTimeout, self)) {
		// Start listening for connections on the specified port
		if (CBSocketBind(&self->socket, false, port)){
			// Add event for accepting connection for both sockets
			if (CBSocketCanAcceptEvent(&self->acceptEvent, self->eventLoop, self->socket, CBRPCServerAcceptConnection)) {
				if(CBSocketAddEvent(self->acceptEvent, 0)){ // No timeout for listening for incomming connections.
					// Start listening
					if (CBSocketListen(self->socket, 1)){
						// Prepare send event
						if (CBSocketCanSendEvent(&self->sendEvent, self->eventLoop, self->socket, CBRPCServerCanSend, NULL)){
							self->resp = NULL;
							return true;
						}else
							CBLogError("Unable to create a \"can send\" event.");
					}else
						CBLogError("Unable to start listening to incoming RPC connections.");
				}else
					CBLogError("Unable to add accept event for incoming RPC connections.");
				CBSocketFreeEvent(self->acceptEvent);
			}else
				CBLogError("Unable to create an accept event for incoming RPC connections.");
			CBCloseSocket(self->socket);
		}else
			CBLogError("Unable to bind a socket for listening to incoming RPC connections.");
		CBExitEventLoop(self->eventLoop);
	}else
		CBLogError("Could not create an event loop for the RPC server.");
	return false;
}

void CBRPCServerAcceptConnection(void * vself, CBDepObject socket){
	CBRPCServer * self = vself;
	CBSocketAddress sockAddr;
	if (! CBSocketAccept(socket, &self->connSocket, &sockAddr)){
		CBLogError("Could not accept RPC connection.");
		return;
	}
	// We want local connections.
	if (!(CBGetIPType(CBByteArrayGetData(sockAddr.ip)) & CB_IP_LOCAL)
		|| !CBRPCServerRespond(self, NULL, CB_HTTP_FORBIDDEN, false, CBRPCServerDisconnect)){
		CBRPCServerDisconnect(self);
		return;
	}
	
}
void CBRPCServerCanSend(void * vself, void * foo){
	CBRPCServer * self = vself;
	int32_t num = CBSocketSend(self->connSocket, (uint8_t *)self->resp + self->bytesSent, self->respSize - self->bytesSent);
	if (num == CB_SOCKET_FAILURE) {
		CBRPCServerDisconnect(self);
		return;
	}
	self->bytesSent += num;
	if (self->bytesSent == self->respSize && self->callback)
		self->callback(self);
}
void CBRPCServerDisconnect(void * vself){
	CBRPCServer * self = vself;
	CBCloseSocket(self->connSocket);
	free(self->resp);
}
void CBRPCServerOnError(void * self){
	
}
void CBRPCServerOnTimeout(void * self, void * foo, CBTimeOutType type){
	
}
bool CBRPCServerRespond(CBRPCServer * self, char * resp, CBRPCServerStatus status, bool keepAlive, void (*callback)(void *)){
	// Prepare response...
	char * statusStr;
	char * okStatusStr = "OK";
	char * badRequestStatusStr = "Bad Request";
	char * forbiddenStatusStr = "Forbidden";
	char * notFoundStatusStr = "Not Found";
	char * errorStatusStr = "Internal Server Error";
	// Get time
	char timeStr[64];
    time_t now = time(NULL);
    strftime(timeStr, sizeof(timeStr), "%a, %d %b %Y %H:%M:%S +0000", gmtime(&now));
	if (status == CB_HTTP_UNAUTHORIZED){
		self->respSize = asprintf(
				 &self->resp,
				 "HTTP/1.0 401 Authorization Required\r\n"
				 "Date: %s\r\n"
				 "Server: cbitcoin-json-rpc/" CB_RPC_VERSION_STRING "\r\n"
				 "WWW-Authenticate: Basic realm=\"jsonrpc\"\r\n"
				 "Content-Type: text/plain\r\n"
				 "Content-Length: 0\r\n"
				 "\r\n"
				 , timeStr
		);
	}else{
		if (status == CB_HTTP_OK)
			statusStr = okStatusStr;
		else if (status == CB_HTTP_BAD_REQUEST)
			statusStr = badRequestStatusStr;
		else if (status == CB_HTTP_FORBIDDEN)
			statusStr = forbiddenStatusStr;
		else if (status == CB_HTTP_NOT_FOUND)
			statusStr = notFoundStatusStr;
		else
			statusStr = errorStatusStr;
		self->respSize = asprintf(&self->resp,
				 "HTTP/1.1 %d %s\r\n"
				 "Date: %s\r\n"
				 "Connection: %s\r\n"
				 "Content-Length: %zu\r\n"
				 "Content-Type: application/json\r\n"
				 "Server: cbitcoin-json-rpc/" CB_RPC_VERSION_STRING "\r\n"
				 "\r\n"
				 "%s",
				 status,
				 statusStr,
				 timeStr,
				 keepAlive ? "keep-alive" : "close",
				 strlen(resp),
				 resp
		);
	}
	self->callback = callback;
	self->bytesSent = 0;
	// Add send event
	if(!CBSocketAddEvent(self->sendEvent, 1000))
		return false;
	return true;
}
