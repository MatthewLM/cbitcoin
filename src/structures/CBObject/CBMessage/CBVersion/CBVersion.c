//
//  CBVersion.c
//  cbitcoin
//
//  Created by Matthew Mitchell on 01/07/2012.
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

#include "CBVersion.h"

//  Constructor

CBVersion * CBNewVersion(int32_t version,u_int64_t services,int64_t time,CBNetworkAddress * addRecv,CBNetworkAddress * addSource,u_int64_t nounce,CBByteArray * userAgent,int32_t blockHeight,CBEvents * events){
	CBVersion * self = malloc(sizeof(*self));
	CBGetObject(self)->free = CBFreeVersion;
	CBInitVersion(self,version,services,time,addRecv,addSource,nounce,userAgent,blockHeight,events);
	return self;
}
CBVersion * CBNewVersionFromData(CBByteArray * data,CBEvents * events){
	CBVersion * self = malloc(sizeof(*self));
	CBGetObject(self)->free = CBFreeVersion;
	CBInitVersionFromData(self, data, events);
	return self;
}

//  Object Getter

CBVersion * CBGetVersion(void * self){
	return self;
}

//  Initialiser

bool CBInitVersion(CBVersion * self,int32_t version,u_int64_t services,int64_t time,CBNetworkAddress * addRecv,CBNetworkAddress * addSource,u_int64_t nounce,CBByteArray * userAgent,int32_t blockHeight,CBEvents * events){
	self->version = version;
	self->services = services;
	self->time = time;
	self->addRecv = addRecv;
	CBRetainObject(addRecv);
	self->addSource = addSource;
	CBRetainObject(addSource);
	self->nounce = nounce;
	self->userAgent = userAgent;
	CBRetainObject(userAgent);
	self->blockHeight = blockHeight;
	if (!CBInitMessageByObject(CBGetMessage(self), events))
		return false;
	return true;
}
bool CBInitVersionFromData(CBVersion * self,CBByteArray * data,CBEvents * events){
	self->addRecv = NULL;
	self->userAgent = NULL;
	if (!CBInitMessageByData(CBGetMessage(self), data, events))
		return false;
	return true;
}

//  Destructor

void CBFreeVersion(void * vself){
	CBVersion * self = vself;
	if (self->addRecv) CBReleaseObject(&self->addRecv);
	if (self->userAgent) CBReleaseObject(&self->userAgent);
	CBFreeObject(self);
}

//  Functions

