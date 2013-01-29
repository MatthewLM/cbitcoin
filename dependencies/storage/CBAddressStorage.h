//
//  CBAddressStorage.h
//  cbitcoin
//
//  Created by Matthew Mitchell on 29/01/2013.
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
 @brief Implements the network address storage dependency with use of a CBDatabase.
 */

#ifndef CBADDRESSSTORAGEH
#define CBADDRESSSTORAGEH

#include "CBDatabase.h"
#include "CBAddressManager.h"

typedef struct{
	CBDatabase base;
	uint64_t numAddresses;
} CBAddressStore;

#endif

