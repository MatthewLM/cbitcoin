//
//  CBEvents.h
//  cbitcoin
//
//  Created by Matthew Mitchell on 10/05/2012.
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
 @brief File for the structure that holds event callback function pointers.
 */

#ifndef CBEVENTSH
#define CBEVENTSH

#include "CBConstants.h"

/**
 @brief Structure holding event callback function pointers.
 */
typedef struct{
	void (*onErrorReceived)(CBError error,char *,...);
	void (*onTimeOut)(void *,void *); /**< Timeout event callback with a CBNetworkCommunicator and CBNetworkAddress. The callback should return as quickly as possible. Use threads for operations that would otherwise delay the event loop for too long. The first argument is the CBNetworkCommunicator responsible for the timeout. The second argument is the node with the timeout. */
	bool (*onMessageReceived)(void *,void *); /**< The callback for when a message has been received from a node. The first argument is the CBNetworkCommunicator responsible for receiving the message. The second argument is the CBNetworkAddress node the message was received from. Return true if the node should be disconnected and false if the node is OK. Access the message by the "receive" feild in the CBNetworkAddress node. Lookup the type of the message and then cast and/or handle the message approriately. The alternative message bytes can be found in the node's "alternativeTypeBytes" field. Do not delay the thread for very long. */
	void (*onBadTime)(void); /**< Called when cbitcoin detects a divergence between the network time and system time whcih suggests the system time may be wrong, in the same way bitcoin-qt detects it. */
	void (*onNetworkError)(void); /**< Called when both IPv4 and IPv6 fails. */
}CBEvents;

#endif
