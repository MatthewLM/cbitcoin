//
//  CBAlert.c
//  cbitcoin
//
//  Created by Matthew Mitchell on 13/07/2012.
//  Copyright (c) 2012 Matthew Mitchell
//  
//  This file is part of cbitcoin. It is subject to the license terms
//  in the LICENSE file found in the top-level directory of this
//  distribution and at http://www.cbitcoin.com/license.html. No part of
//  cbitcoin, including this file, may be copied, modified, propagated,
//  or distributed except according to the terms contained in the
//  LICENSE file.

//  SEE HEADER FILE FOR DOCUMENTATION 

#include "CBAlert.h"

//  Constructors

CBAlert * CBNewAlert(int32_t version, int64_t relayUntil, int64_t expiration, int32_t ID, int32_t cancel, int32_t minVer, int32_t maxVer, int32_t priority, CBByteArray * hiddenComment, CBByteArray * displayedComment, CBByteArray * reserved) {
	
	CBAlert * self = malloc(sizeof(*self));
	CBGetObject(self)->free = CBFreeAlert;
	CBInitAlert(self, version, relayUntil, expiration, ID, cancel, minVer, maxVer, priority, hiddenComment, displayedComment, reserved);
	
	return self;
	
}

CBAlert * CBNewAlertFromData(CBByteArray * data) {
	
	CBAlert * self = malloc(sizeof(*self));
	CBGetObject(self)->free = CBFreeAlert;
	CBInitAlertFromData(self, data);
	
	return self;
	
}

//  Initialisers

void CBInitAlert(CBAlert * self, int32_t version, int64_t relayUntil, int64_t expiration, int32_t ID, int32_t cancel, int32_t minVer, int32_t maxVer, int32_t priority, CBByteArray * hiddenComment, CBByteArray * displayedComment, CBByteArray * reserved) {
	
	self->version = version;
	self->relayUntil = relayUntil;
	self->expiration = expiration;
	self->ID = ID;
	self->cancel = cancel;
	self->minVer = minVer;
	self->maxVer = maxVer;
	self->priority = priority;
	
	self->hiddenComment = hiddenComment;
	if (hiddenComment) CBRetainObject(hiddenComment);
	
	self->displayedComment = displayedComment;
	if (displayedComment) CBRetainObject(displayedComment);
	
	self->reserved = reserved;
	if (reserved) CBRetainObject(reserved);
	
	self->signature = NULL;
	self->payload = NULL;
	
	CBInitMessageByObject(CBGetMessage(self));
	
}

void CBInitAlertFromData(CBAlert * self, CBByteArray * data) {
	
	self->setCancel = NULL;
	self->userAgents = NULL;
	self->hiddenComment = NULL;
	self->displayedComment = NULL;
	self->reserved = NULL;
	self->signature = NULL;
	self->payload = NULL;
	
	CBInitMessageByData(CBGetMessage(self), data);
	
}

//  Destructor

void CBDestroyAlert(void * vself) {
	
	CBAlert * self = vself;
	
	for (int x = 0; x < self->userAgentNum; x++)
		CBReleaseObject(self->userAgents[x]);
	
	free(self->setCancel);
	free(self->userAgents);
	
	if (self->hiddenComment) CBReleaseObject(self->hiddenComment);
	if (self->displayedComment) CBReleaseObject(self->displayedComment);
	if (self->reserved) CBReleaseObject(self->reserved);
	if (self->signature) CBReleaseObject(self->signature);
	if (self->payload) CBReleaseObject(self->payload);
	
	CBDestroyMessage(self);
	
}

void CBFreeAlert(void * self) {
	
	CBDestroyAlert(self);
	free(self);
	
}

//  Functions

void CBAlertAddCancelID(CBAlert * self, int ID) {
	
	self->setCancelNum++;
	int32_t * temp = realloc(self->setCancel, sizeof(*self->setCancel) * self->setCancelNum);
	self->setCancel = temp;
	self->setCancel[self->setCancelNum-1] = ID;
	
}

