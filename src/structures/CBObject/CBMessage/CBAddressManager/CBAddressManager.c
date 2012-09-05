 //
//  CBAddressManager.c
//  cbitcoin
//
//  Created by Matthew Mitchell on 31/07/2012.
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

#include "CBAddressManager.h"

//  Constructors

CBAddressManager * CBNewAddressManager(CBEvents * events){
	CBAddressManager * self = malloc(sizeof(*self));
	if (NOT self) {
		events->onErrorReceived(CB_ERROR_OUT_OF_MEMORY,"Cannot allocate %i bytes of memory in CBNewAddressManager\n",sizeof(*self));
		return NULL;
	}
	CBGetObject(self)->free = CBFreeAddressManager;
	if(CBInitAddressManager(self,events))
		return self;
	free(self);
	return NULL;
}
CBAddressManager * CBNewAddressManagerFromData(CBByteArray * data,CBEvents * events){
	CBAddressManager * self = malloc(sizeof(*self));
	if (NOT self) {
		events->onErrorReceived(CB_ERROR_OUT_OF_MEMORY,"Cannot allocate %i bytes of memory in CBNewAddressManagerFromData\n",sizeof(*self));
		return NULL;
	}
	CBGetObject(self)->free = CBFreeAddressManager;
	if(CBInitAddressManagerFromData(self,data,events))
		return self;
	free(self);
	return NULL;
}

//  Object Getter

CBAddressManager * CBGetAddressManager(void * self){
	return self;
}

//  Initialisers

bool CBInitAddressManager(CBAddressManager * self,CBEvents * events){
	if (NOT CBAddressManagerSetup(self)) return false;
	self->secret = CBSecureRandomInteger(self->rndGen);
	if (NOT CBInitMessageByObject(CBGetMessage(self), events)){
		CBFreeSecureRandomGenerator(self->rndGen);
		return false;
	}
	return true;
}
bool CBInitAddressManagerFromData(CBAddressManager * self,CBByteArray * data,CBEvents * events){
	if (NOT CBAddressManagerSetup(self)) return false;
	if (NOT CBInitMessageByData(CBGetMessage(self), data, events)){
		CBFreeSecureRandomGenerator(self->rndGen);
		return false;
	}
	return true;
}

//  Destructor

void CBFreeAddressManager(void * vself){
	CBAddressManager * self = vself;
	CBFreeSecureRandomGenerator(self->rndGen);
	for (uint8_t x = 0; x < CB_BUCKET_NUM; x++)
		for (uint16_t y = 0; y < (self->buckets + x)->addrNum; y++)
			CBReleaseObject((self->buckets + x)->addresses[y]);
	CBFreeMessage(self);
}

//  Functions

