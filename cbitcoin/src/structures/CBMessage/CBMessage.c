//
//  CBMessage.c
//  cbitcoin
//
//  Created by Matthew Mitchell on 27/04/2012.
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

#include "CBMessage.h"

//  Virtual Table Store

static void * VTStore = NULL;
static int objectNum = 0;

//  Constructor

CBMessage * CBNewMessage(void * params,CBByteArray * bytes,int length,int protocolVersion,bool parseLazy,bool parseRetain,CBEvents * events){
	CBMessage * self = malloc(sizeof(*self));
	objectNum++;
	CBAddVTToObject(CBGetObject(self), &VTStore, &CBCreateMessageVT);
	if (CBInitMessage(self,params,bytes,length,protocolVersion,parseLazy,parseRetain,events))
		return self;
	return NULL; //Initialisation failure
}

//  Virtual Table Creation

CBMessageVT * CBCreateMessageVT(){
	CBMessageVT * VT = malloc(sizeof(*VT));
	CBSetMessageVT(VT);
	return VT;
}
void CBSetMessageVT(CBMessageVT * VT){
	CBSetObjectVT((CBObjectVT *)VT);
	((CBObjectVT *)VT)->free = (void (*)(void *))CBFreeMessage;
	VT->maybeParse = (void (*)(void *))CBMessageMaybeParse;
	VT->parse = (void (*)(void *))CBMessageParse;
	VT->parseLite = (void (*)(void *))CBMessageParseLite;
	VT->processSerialisation = (CBByteArray * (*)(void *))CBMessageProcessBitcoinSerialise;
	VT->readBytes = (CBByteArray * (*)(void *,int))CBMessageReadBytes;
	VT->readVarInt = (u_int64_t (*)(void *))CBMessageReadVarInt;
	VT->unCache = (void (*)(void *))CBMessageUnCache;
	VT->unsafeSerialise = (CBByteArray * (*)(void *))CBMessageUnsafeBitcoinSerialise;
}

//  Virtual Table Getter

CBMessageVT * CBGetMessageVT(void * self){
	return ((CBMessageVT *)(CBGetObject(self))->VT);
}

//  Object Getter

CBMessage * CBGetMessage(void * self){
	return self;
}

//  Initialiser

bool CBInitMessage(CBMessage * self,void * params,CBByteArray * bytes,int length,int protocolVersion,bool parseLazy,bool parseRetain,CBEvents * events){
	if (!CBInitObject(CBGetObject(self)))
		return false;
	self->params = params;
	self->bytes = bytes;
	self->cursor = 0;
	self->length = length;
	self->protocolVersion = protocolVersion;
	self->parseLazy = parseLazy;
	self->parseRetain = parseRetain;
	self->recached = false;
	self->events = events;
	// Retain objects
	CBGetObjectVT(self->bytes)->retain(self->bytes);
	CBGetObjectVT(self->events)->retain(&self->events);
	// Parse
	if (parseLazy){
		CBGetMessageVT(self)->parseLite(self);
		self->parsed = false;
	}else{
		CBGetMessageVT(self)->parseLite(self);
		CBGetMessageVT(self)->parse(self);
		self->parsed = true;
	}
	if (self->length == CB_BYTE_ARRAY_UNKNOWN_LENGTH){
		self->events->onErrorReceived(CB_ERROR_MESSAGE_LENGTH_NOT_SET,"Error: Length has not been set after %s parse.",self->parseLazy ? "lite" : "full");
		return false;
	}
	if (!parseRetain && self->parsed)
		CBGetObjectVT(self->bytes)->release(&self->bytes);
	return true;
}

//  Destructor

void CBFreeMessage(CBMessage * self){
	CBFreeProcessMessage(self);
	CBFree();
}
void CBFreeProcessMessage(CBMessage * self){
	CBFreeProcessObject(CBGetObject(self));
}

//  Functions


