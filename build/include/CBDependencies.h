//
//  CBDependencies.h
//  cbitcoin
//
//  Created by Matthew Mitchell on 15/05/2012.
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
 @brief File for weak linked functions for dependency injection. All these functions are unimplemented. The functions include the crytography functions which are key for the functioning of bitcoin.
 */

#ifndef CBDEPENDENCIESH
#define CBDEPENDENCIESH

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#pragma weak CBSha256
#pragma weak CBRipemd160
#pragma weak CBSha160
#pragma weak CBEcdsaVerify

/*
 @brief SHA-256 cryptographic hash function.
 @param data A pointer to the byte data to hash.
 @param length The length of the data to hash.
 @returns A pointer to a 32-byte hash, allocated in the function
 */
u_int8_t * CBSha256(u_int8_t * data,u_int16_t length);
/*
 @brief RIPEMD-160 cryptographic hash function.
 @param data A pointer to the byte data to hash.
 @param length The length of the data to hash.
 @returns A pointer to a 20-byte hash, allocated in the function
 */
u_int8_t * CBRipemd160(u_int8_t * data,u_int16_t length);
/*
 @brief SHA-1 cryptographic hash function.
 @param data A pointer to the byte data to hash.
 @param length The length of the data to hash.
 @returns A pointer to a 10-byte hash, allocated in the function
 */
u_int8_t * CBSha160(u_int8_t * data,u_int16_t length);
/*
 @brief Verifies an ECDSA signature. This function must stick to the cryptography requirements in OpenSSL version 1.0.0 or any other compatible version. There may be compatibility problems when using libraries or code other than OpenSSL since OpenSSL does not adhere fully to the SEC1 ECDSA standards. This could cause security problems in your code. If in doubt, stick to OpenSSL.
 @param signature BER encoded signature bytes.
 @param sigLen The length of the signature bytes.
 @param hash A 32 byte hash for checking the signature against.
 @param pubKey Public key bytes to check this signature with.
 @param keyLen The length of the public key bytes.
 @returns true if the signature is valid and false if invalid.
 */
bool CBEcdsaVerify(u_int8_t * signature,u_int8_t sigLen,u_int8_t * hash,const u_int8_t * pubKey,u_int8_t keyLen);

#endif
