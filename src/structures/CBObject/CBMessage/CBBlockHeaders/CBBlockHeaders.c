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

CBBlockHeaders * CBNewBlockHeaders(void (*onErrorReceived)(CBError error,char *,...)){
	CBBlockHeaders * self = malloc(sizeof(*self));
	if (NOT self) {
		onErrorReceived(CB_ERROR_OUT_OF_MEMORY,"Cannot allocate %i bytes of memory in CBNewBlockHeaders\n",sizeof(*self));
		return NULL;
	}
	CBGetObject(self)->free = CBFreeBlockHeaders;
	if(CBInitBlockHeaders(self,onErrorReceived))
		return self;
	free(self);
	return NULL;
}
CBBlockHeaders * CBNewBlockHeadersFromData(CBByteArray * data,void (*onErrorReceived)(CBError error,char *,...)){
	CBBlockHeaders * self = malloc(sizeof(*self));
	if (NOT self) {
		onErrorReceived(CB_ERROR_OUT_OF_MEMORY,"Cannot allocate %i bytes of memory in CBNewBlockHeadersFromData\n",sizeof(*self));
		return NULL;
	}
	CBGetObject(self)->free = CBFreeBlockHeaders;
	if(CBInitBlockHeadersFromData(self,data,onErrorReceived))
		return self;
	free(self);
	return NULL;
}

//  Object Getter

CBBlockHeaders * CBGetBlockHeaders(void * self){
	return self;
}

//  Initialisers

bool CBInitBlockHeaders(CBBlockHeaders * self,void (*onErrorReceived)(CBError error,char *,...)){
	self->headerNum = 0;
	self->blockHeaders = NULL;
	if (NOT CBInitMessageByObject(CBGetMessage(self), onErrorReceived))
		return false;
	return true;
}
bool CBInitBlockHeadersFromData(CBBlockHeaders * self,CBByteArray * data,void (*onErrorReceived)(CBError error,char *,...)){
	self->headerNum = 0;
	self->blockHeaders = NULL;
	if (NOT CBInitMessageByData(CBGetMessage(self), data, onErrorReceived))
		return false;
	return true;
}

//  Destructor

void CBFreeBlockHeaders(void * vself){
	CBBlockHeaders * self = vself;
	for (uint16_t x = 0; x < self->headerNum; x++) {
		CBReleaseObject(self->blockHeaders[x]);
	}
	free(self->blockHeaders);
	CBFreeMessage(self);
}

//  Functions

