//
//  CBNetworkAddress.h
//  cbitcoin
//
//  Created by Matthew Mitchell on 02/07/2012.
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
 @brief Contains IP, time, port and services information for a peer as well as data and code for managing individual peers. Used to advertise peers. "bytesTransferred/timeUsed" can be used to rank peers for the most efficient ones which can be useful when selecting prefered peers for download. Inherits CBObject
*/

#ifndef CBNETWORKADDRESSH
#define CBNETWORKADDRESSH

//  Includes

#include "CBMessage.h"
#include "CBDependencies.h"
#include "CBNetworkFunctions.h"

/**
 @brief Structure for CBNetworkAddress objects. @see CBNetworkAddress.h
*/
typedef struct{
	CBMessage base; /**< CBObject base structure */
	uint32_t score; /**< Address score. */
	uint64_t services; /**< Services bit field */
	CBByteArray * ip; /**< IP address. Should be 16 bytes for the IPv6 compatible format. The CBNetworkAddress should have exclusive access to the CBByteArray to avoid potential threading issues as the CBNetworkAddresses are protected by mutexes maintained by CBNetworkCommunicators but the ip is not. */
	CBIPType type; /**< The type of the IP */
	uint16_t port; /**< Port number */
	int32_t version; /**< Protocol version of peer. Set to CB_NODE_VERSION_NOT_SET before it is set once a CBVersion message is received. */
	bool public; /**< If true the address is public and should be relayed. If true, upon a lost or failed connection, return to addresses list. This doesn't include failure from "CBNetworkCommunicatorConnect". If false the address is private and should be forgotten when connections are closed. */
} CBNetworkAddress;

/**
 @brief Creates a new CBNetworkAddress object.
 @param ip The IP address in a 16 byte IPv6 format. If NULL, the IP will be a 16 byte CBByteArray set will all zero.
 @returns A new CBNetworkAddress object.
 */
CBNetworkAddress * CBNewNetworkAddress(uint32_t score,CBByteArray * ip,uint16_t port,uint64_t services,CBEvents * events);
/**
 @brief Creates a new CBNetworkAddress object from serialised data.
 @returns A new CBNetworkAddress object.
 */
CBNetworkAddress * CBNewNetworkAddressFromData(CBByteArray * data,CBEvents * events);

/**
 @brief Gets a CBNetworkAddress from another object. Use this to avoid casts.
 @param self The object to obtain the CBNetworkAddress from.
 @returns The CBNetworkAddress object.
 */
CBNetworkAddress * CBGetNetworkAddress(void * self);

/**
 @brief Initialises a CBNetworkAddress object
 @param self The CBNetworkAddress object to initialise
 @returns true on success, false on failure.
 */
bool CBInitNetworkAddress(CBNetworkAddress * self,uint32_t score,CBByteArray * ip,uint16_t port,uint64_t services,CBEvents * events);
/**
 @brief Initialises a CBNetworkAddress object from serialised data
 @param self The CBNetworkAddress object to initialise
 @returns true on success, false on failure.
 */
bool CBInitNetworkAddressFromData(CBNetworkAddress * self,CBByteArray * data,CBEvents * events);

/**
 @brief Frees a CBNetworkAddress object.
 @param self The CBNetworkAddress object to free.
 */
void CBFreeNetworkAddress(void * self);
 
//  Functions

/**
 @brief Deserialises a CBNetworkAddress so that it can be used as an object.
 @param self The CBNetworkAddress object
 @param time If true a timestamp is expected, else it is not. If a timestamp is not expected then "time" will not be set and will be the previous value.
 @returns The length read on success, 0 on failure.
 */
uint8_t CBNetworkAddressDeserialise(CBNetworkAddress * self,bool score);
/**
 @brief Compares two network addresses
 @param self The CBNetworkAddress object
 @param addr The CBNetworkAddress for comparison
 @returns true if the IP and port match and the IP is not NULL. False otherwise.
 */
bool CBNetworkAddressEquals(CBNetworkAddress * self,CBNetworkAddress * addr);
/**
 @brief Serialises a CBNetworkAddress to the byte data.
 @param self The CBNetworkAddress object
 @param time If true the time will be included, else it will not.
 @returns The length written on success, 0 on failure.
 */
uint8_t CBNetworkAddressSerialise(CBNetworkAddress * self,bool score);

#endif
