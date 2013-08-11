//
//  CBNetworkAddressManager.h
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

/**
 @file
 @brief Stores addresses both unconnected and connected. It also proves serialisation and deserialisation so that addresses can be stored in binary format. Credit largely goes to the addrman.cpp code in the original client, although the code here has differences. Inherits CBMessage. The binary format contains a 32 bit integer for the cbitcoin version, followed by CB_BUCKET_NUM buckets. Each of these buckets is written as a 16 bit integer for the number of addresses and then the serialised CBNetworkAddresses one after the other. After the buckets is the 64 bit integer secret.
*/

#ifndef CBADDRESSMANAGERH
#define CBADDRESSMANAGERH

//  Includes

#include "CBPeer.h"
#include "CBNetworkFunctions.h"
#include "CBAssociativeArray.h"
#include <time.h>

// Constants and Macros

#define CB_BUCKET_NUM 255 // Maximum number of buckets
#define CB_NETWORK_TIME_ALLOWED_TIME_DRIFT 4200 // 70 minutes from system time
#define CBGetNetworkAddressManager(x) ((CBNetworkAddressManager *)x)

/**
 @brief Structure for CBNetworkAddressManager objects. @see CBNetworkAddressManager.h
*/
typedef struct{
	CBMessage base; /**< CBMessage base structure */
	CBAssociativeArray addresses[CB_BUCKET_NUM]; /**< Unconnected addresses, seperated into buckets. */
	CBAssociativeArray addressScores[CB_BUCKET_NUM]; /**< Addresses, seperated into buckets and ordered by CBNetworkAddressCompare. */
	uint32_t addrNum; /**< Number of addresses. */
	CBAssociativeArray peers; /**< The peers sorted by CBNetworkAddressIPPortCompare */
	uint32_t peersNum; /**< Number of connected peers. */
	CBAssociativeArray peerTimeOffsets; /**< The time offsets of the peers */
	uint32_t timeOffsetNum; /**< The number of time offsets recorded. */
	int16_t networkTimeOffset; /**< Offset to get from system time to network time. */
	uint16_t maxAddressesInBucket; /**< Maximum number of addresses that can be stored in a single bucket. */
	uint64_t secret; /**< Securely generated pseudo-random number to generate a secret used to mix-up groups into different buckets. */
	CBDepObject rndGen; /**< Random number generator instance. */
	CBDepObject rndGenForBucketIndices; /**< Random number generator used for generating bucket indices. */
	void * callbackHandler; /**< Sent to onBadTime callback */
	void (*onBadTime)(void *); /**< Called when cbitcoin detects a divergence between the network time and system time which suggests the system time may be wrong, in the same way bitcoin-qt detects it. Has an argument for the callback handler. */
} CBNetworkAddressManager;

/**
 @brief Creates a new CBNetworkAddressManager object. After doing this you can load addresses from storage using CBAddressStorageLoadAddresses.
 @param onBadTime The callback for when cbitcoin detects a divergence between the network time and system time which suggests the system time may be wrong, in the same way bitcoin-qt detects it. Has an argument for the callback handler.
 @param storage The address storage object made with CBNewAddressStorage.
 @returns A new CBNetworkAddressManager object.
*/
CBNetworkAddressManager * CBNewNetworkAddressManager(void (*onBadTime)(void *));

/**
 @brief Initialises a CBNetworkAddressManager object
 @param self The CBNetworkAddressManager object to initialise
 @param onBadTime The callback for when cbitcoin detects a divergence between the network time and system time which suggests the system time may be wrong, in the same way bitcoin-qt detects it. Has an argument for the callback handler.
 @returns true on success, false on failure.
*/
bool CBInitNetworkAddressManager(CBNetworkAddressManager * self, void (*onBadTime)(void *));

/**
 @brief Releases and frees all of the objects stored by the CBNetworkAddressManager object.
 @param self The CBNetworkAddressManager object to destroy.
 */
