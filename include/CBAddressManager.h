//
//  CBAddressManager.h
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

/**
 @file
 @brief Stores addresses both unconnected and connected. It also proves serialisation and deserialisation so that addresses can be stored in binary format. Credit largely goes to the addrman.cpp code in the original client, although the code here has differences. Inherits CBMessage. The binary format contains a 32 bit integer for the cbitcoin version, followed by CB_BUCKET_NUM buckets. Each of these buckets is written as a 16 bit integer for the number of addresses and then the serialised CBNetworkAddresses one after the other. After the buckets is the 64 bit integer secret.
*/

#ifndef CBADDRESSMANAGERH
#define CBADDRESSMANAGERH

//  Includes

#include "CBPeer.h"
#include "CBAddressBroadcast.h"
#include "CBNetworkFunctions.h"
#include "CBAssociativeArray.h"
#include <time.h>

// Constants

#define CB_BUCKET_NUM 255 // Maximum number of buckets
#define CB_NETWORK_TIME_ALLOWED_TIME_DRIFT 4200 // 70 minutes from system time

/**
 @brief Structure for CBAddressManager objects. @see CBAddressManager.h
*/
typedef struct{
	CBMessage base; /**< CBMessage base structure */
	CBAssociativeArray addresses; /**< Unconnected addresses. */
	uint32_t addrNum; /**< Number of addresses. */
	CBAssociativeArray peers; /**< The peers sorted by their time offset */
	uint32_t peersNum; /**< Number of connected peers. */
	int16_t networkTimeOffset; /**< Offset to get from system time to network time. */
	uint16_t maxAddressesInBucket; /**< Maximum number of addresses that can be stored in a single bucket. */
	uint64_t secret; /**< Securely generated pseudo-random number to generate a secret used to mix-up groups into different buckets. */
	uint64_t rndGen; /**< Random number generator instance. */
	uint64_t rndGenForBucketIndices; /**< Random number generator used for generating bucket indices. */
	void * callbackHandler; /**< Sent to onBadTime callback */
	void (*onBadTime)(void *); /**< Called when cbitcoin detects a divergence between the network time and system time which suggests the system time may be wrong, in the same way bitcoin-qt detects it. Has an argument for the callback handler. */
} CBAddressManager;

/**
 @brief Creates a new CBAddressManager object. After doing this you can load addresses from storage using CBAddressStorageLoadAddresses.
 @param onBadTime The callback for when cbitcoin detects a divergence between the network time and system time which suggests the system time may be wrong, in the same way bitcoin-qt detects it. Has an argument for the callback handler.
 @param storage The address storage object made with CBNewAddressStorage.
 @returns A new CBAddressManager object.
*/
CBAddressManager * CBNewAddressManager(void (*onBadTime)(void *));

/**
 @brief Gets a CBAddressManager from another object. Use this to avoid casts.
 @param self The object to obtain the CBAddressManager from.
 @returns The CBAddressManager object.
*/
CBAddressManager * CBGetAddressManager(void * self);

/**
 @brief Initialises a CBAddressManager object
 @param self The CBAddressManager object to initialise
 @param onBadTime The callback for when cbitcoin detects a divergence between the network time and system time which suggests the system time may be wrong, in the same way bitcoin-qt detects it. Has an argument for the callback handler.
 @returns true on success, false on failure.
*/
bool CBInitAddressManager(CBAddressManager * self, void (*onBadTime)(void *));

/**
 @brief Frees a CBAddressManager object.
 @param self The CBAddressManager object to free.
 */
void CBFreeAddressManager(void * vself);

//  Functions

/**
 @brief Adds a CBPeer an places it into the CBAddressManager with a retain, but not permenent storage.
 @param self The CBAddressManager object.
 @param peer The CBNetworkAddress to take.
 @returns true if the address was taken successfully, false if an error occured.
 */
bool CBAddressManagerAddAddress(CBAddressManager * self, CBNetworkAddress * addr);
/**
 @brief Adjust the network time offset with a peer's time.
 @param self The CBAddressManager object.
 @param time Time to adjust network time with.
 */
void CBAddressManagerAdjustTime(CBAddressManager * self);
/**
 @brief Removes all the peers from the peers list.
 @param self The CBAddressManager object.
 */
void CBAddressManagerClearPeers(CBAddressManager * self);
/**
 @brief Gets a number of addresses.
 @param self The CBAddressManager object.
 @param num The maximum number of addresses to get.
 @param addresses A pointer to CBNetworkAddress objects to be set.
 @returns Actual number of addresses which have been obtained.
 */
uint32_t CBAddressManagerGetAddresses(CBAddressManager * self, uint32_t num, CBNetworkAddress ** addresses);
/**
 @brief Sets the bucket index for an address.
 @param self The CBAddressManager object.
 @param addr The CBNetworkAddress.
 */
