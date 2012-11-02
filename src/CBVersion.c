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

CBVersion * CBNewVersion(int32_t version,CBVersionServices services,int64_t time,CBNetworkAddress * addRecv,CBNetworkAddress * addSource,uint64_t nounce,CBByteArray * userAgent,int32_t blockHeight,void (*logError)(char *,...)){
	CBVersion * self = malloc(sizeof(*self));
	if (NOT self) {
		logError("Cannot allocate %i bytes of memory in CBNewVersion\n",sizeof(*self));
		return NULL;
	}
	CBGetObject(self)->free = CBFreeVersion;
	if(CBInitVersion(self,version,services,time,addRecv,addSource,nounce,userAgent,blockHeight,logError))
		return self;
	free(self);
	return NULL;
}
CBVersion * CBNewVersionFromData(CBByteArray * data,void (*logError)(char *,...)){
	CBVersion * self = malloc(sizeof(*self));
	if (NOT self) {
		logError("Cannot allocate %i bytes of memory in CBNewVersionFromData\n",sizeof(*self));
		return NULL;
	}
	CBGetObject(self)->free = CBFreeVersion;
	if(CBInitVersionFromData(self, data, logError))
		return self;
	free(self);
	return NULL;
}

//  Object Getter

CBVersion * CBGetVersion(void * self){
	return self;
}

//  Initialiser

bool CBInitVersion(CBVersion * self,int32_t version,CBVersionServices services,int64_t time,CBNetworkAddress * addRecv,CBNetworkAddress * addSource,uint64_t nonce,CBByteArray * userAgent,int32_t blockHeight,void (*logError)(char *,...)){
	self->version = version;
	self->services = services;
	self->time = time;
	self->addRecv = addRecv;
	CBRetainObject(addRecv);
	self->addSource = addSource;
	CBRetainObject(addSource);
	self->nonce = nonce;
	self->userAgent = userAgent;
	CBRetainObject(userAgent);
	self->blockHeight = blockHeight;
	if (NOT CBInitMessageByObject(CBGetMessage(self), logError))
		return false;
	return true;
}
bool CBInitVersionFromData(CBVersion * self,CBByteArray * data,void (*logError)(char *,...)){
	self->addRecv = NULL;
	self->userAgent = NULL;
	self->addSource = NULL;
	if (NOT CBInitMessageByData(CBGetMessage(self), data, logError))
		return false;
	return true;
}

//  Destructor

void CBFreeVersion(void * vself){
	CBVersion * self = vself;
	if (self->addRecv) CBReleaseObject(self->addRecv);
	if (self->addSource) CBReleaseObject(self->addSource);
	if (self->userAgent) CBReleaseObject(self->userAgent);
	CBFreeMessage(self);
}

//  Functions

