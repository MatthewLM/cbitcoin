//
//  CBNetworkAddress.c
//  cbitcoin
//
//  Created by Matthew Mitchell on 02/07/2012.
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

#include "CBNetworkAddress.h"

//  Constructor

CBNetworkAddress * CBNewNetworkAddress(uint64_t lastSeen, CBByteArray * ip, uint16_t port, CBVersionServices services, bool isPublic){
	CBNetworkAddress * self = malloc(sizeof(*self));
	if (NOT self) {
		CBLogError("Cannot allocate %i bytes of memory in CBNewNetworkAddress\n", sizeof(*self));
		return NULL;
	}
	CBGetObject(self)->free = CBFreeNetworkAddress;
	if (CBInitNetworkAddress(self, lastSeen, ip, port, services, isPublic))
		return self;
	free(self);
	return NULL;
}
CBNetworkAddress * CBNewNetworkAddressFromData(CBByteArray * data, bool isPublic){
	CBNetworkAddress * self = malloc(sizeof(*self));
	if (NOT self) {
		CBLogError("Cannot allocate %i bytes of memory in CBNewNetworkAddressFromData\n", sizeof(*self));
		return NULL;
	}
	CBGetObject(self)->free = CBFreeNetworkAddress;
	if(CBInitNetworkAddressFromData(self, data, isPublic))
		return self;
	free(self);
	return NULL;
}

//  Object Getter

CBNetworkAddress * CBGetNetworkAddress(void * self){
	return self;
}

//  Initialiser

bool CBInitNetworkAddress(CBNetworkAddress * self, uint64_t lastSeen, CBByteArray * ip, uint16_t port, CBVersionServices services, bool isPublic){
	self->lastSeen = lastSeen;
	self->penalty = 0;
	self->ip = ip;
	self->isPublic = isPublic;
	if (NOT ip) {
		ip = CBNewByteArrayOfSize(16);
		if (NOT ip)
			return false;
		memset(CBByteArrayGetData(ip), 0, 16);
		self->type = CB_IP_INVALID;
	}else{
		// Determine IP type
		self->type = CBGetIPType(CBByteArrayGetData(ip));
		CBRetainObject(ip);
	}
	self->port = port;
	self->services = services;
	self->bucketSet = false;
	if (NOT CBInitMessageByObject(CBGetMessage(self))){
		CBReleaseObject(ip);
		return false;
	}
	return true;
}
bool CBInitNetworkAddressFromData(CBNetworkAddress * self, CBByteArray * data, bool isPublic){
	self->ip = NULL;
	self->bucketSet = false;
	if (NOT CBInitMessageByData(CBGetMessage(self), data))
		return false;
	return true;
}

//  Destructor

void CBFreeNetworkAddress(void * vself){
	CBNetworkAddress * self = vself;
	if (self->ip) CBReleaseObject(self->ip); 
	CBFreeMessage(self);
}

//  Functions

uint8_t CBNetworkAddressDeserialise(CBNetworkAddress * self, bool timestamp){
	CBByteArray * bytes = CBGetMessage(self)->bytes;
	if (NOT bytes) {
		CBLogError("Attempting to deserialise a CBNetworkAddress with no bytes.");
		return 0;
	}
	if (bytes->length < 26 + timestamp * 4) {
		CBLogError("Attempting to deserialise a CBNetworkAddress with less bytes than required.");
		return 0;
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
	self->ip = CBNewByteArraySubReference(bytes, start + 8, 16);
	if (NOT self->ip)
		return 0;
	// Determine IP type
	self->type = CBGetIPType(CBByteArrayGetData(self->ip));
	self->port = CBByteArrayReadPort(bytes, start + 24);
	return start + 26;
}
bool CBNetworkAddressEquals(CBNetworkAddress * self, CBNetworkAddress * addr){
	return (self->ip
			&& addr->ip
			&& CBByteArrayCompare(self->ip, addr->ip) == CB_COMPARE_EQUAL
			&& self->port == addr->port);
}
uint8_t CBNetworkAddressSerialise(CBNetworkAddress * self, bool timestamp){
	CBByteArray * bytes = CBGetMessage(self)->bytes;
	if (NOT bytes) {
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
	CBByteArrayCopyByteArray(bytes, cursor, self->ip);
	CBByteArrayChangeReference(self->ip, bytes, cursor);
	cursor += 16;
	CBByteArraySetPort(bytes, cursor, self->port);
	bytes->length = cursor + 2;
	CBGetMessage(self)->serialised = true;
	return cursor + 2;

}
