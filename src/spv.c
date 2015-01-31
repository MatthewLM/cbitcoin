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



CBNetworkAddress * CBReadNetworkAddress(char * ipStr, bool isPublic) {

	CBSocketAddress saddr = {NULL, 8333};
	char * portStart;

	// Determine if dealing with an IPv6 address
	if (ipStr[0] == '[') {
		struct in6_addr ip;
		char * bracket = strstr(ipStr, "]");
		if (!bracket){
			CBLogWarning("No closing bracket in IPv6 address");
			return NULL;
		}
		*bracket = '\0';
		if (inet_pton(AF_INET6, ipStr + 1, &ip) != 1) {
			CBLogWarning("Unable to obtain IPv6 address");
			return NULL;
		}
		*bracket = ']';
		saddr.ip = CBNewByteArrayWithDataCopy(ip.s6_addr, 16);
		portStart = strstr(ipStr, "]:");
		if (portStart) portStart += 2;
	}else{
		// Else IPv4
		struct in_addr ip;
		if (inet_pton(AF_INET, ipStr, &ip) != 1) {
			CBLogWarning("Unable to obtain IPv4 address");
			return NULL;
		}
		saddr.ip = CBNewByteArrayWithDataCopy((uint8_t [16]){0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xFF, 0xFF, 0, 0, 0, 0}, 16);
		CBInt32ToArrayBigEndian(CBByteArrayGetData(saddr.ip), 12, ip.s_addr);
		portStart = strchr(ipStr, ':');
		if (portStart) portStart++;
	}

	if (portStart){
		saddr.port = strtoul(portStart, NULL, 10);
		if (saddr.port == 0) {
			CBReleaseObject(saddr.ip);
			return NULL;
		}
	}

	CBNetworkAddress * addr = CBNewNetworkAddress(0, saddr, 0, isPublic);
	CBReleaseObject(saddr.ip);

	return addr;

}

void onPeerWhatever(CBNetworkCommunicator * foo, CBPeer * bar){
	return;
}


void CBNetworkCommunicatorTryConnectionsVoid(void * comm){
	CBNetworkCommunicatorTryConnections(comm, false);
}


void onNetworkError(CBNetworkCommunicator * comm, CBErrorReason reason){
	CBLogError("DID LOSE LAST NODE");
	exit(EXIT_FAILURE);
}

void onBadTime(void * foo){
	CBLogError("BAD TIME FAIL");
	exit(EXIT_FAILURE);
}


bool acceptType(CBNetworkCommunicator * comm, CBPeer * peer, CBMessageType type){
	if (type == CB_MESSAGE_TYPE_VERACK && peer->handshakeStatus & CB_HANDSHAKE_GOT_ACK) {
		CBLogError("ALREADY HAVE ACK FAIL\n");
		exit(EXIT_FAILURE);
	}
	if (type == CB_MESSAGE_TYPE_VERSION && peer->handshakeStatus & CB_HANDSHAKE_GOT_VERSION) {
		CBLogError("ALREADY HAVE VERSION FAIL\n");
		exit(EXIT_FAILURE);
	}
	return true;
}


CBOnMessageReceivedAction onMessageReceived(CBNetworkCommunicator * comm, CBPeer * peer, CBMessage * theMessage){
	fprintf(stderr,"on message received");
}





CBMessage * CBFDgetVersion(CBNetworkCommunicator *self, CBNetworkAddress * peer){
	return CBGetMessage(CBNetworkCommunicatorGetVersion(self, peer));
}

//CBLogWarning("Good IP (%s) for --addnode", argv[x]);
//CBLogWarning("Bad IP for addnode configuration option at line %u", lineNum);
// commConnect->flags = CB_NETWORK_COMMUNICATOR_AUTO_HANDSHAKE | CB_NETWORK_COMMUNICATOR_AUTO_PING | CB_NETWORK_COMMUNICATOR_AUTO_DISCOVERY;

/*For testing, run this command from the repo root directory:
 *     src/spv 2>/dev/null | hexdump -C -s OFFSET -n LENGTH
 *  ie  src/spv 2>/dev/null | hexdump -C -s 0 -n 4
 *
 */

