//
//  CBNetworkAddress.c
//  cbitcoin
//
//  Created by Matthew Mitchell on 02/07/2012.
//  Copyright (c) 2012 Matthew Mitchell
//  
//  This file is part of cbitcoin. It is subject to the license terms
//  in the LICENSE file found in the top-level directory of this
//  distribution and at http://www.cbitcoin.com/license.html. No part of
//  cbitcoin, including this file, may be copied, modified, propagated,
//  or distributed except according to the terms contained in the
//  LICENSE file.

//  SEE HEADER FILE FOR DOCUMENTATION 

#include "CBNetworkAddress.h"

//  Constructor

CBNetworkAddress * CBNewNetworkAddress(uint64_t lastSeen, CBSocketAddress addr, CBVersionServices services, bool isPublic){
	CBNetworkAddress * self = malloc(sizeof(*self));
	CBGetObject(self)->free = CBFreeNetworkAddress;
	CBInitNetworkAddress(self, lastSeen, addr, services, isPublic);
	return self;
}
CBNetworkAddress * CBNewNetworkAddressFromData(CBByteArray * data, bool isPublic){
	CBNetworkAddress * self = malloc(sizeof(*self));
	CBGetObject(self)->free = CBFreeNetworkAddress;
	CBInitNetworkAddressFromData(self, data, isPublic);
	return self;
}

//  Initialiser

void CBInitNetworkAddress(CBNetworkAddress * self, uint64_t lastSeen, CBSocketAddress addr, CBVersionServices services, bool isPublic){
	self->lastSeen = lastSeen;
	self->penalty = 0;
	self->sockAddr = addr;
	self->isPublic = isPublic;
	if (! self->sockAddr.ip) {
		self->sockAddr.ip = CBNewByteArrayOfSize(16);
		memset(CBByteArrayGetData(self->sockAddr.ip), 0, 16);
		self->type = CB_IP_INVALID;
	}else{
		// Determine IP type
		self->type = CBGetIPType(CBByteArrayGetData(self->sockAddr.ip));
		CBRetainObject(self->sockAddr.ip);
	}
	self->services = services;
	self->bucketSet = false;
	CBInitMessageByObject(CBGetMessage(self));
}
void CBInitNetworkAddressFromData(CBNetworkAddress * self, CBByteArray * data, bool isPublic){
	self->sockAddr.ip = NULL;
	self->bucketSet = false;
	CBInitMessageByData(CBGetMessage(self), data);
}

//  Destructor

void CBDestroyNetworkAddress(void * vself){
	CBNetworkAddress * self = vself;
	if (self->sockAddr.ip) CBReleaseObject(self->sockAddr.ip);
	CBDestroyMessage(self);
}
void CBFreeNetworkAddress(void * self){
	CBDestroyNetworkAddress(self);
	free(self);
}

//  Functions

uint32_t CBNetworkAddressDeserialise(CBNetworkAddress * self, bool timestamp){
	CBByteArray * bytes = CBGetMessage(self)->bytes;
	if (! bytes) {
		CBLogError("Attempting to deserialise a CBNetworkAddress with no bytes.");
		return CB_DESERIALISE_ERROR;
	}
	if (bytes->length < 26 + timestamp * 4) {
		CBLogError("Attempting to deserialise a CBNetworkAddress with less bytes than required.");
		return CB_DESERIALISE_ERROR;
	}
	uint8_t start;
	uint64_t twoHoursAgo = time(NULL) - 3600;
	if (timestamp) {
		// Make sure we do not set self->lastSeen later than one hour ago.
		self->lastSeen = CBByteArrayReadInt32(bytes, 0);
		if (self->lastSeen > twoHoursAgo)
			self->lastSeen = twoHoursAgo;
		start = 4;
	}else{
		self->lastSeen = twoHoursAgo;
		start = 0;
	}
	self->services = (CBVersionServices) CBByteArrayReadInt64(bytes, start);
	self->sockAddr.ip = CBNewByteArraySubReference(bytes, start + 8, 16);
	// Determine IP type
	self->type = CBGetIPType(CBByteArrayGetData(self->sockAddr.ip));
	self->sockAddr.port = CBByteArrayReadPort(bytes, start + 24);
	return start + 26;
}
bool CBNetworkAddressEquals(CBNetworkAddress * self, CBNetworkAddress * addr){
	return (self->sockAddr.ip
			&& addr->sockAddr.ip
			&& CBByteArrayCompare(self->sockAddr.ip, addr->sockAddr.ip) == CB_COMPARE_EQUAL
			&& self->sockAddr.port == addr->sockAddr.port);
}
uint8_t CBNetworkAddressSerialise(CBNetworkAddress * self, bool timestamp){
	CBByteArray * bytes = CBGetMessage(self)->bytes;
	if (! bytes) {
		CBLogError("Attempting to serialise a CBNetworkAddress with no bytes.");
		return 0;
	}
	if (bytes->length < 26 + timestamp * 4) {
		CBLogError("Attempting to serialise a CBNetworkAddress with less bytes than required.");
		return 0;
	}
	uint8_t cursor;
	if (timestamp) {
		CBByteArraySetInt32(bytes, 0, (uint32_t)(self->lastSeen - self->penalty));
		cursor = 4;
	}else cursor = 0;
	CBByteArraySetInt64(bytes, cursor, self->services);
	cursor += 8;
	CBByteArrayCopyByteArray(bytes, cursor, self->sockAddr.ip);
	CBByteArrayChangeReference(self->sockAddr.ip, bytes, cursor);
	cursor += 16;
	CBByteArraySetPort(bytes, cursor, self->sockAddr.port);
	bytes->length = cursor + 2;
	CBGetMessage(self)->serialised = true;
	return cursor + 2;
}
char * CBNetworkAddressToString(CBNetworkAddress * self, char output[48]){
	if (self->type & CB_IP_IP4) {
		sprintf(output, "[::ffff:%u.%u.%u.%u]:%u",
				CBByteArrayGetByte(self->sockAddr.ip, 12),
				CBByteArrayGetByte(self->sockAddr.ip, 13),
				CBByteArrayGetByte(self->sockAddr.ip, 14),
				CBByteArrayGetByte(self->sockAddr.ip, 15),
				self->sockAddr.port);
		return strchr(output, '\0');
	}
	*(output++) = '[';
	uint8_t numColons = 0;
	for (uint8_t x = 0; x < 16; x += 2) {
		// Also can be used for any reading of big endian 16 bit integers
		uint16_t num = CBByteArrayReadPort(self->sockAddr.ip, x);
		if (num == 0) {
			// Skip
			if (numColons < 2) {
				*(output++) = ':';
				if (numColons == 0)
					*(output++) = ':';
				numColons = 2;
			}
		}else{
			// Write number
			sprintf(output, "%x", num);
			output = strchr(output, '\0');
			if (x != 14) {
				*(output++) = ':';
				numColons = 1;
			}
		}
	}
	// Add port number
	*output = ']';
	sprintf(output + 1, ":%u", self->sockAddr.port);
	return strchr(output, '\0');
}
