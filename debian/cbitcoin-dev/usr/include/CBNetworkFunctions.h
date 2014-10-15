//
//  CBNetworkFunctions.c
//  cbitcoin
//
//  Created by Matthew Mitchell on 01/08/2012.
//  Copyright (c) 2012 Matthew Mitchell
//
//  This file is part of cbitcoin. It is subject to the license terms
//  in the LICENSE file found in the top-level directory of this
//  distribution and at http://www.cbitcoin.com/license.html. No part of
//  cbitcoin, including this file, may be copied, modified, propagated,
//  or distributed except according to the terms contained in the
//  LICENSE file.

#ifndef CBNETWORKFUNCTIONSH
#define CBNETWORKFUNCTIONSH

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "CBConstants.h"

// Constants and Macros

typedef enum{
	CB_IP_INVALID = 0,
	CB_IP_IP4 = 1,
	CB_IP_IP6 = 2,
	CB_IP_LOCAL = 4,
	CB_IP_TOR = 8,
	CB_IP_I2P = 16,
	CB_IP_SITT = 32,
	CB_IP_RFC6052 = 64,
	CB_IP_6TO4 = 128,
	CB_IP_TEREDO = 256,
	CB_IP_HENET = 512
} CBIPType;

static const uint8_t IPv4Start[12] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xFF, 0xFF};
static const uint8_t SITTStart[12] = {0, 0, 0, 0, 0, 0, 0, 0, 0xFF, 0xFF, 0, 0};
static const uint8_t RFC6052Start[12] = {0, 0x64, 0xFF, 0x9B, 0, 0, 0, 0, 0, 0, 0, 0};

/**
 @brief Determines the type of IP, including validity.
 @param IP The IP address to check.
 @returns The IP type.
 */
CBIPType CBGetIPType(uint8_t * IP);

/**
 @brief Determines if an IP address is an GarliCat I2P hidden service representation
 @param IP The IP address to check.
 @returns true if the IP address is an I2P address, false otherwise.
 */
bool CBIsI2P(uint8_t * IP);

/**
 @brief Determines if an IP address is an IPv4 address
 @param IP The IP address to check.
 @returns true if the IP address is IPv4, false otherwise.
 */
bool CBIsIPv4(uint8_t * IP);

/**
 @brief Determines if an IP address is an OnionCat Tor hidden service representation
 @param IP The IP address to check.
 @returns true if the IP address is a Tor address, false otherwise.
 */
bool CBIsTor(uint8_t * IP);

#endif
