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

CBNetworkAddress * CBNewNetworkAddress(uint32_t time,CBByteArray * ip,uint16_t port,uint64_t services,CBEvents * events){
	CBNetworkAddress * self = malloc(sizeof(*self));
	CBGetObject(self)->free = CBFreeNetworkAddress;
	bool ok = CBInitNetworkAddress(self,time,ip,port,services,events);
	if (NOT ok) {
		return NULL;
	}
	return self;
}
CBNetworkAddress * CBNewNetworkAddressFromData(CBByteArray * data,CBEvents * events){
	CBNetworkAddress * self = malloc(sizeof(*self));
	CBGetObject(self)->free = CBFreeNetworkAddress;
	bool ok = CBInitNetworkAddressFromData(self,data,events);
	if (NOT ok) {
		return NULL;
	}
	return self;
}

//  Object Getter

CBNetworkAddress * CBGetNetworkAddress(void * self){
	return self;
}

//  Initialiser

bool CBInitNetworkAddress(CBNetworkAddress * self,uint32_t score,CBByteArray * ip,uint16_t port,uint64_t services,CBEvents * events){
	self->score = score;
	self->ip = ip;
	// Determine IP type
	self->type = CBGetIPType(CBByteArrayGetData(ip));
	if (NOT ip) {
		ip = CBNewByteArrayOfSize(16, events);
		memset(CBByteArrayGetData(ip), 0, 16);
	}else{
		CBRetainObject(ip);
	}
	self->port = port;
	self->services = services;
	if (NOT CBInitMessageByObject(CBGetMessage(self), events))
		return false;
	return true;
}
bool CBInitNetworkAddressFromData(CBNetworkAddress * self,CBByteArray * data,CBEvents * events){
	self->ip = NULL;
	if (NOT CBInitMessageByData(CBGetMessage(self), data, events))
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
		CBGetMessage(self)->events->onErrorReceived(CB_ERROR_MESSAGE_DESERIALISATION_NULL_BYTES,"Attempting to deserialise a CBNetworkAddress with no bytes.");
		return 0;
	}
	if (bytes->length < 26 + score * 4) {
		CBGetMessage(self)->events->onErrorReceived(CB_ERROR_MESSAGE_DESERIALISATION_BAD_BYTES,"Attempting to deserialise a CBNetworkAddress with less bytes than required.");
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
	// Determine IP type
	self->type = CBGetIPType(CBByteArrayGetData(self->ip));
	cursor += 16;
	self->port = CBByteArrayReadPort(bytes, cursor);
	return cursor + 2;
}
uint8_t CBNetworkAddressSerialise(CBNetworkAddress * self,bool score){
	CBByteArray * bytes = CBGetMessage(self)->bytes;
	if (NOT bytes) {
		CBGetMessage(self)->events->onErrorReceived(CB_ERROR_MESSAGE_SERIALISATION_NULL_BYTES,"Attempting to serialise a CBNetworkAddress with no bytes.");
		return 0;
	}
	if (bytes->length < 26 + score * 4) {
		CBGetMessage(self)->events->onErrorReceived(CB_ERROR_MESSAGE_SERIALISATION_BAD_BYTES,"Attempting to serialise a CBNetworkAddress with less bytes than required.");
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
