//
//  testCBHDKeys.c
//  cbitcoin
//
//  Created by Matthew Mitchell on 20/10/2013.
//  Copyright (c) 2013 Matthew Mitchell
//  
//  This file is part of cbitcoin. It is subject to the license terms
//  in the LICENSE file found in the top-level directory of this
//  distribution and at http://www.cbitcoin.com/license.html. No part of
//  cbitcoin, including this file, may be copied, modified, propagated,
//  or distributed except according to the terms contained in the
//  LICENSE file.

#include "CBHDKeys.h"
#include "CBChecksumBytes.h"
#include "CBAddress.h"
#include "CBWIF.h"

// BIP0032 test vectors: https://en.bitcoin.it/wiki/BIP_0032_TestVectors

#define NUM_TEST_VECTORS 2
#define NUM_CHILDREN 5

typedef struct{
	char privString[112];
	char pubString[112];
	char addr[35];
	char WIF[53];
	uint8_t chainCode[32];
	CBHDKeyChildID childID;
} CBTestVector;

CBTestVector testVectors[NUM_TEST_VECTORS][NUM_CHILDREN+1] = {
	// Test vector 1
	{
		// m
		{
			"xprv9s21ZrQH143K3QTDL4LXw2F7HEK3wJUD2nW2nRk4stbPy6cq3jPPqjiChkVvvNKmPGJxWUtg6LnF5kejMRNNU3TGtRBeJgk33yuGBxrMPHi",
			"xpub661MyMwAqRbcFtXgS5sYJABqqG9YLmC4Q1Rdap9gSE8NqtwybGhePY2gZ29ESFjqJoCu1Rupje8YtGqsefD265TMg7usUDFdp6W1EGMcet8",
			"15mKKb2eos1hWa6tisdPwwDC1a5J1y9nma",
			"L52XzL2cMkHxqxBXRyEpnPQZGUs3uKiL3R11XbAdHigRzDozKZeW",
			{0x87,0x3d,0xff,0x81,0xc0,0x2f,0x52,0x56,0x23,0xfd,0x1f,0xe5,0x16,0x7e,0xac,0x3a,0x55,0xa0,0x49,0xde,0x3d,0x31,0x4b,0xb4,0x2e,0xe2,0x27,0xff,0xed,0x37,0xd5,0x08},
			{
				false,
				0
			}
		},
		// m/0'
		{
			"xprv9uHRZZhk6KAJC1avXpDAp4MDc3sQKNxDiPvvkX8Br5ngLNv1TxvUxt4cV1rGL5hj6KCesnDYUhd7oWgT11eZG7XnxHrnYeSvkzY7d2bhkJ7",
			"xpub68Gmy5EdvgibQVfPdqkBBCHxA5htiqg55crXYuXoQRKfDBFA1WEjWgP6LHhwBZeNK1VTsfTFUHCdrfp1bgwQ9xv5ski8PX9rL2dZXvgGDnw",
			"19Q2WoS5hSS6T8GjhK8KZLMgmWaq4neXrh",
			"L5BmPijJjrKbiUfG4zbiFKNqkvuJ8usooJmzuD7Z8dkRoTThYnAT",
			{0x47,0xfd,0xac,0xbd,0x0f,0x10,0x97,0x04,0x3b,0x78,0xc6,0x3c,0x20,0xc3,0x4e,0xf4,0xed,0x9a,0x11,0x1d,0x98,0x00,0x47,0xad,0x16,0x28,0x2c,0x7a,0xe6,0x23,0x61,0x41},
			{
				true,
				0
			}
		},
		// m/0'/1
		{
			"xprv9wTYmMFdV23N2TdNG573QoEsfRrWKQgWeibmLntzniatZvR9BmLnvSxqu53Kw1UmYPxLgboyZQaXwTCg8MSY3H2EU4pWcQDnRnrVA1xe8fs",
			"xpub6ASuArnXKPbfEwhqN6e3mwBcDTgzisQN1wXN9BJcM47sSikHjJf3UFHKkNAWbWMiGj7Wf5uMash7SyYq527Hqck2AxYysAA7xmALppuCkwQ",
			"1JQheacLPdM5ySCkrZkV66G2ApAXe1mqLj",
			"KyFAjQ5rgrKvhXvNMtFB5PCSKUYD1yyPEe3xr3T34TZSUHycXtMM",
			{0x2a,0x78,0x57,0x63,0x13,0x86,0xba,0x23,0xda,0xca,0xc3,0x41,0x80,0xdd,0x19,0x83,0x73,0x4e,0x44,0x4f,0xdb,0xf7,0x74,0x04,0x15,0x78,0xe9,0xb6,0xad,0xb3,0x7c,0x19},
			{
				false,
				1
			}
		},
		// m/0'/1/2'
		{
			"xprv9z4pot5VBttmtdRTWfWQmoH1taj2axGVzFqSb8C9xaxKymcFzXBDptWmT7FwuEzG3ryjH4ktypQSAewRiNMjANTtpgP4mLTj34bhnZX7UiM",
			"xpub6D4BDPcP2GT577Vvch3R8wDkScZWzQzMMUm3PWbmWvVJrZwQY4VUNgqFJPMM3No2dFDFGTsxxpG5uJh7n7epu4trkrX7x7DogT5Uv6fcLW5",
			"1NjxqbA9aZWnh17q1UW3rB4EPu79wDXj7x",
			"L43t3od1Gh7Lj55Bzjj1xDAgJDcL7YFo2nEcNaMGiyRZS1CidBVU",
			{0x04,0x46,0x6b,0x9c,0xc8,0xe1,0x61,0xe9,0x66,0x40,0x9c,0xa5,0x29,0x86,0xc5,0x84,0xf0,0x7e,0x9d,0xc8,0x1f,0x73,0x5d,0xb6,0x83,0xc3,0xff,0x6e,0xc7,0xb1,0x50,0x3f},
			{
				true,
				2
			}
		},
		// m/0'/1/2'/2
		{
			"xprvA2JDeKCSNNZky6uBCviVfJSKyQ1mDYahRjijr5idH2WwLsEd4Hsb2Tyh8RfQMuPh7f7RtyzTtdrbdqqsunu5Mm3wDvUAKRHSC34sJ7in334",
			"xpub6FHa3pjLCk84BayeJxFW2SP4XRrFd1JYnxeLeU8EqN3vDfZmbqBqaGJAyiLjTAwm6ZLRQUMv1ZACTj37sR62cfN7fe5JnJ7dh8zL4fiyLHV",
			"1LjmJcdPnDHhNTUgrWyhLGnRDKxQjoxAgt",
			"KwjQsVuMjbCP2Zmr3VaFaStav7NvevwjvvkqrWd5Qmh1XVnCteBR",
			{0xcf,0xb7,0x18,0x83,0xf0,0x16,0x76,0xf5,0x87,0xd0,0x23,0xcc,0x53,0xa3,0x5b,0xc7,0xf8,0x8f,0x72,0x4b,0x1f,0x8c,0x28,0x92,0xac,0x12,0x75,0xac,0x82,0x2a,0x3e,0xdd},
			{
				false,
				2
			}
		},
		// m/0'/1/2'/2/1000000000
		{
			"xprvA41z7zogVVwxVSgdKUHDy1SKmdb533PjDz7J6N6mV6uS3ze1ai8FHa8kmHScGpWmj4WggLyQjgPie1rFSruoUihUZREPSL39UNdE3BBDu76",
			"xpub6H1LXWLaKsWFhvm6RVpEL9P4KfRZSW7abD2ttkWP3SSQvnyA8FSVqNTEcYFgJS2UaFcxupHiYkro49S8yGasTvXEYBVPamhGW6cFJodrTHy",
			"1LZiqrop2HGR4qrH1ULZPyBpU6AUP49Uam",
			"Kybw8izYevo5xMh1TK7aUr7jHFCxXS1zv8p3oqFz3o2zFbhRXHYs",
			{0xc7,0x83,0xe6,0x7b,0x92,0x1d,0x2b,0xeb,0x8f,0x6b,0x38,0x9c,0xc6,0x46,0xd7,0x26,0x3b,0x41,0x45,0x70,0x1d,0xad,0xd2,0x16,0x15,0x48,0xa8,0xb0,0x78,0xe6,0x5e,0x9e},
			{
				false,
				1000000000
			}
		},
	},
	// Test vector 2
	{
		// m
		{
			"xprv9s21ZrQH143K31xYSDQpPDxsXRTUcvj2iNHm5NUtrGiGG5e2DtALGdso3pGz6ssrdK4PFmM8NSpSBHNqPqm55Qn3LqFtT2emdEXVYsCzC2U",
			"xpub661MyMwAqRbcFW31YEwpkMuc5THy2PSt5bDMsktWQcFF8syAmRUapSCGu8ED9W6oDMSgv6Zz8idoc4a6mr8BDzTJY47LJhkJ8UB7WEGuduB",
			"1JEoxevbLLG8cVqeoGKQiAwoWbNYSUyYjg",
			"KyjXhyHF9wTphBkfpxjL8hkDXDUSbE3tKANT94kXSyh6vn6nKaoy",
			{0x60, 0x49, 0x9f, 0x80, 0x1b, 0x89, 0x6d, 0x83, 0x17, 0x9a, 0x43, 0x74, 0xae, 0xb7, 0x82, 0x2a, 0xae, 0xac, 0xea, 0xa0, 0xdb, 0x1f, 0x85, 0xee, 0x3e, 0x90, 0x4c, 0x4d, 0xef, 0xbd, 0x96, 0x89},
			{
				false,
				0
			}
		},
		// m/0
		{
			"xprv9vHkqa6EV4sPZHYqZznhT2NPtPCjKuDKGY38FBWLvgaDx45zo9WQRUT3dKYnjwih2yJD9mkrocEZXo1ex8G81dwSM1fwqWpWkeS3v86pgKt",
			"xpub69H7F5d8KSRgmmdJg2KhpAK8SR3DjMwAdkxj3ZuxV27CprR9LgpeyGmXUbC6wb7ERfvrnKZjXoUmmDznezpbZb7ap6r1D3tgFxHmwMkQTPH",
			"19EuDJdgfRkwCmRzbzVBHZWQG9QNWhftbZ",
			"L2ysLrR6KMSAtx7uPqmYpoTeiRzydXBattRXjXz5GDFPrdfPzKbj",
			{0xf0, 0x90, 0x9a, 0xff, 0xaa, 0x7e, 0xe7, 0xab, 0xe5, 0xdd, 0x4e, 0x10, 0x05, 0x98, 0xd4, 0xdc, 0x53, 0xcd, 0x70, 0x9d, 0x5a, 0x5c, 0x2c, 0xac, 0x40, 0xe7, 0x41, 0x2f, 0x23, 0x2f, 0x7c, 0x9c},
			{
				false,
				0
			}
		},
		// m/0/2147483647'
		{
			"xprv9wSp6B7kry3Vj9m1zSnLvN3xH8RdsPP1Mh7fAaR7aRLcQMKTR2vidYEeEg2mUCTAwCd6vnxVrcjfy2kRgVsFawNzmjuHc2YmYRmagcEPdU9",
			"xpub6ASAVgeehLbnwdqV6UKMHVzgqAG8Gr6riv3Fxxpj8ksbH9ebxaEyBLZ85ySDhKiLDBrQSARLq1uNRts8RuJiHjaDMBU4Zn9h8LZNnBC5y4a",
			"1Lke9bXGhn5VPrBuXgN12uGUphrttUErmk",
			"L1m5VpbXmMp57P3knskwhoMTLdhAAaXiHvnGLMribbfwzVRpz2Sr",
			{0xbe, 0x17, 0xa2, 0x68, 0x47, 0x4a, 0x6b, 0xb9, 0xc6, 0x1e, 0x1d, 0x72, 0x0c, 0xf6, 0x21, 0x5e, 0x2a, 0x88, 0xc5, 0x40, 0x6c, 0x4a, 0xee, 0x7b, 0x38, 0x54, 0x7f, 0x58, 0x5c, 0x9a, 0x37, 0xd9},
			{
				true,
				2147483647
			}
		},
		// m/0/2147483647'/1
		{
			"xprv9zFnWC6h2cLgpmSA46vutJzBcfJ8yaJGg8cX1e5StJh45BBciYTRXSd25UEPVuesF9yog62tGAQtHjXajPPdbRCHuWS6T8XA2ECKADdw4Ef",
			"xpub6DF8uhdarytz3FWdA8TvFSvvAh8dP3283MY7p2V4SeE2wyWmG5mg5EwVvmdMVCQcoNJxGoWaU9DCWh89LojfZ537wTfunKau47EL2dhHKon",
			"1BxrAr2pHpeBheusmd6fHDP2tSLAUa3qsW",
			"KzyzXnznxSv249b4KuNkBwowaN3akiNeEHy5FWoPCJpStZbEKXN2",
			{0xf3, 0x66, 0xf4, 0x8f, 0x1e, 0xa9, 0xf2, 0xd1, 0xd3, 0xfe, 0x95, 0x8c, 0x95, 0xca, 0x84, 0xea, 0x18, 0xe4, 0xc4, 0xdd, 0xb9, 0x36, 0x6c, 0x33, 0x6c, 0x92, 0x7e, 0xb2, 0x46, 0xfb, 0x38, 0xcb},
			{
				false,
				1
			}
		},
		// m/0/2147483647'/1/2147483646'
		{
			"xprvA1RpRA33e1JQ7ifknakTFpgNXPmW2YvmhqLQYMmrj4xJXXWYpDPS3xz7iAxn8L39njGVyuoseXzU6rcxFLJ8HFsTjSyQbLYnMpCqE2VbFWc",
			"xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL",
			"15XVotxCAV7sRx1PSCkQNsGw3W9jT9A94R",
			"L5KhaMvPYRW1ZoFmRjUtxxPypQ94m6BcDrPhqArhggdaTbbAFJEF",
			{0x63, 0x78, 0x07, 0x03, 0x0d, 0x55, 0xd0, 0x1f, 0x9a, 0x0c, 0xb3, 0xa7, 0x83, 0x95, 0x15, 0xd7, 0x96, 0xbd, 0x07, 0x70, 0x63, 0x86, 0xa6, 0xed, 0xdf, 0x06, 0xcc, 0x29, 0xa6, 0x5a, 0x0e, 0x29},
			{
				true,
				2147483646
			}
		},
		// m/0/2147483647'/1/2147483646'/2
		{
			"xprvA2nrNbFZABcdryreWet9Ea4LvTJcGsqrMzxHx98MMrotbir7yrKCEXw7nadnHM8Dq38EGfSh6dqA9QWTyefMLEcBYJUuekgW4BYPJcr9E7j",
			"xpub6FnCn6nSzZAw5Tw7cgR9bi15UV96gLZhjDstkXXxvCLsUXBGXPdSnLFbdpq8p9HmGsApME5hQTZ3emM2rnY5agb9rXpVGyy3bdW6EEgAtqt",
			"14UKfRV9ZPUp6ZC9PLhqbRtxdihW9em3xt",
			"L3WAYNAZPxx1fr7KCz7GN9nD5qMBnNiqEJNJMU1z9MMaannAt4aK",
			{0x94, 0x52, 0xb5, 0x49, 0xbe, 0x8c, 0xea, 0x3e, 0xcb, 0x7a, 0x84, 0xbe, 0xc1, 0x0d, 0xcf, 0xd9, 0x4a, 0xfe, 0x4d, 0x12, 0x9e, 0xbf, 0xd3, 0xb3, 0xcb, 0x58, 0xee, 0xdf, 0x39, 0x4e, 0xd2, 0x71},
			{
				false,
				2
			}
		},
	}
};

