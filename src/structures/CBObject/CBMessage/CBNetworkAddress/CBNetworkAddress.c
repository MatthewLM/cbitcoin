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

CBNetworkAddress * CBNewNetworkAddress(uint32_t time,CBByteArray * ip,uint16_t port,uint64_t services,void (*onErrorReceived)(CBError error,char *,...)){
	CBNetworkAddress * self = malloc(sizeof(*self));
	if (NOT self) {
		onErrorReceived(CB_ERROR_OUT_OF_MEMORY,"Cannot allocate %i bytes of memory in CBNewNetworkAddress\n",sizeof(*self));
		return NULL;
	}
	CBGetObject(self)->free = CBFreeNetworkAddress;
	if (CBInitNetworkAddress(self,time,ip,port,services,onErrorReceived))
		return self;
	free(self);
	return NULL;
}
CBNetworkAddress * CBNewNetworkAddressFromData(CBByteArray * data,void (*onErrorReceived)(CBError error,char *,...)){
	CBNetworkAddress * self = malloc(sizeof(*self));
	if (NOT self) {
		onErrorReceived(CB_ERROR_OUT_OF_MEMORY,"Cannot allocate %i bytes of memory in CBNewNetworkAddressFromData\n",sizeof(*self));
		return NULL;
	}
	CBGetObject(self)->free = CBFreeNetworkAddress;
	if(CBInitNetworkAddressFromData(self,data,onErrorReceived))
		return self;
	free(self);
	return NULL;
}

//  Object Getter

CBNetworkAddress * CBGetNetworkAddress(void * self){
	return self;
}

//  Initialiser

bool CBInitNetworkAddress(CBNetworkAddress * self,uint32_t score,CBByteArray * ip,uint16_t port,uint64_t services,void (*onErrorReceived)(CBError error,char *,...)){
	self->score = score;
	self->ip = ip;
	if (NOT ip) {
		ip = CBNewByteArrayOfSize(16, onErrorReceived);
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
	self->public = false; // Private by default.
	if (NOT CBInitMessageByObject(CBGetMessage(self), onErrorReceived)){
		CBReleaseObject(ip);
		return false;
	}
	return true;
}
bool CBInitNetworkAddressFromData(CBNetworkAddress * self,CBByteArray * data,void (*onErrorReceived)(CBError error,char *,...)){
	self->ip = NULL;
	self->public = false; // Private by default.
	if (NOT CBInitMessageByData(CBGetMessage(self), data, onErrorReceived))
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

uint8_t CBNetworkAddressDeserialise(CBNetworkAddress * self,bool score){
	CBByteArray * bytes = CBGetMessage(self)->bytes;
	if (NOT bytes) {
		CBGetMessage(self)->onErrorReceived(CB_ERROR_MESSAGE_DESERIALISATION_NULL_BYTES,"Attempting to deserialise a CBNetworkAddress with no bytes.");
		return 0;
	}
	if (bytes->length < 26 + score * 4) {
		CBGetMessage(self)->onErrorReceived(CB_ERROR_MESSAGE_DESERIALISATION_BAD_BYTES,"Attempting to deserialise a CBNetworkAddress with less bytes than required.");
		return 0;
	}
	uint8_t cursor;
	if (score) {
		self->score = CBByteArrayReadInt32(bytes, 0);
		cursor = 4;
	}else{
		self->score = 0;
		cursor = 0;
	}
	self->services = CBByteArrayReadInt64(bytes, cursor);
	cursor += 8;
	self->ip = CBNewByteArraySubReference(bytes, cursor, 16);
	if (NOT self->ip)
		return 0;
	// Determine IP type
	self->type = CBGetIPType(CBByteArrayGetData(self->ip));
	cursor += 16;
	self->port = CBByteArrayReadPort(bytes, cursor);
	return cursor + 2;
}
bool CBNetworkAddressEquals(CBNetworkAddress * self,CBNetworkAddress * addr){
	return (self->ip
			&& addr->ip
			&& CBByteArrayCompare(self->ip, addr->ip) == CB_COMPARE_EQUAL
			&& self->port == addr->port);
}
uint8_t CBNetworkAddressSerialise(CBNetworkAddress * self,bool score){
	CBByteArray * bytes = CBGetMessage(self)->bytes;
	if (NOT bytes) {
		CBGetMessage(self)->onErrorReceived(CB_ERROR_MESSAGE_SERIALISATION_NULL_BYTES,"Attempting to serialise a CBNetworkAddress with no bytes.");
		return 0;
	}
	if (bytes->length < 26 + score * 4) {
		CBGetMessage(self)->onErrorReceived(CB_ERROR_MESSAGE_SERIALISATION_BAD_BYTES,"Attempting to serialise a CBNetworkAddress with less bytes than required.");
		return 0;
	}
	uint8_t cursor;
	if (score) {
		CBByteArraySetInt32(bytes, 0, self->score);
		cursor = 4;
	}else cursor = 0;
	CBByteArraySetInt64(bytes, cursor, self->services);
	cursor += 8;
	CBByteArrayCopyByteArray(bytes, cursor, self->ip);
	CBByteArrayChangeReference(self->ip, bytes, cursor);
	cursor += 16;
	CBByteArraySetPort(bytes, cursor, self->port);
	return cursor + 2;

}
