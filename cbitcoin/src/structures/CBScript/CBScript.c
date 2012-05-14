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
	CBGetObjectVT(self->events)->retain(self->events);
	// Parse Script ??? A point to this? Need to implement script proberly with support for non-standard scripts.
	self->cursor = 0;
	self->segmentsLen = 0;
	self->segments = NULL;
	while (self->cursor < program->length){
		int opCode = CBGetScriptVT(self)->getByte(self);
		if (opCode >= 0xF0) { // 16 bit operation code ???
			opCode = opCode << 8 | CBGetScriptVT(self)->getByte(self);
		}
		if (opCode > CB_SCRIPT_OP_0 && opCode < CB_SCRIPT_OP_PUSHDATA1){
			// Read some bytes of data, where how many is the opcode value itself.
			CBGetScriptVT(self)->addBytesToSegment(self,opCode);
		}else if (opCode == CB_SCRIPT_OP_PUSHDATA1){
			int len = CBGetScriptVT(self)->getByte(self);
			CBGetScriptVT(self)->addBytesToSegment(self,len);
		}else if (opCode == CB_SCRIPT_OP_PUSHDATA2){
			// Read a short, then read that many bytes of data.
			int len = CBGetScriptVT(self)->readUInt16(self);
			CBGetScriptVT(self)->addBytesToSegment(self,len);
		}else if (opCode == CB_SCRIPT_OP_PUSHDATA4){
			// Read a uint32, then read that many bytes of data.
			int len = CBGetScriptVT(self)->readUInt32(self);
			CBGetScriptVT(self)->addBytesToSegment(self,len);
		}else{
			self->cursor -= 1; //Go back to read opCode
			CBGetScriptVT(self)->addBytesToSegment(self,1);
		}
	}
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
	CBGetObjectVT(self->events)->release(&self->events);
	for (int x = 0; x < self->segmentsLen; x++) {
		CBGetObjectVT(self->segments[x])->release(self->segments + x);
	}
	CBFreeProcessObject(CBGetObject(self));
}

//  Functions

void CBScriptAddBytesToSegment(CBScript * self,u_int32_t length){
	self->segmentsLen++;
	self->segments = realloc(self->segments, sizeof(*self->segments)*self->segmentsLen);
	self->segments[self->segmentsLen] = CBNewByteArraySubReference(self->program, self->cursor, length);
	self->cursor += length;
}
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
bool CBScriptIsIPTransaction(CBScript * self){
	if (self->segmentsLen != 2)
		return false;
	return CBGetByteArrayVT(self->segments[1])->getByte(self->segments[1],0) == CB_SCRIPT_OP_CHECKSIG && self->segments[0]->length > 1;
}