void CBDestroyNetworkAddressManager(void * vself);
/**
 @brief Frees a CBNetworkAddressManager object and also calls CBDestroyNetworkAddressManager.
 @param self The CBNetworkAddressManager object to free.
 */
void CBFreeNetworkAddressManager(void * vself);

//  Functions

/**
 @brief Adds a CBNetworkAddress an places it into the CBNetworkAddressManager with a retain, but not permenent storage.
 @param self The CBNetworkAddressManager object.
 @param addr The CBNetworkAddress to take.
 @returns true if the address was taken successfully, false if an error occured.
 */
void CBNetworkAddressManagerAddAddress(CBNetworkAddressManager * self, CBNetworkAddress * addr);
/**
 @brief Adds a CBPeer an places it into the CBNetworkAddressManager with a retain.
 @param self The CBNetworkAddressManager object.
 @param peer The CBPeer to take.
 @returns true if the peer was taken successfully, false if an error occured.
 */
void CBNetworkAddressManagerAddPeer(CBNetworkAddressManager * self, CBPeer * peer);
/**
 @brief Adjust the network time offset with a peer's time.
 @param self The CBNetworkAddressManager object.
 @param time Time to adjust network time with.
 */
void CBNetworkAddressManagerAdjustTime(CBNetworkAddressManager * self);
/**
 @brief Removes all the peers from the peers list but does not release them.
 @param self The CBNetworkAddressManager object.
 */
void CBNetworkAddressManagerClearPeers(CBNetworkAddressManager * self);
/**
 @brief Gets a number of addresses.
 @param self The CBNetworkAddressManager object.
 @param num The maximum number of addresses to get.
 @param addresses A pointer to CBNetworkAddress objects to be set.
 @returns Actual number of addresses which have been obtained.
 */
uint32_t CBNetworkAddressManagerGetAddresses(CBNetworkAddressManager * self, uint32_t num, CBNetworkAddress ** addresses);
/**
 @brief Gets the network time.
 @param self The CBNetworkAddressManager object.
 @returns The network time.
 */
uint64_t CBNetworkAddressManagerGetNetworkTime(CBNetworkAddressManager * self);
/**
 @brief Sets the bucket index for an address, if it has not already (ie. "bucketSet" is false).
 @param self The CBNetworkAddressManager object.
 @param addr The CBNetworkAddress.
 */
void CBNetworkAddressManagerSetBucketIndex(CBNetworkAddressManager * self, CBNetworkAddress * addr);
/**
 @brief Gets the group number for an address. It is seperated such as to make it difficult to use an IP in many different groups. This is done such as the original bitcoin client.
 @param self The CBNetworkAddressManager object.
 @param addr The address.
 @returns The address group number.
 */
uint64_t CBNetworkAddressManagerGetGroup(CBNetworkAddressManager * self, CBNetworkAddress * addr);
/**
 @brief Returns the peer at the index "x".
 @param self The CBNetworkAddressManager object.
 @param x The index of the peer.
 @returns A retained peer at this index, or NULL if the index is out of range.
 */
CBPeer * CBNetworkAddressManagerGetPeer(CBNetworkAddressManager * self, uint32_t x);
/**
 @brief Determines if a CBNetworkAddress already exists in the CBNetworkAddressManager. Compares the IP address and port.
 @param self The CBNetworkAddressManager object.
 @param addr The address.
 @returns If the address already exists, returns the existing object. Else returns NULL.
 */
CBNetworkAddress * CBNetworkAddressManagerGotNetworkAddress(CBNetworkAddressManager * self, CBNetworkAddress * addr);
/**
 @brief Determines if a CBNetworkAddress is in the "peers" array. Compares the IP address and port.
 @param self The CBNetworkAddressManager object.
 @param addr The address.
 @returns If the address already exists as a connected peer, returns the existing object. Else returns NULL.
 */
