//
//  CBSafeStorage.h
//  cbitcoin
//
//  Created by Matthew Mitchell on 03/11/2012.
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
 @brief Implements the storage dependencies.
 */

#ifndef CBSAFESTORAGEH
#define CBSAFESTORAGEH

#include "CBSafeOutput.h"
#include "CBDependencies.h"
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/resource.h>
#include <sys/syslimits.h>

/**
 @brief Structure for CBSafeStorage objects. @see CBSafeStorage.h
 */
typedef struct{
	uint64_t fileSizeLimit; /**< The maximum allowed filesize */
	CBSafeOutput output; /**< For outputting data */
	uint64_t backup; /**< The backup file */
	char * dataDir; /**< The data directory */
	uint8_t numOrphans; /**< The number of orhpans */
	CBBlock * orphans[CB_MAX_ORPHAN_CACHE]; /**< The ophan block references */
	uint8_t mainBranch; /**< The index for the main branch */
	uint8_t numBranches; /**< The number of block-chain branches. Cannot exceed CB_MAX_BRANCH_CACHE */
	CBBlockBranch branches[CB_MAX_BRANCH_CACHE]; /**< The block-chain branches. */
} CBSafeStorage;

#endif