bool CBBlockHeadersAddBlockHeader(CBBlockHeaders * self,CBBlock * header){
	CBRetainObject(header);
	return CBBlockHeadersTakeBlockHeader(self,header);
}
uint32_t CBBlockHeadersCalculateLength(CBBlockHeaders * self){
	return CBVarIntSizeOf(self->headerNum) + self->headerNum * 81;
}
uint32_t CBBlockHeadersDeserialise(CBBlockHeaders * self){
	CBByteArray * bytes = CBGetMessage(self)->bytes;
	if (NOT bytes) {
		CBGetMessage(self)->onErrorReceived(CB_ERROR_MESSAGE_DESERIALISATION_NULL_BYTES,"Attempting to deserialise a CBBlockHeaders with no bytes.");
		return 0;
	}
	if (bytes->length < 82) {
		CBGetMessage(self)->onErrorReceived(CB_ERROR_MESSAGE_DESERIALISATION_BAD_BYTES,"Attempting to deserialise a CBBlockHeaders with less bytes than required for one header.");
		return 0;
	}
	CBVarInt headerNum = CBVarIntDecode(bytes, 0);
	if (headerNum.val > 2000) {
		CBGetMessage(self)->onErrorReceived(CB_ERROR_MESSAGE_DESERIALISATION_BAD_BYTES,"Attempting to deserialise a CBBlockHeaders with a var int over 2000.");
		return 0;
	}
	// Deserialise each header
	self->blockHeaders = malloc(sizeof(*self->blockHeaders) * (size_t)headerNum.val);
	if (NOT self->blockHeaders) {
		CBGetMessage(self)->onErrorReceived(CB_ERROR_OUT_OF_MEMORY,"Cannot allocate %i bytes of memory in CBBlockHeadersDeserialise\n",sizeof(*self->blockHeaders) * (size_t)headerNum.val);
		return 0;
	}
	self->headerNum = headerNum.val;
	uint16_t cursor = headerNum.size;
	for (uint16_t x = 0; x < headerNum.val; x++) {
		// Make new CBBlock from the rest of the data.
		CBByteArray * data = CBByteArraySubReference(bytes, cursor, bytes->length-cursor);
		if (NOT data) {
			CBGetMessage(self)->onErrorReceived(CB_ERROR_INIT_FAIL,"Cannot create a new CBByteArray in CBBlockHeadersDeserialise for the header number %u.",x);
			return 0;
		}
		self->blockHeaders[x] = CBNewBlockFromData(data, CBGetMessage(self)->onErrorReceived);
		if (NOT self->blockHeaders[x]){
			CBGetMessage(self)->onErrorReceived(CB_ERROR_INIT_FAIL,"Cannot create a new CBBlock in CBBlockHeadersDeserialise for the header number %u.",x);
			CBReleaseObject(data);
			return 0;
		}
		// Deserialise
		uint8_t len = CBBlockDeserialise(self->blockHeaders[x],false); // false for no transactions. Only the header.
		if (NOT len){
			CBGetMessage(self)->onErrorReceived(CB_ERROR_MESSAGE_DESERIALISATION_BAD_BYTES,"CBBlockHeaders cannot be deserialised because of an error with the CBBlock number %u.",x);
			CBReleaseObject(data);
			return 0;
		}
		// Adjust length
		data->length = len;
		CBReleaseObject(data);
		cursor += len;
	}
	return cursor;
}
uint32_t CBBlockHeadersSerialise(CBBlockHeaders * self){
	CBByteArray * bytes = CBGetMessage(self)->bytes;
	if (NOT bytes) {
		CBGetMessage(self)->onErrorReceived(CB_ERROR_MESSAGE_SERIALISATION_NULL_BYTES,"Attempting to serialise a CBBlockHeaders with no bytes.");
		return 0;
	}
	if (bytes->length < 81 * self->headerNum) {
		CBGetMessage(self)->onErrorReceived(CB_ERROR_MESSAGE_DESERIALISATION_BAD_BYTES,"Attempting to deserialise a CBBlockHeaders with less bytes than minimally required.");
		return 0;
	}
	CBVarInt num = CBVarIntFromUInt64(self->headerNum);
	CBVarIntEncode(bytes, 0, num);
	uint16_t cursor = num.size;
	for (uint16_t x = 0; x < num.val; x++) {
		CBGetMessage(self->blockHeaders[x])->bytes = CBByteArraySubReference(bytes, cursor, bytes->length-cursor);
		if (NOT CBGetMessage(self->blockHeaders[x])->bytes) {
			CBGetMessage(self)->onErrorReceived(CB_ERROR_INIT_FAIL,"Cannot create a new CBByteArray sub reference in CBBlockHeadersSerialise for the header number %u",x);
			return 0;
		}
		uint32_t len = CBBlockSerialise(self->blockHeaders[x],false); // false for no transactions.
		if (NOT len) {
			CBGetMessage(self)->onErrorReceived(CB_ERROR_MESSAGE_SERIALISATION_BAD_BYTES,"CBBlockHeaders cannot be serialised because of an error with the CBBlock number %u.",x);
			// Release CBByteArray objects to avoid problems overwritting pointer without release, if serialisation is tried again.
			for (uint8_t y = 0; y < x + 1; y++) {
				CBReleaseObject(CBGetMessage(self->blockHeaders[y])->bytes);
			}
			return 0;
		}
		CBGetMessage(self->blockHeaders[x])->bytes->length = len;
		cursor += len;
	}
	return cursor;
}
bool CBBlockHeadersTakeBlockHeader(CBBlockHeaders * self,CBBlock * header){
	self->headerNum++;
	CBBlock ** temp = realloc(self->blockHeaders, sizeof(*self->blockHeaders) * self->headerNum);
	if (NOT temp)
		return false;
	self->blockHeaders = temp;
	self->blockHeaders[self->headerNum-1] = header;
	return true;
}
