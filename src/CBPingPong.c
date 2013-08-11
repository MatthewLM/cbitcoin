//
//  CBPingPong.c
//  cbitcoin
//
//  Created by Matthew Mitchell on 19/07/2012.
//  Copyright (c) 2012 Matthew Mitchell
//  
//  This file is part of cbitcoin. It is subject to the license terms
//  in the LICENSE file found in the top-level directory of this
//  distribution and at http://www.cbitcoin.com/license.html. No part of
//  cbitcoin, including this file, may be copied, modified, propagated,
//  or distributed except according to the terms contained in the
//  LICENSE file.

//  SEE HEADER FILE FOR DOCUMENTATION 

#include "CBPingPong.h"

//  Constructors

CBPingPong * CBNewPingPong(uint64_t ID){
	CBPingPong * self = malloc(sizeof(*self));
	CBGetObject(self)->free = CBFreePingPong;
	CBInitPingPong(self, ID);
	return self;
}
CBPingPong * CBNewPingPongFromData(CBByteArray * data){
	CBPingPong * self = malloc(sizeof(*self));
	CBGetObject(self)->free = CBFreePingPong;
	CBInitPingPongFromData(self, data);
	return self;
}

//  Initialisers

void CBInitPingPong(CBPingPong * self, uint64_t ID){
	self->ID = ID;
	CBInitMessageByObject(CBGetMessage(self));
}
void CBInitPingPongFromData(CBPingPong * self, CBByteArray * data){
	CBInitMessageByData(CBGetMessage(self), data);
}

//  Destructor

void CBDestroyPingPong(void * self){
	CBDestroyMessage(self);
}
void CBFreePingPong(void * self){
	CBDestroyPingPong(self);
	free(self);
}

//  Functions

uint8_t CBPingPongDeserialise(CBPingPong * self){
	CBByteArray * bytes = CBGetMessage(self)->bytes;
	if (NOT bytes) {
		CBLogError("Attempting to deserialise a CBPingPong with no bytes.");
		return 0;
	}
	if (bytes->length < 8) {
		CBLogError("Attempting to deserialise a CBPingPong with less than 8 bytes.");
		return 0;
	}
	self->ID = CBByteArrayReadInt64(bytes, 0);
	return 8;
}
uint8_t CBPingPongSerialise(CBPingPong * self){
	CBByteArray * bytes = CBGetMessage(self)->bytes;
	if (NOT bytes) {
		CBLogError("Attempting to serialise a CBPingPong with no bytes.");
		return 0;
	}
	if (bytes->length < 8) {
		CBLogError("Attempting to serialise a CBPingPong with less than 8 bytes.");
		return 0;
	}
	CBByteArraySetInt64(bytes, 0, self->ID);
	bytes->length = 8;
	CBGetMessage(self)->serialised = true;
	return 8;
}