void checkKey(CBHDKey * key, uint8_t x, uint8_t y);
void checkKey(CBHDKey * key, uint8_t x, uint8_t y){
	// Check version bytes
	if (key->versionBytes != CB_HD_KEY_VERSION_PROD_PRIVATE) {
		printf("VERSION BYTES FAIL AT %u - %u\n", x, y);
		exit(EXIT_FAILURE);
	}
	// Check address
	CBAddress * address = CBNewAddressFromRIPEMD160Hash(CBHDKeyGetHash(key), CB_PREFIX_PRODUCTION_ADDRESS, false);
	CBByteArray * str = CBChecksumBytesGetString(CBGetChecksumBytes(address));
	CBReleaseObject(address);
	if (memcmp(CBByteArrayGetData(str), testVectors[x][y].addr, 34) != 0) {
		printf("ADDR FAIL AT %u - %u\n", x, y);
		exit(EXIT_FAILURE);
	}
	CBReleaseObject(str);
	// Check WIF
	CBWIF * wif = CBHDKeyGetWIF(key);
	str = CBChecksumBytesGetString(CBGetChecksumBytes(wif));
	CBReleaseObject(wif);
	if (memcmp(CBByteArrayGetData(str), testVectors[x][y].WIF, 52) != 0) {
		printf("WIF FAIL AT %u - %u\n", x, y);
		exit(EXIT_FAILURE);
	}
	CBReleaseObject(str);
	// Check chain code
	if (memcmp(key->chainCode, testVectors[x][y].chainCode, 32) != 0) {
		printf("CHAIN CODE FAIL AT %u - %u\n", x, y);
		exit(EXIT_FAILURE);
	}
	// Check child ID
	if (memcmp(&key->childID, &testVectors[x][y].childID, sizeof(key->childID)) != 0) {
		printf("CHILD ID FAIL AT %u - %u\n", x, y);
		exit(EXIT_FAILURE);
	}
	// Check depth
	if (key->depth != y) {
		printf("DEPTH FAIL AT %u - %u\n", x, y);
		exit(EXIT_FAILURE);
	}
	// Check serialisation of private key
	uint8_t * keyData = malloc(CB_HD_KEY_STR_SIZE);
	CBHDKeySerialise(key, keyData);
	CBChecksumBytes * checksumBytes = CBNewChecksumBytesFromBytes(keyData, 82, false);
	str = CBChecksumBytesGetString(checksumBytes);
	CBReleaseObject(checksumBytes);
	if (memcmp(CBByteArrayGetData(str), testVectors[x][y].privString, 111) != 0) {
		printf("PRIVATE KEY STRING FAIL AT %u - %u\n", x, y);
		exit(EXIT_FAILURE);
	}
	CBReleaseObject(str);
	// Check serialisation of public key
	key->versionBytes = CB_HD_KEY_VERSION_PROD_PUBLIC;
	keyData = malloc(CB_HD_KEY_STR_SIZE);
	CBHDKeySerialise(key, keyData);
	checksumBytes = CBNewChecksumBytesFromBytes(keyData, 82, false);
	str = CBChecksumBytesGetString(checksumBytes);
	CBReleaseObject(checksumBytes);
	if (memcmp(CBByteArrayGetData(str), testVectors[x][y].pubString, 111) != 0) {
		printf("PUBLIC KEY STRING FAIL AT %u - %u\n", x, y);
		exit(EXIT_FAILURE);
	}
	CBReleaseObject(str);
	// Make private again
	key->versionBytes = CB_HD_KEY_VERSION_PROD_PRIVATE;
}