void CBAlertAddUserAgent(CBAlert * self, CBByteArray * userAgent) {
	
	CBRetainObject(userAgent);
	CBAlertTakeUserAgent(self, userAgent);
	
}

int CBAlertCalculateLength(CBAlert * self) {
	
	int len = 40 + CBVarIntSizeOf(self->userAgentNum) + CBVarIntSizeOf(self->setCancelNum) + self->setCancelNum * 4;
	
	len += self->displayedComment ? self->displayedComment->length : 1;
	len += self->hiddenComment ? self->hiddenComment->length : 1;
	len += self->reserved ? self->reserved->length : 1;
	
	for (int x = 0; x < self->userAgentNum; x++)
		len += CBVarIntSizeOf(self->userAgents[x]->length) + self->userAgents[x]->length;
	
	len += CBVarIntSizeOf(len);
	
	return len + CBVarIntSizeOf(self->signature->length) + self->signature->length;
	
}

int CBAlertDeserialise(CBAlert * self) {
	
	CBByteArray * bytes = CBGetMessage(self)->bytes;

	if (! bytes) {
		CBLogError("Attempting to deserialise a CBAlert with no bytes.");
		return CB_DESERIALISE_ERROR;
	}
	if (bytes->length < 47) {
		CBLogError("Attempting to deserialise a CBAlert with less than 47 bytes minimally required.");
		return CB_DESERIALISE_ERROR;
	}
	
	CBVarInt payloadLen = CBByteArrayReadVarInt(bytes, 0);
	if (bytes->length < payloadLen.size + payloadLen.val + 1) { // Plus one byte for signature var int. After this check the payload size is used to check the payload contents.
		CBLogError("Attempting to deserialise a CBAlert with less bytes than required for payload.");
		return CB_DESERIALISE_ERROR;
	}
	if (payloadLen.val > 2000) { // Prevent too much memory being used.
		CBLogError("Attempting to deserialise a CBAlert with a payload var int larger than 2000 bytes.");
		return CB_DESERIALISE_ERROR;
	}
	if (payloadLen.val < 45) {
		CBLogError("Attempting to deserialise a CBAlert with a payload var int smaller than 45.");
		return CB_DESERIALISE_ERROR;
	}
	
	int cursor = payloadLen.size;
	
	self->version = CBByteArrayReadInt32(bytes, cursor);
	cursor += 4;
	self->relayUntil = CBByteArrayReadInt64(bytes, cursor);
	cursor += 8;
	self->expiration = CBByteArrayReadInt64(bytes, cursor);
	cursor += 8;
	self->ID = CBByteArrayReadInt32(bytes, cursor);
	cursor += 4;
	self->cancel = CBByteArrayReadInt32(bytes, cursor);
	cursor += 4;
	
	// Add cancel ids
	CBVarInt setCancelLen = CBByteArrayReadVarInt(bytes, cursor);
	if (payloadLen.val < 44 + setCancelLen.size + setCancelLen.val * 4) {
		CBLogError("Attempting to deserialise a CBAlert with a payload var int smaller than required to cover the cancel set.");
		return CB_DESERIALISE_ERROR;
	}
	
	self->setCancelNum = setCancelLen.val;
	cursor += setCancelLen.size;
	
	if (self->setCancelNum) {
		self->setCancel = malloc(sizeof(*self->setCancel) * self->setCancelNum);
		for (int x = 0; x < self->setCancelNum; x++) {
			CBByteArrayReadInt32(bytes, cursor);
			cursor += 4;
		}
	}
	
	self->minVer = CBByteArrayReadInt32(bytes, cursor);
	cursor += 4;
	self->maxVer = CBByteArrayReadInt32(bytes, cursor);
	cursor += 4;
	
	// User Agent strings
	CBVarInt userAgentsLen = CBByteArrayReadVarInt(bytes, cursor);
	if (payloadLen.val < 7 + cursor + userAgentsLen.size + userAgentsLen.val - payloadLen.size) { // 7 for priority and 3 strings
		CBLogError("Attempting to deserialise a CBAlert with a payload var int smaller than required to cover the cancel set and the user agent set assuming empty strings.");
		return CB_DESERIALISE_ERROR;
	}
	
	self->userAgentNum = userAgentsLen.val;
	cursor += userAgentsLen.size;
	
	if (self->userAgentNum) {
		
		self->userAgents = malloc(sizeof(*self->userAgents) * self->userAgentNum);
		
		for (int x = 0; x < self->userAgentNum; x++) {
			// Add each user agent checking each time for space in the payload.
			// No need to check space as there is enough data afterwards for safety.
			CBVarInt userAgentLen = CBByteArrayReadVarInt(bytes, cursor);
			cursor += userAgentLen.size;
			
			if (payloadLen.val < 7 + cursor + userAgentLen.val + self->userAgentNum - x - payloadLen.size) { // 7 for priority and 3 strings. The current user agent size and the rest as if empty strings.
				CBLogError("Attempting to deserialise a CBAlert with a payload var int smaller than required to cover the cancel set and the user agent set up to user agent %" PRIu16 ".", x);
				return CB_DESERIALISE_ERROR;
			}
			
			// Enough space so set user agent
			if (userAgentLen.val){
				self->userAgents[x] = CBNewByteArraySubReference(bytes, cursor, (int)userAgentLen.val);
				CBByteArraySanitise(self->userAgents[x]);
			}else
				self->userAgents[x] = NULL;
			
			cursor += userAgentLen.val;
		}
		
	}
	
	self->priority = CBByteArrayReadInt32(bytes, cursor);
	cursor += 4;
	
	// Strings. Make sure to check the first byte in the var ints to ensure enough space
	int size = CBByteArrayReadVarIntSize(bytes, cursor);
	
	if (payloadLen.val < cursor + size + 2u - payloadLen.size) {
		CBLogError("Attempting to deserialise a CBAlert with a payload var int smaller than required to cover the hidden string var int.");
		return CB_DESERIALISE_ERROR;
	}
		
	CBVarInt hiddenCommentLen = CBByteArrayReadVarInt(bytes, cursor);
	cursor += size;
	
	if (payloadLen.val < cursor + hiddenCommentLen.val + 2 - payloadLen.size) {
		CBLogError("Attempting to deserialise a CBAlert with a payload var int smaller than required to cover the hidden string.");
		return CB_DESERIALISE_ERROR;
	}
		
	self->hiddenComment = hiddenCommentLen.val ? CBNewByteArraySubReference(bytes, cursor, (int)hiddenCommentLen.val) : NULL;
	cursor += hiddenCommentLen.val;
	
	// Displayed string.
	size = CBByteArrayReadVarIntSize(bytes, cursor);
	
	if (payloadLen.val >= cursor + size + 1u - payloadLen.size) {
		
		CBVarInt displayedCommentLen = CBByteArrayReadVarInt(bytes, cursor);
		cursor += size;
		
		if (payloadLen.val >= cursor + displayedCommentLen.val + 1 - payloadLen.size) {
			
			self->displayedComment = displayedCommentLen.val ? CBNewByteArraySubReference(bytes, cursor, (int)displayedCommentLen.val) : NULL;
			cursor += displayedCommentLen.val;
			
			// Reserved string
			size = CBByteArrayReadVarIntSize(bytes, cursor);
			
			if (payloadLen.val >= cursor + size - payloadLen.size) {
				
				CBVarInt reservedLen = CBByteArrayReadVarInt(bytes, cursor);
				cursor += size;
				
				if (payloadLen.val == cursor + reservedLen.val - payloadLen.size) {
					
					self->reserved = reservedLen.val ? CBNewByteArraySubReference(bytes, cursor, (int)reservedLen.val) : NULL;
					cursor += reservedLen.val;
					
					// Finally do signature
					size  = CBByteArrayReadVarIntSize(bytes, cursor);
					
					if (bytes->length >= cursor + size) {
						
						CBVarInt sigLen = CBByteArrayReadVarInt(bytes, cursor);
						cursor += size;
						
						if (bytes->length >= cursor + sigLen.val){
							
							self->payload = CBNewByteArraySubReference(bytes, payloadLen.size, (int)payloadLen.val);
							self->signature = CBNewByteArraySubReference(bytes, cursor, (int)sigLen.val);
							
							// Done signature OK. Now return successfully.
							return cursor += sigLen.val;
							
						}else
							CBLogError("Attempting to deserialise a CBAlert with a byte array length smaller than required to cover the signature.");
						
					}else
						CBLogError("Attempting to deserialise a CBAlert with a byte array length smaller than required to cover the signature var int.");
					
					// Did not pass signature. Release third string.
					if (self->reserved) CBReleaseObject(self->reserved);
					
				}else
					CBLogError("Attempting to deserialise a CBAlert with a payload var int smaller than required to cover the reserved string.");

			}else
				CBLogError("Attempting to deserialise a CBAlert with a payload var int smaller than required to cover the reserved string var int.");

			// Did not pass third string. Release second.
			if (self->displayedComment) CBReleaseObject(self->displayedComment);
			
		}else
			CBLogError("Attempting to deserialise a CBAlert with a payload var int smaller than required to cover the displayed string. %" PRIu64 " < %" PRIu64, payloadLen.val, cursor + displayedCommentLen.val + 1);
		
	}else
		CBLogError("Attempting to deserialise a CBAlert with a payload var int smaller than required to cover the displayed string var int.");
	
	// Did not pass second string. Release first.
	if (self->hiddenComment) CBReleaseObject(self->hiddenComment);
	
	// Error
	return CB_DESERIALISE_ERROR;
}

