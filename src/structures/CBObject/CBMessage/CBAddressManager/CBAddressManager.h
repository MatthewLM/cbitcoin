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

#include "CBNode.h"
#include "CBAddressBroadcast.h"
#include "CBNetworkFunctions.h"
#include "CBDependencies.h"
#include <time.h>

/**
 @brief Structure for storing a pointer to a CBNetworkAddress and describing it's location in the CBAddressManager.
 */
typedef struct{
	CBNetworkAddress * addr;
	uint8_t bucketIndex;
	uint16_t addrIndex;
} CBNetworkAddressLocator;

/**
 @brief Structure for placing IP addresses such that is is difficult to obtain IPs that fall into many buckets.
 */
typedef struct{
	CBNetworkAddress ** addresses;
	uint16_t addrNum; /**< The number of addresses in this bucket. */
} CBBucket;

/**
 @brief Structure for CBAddressManager objects. @see CBAddressManager.h
*/
typedef struct{
	CBMessage base; /**< CBMessage base structure */
	CBBucket buckets[CB_BUCKET_NUM]; /**< Unconnected nodes stored in the buckets */
	CBNode ** nodes; /**< Connected nodes sorted by the time offset. */
	uint32_t nodesNum; /**< Number of connected nodes */
	int16_t networkTimeOffset; /**< Offset to get from system time to network time. */
	CBIPType reachablity; /**< Bitfield for reachable address types */
	uint16_t maxAddressesInBucket; /**< Maximum number of addresses that can be stored in a single bucket. */
	uint64_t secret; /**< Securely generated pseudo-random number to generate a secret used to mix-up groups into different buckets. */
	uint64_t rndGen; /**< Random number generator instance. */
	uint64_t rndGenForBucketIndexes; /**< Random number generator used for generating bucket indexes. */
	void * callbackHandler; /**< Sent to onBadTime callback */
} CBAddressManager;

/**
 @brief Creates a new CBAddressManager object.
 @returns A new CBAddressManager object.
*/
CBAddressManager * CBNewAddressManager(CBEvents * events);
/**
@brief Creates a new CBAddressManager object from serialised data.
 @param data Serialised CBAddressManager data.
 @returns A new CBAddressManager object.
*/
CBAddressManager * CBNewAddressManagerFromData(CBByteArray * data,CBEvents * events);

/**
 @brief Gets a CBAddressManager from another object. Use this to avoid casts.
 @param self The object to obtain the CBAddressManager from.
 @returns The CBAddressManager object.
*/
CBAddressManager * CBGetAddressManager(void * self);

/**
 @brief Initialises a CBAddressManager object
 @param self The CBAddressManager object to initialise
 @returns true on success, false on failure.
*/
bool CBInitAddressManager(CBAddressManager * self,CBEvents * events);
/**
 @brief Initialises a CBAddressManager object from serialised data
 @param self The CBAddressManager object to initialise
 @param data The serialised data.
 @returns true on success, false on failure.
*/
bool CBInitAddressManagerFromData(CBAddressManager * self,CBByteArray * data,CBEvents * events);

/**
 @brief Frees a CBAddressManager object.
 @param self The CBAddressManager object to free.
 */
void CBFreeAddressManager(void * vself);
 
//  Functions

/**
 @brief Adds a CBNode an places it into the CBAddressManager with a retain.
 @param self The CBAddressManager object.
 @param node The CBNetworkAddress to take.
 */
void CBAddressManagerAddAddress(CBAddressManager * self,CBNetworkAddress * addr);
/**
 @brief Adjust the network time offset with a node's time.
 @param self The CBAddressManager object.
 @param time Time to adjust network time with.
 */
void CBAddressManagerAdjustTime(CBAddressManager * self);
/**
 @brief Deserialises a CBAddressManager so that it can be used as an object.
 @param self The CBAddressManager object
 @returns The length read on success, 0 on failure.
*/
uint32_t CBAddressManagerDeserialise(CBAddressManager * self);
/**
 @brief Gets a number of addresses.
 @param self The CBAddressManager object.
 @param num The number of addresses to get.
 @returns The addresses in a newly allocated memory block. This block is NULL terminated. The number of addresses returned may be less than "num". CBNetworkAddressLocator is used to locate addresses if needed.
 */
CBNetworkAddressLocator * CBAddressManagerGetAddresses(CBAddressManager * self,u_int32_t num);
/**
 @brief Gets the bucket index for an address.
 @param self The CBAddressManager object.
 @param addr The address.
 @returns The bucket index.
 */
u_int8_t CBAddressManagerGetBucketIndex(CBAddressManager * self,CBNetworkAddress * addr);
/**
 @brief Gets the group number for an address. It is seperated such as to make it difficult to use an IP in many different groups. This is done such as the original bitcoin client.
 @param self The CBAddressManager object.
 @param addr The address.
 @returns The address group number.
 */
uint64_t CBAddressManagerGetGroup(CBAddressManager * self,CBNetworkAddress * addr);
/**
 @brief Gets the total amount of addresses, including connected ones. Not thread safe.
 @param self The CBAddressManager object.
 @returns The total number of addresses.
 */
uint64_t CBAddressManagerGetNumberOfAddresses(CBAddressManager * self);
/**
 @brief Determines if a CBNetworkAddress already exists in the CBAddressManager. Compares the IP address and port.
 @param self The CBAddressManager object.
 @param addr The address.
 @returns If the address already exists, returns the existing object. Else returns NULL.
 */
CBNetworkAddress * CBAddressManagerGotNetworkAddress(CBAddressManager * self,CBNetworkAddress * addr);
/**
 @brief Determines if a CBNetworkAddress is in the "nodes" list. Compares the IP address and port.
 @param self The CBAddressManager object.
 @param addr The address.
 @returns If the address already exists as a connected node, returns the existing object. Else returns NULL.
 */
CBNode * CBAddressManagerGotNode(CBAddressManager * self,CBNetworkAddress * addr);
/**
 @brief Determines if an IP type is reachable.
 @param self The CBAddressManager object.
 @param type The type.
 @returns true if reachable, false if not reachable.
 */
bool CBAddressManagerIsReachable(CBAddressManager * self,CBIPType type);
/**
 @brief Remove a CBNode from the nodes list.
 @param self The CBAddressManager object.
 @param node The CBNode to remove
 */
void CBAddressManagerRemoveNode(CBAddressManager * self,CBNode * node);
/**
 @brief Serialises a CBAddressManager to the byte data.
 @param self The CBAddressManager object
 @returns The length written on success, 0 on failure.
*/
uint32_t CBAddressManagerSerialise(CBAddressManager * self);
/**
 @brief Sets the reachability of an IP type.
 @param self The CBAddressManager object
 @param type The mask to set the types.
 @param reachable true for setting as reachable, false if not.
 */
void CBAddressManagerSetReachability(CBAddressManager * self, CBIPType type, bool reachable);
/**
 @brief Setups the CBAddressMenager for both CBInitAddressManager and CBInitAddressManagerFromData.
 @param self The CBAddressManager object
 @returns true if OK, false if not.
 */
bool CBAddressManagerSetup(CBAddressManager * self);
/**
 @brief Takes a CBNode an places it into the CBAddressManager
 @param self The CBAddressManager object.
 @param node The CBNetworkAddress to take.
 */
void CBAddressManagerTakeAddress(CBAddressManager * self,CBNetworkAddress * addr);
/**
 @brief Takes a CBNode an places it into the nodes list.
 @param self The CBAddressManager object.
 @param node The CBNode to take.
 */
void CBAddressManagerTakeNode(CBAddressManager * self,CBNode * node);

#endif
