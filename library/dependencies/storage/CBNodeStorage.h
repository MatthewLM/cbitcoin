//
//  CBNodeStorage.h
//  cbitcoin
//
//  Created by Matthew Mitchell on 30/09/2013.
//  Copyright (c) 2013 Matthew Mitchell
//
//  This file is part of cbitcoin. It is subject to the license terms
//  in the LICENSE file found in the top-level directory of this
//  distribution and at http://www.cbitcoin.com/license.html. No part of
//  cbitcoin, including this file, may be copied, modified, propagated,
//  or distributed except according to the terms contained in the
//  LICENSE file.

#ifndef CBNODESTORAGEH
#define CBNODESTORAGEH

#include "CBDatabase.h"
#include "CBNetworkAddressManager.h"

#define CB_START_SCANNING_TIME CB_ACCOUNTER_EXTRA_SIZE + CB_BLOCK_CHAIN_EXTRA_SIZE + CB_ADDRESS_EXTRA_SIZE

typedef struct{
	CBDatabase * database;
	CBDatabaseIndex * ourTxs;
	CBDatabaseIndex * otherTxs;
} CBNodeStorage;

#endif

