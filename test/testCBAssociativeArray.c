//
//  testCBAssociativeArray.c
//  cbitcoin
//
//  Created by Matthew Mitchell on 12/11/2012.
//  Copyright (c) 2012 Matthew Mitchell
//
//  This file is part of cbitcoin. It is subject to the license terms
//  in the LICENSE file found in the top-level directory of this
//  distribution and at http://www.cbitcoin.com/license.html. No part of
//  cbitcoin, including this file, may be copied, modified, propagated,
//  or distributed except according to the terms contained in the
//  LICENSE file.

#include "CBAssociativeArray.h"
#include <stdarg.h>
#include <stdio.h>
#include <time.h>
#include <math.h>

void onErrorReceived(char * format, ...);
void onErrorReceived(char * format, ...){
	va_list argptr;
    va_start(argptr, format);
    vfprintf(stderr, format, argptr);
    va_end(argptr);
	printf("\n");
}

uint32_t getLen(CBBTreeNode * self);
uint32_t getLen(CBBTreeNode * self){
	uint32_t len = self->numElements;
	if (self->children[0])
		for (uint8_t x = 0; x < self->numElements + 1; x++)
			len += getLen(self->children[x]);
	return len;
}

int main(){
	// ??? CB_BTREE_ELEMENTS set to 2 gives best results???
	// ??? Add more in-depth tests.
	unsigned int s = (unsigned int)time(NULL);
	printf("Session = %u\n", s);
	srand(s);
	s = 1384284162;
	CBAssociativeArray array;
	CBInitAssociativeArray(&array, CBKeyCompare, NULL, NULL);
	uint8_t key[4];
	CBFindResult res = CBAssociativeArrayFind(&array, key);
	if (res.found) {
		printf("EMPTY TREE FOUND FAIL\n");
		return 1;
	}
	if (res.position.node != array.root) {
		printf("EMPTY TREE FIND ROOT FAIL\n");
		return 1;
	}
	if (res.position.index) {
		printf("EMPTY TREE FIND POS FAIL\n");
		return 1;
	}
	// First try adding CB_BTREE_ELEMENTS random values
	uint8_t keys[CB_BTREE_ELEMENTS][4];
	for (uint8_t x = 0; x < CB_BTREE_ELEMENTS; x++) {
		keys[x][0] = 3;
		keys[x][1] = rand();
		keys[x][2] = x;
		keys[x][3] = rand();
		CBAssociativeArrayInsert(&array, keys[x], CBAssociativeArrayFind(&array, keys[x]).position, NULL);
	}
	// Check in-order
	for (uint8_t x = 0; x < CB_BTREE_ELEMENTS; x++) {
		CBFindResult res2 = CBAssociativeArrayFind(&array, array.root->elements[x]);
		if (! res2.found) {
			printf("ROOT FIND FOUND FAIL\n");
			return 1;
		}
		if (res2.position.node != array.root) {
			printf("ROOT FIND NODE FAIL\n");
			return 1;
		}
		if (res2.position.index != x) {
			printf("ROOT FIND POS FAIL\n");
			return 1;
		}
		if (x) {
			if (memcmp((uint8_t *)array.root->elements[x] + 1, (uint8_t *)array.root->elements[x-1] + 1, 3) <= 0) {
				printf("ROOT FIND ORDER FAIL\n");
				return 1;
			}
		}
	}
	CBFreeAssociativeArray(&array);
	// Create array again and test for insertion overflow situations
	CBInitAssociativeArray(&array, CBKeyCompare, NULL, NULL);
	// Insert CB_BTREE_ELEMENTS elements
	uint8_t keys2[CB_BTREE_ELEMENTS+1][4];
	for (uint8_t x = 0; x < CB_BTREE_ELEMENTS; x++) {
		keys2[x][0] = 3;
		keys2[x][1] = x;
		keys2[x][2] = 126;
		keys2[x][3] = 0;
		CBAssociativeArrayInsert(&array, keys2[x], CBAssociativeArrayFind(&array, keys2[x]).position, NULL);
	}
	// Try inserting value in the middle.
	keys2[CB_BTREE_ELEMENTS][0] = 3;
	keys2[CB_BTREE_ELEMENTS][1] = CB_BTREE_HALF_ELEMENTS;
	keys2[CB_BTREE_ELEMENTS][2] = 0;
	keys2[CB_BTREE_ELEMENTS][3] = 0;
	CBAssociativeArrayInsert(&array, keys2[CB_BTREE_ELEMENTS], CBAssociativeArrayFind(&array, keys2[CB_BTREE_ELEMENTS]).position, NULL);
	// Check the new root.
	res = CBAssociativeArrayFind(&array, keys2[CB_BTREE_ELEMENTS]);
	if (res.position.node != array.root) {
		printf("INSERT MIDDLE SPLIT NODE FAIL\n");
		return 1;
	}
	if (! res.found) {
		printf("INSERT MIDDLE SPLIT FOUND FAIL\n");
		return 1;
	}
	if (res.position.index) {
		printf("INSERT MIDDLE SPLIT POS FAIL\n");
		return 1;
	}
	// Check the split sides
	// Left side
	key[0] = 3;
	for (uint8_t x = 0; x < CB_BTREE_HALF_ELEMENTS; x++) {
		key[1] = x;
		key[2] = 126;
		key[3] = 0;
		res = CBAssociativeArrayFind(&array, key);
		if (res.position.node != array.root->children[0]) {
			printf("LEFT CHILD NODE FAIL\n");
			return 1;
		}
		if (! res.found) {
			printf("LEFT CHILD FOUND FAIL\n");
			return 1;
		}
		if (res.position.index != x) {
			printf("LEFT CHILD POS FAIL\n");
			return 1;
		}
	}
	// Right side
	for (uint8_t x = CB_BTREE_HALF_ELEMENTS; x < CB_BTREE_ELEMENTS; x++) {
		key[1] = x;
		key[2] = 126;
		key[3] = 0;
		res = CBAssociativeArrayFind(&array, key);
		if (res.position.node != array.root->children[1]) {
			printf("RIGHT CHILD NODE FAIL\n");
			return 1;
		}
		if (! res.found) {
			printf("RIGHT CHILD FOUND FAIL\n");
			return 1;
		}
		if (res.position.index != x - CB_BTREE_HALF_ELEMENTS) {
			printf("RIGHT CHILD POS FAIL\n");
			return 1;
		}
	}
	// Insert 16 values to left child
	uint8_t keys3[CB_BTREE_HALF_ELEMENTS + 1][4];
	for (uint8_t x = 0; x < CB_BTREE_HALF_ELEMENTS; x++) {
		keys3[x][0] = 3;
		keys3[x][1] = CB_BTREE_HALF_ELEMENTS - 1;
		keys3[x][2] = 128 + x;
		keys3[x][3] = 0;
		CBAssociativeArrayInsert(&array, keys3[x], CBAssociativeArrayFind(&array, keys3[x]).position, NULL);
	}
	// Add value to the right of the left child
	keys3[CB_BTREE_HALF_ELEMENTS][0] = 3;
	keys3[CB_BTREE_HALF_ELEMENTS][1] = CB_BTREE_HALF_ELEMENTS - 1;
	keys3[CB_BTREE_HALF_ELEMENTS][2] = 128 + CB_BTREE_HALF_ELEMENTS/2;
	keys3[CB_BTREE_HALF_ELEMENTS][3] = 127;
	CBFindResult res2 = CBAssociativeArrayFind(&array, keys3[CB_BTREE_HALF_ELEMENTS]);
	CBAssociativeArrayInsert(&array, keys3[CB_BTREE_HALF_ELEMENTS], res2.position, NULL);
	// Now check root
	key[1] = CB_BTREE_HALF_ELEMENTS - 1;
	key[2] = 128;
	key[3] = 0;
	res = CBAssociativeArrayFind(&array, key);
	if (res.position.node != array.root) {
		printf("INSERT RIGHT SPLIT NODE FAIL\n");
		return 1;
	}
	if (! res.found) {
		printf("INSERT RIGHT SPLIT FOUND FAIL\n");
		return 1;
	}
	if (res.position.index) {
		printf("INSERT RIGHT SPLIT POS FAIL\n");
		return 1;
	}
	// Check left child
	for (uint8_t x = 0; x < CB_BTREE_HALF_ELEMENTS; x++) {
		key[1] = x;
		key[2] = 126;
		key[3] = 0;
		res = CBAssociativeArrayFind(&array, key);
		if (res.position.node != array.root->children[0]) {
			printf("RIGHT SPLIT LEFT CHILD NODE FAIL\n");
			return 1;
		}
		if (! res.found) {
			printf("RIGHT SPLIT LEFT CHILD FOUND FAIL\n");
			return 1;
		}
		if (res.position.index != x) {
			printf("RIGHT SPLIT LEFT CHILD POS FAIL\n");
			return 1;
		}
	}
	// Check middle child
	for (uint8_t x = 0; x < CB_BTREE_HALF_ELEMENTS; x++) {
		key[1] = CB_BTREE_HALF_ELEMENTS - 1;
		key[2] = 128 + x + 1 - ((x >= CB_BTREE_HALF_ELEMENTS/2) ? 1 : 0);
		key[3] = (x == CB_BTREE_HALF_ELEMENTS/2) ? 127 : 0;
		res = CBAssociativeArrayFind(&array, key);
		if (res.position.node != array.root->children[1]) {
			printf("RIGHT SPLIT MID CHILD NODE FAIL\n");
			return 1;
		}
		if (! res.found) {
			printf("RIGHT SPLIT MID CHILD FOUND FAIL\n");
			return 1;
		}
		if (res.position.index != x) {
			printf("RIGHT SPLIT MID CHILD POS FAIL\n");
			return 1;
		}
	}
	// Insert 16 values to right child
	uint8_t keys4[CB_BTREE_HALF_ELEMENTS + 1][4];
	for (uint8_t x = 0; x < CB_BTREE_HALF_ELEMENTS; x++) {
		keys4[x][0] = 3;
		keys4[x][1] = CB_BTREE_ELEMENTS;
		keys4[x][2] = x;
		keys4[x][3] = 0;
		CBAssociativeArrayInsert(&array, keys4[x], CBAssociativeArrayFind(&array, keys4[x]).position, NULL);
	}
	// Add value to left side of right child
	keys4[CB_BTREE_HALF_ELEMENTS][0] = 3;
	keys4[CB_BTREE_HALF_ELEMENTS][1] = (CB_BTREE_ELEMENTS*3)/4;
	keys4[CB_BTREE_HALF_ELEMENTS][2] = 125;
	keys4[CB_BTREE_HALF_ELEMENTS][3] = 0;
	CBAssociativeArrayInsert(&array, keys4[CB_BTREE_HALF_ELEMENTS], CBAssociativeArrayFind(&array, keys4[CB_BTREE_HALF_ELEMENTS]).position, NULL);
	// Check root
	key[1] = CB_BTREE_ELEMENTS - 1;
	key[2] = 126;
	key[3] = 0;
	res = CBAssociativeArrayFind(&array, key);
	if (res.position.node != array.root) {
		printf("INSERT LEFT SPLIT NODE FAIL\n");
		return 1;
	}
	if (! res.found) {
		printf("INSERT LEFT SPLIT FOUND FAIL\n");
		return 1;
	}
	if (res.position.index != 2) {
		printf("INSERT LEFT SPLIT POS FAIL\n");
		return 1;
	}
	// Check 3rd child
	for (uint8_t x = 0; x < CB_BTREE_HALF_ELEMENTS; x++) {
		key[1] = CB_BTREE_HALF_ELEMENTS + x - ((x > CB_BTREE_HALF_ELEMENTS/2) ? 1 : 0);
		key[2] = 126 - ((x == CB_BTREE_HALF_ELEMENTS/2) ? 1 : 0);
		key[3] = 0;
		res = CBAssociativeArrayFind(&array, key);
		if (res.position.node != array.root->children[2]) {
			printf("LEFT SPLIT 3RD CHILD NODE FAIL\n");
			return 1;
		}
		if (! res.found) {
			printf("LEFT SPLIT 3RD CHILD FOUND FAIL\n");
			return 1;
		}
		if (res.position.index != x) {
			printf("LEFT SPLIT 3RD CHILD POS FAIL\n");
			return 1;
		}
	}
	// Check 4th child
	for (uint8_t x = 0; x < CB_BTREE_HALF_ELEMENTS; x++) {
		key[1] = CB_BTREE_ELEMENTS;
		key[2] = x;
		key[3] = 0;
		res = CBAssociativeArrayFind(&array, key);
		if (res.position.node != array.root->children[3]) {
			printf("LEFT SPLIT 4TH CHILD NODE FAIL\n");
			return 1;
		}
		if (! res.found) {
			printf("LEFT SPLIT 4TH CHILD FOUND FAIL\n");
			return 1;
		}
		if (res.position.index != x) {
			printf("LEFT SPLIT 4TH CHILD POS FAIL\n");
			return 1;
		}
	}
	// Test deletions
	// Here is the tree as it is:
	//
	// Root = [15, 128, 0] , [16, 0, 0] , [31, 126, 0]
	//
	// Root child 0 = [0, 126, 0] , [1, 126, 0] , [2, 126, 0] , [3, 126, 0] , [4, 126, 0] , [5, 126, 0] , [6, 126, 0] , [7, 126, 0] , [8, 126, 0] , [9, 126, 0] , [10, 126, 0] , [11, 126, 0] , [12, 126, 0] , [13, 126, 0] , [14, 126, 0] , [15, 126, 0] , 
	//
	// Root child 1 = [15, 129, 0] , [15, 130, 0] , [15, 131, 0] , [15, 132, 0] , [15, 133, 0] , [15, 134, 0] , [15, 135, 0] , [15, 136, 0] , [15, 136, 127] , [15, 137, 0] , [15, 138, 0] , [15, 139, 0] , [15, 140, 0] , [15, 141, 0] , [15, 142, 0] , [15, 143, 0]
	//
	// Root child 2 = [16, 126, 0] , [17, 126, 0] , [18, 126, 0] , [19, 126, 0] , [20, 126, 0] , [21, 126, 0] , [22, 126, 0] , [23, 126, 0] , [24, 125, 0] , [24, 126, 0] , [25, 126, 0] , [26, 126, 0] , [27, 126, 0] , [28, 126, 0] , [29, 126, 0] , [30, 126, 0] , 
	//
	// Root child 3 = [32, 0, 0] , [32, 1, 0] , [32, 2, 0] , [32, 3, 0] , [32, 4, 0] , [32, 5, 0] , [32, 6, 0] , [32, 7, 0] , [32, 8, 0] , [32, 9, 0] , [32, 10, 0] , [32, 11, 0] , [32, 12, 0] , [32, 13, 0] , [32, 14, 0] , [32, 15, 0]
	//
	// Test deletion from leaf causing merge. Try [1, 126, 0]
	key[1] = 1;
	key[2] = 126;
	key[3] = 0;
	CBAssociativeArrayDelete(&array, CBAssociativeArrayFind(&array, key).position, false);
	// Child 0 should be merged with child 1. Check new root.
	if (array.root->numElements != 2) {
		printf("REMOVE [1, 126, 0] ROOT NUM ELEMENTS FAIL\n");
		return 1;
	}
	if (memcmp(array.root->elements[0], (uint8_t [4]){3, 16, 0, 0}, 4)
		&& memcmp(array.root->elements[1], (uint8_t [4]){3, 31, 126, 0}, 4)) {
		printf("REMOVE [1, 126, 0] ROOT DATA FAIL\n");
		return 1;
	}
	// Check new child 0
	CBBTreeNode * child0 = array.root->children[0];
	if (child0->numElements != CB_BTREE_ELEMENTS) {
		printf("REMOVE [1, 126, 0] CHILD 0 NUM ELEMENTS FAIL\n");
		return 1;
	}
	if (memcmp(child0->elements[0], (uint8_t [4]){3, 0, 126, 0}, 4)
		&& memcmp(child0->elements[1], (uint8_t [4]){3, 2, 126, 0}, 4)
		&& memcmp(child0->elements[2], (uint8_t [4]){3, 3, 126, 0}, 4)
		&& memcmp(child0->elements[3], (uint8_t [4]){3, 4, 126, 0}, 4)
		&& memcmp(child0->elements[4], (uint8_t [4]){3, 5, 126, 0}, 4)
		&& memcmp(child0->elements[5], (uint8_t [4]){3, 6, 126, 0}, 4)
		&& memcmp(child0->elements[6], (uint8_t [4]){3, 7, 126, 0}, 4)
		&& memcmp(child0->elements[7], (uint8_t [4]){3, 8, 126, 0}, 4)
		&& memcmp(child0->elements[8], (uint8_t [4]){3, 9, 126, 0}, 4)
		&& memcmp(child0->elements[9], (uint8_t [4]){3, 10, 126, 0}, 4)
		&& memcmp(child0->elements[10], (uint8_t [4]){3, 11, 126, 0}, 4)
		&& memcmp(child0->elements[11], (uint8_t [4]){3, 12, 126, 0}, 4)
		&& memcmp(child0->elements[12], (uint8_t [4]){3, 13, 126, 0}, 4)
		&& memcmp(child0->elements[13], (uint8_t [4]){3, 14, 126, 0}, 4)
		&& memcmp(child0->elements[14], (uint8_t [4]){3, 15, 126, 0}, 4)
		&& memcmp(child0->elements[15], (uint8_t [4]){3, 15, 128, 0}, 4)
		&& memcmp(child0->elements[16], (uint8_t [4]){3, 15, 129, 0}, 4)
		&& memcmp(child0->elements[17], (uint8_t [4]){3, 15, 130, 0}, 4)
		&& memcmp(child0->elements[18], (uint8_t [4]){3, 15, 131, 0}, 4)
		&& memcmp(child0->elements[19], (uint8_t [4]){3, 15, 132, 0}, 4)
		&& memcmp(child0->elements[20], (uint8_t [4]){3, 15, 133, 0}, 4)
		&& memcmp(child0->elements[21], (uint8_t [4]){3, 15, 134, 0}, 4)
		&& memcmp(child0->elements[22], (uint8_t [4]){3, 15, 135, 0}, 4)
		&& memcmp(child0->elements[23], (uint8_t [4]){3, 15, 136, 0}, 4)
		&& memcmp(child0->elements[24], (uint8_t [4]){3, 15, 136, 127}, 4)
		&& memcmp(child0->elements[25], (uint8_t [4]){3, 15, 137, 0}, 4)
		&& memcmp(child0->elements[26], (uint8_t [4]){3, 15, 138, 0}, 4)
		&& memcmp(child0->elements[27], (uint8_t [4]){3, 15, 139, 0}, 4)
		&& memcmp(child0->elements[28], (uint8_t [4]){3, 15, 140, 0}, 4)
		&& memcmp(child0->elements[29], (uint8_t [4]){3, 15, 141, 0}, 4)
		&& memcmp(child0->elements[30], (uint8_t [4]){3, 15, 142, 0}, 4)
		&& memcmp(child0->elements[31], (uint8_t [4]){3, 15, 143, 0}, 4)) {
		printf("REMOVE [1, 126, 0] CHILD 0 DATA FAIL\n");
		return 1;
	}
	// Now test merge of child 2 with child 1. Try removing [32, 15, 0]
	key[1] = 32;
	key[2] = 15;
	key[3] = 0;
	CBAssociativeArrayDelete(&array, CBAssociativeArrayFind(&array, key).position, false);
	// Check root
	if (array.root->numElements != 1) {
		printf("REMOVE [32, 15, 0] ROOT NUM ELEMENTS FAIL\n");
		return 1;
	}
	if (memcmp(array.root->elements[0], (uint8_t [4]){3, 16, 0, 0}, 4)) {
		printf("REMOVE [32, 15, 0] ROOT DATA FAIL\n");
		return 1;
	}
	// Check new child 1
	CBBTreeNode * child1 = array.root->children[1];
	if (child1->numElements != CB_BTREE_ELEMENTS) {
		printf("REMOVE [32, 15, 0] CHILD 1 NUM ELEMENTS FAIL\n");
		return 1;
	}
	if (memcmp(child1->elements[0], (uint8_t [4]){3, 16, 126, 0}, 4)
		&& memcmp(child1->elements[1], (uint8_t [4]){3, 17, 126, 0}, 4)
		&& memcmp(child1->elements[2], (uint8_t [4]){3, 18, 126, 0}, 4)
		&& memcmp(child1->elements[3], (uint8_t [4]){3, 19, 126, 0}, 4)
		&& memcmp(child1->elements[4], (uint8_t [4]){3, 20, 126, 0}, 4)
		&& memcmp(child1->elements[5], (uint8_t [4]){3, 21, 126, 0}, 4)
		&& memcmp(child1->elements[6], (uint8_t [4]){3, 22, 126, 0}, 4)
		&& memcmp(child1->elements[7], (uint8_t [4]){3, 23, 126, 0}, 4)
		&& memcmp(child1->elements[8], (uint8_t [4]){3, 24, 125, 0}, 4)
		&& memcmp(child1->elements[9], (uint8_t [4]){3, 24, 126, 0}, 4)
		&& memcmp(child1->elements[10], (uint8_t [4]){3, 25, 126, 0}, 4)
		&& memcmp(child1->elements[11], (uint8_t [4]){3, 26, 126, 0}, 4)
		&& memcmp(child1->elements[12], (uint8_t [4]){3, 27, 126, 0}, 4)
		&& memcmp(child1->elements[13], (uint8_t [4]){3, 28, 126, 0}, 4)
		&& memcmp(child1->elements[14], (uint8_t [4]){3, 29, 126, 0}, 4)
		&& memcmp(child1->elements[15], (uint8_t [4]){3, 30, 128, 0}, 4)
		&& memcmp(child1->elements[16], (uint8_t [4]){3, 31, 129, 0}, 4)
		&& memcmp(child1->elements[17], (uint8_t [4]){3, 32, 0, 0}, 4)
		&& memcmp(child1->elements[18], (uint8_t [4]){3, 32, 1, 0}, 4)
		&& memcmp(child1->elements[19], (uint8_t [4]){3, 32, 2, 0}, 4)
		&& memcmp(child1->elements[20], (uint8_t [4]){3, 32, 3, 0}, 4)
		&& memcmp(child1->elements[21], (uint8_t [4]){3, 32, 4, 0}, 4)
		&& memcmp(child1->elements[22], (uint8_t [4]){3, 32, 5, 0}, 4)
		&& memcmp(child1->elements[23], (uint8_t [4]){3, 32, 6, 0}, 4)
		&& memcmp(child1->elements[24], (uint8_t [4]){3, 32, 7, 0}, 4)
		&& memcmp(child1->elements[25], (uint8_t [4]){3, 32, 8, 0}, 4)
		&& memcmp(child1->elements[26], (uint8_t [4]){3, 32, 9, 0}, 4)
		&& memcmp(child1->elements[27], (uint8_t [4]){3, 32, 10, 0}, 4)
		&& memcmp(child1->elements[28], (uint8_t [4]){3, 32, 11, 0}, 4)
		&& memcmp(child1->elements[29], (uint8_t [4]){3, 32, 12, 0}, 4)
		&& memcmp(child1->elements[30], (uint8_t [4]){3, 32, 13, 0}, 4)
		&& memcmp(child1->elements[31], (uint8_t [4]){3, 32, 14, 0}, 4)) {
		printf("REMOVE [32, 15, 0] CHILD 1 DATA FAIL\n");
		return 1;
	}
	// Remove 17 elements from child1 to test taking element from left.
	for (uint8_t x = 0; x < CB_BTREE_HALF_ELEMENTS + 1; x++) {
		key[1] = CB_BTREE_HALF_ELEMENTS + x - ((x > 8)? 1 : 0);
		key[2] = 126 - ((x == 8)? 1 : 0);
		key[3] = 0;
		CBAssociativeArrayDelete(&array, CBAssociativeArrayFind(&array, key).position, false);
	}
	// Check child 0
	if (child0->numElements != CB_BTREE_ELEMENTS - 1) {
		printf("TAKE FROM LEFT CHILD 0 NUM ELEMENTS FAIL\n");
		return 1;
	}
	if (memcmp(child0->elements[0], (uint8_t [4]){3, 0, 126, 0}, 4)
		&& memcmp(child0->elements[1], (uint8_t [4]){3, 2, 126, 0}, 4)
		&& memcmp(child0->elements[2], (uint8_t [4]){3, 3, 126, 0}, 4)
		&& memcmp(child0->elements[3], (uint8_t [4]){3, 4, 126, 0}, 4)
		&& memcmp(child0->elements[4], (uint8_t [4]){3, 5, 126, 0}, 4)
		&& memcmp(child0->elements[5], (uint8_t [4]){3, 6, 126, 0}, 4)
		&& memcmp(child0->elements[6], (uint8_t [4]){3, 7, 126, 0}, 4)
		&& memcmp(child0->elements[7], (uint8_t [4]){3, 8, 126, 0}, 4)
		&& memcmp(child0->elements[8], (uint8_t [4]){3, 9, 125, 0}, 4)
		&& memcmp(child0->elements[9], (uint8_t [4]){3, 10, 126, 0}, 4)
		&& memcmp(child0->elements[10], (uint8_t [4]){3, 11, 126, 0}, 4)
		&& memcmp(child0->elements[11], (uint8_t [4]){3, 12, 126, 0}, 4)
		&& memcmp(child0->elements[12], (uint8_t [4]){3, 13, 126, 0}, 4)
		&& memcmp(child0->elements[13], (uint8_t [4]){3, 14, 126, 0}, 4)
		&& memcmp(child0->elements[14], (uint8_t [4]){3, 15, 126, 0}, 4)
		&& memcmp(child0->elements[15], (uint8_t [4]){3, 15, 128, 0}, 4)
		&& memcmp(child0->elements[16], (uint8_t [4]){3, 15, 129, 0}, 4)
		&& memcmp(child0->elements[17], (uint8_t [4]){3, 15, 130, 0}, 4)
		&& memcmp(child0->elements[18], (uint8_t [4]){3, 15, 131, 0}, 4)
		&& memcmp(child0->elements[19], (uint8_t [4]){3, 15, 132, 0}, 4)
		&& memcmp(child0->elements[20], (uint8_t [4]){3, 15, 133, 0}, 4)
		&& memcmp(child0->elements[21], (uint8_t [4]){3, 15, 134, 0}, 4)
		&& memcmp(child0->elements[22], (uint8_t [4]){3, 15, 135, 0}, 4)
		&& memcmp(child0->elements[23], (uint8_t [4]){3, 15, 136, 0}, 4)
		&& memcmp(child0->elements[24], (uint8_t [4]){3, 15, 136, 127}, 4)
		&& memcmp(child0->elements[25], (uint8_t [4]){3, 15, 137, 0}, 4)
		&& memcmp(child0->elements[26], (uint8_t [4]){3, 15, 138, 0}, 4)
		&& memcmp(child0->elements[27], (uint8_t [4]){3, 15, 139, 0}, 4)
		&& memcmp(child0->elements[28], (uint8_t [4]){3, 15, 140, 0}, 4)
		&& memcmp(child0->elements[29], (uint8_t [4]){3, 15, 141, 0}, 4)
		&& memcmp(child0->elements[30], (uint8_t [4]){3, 15, 142, 0}, 4)) {
		printf("TAKE FROM LEFT CHILD 0 DATA FAIL\n");
		return 1;
	}
	// Check root
	if (array.root->numElements != 1) {
		printf("TAKE FROM LEFT ROOT NUM ELEMENTS FAIL\n");
		return 1;
	}
	if (memcmp(array.root->elements[0], (uint8_t [4]){3, 15, 143, 0}, 4)) {
		printf("TAKE FROM LEFT ROOT DATA FAIL\n");
		return 1;
	}
	// Check child 1
	if (child1->numElements != CB_BTREE_HALF_ELEMENTS) {
		printf("TAKE FROM LEFT CHILD 1 NUM ELEMENTS FAIL\n");
		return 1;
	}
	if (memcmp(child1->elements[0], (uint8_t [4]){3, 16, 0, 0}, 4)
		&& memcmp(child1->elements[1], (uint8_t [4]){3, 32, 0, 0}, 4)
		&& memcmp(child1->elements[2], (uint8_t [4]){3, 32, 1, 0}, 4)
		&& memcmp(child1->elements[3], (uint8_t [4]){3, 32, 2, 0}, 4)
		&& memcmp(child1->elements[4], (uint8_t [4]){3, 32, 3, 0}, 4)
		&& memcmp(child1->elements[5], (uint8_t [4]){3, 32, 4, 0}, 4)
		&& memcmp(child1->elements[6], (uint8_t [4]){3, 32, 5, 0}, 4)
		&& memcmp(child1->elements[7], (uint8_t [4]){3, 32, 6, 0}, 4)
		&& memcmp(child1->elements[8], (uint8_t [4]){3, 32, 7, 0}, 4)
		&& memcmp(child1->elements[9], (uint8_t [4]){3, 32, 8, 0}, 4)
		&& memcmp(child1->elements[10], (uint8_t [4]){3, 32, 9, 0}, 4)
		&& memcmp(child1->elements[11], (uint8_t [4]){3, 32, 10, 0}, 4)
		&& memcmp(child1->elements[12], (uint8_t [4]){3, 32, 11, 0}, 4)
		&& memcmp(child1->elements[13], (uint8_t [4]){3, 32, 12, 0}, 4)
		&& memcmp(child1->elements[14], (uint8_t [4]){3, 32, 13, 0}, 4)
		&& memcmp(child1->elements[15], (uint8_t [4]){3, 32, 14, 0}, 4)) {
		printf("TAKE FROM LEFT CHILD 1 DATA FAIL\n");
		return 1;
	}
	// Now test taking from right
	uint8_t key2[4];
	key2[0] = 3;
	key2[1] = 15;
	key2[2] = 144;
	key2[3] = 0;
	CBAssociativeArrayInsert(&array, key2, CBAssociativeArrayFind(&array, key2).position, NULL);
	// Remove 16 elements from child0 to test taking element from right.
	for (uint8_t x = 0; x < CB_BTREE_HALF_ELEMENTS; x++) {
		key[1] = 15;
		key[2] = 128 + x - ((x > 8) ? 1 : 0);
		key[3] = (x == 8) ? 127 : 0;
		CBAssociativeArrayDelete(&array, CBAssociativeArrayFind(&array, key).position, false);
	}
	// Check root
	if (array.root->numElements != 1) {
		printf("TAKE FROM RIGHT ROOT NUM ELEMENTS FAIL\n");
		return 1;
	}
	if (memcmp(array.root->elements[0], (uint8_t [4]){3, 15, 144, 0}, 4)) {
		printf("TAKE FROM RIGHT ROOT DATA FAIL\n");
		return 1;
	}
	// Check child 0
	if (child0->numElements != CB_BTREE_HALF_ELEMENTS) {
		printf("TAKE FROM RIGHT CHILD 0 NUM ELEMENTS FAIL\n");
		return 1;
	}
	if (memcmp(child0->elements[0], (uint8_t [4]){3, 0, 126, 0}, 4)
		&& memcmp(child0->elements[1], (uint8_t [4]){3, 2, 126, 0}, 4)
		&& memcmp(child0->elements[2], (uint8_t [4]){3, 3, 126, 0}, 4)
		&& memcmp(child0->elements[3], (uint8_t [4]){3, 4, 126, 0}, 4)
		&& memcmp(child0->elements[4], (uint8_t [4]){3, 5, 126, 0}, 4)
		&& memcmp(child0->elements[5], (uint8_t [4]){3, 6, 126, 0}, 4)
		&& memcmp(child0->elements[6], (uint8_t [4]){3, 7, 126, 0}, 4)
		&& memcmp(child0->elements[7], (uint8_t [4]){3, 8, 126, 0}, 4)
		&& memcmp(child0->elements[8], (uint8_t [4]){3, 9, 125, 0}, 4)
		&& memcmp(child0->elements[9], (uint8_t [4]){3, 10, 126, 0}, 4)
		&& memcmp(child0->elements[10], (uint8_t [4]){3, 11, 126, 0}, 4)
		&& memcmp(child0->elements[11], (uint8_t [4]){3, 12, 126, 0}, 4)
		&& memcmp(child0->elements[12], (uint8_t [4]){3, 13, 126, 0}, 4)
		&& memcmp(child0->elements[13], (uint8_t [4]){3, 14, 126, 0}, 4)
		&& memcmp(child0->elements[14], (uint8_t [4]){3, 15, 126, 0}, 4)
		&& memcmp(child0->elements[15], (uint8_t [4]){3, 15, 143, 0}, 4)) {
		printf("TAKE FROM LEFT CHILD 0 DATA FAIL\n");
		return 1;
	}
	// Check child 1
	if (child1->numElements != CB_BTREE_HALF_ELEMENTS) {
		printf("TAKE FROM RIGHT CHILD 1 NUM ELEMENTS FAIL\n");
		return 1;
	}
	if (memcmp(child1->elements[0], (uint8_t [4]){3, 16, 0, 0}, 4)
		&& memcmp(child1->elements[1], (uint8_t [4]){3, 32, 0, 0}, 4)
		&& memcmp(child1->elements[2], (uint8_t [4]){3, 32, 1, 0}, 4)
		&& memcmp(child1->elements[3], (uint8_t [4]){3, 32, 2, 0}, 4)
		&& memcmp(child1->elements[4], (uint8_t [4]){3, 32, 3, 0}, 4)
		&& memcmp(child1->elements[5], (uint8_t [4]){3, 32, 4, 0}, 4)
		&& memcmp(child1->elements[6], (uint8_t [4]){3, 32, 5, 0}, 4)
		&& memcmp(child1->elements[7], (uint8_t [4]){3, 32, 6, 0}, 4)
		&& memcmp(child1->elements[8], (uint8_t [4]){3, 32, 7, 0}, 4)
		&& memcmp(child1->elements[9], (uint8_t [4]){3, 32, 8, 0}, 4)
		&& memcmp(child1->elements[10], (uint8_t [4]){3, 32, 9, 0}, 4)
		&& memcmp(child1->elements[11], (uint8_t [4]){3, 32, 10, 0}, 4)
		&& memcmp(child1->elements[12], (uint8_t [4]){3, 32, 11, 0}, 4)
		&& memcmp(child1->elements[13], (uint8_t [4]){3, 32, 12, 0}, 4)
		&& memcmp(child1->elements[14], (uint8_t [4]){3, 32, 13, 0}, 4)
		&& memcmp(child1->elements[15], (uint8_t [4]){3, 32, 14, 0}, 4)) {
		printf("TAKE FROM RIGHT CHILD 1 DATA FAIL\n");
		return 1;
	}
	CBFreeAssociativeArray(&array);
	// Test lots of random keys
	CBInitAssociativeArray(&array, CBKeyCompare, NULL, NULL);
	// Generate keys
	int size = 52800;
	uint8_t * keys5 = malloc(size);
	for (int x = 0; x < size; x++) {
		if (x % 10)
			keys5[x] = rand();
		else
			keys5[x] = 9;
	}
	for (int x = 0; x < size; x += 10) {
		CBAssociativeArrayInsert(&array, keys5 + x, CBAssociativeArrayFind(&array, keys5 + x).position, NULL);
		for (int y = 0; y <= x; y += 10) {
			if (! CBAssociativeArrayFind(&array, keys5 + y).found) {
				printf("RANDOM FIND FAIL %u - %u\n", y, x);
				return 1;
			}
		}
		// Check length
		if (getLen(array.root) != x/10 + 1) {
			printf("INSERT LENGTH FAIL");
			return 1;
		}
	}
	// Test iteration
	CBPosition it;
	CBPosition it2;
	CBAssociativeArrayGetFirst(&array, &it);
	uint8_t * lastKey;
	bool end = false;
	uint8_t * minKey;
	uint8_t * maxKey;
	for (int x = 0; x < size; x += 10) {
		if (x == 40)
			minKey = it.node->elements[it.index];
		if (x == 130)
			maxKey = it.node->elements[it.index];
		if (end) {
			printf("ITERATOR END FALSE FAIL\n");
			return 1;
		}
		if (! CBAssociativeArrayFind(&array, it.node->elements[it.index]).found) {
			printf("ITERATE FIND FAIL %u\n", x);
			return 1;
		}
		if (! CBAssociativeArrayGetElement(&array, &it2, x/10)) {
			printf("GET ELEMENT FAIL %u\n", x);
			return 1;
		}
		if (it.node != it2.node || it.index != it2.index) {
			printf("GET ELEMENT AND ITERATOR CONSISTENCY FAIL %u\n", x);
			return 1;
		}
		lastKey = it.node->elements[it.index];
		if (x + 10 == size) {
			if (! CBAssociativeArrayGetLast(&array, &it2)){
				printf("GET LAST FAIL");
				return 1;
			}
			if (it.node != it2.node || it.index != it2.index) {
				printf("GET LAST AND ITERATOR CONSISTENCY FAIL %u\n", x);
				return 1;
			}
		}
		end = CBAssociativeArrayIterate(&array, &it);
		if (! end && memcmp(it.node->elements[it.index], lastKey, 10) <= 0) {
			printf("ITERATE ORDER FAIL %u\n", x);
			return 1;
		}
	}
	end = false;
	CBAssociativeArrayGetLast(&array, &it);
	// Test reverse iteration
	for (int x = size - 10; x >= 0; x -= 10) {
		if (end) {
			printf("ITERATOR BACKWARDS END FALSE FAIL\n");
			return 1;
		}
		if (! CBAssociativeArrayFind(&array, it.node->elements[it.index]).found) {
			printf("ITERATE BACKWARDS FIND FAIL %u\n", x);
			return 1;
		}
		if (x == 0) {
			if (! CBAssociativeArrayGetFirst(&array, &it2)){
				printf("GET FIRST FAIL");
				return 1;
			}
			if (it.node != it2.node || it.index != it2.index) {
				printf("GET FIRST AND ITERATOR CONSISTENCY FAIL %u\n", x);
				return 1;
			}
		}
		lastKey = it.node->elements[it.index];
		end = CBAssociativeArrayIterateBack(&array, &it);
		if (! end && memcmp(it.node->elements[it.index], lastKey, 10) >= 0) {
			printf("ITERATE BACKWARDS ORDER FAIL %u\n", x);
			return 1;
		}
	}
	// Test range interation
	CBRangeIterator rit = {minKey, maxKey};
	if (! CBAssociativeArrayRangeIteratorStart(&array, &rit)){
		printf("RANGER INTERATOR START FAIL\n");
		return 1;
	}
	uint8_t * itKey = CBRangeIteratorGetPointer(&rit);
	if (memcmp(minKey, itKey, 10)) {
		printf("RANGE ITERATOR START COMPARE FAIL\n");
		return 1;
	}
	for (uint8_t x = 0;; x++) {
		if (x == 9) {
			uint8_t * itKey = CBRangeIteratorGetPointer(&rit);
			if (memcmp(maxKey, itKey, 10)) {
				printf("RANGE ITERATOR END COMPARE FAIL\n");
				return 1;
			}
		}
		if (CBAssociativeArrayRangeIteratorNext(&array, &rit)) {
			if (x != 9) {
				printf("RANGE ITERATOR END NUM FAIL\n");
				return 1;
			}
			break;
		}
	}
	if (! end) {
		printf("ITERATOR END TRUE FAIL\n");
		return 1;
	}
	// Test reverse iteration
	if (! CBAssociativeArrayRangeIteratorLast(&array, &rit)){
		printf("RANGER INTERATOR LAST FAIL\n");
		return 1;
	}
	itKey = CBRangeIteratorGetPointer(&rit);
	if (memcmp(maxKey, itKey, 10)) {
		printf("RANGE ITERATOR LAST COMPARE FAIL\n");
		return 1;
	}
	for (uint8_t x = 0;; x++) {
		if (x == 9) {
			uint8_t * itKey = CBRangeIteratorGetPointer(&rit);
			if (memcmp(minKey, itKey, 10)) {
				printf("RANGE ITERATOR PREV COMPARE FAIL\n");
				return 1;
			}
		}
		if (CBAssociativeArrayRangeIteratorPrev(&array, &rit)) {
			if (x != 9) {
				printf("RANGE ITERATOR PREV END NUM FAIL\n");
				return 1;
			}
			break;
		}
	}
	if (! end) {
		printf("ITERATOR PREV END TRUE FAIL\n");
		return 1;
	}
	// Try removing half of elements
	for (int x = 0; x < size/2; x += 10){
		CBAssociativeArrayDelete(&array, CBAssociativeArrayFind(&array, keys5 + x).position, false);
		// Check length
		if (getLen(array.root) != (size-x)/10 - 1) {
			printf("DELETE LENGTH FAIL");
			return 1;
		}
	}
	// Now ensure the rest can be found
	for (int x = size/2; x < size; x += 10) {
		CBFindResult res = CBAssociativeArrayFind(&array, keys5 + x);
		if (! res.found) {
			printf("RANDOM FOUND FAIL\n");
			return 1;
		}
	}
	// Test CBAssociativeArrayRangeIteratorLast
	CBFreeAssociativeArray(&array);
	CBInitAssociativeArray(&array, CBKeyCompare, NULL, NULL);
	size = CB_BTREE_ELEMENTS * (CB_BTREE_ELEMENTS + 2) * 6;
	uint8_t * keys6 = malloc(size);
	for (uint16_t x = 0; x < size; x += 3) {
		keys6[x] = 2;
		keys6[x+1] = (x/3) >> 8;
		keys6[x+2] = x/3;
		CBAssociativeArrayInsert(&array, keys6 + x, CBAssociativeArrayFind(&array, keys6 + x).position, NULL);
		CBAssociativeArrayGetLast(&array, &it);
		uint8_t * key = (uint8_t *)it.node->elements[it.index];
		if (key[1] != ((x/3) >> 8) || key[2] != (uint8_t)(x/3)) {
			printf("INSERT GET LAST FAIL\n");
			return 1;
		}
	}
	CBRangeIterator iter = {(uint8_t []){2,0,0}, (uint8_t []){2,0,0}};
	for (uint16_t x = 0; x < size; x += 3) {
		((uint8_t *)iter.maxElement)[1] = (x/3 + (x % 3)) >> 8;
		((uint8_t *)iter.maxElement)[2] = x/3 + (x % 3);
		for (uint16_t y = 0; y <= x; y += 3) {
			((uint8_t *)iter.minElement)[1] = (y/3) >> 8;
			((uint8_t *)iter.minElement)[2] = y/3;
			if (! CBAssociativeArrayRangeIteratorLast(&array, &iter)) {
				printf("RANGE ITERATOR TO LAST FAIL %u - %u\n",x,y);
				return 1;
			}
			uint8_t * itKey = CBRangeIteratorGetPointer(&iter);
			if (itKey[1] != ((x/3) >> 8) || itKey[2] != (uint8_t)(x/3)) {
				printf("RANGE ITERATOR KEY FAIL %u - %u\n",x,y);
				return 1;
			}
		}
	}
	// Test for each macro
	uint16_t x = 0;
	CBAssociativeArrayForEach(uint8_t * el, &array){
		if (el[1] != ((x/3) >> 8) || el[2] != (uint8_t)(x/3)) {
			printf("FOR EACH FAIL %u\n", x);
			return 1;
		}
		// Test break
		if (x == 333)
			break;
		x+=3;
	}
	CBFreeAssociativeArray(&array);
	return 0;
}
