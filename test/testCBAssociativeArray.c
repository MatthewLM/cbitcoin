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
	// ??? Add more in-depth tests.
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
		return 1;
	}
	if (NOT res.found) {
		printf("INSERT MIDDLE SPLIT FOUND FAIL\n");
		return 1;
	}
	if (res.pos) {
		printf("INSERT MIDDLE SPLIT POS FAIL\n");
		return 1;
	}
	if (memcmp((uint8_t *)(res.node + 1), (uint8_t *)(res.node + 1) + CB_BTREE_ORDER*3, 3)) {
		printf("INSERT MIDDLE SPLIT DATA FAIL\n");
		return 1;
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
			return 1;
		}
		if (NOT res.found) {
			printf("LEFT CHILD FOUND FAIL\n");
			return 1;
		}
		if (res.pos != x) {
			printf("LEFT CHILD POS FAIL\n");
			return 1;
		}
		if (memcmp((uint8_t *)(res.node + 1) + x*3, (uint8_t *)(res.node + 1) + (CB_BTREE_ORDER + x)*3, 3)) {
			printf("LEFT CHILD DATA FAIL\n");
			return 1;
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
			return 1;
		}
		if (NOT res.found) {
			printf("RIGHT CHILD FOUND FAIL\n");
			return 1;
		}
		if (res.pos != x - CB_BTREE_HALF_ORDER) {
			printf("RIGHT CHILD POS FAIL\n");
			return 1;
		}
		if (memcmp((uint8_t *)(res.node + 1) + x*3, (uint8_t *)(res.node + 1) + (CB_BTREE_ORDER + x)*3, 3)) {
			printf("RIGHT CHILD DATA FAIL\n");
			return 1;
		}
	}
	// Insert 16 values to left child
	for (uint8_t x = 0; x < CB_BTREE_HALF_ORDER; x++) {
		key[0] = CB_BTREE_HALF_ORDER - 1;
		key[1] = 128 + x;
		key[2] = 0;
		CBAssociativeArrayInsert(&array, key, key, CBAssociativeArrayFind(&array, key), NULL);
	}
	// Add value to the right of the left child
	key[0] = CB_BTREE_HALF_ORDER - 1;
	key[1] = 128 + CB_BTREE_HALF_ORDER/2;
	key[2] = 127;
	CBAssociativeArrayInsert(&array, key, key, CBAssociativeArrayFind(&array, key), NULL);
	// Now check root
	key[0] = CB_BTREE_HALF_ORDER - 1;
	key[1] = 128;
	key[2] = 0;
	res = CBAssociativeArrayFind(&array, key);
	if (res.node != array.root) {
		printf("INSERT RIGHT SPLIT NODE FAIL\n");
		return 1;
	}
	if (NOT res.found) {
		printf("INSERT RIGHT SPLIT FOUND FAIL\n");
		return 1;
	}
	if (res.pos) {
		printf("INSERT RIGHT SPLIT POS FAIL\n");
		return 1;
	}
	if (memcmp((uint8_t *)(res.node + 1), (uint8_t *)(res.node + 1) + CB_BTREE_ORDER*3, 3)) {
		printf("INSERT RIGHT SPLIT DATA FAIL\n");
		return 1;
	}
	// Check left child
	for (uint8_t x = 0; x < CB_BTREE_HALF_ORDER; x++) {
		key[0] = x;
		key[1] = 126;
		key[2] = 0;
		res = CBAssociativeArrayFind(&array, key);
		if (res.node != array.root->children[0]) {
			printf("RIGHT SPLIT LEFT CHILD NODE FAIL\n");
			return 1;
		}
		if (NOT res.found) {
			printf("RIGHT SPLIT LEFT CHILD FOUND FAIL\n");
			return 1;
		}
		if (res.pos != x) {
			printf("RIGHT SPLIT LEFT CHILD POS FAIL\n");
			return 1;
		}
		if (memcmp((uint8_t *)(res.node + 1) + x*3, (uint8_t *)(res.node + 1) + (CB_BTREE_ORDER + x)*3, 3)) {
			printf("RIGHT SPLIT LEFT CHILD DATA FAIL\n");
			return 1;
		}
	}
	// Check middle child
	for (uint8_t x = 0; x < CB_BTREE_HALF_ORDER; x++) {
		key[0] = CB_BTREE_HALF_ORDER - 1;
		key[1] = 128 + x + 1 - ((x >= CB_BTREE_HALF_ORDER/2) ? 1 : 0);
		key[2] = (x == CB_BTREE_HALF_ORDER/2) ? 127 : 0;
		res = CBAssociativeArrayFind(&array, key);
		if (res.node != array.root->children[1]) {
			printf("RIGHT SPLIT MID CHILD NODE FAIL\n");
			return 1;
		}
		if (NOT res.found) {
			printf("RIGHT SPLIT MID CHILD FOUND FAIL\n");
			return 1;
		}
		if (res.pos != x) {
			printf("RIGHT SPLIT MID CHILD POS FAIL\n");
			return 1;
		}
		if (memcmp((uint8_t *)(res.node + 1) + x*3, (uint8_t *)(res.node + 1) + (CB_BTREE_ORDER + x)*3, 3)) {
			printf("RIGHT SPLIT MID CHILD DATA FAIL\n");
			return 1;
		}
	}
	// Insert 16 values to right child
	for (uint8_t x = 0; x < CB_BTREE_HALF_ORDER; x++) {
		key[0] = CB_BTREE_ORDER;
		key[1] = x;
		key[2] = 0;
		CBAssociativeArrayInsert(&array, key, key, CBAssociativeArrayFind(&array, key), NULL);
	}
	// Add value to left side of right child
	key[0] = (CB_BTREE_ORDER*3)/4;
	key[1] = 125;
	key[2] = 0;
	CBAssociativeArrayInsert(&array, key, key, CBAssociativeArrayFind(&array, key), NULL);
	// Check root
	key[0] = CB_BTREE_ORDER - 1;
	key[1] = 126;
	key[2] = 0;
	res = CBAssociativeArrayFind(&array, key);
	if (res.node != array.root) {
		printf("INSERT LEFT SPLIT NODE FAIL\n");
		return 1;
	}
	if (NOT res.found) {
		printf("INSERT LEFT SPLIT FOUND FAIL\n");
		return 1;
	}
	if (res.pos != 2) {
		printf("INSERT LEFT SPLIT POS FAIL\n");
		return 1;
	}
	if (memcmp((uint8_t *)(res.node + 1) + 6, (uint8_t *)(res.node + 1) + CB_BTREE_ORDER*3 + 6, 3)) {
		printf("INSERT LEFT SPLIT DATA FAIL\n");
		return 1;
	}
	// Check 3rd child
	for (uint8_t x = 0; x < CB_BTREE_HALF_ORDER; x++) {
		key[0] = CB_BTREE_HALF_ORDER + x - ((x > CB_BTREE_HALF_ORDER/2) ? 1 : 0);
		key[1] = 126 - ((x == CB_BTREE_HALF_ORDER/2) ? 1 : 0);
		key[2] = 0;
		res = CBAssociativeArrayFind(&array, key);
		if (res.node != array.root->children[2]) {
			printf("LEFT SPLIT 3RD CHILD NODE FAIL\n");
			return 1;
		}
		if (NOT res.found) {
			printf("LEFT SPLIT 3RD CHILD FOUND FAIL\n");
			return 1;
		}
		if (res.pos != x) {
			printf("LEFT SPLIT 3RD CHILD POS FAIL\n");
			return 1;
		}
		if (memcmp((uint8_t *)(res.node + 1) + x*3, (uint8_t *)(res.node + 1) + (CB_BTREE_ORDER + x)*3, 3)) {
			printf("LEFT SPLIT 3RD CHILD DATA FAIL\n");
			return 1;
		}
	}
	// Check 4th child
	for (uint8_t x = 0; x < CB_BTREE_HALF_ORDER; x++) {
		key[0] = CB_BTREE_ORDER;
		key[1] = x;
		key[2] = 0;
		res = CBAssociativeArrayFind(&array, key);
		if (res.node != array.root->children[3]) {
			printf("LEFT SPLIT 4TH CHILD NODE FAIL\n");
			return 1;
		}
		if (NOT res.found) {
			printf("LEFT SPLIT 4TH CHILD FOUND FAIL\n");
			return 1;
		}
		if (res.pos != x) {
			printf("LEFT SPLIT 4TH CHILD POS FAIL\n");
			return 1;
		}
		if (memcmp((uint8_t *)(res.node + 1) + x*3, (uint8_t *)(res.node + 1) + (CB_BTREE_ORDER + x)*3, 3)) {
			printf("LEFT SPLIT 4TH CHILD DATA FAIL\n");
			return 1;
		}
	}
	return 0;
}
