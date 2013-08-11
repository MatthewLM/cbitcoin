//
//  CBAlert.h
//  cbitcoin
//
//  Created by Matthew Mitchell on 13/07/2012.
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
 @brief A message containing an alert signed by one of the bitcoin developers. Inherits CBMessage
*/

#ifndef CBALERTH
#define CBALERTH

//  Includes

#include "CBMessage.h"

// Constants and Macros

#define CB_ALERT_MAX_SIZE 2100 // Max size for payload is 2000. Extra 100 for signature though the signature will likely be less.
#define CBGetAlert(x) ((CBAlert *)x)

/**
 @brief Structure for CBAlert objects. @see CBAlert.h
*/
typedef struct{
	CBMessage base; /**< CBMessage base structure */
	int32_t version; /**< The version of the alert */
	int64_t relayUntil; /**< Do not relay past this date */
	int64_t expiration; /**< The date at which the alert is no longer in effect */
	int32_t ID; /**< Unique id for this alert */
	int32_t cancel; /**< Alerts under this ID should be deleted and not relayed in the future */
	uint16_t setCancelNum; /**< Number of IDs to cancel */
	int32_t * setCancel; /**< A selection of IDs to cancel */
	int32_t minVer; /**< The alert is valid for versions greater than this. */
	int32_t maxVer; /**< The alert is valid for versions less than this. */
	uint16_t userAgentNum; /**< Number of user agents. */
	CBByteArray ** userAgents; /**< List of the userAgents of the effected versions. */
	int32_t priority; /**< Priority compared to other alerts */
	CBByteArray * hiddenComment; /**< Comment that should be hidden from end-users */
	CBByteArray * displayedComment; /**< Comment that can be displayed to end-users */
	CBByteArray * reserved; /**< Reserved bytes */
	CBByteArray * signature; /**< ECDSA signature for data */
} CBAlert;

/**
 @brief Creates a new CBAlert object.
 @returns A new CBAlert object.
*/
CBAlert * CBNewAlert(int32_t version, int64_t relayUntil, int64_t expiration, int32_t ID, int32_t cancel, int32_t minVer, int32_t maxVer, int32_t priority, CBByteArray * hiddenComment, CBByteArray * displayedComment, CBByteArray * reserved, CBByteArray * signature);
/**
@brief Creates a new CBAlert object from serialised data.
 @param data Serialised CBAlert data.
 @returns A new CBAlert object.
*/
CBAlert * CBNewAlertFromData(CBByteArray * data);

/**
 @brief Initialises a CBAlert object
 @param self The CBAlert object to initialise
*/
void CBInitAlert(CBAlert * self, int32_t version, int64_t relayUntil, int64_t expiration, int32_t ID, int32_t cancel, int32_t minVer, int32_t maxVer, int32_t priority, CBByteArray * hiddenComment, CBByteArray * displayedComment, CBByteArray * reserved, CBByteArray * signature);
/**
 @brief Initialises a CBAlert object from serialised data
 @param self The CBAlert object to initialise
 @param data The serialised data.
*/
void CBInitAlertFromData(CBAlert * self, CBByteArray * data);

/**
 @brief Releases and frees all of the objects stored by the CBAlert object.
 @param self The CBAlert object to destory.
 */
void CBDestroyAlert(void * self);
/**
 @brief Frees a CBAlert object and also calls CBDestroyAlert.
 @param self The CBAlert object to free.
 */
void CBFreeAlert(void * self);
 
//  Functions

/**
 @brief Adds a ID to the cancel set
 @param self The CBAlert object
 @param ID The id to add.
 */
void CBAlertAddCancelID(CBAlert * self, uint32_t ID);
/**
 @brief Adds a user agent to the user agent set
 @param self The CBAlert object
 @param userAgent The user agent to add.
 */
void CBAlertAddUserAgent(CBAlert * self, CBByteArray * userAgent);
/**
 @brief Calculates the length needed to serialise the object.
 @param self The CBAlert object.
 @returns The length.
 */
uint32_t CBAlertCalculateLength(CBAlert * self);
/**
 @brief Deserialises a CBAlert so that it can be used as an object.
 @param self The CBAlert object
 @returns The length read on success, 0 on failure.
*/
uint32_t CBAlertDeserialise(CBAlert * self);
/**
 @brief Gets the payload from the data. Should be serialised beforehand.
 @param self The CBAlert object
 @returns A CBByteArray object for the payload. Can be used to check the signature. When checking the signature the payload should be hashed by SHA256 twice.
 */
CBByteArray * CBAlertGetPayload(CBAlert * self);
/**
 @brief Serialises the payload of a CBAlert and returns a CBByteArray to the specifc payload bytes to aid in creating a signature.
 @param self The CBAlert object
 @returns The payload bytes on sucess. NULL on failure.
*/
CBByteArray * CBAlertSerialisePayload(CBAlert * self);
/**
 @brief Serialises the signature of a CBAlert.
 @param self The CBAlert object
 @param offset The offset to the begining of the signature which should be exactly after the payload.
 @returns The total length of the serialised CBAlert on sucess, else false.
 */
uint16_t CBAlertSerialiseSignature(CBAlert * self, uint16_t offset);
/**
 @brief Takes a user agent for the user agent set. This does not retain the CBByteArray so you can pass an CBByteArray into this while releasing control from the calling function.
 @param self The CBAlert object
 @param userAgent The user agent to take.
 */
void CBAlertTakeUserAgent(CBAlert * self, CBByteArray * userAgent);

#endif
