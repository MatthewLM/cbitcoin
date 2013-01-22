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

CBAddressManager * CBNewAddressManager(void (*onBadTime)(void *)){
	CBAddressManager * self = malloc(sizeof(*self));
	if (NOT self) {
		CBLogError("Cannot allocate %i bytes of memory in CBNewAddressManager\n", sizeof(*self));
		return NULL;
	}
	CBGetObject(self)->free = CBFreeAddressManager;
	if(CBInitAddressManager(self, onBadTime))
		return self;
	free(self);
	return NULL;
}

//  Object Getter

CBAddressManager * CBGetAddressManager(void * self){
	return self;
}

//  Initialiser

bool CBInitAddressManager(CBAddressManager * self, void (*onBadTime)(void *)){
	if (NOT CBInitMessageByObject(CBGetMessage(self)))
		return false;
	// We start with no peers or addresses.
	self->peersNum = 0;
	self->addrNum = 0;
	// Assign arguments to structure
	self->onBadTime = onBadTime;
	// Create random number generators.
	if (NOT CBNewSecureRandomGenerator(&self->rndGen)) {
		CBLogError("Could not create a random number generator.");
		return false;
	}
	// Seed the random number generator.
	CBSecureRandomSeed(self->rndGen);
	if (NOT CBNewSecureRandomGenerator(&self->rndGenForBucketIndices)){
		CBFreeSecureRandomGenerator(self->rndGen);
		CBLogError("Could not create a random number generator for the bucket indices.");
		return false;
	}
	// Generate secret
	self->secret = CBSecureRandomInteger(self->rndGen);
	// Initialise arrays for addresses and peers.
	if (NOT CBInitAssociativeArray(&self->addresses, CBNetworkAddressCompare, CBReleaseObject)){
		CBFreeSecureRandomGenerator(self->rndGen);
		CBFreeSecureRandomGenerator(self->rndGenForBucketIndices);
		CBLogError("Could not initialise the array for the network addresses.");
		return false;
	}
	if (NOT CBInitAssociativeArray(&self->peers, CBPeerCompareByTime, CBReleaseObject)){
		CBFreeSecureRandomGenerator(self->rndGen);
		CBFreeSecureRandomGenerator(self->rndGenForBucketIndices);
		CBFreeAssociativeArray(&self->addresses);
		CBLogError("Could not initialise the array for the network peers.");
		return false;
	}
	return true;
}

//  Destructor

void CBFreeAddressManager(void * vself){
	CBAddressManager * self = vself;
	// Free the random number generators
	CBFreeSecureRandomGenerator(self->rndGenForBucketIndices);
	CBFreeSecureRandomGenerator(self->rndGen);
	// Free the arrays
	CBFreeAssociativeArray(&self->addresses);
	CBFreeAssociativeArray(&self->peers);
	CBFreeMessage(self);
}

//  Functions