CBByteArray * CBMessageBitcoinSerialise(CBMessage * self){
	CBByteArray * arr = CBGetMessageVT(self)->unsafeSerialise(self);
	if (self->bytes == arr){
		// Needs copy since returned byte array is cached and cannot be safely changed
		CBByteArray * copy = CBGetByteArrayVT(arr)->copy(arr);
		CBGetObjectVT(arr)->release(&arr); // Release arr since it is no longer needed.
		return copy; // Return the copy.
	}
	// Just return, the returned byte array is new
	return arr;
}
int CBMessageGetLength(CBMessage * self){
	if (self->length != CB_BYTE_ARRAY_UNKNOWN_LENGTH)
		return self->length;
	CBGetMessageVT(self)->maybeParse(self);
	if (self->length != CB_BYTE_ARRAY_UNKNOWN_LENGTH)
		self->events->onErrorReceived(CB_ERROR_MESSAGE_LENGTH_NOT_SET,"Error: Cannot get length of message after full parse.");
	return self->length;
}
void CBMessageMaybeParse(CBMessage * self){
	if (self->parsed || self->bytes == NULL)
		return;
	CBGetMessageVT(self)->parse(self);
	self->parsed = true;
	if (!self->parseRetain)
		CBGetObjectVT(self->bytes)->release(&self->bytes);
}
void CBMessageParse(CBMessage * self){
	
}
void CBMessageParseLite(CBMessage * self){
	
}
CBByteArray * CBMessageProcessBitcoinSerialise(CBMessage * self){
	self->events->onErrorReceived(CB_ERROR_MESSAGE_NO_SERIALISATION_IMPLEMENTATION,"Error: CBMessageProcessBitcoinSerialise has not been overriden. Returning no data.");
	return CBNewByteArrayOfSize(0,self->events);
}
CBByteArray * CBMessageReadByteArray(CBMessage * self){
	int length = (int)CBGetMessageVT(self)->readVarInt(self);
	return CBGetMessageVT(self)->readBytes(self,length);
}
CBByteArray * CBMessageReadBytes(CBMessage * self,int length){
	CBByteArray * result = CBGetByteArrayVT(self->bytes)->subCopy(self->bytes,self->cursor,self->length);
	self->cursor += length;
	return result;
}
CBSha256Hash * CBMessageReadHash(CBMessage * self){
	CBByteArray * bytes = CBGetMessageVT(self)->readBytes(self,32);
	CBGetByteArrayVT(bytes)->reverse(bytes); // Flip bytes around
	self->cursor += 32;
	CBSha256Hash * hash = CBNewSha256HashFromByteArray(bytes, self->events);
	CBGetObjectVT(bytes)->release(&bytes); // No longer needed, the hash is needed.
	return hash;
}
u_int32_t CBMessageReadUInt32(CBMessage * self){
	u_int32_t result = CBGetByteArrayVT(self->bytes)->readUInt32(self->bytes,self->cursor);
	self->cursor += 4;
	return result;
}
u_int64_t CBMessageReadUInt64(CBMessage * self){
	u_int64_t result = CBGetByteArrayVT(self->bytes)->readUInt64(self->bytes,self->cursor);
	self->cursor += 8;
	return result;
}
u_int64_t CBMessageReadVarInt(CBMessage * self){
	CBVarInt varInt = CBVarIntDecode(self->bytes, self->cursor);
	self->cursor += varInt.size;
	return varInt.val;
}
bool CBMessageSetChecksum(CBMessage * self,CBByteArray * checksum){
	if (checksum->length != 4) {
		self->events->onErrorReceived(CB_ERROR_MESSAGE_CHECKSUM_BAD_SIZE,"Error: Tried to set a checksum to a message that was %i bytes in length. Checksums should be 4 bytes long.",checksum->length);
		return false;
	}
	self->checksum = checksum;
	return true;
}
void CBMessageUnCache(CBMessage * self){
	CBMessageMaybeParse(self);
	CBGetObjectVT(self->checksum)->release(&self->checksum);
	CBGetObjectVT(self->bytes)->release(&self->bytes);
	self->recached = false;
}
CBByteArray * CBMessageUnsafeBitcoinSerialise(CBMessage * self){
	if (self->bytes != NULL) {
		//Return byte array as it has been cached.
		CBGetObjectVT(self->bytes)->retain(self->bytes); // Retain when returning an object from a function
		return self->bytes;
	}
	if (self->parseRetain) {
		// Recache serialised data and return
		self->bytes = CBGetMessageVT(self)->processSerialisation(self);
		self->recached = true;
		CBGetObjectVT(self->bytes)->retain(self->bytes); // Retain when returning an object from a function
		return self->bytes;
	}
	//Do no caching, just receive the data. No retain needed since the array should be retained in processSerialisation
	return CBGetMessageVT(self)->processSerialisation(self);
}
