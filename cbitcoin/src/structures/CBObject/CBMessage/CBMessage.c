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

//  SEE HEADER FILE FOR DOCUMENTATION

#include "CBMessage.h"

//  Virtual Table Store

static void * VTStore = NULL;
static int objectNum = 0;

//  Constructor

CBMessage * CBNewMessageByObject(void * params,u_int32_t length,u_int32_t protocolVersion,CBEvents * events){
	CBMessage * self = malloc(sizeof(*self));
	objectNum++;
	CBAddVTToObject(CBGetObject(self), &VTStore, &CBCreateMessageVT);
	if (CBInitMessageByObject(self,params,length,protocolVersion,events))
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
	VT->deserialise = (bool (*)(void *)) CBMessageBitcoinDeserialise;
	VT->serialise = (bool (*)(void *))CBMessageBitcoinSerialise;
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

bool CBInitMessageByObject(CBMessage * self,void * params,u_int32_t length,u_int32_t protocolVersion,CBEvents * events){
	if (!CBInitObject(CBGetObject(self)))
		return false;
	self->params = params;
	self->bytes = NULL;
	self->length = length;
	self->protocolVersion = protocolVersion;
	self->events = events;
	return true;
}
bool CBInitMessageByData(CBMessage * self,void * params,CBByteArray * data,u_int32_t protocolVersion,CBEvents * events){
	if (!CBInitObject(CBGetObject(self)))
		return false;
	self->params = params;
	self->bytes = data;
	CBGetObjectVT(data)->retain(data); // Retain data for this object.
	self->protocolVersion = protocolVersion;
	self->events = events;
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

bool CBMessageBitcoinDeserialise(CBMessage * self){
	self->events->onErrorReceived(CB_ERROR_MESSAGE_NO_DESERIALISATION_IMPLEMENTATION,"CBMessageProcessBitcoinDeserialise has not been overriden. Returning no data.");
	return false;
}
bool CBMessageBitcoinSerialise(CBMessage * self){
	self->events->onErrorReceived(CB_ERROR_MESSAGE_NO_SERIALISATION_IMPLEMENTATION,"CBMessageProcessBitcoinSerialise has not been overriden. Returning no data.");
	return false;
}
bool CBMessageSetChecksum(CBMessage * self,CBByteArray * checksum){
	if (checksum->length != 4) {
		self->events->onErrorReceived(CB_ERROR_MESSAGE_CHECKSUM_BAD_SIZE,"Tried to set a checksum to a message that was %i bytes in length. Checksums should be 4 bytes long.",checksum->length);
		return false;
	}
	self->checksum = checksum;
	return true;
}
