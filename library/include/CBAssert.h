//
//  CBAssert.h
//  cbitcoin
//
//  Created by Matthew Mitchell on 10/03/2014.
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
 @brief CBAssertExpression will execute the expression regardless of whether or not this is a debug build but only check it against the assertExp for a debug build
 */

#ifndef CBASSERTH
#define CBASSERTH

#include <assert.h>

#ifdef CBDEBUG

#define CBAssertExpression(expr, assertExp) assert((expr) assertExp)

#else

#define CBAssertExpression(expr, assertExp) (expr)

#endif

#endif
