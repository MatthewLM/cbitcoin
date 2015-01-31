//
//  main.c
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

#include "spv.h"


CBPeer * SPVcreateNewPeer(char * ipAddr, bool isPublic){
	return CBNewPeer(CBReadNetworkAddress(ipAddr, isPublic));
}
/*
CBMessage * CBFDgetVersion(CBNetworkCommunicator *self, CBNetworkAddress * peer){

}
*/

CBNetworkCommunicator * SPVcreateSelf(CBNetworkAddress * selfAddress){
	CBNetworkCommunicator * self;
	CBNetworkCommunicatorCallbacks callbacks = {
		onPeerWhatever,
		acceptType,
		onMessageReceived,
		onNetworkError
	};
	self = CBNewNetworkCommunicator(0, callbacks);
	self->networkID = CB_PRODUCTION_NETWORK_BYTES;
	self->flags = CB_NETWORK_COMMUNICATOR_AUTO_HANDSHAKE | CB_NETWORK_COMMUNICATOR_AUTO_PING | CB_NETWORK_COMMUNICATOR_AUTO_DISCOVERY;
	self->version = CB_PONG_VERSION;
	self->maxConnections = 2;
	self->maxIncommingConnections = 0;
	self->heartBeat = 2000;
	self->timeOut = 3000;
	self->recvTimeOut = 1000;

	self->ipData[CB_TOR_NETWORK].isSet = false;
	self->ipData[CB_I2P_NETWORK].isSet = false;
	self->ipData[CB_IP6_NETWORK].isSet = false;

	self->ipData[CB_IP4_NETWORK].isSet = true;
	self->ipData[CB_IP4_NETWORK].isListening = false;
	self->ipData[CB_IP4_NETWORK].ourAddress = selfAddress;

	return self;

}

CBVersion * CBNetworkCommunicatorGetVersion(CBNetworkCommunicator * self, CBNetworkAddress * addRecv){
	CBNetworkAddress * sourceAddr = CBNetworkCommunicatorGetOurMainAddress(self, addRecv->type);
	self->nonce = rand();
	// If the peer's address is local give a null address
	if (addRecv->type & CB_IP_LOCAL) {
		CBByteArray * ip = CBNewByteArrayWithDataCopy(CB_NULL_ADDRESS, 16);
		addRecv = CBNewNetworkAddress(0, (CBSocketAddress){ip, 0}, 0, false);
		CBReleaseObject(ip);
	}else
		CBRetainObject(addRecv);
	CBVersion * version = CBNewVersion(self->version, self->services, time(NULL), addRecv, sourceAddr, self->nonce, self->userAgent, self->blockHeight);
	CBReleaseObject(addRecv);
	return version;
}

int main(int argc, char * argv[]) {
	fprintf(stderr, "Error: Hello\n");
	// Get home directory and set the default data directory.
    // hi
	CBPeer * peer = SPVcreateNewPeer("10.21.0.189",false);


	CBNetworkCommunicator *self = SPVcreateSelf(CBReadNetworkAddress("10.21.0.67", false));
	CBMessage *msg = CBNetworkCommunicatorGetVersion(self,peer->addr);


	//fprintf(stderr, "Error: Size(%d) 4\n",CBGetMessage(version)->bytes->length);

//CBNetworkCommunicatorSendMessage, then see CBNetworkCommunicatorOnCanSend

}
