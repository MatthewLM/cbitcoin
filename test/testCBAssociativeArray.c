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
	// ??? Add more in-depth tests.
	unsigned int s = (unsigned int)time(NULL);
	s = 1353092048;
	printf("Session = %u\n", s);
	//srand(s);
	CBAssociativeArray array;
	CBInitAssociativeArray(&array, CBKeyCompare, NULL);
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
	// First try adding CB_BTREE_ORDER random values
	uint8_t keys[CB_BTREE_ORDER][4];
	for (uint8_t x = 0; x < CB_BTREE_ORDER; x++) {
		keys[x][0] = 3;
		keys[x][1] = rand();
		keys[x][2] = x;
		keys[x][3] = rand();
		CBAssociativeArrayInsert(&array, keys[x], CBAssociativeArrayFind(&array, keys[x]).position, NULL);
	}
	// Check in-order
	for (uint8_t x = 0; x < CB_BTREE_ORDER; x++) {
		CBFindResult res2 = CBAssociativeArrayFind(&array, array.root->elements[x]);
		if (NOT res2.found) {
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
	CBInitAssociativeArray(&array, CBKeyCompare, NULL);
	// Insert CB_BTREE_ORDER elements
	uint8_t keys2[CB_BTREE_ORDER+1][4];
	for (uint8_t x = 0; x < CB_BTREE_ORDER; x++) {
		keys2[x][0] = 3;
		keys2[x][1] = x;
		keys2[x][2] = 126;
		keys2[x][3] = 0;
		CBAssociativeArrayInsert(&array, keys2[x], CBAssociativeArrayFind(&array, keys2[x]).position, NULL);
	}
	// Try inserting value in the middle.
	keys2[CB_BTREE_ORDER][0] = 3;
	keys2[CB_BTREE_ORDER][1] = CB_BTREE_HALF_ORDER;
	keys2[CB_BTREE_ORDER][2] = 0;
	keys2[CB_BTREE_ORDER][3] = 0;
	CBAssociativeArrayInsert(&array, keys2[CB_BTREE_ORDER], CBAssociativeArrayFind(&array, keys2[CB_BTREE_ORDER]).position, NULL);
	// Check the new root.
	res = CBAssociativeArrayFind(&array, keys2[CB_BTREE_ORDER]);
	if (res.position.node != array.root) {
		printf("INSERT MIDDLE SPLIT NODE FAIL\n");
		return 1;
	}
	if (NOT res.found) {
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
	for (uint8_t x = 0; x < CB_BTREE_HALF_ORDER; x++) {
		key[1] = x;
		key[2] = 126;
		key[3] = 0;
		res = CBAssociativeArrayFind(&array, key);
		if (res.position.node != array.root->children[0]) {
			printf("LEFT CHILD NODE FAIL\n");
			return 1;
		}
		if (NOT res.found) {
			printf("LEFT CHILD FOUND FAIL\n");
			return 1;
		}
		if (res.position.index != x) {
			printf("LEFT CHILD POS FAIL\n");
			return 1;
		}
	}
	// Right side
	for (uint8_t x = CB_BTREE_HALF_ORDER; x < CB_BTREE_ORDER; x++) {
		key[1] = x;
		key[2] = 126;
		key[3] = 0;
		res = CBAssociativeArrayFind(&array, key);
		if (res.position.node != array.root->children[1]) {
			printf("RIGHT CHILD NODE FAIL\n");
			return 1;
		}
		if (NOT res.found) {
			printf("RIGHT CHILD FOUND FAIL\n");
			return 1;
		}
		if (res.position.index != x - CB_BTREE_HALF_ORDER) {
			printf("RIGHT CHILD POS FAIL\n");
			return 1;
		}
	}
	// Insert 16 values to left child
	uint8_t keys3[CB_BTREE_HALF_ORDER + 1][4];
	for (uint8_t x = 0; x < CB_BTREE_HALF_ORDER; x++) {
		keys3[x][0] = 3;
		keys3[x][1] = CB_BTREE_HALF_ORDER - 1;
		keys3[x][2] = 128 + x;
		keys3[x][3] = 0;
		CBAssociativeArrayInsert(&array, keys3[x], CBAssociativeArrayFind(&array, keys3[x]).position, NULL);
	}
	// Add value to the right of the left child
	keys3[CB_BTREE_HALF_ORDER][0] = 3;
	keys3[CB_BTREE_HALF_ORDER][1] = CB_BTREE_HALF_ORDER - 1;
	keys3[CB_BTREE_HALF_ORDER][2] = 128 + CB_BTREE_HALF_ORDER/2;
	keys3[CB_BTREE_HALF_ORDER][3] = 127;
	CBFindResult res2 = CBAssociativeArrayFind(&array, keys3[CB_BTREE_HALF_ORDER]);
	CBAssociativeArrayInsert(&array, keys3[CB_BTREE_HALF_ORDER], res2.position, NULL);
	// Now check root
	key[1] = CB_BTREE_HALF_ORDER - 1;
	key[2] = 128;
	key[3] = 0;
	res = CBAssociativeArrayFind(&array, key);
	if (res.position.node != array.root) {
		printf("INSERT RIGHT SPLIT NODE FAIL\n");
		return 1;
	}
	if (NOT res.found) {
		printf("INSERT RIGHT SPLIT FOUND FAIL\n");
		return 1;
	}
	if (res.position.index) {
		printf("INSERT RIGHT SPLIT POS FAIL\n");
		return 1;
	}
	// Check left child
	for (uint8_t x = 0; x < CB_BTREE_HALF_ORDER; x++) {
		key[1] = x;
		key[2] = 126;
		key[3] = 0;
		res = CBAssociativeArrayFind(&array, key);
		if (res.position.node != array.root->children[0]) {
			printf("RIGHT SPLIT LEFT CHILD NODE FAIL\n");
			return 1;
		}
		if (NOT res.found) {
			printf("RIGHT SPLIT LEFT CHILD FOUND FAIL\n");
			return 1;
		}
		if (res.position.index != x) {
			printf("RIGHT SPLIT LEFT CHILD POS FAIL\n");
			return 1;
		}
	}
	// Check middle child
	for (uint8_t x = 0; x < CB_BTREE_HALF_ORDER; x++) {
		key[1] = CB_BTREE_HALF_ORDER - 1;
		key[2] = 128 + x + 1 - ((x >= CB_BTREE_HALF_ORDER/2) ? 1 : 0);
		key[3] = (x == CB_BTREE_HALF_ORDER/2) ? 127 : 0;
		res = CBAssociativeArrayFind(&array, key);
		if (res.position.node != array.root->children[1]) {
			printf("RIGHT SPLIT MID CHILD NODE FAIL\n");
			return 1;
		}
		if (NOT res.found) {
			printf("RIGHT SPLIT MID CHILD FOUND FAIL\n");
			return 1;
		}
		if (res.position.index != x) {
			printf("RIGHT SPLIT MID CHILD POS FAIL\n");
			return 1;
		}
	}
	// Insert 16 values to right child
	uint8_t keys4[CB_BTREE_HALF_ORDER + 1][4];
	for (uint8_t x = 0; x < CB_BTREE_HALF_ORDER; x++) {
		keys4[x][0] = 3;
		keys4[x][1] = CB_BTREE_ORDER;
		keys4[x][2] = x;
		keys4[x][3] = 0;
		CBAssociativeArrayInsert(&array, keys4[x], CBAssociativeArrayFind(&array, keys4[x]).position, NULL);
	}
	// Add value to left side of right child
	keys4[CB_BTREE_HALF_ORDER][0] = 3;
	keys4[CB_BTREE_HALF_ORDER][1] = (CB_BTREE_ORDER*3)/4;
	keys4[CB_BTREE_HALF_ORDER][2] = 125;
	keys4[CB_BTREE_HALF_ORDER][3] = 0;
	CBAssociativeArrayInsert(&array, keys4[CB_BTREE_HALF_ORDER], CBAssociativeArrayFind(&array, keys4[CB_BTREE_HALF_ORDER]).position, NULL);
	// Check root
	key[1] = CB_BTREE_ORDER - 1;
	key[2] = 126;
	key[3] = 0;
	res = CBAssociativeArrayFind(&array, key);
	if (res.position.node != array.root) {
		printf("INSERT LEFT SPLIT NODE FAIL\n");
		return 1;
	}
	if (NOT res.found) {
		printf("INSERT LEFT SPLIT FOUND FAIL\n");
		return 1;
	}
	if (res.position.index != 2) {
		printf("INSERT LEFT SPLIT POS FAIL\n");
		return 1;
	}
	// Check 3rd child
	for (uint8_t x = 0; x < CB_BTREE_HALF_ORDER; x++) {
		key[1] = CB_BTREE_HALF_ORDER + x - ((x > CB_BTREE_HALF_ORDER/2) ? 1 : 0);
		key[2] = 126 - ((x == CB_BTREE_HALF_ORDER/2) ? 1 : 0);
		key[3] = 0;
		res = CBAssociativeArrayFind(&array, key);
		if (res.position.node != array.root->children[2]) {
			printf("LEFT SPLIT 3RD CHILD NODE FAIL\n");
			return 1;
		}
		if (NOT res.found) {
			printf("LEFT SPLIT 3RD CHILD FOUND FAIL\n");
			return 1;
		}
		if (res.position.index != x) {
			printf("LEFT SPLIT 3RD CHILD POS FAIL\n");
			return 1;
		}
	}
	// Check 4th child
	for (uint8_t x = 0; x < CB_BTREE_HALF_ORDER; x++) {
		key[1] = CB_BTREE_ORDER;
		key[2] = x;
		key[3] = 0;
		res = CBAssociativeArrayFind(&array, key);
		if (res.position.node != array.root->children[3]) {
			printf("LEFT SPLIT 4TH CHILD NODE FAIL\n");
			return 1;
		}
		if (NOT res.found) {
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
	if (child0->numElements != CB_BTREE_ORDER) {
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
	if (child1->numElements != CB_BTREE_ORDER) {
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
	for (uint8_t x = 0; x < CB_BTREE_HALF_ORDER + 1; x++) {
		key[1] = CB_BTREE_HALF_ORDER + x - ((x > 8)? 1 : 0);
		key[2] = 126 - ((x == 8)? 1 : 0);
		key[3] = 0;
		CBAssociativeArrayDelete(&array, CBAssociativeArrayFind(&array, key).position, false);
	}
	// Check child 0
	if (child0->numElements != CB_BTREE_ORDER - 1) {
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
	if (child1->numElements != CB_BTREE_HALF_ORDER) {
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
	for (uint8_t x = 0; x < CB_BTREE_HALF_ORDER; x++) {
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
	if (child0->numElements != CB_BTREE_HALF_ORDER) {
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
	if (child1->numElements != CB_BTREE_HALF_ORDER) {
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
	CBInitAssociativeArray(&array, CBKeyCompare, NULL);
	// Generate keys
	int size = CB_BTREE_ORDER * (CB_BTREE_ORDER + 2) * 20;
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
			if (NOT CBAssociativeArrayFind(&array, keys5 + y).found) {
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
	for (int x = 0; x < size; x += 10) {
		if (end) {
			printf("ITERATOR END FALSE FAIL\n");
			return 1;
		}
		if (NOT CBAssociativeArrayFind(&array, it.node->elements[it.index]).found) {
			printf("ITERATE FIND FAIL %u\n", x);
			return 1;
		}
		if (NOT CBAssociativeArrayGetElement(&array, &it2, x/10)) {
			printf("GET ELEMENT FAIL %u\n", x);
			return 1;
		}
		if (it.node != it2.node || it.index != it2.index) {
			printf("GET ELEMENT AND ITERATOR CONSISTENCY FAIL %u\n", x);
			return 1;
		}
		lastKey = it.node->elements[it.index];
		if (x + 10 == size) {
			if (NOT CBAssociativeArrayGetLast(&array, &it2)){
				printf("GET LAST FAIL");
				return 1;
			}
			if (it.node != it2.node || it.index != it2.index) {
				printf("GET LAST AND ITERATOR CONSISTENCY FAIL %u\n", x);
				return 1;
			}
		}
		end = CBAssociativeArrayIterate(&array, &it);
		if (NOT end && memcmp(it.node->elements[it.index], lastKey, 10) <= 0) {
			printf("ITERATE ORDER FAIL %u\n", x);
			return 1;
		}
	}
	if (NOT end) {
		printf("ITERATOR END TRUE FAIL\n");
		return 1;
	}
	// Try removing half of elements
	for (int x = 0; x < size/2; x += 10)
		CBAssociativeArrayDelete(&array, CBAssociativeArrayFind(&array, keys5 + x).position, false);
	// Now ensure the rest can be found
	for (int x = size/2; x < size; x += 10) {
		CBFindResult res = CBAssociativeArrayFind(&array, keys5 + x);
		if (NOT res.found) {
			printf("RANDOM FOUND FAIL\n");
			return 1;
		}
	}
	return 0;
}
