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



bool SPVsendMessage(CBNetworkCommunicator * self, CBPeer * peer, CBMessage * message){
	//fprintf(stderr,"SPVsendMessage hello 1\n");
//	if (peer->sendQueueSize == CB_SEND_QUEUE_MAX_SIZE || !peer->connectionWorking)
//		return false;
	//char typeStr[CB_MESSAGE_TYPE_STR_SIZE];
	//CBMessageTypeToString(message->type, typeStr);
	//CBLogVerbose("Sending message of type %s (%u) to %s.", typeStr, message->type, peer->peerStr);
	// Serialise message if needed.
	//fprintf(stderr,"SPVsendMessage hello 2\n");



	if (! message->serialised) {
		uint32_t len;
		//fprintf(stderr,"SPVsendMessage hello 3\n");
		switch (message->type) {

			case CB_MESSAGE_TYPE_VERSION:
				CBVersionPrepareBytes(CBGetVersion(message));
				len = CBVersionSerialise(CBGetVersion(message), false);
				fprintf(stderr,"Serialising Version [len=%d]",len);
				break;

			case CB_MESSAGE_TYPE_ADDR:
				CBNetworkAddressListPrepareBytes(CBGetNetworkAddressList(message));
				len = CBNetworkAddressListSerialise(CBGetNetworkAddressList(message), false);
				break;

			case CB_MESSAGE_TYPE_INV:
			case CB_MESSAGE_TYPE_GETDATA:
				CBInventoryPrepareBytes(CBGetInventory(message));
				len = CBInventorySerialise(CBGetInventory(message), false);
				break;

			case CB_MESSAGE_TYPE_GETBLOCKS:
			case CB_MESSAGE_TYPE_GETHEADERS:
				CBGetBlocksPrepareBytes(CBGetGetBlocks(message));
				len = CBGetBlocksSerialise(CBGetGetBlocks(message), false);
				break;

			case CB_MESSAGE_TYPE_TX:
				CBTransactionPrepareBytes(CBGetTransaction(message));
				len = CBTransactionSerialise(CBGetTransaction(message), false);
				break;

			case CB_MESSAGE_TYPE_BLOCK:
				// true -> Including transactions.
				CBBlockPrepareBytes(CBGetBlock(message), true);
				len = CBBlockSerialise(CBGetBlock(message), true, false);
				break;

			case CB_MESSAGE_TYPE_HEADERS:
				CBBlockHeadersPrepareBytes(CBGetBlockHeaders(message));
				len = CBBlockHeadersSerialise(CBGetBlockHeaders(message), false);
				break;

			case CB_MESSAGE_TYPE_PING:
				if (peer->versionMessage->version >= 60000 && self->version >= 60000){
					CBPingPongPrepareBytes(CBGetPingPong(message));
					len = CBPingPongSerialise(CBGetPingPong(message));
				}

			case CB_MESSAGE_TYPE_PONG:
				CBPingPongPrepareBytes(CBGetPingPong(message));
				len = CBPingPongSerialise(CBGetPingPong(message));
				break;

			case CB_MESSAGE_TYPE_ALERT:
				// This should have been serialised before!
				return false;
				break;

			default:
				break;

		}
		//fprintf(stderr,"SPVsendMessage hello 4\n");
		if (message->bytes) {
			//fprintf(stderr,"SPVsendMessage hello 5\n");
			if (message->bytes->length != len){
				fprintf(stderr,"Length does not match \n");
				return false;
			}

		}
	}
	if (message->bytes) {
		// Make checksum
		//fprintf(stderr,"SPVsendMessage hello 6\n");
		uint8_t hash[32];
		uint8_t hash2[32];
		CBSha256(CBByteArrayGetData(message->bytes), message->bytes->length, hash);
		CBSha256(hash, 32, hash2);
		memcpy(message->checksum, hash2, 4);
	}else{
		// Empty bytes checksum
		message->checksum[0] = 0x5D;
		message->checksum[1] = 0xF6;
		message->checksum[2] = 0xE0;
		message->checksum[3] = 0xE2;
	}
	// Add the message and callback to the send queue
	bool res = SPVsendMessageViaPeer(self,peer,message);
	if(!res){
		fprintf(stderr,"Failed to send message\n");
	}
	CBRetainObject(message);
	return res;
}

