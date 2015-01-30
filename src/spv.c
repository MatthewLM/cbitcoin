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


bool CBAddNetworkAddress(CBNetworkAddressLinkedList * nodes, char * ip, bool isPublic) {

	CBNetworkAddressLinkedListNode * node = malloc(sizeof(*node));

	node->addr = CBReadNetworkAddress(ip, isPublic);
	if (!node->addr)
		return false;

	node->next = NULL;

	if (nodes->last == NULL)
		nodes->start = nodes->last = node;
	else
		nodes->last = nodes->last->next = node;

	return true;

}

bool CBCheckNumber(uint32_t num) {

	if (num == 0){
		if (errno == ERANGE){
			CBLogWarning("Number out of range.");
			return false;
		}else if (errno == EINVAL){
			CBLogWarning("Invalid number.");
			return false;
		}
	}

	return true;

}

uint64_t CBGetMilliseconds(void) {

	struct timeval tv;

	gettimeofday(&tv, NULL);

	return tv.tv_sec * 1000 + tv.tv_usec / 1000;

}

void CBOnDoubleSpend(CBNode * node, uint8_t * txHash) {

}

void CBOnNewBlock(CBNode * node, CBBlock * block, uint32_t forkPoint) {

}

void CBOnNewTransaction(CBNode * node, CBTransaction * tx, uint64_t timestamp, uint32_t blockHeight, CBTransactionAccountDetailList * details) {

}

void CBOnFatalNodeError(CBNode * node, CBErrorReason reason) {

	if (reason == CB_ERROR_NO_PEERS)
		CBLogWarning("Could not connect to nodes. Trying again in 20 seconds (if not incoming only).");
	else{
		CBLogError("Fatal Node Error");
		exit(EXIT_FAILURE);
	}

}

void CBOnTransactionConfirmed(CBNode * node, uint8_t * txHash, uint32_t blockHeight) {

}

void CBOnTransactionUnconfirmed(CBNode * node, uint8_t * txHash) {

}

void CBUpToDate(CBNode * node, bool uptodate) {

}

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


void CBStartNode(void * vcomm) {
	puts("Start Node");
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
	CBByteArray * localpeer = CBNewByteArrayWithDataCopy((uint8_t [16]){0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xFF, 0xFF, 10, 21, 0, 67}, 16);
	CBByteArray * btcpeer = CBNewByteArrayWithDataCopy((uint8_t [16]){0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xFF, 0xFF, 10, 21, 0, 189}, 16);
	CBNetworkAddress * addDest = CBNewNetworkAddress(0, (CBSocketAddress){btcpeer, 8333}, 0, false);
	CBNetworkAddress * addSource = CBNewNetworkAddress(0, (CBSocketAddress){localpeer, 8333}, 0, false);
	CBByteArray * userAgent = CBNewByteArrayFromString(CB_USER_AGENT_SEGMENT, false);
	CBVersion * version = CBNewVersion(
			CB_PONG_VERSION,
			CB_SERVICE_FULL_BLOCKS,
			time(NULL),
			addDest,
			addSource,
			rand(), // uint64_t nonce
			userAgent,
			1
	);
	uint32_t len;
	CBMessage *message = CBGetMessage(version);

	char typeStr[CB_MESSAGE_TYPE_STR_SIZE];
	CBMessageTypeToString(message->type, typeStr);

	// we have a version, but it is not serialized
	CBVersionPrepareBytes(CBGetVersion(message));
	len = CBVersionSerialise(CBGetVersion(message), false);

	// Make checksum
	uint8_t hash[32];
	uint8_t hash2[32];
	CBSha256(CBByteArrayGetData(message->bytes), message->bytes->length, hash);
	CBSha256(hash, 32, hash2);
	memcpy(message->checksum, hash2, 4);
	if (message->bytes->length != len)
		fprintf(stderr,"%d vs %d message length mismatch",len,message->bytes->length);

	uint8_t *header = malloc(24);
	// Network ID
	//... check CBConstants.h for CB_PRODUCTION_NETWORK_BYTES
	CBInt32ToArray(header, CB_MESSAGE_HEADER_NETWORK_ID, CB_PRODUCTION_NETWORK_BYTES);
	memcpy(header + CB_MESSAGE_HEADER_TYPE, "version\0\0\0\0\0", 12);
	CBInt32ToArray(header, CB_MESSAGE_HEADER_LENGTH, message->bytes->length);
	memcpy(header + CB_MESSAGE_HEADER_CHECKSUM, message->checksum, 4);


	fprintf(stderr,"Hello 3");
    //fprintf(stderr,"%d sent to server.",size);
	write(STDOUT_FILENO, header, 24);
    write(STDOUT_FILENO, CBByteArrayGetData(message->bytes), message->bytes->length);

	fprintf(stderr, "Error: Size(%d) 4\n",CBGetMessage(version)->bytes->length);

//CBNetworkCommunicatorSendMessage, then see CBNetworkCommunicatorOnCanSend
	uint32_t n;
	uint8_t buff[20];

	while((n = read(STDIN_FILENO, buff, 20)) > 0){
		fprintf(stderr,"From Bitcoind: %s with n bytes read in",buff,n);
	}
}
