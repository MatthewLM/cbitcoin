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
	if (NOT CBInitAssociativeArray(&self->peers, CBNetworkAddressCompare, CBReleaseObject)){
		CBFreeSecureRandomGenerator(self->rndGen);
		CBFreeSecureRandomGenerator(self->rndGenForBucketIndices);
		CBLogError("Could not initialise the array for the network peers.");
		return false;
	}
	if (NOT CBInitAssociativeArray(&self->peersByTimeOffset, CBPeerCompareByTime, NULL)){
		CBFreeSecureRandomGenerator(self->rndGen);
		CBFreeSecureRandomGenerator(self->rndGenForBucketIndices);
		CBFreeAssociativeArray(&self->peers);
		CBLogError("Could not initialise the array for the network peers sorted by time offset.");
		return false;
	}
	for (uint8_t x = 0; x < CB_BUCKET_NUM; x++) {
		if (NOT CBInitAssociativeArray(&self->addresses[x], CBNetworkAddressCompare, CBReleaseObject)){
			CBFreeSecureRandomGenerator(self->rndGen);
			CBFreeSecureRandomGenerator(self->rndGenForBucketIndices);
			CBFreeAssociativeArray(&self->peers);
			CBFreeAssociativeArray(&self->peersByTimeOffset);
			for (uint8_t y = 0; y < x; y++)
				CBFreeAssociativeArray(&self->addresses[y]);
			CBLogError("Could not initialise an array for the network addresses.");
			return false;
		}
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
	CBFreeAssociativeArray(&self->peers);
	for (uint8_t x = 0; x < CB_BUCKET_NUM; x++)
		CBFreeAssociativeArray(&self->addresses[x]);
	CBFreeMessage(self);
}

//  Functions

