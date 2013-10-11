//
//  CBMessage.c
//  cbitcoin
//
//  Created by Matthew Mitchell on 27/04/2012.
//  Copyright (c) 2012 Matthew Mitchell
//  
//  This file is part of cbitcoin. It is subject to the license terms
//  in the LICENSE file found in the top-level directory of this
//  distribution and at http://www.cbitcoin.com/license.html. No part of
//  cbitcoin, including this file, may be copied, modified, propagated,
//  or distributed except according to the terms contained in the
//  LICENSE file.

//  SEE HEADER FILE FOR DOCUMENTATION

#include "CBMessage.h"

//  Constructor

CBMessage * CBNewMessageByObject(){
	CBMessage * self = malloc(sizeof(*self));
	CBGetObject(self)->free = CBFreeMessage;
	CBInitMessageByObject(self);
	return self;
}

//  Initialiser

void CBInitMessageByObject(CBMessage * self){
	CBInitObject(CBGetObject(self));
	self->bytes = NULL;
	self->expectResponse = false;
	self->serialised = false;
}
void CBInitMessageByData(CBMessage * self, CBByteArray * data){
	CBInitObject(CBGetObject(self));
	self->bytes = data;
	CBRetainObject(data); // Retain data for this object.
	self->expectResponse = false;
	self->serialised = true;
}

//  Destructor

void CBDestroyMessage(void * vself){
	CBMessage * self = vself;
	if (self->bytes) CBReleaseObject(self->bytes);
}
void CBFreeMessage(void * self){
	CBDestroyMessage(self);
	free(self);
}

//  Functions

void CBMessageTypeToString(CBMessageType type, char output[CB_MESSAGE_TYPE_STR_SIZE]){
	switch (type) {
		case CB_MESSAGE_TYPE_ADDR:
			strcpy(output, "addr");
			break;
		case CB_MESSAGE_TYPE_ALERT:
			strcpy(output, "alert");
			break;
		case CB_MESSAGE_TYPE_ALT:
			strcpy(output, "alt");
			break;
		case CB_MESSAGE_TYPE_BLOCK:
			strcpy(output, "block");
			break;
		case CB_MESSAGE_TYPE_GETADDR:
			strcpy(output, "getaddr");
			break;
		case CB_MESSAGE_TYPE_GETBLOCKS:
			strcpy(output, "getblocks");
			break;
		case CB_MESSAGE_TYPE_GETHEADERS:
			strcpy(output, "getheaders");
			break;
		case CB_MESSAGE_TYPE_GETDATA:
			strcpy(output, "getdata");
			break;
		case CB_MESSAGE_TYPE_HEADERS:
			strcpy(output, "headers");
			break;
		case CB_MESSAGE_TYPE_INV:
			strcpy(output, "inv");
			break;
		case CB_MESSAGE_TYPE_PING:
			strcpy(output, "ping");
			break;
		case CB_MESSAGE_TYPE_PONG:
			strcpy(output, "pong");
			break;
		case CB_MESSAGE_TYPE_TX:
			strcpy(output, "tx");
			break;
		case CB_MESSAGE_TYPE_VERACK:
			strcpy(output, "verack");
			break;
		case CB_MESSAGE_TYPE_VERSION:
			strcpy(output, "version");
			break;
		default:
			strcpy(output, "unknown");
			break;
	}
}
