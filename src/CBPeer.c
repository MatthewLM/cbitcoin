//
//  CBPeer.c
//  cbitcoin
//
//  Created by Matthew Mitchell on 22/07/2012.
//  Copyright (c) 2012 Matthew Mitchell
//
//  This file is part of cbitcoin. It is subject to the license terms
//  in the LICENSE file found in the top-level directory of this
//  distribution and at http://www.cbitcoin.com/license.html. No part of
//  cbitcoin, including this file, may be copied, modified, propagated,
//  or distributed except according to the terms contained in the
//  LICENSE file.

//  SEE HEADER FILE FOR DOCUMENTATION

#include "CBPeer.h"

//  Constructor

CBPeer * CBNewPeer(CBNetworkAddress * addr){
	CBPeer * self = malloc(sizeof(*self));
	CBGetObject(self)->free = CBFreePeer;
	CBInitPeer(self, addr);
	return self;
}

//  Initialiser

void CBInitPeer(CBPeer * self, CBNetworkAddress * addr){
	CBInitObject(CBGetObject(self), true);
	CBRetainObject(addr);
	self->addr = addr;
	self->receive = NULL;
	self->receivedHeader = false;
	self->handshakeStatus = CB_HANDSHAKE_NONE;
	self->versionMessage = NULL;
	self->timeOffset = 0;
	self->sentHeader = false;
	self->messageSent = 0;
	self->time = 0;
	self->connectionWorking = false;
	self->connecting = false;
	self->downloadTime = 0;
	self->downloadAmount = 0;
	self->downloadTimerStart = 0;
	self->sendQueueFront = 0;
	self->sendQueueSize = 0;
	self->messageReceived = false;
	self->allowRelay = true;
	self->disconnected = false;
	self->typeExpected = CB_MESSAGE_TYPE_NONE;
	strcpy(self->peerStr, "unknown");
}

void CBDestroyPeer(CBPeer * peer){
	CBReleaseObject(peer->addr);
}
void CBFreePeer(void * peer){
	CBDestroyPeer(peer);
	free(peer);
}
