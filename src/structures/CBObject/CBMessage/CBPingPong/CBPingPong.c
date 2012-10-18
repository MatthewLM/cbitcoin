//
//  CBPingPong.c
//  cbitcoin
//
//  Created by Matthew Mitchell on 19/07/2012.
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

#include "CBPingPong.h"

//  Constructors

CBPingPong * CBNewPingPong(uint64_t ID,void (*onErrorReceived)(CBError error,char *,...)){
	CBPingPong * self = malloc(sizeof(*self));
	if (NOT self) {
		onErrorReceived(CB_ERROR_OUT_OF_MEMORY,"Cannot allocate %i bytes of memory in CBNewPingPong\n",sizeof(*self));
		return NULL;
	}
	CBGetObject(self)->free = CBFreePingPong;
	if(CBInitPingPong(self,ID,onErrorReceived))
		return self;
	free(self);
	return NULL;
}
CBPingPong * CBNewPingPongFromData(CBByteArray * data,void (*onErrorReceived)(CBError error,char *,...)){
	CBPingPong * self = malloc(sizeof(*self));
	if (NOT self) {
		onErrorReceived(CB_ERROR_OUT_OF_MEMORY,"Cannot allocate %i bytes of memory in CBNewPingPongFromData\n",sizeof(*self));
		return NULL;
	}
	CBGetObject(self)->free = CBFreePingPong;
	if(CBInitPingPongFromData(self,data,onErrorReceived))
		return self;
	free(self);
	return NULL;
}

//  Object Getter

CBPingPong * CBGetPingPong(void * self){
	return self;
}

//  Initialisers

bool CBInitPingPong(CBPingPong * self,uint64_t ID,void (*onErrorReceived)(CBError error,char *,...)){
	self->ID = ID;
	if (NOT CBInitMessageByObject(CBGetMessage(self), onErrorReceived))
		return false;
	return true;
}
bool CBInitPingPongFromData(CBPingPong * self,CBByteArray * data,void (*onErrorReceived)(CBError error,char *,...)){
	if (NOT CBInitMessageByData(CBGetMessage(self), data, onErrorReceived))
		return false;
	return true;
}

//  Destructor

void CBFreePingPong(void * self){
	CBFreeMessage(self);
}

//  Functions

uint8_t CBPingPongDeserialise(CBPingPong * self){
	CBByteArray * bytes = CBGetMessage(self)->bytes;
	if (NOT bytes) {
		CBGetMessage(self)->onErrorReceived(CB_ERROR_MESSAGE_DESERIALISATION_NULL_BYTES,"Attempting to deserialise a CBPingPong with no bytes.");
		return 0;
	}
	if (bytes->length < 8) {
		CBGetMessage(self)->onErrorReceived(CB_ERROR_MESSAGE_DESERIALISATION_BAD_BYTES,"Attempting to deserialise a CBPingPong with less than 8 bytes.");
		return 0;
	}
	self->ID = CBByteArrayReadInt64(bytes, 0);
	return 8;
}
uint8_t CBPingPongSerialise(CBPingPong * self){
	CBByteArray * bytes = CBGetMessage(self)->bytes;
	if (NOT bytes) {
		CBGetMessage(self)->onErrorReceived(CB_ERROR_MESSAGE_SERIALISATION_NULL_BYTES,"Attempting to serialise a CBPingPong with no bytes.");
		return 0;
	}
	if (bytes->length < 8) {
		CBGetMessage(self)->onErrorReceived(CB_ERROR_MESSAGE_SERIALISATION_BAD_BYTES,"Attempting to serialise a CBPingPong with less than 8 bytes.");
		return 0;
	}
	CBByteArraySetInt64(bytes, 0, self->ID);
	CBGetMessage(self)->serialised = true;
	return 8;
}

