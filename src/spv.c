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

#include "main.h"



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
		if (!CBCheckNumber(saddr.port)) {
			CBReleaseObject(saddr.ip);
			return NULL;
		}
	}

	CBNetworkAddress * addr = CBNewNetworkAddress(0, saddr, 0, isPublic);
	CBReleaseObject(saddr.ip);

	return addr;

}

//CBLogWarning("Good IP (%s) for --addnode", argv[x]);
//CBLogWarning("Bad IP for addnode configuration option at line %u", lineNum);
// commConnect->flags = CB_NETWORK_COMMUNICATOR_AUTO_HANDSHAKE | CB_NETWORK_COMMUNICATOR_AUTO_PING | CB_NETWORK_COMMUNICATOR_AUTO_DISCOVERY;

/*For testing, run this command from the repo root directory:
 *     src/spv 2>/dev/null | hexdump -C -s OFFSET -n LENGTH
 *  ie  src/spv 2>/dev/null | hexdump -C -s 0 -n 4
 *
 */


int main(int argc, char * argv[]) {
	fprintf(stderr, "Error: Hello\n");
	// Get home directory and set the default data directory.
    // hi
	CBNetworkAddress * peerAddress = CBReadNetworkAddress("10.21.0.189", false);
	CBNetworkAddress * selfAddress = CBReadNetworkAddress("10.21.0.67", false);




	fprintf(stderr, "Error: Size(%d) 4\n",CBGetMessage(version)->bytes->length);

//CBNetworkCommunicatorSendMessage, then see CBNetworkCommunicatorOnCanSend

}
