//
//  CBSanitiseOutput.h
//  cbitcoin
//
//  Created by Matthew Mitchell on 23/03/2014
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
 @brief The output of strings taken from the network or some untrusted source could contain undesirable characters such as control codes, so CBSanitiseOutput replaces non-whitelisted characters with '.'
*/

#ifndef CBSANITISEOUTPUTH
#define CBSANITISEOUTPUTH

#include <string.h>

void CBSanitiseOutput(char * str, size_t strLen);

#endif
