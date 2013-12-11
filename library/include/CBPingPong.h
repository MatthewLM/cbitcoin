//
//  CBPingPong.h
//  cbitcoin
//
//  Created by Matthew Mitchell on 19/07/2012.
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
 @brief This message contains the ID used in ping and pong messages for protocol versions over 60000. A CBNetworkCommunicator will accept this message for compatible protocol numbers and use an empty message for pings otherwise. Inherits CBMessage
*/

#ifndef CBPINGPONGH
#define CBPINGPONGH

//  Includes

#include "CBMessage.h"

// Getter

#define CBGetPingPong(x) ((CBPingPong *)x)

/**
 @brief Structure for CBPingPong objects. @see CBPingPong.h
*/
typedef struct{
	CBMessage base; /**< CBMessage base structure */
	uint64_t ID; /**< Used to identify a ping/pong. Set ping to zero if no identiication is needed. Set pongs to the same as the request ping. */
} CBPingPong;

/**
 @brief Creates a new CBPingPong object.
 @param ID The identifier used in a ping/pong communcation. Use zero for no identification.
 @returns A new CBPingPong object.
*/
CBPingPong * CBNewPingPong(uint64_t ID);
/**
@brief Creates a new CBPingPong object from serialised data.
 @param data Serialised CBPingPong data.
 @returns A new CBPingPong object.
*/
CBPingPong * CBNewPingPongFromData(CBByteArray * data);

/**
 @brief Initialises a CBPingPong object
 @param self The CBPingPong object to initialise
 @param ID The identifier used in a ping/pong communcation. Use zero for no identification.
*/
void CBInitPingPong(CBPingPong * self, uint64_t ID);
/**
 @brief Initialises a CBPingPong object from serialised data
 @param self The CBPingPong object to initialise
 @param data The serialised data.
*/
void CBInitPingPongFromData(CBPingPong * self, CBByteArray * data);

/**
 @brief Release and free all of the objects stored by the CBPingPong object.
 @param self The CBPingPong object to destroy.
 */
void CBDestroyPingPong(void * self);
/**
 @brief Frees a CBPingPong object and also calls CBDestroyPingPong.
 @param self The CBPingPong object to free.
 */
void CBFreePingPong(void * self);
 
//  Functions

/**
 @brief Deserialises a CBPingPong so that it can be used as an object.
 @param self The CBPingPong object
 @returns Length read if successful, zero otherwise.
*/
uint8_t CBPingPongDeserialise(CBPingPong * self);
/**
 @brief Serialises a CBPingPong to the byte data.
 @param self The CBPingPong object
 @returns Length written if successful, zero otherwise.
*/
uint8_t CBPingPongSerialise(CBPingPong * self);

#endif
