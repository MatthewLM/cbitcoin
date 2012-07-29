//
//  CBBlockHeaders.c
//  cbitcoin
//
//  Created by Matthew Mitchell on 12/07/2012.
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

#include "CBBlockHeaders.h"

//  Constructors

CBBlockHeaders * CBNewBlockHeaders(CBEvents * events){
	CBBlockHeaders * self = malloc(sizeof(*self));
	CBGetObject(self)->free = CBFreeBlockHeaders;
	CBInitBlockHeaders(self,events);
	return self;
}
CBBlockHeaders * CBNewBlockHeadersFromData(CBByteArray * data,CBEvents * events){
	CBBlockHeaders * self = malloc(sizeof(*self));
	CBGetObject(self)->free = CBFreeBlockHeaders;
	CBInitBlockHeadersFromData(self,data,events);
	return self;
}

//  Object Getter

CBBlockHeaders * CBGetBlockHeaders(void * self){
	return self;
}

//  Initialisers

bool CBInitBlockHeaders(CBBlockHeaders * self,CBEvents * events){
	self->headerNum = 0;
	self->blockHeaders = NULL;
	if (!CBInitMessageByObject(CBGetMessage(self), events))
		return false;
	return true;
}
bool CBInitBlockHeadersFromData(CBBlockHeaders * self,CBByteArray * data,CBEvents * events){
	self->headerNum = 0;
	self->blockHeaders = NULL;
	if (!CBInitMessageByData(CBGetMessage(self), data, events))
		return false;
	return true;
}

//  Destructor

void CBFreeBlockHeaders(void * vself){
	CBBlockHeaders * self = vself;
	for (u_int16_t x = 0; x < self->headerNum; x++) {
		CBReleaseObject(&self->blockHeaders[x]);
	}
	free(self->blockHeaders);
	CBFreeMessage(self);
}

//  Functions

void CBBlockHeadersAddBlockHeader(CBBlockHeaders * self,CBBlock * header){
	CBBlockHeadersTakeBlockHeader(self,header);
	CBRetainObject(header);
}
u_int32_t CBBlockHeadersDeserialise(CBBlockHeaders * self){
	CBByteArray * bytes = CBGetMessage(self)->bytes;
	if (!bytes) {
		CBGetMessage(self)->events->onErrorReceived(CB_ERROR_MESSAGE_DESERIALISATION_NULL_BYTES,"Attempting to deserialise a CBBlockHeaders with no bytes.");
		return 0;
	}
	if (bytes->length < 82) {
		CBGetMessage(self)->events->onErrorReceived(CB_ERROR_MESSAGE_DESERIALISATION_BAD_BYTES,"Attempting to deserialise a CBBlockHeaders with less bytes than required for one header.");
		return 0;
	}
	CBVarInt headerNum = CBVarIntDecode(bytes, 0);
	if (headerNum.val > 2000) {
		CBGetMessage(self)->events->onErrorReceived(CB_ERROR_MESSAGE_DESERIALISATION_BAD_BYTES,"Attempting to deserialise a CBBlockHeaders with a var int over 2000.");
		return 0;
	}
	// Deserialise each header
	self->blockHeaders = malloc(sizeof(*self->blockHeaders) * (size_t)headerNum.val);
	self->headerNum = headerNum.val;
	u_int16_t cursor = headerNum.size;
	for (u_int16_t x = 0; x < headerNum.val; x++) {
		// Make new CBBlock from the rest of the data.
		CBByteArray * data = CBByteArraySubReference(bytes, cursor, bytes->length-cursor);
		self->blockHeaders[x] = CBNewBlockFromData(data, CBGetMessage(self)->events);
		// Deserialise
		u_int8_t len = CBBlockDeserialise(self->blockHeaders[x],false); // false for no transactions. Only the header.
		if (!len) {
			CBGetMessage(self)->events->onErrorReceived(CB_ERROR_MESSAGE_DESERIALISATION_BAD_BYTES,"CBBlockHeaders cannot be deserialised because of an error with the CBBlock number %u.",x);
			// Release bytes
			CBReleaseObject(&data);
			// Because of failure release the CBInventoryItems
			for (u_int16_t y = 0; y < x + 1; y++) {
				CBReleaseObject(&self->blockHeaders[y]);
			}
			free(self->blockHeaders);
			self->blockHeaders = NULL;
			self->headerNum = 0;
			return 0;
		}
		// Adjust length
		data->length = len;
		CBReleaseObject(&data);
		cursor += len;
	}
	return cursor;
}
u_int32_t CBBlockHeadersSerialise(CBBlockHeaders * self){
	CBByteArray * bytes = CBGetMessage(self)->bytes;
	if (!bytes) {
		CBGetMessage(self)->events->onErrorReceived(CB_ERROR_MESSAGE_SERIALISATION_NULL_BYTES,"Attempting to serialise a CBBlockHeaders with no bytes.");
		return 0;
	}
	if (bytes->length < 81 * self->headerNum) {
		CBGetMessage(self)->events->onErrorReceived(CB_ERROR_MESSAGE_DESERIALISATION_BAD_BYTES,"Attempting to deserialise a CBBlockHeaders with less bytes than minimally required.");
		return 0;
	}
	CBVarInt num = CBVarIntFromUInt64(self->headerNum);
	CBVarIntEncode(bytes, 0, num);
	u_int16_t cursor = num.size;
	for (u_int16_t x = 0; x < num.val; x++) {
		CBGetMessage(self->blockHeaders[x])->bytes = CBByteArraySubReference(bytes, cursor, bytes->length-cursor);
		u_int32_t len = CBBlockSerialise(self->blockHeaders[x],false); // false for no transactions.
		if (!len) {
			CBGetMessage(self)->events->onErrorReceived(CB_ERROR_MESSAGE_SERIALISATION_BAD_BYTES,"CBBlockHeaders cannot be serialised because of an error with the CBBlock number %u.",x);
			// Release CBByteArray objects to avoid problems overwritting pointer without release, if serialisation is tried again.
			for (u_int8_t y = 0; y < x + 1; y++) {
				CBReleaseObject(&CBGetMessage(self->blockHeaders[y])->bytes);
			}
			return 0;
		}
		CBGetMessage(self->blockHeaders[x])->bytes->length = len;
		cursor += len;
	}
	return cursor;
}
void CBBlockHeadersTakeBlockHeader(CBBlockHeaders * self,CBBlock * header){
	self->headerNum++;
	self->blockHeaders = realloc(self->blockHeaders, sizeof(*self->blockHeaders) * self->headerNum);
	self->blockHeaders[self->headerNum-1] = header;
}