bool CBAddressManagerAddAddress(CBAddressManager * self, CBNetworkAddress * addr){
	CBRetainObject(addr);
	return CBAddressManagerTakeAddress(self, addr);
}
bool CBAddressManagerAddPeer(CBAddressManager * self, CBPeer * peer){
	CBRetainObject(peer);
	return CBAddressManagerTakePeer(self, peer);
}
void CBAddressManagerAdjustTime(CBAddressManager * self){
	// Get median timeOffset. Nodes are pre-ordered to the timeOffset.
	uint32_t index = (self->peersNum - 1) / 2;
	CBPosition it;
	CBAssociativeArrayGetElement(&self->peersByTimeOffset, &it, index);
	int16_t median = ((CBPeer *)it.node->elements[it.index])->timeOffset;
	if (NOT (self->peersNum % 2)){
		// Average middle pair
		CBAssociativeArrayGetElement(&self->peersByTimeOffset, &it, index + 1);
		median = (((CBPeer *)it.node->elements[it.index])->timeOffset + median) / 2;
	}
	if (abs(median) > CB_NETWORK_TIME_ALLOWED_TIME_DRIFT) {
		// Revert to system time
		self->networkTimeOffset = 0;
		// Check to see if any peers are within 5 minutes of the system time and do not have the same time, else give a bad time error.
		bool found = false;
		if (CBAssociativeArrayGetFirst(&self->peers, &it)) for (;;){
			CBPeer * peer = it.node->elements[it.index];
			if (peer->timeOffset < 300 && peer->timeOffset){
				found = true;
				break;
			}
			if (CBAssociativeArrayIterate(&self->peers, &it))
				break;
		}
		if (NOT found)
			self->onBadTime(self->callbackHandler);
	}else
		self->networkTimeOffset = median;
}
void CBAddressManagerClearPeers(CBAddressManager * self){
	CBFreeBTreeNode(self->peers.root, self->peers.onFree, true);
	CBFreeBTreeNode(self->peersByTimeOffset.root, self->peers.onFree, true);
	self->peersNum = 0;
}
uint32_t CBAddressManagerGetAddresses(CBAddressManager * self, uint32_t num, CBNetworkAddress ** addresses){
	uint8_t start = CBSecureRandomInteger(self->rndGen) % CB_BUCKET_NUM;
	uint8_t bucketIndex = start;
	if (num > self->addrNum)
		// We can only retrieve no more than the number of addresses we actually have
		num = self->addrNum;
	uint32_t index = 0;
	for (uint8_t x = 0; x < num;) {
		// See if an address is in the bucket at the index.
		CBPosition it;
		if (CBAssociativeArrayGetElement(&self->addresses[bucketIndex], &it, index)) {
			// An address was found for this bucket, insert it insto "addresses"
			addresses[x] = it.node->elements[it.index];
			// Retain the address
			CBRetainObject(addresses[x]);
			// Increment x as we have an address.
			x++;
		}
		// Move to the next bucket
		bucketIndex++;
		// If we have gone past the allowable bucker range, return to the start
		if (bucketIndex == CB_BUCKET_NUM)
			bucketIndex = 0;
		// If we have reached the start again, increase the index
		if (bucketIndex == start)
			index++;
	}
	return num;
}
void CBAddressManagerSetBucketIndex(CBAddressManager * self, CBNetworkAddress * addr){
	if (NOT addr->bucketSet) {
		uint64_t group = CBAddressManagerGetGroup(self, addr);
		CBRandomSeed(self->rndGenForBucketIndices, group + self->secret); // Add the group with the secure secret generated during initialisation.
		addr->bucket = CBSecureRandomInteger(self->rndGenForBucketIndices) % CB_BUCKET_NUM;
		addr->bucketSet = true;
	}
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
	CBPosition it;
	if (NOT CBAssociativeArrayGetElement(&self->peers, &it, x)){
		CBLogError("Attempted to get a peer with an out of range index");
		return NULL;
	}
	// Retain peer
	CBRetainObject(it.node->elements[it.index]);
	return it.node->elements[it.index];
}
CBNetworkAddress * CBAddressManagerGotNetworkAddress(CBAddressManager * self, CBNetworkAddress * addr){
	CBAddressManagerSetBucketIndex(self, addr);
	CBFindResult res = CBAssociativeArrayFind(&self->addresses[addr->bucket], addr);
	if (res.found){
		// Retain network address
		CBRetainObject(res.position.node->elements[res.position.index]);
		return res.position.node->elements[res.position.index];
	}
	return NULL;
}
CBPeer * CBAddressManagerGotPeer(CBAddressManager * self, CBNetworkAddress * addr){
	CBFindResult res = CBAssociativeArrayFind(&self->peers, addr);
	if (res.found){
		// Retain peer
		CBRetainObject(res.position.node->elements[res.position.index]);
		return res.position.node->elements[res.position.index];
	}
	return NULL;
}
void CBAddressManagerRemoveAddress(CBAddressManager * self, CBNetworkAddress * addr){
	CBAddressManagerSetBucketIndex(self, addr);
	CBFindResult res = CBAssociativeArrayFind(&self->addresses[addr->bucket], addr);
	if (res.found){
		// Release address
		CBReleaseObject(addr);
		CBAssociativeArrayDelete(&self->addresses[addr->bucket], res.position);
		self->addrNum--;
	}
}
void CBAddressManagerRemovePeer(CBAddressManager * self, CBPeer * peer){
	CBFindResult res = CBAssociativeArrayFind(&self->peers, peer);
	if (res.found){
		// Release peer
		CBReleaseObject(peer);
		// Remove from arrays
		CBAssociativeArrayDelete(&self->peers, res.position);
		CBAssociativeArrayDelete(&self->peersByTimeOffset, CBAssociativeArrayFind(&self->peersByTimeOffset, peer).position);
		// Decrement number of peers
		self->peersNum--;
	}
}
CBNetworkAddress * CBAddressManagerSelectAndRemoveAddress(CBAddressManager * self){
	uint8_t bucketIndex = CBSecureRandomInteger(self->rndGen) % CB_BUCKET_NUM;
	for (;;) {
		// See if an address is in the bucket
		CBPosition it;
		if (CBAssociativeArrayGetLast(&self->addresses[bucketIndex], &it)) {
			// Got an address, remove and then return it.
			CBNetworkAddress * addr = it.node->elements[it.index];
			CBAssociativeArrayDelete(&self->addresses[bucketIndex], it);
			self->addrNum--;
			return addr;
		}
		// Move to the next bucket
		bucketIndex++;
		// If we have gone past the allowable bucker range, return to the start
		if (bucketIndex == CB_BUCKET_NUM)
			bucketIndex = 0;
	}

}
bool CBAddressManagerTakeAddress(CBAddressManager * self, CBNetworkAddress * addr){
	// Find the bucket for this address and insert it into the object
	CBAddressManagerSetBucketIndex(self, addr);
	// Add address to array
	if (NOT CBAssociativeArrayInsert(&self->addresses[addr->bucket], addr, CBAssociativeArrayFind(&self->addresses[addr->bucket], addr).position, NULL)){
		CBLogError("Could not insert an address into the addresses array");
		return false;
	}
	// Increase the number of addresses.
	self->addrNum++;
	return true;
}
bool CBAddressManagerTakePeer(CBAddressManager * self, CBPeer * peer){
	// Add peer to arrays
	CBPosition firstArrPos = CBAssociativeArrayFind(&self->peers, peer).position;
	if (NOT CBAssociativeArrayInsert(&self->peers, peer, firstArrPos, NULL)){
		CBLogError("Could not insert a peer into the peers array");
		return false;
	}
	if (NOT CBAssociativeArrayInsert(&self->peersByTimeOffset, peer, CBAssociativeArrayFind(&self->peersByTimeOffset, peer).position, NULL)){
		CBLogError("Could not insert a peer into the peers array which is ordered by the time offset.");
		// Remove from the other array since it failed for the second.
		CBAssociativeArrayDelete(&self->peers, firstArrPos);
		return false;
	}
	// Increase the number of peers.
	self->peersNum++;
	// Adjust time
	CBAddressManagerAdjustTime(self);
	return true;
}
CBCompare CBPeerCompareByTime(void * peer1, void * peer2){
	CBPeer * peerObj1 = peer1;
	CBPeer * peerObj2 = peer2;
	if (peerObj1->timeOffset < peerObj2->timeOffset)
		return CB_COMPARE_MORE_THAN;
	if (peerObj1->timeOffset > peerObj2->timeOffset)
		return CB_COMPARE_LESS_THAN;
	return CBNetworkAddressIPPortCompare(peer1, peer2);
}
CBCompare CBNetworkAddressCompare(void * address1, void * address2){
	CBNetworkAddress * addrObj1 = address1;
	CBNetworkAddress * addrObj2 = address2;
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
