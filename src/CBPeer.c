//
//  CBPeer.c
//  cbitcoin
//
//  Created by Matthew Mitchell on 22/07/2012.
//  Copyright (c) 2012 Matthew Mitchell
//
//  This file is part of cbitcoin.
//
//  cbitcoin is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  cbitcoin is distributed in the hope that it will be useful, 
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with cbitcoin.  If not, see <http://www.gnu.org/licenses/>.

//  SEE HEADER FILE FOR DOCUMENTATION

#include "CBPeer.h"

//  Constructor

CBPeer * CBNewPeerByTakingNetworkAddress(CBNetworkAddress * addr){
	CBPeer * self = CBGetPeer(addr);
	self = realloc(self, sizeof(*self));
	if (NOT self) {
		CBLogError("Cannot reallocate to %i bytes of memory in CBNewNodeByTakingNetworkAddress\n", sizeof(*self));
		return NULL;
	}
	if(CBInitPeerByTakingNetworkAddress(self))
		return self;
	free(self);
	return NULL;
}

//  Object Getter

CBPeer * CBGetPeer(void * self){
	return self;
}

//  Initialiser

bool CBInitPeerByTakingNetworkAddress(CBPeer * self){
	self->receive = NULL;
	self->receivedHeader = false;
	self->versionSent = false;
	self->versionAck = false;
	self->versionMessage = NULL;
	self->acceptedTypes = 0;
	self->timeOffset = 0;
	self->sentHeader = false;
	self->messageSent = 0;
	self->timeBroadcast = 0;
	self->connectionWorking = false;
	self->typesExpectedNum = 0;
	self->downloadTime = 0;
	self->downloadAmount = 0;
	self->latencyTime = 0;
	self->responses = 0;
	self->latencyTimerStart = 0;
	self->downloadTimerStart = 0;
	self->sendQueueFront = 0;
	self->sendQueueSize = 0;
	self->messageReceived = false;
	return true;
}