bool SPVsendMessageViaPeer(CBNetworkCommunicator *self,CBPeer *peer, CBMessage *toSend){
	peer->messageSent = 0;
	bool finished = false;

	// Need to send the header
	if (peer->messageSent == 0) {
		// Create header
		peer->sendingHeader = malloc(24);
		// Network ID
		CBInt32ToArray(peer->sendingHeader, CB_MESSAGE_HEADER_NETWORK_ID, self->networkID);
		// Message type text
		switch (toSend->type) {
			case CB_MESSAGE_TYPE_VERSION:
				memcpy(peer->sendingHeader + CB_MESSAGE_HEADER_TYPE, "version\0\0\0\0\0", 12);
				break;
			case CB_MESSAGE_TYPE_VERACK:
				memcpy(peer->sendingHeader + CB_MESSAGE_HEADER_TYPE, "verack\0\0\0\0\0\0", 12);
				break;
			case CB_MESSAGE_TYPE_ADDR:
				memcpy(peer->sendingHeader + CB_MESSAGE_HEADER_TYPE, "addr\0\0\0\0\0\0\0\0", 12);
				break;
			case CB_MESSAGE_TYPE_INV:
				memcpy(peer->sendingHeader + CB_MESSAGE_HEADER_TYPE, "inv\0\0\0\0\0\0\0\0\0", 12);
				break;
			case CB_MESSAGE_TYPE_GETDATA:
				memcpy(peer->sendingHeader + CB_MESSAGE_HEADER_TYPE, "getdata\0\0\0\0\0", 12);
				break;
			case CB_MESSAGE_TYPE_GETBLOCKS:
				memcpy(peer->sendingHeader + CB_MESSAGE_HEADER_TYPE, "getblocks\0\0\0", 12);
				break;
			case CB_MESSAGE_TYPE_GETHEADERS:
				memcpy(peer->sendingHeader + CB_MESSAGE_HEADER_TYPE, "getheaders\0\0", 12);
				break;
			case CB_MESSAGE_TYPE_TX:
				memcpy(peer->sendingHeader + CB_MESSAGE_HEADER_TYPE, "tx\0\0\0\0\0\0\0\0\0\0", 12);
				break;
			case CB_MESSAGE_TYPE_BLOCK:
				memcpy(peer->sendingHeader + CB_MESSAGE_HEADER_TYPE, "block\0\0\0\0\0\0\0", 12);
				break;
			case CB_MESSAGE_TYPE_HEADERS:
				memcpy(peer->sendingHeader + CB_MESSAGE_HEADER_TYPE, "headers\0\0\0\0\0", 12);
				break;
			case CB_MESSAGE_TYPE_GETADDR:
				memcpy(peer->sendingHeader + CB_MESSAGE_HEADER_TYPE, "getaddr\0\0\0\0\0", 12);
				break;
			case CB_MESSAGE_TYPE_PING:
				memcpy(peer->sendingHeader + CB_MESSAGE_HEADER_TYPE, "ping\0\0\0\0\0\0\0\0", 12);
				break;
			case CB_MESSAGE_TYPE_PONG:
				memcpy(peer->sendingHeader + CB_MESSAGE_HEADER_TYPE, "pong\0\0\0\0\0\0\0\0", 12);
				break;
			case CB_MESSAGE_TYPE_ALERT:
				memcpy(peer->sendingHeader + CB_MESSAGE_HEADER_TYPE, "alert\0\0\0\0\0\0\0", 12);
				break;
			default:
				memcpy(peer->sendingHeader + CB_MESSAGE_HEADER_TYPE, toSend->altText, 12);
				break;
		}
		// Length
		if (toSend->bytes){
			//fprintf(stderr,"Printing length %d \n",toSend->bytes->length);
			CBInt32ToArray(peer->sendingHeader, CB_MESSAGE_HEADER_LENGTH, toSend->bytes->length);
		}else
			memset(peer->sendingHeader + CB_MESSAGE_HEADER_LENGTH, 0, 4);
		// Checksum
		memcpy(peer->sendingHeader + CB_MESSAGE_HEADER_CHECKSUM, toSend->checksum, 4);
	}
	//int32_t len = CBSocketSend(peer->socketID, peer->sendingHeader + peer->messageSent, 24 - peer->messageSent);
	//int32_t len = write(STDOUT_FILENO,peer->sendingHeader,24);
	//CBByteArray * outbound = CBNewByteArrayOfSize(toSend);

/*
	if (0) {
		free(peer->sendingHeader);
		fprintf(stderr,"Did not send full header");
		exit;// exit the program and kill ourselves, because we cannot talk to the multiplexer
	}else{
		peer->messageSent += len;
		if (peer->messageSent == 24) {
			// Done header
			free(peer->sendingHeader);
			if (toSend->bytes) {
				// Now send the bytes
				peer->messageSent = 0;
				peer->sentHeader = true;
			}else
				// Nothing more to send!
				finished = true;
		}
		else{
			fprintf(stderr,"failed to send full header");
			exit;
		}
	}*/
	//int32_t len = write(STDOUT_FILENO,peer->sendingHeader,24);
	CBByteArray * outbound = CBNewByteArrayOfSize(24+toSend->bytes->length);

	for(uint32_t i=0;i<24+toSend->bytes->length;i++ ){
		if(i < 24){
			CBByteArraySetByte(outbound, i, peer->sendingHeader[i]);
		}
		else{
			 CBByteArraySetByte(outbound, i, CBByteArrayGetByte(toSend->bytes,i-24));
		}
	}
	// Sent header
	//int32_t len = CBSocketSend(peer->socketID, CBByteArrayGetData(toSend->bytes) + peer->messageSent, toSend->bytes->length - peer->messageSent);
	//len = write(STDOUT_FILENO,CBByteArrayGetData(toSend->bytes),toSend->bytes->length);
	CHECK(0 <= mq_send(self->outbound, CBByteArrayGetData(outbound), outbound->length, 0));

/*
	if (len == CB_SOCKET_FAILURE)
		CBNetworkCommunicatorDisconnect(self, peer, 0, false);
	else{
		peer->messageSent += len;
		if (peer->messageSent == toSend->bytes->length){
			// Sent the entire payload.
			finished = true;
		}
		else{
			// die because we did not send the whole message
			fprintf(stderr,"could not send the whole message");
			exit;
		}

	}
*/
	// If we sent version or verack, record this
	if (toSend->type == CB_MESSAGE_TYPE_VERSION)
		peer->handshakeStatus |= CB_HANDSHAKE_SENT_VERSION;
	else if (toSend->type == CB_MESSAGE_TYPE_VERACK)
		peer->handshakeStatus |= CB_HANDSHAKE_SENT_ACK;

	// Reset variables for next send.
	peer->messageSent = 0;
	peer->sentHeader = false;
	// Done sending message.
/*	if (peer->typeExpected != CB_MESSAGE_TYPE_NONE)
		CBSocketAddEvent(peer->receiveEvent, self->responseTimeOut); // Expect response.
*/
	return true;
}

