//
//  CBNetworkAddress.h
//  cbitcoin
//
//  Created by Matthew Mitchell on 02/07/2012.
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
 @brief Contains IP, time, port and services information for a peer as well as data and code for managing individual peers. Used to advertise peers. "bytesTransferred/timeUsed" can be used to rank peers for the most efficient ones which can be useful when selecting prefered peers for download. Inherits CBObject
*/

#ifndef CBNETWORKADDRESSH
#define CBNETWORKADDRESSH

//  Includes

#include "CBMessage.h"
#include "CBDependencies.h"
#include "CBNetworkFunctions.h"
#include <time.h>
#include <stdio.h>

// Constants and Macros

#define CBGetNetworkAddress(x) ((CBNetworkAddress *)x)
#define CB_NETWORK_ADDR_STR_SIZE 48

typedef enum{
	CB_SERVICE_NO_FULL_BLOCKS = 0,
	CB_SERVICE_FULL_BLOCKS = 1, /**< Service for full blocks. Node maintains the entire blockchain. */
}CBVersionServices;

typedef struct{
	CBByteArray * ip; /**< IP address. Should be 16 bytes for the IPv6 compatible format. IPv4 addresses should use the IPv4 mapped IPv6 addresses. */
	uint16_t port; /**< Port number */
} CBSocketAddress;

/**
 @brief Structure for CBNetworkAddress objects. @see CBNetworkAddress.h
*/
typedef struct{
	CBMessage base; /**< CBObject base structure */
	uint64_t lastSeen; /**< The timestamp the address was last seen */
	uint32_t penalty; /**< Addresses are selected partially by their time last seen, but the time last seen is adjusted with a penalty for bad behaviour. This is taken awat from the time last seen. */
	CBVersionServices services; /**< Services bit field */
	CBSocketAddress sockAddr;
	CBIPType type; /**< The type of the IP */
	//int32_t version; /**< Protocol version of peer. */
	bool isPublic; /**< If true the address is public and should be relayed. If true, upon a lost or failed connection, return to addresses list. If false the address is private and should be forgotten when connections are closed and never relayed. Addresses are made public when we receive them in an address broadcast. */
	uint8_t bucket; /**< The bucket number for this address. */
	bool bucketSet; /**< True if the bucket has been previously set */
} CBNetworkAddress;

/**
 @brief Creates a new CBNetworkAddress object.
 @param lastSeen The time this address was last seen.
 @param ip The IP address in a 16 byte IPv6 format. If NULL, the IP will be a 16 byte CBByteArray set will all zero.
 @param port The port for this address.
 @param services The services this address can provide.
 @param isPublic If true the address is public and thus can be relayed and returned to the address manager.
 @returns A new CBNetworkAddress object.
 */
CBNetworkAddress * CBNewNetworkAddress(uint64_t lastSeen, CBSocketAddress addr, CBVersionServices services, bool isPublic);

/**
 @brief Creates a new CBNetworkAddress object from serialised data.
 @param isPublic If true the address is public and thus can be relayed and returned to the address manager.
 @returns A new CBNetworkAddress object.
 */
CBNetworkAddress * CBNewNetworkAddressFromData(CBByteArray * data, bool isPublic);

/**
 @brief Initialises a CBNetworkAddress object
 @param self The CBNetworkAddress object to initialise
 @param lastSeen The time this address was last seen.
 @param ip The IP address in a 16 byte IPv6 format. If NULL, the IP will be a 16 byte CBByteArray set will all zero.
 @param port The port for this address.
 @param services The services this address can provide.
 @param isPublic If true the address is public and thus can be relayed and returned to the address manager.
 @returns true on success, false on failure.
 */
void CBInitNetworkAddress(CBNetworkAddress * self, uint64_t lastSeen, CBSocketAddress addr, CBVersionServices services, bool isPublic);

/**
 @brief Initialises a CBNetworkAddress object from serialised data
 @param self The CBNetworkAddress object to initialise
 @param isPublic If true the address is public and thus can be relayed and returned to the address manager.
 @returns true on success, false on failure.
 */
void CBInitNetworkAddressFromData(CBNetworkAddress * self, CBByteArray * data, bool isPublic);

/**
 @brief Release and free all of the objects stored by the CBNetworkAddress object.
 @param self The CBNetworkAddress object to destroy.
 */
void CBDestroyNetworkAddress(void * self);

/**
 @brief Frees a CBNetworkAddress object and also calls CBDestroyNetworkAddress.
 @param self The CBNetworkAddress object to free.
 */
void CBFreeNetworkAddress(void * self);
 
//  Functions

/**
 @brief Deserialises a CBNetworkAddress so that it can be used as an object.
 @param self The CBNetworkAddress object
 @param timestamp If true a timestamp is expected, else it is not. If a timestamp is not expected then "lastSeen" will be set to two hours ago.
 @returns The length read on success, 0 on failure.
 */
uint32_t CBNetworkAddressDeserialise(CBNetworkAddress * self, bool timestamp);

/**
 @brief Compares two network addresses
 @param self The CBNetworkAddress object
 @param addr The CBNetworkAddress for comparison
 @returns true if the IP and port match and the IP is not NULL. False otherwise.
 */
bool CBNetworkAddressEquals(CBNetworkAddress * self, CBNetworkAddress * addr);

/**
 @brief Serialises a CBNetworkAddress to the byte data.
 @param self The CBNetworkAddress object
 @param timestamp If true the time will be included, else it will not.
 @returns The length written on success, 0 on failure.
 */
uint8_t CBNetworkAddressSerialise(CBNetworkAddress * self, bool timestamp);
char * CBNetworkAddressToString(CBNetworkAddress * self, char output[48]);

#endif