u_int32_t CBVersionDeserialise(CBVersion * self){
	CBByteArray * bytes = CBGetMessage(self)->bytes;
	if (!bytes) {
		CBGetMessage(self)->events->onErrorReceived(CB_ERROR_MESSAGE_DESERIALISATION_NULL_BYTES,"Attempting to deserialise a CBVersion with no bytes.");
		return 0;
	}
	if (bytes->length < 46) {
		CBGetMessage(self)->events->onErrorReceived(CB_ERROR_MESSAGE_DESERIALISATION_BAD_BYTES,"Attempting to deserialise a CBVersion with less than 46 bytes.");
		return 0;
	}
	self->version = CBByteArrayReadInt32(bytes, 0);
	self->services = CBByteArrayReadInt64(bytes, 4);
	self->time = CBByteArrayReadInt64(bytes, 12);
	self->addRecv = CBNewNetworkAddressFromData(CBByteArraySubReference(bytes, 20, bytes->length-20), CBGetMessage(self)->events);
	u_int32_t len = CBNetworkAddressDeserialise(self->addRecv, false); // No time from version message.
	if (!len) {
		CBGetMessage(self)->events->onErrorReceived(CB_ERROR_MESSAGE_DESERIALISATION_BAD_BYTES,"CBVersion cannot be deserialised because of an error with the receiving address.");
		CBReleaseObject(&self->addRecv);
		return 0;
	}
	CBGetMessage(self->addRecv)->bytes->length = len;
	if (self->version >= 106) {
		if (bytes->length < 85) {
			CBGetMessage(self)->events->onErrorReceived(CB_ERROR_MESSAGE_DESERIALISATION_BAD_BYTES,"Attempting to deserialise a CBVersion with less than 85 bytes required.");
			return 0;
		}
		self->addSource = CBNewNetworkAddressFromData(CBByteArraySubReference(bytes, 46, bytes->length-46), CBGetMessage(self)->events);
		u_int32_t len = CBNetworkAddressDeserialise(self->addSource, false); // No time from version message.
		if (!len) {
			CBGetMessage(self)->events->onErrorReceived(CB_ERROR_MESSAGE_DESERIALISATION_BAD_BYTES,"CBVersion cannot be deserialised because of an error with the source address.");
			CBReleaseObject(&self->addRecv);
			CBReleaseObject(&self->addSource);
			return 0;
		}
		CBGetMessage(self->addSource)->bytes->length = len;
		self->nounce = CBByteArrayReadInt64(bytes, 72);
		u_int8_t x = CBByteArrayGetByte(bytes, 80); // Check length for decoding CBVarInt
		if (x > 253){
			CBGetMessage(self)->events->onErrorReceived(CB_ERROR_MESSAGE_DESERIALISATION_BAD_BYTES,"Attempting to deserialise a CBVersion with a var string that is too big.");
			return 0;
		}
		CBVarInt varInt = CBVarIntDecode(bytes, 80);
		if (bytes->length < 84 + varInt.size + varInt.val) {
			CBGetMessage(self)->events->onErrorReceived(CB_ERROR_MESSAGE_DESERIALISATION_BAD_BYTES,"Attempting to deserialise a CBVersion without enough space to cover the userAgent and block height.");
			return 0;
		}
		if (varInt.val > 400) { // cbitcoin accepts userAgents upto 400 bytes
			CBGetMessage(self)->events->onErrorReceived(CB_ERROR_MESSAGE_DESERIALISATION_BAD_BYTES,"Attempting to deserialise a CBVersion with a userAgent over 400 bytes.");
			return 0;
		}
		self->userAgent = CBNewByteArraySubReference(bytes, 80 + varInt.size, (u_int32_t)varInt.val);
		self->blockHeight = CBByteArrayReadInt32(bytes, 80 + varInt.size + (u_int32_t)varInt.val);
		return 84 + varInt.size + (u_int32_t)varInt.val;
	}else return 46; // Not the further message.
}
u_int32_t CBVersionSerialise(CBVersion * self){
	CBByteArray * bytes = CBGetMessage(self)->bytes;
	if (!bytes) {
		CBGetMessage(self)->events->onErrorReceived(CB_ERROR_MESSAGE_DESERIALISATION_NULL_BYTES,"Attempting to serialise a CBVersion with no bytes.");
		return 0;
	}
	if (bytes->length < 46) {
		CBGetMessage(self)->events->onErrorReceived(CB_ERROR_MESSAGE_DESERIALISATION_BAD_BYTES,"Attempting to serialise a CBVersion with less than 46 bytes.");
		return 0;
	}
	CBByteArraySetInt32(bytes, 0, self->version);
	CBByteArraySetInt64(bytes, 4, self->services);
	CBByteArraySetInt64(bytes, 12, self->time);
	CBGetMessage(self->addRecv)->bytes = CBByteArraySubReference(bytes, 20, bytes->length-20);
	u_int32_t len = CBNetworkAddressSerialise(self->addRecv,false);
	if (!len) {
		CBGetMessage(self)->events->onErrorReceived(CB_ERROR_MESSAGE_DESERIALISATION_BAD_BYTES,"CBVersion cannot be serialised because of an error with the receiving CBNetworkAddress");
		return 0;
	}
	CBGetMessage(self->addRecv)->bytes->length = len;
	if (self->version >= 106) {
		if (bytes->length < 85) {
			CBGetMessage(self)->events->onErrorReceived(CB_ERROR_MESSAGE_DESERIALISATION_BAD_BYTES,"Attempting to serialise a CBVersion with less than 85 bytes required.");
			return 0;
		}
		if (self->userAgent->length > 400) {
			CBGetMessage(self)->events->onErrorReceived(CB_ERROR_MESSAGE_DESERIALISATION_BAD_BYTES,"Attempting to serialise a CBVersion with a userAgent over 400 bytes.");
			return 0;
		}
		if (bytes->length < 85) {
			CBGetMessage(self)->events->onErrorReceived(CB_ERROR_MESSAGE_DESERIALISATION_BAD_BYTES,"Attempting to deserialise a CBVersion with less than 85 bytes required.");
			return 0;
		}
		CBGetMessage(self->addSource)->bytes = CBByteArraySubReference(bytes, 46, bytes->length-46);
		u_int32_t len = CBNetworkAddressSerialise(self->addSource,false);
		if (!len) {
			CBGetMessage(self)->events->onErrorReceived(CB_ERROR_MESSAGE_DESERIALISATION_BAD_BYTES,"CBVersion cannot be serialised because of an error with the source CBNetworkAddress");
			return 0;
		}
		CBGetMessage(self->addSource)->bytes->length = len;
		CBByteArraySetInt64(bytes, 72, self->nounce);
		CBVarInt varInt = CBVarIntFromUInt64(self->userAgent->length);
		CBVarIntEncode(bytes, 80, varInt);
		if (bytes->length < 84 + varInt.size + varInt.val) {
			CBGetMessage(self)->events->onErrorReceived(CB_ERROR_MESSAGE_DESERIALISATION_BAD_BYTES,"Attempting to deserialise a CBVersion without enough space to cover the userAgent and block height.");
			return 0;
		}
		CBByteArrayCopyByteArray(bytes, 80 + varInt.size,self->userAgent);
		CBByteArrayChangeReference(self->userAgent,bytes,80 + varInt.size);
		CBByteArraySetInt32(bytes, 80 + varInt.size + (u_int32_t)varInt.val, self->blockHeight);
		return 84 + varInt.size + (u_int32_t)varInt.val;
	}else return 46; // Not the further message.
}
