 //
//  CBNetworkAddressManager.c
//  cbitcoin
//
//  Created by Matthew Mitchell on 31/07/2012.
//  Copyright (c) 2012 Matthew Mitchell
//
//  This file is part of cbitcoin. It is subject to the license terms
//  in the LICENSE file found in the top-level directory of this
//  distribution and at http://www.cbitcoin.com/license.html. No part of
//  cbitcoin, including this file, may be copied, modified, propagated,
//  or distributed except according to the terms contained in the
//  LICENSE file.

//  SEE HEADER FILE FOR DOCUMENTATION

#include "CBNetworkAddressManager.h"

//  Constructors

CBNetworkAddressManager * CBNewNetworkAddressManager(void (*onBadTime)(void *)){
	CBNetworkAddressManager * self = malloc(sizeof(*self));
	CBGetObject(self)->free = CBFreeNetworkAddressManager;
	if(CBInitNetworkAddressManager(self, onBadTime))
		return self;
	free(self);
	return NULL;
}

//  Initialiser

bool CBInitNetworkAddressManager(CBNetworkAddressManager * self, void (*onBadTime)(void *)){
	CBInitMessageByObject(CBGetMessage(self));
	// We start with no peers or addresses.
	self->peersNum = 0;
	self->addrNum = 0;
	self->timeOffsetNum = 0;
	// Assign arguments to structure
	self->onBadTime = onBadTime;
	// Create random number generators.
	if (! CBNewSecureRandomGenerator(&self->rndGen)) {
		CBLogError("Could not create a random number generator.");
		return false;
	}
	// Seed the random number generator.
	if (! CBSecureRandomSeed(self->rndGen)){
		CBFreeSecureRandomGenerator(self->rndGen);
		CBLogError("Could not securely seed the random number generator.");
		return false;
	}
	if (! CBNewSecureRandomGenerator(&self->rndGenForBucketIndices)){
		CBFreeSecureRandomGenerator(self->rndGen);
		CBLogError("Could not create a random number generator for the bucket indices.");
		return false;
	}
	// Generate secret
	self->secret = CBSecureRandomInteger(self->rndGen);
	// Initialise arrays for addresses and peers.
	CBInitAssociativeArray(&self->peers, CBNetworkAddressIPPortCompare, NULL, CBReleaseObject);
	CBInitAssociativeArray(&self->peerTimeOffsets, CBPeerCompareByTime, NULL, NULL);
	for (uint8_t x = 0; x < CB_BUCKET_NUM; x++){
		CBInitAssociativeArray(&self->addresses[x], CBNetworkAddressIPPortCompare, NULL, CBReleaseObject);
		CBInitAssociativeArray(&self->addressScores[x], CBNetworkAddressCompare, NULL, NULL);
	}
	return true;
}

//  Destructor

void CBDestroyNetworkAddressManager(void * vself){
	CBNetworkAddressManager * self = vself;
	// Free the random number generators
	CBFreeSecureRandomGenerator(self->rndGenForBucketIndices);
	CBFreeSecureRandomGenerator(self->rndGen);
	// Free the arrays
	CBFreeAssociativeArray(&self->peers);
	CBFreeAssociativeArray(&self->peerTimeOffsets);
	for (uint8_t x = 0; x < CB_BUCKET_NUM; x++){
		CBFreeAssociativeArray(&self->addresses[x]);
		CBFreeAssociativeArray(&self->addressScores[x]);
	}
	CBDestroyMessage(self);
}
void CBFreeNetworkAddressManager(void * self){
	CBDestroyNetworkAddressManager(self);
	free(self);
}

//  Functions