CBPeer * CBNetworkAddressManagerGotPeer(CBNetworkAddressManager * self, CBNetworkAddress * peer);
/**
 @brief Removes a CBNetworkAddress if the CBNetworkAddressManager has it. This does not remove it from the permenent storage.
 @param self The CBNetworkAddressManager object.
 @param addr The CBNetworkAddress to remove
 */
void CBNetworkAddressManagerRemoveAddress(CBNetworkAddressManager * self, CBNetworkAddress * addr);
/**
 @brief Remove a CBPeer from the peers list and also the peer's time offset, if it exists int he time offsets array.
 @param self The CBNetworkAddressManager object.
 @param peer The CBPeer to remove
 */
void CBNetworkAddressManagerRemovePeer(CBNetworkAddressManager * self, CBPeer * peer);
/**
 @brief Remove a CBPeer from the peer time offset list.
 @param self The CBNetworkAddressManager object.
 @param peer The CBPeer to remove
 */
void CBNetworkAddressManagerRemovePeerTimeOffset(CBNetworkAddressManager * self, CBPeer * peer);
/**
 @brief Removes an address from the address manager. It will remove the address from a random bucket which has the lowest score in that bucket.
 @param self The CBNetworkAddressManager object.
 @returns The network address which was removed from the address manager.
 */
CBNetworkAddress * CBNetworkAddressManagerSelectAndRemoveAddress(CBNetworkAddressManager * self);
/**
 @brief Takes a CBPeer an places it into the CBNetworkAddressManager, but not permenent storage.
 @param self The CBNetworkAddressManager object.
 @param peer The CBNetworkAddress to take.
 */
void CBNetworkAddressManagerTakeAddress(CBNetworkAddressManager * self, CBNetworkAddress * addr);
/**
 @brief Takes a CBPeer an places it into the peers list.
 @param self The CBNetworkAddressManager object.
 @param peer The CBPeer to take.
 */
void CBNetworkAddressManagerTakePeer(CBNetworkAddressManager * self, CBPeer * peer);
/**
 @brief Records a peers time offset for dtemining the median time.
 @param self The CBNetworkAddressManager object.
 @param peer The peer with the time offset.
 */
void CBNetworkAddressManagerTakePeerTimeOffset(CBNetworkAddressManager * self, CBPeer * peer);
/**
 @brief Compares two peers for ordering by the time offset.
 @param peer1 The first CBPeer.
 @param peer2 The second CBPeer.
 @returns If the first address has a higher time offset then CB_COMPARE_MORE_THAN is returned. If the first address has a lower time offset CB_COMPARE_LESS_THAN is returned. If the time offset are equal then the ip/ports are compared in the same way. If they are equal then CB_COMPARE_EQUAL is returned.
 */
CBCompare CBPeerCompareByTime(CBAssociativeArray * array, void * peer1, void * peer2);
/**
 @brief Compares two network addresses.
 @param address1 The first CBNetworkAddress.
 @param address2 The second CBNetworkAddress.
 @returns If the first address has a higher (-lastSeen + penalty)/IP/port then CB_COMPARE_MORE_THAN is returned. If the (-lastSeen + penalty)/IP/ports are equal thrn CB_COMPARE_EQUAL is returned. If the first address has a lower (-lastSeen + penalty)/IP/port CB_COMPARE_LESS_THAN is returned.
 */
CBCompare CBNetworkAddressCompare(CBAssociativeArray * array, void * address1, void * address2);
/**
 @brief Compares the IP and port of two network addresses, used by CBPeerCompareByTime and CBNetworkAddressCompare.
 @param address1 The first CBNetworkAddress.
 @param address2 The second CBNetworkAddress.
 @returns If the first address has a higher IP/port then CB_COMPARE_MORE_THAN is returned. If the IP/ports are equal thrn CB_COMPARE_EQUAL is returned. If the first address has a lower IP/port CB_COMPARE_LESS_THAN is returned.
 */
CBCompare CBNetworkAddressIPPortCompare(CBAssociativeArray * array, void * address1, void * address2);

#endif