// see CBNetworkCommunicatorOnCanReceive
//
bool SPVreceiveMessageHeader(CBNetworkCommunicator * self, CBPeer * peer){
	peer->receive = CBNewMessageByObject();
	peer->receive->serialised = true;
	peer->headerBuffer = malloc(24); // Twenty-four bytes for the message header.
	peer->messageReceived = 0; // So far received nothing.

	int len = 0;
	free(peer->headerBuffer);
	peer->headerBuffer = malloc(24);
	ssize_t bytes_read;

	uint8_t *buffer = malloc(MAX_SIZE + 1);
	CBByteArray * buffarray = CBNewByteArrayWithData(buffer, MAX_SIZE + 1);

	bytes_read = mq_receive(self->inbound, CBByteArrayGetData(buffarray), MAX_SIZE + 1, NULL);

	CBByteArray * msgbody;
	CBInitByteArraySubReference(msgbody,buffarray,0,bytes_read);

	bool success = SPVreadHeader(self,peer,msgbody);
	CBFreeByteArray(msgbody);
	CBFreeByteArray(buffarray);

	return success;
}

bool SPVreadHeader(CBNetworkCommunicator *self, CBPeer * peer, CBByteArray *msgbody){

	CBInitMessageByData(peer->receive,msgbody);
	return true;
	/*

	CBByteArray * header = CBNewByteArrayWithData(peer->headerBuffer, 24);
	uint32_t networkID = CBByteArrayReadInt32(header, 0);
	if (networkID != self->networkID){
		fprintf(stderr,"Wrong network id\n");
		CBReleaseObject(header);
		return false;
	}

	CBMessageType type = CB_MESSAGE_TYPE_NONE;

	uint32_t size = CBByteArrayReadInt32(header, 16);

	bool error = false;

	CBByteArray * typeBytes = CBNewByteArraySubReference(header, 4, 12);

	if (! memcmp(CBByteArrayGetData(typeBytes), "version\0\0\0\0\0", 12)) {
		// Version message
		// Check that we have not received their version yet.
		type = CB_MESSAGE_TYPE_VERSION;
		if (peer->handshakeStatus & CB_HANDSHAKE_GOT_VERSION)
			 error = true;
	}else if (!(peer->handshakeStatus & CB_HANDSHAKE_GOT_VERSION)){
		// We have not yet received the version message.
		CBLogWarning("Received non-version message before version message from %s.", peer->peerStr);
		error = true;
	}else if (! memcmp(CBByteArrayGetData(typeBytes), "verack\0\0\0\0\0\0", 12)){
		// Version acknowledgement message
		// Chek we have sent the version and not received a verack already.
		type = CB_MESSAGE_TYPE_VERACK;
		if (!(peer->handshakeStatus & CB_HANDSHAKE_SENT_VERSION)
			|| peer->handshakeStatus & CB_HANDSHAKE_GOT_ACK)
			error = true;
	}else{
		if (! memcmp(CBByteArrayGetData(typeBytes), "addr\0\0\0\0\0\0\0\0", 12)){
			// Address broadcast message
			type = CB_MESSAGE_TYPE_ADDR;
		}else if (! memcmp(CBByteArrayGetData(typeBytes), "inv\0\0\0\0\0\0\0\0\0", 12)){
			// Inventory broadcast message
			type = CB_MESSAGE_TYPE_INV;
		}else if (! memcmp(CBByteArrayGetData(typeBytes), "getdata\0\0\0\0\0", 12)){
			// Get data message
			type = CB_MESSAGE_TYPE_GETDATA;
		}else if (! memcmp(CBByteArrayGetData(typeBytes), "getblocks\0\0\0", 12)){
			// Get blocks message
			type = CB_MESSAGE_TYPE_GETBLOCKS;
		}else if (! memcmp(CBByteArrayGetData(typeBytes), "getheaders\0\0", 12)){
			// Get headers message
			type = CB_MESSAGE_TYPE_GETHEADERS;
		}else if (! memcmp(CBByteArrayGetData(typeBytes), "tx\0\0\0\0\0\0\0\0\0\0", 12)){
			// Transaction message
			type = CB_MESSAGE_TYPE_TX;
			if (size > CB_BLOCK_MAX_SIZE)
				error = true;
		}else if (! memcmp(CBByteArrayGetData(typeBytes), "block\0\0\0\0\0\0\0", 12)){
			// Block message
			type = CB_MESSAGE_TYPE_BLOCK;
			if (size > CB_BLOCK_MAX_SIZE)
				error = true;
		}else if (! memcmp(CBByteArrayGetData(typeBytes), "headers\0\0\0\0\0", 12)){
			// Block headers message
			type = CB_MESSAGE_TYPE_HEADERS;
		}else if (! memcmp(CBByteArrayGetData(typeBytes), "getaddr\0\0\0\0\0", 12)){
			// Get Addresses message
			type = CB_MESSAGE_TYPE_GETADDR;
		}else if (! memcmp(CBByteArrayGetData(typeBytes), "ping\0\0\0\0\0\0\0\0", 12)){
			// Ping message
			// Should be empty before version 60000.
			type = CB_MESSAGE_TYPE_PING;
			if ((peer->versionMessage->version < CB_PONG_VERSION || self->version < CB_PONG_VERSION) && size)
				error = true;
		}else if (! memcmp(CBByteArrayGetData(typeBytes), "pong\0\0\0\0\0\0\0\0", 12)){
			// Pong message
			type = CB_MESSAGE_TYPE_PONG;
		}else if (! memcmp(CBByteArrayGetData(typeBytes), "alert\0\0\0\0\0\0\0", 12)){
			// Alert message
			type = CB_MESSAGE_TYPE_ALERT;
		}else{
			// Either alternative or unknown. ??? Add tests for this.
			if (self->alternativeMessages) {
				for (uint16_t x = 0; x < self->alternativeMessages->length / 12; x++) {
					// Check this alternative message
					if (! memcmp(CBByteArrayGetData(typeBytes), CBByteArrayGetData(self->alternativeMessages) + 12 * x, 12)){
						// Alternative message
						type = CB_MESSAGE_TYPE_ALT;
						// Check payload size
						uint32_t altSize;
						if (self->altMaxSizes)
							altSize = self->altMaxSizes[x];
						else
							altSize = CB_MAX_MESSAGE_SIZE;
						if (size <= altSize) // Should correspond to the user given value
							error = false;
						break;
					}
				}
			}
		}
	}
	fprintf(stderr, "Received a message header from %s with the type %s and expected size of %u.", peer->peerStr, CBByteArrayGetData(typeBytes), size);
	//if (!self->callbacks.acceptingType(self, peer, type) ) {
	CBReleaseObject(typeBytes);
	if (error) {
		fprintf(stderr, "There was an error with the message.\n");
		//CBNetworkCommunicatorDisconnect(self, peer, CB_24_HOURS, false);
		CBReleaseObject(header);
		return false;
	}
	/*
	 * still lots more to do !
	 */

	return false;
}

