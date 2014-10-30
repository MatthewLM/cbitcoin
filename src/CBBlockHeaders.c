//
//  CBBlockHeaders.c
//  cbitcoin
//
//  Created by Matthew Mitchell on 12/07/2012.
//  Copyright (c) 2012 Matthew Mitchell
//  
//  This file is part of cbitcoin. It is subject to the license terms
//  in the LICENSE file found in the top-level directory of this
//  distribution and at http://www.cbitcoin.com/license.html. No part of
//  cbitcoin, including this file, may be copied, modified, propagated,
//  or distributed except according to the terms contained in the
//  LICENSE file.

//  SEE HEADER FILE FOR DOCUMENTATION 

#include "CBBlockHeaders.h"

//  Constructors

CBBlockHeaders * CBNewBlockHeaders() {
	
	CBBlockHeaders * self = malloc(sizeof(*self));
	CBGetObject(self)->free = CBFreeBlockHeaders;
	CBInitBlockHeaders(self);
	
	return self;
	
}

CBBlockHeaders * CBNewBlockHeadersFromData(CBByteArray * data) {
	
	CBBlockHeaders * self = malloc(sizeof(*self));
	CBGetObject(self)->free = CBFreeBlockHeaders;
	CBInitBlockHeadersFromData(self, data);
	
	return self;
	
}

//  Initialisers

void CBInitBlockHeaders(CBBlockHeaders * self) {
	
	self->headerNum = 0;
	CBInitMessageByObject(CBGetMessage(self));
	
}

void CBInitBlockHeadersFromData(CBBlockHeaders * self, CBByteArray * data) {
	
	self->headerNum = 0;
	CBInitMessageByData(CBGetMessage(self), data);
	
}

//  Destructor

void CBDestroyBlockHeaders(void * vself) {
	
	CBBlockHeaders * self = vself;
	for (uint16_t x = 0; x < self->headerNum; x++)
		CBReleaseObject(self->blockHeaders[x]);
	CBDestroyMessage(self);
	
}

void CBFreeBlockHeaders(void * self) {
	
	CBDestroyBlockHeaders(self);
	free(self);
	
}

//  Functions

void CBBlockHeadersAddBlockHeader(CBBlockHeaders * self, CBBlock * header) {
	
	CBRetainObject(header);
	CBBlockHeadersTakeBlockHeader(self, header);
	
}

uint32_t CBBlockHeadersCalculateLength(CBBlockHeaders * self) {
	
	return CBVarIntSizeOf(self->headerNum) + self->headerNum * 81;
	
}

uint32_t CBBlockHeadersDeserialise(CBBlockHeaders * self) {
	
	CBByteArray * bytes = CBGetMessage(self)->bytes;
	if (! bytes) {
		CBLogError("Attempting to deserialise a CBBlockHeaders with no bytes.");
		return CB_DESERIALISE_ERROR;
	}
	if (bytes->length < 82) {
		CBLogError("Attempting to deserialise a CBBlockHeaders with less bytes than required for one header.");
		return CB_DESERIALISE_ERROR;
	}
	
	CBVarInt headerNum = CBByteArrayReadVarInt(bytes, 0);
	if (headerNum.val > 2000) {
		CBLogError("Attempting to deserialise a CBBlockHeaders with a var int over 2000.");
		return CB_DESERIALISE_ERROR;
	}
	
	// Deserialise each header
	self->headerNum = headerNum.val;
	uint16_t cursor = headerNum.size;
	
	for (uint16_t x = 0; x < headerNum.val; x++) {
		
		// Make new CBBlock from the rest of the data.
		CBByteArray * data = CBByteArraySubReference(bytes, cursor, bytes->length-cursor);
		self->blockHeaders[x] = CBNewBlockFromData(data);
		
		// Deserialise
		uint32_t len = CBBlockDeserialise(self->blockHeaders[x], false); // false for no transactions. Only the header.
		if (len == CB_DESERIALISE_ERROR){
			CBLogError("CBBlockHeaders cannot be deserialised because of an error with the CBBlock number %" PRIu16 ".", x);
			CBReleaseObject(data);
			return CB_DESERIALISE_ERROR;
		}
		
		// Adjust length
		data->length = len;
		CBReleaseObject(data);
		cursor += len;
		
	}
	
	return cursor;
	
}

void CBBlockHeadersPrepareBytes(CBBlockHeaders * self) {
	
	CBMessagePrepareBytes(CBGetMessage(self), CBBlockHeadersCalculateLength(self));
	
}

uint32_t CBBlockHeadersSerialise(CBBlockHeaders * self, bool force) {
	
	CBByteArray * bytes = CBGetMessage(self)->bytes;
	if (! bytes) {
		CBLogError("Attempting to serialise a CBBlockHeaders with no bytes.");
		return 0;
	}
	if (bytes->length < 81 * self->headerNum) {
		CBLogError("Attempting to deserialise a CBBlockHeaders with less bytes than minimally required.");
		return 0;
	}
	
	CBVarInt num = CBVarIntFromUInt64(self->headerNum);
	CBByteArraySetVarInt(bytes, 0, num);
	uint16_t cursor = num.size;
	
	for (uint16_t x = 0; x < num.val; x++) {
		
		if (! CBGetMessage(self->blockHeaders[x])->serialised // Serailise if not serialised yet.
			// Serialise if force is true.
			|| force
			// If the data shares the same data as the block headers message, re-serialise the block header, in case it got overwritten.
			|| CBGetMessage(self->blockHeaders[x])->bytes->sharedData == bytes->sharedData) {
			
			if (CBGetMessage(self->blockHeaders[x])->serialised)
				// Release old byte array
				CBReleaseObject(CBGetMessage(self->blockHeaders[x])->bytes);
			
			CBGetMessage(self->blockHeaders[x])->bytes = CBByteArraySubReference(bytes, cursor, bytes->length-cursor);
			
			if (! CBBlockSerialise(self->blockHeaders[x], false, force)) { // false for no transactions.
				
				CBLogError("CBBlockHeaders cannot be serialised because of an error with the CBBlock number %" PRIu16 ".", x);
				
				// Release CBByteArray objects to avoid problems overwritting pointer without release, if serialisation is tried again.
				for (uint8_t y = 0; y < x + 1; y++)
					CBReleaseObject(CBGetMessage(self->blockHeaders[y])->bytes);
				
				return 0;
				
			}
			
		}else{
			
			// Move serialsed data to one location
			CBByteArrayCopyByteArray(bytes, cursor, CBGetMessage(self->blockHeaders[x])->bytes);
			CBByteArrayChangeReference(CBGetMessage(self->blockHeaders[x])->bytes, bytes, cursor);
			
		}
		
		cursor += CBGetMessage(self->blockHeaders[x])->bytes->length;
	}
	
	// Ensure length is correct
	bytes->length = cursor;
	
	// Is now serialised
	CBGetMessage(self)->serialised = true;
	
	return cursor;
	
}

void CBBlockHeadersTakeBlockHeader(CBBlockHeaders * self, CBBlock * header) {
	
	self->blockHeaders[self->headerNum++] = header;
	
}
