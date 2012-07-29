//
//  CBGetBlocks.c
//  cbitcoin
//
//  Created by Matthew Mitchell on 11/07/2012.
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

#include "CBGetBlocks.h"

//  Constructors

CBGetBlocks * CBNewGetBlocks(u_int32_t version,CBChainDescriptor * chainDescriptor,CBByteArray * stopAtHash,CBEvents * events){
	CBGetBlocks * self = malloc(sizeof(*self));
	CBGetObject(self)->free = CBFreeGetBlocks;
	CBInitGetBlocks(self,version,chainDescriptor,stopAtHash,events);
	return self;
}
CBGetBlocks * CBNewGetBlocksFromData(CBByteArray * data,CBEvents * events){
	CBGetBlocks * self = malloc(sizeof(*self));
	CBGetObject(self)->free = CBFreeGetBlocks;
	CBInitGetBlocksFromData(self,data,events);
	return self;
}

//  Object Getter

CBGetBlocks * CBGetGetBlocks(void * self){
	return self;
}

//  Initialisers

bool CBInitGetBlocks(CBGetBlocks * self,u_int32_t version,CBChainDescriptor * chainDescriptor,CBByteArray * stopAtHash,CBEvents * events){
	self->version = version;
	self->chainDescriptor = chainDescriptor;
	CBRetainObject(chainDescriptor);
	self->stopAtHash = stopAtHash;
	CBRetainObject(stopAtHash);
	if (!CBInitMessageByObject(CBGetMessage(self), events))
		return false;
	return true;
}
bool CBInitGetBlocksFromData(CBGetBlocks * self,CBByteArray * data,CBEvents * events){
	self->chainDescriptor = NULL;
	self->stopAtHash = NULL;
	if (!CBInitMessageByData(CBGetMessage(self), data, events))
		return false;
	return true;
}

//  Destructor

void CBFreeGetBlocks(void * vself){
	CBGetBlocks * self = vself;
	if (self->chainDescriptor) CBReleaseObject(&self->chainDescriptor);
	if (self->stopAtHash) CBReleaseObject(&self->stopAtHash);
	CBFreeMessage(self);
}

//  Functions

u_int16_t CBGetBlocksDeserialise(CBGetBlocks * self){
	CBByteArray * bytes = CBGetMessage(self)->bytes;
	if (!bytes) {
		CBGetMessage(self)->events->onErrorReceived(CB_ERROR_MESSAGE_DESERIALISATION_NULL_BYTES,"Attempting to deserialise a CBGetBlocks with no bytes.");
		return 0;
	}
	if (bytes->length < 69) { // 4 version bytes, 33 chain descriptor bytes, 32 stop at hash bytes
		CBGetMessage(self)->events->onErrorReceived(CB_ERROR_MESSAGE_DESERIALISATION_BAD_BYTES,"Attempting to deserialise a CBGetBlocks with less bytes than required for one hash.");
		return 0;
	}
	self->version = CBByteArrayReadInt32(bytes, 0);
	// Deserialise the CBChainDescriptor
	CBByteArray * data = CBByteArraySubReference(bytes, 4, bytes->length-4);
	self->chainDescriptor = CBNewChainDescriptorFromData(data, CBGetMessage(self)->events);
	u_int16_t len = CBChainDescriptorDeserialise(self->chainDescriptor);
	if (!len) {
		CBGetMessage(self)->events->onErrorReceived(CB_ERROR_MESSAGE_DESERIALISATION_BAD_BYTES,"CBGetBlocks cannot be deserialised because of an error with the CBChainDescriptor.");
		CBReleaseObject(&self->chainDescriptor);
		CBReleaseObject(&data);
		return 0;
	}
	data->length = len; // Re-adjust length for the chain descriptor object's reference.
	CBReleaseObject(&data); // Release this reference.
	// Deserialise stopAtHash
	if (bytes->length < len + 36) { // The chain descriptor length plus the version and stopAtHash bytes.
		CBGetMessage(self)->events->onErrorReceived(CB_ERROR_MESSAGE_DESERIALISATION_BAD_BYTES,"Attempting to deserialise a CBGetBlocks with less bytes than required for the stopAtHash.");
		return 0;
	}
	self->stopAtHash = CBNewByteArraySubReference(bytes, len + 4, 32);
	return len + 36;
}
u_int16_t CBGetBlocksSerialise(CBGetBlocks * self){
	CBByteArray * bytes = CBGetMessage(self)->bytes;
	if (!bytes) {
		CBGetMessage(self)->events->onErrorReceived(CB_ERROR_MESSAGE_SERIALISATION_NULL_BYTES,"Attempting to serialise a CBGetBlocks with no bytes.");
		return 0;
	}
	if (bytes->length < 36 + self->chainDescriptor->hashNum * 32 + CBVarIntSizeOf(self->chainDescriptor->hashNum)) {
		CBGetMessage(self)->events->onErrorReceived(CB_ERROR_MESSAGE_SERIALISATION_BAD_BYTES,"Attempting to serialise a CBGetBlocks with less bytes than required.");
		return 0;
	}
	CBByteArraySetInt32(bytes, 0, self->version);
	// Serialise chain descriptor
	CBGetMessage(self->chainDescriptor)->bytes = CBByteArraySubReference(bytes, 4, bytes->length-4);
	u_int32_t len = CBChainDescriptorSerialise(self->chainDescriptor);
	if (!len) {
		CBGetMessage(self)->events->onErrorReceived(CB_ERROR_MESSAGE_SERIALISATION_BAD_BYTES,"CBGetBlocks cannot be serialised because of an error with the chain descriptor. This error should never occur... :o");
		// Release bytes to avoid problems overwritting pointer without release, if serialisation is tried again.
		CBReleaseObject(&CBGetMessage(self->chainDescriptor)->bytes);
		return 0;
	}
	CBGetMessage(self->chainDescriptor)->bytes->length = len;
	// Serialise stopAtHash
	CBByteArrayCopyByteArray(bytes, len + 4, self->stopAtHash);
	CBByteArrayChangeReference(self->stopAtHash, bytes, len + 4);
	return len + 36;
}

