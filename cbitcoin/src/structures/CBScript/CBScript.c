//
//  CBScript.c
//  cbitcoin
//
//  Created by Matthew Mitchell on 02/05/2012.
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

#include "CBScript.h"

//  Virtual Table Store

static CBScriptVT * VTStore = NULL;
static int objectNum = 0;

//  Constructor

CBScript * CBNewScript(CBNetworkParameters * params,CBByteArray * program,CBEvents * events){
	CBScript * self = malloc(sizeof(*self));
	CBAddVTToObject(CBGetObject(self), VTStore, CBCreateScriptVT);
	CBInitScript(self,params,program,events);
	return self;
}

//  Virtual Table Creation

CBScriptVT * CBCreateScriptVT(){
	CBScriptVT * VT = malloc(sizeof(*VT));
	CBSetScriptVT(VT);
	return VT;
}
void CBSetScriptVT(CBScriptVT * VT){
	CBSetObjectVT((CBObjectVT *)VT);
	((CBObjectVT *)VT)->free = (void (*)(void *))CBFreeScript;
	VT->getByte = (u_int8_t (*)(void *))CBScriptGetByte;
	VT->addBytesToSegment = (void (*)(void *,u_int32_t))CBScriptAddBytesToSegment;
	VT->readUInt16 = (u_int16_t (*)(void *))CBScriptReadUInt16;
	VT->readUInt32 = (u_int32_t (*)(void *))CBScriptReadUInt32;
	VT->readUInt64 = (u_int64_t (*)(void *))CBScriptReadUInt64;
	VT->isIPTransaction = (bool (*)(void *))CBScriptIsIPTransaction;
}

//  Virtual Table Getter

CBScriptVT * CBGetScriptVT(void * self){
	return ((CBScriptVT *)(CBGetObject(self))->VT);
}

//  Object Getter

CBScript * CBGetScript(void * self){
	return self;
}

//  Initialiser

bool CBInitScript(CBScript * self,CBNetworkParameters * params,CBByteArray * program,CBEvents * events){
	if (!CBInitObject(CBGetObject(self)))
		return false;
	self->params = params;
	self->program = program;
	self->events = events;
	// Retain objects now held
	CBGetObjectVT(self->params)->retain(self->params);
	CBGetObjectVT(self->program)->retain(self->program);
	return true;
}

//  Destructor

void CBFreeScript(CBScript * self){
	CBFreeProcessScript(self);
	CBFree();
}
void CBFreeProcessScript(CBScript * self){
	CBGetObjectVT(self->params)->release(&self->params);
	CBGetObjectVT(self->program)->release(&self->program);
	for (int x = 0; x < self->segmentsLen; x++) {
		CBGetObjectVT(self->segments[x])->release(self->segments + x);
	}
	CBFreeProcessObject(CBGetObject(self));
}

//  Functions

u_int8_t CBScriptGetByte(CBScript * self){
	u_int8_t byte = CBGetByteArrayVT(self->program)->getByte(self->program,self->cursor);
	self->cursor++;
	return byte;
}
u_int16_t CBScriptReadUInt16(CBScript * self){
	u_int16_t result = CBGetByteArrayVT(self->program)->readUInt16(self->program,self->cursor);
	self->cursor += 2;
	return result;
}
u_int32_t CBScriptReadUInt32(CBScript * self){
	u_int32_t result = CBGetByteArrayVT(self->program)->readUInt32(self->program,self->cursor);
	self->cursor += 4;
	return result;
}
u_int64_t CBScriptReadUInt64(CBScript * self){
	u_int64_t result = CBGetByteArrayVT(self->program)->readUInt64(self->program,self->cursor);
	self->cursor += 8;
	return result;
}