CBByteArray * CBAlertGetPayload(CBAlert * self) {
	
	CBByteArray * bytes = CBGetMessage(self)->bytes;
	CBVarInt payloadLen = CBByteArrayReadVarInt(bytes, 0);
	
	return CBNewByteArraySubReference(bytes, payloadLen.size, (int)payloadLen.val);
	
}

void CBAlertPrepareBytes(CBAlert * self) {
	
	CBMessagePrepareBytes(CBGetMessage(self), CBAlertCalculateLength(self));
	
}

bool CBAlertSerialisePayload(CBAlert * self) {
	
	CBByteArray * bytes = CBGetMessage(self)->bytes;
	if (! bytes) {
		CBLogError("Attempting to serialise a CBAlert with no bytes.");
		return false;
	}
	
	// Create varints and calculate length whilst at it.
	CBVarInt setCancelLen = CBVarIntFromUInt64(self->setCancelNum);
	CBVarInt userAgentLen = CBVarIntFromUInt64(self->userAgentNum);
	CBVarInt hiddenCommentLen = CBVarIntFromUInt64(self->hiddenComment ? self->hiddenComment->length : 0);
	CBVarInt displayedCommentLen = CBVarIntFromUInt64(self->displayedComment ? self->displayedComment->length : 0);
	CBVarInt reservedLen = CBVarIntFromUInt64(self->reserved ? self->reserved->length : 0);
	int length = 40 + setCancelLen.size + setCancelLen.val * 4 + userAgentLen.size + hiddenCommentLen.size + hiddenCommentLen.val + displayedCommentLen.size + displayedCommentLen.val + reservedLen.size + reservedLen.val;
	
	// Add user agents to length
	for (int x = 0; x < self->userAgentNum; x++)
		length += self->userAgents[x]->length + CBVarIntSizeOf(self->userAgents[x]->length);
	
	// So far length is for payload so make payload var int.
	CBVarInt payloadLen = CBVarIntFromUInt64(length);
	
	// Add the length of this var int.
	length += payloadLen.size;
	
	// Check length, with at least one byte for signature (could be 0 varint)
	if (bytes->length < length + 1) {
		CBLogError("Attempting to serialise a CBAlert with less bytes than required for the payload and a signature var int.");
		return false;
	}
	
	// Serialise the payload section.
	
	CBByteArraySetVarInt(bytes, 0, payloadLen);
	
	int cursor = payloadLen.size;
	CBByteArraySetInt32(bytes, cursor, self->version);
	cursor += 4;
	CBByteArraySetInt64(bytes, cursor, self->relayUntil);
	cursor += 8;
	CBByteArraySetInt64(bytes, cursor, self->expiration);
	cursor += 8;
	CBByteArraySetInt32(bytes, cursor, self->ID);
	cursor += 4;
	CBByteArraySetInt32(bytes, cursor, self->cancel);
	cursor += 4;
	
	// Cancel set
	CBByteArraySetVarInt(bytes, cursor, setCancelLen);
	cursor += setCancelLen.size;
	for (int x = 0; x < self->setCancelNum; x++) {
		CBByteArraySetInt32(bytes, cursor, self->setCancel[x]);
		cursor += 4;
	}
	
	CBByteArraySetInt32(bytes, cursor, self->minVer);
	cursor += 4;
	CBByteArraySetInt32(bytes, cursor, self->maxVer);
	cursor += 4;
	
	// User agent set
	CBByteArraySetVarInt(bytes, cursor, userAgentLen);
	cursor += userAgentLen.size;
	for (int x = 0; x < self->userAgentNum; x++) {
		CBVarInt aUserAgentLen = CBVarIntFromUInt64(self->userAgents[x]->length);
		CBByteArraySetVarInt(bytes, cursor, aUserAgentLen);
		cursor += aUserAgentLen.size;
		CBByteArrayCopyByteArray(bytes, cursor, self->userAgents[x]);
		CBByteArrayChangeReference(self->userAgents[x], bytes, cursor);
		cursor += aUserAgentLen.val;
	}
	
	CBByteArraySetInt32(bytes, cursor, self->priority);
	cursor += 4;
	
	// Hidden comment
	CBByteArraySetVarInt(bytes, cursor, hiddenCommentLen);
	cursor += hiddenCommentLen.size;
	if (self->hiddenComment) {
		CBByteArrayCopyByteArray(bytes, cursor, self->hiddenComment);
		CBByteArrayChangeReference(self->hiddenComment, bytes, cursor);
		cursor += hiddenCommentLen.val;
	}
	
	// Displayed comment
	CBByteArraySetVarInt(bytes, cursor, displayedCommentLen);
	cursor += displayedCommentLen.size;
	if (self->displayedComment) {
		CBByteArrayCopyByteArray(bytes, cursor, self->displayedComment);
		CBByteArrayChangeReference(self->displayedComment, bytes, cursor);
		cursor += displayedCommentLen.val;
	}
	
	// Reserved
	CBByteArraySetVarInt(bytes, cursor, reservedLen);
	cursor += reservedLen.size;
	if (self->reserved) {
		CBByteArrayCopyByteArray(bytes, cursor, self->reserved);
		CBByteArrayChangeReference(self->reserved, bytes, cursor);
		cursor += reservedLen.val;
	}
	
	self->payload = CBNewByteArraySubReference(bytes, payloadLen.size, cursor - payloadLen.size);
	
	return true;
	
}

int CBAlertSerialiseSignature(CBAlert * self, int offset) {
	
	CBByteArray * bytes = CBGetMessage(self)->bytes;
	if (! bytes) {
		CBLogError("Attempting to serialise a CBAlert with no bytes.");
		return 0;
	}
	
	CBVarInt sigLen = CBVarIntFromUInt64(self->signature->length);
	if (bytes->length < offset + sigLen.size + sigLen.val) {
		CBLogError("Attempting to serialise a CBAlert with less bytes than required for the signature.");
		return 0;
	}
	
	CBByteArraySetVarInt(bytes, offset, sigLen);
	offset += sigLen.size;
	
	CBByteArrayCopyByteArray(bytes, offset, self->signature);
	CBByteArrayChangeReference(self->signature, bytes, offset);
	
	bytes->length = offset + (int)sigLen.val;
	
	CBGetMessage(self)->serialised = true;
	
	return bytes->length;
	
}

void CBAlertTakeUserAgent(CBAlert * self, CBByteArray * userAgent) {
	
	self->userAgents = realloc(self->userAgents, sizeof(*self->userAgents) * ++self->userAgentNum);
	self->userAgents[self->userAgentNum-1] = userAgent;
	
}
