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
 @brief Contains IP, time, port and services information for a node as well as data and code for managing individual nodes. Used to advertise nodes. "bytesTransferred/timeUsed" can be used to rank nodes for the most efficient ones which can be useful when selecting prefered nodes for download. Inherits CBObject
*/

#ifndef CBNETWORKADDRESSH
#define CBNETWORKADDRESSH

//  Includes

#include "CBMessage.h"
#include "CBDependencies.h"

/**
 @brief Structure for CBNetworkAddress objects. @see CBNetworkAddress.h
*/
typedef struct{
	CBMessage base; /**< CBObject base structure */
	u_int32_t time; /**< Timestamp */
	u_int64_t services; /**< Services bit field */
	CBByteArray * ip; /**< IP address. Should be 16 bytes for the IPv6 compatible format. The CBNetworkAddress should have exclusive access to the CBByteArray to avoid potential threading issues as the CBNetworkAddresses are protected by mutexes maintained by CBNetworkCommunicators but the ip is not. */
	u_int16_t port; /**< Port number */
	int32_t version; /**< Protocol version of node. Set to CB_NODE_VERSION_NOT_SET before it is set once a CBVersion message is received. */
} CBNetworkAddress;

/**
 @brief Creates a new CBNetworkAddress object.
 @returns A new CBNetworkAddress object.
 */
CBNetworkAddress * CBNewNetworkAddress(u_int32_t time,CBByteArray * ip,u_int16_t port,u_int64_t services,CBEvents * events);
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
bool CBInitNetworkAddress(CBNetworkAddress * self,u_int32_t time,CBByteArray * ip,u_int16_t port,u_int64_t services,CBEvents * events);
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
u_int8_t CBNetworkAddressDeserialise(CBNetworkAddress * self,bool time);
/**
 @brief Serialises a CBNetworkAddress to the byte data.
 @param self The CBNetworkAddress object
 @param time If true the time will be included, else it will not.
 @returns The length written on success, 0 on failure.
 */
u_int8_t CBNetworkAddressSerialise(CBNetworkAddress * self,bool time);

#endif
