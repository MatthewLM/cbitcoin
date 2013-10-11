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
#include "CBVarInt.h"
#include "CBDependencies.h"

// Constants and Macros

#define CB_MESSAGE_TYPE_STR_SIZE 11
#define CBGetMessage(x) ((CBMessage *)x)

/*
 @brief The type of a CBMessage.
 */
typedef enum{
	CB_MESSAGE_TYPE_NONE = 0,
	CB_MESSAGE_TYPE_VERSION = 1, /**< @see CBVersion.h */
	CB_MESSAGE_TYPE_VERACK = 2, /**< Acknowledgement and acceptance of a peer's version and connection. */
	CB_MESSAGE_TYPE_ADDR = 4, /**< @see CBNetworkAddress.h */
	CB_MESSAGE_TYPE_INV = 8, /**< @see CBInventory.h */
	CB_MESSAGE_TYPE_GETDATA = 16, /**< @see CBInventory.h */
	CB_MESSAGE_TYPE_GETBLOCKS = 32, /**< @see CBGetBlocks.h */
	CB_MESSAGE_TYPE_GETHEADERS = 64, /**< @see CBGetBlocks.h */
	CB_MESSAGE_TYPE_TX = 128, /**< @see CBTransaction.h */
	CB_MESSAGE_TYPE_BLOCK = 256, /**< @see CBBlock.h */
	CB_MESSAGE_TYPE_HEADERS = 512, /**< @see CBBlockHeaders.h */
	CB_MESSAGE_TYPE_GETADDR = 1024, /**< Request for "active peers". bitcoin-qt consiers active peers to be those that have sent messages in the last 30 minutes. */
	CB_MESSAGE_TYPE_PING = 2048, /**< @see CBPingPong.h */
	CB_MESSAGE_TYPE_PONG = 4096, /**< @see CBPingPong.h */
	CB_MESSAGE_TYPE_ALERT = 8192, /**< @see CBAlert.h */
	CB_MESSAGE_TYPE_ALT = 16384, /**< The message was defined by "alternativeMessages" in a CBNetworkCommunicator */
	CB_MESSAGE_TYPE_ADDRMAN = 32768, /**< @see CBNetworkAddressManager.h */
	CB_MESSAGE_TYPE_CHAINDESC = 65536, /**< @see CBChainDescriptor.h */
}CBMessageType;

/**
 @brief Structure for CBMessage objects. @see CBMessage.h
 */
typedef struct CBMessage{
	CBObject base; /**< CBObject base structure */
	CBMessageType type; /**< The type of the message */
	uint8_t * altText; /**< For an alternative message: This is the type text. */
	CBByteArray * bytes; /**< Raw message data minus the message header. When serialising this should be assigned to a CBByteArray large enough to hold the serialised data. */
	uint8_t checksum[4]; /**< The message checksum. When sending messages using a CBNetworkCommunicator, this is calculated for you. */
	CBMessageType expectResponse; /**< Set to zero if no message expected or the type of message expected as a response. */
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

void CBMessageTypeToString(CBMessageType type, char output[CB_MESSAGE_TYPE_STR_SIZE]);

#endif
