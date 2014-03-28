//
//  CBVersion.c
//  cbitcoin
//
//  Created by Matthew Mitchell on 01/07/2012.
//  Copyright (c) 2012 Matthew Mitchell
//  
//  This file is part of cbitcoin. It is subject to the license terms
//  in the LICENSE file found in the top-level directory of this
//  distribution and at http://www.cbitcoin.com/license.html. No part of
//  cbitcoin, including this file, may be copied, modified, propagated,
//  or distributed except according to the terms contained in the
//  LICENSE file.

//  SEE HEADER FILE FOR DOCUMENTATION 

#include "CBVersion.h"

//  Constructor

CBVersion * CBNewVersion(int32_t version, CBVersionServices services, int64_t time, CBNetworkAddress * addRecv, CBNetworkAddress * addSource, uint64_t nounce, CBByteArray * userAgent, int32_t blockHeight) {
	
	CBVersion * self = malloc(sizeof(*self));
	CBGetObject(self)->free = CBFreeVersion;
	CBInitVersion(self, version, services, time, addRecv, addSource, nounce, userAgent, blockHeight);
	
	return self;
	
}

CBVersion * CBNewVersionFromData(CBByteArray * data) {
	
	CBVersion * self = malloc(sizeof(*self));
	CBGetObject(self)->free = CBFreeVersion;
	CBInitVersionFromData(self, data);
	
	return self;
	
}

//  Initialiser

void CBInitVersion(CBVersion * self, int32_t version, CBVersionServices services, int64_t time, CBNetworkAddress * addRecv, CBNetworkAddress * addSource, uint64_t nonce, CBByteArray * userAgent, int32_t blockHeight) {
	
	self->version = version;
	self->services = services;
	self->time = time;
	self->nonce = nonce;
	self->blockHeight = blockHeight;
	
	self->addRecv = addRecv;
	CBRetainObject(addRecv);
	
	self->addSource = addSource;
	CBRetainObject(addSource);
	
	self->userAgent = userAgent;
	CBRetainObject(userAgent);
	
	CBInitMessageByObject(CBGetMessage(self));
	
}

void CBInitVersionFromData(CBVersion * self, CBByteArray * data) {
	
	self->addRecv = NULL;
	self->userAgent = NULL;
	self->addSource = NULL;
	
	CBInitMessageByData(CBGetMessage(self), data);
	
}

//  Destructor

void CBDestroyVersion(void * vself) {
	
	CBVersion * self = vself;
	
	if (self->addRecv) CBReleaseObject(self->addRecv);
	if (self->addSource) CBReleaseObject(self->addSource);
	if (self->userAgent) CBReleaseObject(self->userAgent);
	
	CBDestroyMessage(self);
	
}

void CBFreeVersion(void * self) {
	
	CBDestroyVersion(self);
	free(self);
	
}

//  Functions