void CBNetworkAddressManagerAddAddress(CBNetworkAddressManager * self, CBNetworkAddress * addr){
	CBRetainObject(addr);
	CBNetworkAddressManagerTakeAddress(self, addr);
}
void CBNetworkAddressManagerAddPeer(CBNetworkAddressManager * self, CBPeer * peer){
	CBRetainObject(peer);
	CBNetworkAddressManagerTakePeer(self, peer);
}
void CBNetworkAddressManagerAdjustTime(CBNetworkAddressManager * self){
	// Get median timeOffset. Nodes are pre-ordered to the timeOffset.
	if (self->timeOffsetNum) {
		// There are time offsets. Get the index for the median value
		uint32_t index = (self->timeOffsetNum - 1) / 2;
		CBPosition it;
		CBAssociativeArrayGetElement(&self->peerTimeOffsets, &it, index);
		int16_t median = ((CBPeer *)it.node->elements[it.index])->timeOffset;
		if (! (self->timeOffsetNum % 2)){
			// Average middle pair
			CBAssociativeArrayGetElement(&self->peerTimeOffsets, &it, index + 1);
			median = (((CBPeer *)it.node->elements[it.index])->timeOffset + median) / 2;
		}
		if (abs(median) > CB_NETWORK_TIME_ALLOWED_TIME_DRIFT) {
			// Revert to system time
			self->networkTimeOffset = 0;
			// Check to see if any peers are within 5 minutes of the system time and do not have the same time, else give a bad time error.
			bool found = false;
			CBAssociativeArrayForEach(CBPeer * peer, &self->peers)
				if (peer->timeOffset < 300 && peer->timeOffset){
					found = true;
					break;
				}
			if (! found)
				self->onBadTime(self->callbackHandler);
		}else
			self->networkTimeOffset = median;
	}else
		// There are no time offsets so make the time offset 0.
		self->networkTimeOffset = 0;
}
void CBNetworkAddressManagerClearPeers(CBNetworkAddressManager * self){
	CBFreeBTreeNode(self->peers.root, NULL, true);
	CBFreeBTreeNode(self->peerTimeOffsets.root, NULL, true);
	self->peersNum = 0;
	self->timeOffsetNum = 0;
}
uint32_t CBNetworkAddressManagerGetAddresses(CBNetworkAddressManager * self, uint32_t num, CBNetworkAddress ** addresses){
	uint8_t start = CBSecureRandomInteger(self->rndGen) % CB_BUCKET_NUM;
	uint8_t bucketIndex = start;
	if (num > self->addrNum)
		// We can only retrieve no more than the number of addresses we actually have
		num = self->addrNum;
	uint32_t index = 0;
	for (uint8_t x = 0; x < num;) {
		// See if an address is in the bucket at the index.
		CBPosition it;
		if (CBAssociativeArrayGetElement(&self->addressScores[bucketIndex], &it, index)) {
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
uint64_t CBNetworkAddressManagerGetNetworkTime(CBNetworkAddressManager * self){
	return time(NULL) + self->networkTimeOffset;
}
void CBNetworkAddressManagerSetBucketIndex(CBNetworkAddressManager * self, CBNetworkAddress * addr){
	if (! addr->bucketSet) {
		uint64_t group = CBNetworkAddressManagerGetGroup(self, addr);
		// Add the group with the secure secret generated during initialisation.
		CBRandomSeed(self->rndGenForBucketIndices, group + self->secret);
		addr->bucket = CBSecureRandomInteger(self->rndGenForBucketIndices) % CB_BUCKET_NUM;
		addr->bucketSet = true;
	}
}
uint64_t CBNetworkAddressManagerGetGroup(CBNetworkAddressManager * self, CBNetworkAddress * addr){
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
CBPeer * CBNetworkAddressManagerGetPeer(CBNetworkAddressManager * self, uint32_t x){
	CBPosition it;
	if (! CBAssociativeArrayGetElement(&self->peers, &it, x)){
		CBLogError("Attempted to get a peer with an out of range index");
		return NULL;
	}
	// Retain peer
	CBRetainObject(it.node->elements[it.index]);
	return it.node->elements[it.index];
}
CBNetworkAddress * CBNetworkAddressManagerGotNetworkAddress(CBNetworkAddressManager * self, CBNetworkAddress * addr){
	CBNetworkAddressManagerSetBucketIndex(self, addr);
	CBFindResult res = CBAssociativeArrayFind(&self->addresses[addr->bucket], addr);
	if (res.found){
		// Retain network address
		CBRetainObject(CBFindResultToPointer(res));
		return CBFindResultToPointer(res);
	}
	return NULL;
}
CBPeer * CBNetworkAddressManagerGotPeer(CBNetworkAddressManager * self, CBNetworkAddress * addr){
	CBFindResult res = CBAssociativeArrayFind(&self->peers, addr);
	if (res.found){
		// Retain peer
		CBPeer * peer = CBFindResultToPointer(res);
		CBRetainObject(peer);
		return peer;
	}
	return NULL;
}
void CBNetworkAddressManagerRemoveAddress(CBNetworkAddressManager * self, CBNetworkAddress * addr){
	CBNetworkAddressManagerSetBucketIndex(self, addr);
	CBFindResult res = CBAssociativeArrayFind(&self->addresses[addr->bucket], addr);
	if (res.found){
		CBAssociativeArrayDelete(&self->addresses[addr->bucket], res.position, true);
		// Delete from the score ordered array
		CBAssociativeArrayDelete(&self->addressScores[addr->bucket], CBAssociativeArrayFind(&self->addressScores[addr->bucket], addr).position, false);
		// Decrement the number of addresses.
		self->addrNum--;
	}
}
void CBNetworkAddressManagerRemovePeer(CBNetworkAddressManager * self, CBPeer * peer){
	CBFindResult peersRes = CBAssociativeArrayFind(&self->peers, peer);
	if (peersRes.found){
		// Find if in time offsets array and where.
		CBNetworkAddressManagerRemovePeerTimeOffset(self, peer);
		// Remove from peers array
		CBAssociativeArrayDelete(&self->peers, peersRes.position, true);
		// Decrement number of peers
		self->peersNum--;
	}
}
void CBNetworkAddressManagerRemovePeerTimeOffset(CBNetworkAddressManager * self, CBPeer * peer){
	CBFindResult timeOffsetRes = CBAssociativeArrayFind(&self->peerTimeOffsets, peer);
	if (timeOffsetRes.found){
		// The peer is found in this array, so remove it and re-adjust the time.
		CBAssociativeArrayDelete(&self->peerTimeOffsets, timeOffsetRes.position, false);
		self->timeOffsetNum--;
		// Adjust time
		CBNetworkAddressManagerAdjustTime(self);
	}
}
CBNetworkAddress * CBNetworkAddressManagerSelectAndRemoveAddress(CBNetworkAddressManager * self){
	uint8_t bucketIndex = CBSecureRandomInteger(self->rndGen) % CB_BUCKET_NUM;
	for (;;) {
		// See if an address is in the bucket
		CBPosition it;
		if (CBAssociativeArrayGetLast(&self->addressScores[bucketIndex], &it)) {
			// Got an address, remove and then return it.
			// Remove from both arrays.
			// Do not release the data, as we want to return it, so pass false to "doFree"
			CBAssociativeArrayDelete(&self->addresses[bucketIndex], CBAssociativeArrayFind(&self->addresses[bucketIndex], it.node->elements[it.index]).position, false);
			CBAssociativeArrayDelete(&self->addressScores[bucketIndex], it, false);
			self->addrNum--;
			return it.node->elements[it.index];
		}
		// Move to the next bucket
		bucketIndex++;
		// If we have gone past the allowable bucker range, return to the start
		if (bucketIndex == CB_BUCKET_NUM)
			bucketIndex = 0;
	}

}
void CBNetworkAddressManagerTakeAddress(CBNetworkAddressManager * self, CBNetworkAddress * addr){
	// Find the bucket for this address and insert it into the object
	CBNetworkAddressManagerSetBucketIndex(self, addr);
	// Add address to array
	CBPosition pos = CBAssociativeArrayFind(&self->addresses[addr->bucket], addr).position;
	CBAssociativeArrayInsert(&self->addresses[addr->bucket], addr, pos, NULL);
	// Add address to array sorted by the score
	CBAssociativeArrayInsert(&self->addressScores[addr->bucket], addr, CBAssociativeArrayFind(&self->addressScores[addr->bucket], addr).position, NULL);
	// Increase the number of addresses.
	self->addrNum++;
}
void CBNetworkAddressManagerTakePeer(CBNetworkAddressManager * self, CBPeer * peer){
	// Add peer to array
	CBAssociativeArrayInsert(&self->peers, peer, CBAssociativeArrayFind(&self->peers, peer).position, NULL);
	// Increase the number of peers.
	self->peersNum++;
}
void CBNetworkAddressManagerTakePeerTimeOffset(CBNetworkAddressManager * self, CBPeer * peer){
	// Add peer to array
	CBAssociativeArrayInsert(&self->peerTimeOffsets, peer, CBAssociativeArrayFind(&self->peerTimeOffsets, peer).position, NULL);
	self->timeOffsetNum++;
	// Adjust time
	CBNetworkAddressManagerAdjustTime(self);
}
CBCompare CBPeerCompareByTime(CBAssociativeArray * array, void * peer1, void * peer2){
	CBPeer * peerObj1 = peer1;
	CBPeer * peerObj2 = peer2;
	if (peerObj1->timeOffset < peerObj2->timeOffset)
		return CB_COMPARE_MORE_THAN;
	if (peerObj1->timeOffset > peerObj2->timeOffset)
		return CB_COMPARE_LESS_THAN;
	return CBNetworkAddressIPPortCompare(array, peer1, peer2);
}
CBCompare CBNetworkAddressCompare(CBAssociativeArray * array, void * address1, void * address2){
	CBNetworkAddress * addrObj1 = address1;
	CBNetworkAddress * addrObj2 = address2;
	uint64_t addrScore1 = addrObj1->lastSeen - addrObj1->penalty;
	uint64_t addrScore2 = addrObj2->lastSeen - addrObj2->penalty;
	if (addrScore1 < addrScore2)
		return CB_COMPARE_MORE_THAN;
	if (addrScore1 > addrScore2)
		return CB_COMPARE_LESS_THAN;
	return CBNetworkAddressIPPortCompare(array, address1, address2);
}
CBCompare CBNetworkAddressIPPortCompare(CBAssociativeArray * array, void * address1, void * address2){
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
