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
	uint8_t * elements[CB_BTREE_ORDER]; /**< The elements cointaining key-value pairs and can be dereferenced to uint8_t for access to the keys */
} CBBTreeNode;

/**
 @brief Describes the result of a find operation
 */
typedef struct{
	CBBTreeNode * node; /**< If found, this is the node with the data, otherwise it is the node where the data should be inserted. */
	uint8_t pos; /**< If found, this is the element with the data, otherwise it is the position where the data should be inserted. */
	bool found; /**< True if found, false otherwise. */
} CBFindResult;

/**
 @brief References an element in an array which can be iterated.
 */
typedef struct{
	CBBTreeNode * node; /**< The node for the element */
	uint8_t pos; /**< The element position in the node */
} CBIterator;

/**
 @brief @see CBAssociativeArray.h
 */
typedef struct{
	CBBTreeNode * root; /**< The root of the B-tree */
} CBAssociativeArray;


/**
 @brief Deletes an element from an array.
 @param self The array object
 @param pos The result from CBAssociativeArrayFind which determines the position to delete data.
 */
void CBAssociativeArrayDelete(CBAssociativeArray * self, CBFindResult pos);
/**
 @brief Finds data for a key in the array
 @param self The array object
 @param key The key to search for.
 @returns The result of the find.
 */
CBFindResult CBAssociativeArrayFind(CBAssociativeArray * self, uint8_t * key);
/**
 @brief Gets the first element (lowest key) in the array.
 @param self The array object
 @param it The CBIterator object to be set to the first element.
 */
bool CBAssociativeArrayGetFirst(CBAssociativeArray * self, CBIterator * it);
/**
 @brief Inserts an element into an array.
 @param self The array object.
 @param key The key-value pair to insert.
 @param pos The result from CBAssociativeArrayFind which determines the position to insert data.
 @oaram right The child to the right of the value we are inserting, which will be a new child from a split or NULL for a new value.
 @returns true on success and false on failure.
 */
bool CBAssociativeArrayInsert(CBAssociativeArray * self, uint8_t * keyValue, CBFindResult pos, CBBTreeNode * right);
/**
 @brief Iterates to the next element, in order.
 @param self The array object.
 @param it The CBFindResult to be iterated.
 @returns true if the end of the array has been reached and no iteration could take place. false if the end has not been reached.
 */
bool CBAssociativeArrayIterate(CBAssociativeArray * self, CBIterator * it);
/**
 @brief Does a binary search on a B-tree node.
 @param self The node
 @param key Key to search for.
 @returns The position and wether or not the key exists at this position.
 */
CBFindResult CBBTreeNodeBinarySearch(CBBTreeNode * self, uint8_t * key);
/**
 @brief Frees an associative array.
 @param self The array object.
 @param elDel If true, delete elements.
 */
void CBFreeAssociativeArray(CBAssociativeArray * self, bool elDel);
/**
 @brief Frees a B-tree node.
 @param self The node
 @param elDel If true, delete elements.
 */
void CBFreeBTreeNode(CBBTreeNode * self, bool elDel);
/**
 @brief Initialises an empty associative array.
 @param self The array object
 @returns true on success and false on failure.
 */
bool CBInitAssociativeArray(CBAssociativeArray * self);

#endif
