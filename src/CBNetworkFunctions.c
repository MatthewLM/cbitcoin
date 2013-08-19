//
//  CBNetworkFunctions.h
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

#include "CBNetworkFunctions.h"

CBIPType CBGetIPType(uint8_t * IP){
	if (! memcmp(IP, IPv4Start+3, 9)) // Something to do with old bitcoin versions...
        return CB_IP_INVALID;
	// Check for unspecified address.
	uint8_t x = 0;
	for (; x < 16; x++) {
		if (IP[x])
			break;
	}
	if (x == 16)
		return CB_IP_INVALID;
	if(! x && CBIsTor(IP)) // Check x first to see that the first byte was not zero. Just prCBLogError uneccessary memory comparisons.
		return CB_IP_TOR;
	if(! x && CBIsI2P(IP))
		return CB_IP_I2P;
	if (x == 10 && IP[10] == 0xFF && IP[11] == 0xFF) { // Faster than CBIsIPv4
		if (IP[12] == 0x7F || IP[12] == 0x0)
			// Loopback address
			return CB_IP_IPv4 | CB_IP_LOCAL;
		if(IP[12] == 0x0A || (IP[12] == 0xC0 && IP[13] == 0xA3) || (IP[12] == 0xAC && (IP[13] >= 0x10 && IP[13] <= 0x1F)))
			// Reserved for private internets.
			return CB_IP_INVALID;
		if (IP[12] == 0xA9 && IP[13] == 0xFE)
			// Link-Local reserved addresses.
			return CB_IP_INVALID;
		if (IP[12] == 0x00 && IP[13] == 0x00 && IP[14] == 0x00 && IP[15] == 0x00)
			// Zero network
			return CB_IP_INVALID;
		if (IP[12] == 0xFF && IP[13] == 0xFF && IP[14] == 0xFF && IP[15] == 0xFF)
			// Broadcast address
			return CB_IP_INVALID;
		return CB_IP_IPv4;
	}
	if (! memcmp(IP, SITTStart, 12)) {
		return CB_IP_IPv6 | CB_IP_SITT;
	}
	if (! memcmp(IP, RFC6052Start, 12)) {
		return CB_IP_IPv6 | CB_IP_RFC6052;
	}
	if (IP[0] == 0x20 && IP[1] == 0x01 && IP[2] == 0 && IP[3] == 0) {
		return CB_IP_IPv6 | CB_IP_TEREDO;
	}
	if (IP[0] == 0x20 && IP[1] == 0x02) {
		return CB_IP_IPv6 | CB_IP_6TO4;
	}
	if (IP[0] == 0x20 && IP[1] == 0x11 && IP[2] == 0x04 && IP[3] == 0x70) {
		return CB_IP_IPv6 | CB_IP_HENET;
	}
	if (x == 15 && IP[15] == 1)
		// Loopback address
		return CB_IP_LOCAL | CB_IP_IPv6;
	if (IP[0] == 0x20 && IP[1] == 0x01 && IP[2] == 0x0D && IP[3] == 0xB8)
		// Reserved for documentation
		return CB_IP_INVALID;
	if (! memcmp(IP, (uint8_t []){0xFE, 0x80, 0, 0, 0, 0, 0, 0}, 8))
		// Link-Local RFC4862 reserved addresses
		return CB_IP_INVALID;
	if (IP[0] == 0x20 && IP[1] == 0x01 && IP[2] == 0x00 && (IP[3] & 0xF0) == 0x10)
		// RFC4843 addresses
		return CB_IP_INVALID;
	if ((IP[0] & 0xFE) == 0xFC)
		// Reserved Unicast addresses
		return CB_IP_INVALID;
	return CB_IP_IPv6;
}
bool CBIsI2P(uint8_t * IP){
	return ! memcmp(IP, (uint8_t []){0xFD, 0x60, 0xDB, 0x4D, 0xDD, 0xB5}, 6);
}
bool CBIsIPv4(uint8_t * IP){
	return ! memcmp(IP, IPv4Start, 12);
}
bool CBIsTor(uint8_t * IP){
	return ! memcmp(IP, (uint8_t []){0xFD, 0x87, 0xD8, 0x7E, 0xEB, 0x43}, 6);
}
