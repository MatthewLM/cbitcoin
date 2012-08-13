//
//  CBPingPong.h
//  cbitcoin
//
//  Created by Matthew Mitchell on 19/07/2012.
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
 @brief This message contains the ID used in ping and pong messages for protocol versions over 60000. A CBNetworkCommunicator will accept this message for compatible protocol numbers and use an empty message for pings otherwise. Inherits CBMessage
*/

#ifndef CBPINGPONGH
#define CBPINGPONGH

//  Includes

#include "CBMessage.h"

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
CBPingPong * CBNewPingPong(uint64_t ID,CBEvents * events);
/**
@brief Creates a new CBPingPong object from serialised data.
 @param data Serialised CBPingPong data.
 @returns A new CBPingPong object.
*/
CBPingPong * CBNewPingPongFromData(CBByteArray * data,CBEvents * events);

/**
 @brief Gets a CBPingPong from another object. Use this to avoid casts.
 @param self The object to obtain the CBPingPong from.
 @returns The CBPingPong object.
*/
CBPingPong * CBGetPingPong(void * self);

/**
 @brief Initialises a CBPingPong object
 @param self The CBPingPong object to initialise
 @param ID The identifier used in a ping/pong communcation. Use zero for no identification.
 @returns true on success, false on failure.
*/
bool CBInitPingPong(CBPingPong * self,uint64_t ID,CBEvents * events);
/**
 @brief Initialises a CBPingPong object from serialised data
 @param self The CBPingPong object to initialise
 @param data The serialised data.
 @returns true on success, false on failure.
*/
bool CBInitPingPongFromData(CBPingPong * self,CBByteArray * data,CBEvents * events);

/**
 @brief Frees a CBPingPong object.
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
