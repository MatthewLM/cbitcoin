//
//  CBAddressStorage.h
//  cbitcoin
//
//  Created by Matthew Mitchell on 29/01/2013.
//  Copyright (c) 2013 Matthew Mitchell
//
//  This file is part of cbitcoin. It is subject to the license terms
//  in the LICENSE file found in the top-level directory of this
//  distribution and at http://www.cbitcoin.com/license.html. No part of
//  cbitcoin, including this file, may be copied, modified, propagated,
//  or distributed except according to the terms contained in the
//  LICENSE file.

/**
 @file
 @brief Implements the network address storage dependency with use of a CBDatabase.
 */

#ifndef CBADDRESSSTORAGEH
#define CBADDRESSSTORAGEH

#include "CBDatabase.h"
#include "CBNetworkAddressManager.h"

typedef struct{
	CBDatabase base;
	uint64_t numAddresses;
} CBAddressStore;

#endif

