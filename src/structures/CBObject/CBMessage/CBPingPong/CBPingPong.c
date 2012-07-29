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

CBPingPong * CBNewPingPong(u_int64_t ID,CBEvents * events){
	CBPingPong * self = malloc(sizeof(*self));
	CBGetObject(self)->free = CBFreePingPong;
	CBInitPingPong(self,ID,events);
	return self;
}
CBPingPong * CBNewPingPongFromData(CBByteArray * data,CBEvents * events){
	CBPingPong * self = malloc(sizeof(*self));
	CBGetObject(self)->free = CBFreePingPong;
	CBInitPingPongFromData(self,data,events);
	return self;
}

//  Object Getter

CBPingPong * CBGetPingPong(void * self){
	return self;
}

//  Initialisers

bool CBInitPingPong(CBPingPong * self,u_int64_t ID,CBEvents * events){
	self->ID = ID;
	if (!CBInitMessageByObject(CBGetMessage(self), events))
		return false;
	return true;
}
bool CBInitPingPongFromData(CBPingPong * self,CBByteArray * data,CBEvents * events){
	if (!CBInitMessageByData(CBGetMessage(self), data, events))
		return false;
	return true;
}

//  Destructor

void CBFreePingPong(void * self){
	CBFreeMessage(self);
}

//  Functions

bool CBPingPongDeserialise(CBPingPong * self){
	CBByteArray * bytes = CBGetMessage(self)->bytes;
	if (!bytes) {
		CBGetMessage(self)->events->onErrorReceived(CB_ERROR_MESSAGE_DESERIALISATION_NULL_BYTES,"Attempting to deserialise a CBPingPong with no bytes.");
		return false;
	}
	if (bytes->length < 8) {
		CBGetMessage(self)->events->onErrorReceived(CB_ERROR_MESSAGE_DESERIALISATION_BAD_BYTES,"Attempting to deserialise a CBPingPong with less than 8 bytes.");
		return 0;
	}
	self->ID = CBByteArrayReadInt64(bytes, 0);
}
bool CBPingPongSerialise(CBPingPong * self){
	CBByteArray * bytes = CBGetMessage(self)->bytes;
	if (!bytes) {
		CBGetMessage(self)->events->onErrorReceived(CB_ERROR_MESSAGE_SERIALISATION_NULL_BYTES,"Attempting to serialise a CBPingPong with no bytes.");
		return false;
	}
	if (bytes->length < 8) {
		CBGetMessage(self)->events->onErrorReceived(CB_ERROR_MESSAGE_SERIALISATION_BAD_BYTES,"Attempting to serialise a CBPingPong with less than 8 bytes.");
		return 0;
	}
	CBByteArraySetInt64(bytes, 0, self->ID);
}

