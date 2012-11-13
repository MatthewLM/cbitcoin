//
//  testCBAssociativeArray.c
//  cbitcoin
//
//  Created by Matthew Mitchell on 12/11/2012.
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

#include "CBAssociativeArray.h"
#include <stdarg.h>
#include <stdio.h>
#include <time.h>

void onErrorReceived(char * format,...);
void onErrorReceived(char * format,...){
	va_list argptr;
    va_start(argptr, format);
    vfprintf(stderr, format, argptr);
    va_end(argptr);
	printf("\n");
}

int main(){
	unsigned int s = (unsigned int)time(NULL);
	printf("Session = %u\n",s);
	srand(s);
	CBAssociativeArray array;
	CBInitAssociativeArray(&array, 3, 3);
	uint8_t key[3];
	CBFindResult res = CBAssociativeArrayFind(&array, key);
	if (res.found) {
		printf("EMPTY TREE FOUND FAIL\n");
		return 1;
	}
	if (res.node != array.root) {
		printf("EMPTY TREE FIND ROOT FAIL\n");
		return 1;
	}
	if (res.pos) {
		printf("EMPTY TREE FIND POS FAIL\n");
		return 1;
	}
	// First try adding CB_BTREE_ORDER random values
	for (uint8_t x = 0; x < CB_BTREE_ORDER; x++) {
		key[0] = rand();
		key[1] = x;
		key[2] = rand();
		CBAssociativeArrayInsert(&array, key, key, CBAssociativeArrayFind(&array, key), NULL);
	}
	// Check in-order
	for (uint8_t x = 0; x < CB_BTREE_ORDER; x++) {
		uint8_t * key;
		key = (uint8_t *)(array.root + 1) + x*3;
		CBFindResult res = CBAssociativeArrayFind(&array, key);
		if (NOT res.found) {
			printf("ROOT FIND FOUND FAIL\n");
			return 1;
		}
		if (res.node != array.root) {
			printf("ROOT FIND NODE FAIL\n");
			return 1;
		}
		if (res.pos != x) {
			printf("ROOT FIND POS FAIL\n");
			return 1;
		}
		if (x) {
			if (memcmp(key, (uint8_t *)(array.root + 1) + x*3 - 3, 3) <= 0) {
				printf("ROOT FIND ORDER FAIL\n");
				return 1;
			}
		}
	}
	CBFreeAssociativeArray(&array);
	// Create array again and test for insertion overflow situations
	CBInitAssociativeArray(&array, 3, 3);
	// Insert CB_BTREE_ORDER elements
	for (uint8_t x = 0; x < CB_BTREE_ORDER; x++) {
		key[0] = x;
		key[1] = 126;
		key[2] = 0;
		CBAssociativeArrayInsert(&array, key, key, CBAssociativeArrayFind(&array, key), NULL);
	}
	// Try inserting value in the middle.
	key[0] = CB_BTREE_HALF_ORDER;
	key[1] = 0;
	key[2] = 0;
	CBAssociativeArrayInsert(&array, key, key, CBAssociativeArrayFind(&array, key), NULL);
	// Check the new root.
	res = CBAssociativeArrayFind(&array, key);
	if (res.node != array.root) {
		printf("INSERT MIDDLE SPLIT NODE FAIL\n");
		return 0;
	}
	if (NOT res.found) {
		printf("INSERT MIDDLE SPLIT FOUND FAIL\n");
		return 0;
	}
	if (res.pos) {
		printf("INSERT MIDDLE SPLIT POS FAIL\n");
		return 0;
	}
	if (memcmp((uint8_t *)(res.node + 1), (uint8_t *)(res.node + 1) + CB_BTREE_ORDER*3, 3)) {
		printf("INSERT MIDDLE SPLIT DATA FAIL\n");
		return 0;
	}
	// Check the split sides
	// Left side
	for (uint8_t x = 0; x < CB_BTREE_HALF_ORDER; x++) {
		key[0] = x;
		key[1] = 126;
		key[2] = 0;
		res = CBAssociativeArrayFind(&array, key);
		if (res.node != array.root->children[0]) {
			printf("LEFT CHILD NODE FAIL\n");
			return 0;
		}
		if (NOT res.found) {
			printf("LEFT CHILD FOUND FAIL\n");
			return 0;
		}
		if (res.pos != x) {
			printf("LEFT CHILD POS FAIL\n");
			return 0;
		}
		if (memcmp((uint8_t *)(res.node + 1) + x*3, (uint8_t *)(res.node + 1) + (CB_BTREE_ORDER + x)*3, 3)) {
			printf("LEFT CHILD DATA FAIL\n");
			return 0;
		}
	}
	// Right side
	for (uint8_t x = CB_BTREE_HALF_ORDER; x < CB_BTREE_ORDER; x++) {
		key[0] = x;
		key[1] = 126;
		key[2] = 0;
		res = CBAssociativeArrayFind(&array, key);
		if (res.node != array.root->children[1]) {
			printf("RIGHT CHILD NODE FAIL\n");
			return 0;
		}
		if (NOT res.found) {
			printf("RIGHT CHILD FOUND FAIL\n");
			return 0;
		}
		if (res.pos != x - CB_BTREE_HALF_ORDER) {
			printf("RIGHT CHILD POS FAIL\n");
			return 0;
		}
		if (memcmp((uint8_t *)(res.node + 1) + x*3, (uint8_t *)(res.node + 1) + (CB_BTREE_ORDER + x)*3, 3)) {
			printf("RIGHT CHILD DATA FAIL\n");
			return 0;
		}
	}
	// Insert 16 values to left child
	
	return 0;
}
