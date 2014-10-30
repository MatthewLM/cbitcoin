//
//  CBWIF.h
//  cbitcoin
//
//  Created by Matthew Mitchell on 20/10/2012.
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
 @brief The Wallet Import Format Private key representation. See: https://en.bitcoin.it/wiki/Wallet_import_format. Inherits CBChecksumBytes.
*/

#ifndef CBWIFH
#define CBWIFH

// Getter

#define CBGetWIF(x) ((CBWIF *)x)

//  Includes

#include "CBChecksumBytes.h"

/**
 @brief Structure for CBWIF objects. @see CBWIF.h Alias of CBChecksumBytes
*/
typedef CBChecksumBytes CBWIF;

/**
 @brief Creates a new CBWIF object from a RIPEMD-160 hash.
 @param privateKey The 32 byte private key.
 @param cacheString If true, the bitcoin string for this object will be cached in memory.
 @returns A new CBWIF object.
 */
CBWIF * CBNewWIFFromPrivateKey(uint8_t * privKey, bool useCompression, CBBase58Prefix prefix, bool cacheString);

/**
 @brief Creates a new CBWIF object from a base-58 encoded string.
 @param self The CBWIF object to initialise.
 @param string The base-58 encoded CBString with a termination character.
 @param cacheString If true, the bitcoin string for this object will be cached in memory.
 @returns A new CBWIF object. Returns NULL on failure such as an invalid bitcoin WIF.
 */
CBWIF * CBNewWIFFromString(CBByteArray * string, bool cacheString);

/**
 @brief Initialises a CBWIF object from a RIPEMD-160 hash.
 @param self The CBWIF object to initialise.
 @param privateKey The 32 byte private key.
 @param cacheString If true, the bitcoin string for this object will be cached in memory.
 */
void CBInitWIFFromPrivateKey(CBWIF * self, uint8_t * privKey, bool useCompression, CBBase58Prefix prefix, bool cacheString);

/**
 @brief Initialises a CBWIF object from a base-58 encoded string.
 @param self The CBWIF object to initialise.
 @param string The base-58 encoded CBString with a termination character.
 @param cacheString If true, the bitcoin string for this object will be cached in memory.
 @returns true on success, false on failure.
 */
bool CBInitWIFFromString(CBWIF * self, CBByteArray * string, bool cacheString);

/**
 @brief Releases and frees all of the objects stored by the CBWIF object.
 @param self The CBWIF object to destroy.
 */
void CBDestroyWIF(void * self);

/**
 @brief Frees a CBWIF object and also calls CBDestoryWIF
 @param self The CBWIF object to free.
 */
void CBFreeWIF(void * self);
 
//  Functions

void CBWIFGetPrivateKey(CBWIF * self, uint8_t * privKey);
bool CBWIFUseCompression(CBWIF * self);

#endif