void CBAddressManagerSetBucketIndex(CBAddressManager * self, CBNetworkAddress * addr);
/**
 @brief Gets the group number for an address. It is seperated such as to make it difficult to use an IP in many different groups. This is done such as the original bitcoin client.
 @param self The CBAddressManager object.
 @param addr The address.
 @returns The address group number.
 */
uint64_t CBAddressManagerGetGroup(CBAddressManager * self, CBNetworkAddress * addr);
/**
 @brief Returns the peer at the index "x".
 @param self The CBAddressManager object.
 @param x The index of the peer.
 @returns A retained peer at this index, or NULL if the index is out of range.
 */
CBPeer * CBAddressManagerGetPeer(CBAddressManager * self, uint32_t x);
/**
 @brief Determines if a CBNetworkAddress already exists in the CBAddressManager. Compares the IP address and port.
 @param self The CBAddressManager object.
 @param addr The address.
 @returns If the address already exists, returns the existing object. Else returns NULL.
 */
CBNetworkAddress * CBAddressManagerGotNetworkAddress(CBAddressManager * self, CBNetworkAddress * addr);
/**
 @brief Determines if a CBNetworkAddress is in the "peers" array. Compares the IP address and port.
 @param self The CBAddressManager object.
 @param addr The address.
 @returns If the address already exists as a connected peer, returns the existing object. Else returns NULL.
 */
CBPeer * CBAddressManagerGotPeer(CBAddressManager * self, CBNetworkAddress * addr);
/**
 @brief Removes a CBNetworkAddress if the CBAddressManager has it. This does not remove it from the permenent storage.
 @param self The CBAddressManager object.
 @param addr The CBNetworkAddress to remove
 */
void CBAddressManagerRemoveAddress(CBAddressManager * self, CBNetworkAddress * addr);
/**
 @brief Remove a CBPeer from the peers list.
 @param self The CBAddressManager object.
 @param peer The CBPeer to remove
 */
void CBAddressManagerRemovePeer(CBAddressManager * self, CBPeer * peer);
/**
 @brief Takes a CBPeer an places it into the CBAddressManager, but not permenent storage.
 @param self The CBAddressManager object.
 @param peer The CBNetworkAddress to take.
 @returns true if the address was taken successfully, false if an error occured.
 */
bool CBAddressManagerTakeAddress(CBAddressManager * self, CBNetworkAddress * addr);
/**
 @brief Takes a CBPeer an places it into the peers list.
 @param self The CBAddressManager object.
 @param peer The CBPeer to take.
 */
bool CBAddressManagerTakePeer(CBAddressManager * self, CBPeer * peer);
/**
 @brief Compares a bucket number to a network address.
 @param bucket The bucket number as an 8 bit integer.
 @param address The CBNetworkAddress.
 @returns If the bucket number is higher than in the CBNetworkAddress then CB_COMPARE_LESS_THAN is returned, if the bucket number is lower then in the CBNetworkAddress then CB_COMPARE_MORE_THAN is returned, else CB_COMPARE_EQUAL is returned.
 */
CBCompare CBBucketCompare(void * bucket, void * address);
/**
 @brief Compares two peers for ordering by the time last seen.
 @param peer1 The first CBPeer.
 @param peer2 The second CBPeer.
 @returns If the first address has a higher time last seen then CB_COMPARE_MORE_THAN is returned. If the first address has a lower time last seen CB_COMPARE_LESS_THAN is returned. If the time last seen are equal then the ip/ports are compared in the same way. If they are equal then CB_COMPARE_EQUAL is returned.
 */
CBCompare CBPeerCompareByTime(void * peer1, void * peer2);
/**
 @brief Compares two network addresses.
 @param address1 The first CBNetworkAddress.
 @param address2 The second CBNetworkAddress.
 @returns If the first address has a higher bucket/(-lastSeen + penalty)/IP/port then CB_COMPARE_MORE_THAN is returned. If the bucket/(-lastSeen + penalty)/IP/ports are equal thrn CB_COMPARE_EQUAL is returned. If the first address has a lower bucket/(-lastSeen + penalty)/IP/port CB_COMPARE_LESS_THAN is returned.
 */
CBCompare CBNetworkAddressCompare(void * address1, void * address2);
/**
 @brief Compares the IP and port of two network addresses, used by CBPeerCompareByTime and CBNetworkAddressCompare.
 @param address1 The first CBNetworkAddress.
 @param address2 The second CBNetworkAddress.
 @returns If the first address has a higher IP/port then CB_COMPARE_MORE_THAN is returned. If the IP/ports are equal thrn CB_COMPARE_EQUAL is returned. If the first address has a lower IP/port CB_COMPARE_LESS_THAN is returned.
 */
CBCompare CBNetworkAddressIPPortCompare(void * address1, void * address2);

#endif