bool CBAddressManagerAddAddress(CBAddressManager * self,CBNetworkAddress * addr){
	CBRetainObject(addr);
	return CBAddressManagerTakeAddress(self, addr);
}
void CBAddressManagerAdjustTime(CBAddressManager * self){
	// Get median timeOffset. Nodes are pre-ordered to the timeOffset.
	uint32_t index = (self->peersNum-1)/2;
	int16_t median = self->peers[index]->timeOffset;
	if (NOT self->peersNum % 2)
		// Average middle pair
		median = (self->peers[index + 1]->timeOffset + median)/2;
	if (median > CB_NETWORK_TIME_ALLOWED_TIME_DRIFT) {
		// Revert to system time
		self->networkTimeOffset = 0;
		// Check to see if any peers are within 5 minutes of the system time and do not have the same time, else give bad time error.
		bool found = false;
		for (uint16_t x = 0; x < self->peersNum; x++)
			if (self->peers[x]->timeOffset < 300 && self->peers[x]->timeOffset)
				found = true;
		if (NOT found)
			CBGetMessage(self)->events->onBadTime(self->callbackHandler,self);
	}else
		self->networkTimeOffset = median;
}
uint32_t CBAddressManagerDeserialise(CBAddressManager * self){
	CBByteArray * bytes = CBGetMessage(self)->bytes;
	if (NOT bytes) {
		CBGetMessage(self)->events->onErrorReceived(CB_ERROR_MESSAGE_DESERIALISATION_NULL_BYTES,"Attempting to deserialise a CBAddressManager with no bytes.");
		return 0;
	}
	if (bytes->length < CB_BUCKET_NUM * 2 + 12) { // The minimum size is to hold the 16 bit integers for the number of addresses, the version and the secret integer.
		CBGetMessage(self)->events->onErrorReceived(CB_ERROR_MESSAGE_DESERIALISATION_BAD_BYTES,"Attempting to deserialise a CBAddressManager with too few bytes");
		return 0;
	}
	uint32_t cursor = 4;
	for (uint8_t x = 0; x < CB_BUCKET_NUM; x++) {
		CBBucket * bucket = self->buckets + x;
		bucket->addrNum = CBByteArrayReadInt16(bytes, cursor);
		cursor += 2;
		if (bytes->length < cursor + 30 * bucket->addrNum + (CB_BUCKET_NUM - x - 1) * 2 + 8) {
			CBGetMessage(self)->events->onErrorReceived(CB_ERROR_MESSAGE_DESERIALISATION_BAD_BYTES,"Attempting to deserialise a CBAddressManager with too few bytes at bucket %u: %u < %u",x,bytes->length, cursor + 30 * bucket->addrNum + (CB_BUCKET_NUM - x - 1) * 2 + 8);
			return 0;
		}
		bucket->addresses = malloc(sizeof(*bucket->addresses) * bucket->addrNum);
		if (NOT bucket->addresses) {
			CBGetMessage(self)->events->onErrorReceived(CB_ERROR_OUT_OF_MEMORY,"Cannot allocate %i bytes of memory in CBAddressManagerDeserialise",sizeof(*bucket->addresses) * bucket->addrNum);
			// Free data.
			for (uint8_t y = 0; y < x; y++) {
				CBBucket * freeBucket = self->buckets + y;
				for (uint16_t z = 0; z < freeBucket->addrNum; x++)
					CBReleaseObject(freeBucket->addresses[z]);
				free(freeBucket->addresses);
			}
			return 0;
		}
		for (uint16_t y = 0; y < bucket->addrNum; y++) {
			CBByteArray * data = CBByteArraySubReference(bytes, cursor, 30);
			if (NOT data) {
				CBGetMessage(self)->events->onErrorReceived(CB_ERROR_INIT_FAIL,"Could not create CBByteArray in CBAddressManager for the network address number %u.",x);
				return 0;
			}
			bucket->addresses[y] = CBNewNetworkAddressFromData(data, CBGetMessage(self)->events);
			if (NOT bucket->addresses[y]) {
				CBGetMessage(self)->events->onErrorReceived(CB_ERROR_INIT_FAIL,"Could not create CBNetworkAddress in CBAddressManager for the network address number %u.",x);
				CBReleaseObject(data);
				return 0;
			}
			CBReleaseObject(data);
			if(NOT CBNetworkAddressDeserialise(bucket->addresses[y], true)){
				CBGetMessage(self)->events->onErrorReceived(CB_ERROR_MESSAGE_DESERIALISATION_BAD_BYTES,"Cannot deserialise a CBAddressManager due to an error with the network address number %u",x);
				return 0;
			}
			cursor += 30;
		}
	}
	self->secret = CBByteArrayReadInt64(bytes, cursor);
	return cursor + 8;
}
CBNetworkAddressLocator * CBAddressManagerGetAddresses(CBAddressManager * self,uint32_t num){
	uint8_t start = CBSecureRandomInteger(self->rndGen) % CB_BUCKET_NUM;
	uint8_t bucketIndex = start;
	uint16_t index = 0;
	int16_t firstEmpty = -1;
	CBNetworkAddressLocator * addrs = malloc(sizeof(*addrs) * (num + 1)); // Plus one for termination
	if (NOT addrs) {
		CBGetMessage(self)->events->onErrorReceived(CB_ERROR_OUT_OF_MEMORY,"Cannot allocate %i bytes of memory in CBAddressManagerGetAddresses\n",sizeof(*addrs) * (num + 1));
		return NULL;
	}
	uint8_t x = 0;
	while (x < num) {
		if (bucketIndex == CB_BUCKET_NUM)
			bucketIndex = 0;
		if (bucketIndex == firstEmpty)
			break;
		CBBucket * bucket = self->buckets + bucketIndex;
		if (bucket->addrNum > index) {
			// Are addresses to fetch.
			(addrs + x)->addr = bucket->addresses[bucket->addrNum - index - 1];
			(addrs + x)->bucketIndex = bucketIndex;
			(addrs + x)->addrIndex = bucket->addrNum - index - 1;
			CBRetainObject((addrs + x)->addr);
			x++;
			firstEmpty = -1; // Not got all around to first empty. Wait again until we see an empty bucket
		}else if (firstEmpty == -1)
			// First empty one we have seen.
			firstEmpty = bucketIndex;
		// Move to next bucket
		bucketIndex++;
		// If we are back at the start, increment the index
		if (bucketIndex == start)
			index++;
	}
	(addrs + x)->addr = NULL; // NULL termination
	return addrs;
}
uint8_t CBAddressManagerGetBucketIndex(CBAddressManager * self,CBNetworkAddress * addr){
	uint64_t group = CBAddressManagerGetGroup(self, addr);
	CBRandomSeed(self->rndGenForBucketIndexes, group + self->secret); // Add the group with the secure secret generated during initialisation.
	return CBSecureRandomInteger(self->rndGenForBucketIndexes) % CB_BUCKET_NUM;
}
uint64_t CBAddressManagerGetGroup(CBAddressManager * self,CBNetworkAddress * addr){
	uint8_t start = 0;
	int8_t bits = 16;
	uint64_t group;
	switch (addr->type) {
		case CB_IP_I2P:
		case CB_IP_TOR:
			group = addr->type;
			start = 6;
			bits = 4;
			break;
		case CB_IP_SITT:
		case CB_IP_RFC6052:
			group = CB_IP_IPv4;
			start = 12;
			break;
		case CB_IP_6TO4:
			group = CB_IP_IPv4;
			start = 2;
			break;
		case CB_IP_TEREDO:
			return CB_IP_IPv4 | ((CBByteArrayGetByte(addr->ip, 12) ^ 0xFF) << 8) | ((CBByteArrayGetByte(addr->ip, 13) ^ 0xFF) << 16);
		case CB_IP_HENET:
			group = CB_IP_IPv6;
			bits = 36;
			break;
		case CB_IP_IPv6:
			group = CB_IP_IPv6;
			bits = 32;
			break;
		case CB_IP_IPv4:
			group = CB_IP_IPv4;
			start = 12;
			break;
		default:
			group = addr->type;
			bits = 0;
			break;
	}
	uint8_t x = 8;
	for (; bits >= 8; bits -= 8, x += 8, start++)
		group |= CBByteArrayGetByte(addr->ip, start) << x;
	if (bits > 0)
		group |= (CBByteArrayGetByte(addr->ip, start) | ((1 << bits) - 1)) << x;
	return group;
}
uint64_t CBAddressManagerGetNumberOfAddresses(CBAddressManager * self){
	uint64_t total = self->peersNum;
	for (uint8_t x = 0; x < CB_BUCKET_NUM; x++) {
		total += (self->buckets + x)->addrNum;
	}
	return total;
}
CBNetworkAddress * CBAddressManagerGotNetworkAddress(CBAddressManager * self,CBNetworkAddress * addr){
	// Find the bucket for this address.
	CBBucket * bucket = self->buckets + CBAddressManagerGetBucketIndex(self, addr);
	// Look in that bucket for the address...
	for (uint16_t x = 0; x < bucket->addrNum; x++)
		if (CBNetworkAddressEquals(bucket->addresses[x], addr))
			// IP and port match
			return bucket->addresses[x];
	return NULL;
}
CBPeer * CBAddressManagerGotNode(CBAddressManager * self,CBNetworkAddress * addr){
	for (uint16_t x = 0; x < self->peersNum; x++)
		if (CBNetworkAddressEquals(CBGetNetworkAddress(self->peers[x]), addr))
			return self->peers[x];
	return NULL;
}
bool CBAddressManagerIsReachable(CBAddressManager * self,CBIPType type){
	if (type == CB_IP_INVALID)
		return false;
	return self->reachability & type;
}
void CBAddressManagerRemoveAddress(CBAddressManager * self,CBNetworkAddress * addr){
	CBBucket * bucket = (self->buckets + CBAddressManagerGetBucketIndex(self, addr));
	for (uint16_t x = 0; x < bucket->addrNum; x++) {
		if (CBNetworkAddressEquals(addr,bucket->addresses[x])) {
			bucket->addrNum--;
			if (bucket->addrNum) {
				// Move memory down.
				if (bucket->addrNum - x)
					memmove(bucket->addresses + x, bucket->addresses + x + 1, (bucket->addrNum - x) * sizeof(*bucket->addresses));
				// Reallocate memory.
				CBNetworkAddress ** temp = realloc(bucket->addresses, sizeof(*bucket->addresses) * bucket->addrNum);
				// Even when shrinking the memory block, memory errors can occur. Best not to make assumptions for a library.
				if (temp)
					bucket->addresses = temp;
				// If shinking failed we continue we the old larger memort block.
			}
			// Release from addresses.
			CBReleaseObject(bucket->addresses[x]->ip);
			break;
		}
	}
}
void CBAddressManagerRemoveNode(CBAddressManager * self,CBPeer * peer){
	// Find position of peer. Basic linear search (Better alternative in this case ???). Assumes peer is in the list properly or a fatal overflow is caused.
	uint16_t peerPos = 0;
	for (;; peerPos++) if (self->peers[peerPos] == peer) break;
	// Moves rest of peers down
	memmove(self->peers + peerPos, self->peers + peerPos + 1, (self->peersNum - peerPos - 1) * sizeof(*self->peers));
	self->peersNum--; // Removed peer. Don't bother reallocating memory which will be done upon a new peer.
	if (CBGetNetworkAddress(peer)->public){
		// Public peer, return to addresses list.
		CBNetworkAddress * temp = realloc(peer, sizeof(CBNetworkAddress));
		if (temp)
			// Taking the address may fail. If it does, we just ignore it and lose the address.
			CBAddressManagerTakeAddress(self, temp);
		else
			CBAddressManagerTakeAddress(self, (CBNetworkAddress *)peer); // Realloc failed so jsut use larger memory block.
	}else
		// Private peer (Never been advertised as address)
		CBReleaseObject(peer);
	// Re-adjust time without this peer. If no peers are available do not adjust anything.
	if (self->peersNum) CBAddressManagerAdjustTime(self);
}
uint32_t CBAddressManagerSerialise(CBAddressManager * self){
	CBByteArray * bytes = CBGetMessage(self)->bytes;
	if (NOT bytes) {
		CBGetMessage(self)->events->onErrorReceived(CB_ERROR_MESSAGE_SERIALISATION_NULL_BYTES,"Attempting to serialise a CBAddressManager with no bytes.");
		return 0;
	}
	if (bytes->length < CB_BUCKET_NUM * 2 + 12) {
		CBGetMessage(self)->events->onErrorReceived(CB_ERROR_MESSAGE_SERIALISATION_BAD_BYTES,"Attempting to serialise a CBAddressManager with too few bytes.");
		return 0;
	}
	CBByteArraySetInt32(bytes, 0, CB_LIBRARY_VERSION);
	uint32_t cursor = 4;
	for (uint8_t x = 0; x < CB_BUCKET_NUM; x++) {
		CBBucket * bucket = self->buckets + x;
		CBByteArraySetInt16(bytes, cursor, bucket->addrNum);
		cursor += 2;
		if (bytes->length < cursor + 30 * bucket->addrNum + (CB_BUCKET_NUM - x - 1) * 2 + 8) {
			CBGetMessage(self)->events->onErrorReceived(CB_ERROR_MESSAGE_SERIALISATION_BAD_BYTES,"Attempting to serialise a CBAddressManager with too few bytes at the bucket %u.",x);
			bucket->addrNum = 0; // Modify so that freeing works.
			return 0;
		}
		for (uint16_t y = 0; y < bucket->addrNum; y++) {
			CBGetMessage(bucket->addresses[y])->bytes = CBByteArraySubReference(bytes, cursor, 30);
			if (NOT CBGetMessage(bucket->addresses[y])->bytes) {
				CBGetMessage(self)->events->onErrorReceived(CB_ERROR_INIT_FAIL,"Cannot create a new CBByteArray sub reference in CBAddressManagerSerialise for the network address %u - %u.",x,y);
				bucket->addrNum = y; // Modify so that we can free the address manager later.
				return 0;
			}
			CBNetworkAddressSerialise(bucket->addresses[y], true);
			cursor += 30;
		}
	}
	CBByteArraySetInt64(bytes, cursor, self->secret);
	return cursor + 8;
}
void CBAddressManagerSetReachability(CBAddressManager * self, CBIPType type, bool reachable){
	if (reachable)
		self->reachability |= type;
	else
		self->reachability &= ~type;
}
bool CBAddressManagerSetup(CBAddressManager * self){
	self->peers = NULL;
	self->peersNum = 0;
	self->reachability = 0;
	// Clear buckets
	memset(self->buckets, 0, sizeof(*self->buckets) * CB_BUCKET_NUM);
	// Create random number generators.
	if (CBNewSecureRandomGenerator(&self->rndGen)) {
		CBSecureRandomSeed(self->rndGen); // Cryptographically secure.
		if (CBNewSecureRandomGenerator(&self->rndGenForBucketIndexes))
			return true;
		CBFreeSecureRandomGenerator(self->rndGen);
	}
	return false;
}
bool CBAddressManagerTakeAddress(CBAddressManager * self,CBNetworkAddress * addr){
	// Find the bucket for this address.
	CBBucket * bucket = self->buckets + CBAddressManagerGetBucketIndex(self, addr);
	// Find insersion point for address
	uint16_t insert = 0;
	for (; insert < bucket->addrNum; insert++)
		if (bucket->addresses[insert]->score > addr->score)
			// Insert here
			break;
	if (bucket->addrNum == self->maxAddressesInBucket) {
		// A lot of addresses stored, remove random address but with bias to a low scoring address.
		uint16_t remove = (CBSecureRandomInteger(self->rndGen) % bucket->addrNum);
		remove *= remove;
		remove /= bucket->addrNum;
		// Release address
		CBReleaseObject(bucket->addresses[remove]);
		if (insert < remove)
			// Insersion happens below removal. Move memory up to overwrite removed address and make space to insert.
			memmove(bucket->addresses + insert + 1, bucket->addresses + insert, (remove-insert) * sizeof(*bucket->addresses));
		else if (insert > remove){
			// Insersion happens above removal. Move memory down to overwrite removed address and make space to insert.
			memmove(bucket->addresses + remove, bucket->addresses + remove + 1, (insert-remove) * sizeof(*bucket->addresses));
			insert--; // Move insert down since we moved memory down.
		}
	}else{
		bucket->addrNum++;
		CBNetworkAddress ** temp = realloc(bucket->addresses, sizeof(*bucket->addresses) * bucket->addrNum);
		if (NOT temp)
			return false;
		bucket->addresses = temp;
		// Move memory up to allow insertion of address.
		if (bucket->addrNum - insert - 1)
			memmove(bucket->addresses + insert + 1, bucket->addresses + insert, (bucket->addrNum - insert - 1) * sizeof(*bucket->addresses));
	}
	bucket->addresses[insert] = addr;
	return true;
}
bool CBAddressManagerTakePeer(CBAddressManager * self,CBPeer * peer){
	// Find insertion point
	uint16_t insert = 0;
	for (; insert < self->peersNum; insert++)
		if (self->peers[insert]->timeOffset > peer->timeOffset)
			// Insert here
			break;
	// Reallocate peers
	self->peersNum++;
	CBPeer ** temp = realloc(self->peers, sizeof(*self->peers) * self->peersNum);
	if (NOT temp)
		return false;
	self->peers = temp;
	// Move memory up to allow for peer
	memmove(self->peers + insert + 1, self->peers + insert, (self->peersNum - insert - 1) * sizeof(*self->peers));
	// Insert
	self->peers[insert] = peer;
	// Adjust time
	CBAddressManagerAdjustTime(self);
	return true;
}