uint32_t CBVersionDeserialise(CBVersion * self){
	CBByteArray * bytes = CBGetMessage(self)->bytes;
	if (NOT bytes) {
		CBGetMessage(self)->logError("Attempting to deserialise a CBVersion with no bytes.");
		return 0;
	}
	if (bytes->length < 46) {
		CBGetMessage(self)->logError("Attempting to deserialise a CBVersion with less than 46 bytes.");
		return 0;
	}
	self->version = CBByteArrayReadInt32(bytes, 0);
	self->services = (CBVersionServices) CBByteArrayReadInt64(bytes, 4);
	self->time = CBByteArrayReadInt64(bytes, 12);
	CBByteArray * data = CBByteArraySubReference(bytes, 20, bytes->length-20); // Get data from 20 bytes to the end of the byte array to deserialise the recieving network address.
	if (NOT data) {
		CBGetMessage(self)->logError("Cannot create a new CBByteArray in CBVersionDeserialise for the receiving address.");
		return 0;
	}
	self->addRecv = CBNewNetworkAddressFromData(data, CBGetMessage(self)->logError);
	if (NOT self->addRecv) {
		CBGetMessage(self)->logError("Cannot create a new CBNetworkAddress in CBVersionDeserialise for the receiving address.");
		CBReleaseObject(data);
		return 0;
	}
	uint32_t len = CBNetworkAddressDeserialise(self->addRecv, false); // No time from version message.
	if (NOT len){
		CBGetMessage(self)->logError("CBVersion cannot be deserialised because of an error with the receiving address.");
		CBReleaseObject(data);
		return 0;
	}
	// Readjust CBByteArray length for recieving address
	data->length = len;
	CBReleaseObject(data);
	if (self->version >= 106) { // ??? Very old. No need for backwards compatibility? Might as well.
		if (bytes->length < 85) {
			CBGetMessage(self)->logError("Attempting to deserialise a CBVersion with less than 85 bytes required.");
			return 0;
		}
		// Source address deserialisation
		data = CBByteArraySubReference(bytes, 46, bytes->length-46);
		if (NOT data) {
			CBGetMessage(self)->logError("Cannot create a new CBByteArray in CBVersionDeserialise for the source address.");
			return 0;
		}
		self->addSource = CBNewNetworkAddressFromData(data, CBGetMessage(self)->logError);
		if (NOT self->addSource) {
			CBGetMessage(self)->logError("Cannot create a new CBNetworkAddress in CBVersionDeserialise for the source address.");
			CBReleaseObject(data);
			return 0;
		}
		uint32_t len = CBNetworkAddressDeserialise(self->addSource, false); // No time from version message.
		if (NOT len){
			CBGetMessage(self)->logError("CBVersion cannot be deserialised because of an error with the source address.");
			CBReleaseObject(data);
			return 0;
		}
		// Readjust CBByteArray length for source address
		data->length = len;
		CBReleaseObject(data);
		self->nonce = CBByteArrayReadInt64(bytes, 72);
		uint8_t x = CBByteArrayGetByte(bytes, 80); // Check length for decoding CBVarInt
		if (x > 253){
			CBGetMessage(self)->logError("Attempting to deserialise a CBVersion with a var string that is too big.");
			return 0;
		}
		CBVarInt varInt = CBVarIntDecode(bytes, 80);
		if (bytes->length < 84 + varInt.size + varInt.val) {
			CBGetMessage(self)->logError("Attempting to deserialise a CBVersion without enough space to cover the userAgent and block height.");
			return 0;
		}
		if (varInt.val > 400) { // cbitcoin accepts userAgents upto 400 bytes
			CBGetMessage(self)->logError("Attempting to deserialise a CBVersion with a userAgent over 400 bytes.");
			return 0;
		}
		self->userAgent = CBNewByteArraySubReference(bytes, 80 + varInt.size, (uint32_t)varInt.val);
		if (NOT self->userAgent) {
			CBGetMessage(self)->logError("Cannot create a new CBByteArray in CBVersionDeserialise");
			return 0;
		}
		self->blockHeight = CBByteArrayReadInt32(bytes, 80 + varInt.size + (uint32_t)varInt.val);
		return 84 + varInt.size + (uint32_t)varInt.val;
	}else return 46; // Not the further message.
}
uint32_t CBVersionCalculateLength(CBVersion * self){
	uint32_t len = 46; // Version, services, time and receiving address.
	if (self->version >= 106) {
		if (self->userAgent->length > 400)
			return 0;
		len += 38 + CBVarIntSizeOf(self->userAgent->length) + self->userAgent->length; // Source address, nounce, user-agent and block height.
	}
	return len;
}
uint32_t CBVersionSerialise(CBVersion * self, bool force){
	CBByteArray * bytes = CBGetMessage(self)->bytes;
	if (NOT bytes) {
		CBGetMessage(self)->logError("Attempting to serialise a CBVersion with no bytes.");
		return 0;
	}
	if (bytes->length < 46) {
		CBGetMessage(self)->logError("Attempting to serialise a CBVersion with less than 46 bytes.");
		return 0;
	}
	CBByteArraySetInt32(bytes, 0, self->version);
	CBByteArraySetInt64(bytes, 4, self->services);
	CBByteArraySetInt64(bytes, 12, self->time);
	if (force && CBGetMessage(self->addRecv)->serialised)
		CBReleaseObject(CBGetMessage(self->addRecv)->bytes);
	if (NOT CBGetMessage(self->addRecv)->serialised || force) {
		CBGetMessage(self->addRecv)->bytes = CBByteArraySubReference(bytes, 20, bytes->length-20);
		if (NOT CBGetMessage(self->addRecv)->bytes) {
			CBGetMessage(self)->logError("Cannot create a new CBByteArray sub reference in CBVersionSerialise for receiving address.");
			return 0;
		}
		uint32_t len = CBNetworkAddressSerialise(self->addRecv,false);
		if (NOT len) {
			CBGetMessage(self)->logError("CBVersion cannot be serialised because of an error with the receiving CBNetworkAddress");
			// Release bytes to avoid problems overwritting pointer without release, if serialisation is tried again.
			CBReleaseObject(CBGetMessage(self->addRecv)->bytes);
			return 0;
		}
		CBGetMessage(self->addRecv)->bytes->length = len;
	}else if (CBGetMessage(self->addRecv)->bytes->sharedData != bytes->sharedData){
		// Move serialsed data to one location
		CBByteArrayCopyByteArray(bytes, 20, CBGetMessage(self->addRecv)->bytes);
		CBByteArrayChangeReference(CBGetMessage(self->addRecv)->bytes, bytes, 20);
	}
	if (self->version >= 106) {
		if (bytes->length < 85) {
			CBGetMessage(self)->logError("Attempting to serialise a CBVersion with less than 85 bytes required.");
			return 0;
		}
		if (self->userAgent->length > 400) {
			CBGetMessage(self)->logError("Attempting to serialise a CBVersion with a userAgent over 400 bytes.");
			return 0;
		}
		if (force && CBGetMessage(self->addSource)->serialised)
			CBReleaseObject(CBGetMessage(self->addSource)->bytes);
		if (NOT CBGetMessage(self->addSource)->serialised || force) {
			CBGetMessage(self->addSource)->bytes = CBByteArraySubReference(bytes, 46, bytes->length-46);
			if (NOT CBGetMessage(self->addSource)->bytes) {
				CBGetMessage(self)->logError("Cannot create a new CBByteArray sub reference in CBVersionSerialise for source address.");
				return 0;
			}
			uint32_t len = CBNetworkAddressSerialise(self->addSource,false);
			if (NOT len) {
				CBGetMessage(self)->logError("CBVersion cannot be serialised because of an error with the source CBNetworkAddress");
				// Release bytes to avoid problems overwritting pointer without release, if serialisation is tried again.
				CBReleaseObject(CBGetMessage(self->addSource)->bytes);
				return 0;
			}
			CBGetMessage(self->addSource)->bytes->length = len;
		}else if (CBGetMessage(self->addSource)->bytes->sharedData != bytes->sharedData){
			// Move serialsed data to one location
			CBByteArrayCopyByteArray(bytes, 46, CBGetMessage(self->addSource)->bytes);
			CBByteArrayChangeReference(CBGetMessage(self->addSource)->bytes, bytes, 46);
		}
		CBByteArraySetInt64(bytes, 72, self->nonce);
		CBVarInt varInt = CBVarIntFromUInt64(self->userAgent->length);
		CBVarIntEncode(bytes, 80, varInt);
		if (bytes->length < 84 + varInt.size + varInt.val) {
			CBGetMessage(self)->logError("Attempting to deserialise a CBVersion without enough space to cover the userAgent and block height.");
			return 0;
		}
		CBByteArrayCopyByteArray(bytes, 80 + varInt.size,self->userAgent);
		CBByteArrayChangeReference(self->userAgent,bytes,80 + varInt.size);
		CBByteArraySetInt32(bytes, 80 + varInt.size + (uint32_t)varInt.val, self->blockHeight);
		CBGetMessage(self)->serialised = true;
		return 84 + varInt.size + (uint32_t)varInt.val;
	}else{
		// Not the further message.
		CBGetMessage(self)->serialised = true;
		return 46;
	}
}
