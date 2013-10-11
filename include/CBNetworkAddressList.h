//
//  CBNetworkAddress.h
//  cbitcoin
//
//  Created by Matthew Mitchell on 03/07/2012.
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
 @brief Message for broadcasting CBNetworkAddresses over the network. This is used to share socket information for peers that are accepting incomming connections. When making an object, get the addresses you want to add and then set the "addrNum" and "addresses" fields. When managing the addresses you want to share, it's best to keep one object and change the addresses when necessary by reorganising the pointers with the "addresses" field. Inherits CBMessage
*/

#ifndef CBNetworkAddressH
#define CBNetworkAddressH

//  Includes

#include "CBNetworkAddress.h"

// Constants and Macros

#define CB_NETWORK_ADDRESS_LIST_MAX_SIZE 1000
#define CBGetNetworkAddressList(x) ((CBNetworkAddressList *)x)

/**
 @brief Structure for CBNetworkAddressList objects. @see CBNetworkAddressList.h
*/
typedef struct{
	CBMessage base; /**< CBMessage base structure */
	bool timeStamps; /**< If true, timestamps are included with the CBNetworkAddresses */
	uint8_t addrNum; /**< Number of addresses. Maximum is 30. */
	CBNetworkAddress ** addresses; /**< List of CBNetworkAddresses shared so that peers can find new peers. */
} CBNetworkAddressList;

/**
 @brief Creates a new CBNetworkAddressList object.
 @returns A new CBNetworkAddressList object.
 */
CBNetworkAddressList * CBNewNetworkAddressList(bool timeStamps);
/**
 @brief Creates a new CBNetworkAddressList object from serialised data.
 @param data Serialised CBNetworkAddressList data.
 @returns A new CBNetworkAddressList object.
*/
CBNetworkAddressList * CBNewNetworkAddressListFromData(CBByteArray * data, bool timeStamps);

/**
 @brief Initialises a CBNetworkAddressList object
 @param self The CBNetworkAddressList object to initialise
 @returns true on success, false on failure.
*/
void CBInitNetworkAddressList(CBNetworkAddressList * self, bool timeStamps);
/**
 @brief Initialises a CBNetworkAddressList object from serialised data
 @param self The CBNetworkAddressList object to initialise
 @param data The serialised data.
 @returns true on success, false on failure.
*/
void CBInitNetworkAddressListFromData(CBNetworkAddressList * self, bool timeStamps, CBByteArray * data);

/**
 @brief Release and free all of the objects stored by the CBNetworkAddressList object.
 @param self The CBNetworkAddress object to destroy.
 */
void CBDestroyNetworkAddressList(void * self);
/**
 @brief Frees a CBNetworkAddressList object and also calls CBDestroyNetworkAddressList.
 @param self The CBNetworkAddressList object to free.
 */
void CBFreeNetworkAddressList(void * self);
 
//  Functions

/**
 @brief Adds a CBNetworkAddress to the list.
 @param self The CBNetworkAddressList object
 @param address The CBNetworkAddress to add.
 */
void CBNetworkAddressListAddNetworkAddress(CBNetworkAddressList * self, CBNetworkAddress * address);
/**
 @brief Calculates the length needed to serialise the object.
 @param self The CBNetworkAddressList object.
 @returns The length read on success, 0 on failure.
 */
uint32_t CBNetworkAddressListCalculateLength(CBNetworkAddressList * self);
/**
 @brief Deserialises a CBNetworkAddressList so that it can be used as an object.
 @param self The CBNetworkAddressList object
 @returns The length read on success, 0 on failure.
*/
uint32_t CBNetworkAddressListDeserialise(CBNetworkAddressList * self);
/**
 @brief Serialises a CBNetworkAddressList to the byte data.
 @param self The CBNetworkAddressList object
 @param force Serialises everything, replacing any previous serialisation of children objects.
 @returns The length written on success, 0 on failure.
*/
uint32_t CBNetworkAddressListSerialise(CBNetworkAddressList * self, bool force);
uint16_t CBNetworkAddressListStringMaxSize(CBNetworkAddressList * self);
void CBNetworkAddressListToString(CBNetworkAddressList * self, char * output);
/**
 @brief Takes a CBNetworkAddress to the list for broadcasting. This does not retain the CBNetworkAddress so you can pass an CBNetworkAddress into this while releasing control in the calling function.
 @param self The CBNetworkAddressList object
 @param address The CBNetworkAddress to take.
 */
void CBNetworkAddressListTakeNetworkAddress(CBNetworkAddressList * self, CBNetworkAddress * address);

#endif