uint32_t CBVersionDeserialise(CBVersion * self) {
	
	CBByteArray * bytes = CBGetMessage(self)->bytes;
	if (! bytes) {
		CBLogError("Attempting to deserialise a CBVersion with no bytes.");
		return CB_DESERIALISE_ERROR;
	}
	if (bytes->length < 85) {
		CBLogError("Attempting to deserialise a CBVersion with less than 85 bytes.");
		return CB_DESERIALISE_ERROR;
	}
	
	self->version = CBByteArrayReadInt32(bytes, 0);
	self->services = (CBVersionServices) CBByteArrayReadInt64(bytes, 4);
	self->time = CBByteArrayReadInt64(bytes, 12);
	
	// Get data from 20 bytes to the end of the byte array to deserialise the recieving network address.
	CBByteArray * data = CBByteArraySubReference(bytes, 20, bytes->length - 20);
	self->addRecv = CBNewNetworkAddressFromData(data, false);
	uint32_t len = CBNetworkAddressDeserialise(self->addRecv, false); // No time from version message.
	if (len == CB_DESERIALISE_ERROR){
		CBLogError("CBVersion cannot be deserialised because of an error with the receiving address.");
		CBReleaseObject(data);
		return CB_DESERIALISE_ERROR;
	}
	
	// Readjust CBByteArray length for recieving address
	data->length = len;
	CBReleaseObject(data);
	
	// Source address deserialisation
	data = CBByteArraySubReference(bytes, 46, bytes->length-46);
	self->addSource = CBNewNetworkAddressFromData(data, false);
	len = CBNetworkAddressDeserialise(self->addSource, false); // No time from version message.
	if (len == CB_DESERIALISE_ERROR){
		CBLogError("CBVersion cannot be deserialised because of an error with the source address.");
		CBReleaseObject(data);
		return CB_DESERIALISE_ERROR;
	}
	
	// Readjust CBByteArray length for source address
	data->length = len;
	CBReleaseObject(data);
	
	self->nonce = CBByteArrayReadInt64(bytes, 72);
	
	uint8_t x = CBByteArrayGetByte(bytes, 80); // Check length for decoding CBVarInt
	if (x > 253){
		CBLogError("Attempting to deserialise a CBVersion with a var string that is too big.");
		return CB_DESERIALISE_ERROR;
	}
	
	CBVarInt varInt = CBByteArrayReadVarInt(bytes, 80);
	if (bytes->length < 84 + varInt.size + varInt.val) {
		CBLogError("Attempting to deserialise a CBVersion without enough space to cover the userAgent and block height.");
		return CB_DESERIALISE_ERROR;
	}
	if (varInt.val > 400) { // cbitcoin accepts userAgents upto 400 bytes
		CBLogError("Attempting to deserialise a CBVersion with a userAgent over 400 bytes.");
		return CB_DESERIALISE_ERROR;
	}
	
	self->userAgent = CBNewByteArraySubReference(bytes, 80 + varInt.size, (uint32_t)varInt.val);
	self->blockHeight = CBByteArrayReadInt32(bytes, 80 + varInt.size + (uint32_t)varInt.val);
	return 84 + varInt.size + (uint32_t)varInt.val;
}
uint32_t CBVersionCalculateLength(CBVersion * self){
	uint32_t len = 46; // Version, services, time and receiving address.
	if (self->version >= 106) {
		if (self->userAgent->length > 400)
			return 0;
		len += 38 + CBVarIntSizeOf(self->userAgent->length) + self->userAgent->length; // Source address, nounce, user-agent and block height.
	}
	return len;
}

void CBVersionPrepareBytes(CBVersion * self){
	
	CBMessagePrepareBytes(CBGetMessage(self), CBVersionCalculateLength(self));
	
}