int main(){
	CBByteArray * walletKeyString = CBNewByteArrayFromString("xpub6DRhpXssnj7X6CwJgseK9oyFxSC8jk6nJz2SWkf5pjsQs12xv89Dfr627TtaZKkFbG6Aq23fmaNaf5KRo9iGfEXTTXvtd6gsXJTB8Sdah3B", false);
    CBChecksumBytes * walletKeyData = CBNewChecksumBytesFromString(walletKeyString, false);
    CBHDKey * cbkey = CBNewHDKeyFromData(CBByteArrayGetData(CBGetByteArray(walletKeyData)));
	CBAddress * address = CBNewAddressFromRIPEMD160Hash(CBHDKeyGetHash(cbkey), CB_PREFIX_PRODUCTION_ADDRESS, false);
	CBByteArray * str = CBChecksumBytesGetString(CBGetChecksumBytes(address));
	printf("%s\n", CBByteArrayGetData(str));
	CBReleaseObject(address);
	// Test type
	if (CBHDKeyGetType(CB_HD_KEY_VERSION_PROD_PRIVATE) != CB_HD_KEY_TYPE_PRIVATE) {
		printf("CB_HD_KEY_VERSION_PROD_PRIVATE TYPE FAIL\n");
		return EXIT_FAILURE;
	}
	if (CBHDKeyGetType(CB_HD_KEY_VERSION_PROD_PUBLIC) != CB_HD_KEY_TYPE_PUBLIC) {
		printf("CB_HD_KEY_VERSION_PROD_PUBLIC TYPE FAIL\n");
		return EXIT_FAILURE;
	}
	if (CBHDKeyGetType(CB_HD_KEY_VERSION_TEST_PRIVATE) != CB_HD_KEY_TYPE_PRIVATE) {
		printf("CB_HD_KEY_VERSION_TEST_PRIVATE TYPE FAIL\n");
		return EXIT_FAILURE;
	}
	if (CBHDKeyGetType(CB_HD_KEY_VERSION_TEST_PUBLIC) != CB_HD_KEY_TYPE_PUBLIC) {
		printf("CB_HD_KEY_VERSION_TEST_PUBLIC TYPE FAIL\n");
		return EXIT_FAILURE;
	}
	// Test HMAC-SHA512
	uint8_t hash[64];
	CBHDKeyHmacSha512((uint8_t [37]){0x2f, 0xf7, 0xd6, 0x9f, 0x7a, 0x59, 0x0b, 0xb0, 0x5e, 0x68, 0xd1, 0xdc, 0x0f, 0xcf, 0x8d, 0xc2, 0x17, 0x59, 0xc9, 0x39, 0xbb, 0x6b, 0x9b, 0x02, 0x0f, 0x65, 0x5d, 0x53, 0x85, 0x3c, 0xb5, 0xc2, 0x14, 0x61, 0x4b, 0x24, 0x42}, (uint8_t [32]){0xa2, 0x55, 0x21, 0xe3, 0xc5, 0x5b, 0x65, 0xd1, 0xcf, 0x25, 0x4b, 0x6c, 0x85, 0x23, 0xdc, 0xbf, 0x89, 0x46, 0x8d, 0x1f, 0x09, 0x1f, 0x15, 0x87, 0x6b, 0xbb, 0xc7, 0xfd, 0xd5, 0x44, 0x28, 0x43}, hash);
	if (memcmp(hash, (uint8_t [64]){0xfa, 0xa7, 0x9d, 0x85, 0xe0, 0xe4, 0x3d, 0xae, 0x8c, 0x3f, 0x99, 0xf0, 0x70, 0xdf, 0x97, 0x56, 0x2b, 0x3f, 0xbb, 0x17, 0x35, 0x20, 0xe0, 0x87, 0x32, 0xa6, 0x64, 0xca, 0xd4, 0x55, 0x0b, 0xbe, 0xc1, 0x11, 0xe5, 0xf8, 0x80, 0xdb, 0xb7, 0x3d, 0x67, 0x74, 0xbb, 0xc2, 0x9f, 0x67, 0xd9, 0x67, 0xaa, 0x10, 0xac, 0x60, 0x18, 0x90, 0x7f, 0x35, 0x53, 0xe3, 0x21, 0x38, 0xf6, 0x5b, 0xbe, 0x69}, 64) != 0) {
		printf("HMAC FAIL\n");
		return EXIT_FAILURE;
	}
	for (uint8_t x = 0; x < NUM_TEST_VECTORS; x++) {
		// Deserialise private key
		CBByteArray * masterString = CBNewByteArrayFromString(testVectors[x][0].privString, true);
		CBChecksumBytes * masterData = CBNewChecksumBytesFromString(masterString, false);
		CBReleaseObject(masterString);
		CBHDKey * key = CBNewHDKeyFromData(CBByteArrayGetData(CBGetByteArray(masterData)));
		CBReleaseObject(masterData);
		checkKey(key, x, 0);
		for (uint8_t y = 0; y < NUM_CHILDREN; y++) {
			if (testVectors[x][y+1].childID.priv == false) {
				// Derive public child and check public key is correct by address
				CBHDKey * newKey = CBNewHDKey(false);
				key->versionBytes = CB_HD_KEY_VERSION_PROD_PUBLIC;
				CBHDKeyDeriveChild(key, testVectors[x][y+1].childID, newKey);
				key->versionBytes = CB_HD_KEY_VERSION_PROD_PRIVATE;
				CBAddress * address = CBNewAddressFromRIPEMD160Hash(CBHDKeyGetHash(newKey), CB_PREFIX_PRODUCTION_ADDRESS, false);
				CBByteArray * str = CBChecksumBytesGetString(CBGetChecksumBytes(address));
				CBReleaseObject(address);
				if (memcmp(CBByteArrayGetData(str), testVectors[x][y + 1].addr, 34) != 0) {
					printf("ADDR FROM PUB FAIL AT %u - %u\n", x, y + 1);
					exit(EXIT_FAILURE);
				}
				CBReleaseObject(str);
				// Check serialisation of public key
				uint8_t * keyData = malloc(CB_HD_KEY_STR_SIZE);
				CBHDKeySerialise(newKey, keyData);
				CBChecksumBytes * checksumBytes = CBNewChecksumBytesFromBytes(keyData, 82, false);
				str = CBChecksumBytesGetString(checksumBytes);
				CBReleaseObject(checksumBytes);
				if (memcmp(CBByteArrayGetData(str), testVectors[x][y+1].pubString, 111) != 0) {
					printf("PUBLIC KEY STRING FROM PUB FAIL AT %u - %u\n", x, y);
					exit(EXIT_FAILURE);
				}
				CBReleaseObject(str);
				free(newKey);
			}
			// Derive private child
			CBHDKey * newKey = CBNewHDKey(true);
			CBHDKeyDeriveChild(key, testVectors[x][y+1].childID, newKey);
			free(key);
			key = newKey;
			checkKey(key, x, y+1);
		}
		free(key);
	}
	return EXIT_SUCCESS;
}
