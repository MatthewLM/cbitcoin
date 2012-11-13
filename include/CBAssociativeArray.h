//
//  CBAssociativeArray.h
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

/**
 @file
 @brief Stores keys and values with insert and search functions. The keys and values must all be of equal length. Useful for producing indexes.
 */

#ifndef CBASSOCIATIVEARRAYH
#define CBASSOCIATIVEARRAYH

#include "CBConstants.h"
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

/**
 @brief A node for a B-Tree. After this data should come the keys and the data elements.
 */
typedef struct{
	void * parent; /**< The parent node */
	void * children[CB_BTREE_ORDER + 1]; /**< Children nodes */
	uint8_t numElements; /**< The number of elements */
} CBBTreeNode;

/**
 @brief Describes the result of a find operation
 */
typedef struct{
	bool found; /**< True if found, false otherwise. */
	CBBTreeNode * node; /**< If found, this is the node with the data, otherwise it is the node where the data should be inserted. */
	uint8_t pos; /**< If found, this is the element with the data, otherwise it is the position where the data should be inserted. */
} CBFindResult;

/**
 @brief @see CBAssociativeArray.h
 */
typedef struct{
	uint8_t keySize; /**< The size of the keys */
	uint8_t dataSize; /**< The size of the data */
	uint16_t nodeSize; /**< The size of a B-tree node */
	CBBTreeNode * root; /**< The root of the B-tree */
} CBAssociativeArray;

/**
 @brief Finds data for a key in the array
 @param self The array object
 @param key The key to search for.
 @returns The result of the find.
 */
CBFindResult CBAssociativeArrayFind(CBAssociativeArray * self, uint8_t * key);
/**
 @brief Finds data for a key in the array
 @param self The array object
 @param key The key to search for.
 @param pos The result from CBAssociativeArrayFind which determines the position to insert data.
 @oaram right The child to the right of the value we are inserting, which will be a new child from a split or NULL for a new value.
 @returns true on success and false on failure.
 */
bool CBAssociativeArrayInsert(CBAssociativeArray * self, uint8_t * key, uint8_t * data, CBFindResult pos, CBBTreeNode * right);
/**
 @brief Does a binary search on a B-tree node.
 @param self The node
 @param key Key to search for.
 @param keySize The size of keys.
 @returns The position and wether or not the key exists at this position.
 */
CBFindResult CBBTreeNodeBinarySearch(CBBTreeNode * self, uint8_t * key, uint8_t keySize);
/**
 @brief Frees an associative array.
 @param self The array object.
 */
void CBFreeAssociativeArray(CBAssociativeArray * self);
/**
 @brief Frees a B-tree node.
 @param self The node
 */
void CBFreeBTreeNode(CBBTreeNode * self);
/**
 @brief Initialises an empty associative array.
 @param self The array object
 @param keySize The size of the keys for the array in bytes.
 @param dataSize The size of the data elements for the array in bytes.
 @returns true on success and false on failure.
 */
bool CBInitAssociativeArray(CBAssociativeArray * self, uint8_t keySize, uint8_t dataSize);

#endif