bool CBAddressManagerAddAddress(CBAddressManager * self, CBNetworkAddress * addr){
	CBRetainObject(addr);
	return CBAddressManagerTakeAddress(self, addr);
}
void CBAddressManagerAdjustTime(CBAddressManager * self){
	// Get median timeOffset. Nodes are pre-ordered to the timeOffset.
	uint32_t index = (self->peersNum - 1) / 2;
	CBIterator it;
	CBAssociativeArrayGetElement(&self->peers, &it, index);
	int16_t median = ((CBPeer *)it.node->elements[it.pos])->timeOffset;
	if (NOT self->peersNum % 2){
		// Average middle pair
		CBAssociativeArrayGetElement(&self->peers, &it, index + 1);
		median = (((CBPeer *)it.node->elements[it.pos])->timeOffset + median)/2;
	}
	if (abs(median) > CB_NETWORK_TIME_ALLOWED_TIME_DRIFT) {
		// Revert to system time
		self->networkTimeOffset = 0;
		// Check to see if any peers are within 5 minutes of the system time and do not have the same time, else give a bad time error.
		bool found = false;
		for (uint16_t x = 0; x < self->peersNum; x++){
			CBAssociativeArrayGetElement(&self->peers, &it, x);
			CBPeer * peer = it.node->elements[it.pos];
			if (peer->timeOffset < 300 && peer->timeOffset){
				found = true;
				break;
			}
		}
		if (NOT found)
			self->onBadTime(self->callbackHandler);
	}else
		self->networkTimeOffset = median;
}
void CBAddressManagerClearPeers(CBAddressManager * self){
	CBFreeBTreeNode(self->peers.root, self->peers.onFree, true);
}
uint32_t CBAddressManagerGetAddresses(CBAddressManager * self, uint32_t num, CBNetworkAddress ** addresses){
	// ??? Add bias to addresses with a better score.
	uint8_t start = CBSecureRandomInteger(self->rndGen) % CB_BUCKET_NUM;
	uint8_t bucketIndex = start;
	if (num > self->addrNum)
		// We can only retrieve no more than the number of addresses we actually have
		num = self->addrNum;
	for (uint8_t x = 0; x < num; bucketIndex++) {
		// If we have gone past the allowable bucker range, return to the start
		if (bucketIndex == CB_BUCKET_NUM)
			bucketIndex = 0;
		// Check to see if we can find an address for a bucket.
		// Temporarily change comparison function to CBBucketCompare, to find an address with the bucket number.
		self->addresses.compareFunc = CBBucketCompare;
		// Now try to find a network address for the bucket.
		CBFindResult res = CBAssociativeArrayFind(&self->addresses, &bucketIndex);
		// Change the comparison function back to CBNetworkAddressCompare
		self->addresses.compareFunc = CBNetworkAddressCompare;
		if (res.found) {
			// An address was found for this bucket, insert it insto "addresses"
			addresses[x] = res.node->elements[res.pos];
			// Retain the address
			CBRetainObject(addresses[x]);
			// Increment x as we have an address.
			x++;
		}
	}
	return num;
}
void CBAddressManagerSetBucketIndex(CBAddressManager * self, CBNetworkAddress * addr){
	uint64_t group = CBAddressManagerGetGroup(self, addr);
	CBRandomSeed(self->rndGenForBucketIndices, group + self->secret); // Add the group with the secure secret generated during initialisation.
	addr->bucket = CBSecureRandomInteger(self->rndGenForBucketIndices) % CB_BUCKET_NUM;
}
uint64_t CBAddressManagerGetGroup(CBAddressManager * self, CBNetworkAddress * addr){
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
CBPeer * CBAddressManagerGetPeer(CBAddressManager * self, uint32_t x){
	CBIterator it;
	if (NOT CBAssociativeArrayGetElement(&self->peers, &it, x)){
		CBLogError("Attempted to get a peer with an out of range index");
		return NULL;
	}
	// Retain peer
	CBRetainObject(it.node->elements[it.pos]);
	return it.node->elements[it.pos];
}
CBNetworkAddress * CBAddressManagerGotNetworkAddress(CBAddressManager * self, CBNetworkAddress * addr){
	CBFindResult res = CBAssociativeArrayFind(&self->addresses, addr);
	if (res.found){
		// Retain network address
		CBRetainObject(res.node->elements[res.pos]);
		return res.node->elements[res.pos];
	}
	return NULL;
}
CBPeer * CBAddressManagerGotPeer(CBAddressManager * self, CBNetworkAddress * addr){
	CBFindResult res = CBAssociativeArrayFind(&self->peers, addr);
	if (res.found){
		// Retain peer
		CBRetainObject(res.node->elements[res.pos]);
		return res.node->elements[res.pos];
	}
	return NULL;
}
void CBAddressManagerRemoveAddress(CBAddressManager * self, CBNetworkAddress * addr){
	CBFindResult res = CBAssociativeArrayFind(&self->addresses, addr);
	if (res.found){
		// Release address
		CBReleaseObject(addr);
		CBAssociativeArrayDelete(&self->addresses, res);
	}
}
void CBAddressManagerRemovePeer(CBAddressManager * self, CBPeer * peer){
	CBFindResult res = CBAssociativeArrayFind(&self->peers, peer);
	if (res.found){
		// Release peer
		CBReleaseObject(peer);
		CBAssociativeArrayDelete(&self->peers, res);
	}
}
bool CBAddressManagerTakeAddress(CBAddressManager * self, CBNetworkAddress * addr){
	// Find the bucket for this address and insert it into the object
	CBAddressManagerSetBucketIndex(self, addr);
	// Add address to array
	if (NOT CBAssociativeArrayInsert(&self->addresses, addr, CBAssociativeArrayFind(&self->addresses, addr), NULL)){
		CBLogError("Could not insert an address into the addresses array");
		return false;
	}
	// Increase the number of addresses.
	self->addrNum++;
	return true;
}
bool CBAddressManagerTakePeer(CBAddressManager * self, CBPeer * peer){
	// Add peer to array
	if (NOT CBAssociativeArrayInsert(&self->peers, peer, CBAssociativeArrayFind(&self->peers, peer), NULL)){
		CBLogError("Could not insert a peer into the peers array");
		return false;
	}
	// Increase the number of peers.
	self->peersNum++;
	// Adjust time
	CBAddressManagerAdjustTime(self);
	return true;
}
CBCompare CBBucketCompare(void * bucket, void * address){
	uint8_t bucketNum = *(uint8_t *)bucket;
	CBNetworkAddress * addrObj = address;
	if (bucketNum > addrObj->bucket)
		return CB_COMPARE_MORE_THAN;
	if (bucketNum < addrObj->bucket)
		return CB_COMPARE_LESS_THAN;
	return CB_COMPARE_EQUAL;
}
CBCompare CBPeerCompareByTime(void * peer1, void * peer2){
	CBNetworkAddress * peerObj1 = peer1;
	CBNetworkAddress * peerObj2 = peer2;
	if (peerObj1->lastSeen < peerObj2->lastSeen)
		return CB_COMPARE_MORE_THAN;
	if (peerObj1->lastSeen > peerObj2->lastSeen)
		return CB_COMPARE_LESS_THAN;
	return CBNetworkAddressIPPortCompare(peer1, peer2);
}
CBCompare CBNetworkAddressCompare(void * address1, void * address2){
	CBNetworkAddress * addrObj1 = address1;
	CBNetworkAddress * addrObj2 = address2;
	if (addrObj1->bucket > addrObj2->bucket)
		return CB_COMPARE_MORE_THAN;
	if (addrObj1->bucket < addrObj2->bucket)
		return CB_COMPARE_LESS_THAN;
	uint64_t addrScore1 = addrObj1->lastSeen - addrObj1->penalty;
	uint64_t addrScore2 = addrObj2->lastSeen - addrObj2->penalty;
	if (addrScore1 < addrScore2)
		return CB_COMPARE_MORE_THAN;
	if (addrScore1 > addrScore2)
		return CB_COMPARE_LESS_THAN;
	return CBNetworkAddressIPPortCompare(address1, address2);
}
CBCompare CBNetworkAddressIPPortCompare(void * address1, void * address2){
	CBNetworkAddress * addrObj1 = address1;
	CBNetworkAddress * addrObj2 = address2;
	CBCompare res = CBByteArrayCompare(addrObj1->ip, addrObj2->ip);
	if (res == CB_COMPARE_EQUAL) {
		if (addrObj1->port > addrObj2->port)
			return CB_COMPARE_MORE_THAN;
		if (addrObj1->port < addrObj2->port)
			return CB_COMPARE_LESS_THAN;
	}
	return res;
}
