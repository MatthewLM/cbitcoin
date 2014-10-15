//
//  CBMessage.h
//  cbitcoin
//
//  Created by Matthew Mitchell on 27/04/2012.
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
 @brief Serialisable data structure. Provides the basic framework for serialising and deserialising bitcoin messages. Messages shouldn't be serialised including message headers. Message headers should be added before sending messages and removed when deserialising. Inherits CBObject
 */

#ifndef CBMESSAGEH
#define CBMESSAGEH

//  Includes

#include <stdlib.h>
#include <stdbool.h>
#include "CBByteArray.h"
#include "CBVarInt.h"
#include "CBDependencies.h"

// Constants and Macros

#define CB_MESSAGE_TYPE_STR_SIZE 11
#define CB_DESERIALISE_ERROR UINT32_MAX
#define CBGetMessage(x) ((CBMessage *)x)

/*
 @brief The type of a CBMessage.
 */
typedef enum{
	CB_MESSAGE_TYPE_VERSION, /**< @see CBVersion.h */
	CB_MESSAGE_TYPE_VERACK, /**< Acknowledgement and acceptance of a peer's version and connection. */
	CB_MESSAGE_TYPE_ADDR, /**< @see CBNetworkAddress.h */
	CB_MESSAGE_TYPE_INV, /**< @see CBInventory.h */
	CB_MESSAGE_TYPE_GETDATA, /**< @see CBInventory.h */
	CB_MESSAGE_TYPE_GETBLOCKS, /**< @see CBGetBlocks.h */
	CB_MESSAGE_TYPE_GETHEADERS, /**< @see CBGetBlocks.h */
	CB_MESSAGE_TYPE_TX, /**< @see CBTransaction.h */
	CB_MESSAGE_TYPE_BLOCK, /**< @see CBBlock.h */
	CB_MESSAGE_TYPE_HEADERS, /**< @see CBBlockHeaders.h */
	CB_MESSAGE_TYPE_GETADDR, /**< Request for "active peers". bitcoin-qt consiers active peers to be those that have sent messages in the last 30 minutes. */
	CB_MESSAGE_TYPE_PING, /**< @see CBPingPong.h */
	CB_MESSAGE_TYPE_PONG, /**< @see CBPingPong.h */
	CB_MESSAGE_TYPE_ALERT, /**< @see CBAlert.h */
	CB_MESSAGE_TYPE_ALT, /**< The message was defined by "alternativeMessages" in a CBNetworkCommunicator */
	CB_MESSAGE_TYPE_NUM, /**< Number of messages */
	CB_MESSAGE_TYPE_NONE = UINT8_MAX,
}CBMessageType;

/**
 @brief Structure for CBMessage objects. @see CBMessage.h
 */
typedef struct CBMessage{
	CBObjectMutex base; /**< CBObjectMutex base structure */
	CBMessageType type; /**< The type of the message */
	uint8_t * altText; /**< For an alternative message: This is the type text. */
	CBByteArray * bytes; /**< Raw message data minus the message header. When serialising this should be assigned to a CBByteArray large enough to hold the serialised data. */
	uint8_t checksum[4]; /**< The message checksum. When sending messages using a CBNetworkCommunicator, this is calculated for you. */
	bool serialised; /**< True if this object has been serialised. If an object as already been serialised it is not serialised by parent objects. For instance when serialising a block, the transactions are not serialised if they have been already. However objects can be explicitly reserialised */
} CBMessage;

/**
 @brief Creates a new CBMessage object. This message will be created with object data and not with byte data. The message can be serialised for the byte data used over the network.
 @param 
 @returns A new CBMessage object.
 */
CBMessage * CBNewMessageByObject(void);

/**
 @brief Initialises a CBMessage object
 @param self The CBMessage object to initialise
 @returns true on success, false on failure.
 */
void CBInitMessageByObject(CBMessage * self);

/**
 @brief Initialises a CBMessage object from byte data.
 @param self The CBMessage object to initialise
 @param data The byte data for the object. The data will not be copied but retained by this object. 
 */
void CBInitMessageByData(CBMessage * self, CBByteArray * data);

/**
 @brief Release and free all of the objects stored by the CBMessage object.
 @param self The CBMessage object to free.
 */
void CBDestroyMessage(void * self);

/**
 @brief Frees a CBMessage object and also calles CBDestroyMessage.
 @param self The CBMessage object to free.
 */
void CBFreeMessage(void * self);

//  Functions

void CBMessagePrepareBytes(CBMessage * message, uint32_t length);
						 
void CBMessageTypeToString(CBMessageType type, char output[CB_MESSAGE_TYPE_STR_SIZE]);

#endif
