//
//  CBDependencies.h
//  cbitcoin
//
//  Created by Matthew Mitchell on 10/05/2012.
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
 @brief File for the structure that holds function pointers for dependency injection.
 */

#ifndef CBDEPENDENCIESH
#define CBDEPENDENCIESH

#include <stdint.h>

/**
 @brief Structure holding event callback function pointers.
 */
typedef struct{
	u_int8_t * (*sha256)(u_int8_t *,u_int16_t); /**< SHA-256 cryptographic hash function. The first argument is a pointer to the byte data. The second argument is the length of the data to hash. Should return a pointer to a 32-byte hash. */
}CBDependencies;

#endif
