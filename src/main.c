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


CBPeer * SPVcreateNewPeer(CBNetworkAddress *addr){
	CBPeer *peer = CBNewPeer(addr);
	peer->sendQueueSize = 1;
	peer->connectionWorking = 1;

	return peer;
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
	self->services = CB_SERVICE_NO_FULL_BLOCKS;
	self->nonce = rand();

	self->blockHeight = 0;


	//CBNetworkAddressManager * addrManager = CBNewNetworkAddressManager(onBadTime);
	//addrManager->maxAddressesInBucket = 2;
	CBByteArray * userAgent = CBNewByteArrayFromString(CB_USER_AGENT_SEGMENT, false);

	CBNetworkCommunicatorSetReachability(self, CB_IP_IP4 | CB_IP_LOCAL, true);
	CBNetworkCommunicatorSetAlternativeMessages(self, NULL, NULL);
	//CBNetworkCommunicatorSetNetworkAddressManager(self, addrManager);
	CBNetworkCommunicatorSetUserAgent(self, userAgent);
	CBNetworkCommunicatorSetOurIPv4(self, selfAddress);

	self->ipData[CB_TOR_NETWORK].isSet = false;
	self->ipData[CB_I2P_NETWORK].isSet = false;
	self->ipData[CB_IP6_NETWORK].isSet = false;

	//self->ipData[CB_IP4_NETWORK].isSet = true;
	//self->ipData[CB_IP4_NETWORK].isListening = false;
	//self->ipData[CB_IP4_NETWORK].ourAddress = selfAddress;

	//CBReleaseObject(userAgent);

	return self;

}

CBVersion * SPVNetworkCommunicatorGetVersion(CBNetworkCommunicator * self,CBPeer *peer){
	CBNetworkAddress * addRecv = peer->addr;
	CBNetworkAddress * sourceAddr = CBNetworkCommunicatorGetOurMainAddress(self, addRecv->type);
	//CBNetworkAddress * sourceAddr = self->ipData[CB_IP4_NETWORK].ourAddress;
	char x[300];
	CBNetworkAddressToString(addRecv,x);
	fprintf(stderr,"Peer Address: %s \n",x);
	CBNetworkAddressToString(sourceAddr,x);
	fprintf(stderr,"Our Address: %s \n",x);


	// If the peer's address is local give a null address
	if (addRecv->type & CB_IP_LOCAL) {
		CBByteArray * ip = CBNewByteArrayWithDataCopy(CB_NULL_ADDRESS, 16);
		addRecv = CBNewNetworkAddress(0, (CBSocketAddress){ip, 0}, 0, true);
		CBReleaseObject(ip);
	}else
		CBRetainObject(addRecv);


	CBVersion * version = CBNewVersion(
			self->version, self->services, time(NULL),
			sourceAddr, addRecv , self->nonce, self->userAgent,
			self->blockHeight
	);

	//CBReleaseObject(addRecv);
	//CBVersion *version;
	return version;
}



int main(int argc, char * argv[]) {
	fprintf(stderr, "Error: Hello\n");
	// Get home directory and set the default data directory.
    // hi
	// create peer ip address
	CBByteArray * peeraddr = CBNewByteArrayWithDataCopy((uint8_t [16]){0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xFF, 0xFF, 10, 21, 0, 189}, 16);
	CBByteArray * selfaddr = CBNewByteArrayWithDataCopy((uint8_t [16]){0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xFF, 0xFF, 10, 21, 0, 67}, 16);
	CBNetworkCommunicator *self = SPVcreateSelf(CBNewNetworkAddress(0, (CBSocketAddress){selfaddr, 8333}, 0, false));
	CBPeer * peer = SPVcreateNewPeer(CBNewNetworkAddress(0, (CBSocketAddress){peeraddr, 8333}, 0, false));
	//CBReleaseObject(peeraddr);
	//CBReleaseObject(selfaddr);

	CBVersion *version = SPVNetworkCommunicatorGetVersion(self,peer);
	CBMessage *msg = CBGetMessage(version);

	// bool SPVsendMessage(CBNetworkCommunicator * self, CBPeer * peer, CBMessage * message);
	char verstr[CBVersionStringMaxSize(CBGetVersion(msg))];
	CBVersionToString(CBGetVersion(msg), verstr);
	fprintf(stderr,"main hello 3\n---\n%s\n---\n",verstr);

	SPVsendMessage(self,peer,msg);

	//fprintf(stderr, "Error: Size(%d) 4\n",msg->bytes->length);
	//return 0;
	//CBNetworkCommunicatorSendMessage, then see CBNetworkCommunicatorOnCanSend
	int x;
	uint8_t buf[24];
	fprintf(stderr,"main hello outside forza \n");
	if(SPVreceiveMessageHeader(self,peer)){
		fprintf(stderr,"main hello inside\n");
		return 1;
	}
	fprintf(stderr,"main hello 4\n");
}