uint32_t CBVersionSerialise(CBVersion * self, bool force){
	CBByteArray * bytes = CBGetMessage(self)->bytes;
	if (! bytes) {
		CBLogError("Attempting to serialise a CBVersion with no bytes.");
		return 0;
	}
	if (bytes->length < 46) {
		CBLogError("Attempting to serialise a CBVersion with less than 46 bytes.");
		return 0;
	}
	CBByteArraySetInt32(bytes, 0, self->version);
	CBByteArraySetInt64(bytes, 4, self->services);
	CBByteArraySetInt64(bytes, 12, self->time);
	if (! CBGetMessage(self->addRecv)->serialised // Serailise if not serialised yet.
		// Serialise if force is true.
		|| force
		// If the data shares the same data as this version message, re-serialise the receiving address, in case it got overwritten.
		|| CBGetMessage(self->addRecv)->bytes->sharedData == bytes->sharedData
		// Reserialise if the address has a timestamp
		|| (CBGetMessage(self->addRecv)->bytes->length != 26)) {
		if (CBGetMessage(self->addRecv)->serialised)
			// Release old byte array
			CBReleaseObject(CBGetMessage(self->addRecv)->bytes);
		CBGetMessage(self->addRecv)->bytes = CBByteArraySubReference(bytes, 20, bytes->length-20);
		if (! CBNetworkAddressSerialise(self->addRecv, false)) {
			CBLogError("CBVersion cannot be serialised because of an error with the receiving CBNetworkAddress");
			// Release bytes to avoid problems overwritting pointer without release, if serialisation is tried again.
			CBReleaseObject(CBGetMessage(self->addRecv)->bytes);
			return 0;
		}
	}else{
		// Move serialsed data to one location
		CBByteArrayCopyByteArray(bytes, 20, CBGetMessage(self->addRecv)->bytes);
		CBByteArrayChangeReference(CBGetMessage(self->addRecv)->bytes, bytes, 20);
	}
	if (self->version >= 106) {
		if (bytes->length < 85) {
			CBLogError("Attempting to serialise a CBVersion with less than 85 bytes required.");
			return 0;
		}
		if (self->userAgent->length > 400) {
			CBLogError("Attempting to serialise a CBVersion with a userAgent over 400 bytes.");
			return 0;
		}
		if (! CBGetMessage(self->addSource)->serialised // Serailise if not serialised yet.
			// Serialise if force is true.
			|| force
			// If the data shares the same data as this version message, re-serialise the source address, in case it got overwritten.
			|| CBGetMessage(self->addSource)->bytes->sharedData == bytes->sharedData
			// Reserialise if the address has a timestamp
			|| (CBGetMessage(self->addSource)->bytes->length != 26)) {
			if (CBGetMessage(self->addSource)->serialised)
				// Release old byte array
				CBReleaseObject(CBGetMessage(self->addSource)->bytes);
			CBGetMessage(self->addSource)->bytes = CBByteArraySubReference(bytes, 46, bytes->length-46);
			if (! CBNetworkAddressSerialise(self->addSource, false)) {
				CBLogError("CBVersion cannot be serialised because of an error with the source CBNetworkAddress");
				// Release bytes to avoid problems overwritting pointer without release, if serialisation is tried again.
				CBReleaseObject(CBGetMessage(self->addSource)->bytes);
				return 0;
			}
		}else{
			// Move serialsed data to one location
			CBByteArrayCopyByteArray(bytes, 46, CBGetMessage(self->addSource)->bytes);
			CBByteArrayChangeReference(CBGetMessage(self->addSource)->bytes, bytes, 46);
		}
		CBByteArraySetInt64(bytes, 72, self->nonce);
		CBVarInt varInt = CBVarIntFromUInt64(self->userAgent->length);
		CBByteArraySetVarInt(bytes, 80, varInt);
		if (bytes->length < 84 + varInt.size + varInt.val) {
			CBLogError("Attempting to deserialise a CBVersion without enough space to cover the userAgent and block height.");
			return 0;
		}
		CBByteArrayCopyByteArray(bytes, 80 + varInt.size, self->userAgent);
		CBByteArrayChangeReference(self->userAgent, bytes, 80 + varInt.size);
		CBByteArraySetInt32(bytes, 80 + varInt.size + (uint32_t)varInt.val, self->blockHeight);
		// Ensure length is correct
		bytes->length = 84 + varInt.size + (uint32_t)varInt.val;
		CBGetMessage(self)->serialised = true;
		return bytes->length;
	}else{
		// Not the further message
		// Ensure length is correct
		bytes->length = 46;
	}
	CBGetMessage(self)->serialised = true;
	return bytes->length;
}
uint16_t CBVersionStringMaxSize(CBVersion * self){
	return 143 + CB_NETWORK_ADDR_STR_SIZE + self->userAgent->length;
}
void CBVersionToString(CBVersion * self, char output[]){
	char receiveStr[CB_NETWORK_ADDR_STR_SIZE];
	CBNetworkAddressToString(self->addRecv, receiveStr);
	sprintf(output, "Version        = %i\n"
					"Full blocks    = %u\n"
					"Timestamp      = %" PRId64 "\n"
					"Our addr       = %s\n"
					"User agent     = ",
					self->version,
					self->services | CB_SERVICE_FULL_BLOCKS,
					self->time,
					receiveStr);
	output = strchr(output, '\0');
	memcpy(output, CBByteArrayGetData(self->userAgent), self->userAgent->length);
	output += self->userAgent->length;
	sprintf(output, "\nBlock height   = %i", self->blockHeight);
}
