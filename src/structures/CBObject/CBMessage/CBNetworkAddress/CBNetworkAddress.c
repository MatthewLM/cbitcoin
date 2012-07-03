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

CBNetworkAddress * CBNewNetworkAddress(u_int32_t time,CBByteArray * ip,u_int16_t port,u_int64_t services,CBEvents * events){
	CBNetworkAddress * self = malloc(sizeof(*self));
	CBGetObject(self)->free = CBFreeNetworkAddress;
	CBInitNetworkAddress(self,time,ip,port,services,events);
	return self;
}
CBNetworkAddress * CBNewNetworkAddressFromData(CBByteArray * data,CBEvents * events){
	CBNetworkAddress * self = malloc(sizeof(*self));
	CBGetObject(self)->free = CBFreeNetworkAddress;
	CBInitNetworkAddressFromData(self,data,events);
	return self;
}

//  Object Getter

CBNetworkAddress * CBGetNetworkAddress(void * self){
	return self;
}

//  Initialiser

bool CBInitNetworkAddress(CBNetworkAddress * self,u_int32_t time,CBByteArray * ip,u_int16_t port,u_int64_t services,CBEvents * events){
	self->time = time;
	self->ip = ip;
	CBRetainObject(ip);
	self->port = port;
	self->services = services;
	if (!CBInitMessageByObject(CBGetMessage(self), events))
		return false;
	return true;
}
bool CBInitNetworkAddressFromData(CBNetworkAddress * self,CBByteArray * data,CBEvents * events){
	self->ip = NULL;
	if (!CBInitMessageByData(CBGetMessage(self), data, events))
		return false;
	return true;
}

//  Destructor

void CBFreeNetworkAddress(void * vself){
	CBNetworkAddress * self = vself;
	if (self->ip) CBReleaseObject(&self->ip); 
	CBFreeObject(self);
}

//  Functions

u_int32_t CBNetworkAddressDeserialise(CBNetworkAddress * self,bool time){
	CBByteArray * bytes = CBGetMessage(self)->bytes;
	if (!bytes) {
		CBGetMessage(self)->events->onErrorReceived(CB_ERROR_MESSAGE_DESERIALISATION_NULL_BYTES,"Attempting to deserialise a CBNetworkAddress with no bytes.");
		return 0;
	}
	if (bytes->length < 26 + time*4) {
		CBGetMessage(self)->events->onErrorReceived(CB_ERROR_MESSAGE_DESERIALISATION_BAD_BYTES,"Attempting to deserialise a CBNetworkAddress with less bytes than required.");
		return 0;
	}
	u_int8_t cursor;
	if (time) {
		self->time = CBByteArrayReadInt32(bytes, 0);
		cursor = 4;
	}else cursor = 0;
	self->services = CBByteArrayReadInt64(bytes, cursor);
	self->ip = CBNewByteArraySubReference(bytes, cursor + 8, 16);
	self->port = CBByteArrayReadInt16(bytes, cursor + 24);
	return cursor + 2;
}
u_int32_t CBNetworkAddressSerialise(CBNetworkAddress * self,bool time){
	CBByteArray * bytes = CBGetMessage(self)->bytes;
	if (!bytes) {
		CBGetMessage(self)->events->onErrorReceived(CB_ERROR_MESSAGE_SERIALISATION_NULL_BYTES,"Attempting to serialise a CBNetworkAddress with no bytes.");
		return 0;
	}
	if (bytes->length < 26 + time*4) {
		CBGetMessage(self)->events->onErrorReceived(CB_ERROR_MESSAGE_SERIALISATION_BAD_BYTES,"Attempting to serialise a CBNetworkAddress with less bytes than required.");
		return 0;
	}
	u_int8_t cursor;
	if (time) {
		CBByteArraySetInt32(bytes, 0, self->time);
		cursor = 4;
	}else cursor = 0;
	CBByteArraySetInt64(bytes, cursor, self->services);
	CBByteArrayCopyByteArray(bytes, cursor + 8, self->ip);
	CBByteArrayChangeReference(self->ip, bytes, cursor + 8);
	CBByteArraySetInt16(bytes, cursor + 24, self->port);
	return cursor + 2;

}
