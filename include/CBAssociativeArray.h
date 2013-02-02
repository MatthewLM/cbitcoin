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
 @brief Stores elements with insert and search functions. The elements act as the key to insert and find values, and also doubles as the value. Useful for producing indexes.
 */

#ifndef CBASSOCIATIVEARRAYH
#define CBASSOCIATIVEARRAYH

// Includes

#include "CBConstants.h"
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

// Constants

#define CB_BTREE_ORDER 32 // Algorithm only works with even values. Best with powers of 2. This refers to the number of elements and not children.
#define CB_BTREE_HALF_ORDER CB_BTREE_ORDER/2

/**
 @brief A node for a B-Tree. After this data should come the keys and the data elements.
 */
typedef struct{
	void * parent; /**< The parent node */
	void * children[CB_BTREE_ORDER + 1]; /**< Children nodes */
	uint8_t numElements; /**< The number of elements */
	void * elements[CB_BTREE_ORDER]; /**< The elements cointaining the actual data, including the key information to be compared with the compareFunc function. */
} CBBTreeNode;

/**
 @brief References an element in an array or a position for insertion.
 */
typedef struct{
	CBBTreeNode * node; /**< The node for the element */
	uint8_t index; /**< The element index in the node */
} CBPosition;

/**
 @brief Describes the result of a find operation
 */
typedef struct{
	CBPosition position; /**< If found, this is the position of the data, otherwise it is the position where the data should be inserted. */
	bool found; /**< True if found, false otherwise. */
} CBFindResult;

/**
 @brief @see CBAssociativeArray.h
 */
typedef struct{
	CBBTreeNode * root; /**< The root of the B-tree */
	CBCompare (*compareFunc)(void *, void *); /**< The function pointer for comparing two keys, to see if the first key is higher, equal or lower to the second. */
	void (*onFree)(void *); /**< Called for each element in the array when CBFreeAssociativeArray is called. The arguement is the element. If assigned to NULL, instead nothing will happen. */
} CBAssociativeArray;

/**
 @brief Clears an array of all elements.
 @param self The array object
 */
void CBAssociativeArrayClear(CBAssociativeArray * self);
/**
 @brief Deletes an element from an array.
 @param self The array object
 @param pos The result from CBAssociativeArrayFind which determines the position to delete data.
 @param doFree If true, this will call the onFree function for the element being removed.
 */
void CBAssociativeArrayDelete(CBAssociativeArray * self, CBPosition pos, bool doFree);
/**
 @brief Finds data for a key in the array
 @param self The array object
 @param element The element to search for.
 @returns The result of the find.
 */
CBFindResult CBAssociativeArrayFind(CBAssociativeArray * self, void * element);
/**
 @brief Gets the element in the array at a specified index.
 @param self The array object
 @param it The CBPosition object to be set to the element.
 @param index The index to receive the element.
 @returns true if the element exists at the index, or false.
 */
bool CBAssociativeArrayGetElement(CBAssociativeArray * self, CBPosition * it, uint32_t index);
/**
 @brief Gets the first element (lowest key) in the array.
 @param self The array object
 @param it The CBPosition object to be set to the first element.
 @returns true if there is at least one element, or false.
 */
bool CBAssociativeArrayGetFirst(CBAssociativeArray * self, CBPosition * it);
/**
 @brief Gets the last element (highest key) in the array.
 @param self The array object
 @param it The CBPosition object to be set to the last element.
 @returns true if there is at least one element, or false.
 */
bool CBAssociativeArrayGetLast(CBAssociativeArray * self, CBPosition * it);
/**
 @brief Inserts an element into an array.
 @param self The array object.
 @param element The key-value pair to insert.
 @param pos The position to insert data.
 @oaram right The child to the right of the value we are inserting, which will be a new child from a split or NULL for a new value.
 @returns true on success and false on failure.
 */
bool CBAssociativeArrayInsert(CBAssociativeArray * self, void * element, CBPosition pos, CBBTreeNode * right);
/**
 @brief Iterates to the next element, in order.
 @param self The array object.
 @param it The CBFindResult to be iterated.
 @returns true if the end of the array has been reached and no iteration could take place. false if the end has not been reached.
 */
bool CBAssociativeArrayIterate(CBAssociativeArray * self, CBPosition * it);
/**
 @brief Does a binary search on a B-tree node.
 @param self The node
 @param key Key to search for.
 @param compareFunc The comparison function to use.
 @returns The position and wether or not the key exists at this position.
 */
CBFindResult CBBTreeNodeBinarySearch(CBBTreeNode * self, void * key, CBCompare (*compareFunc)(void *, void *));
/**
 @brief Frees an associative array and calls onFree for each element, unless onFree is NULL
 @param self The array object.
 */
void CBFreeAssociativeArray(CBAssociativeArray * self);
/**
 @brief Frees a B-tree node.
 @param self The node
 @param onFree Called for each element in the node and passed down each node. The argument is the element. If assigned to NULL, instead nothing will happen.
 @param onlyChildrem Only frees the children when true, not the root node passed into this function.
 @param elDel If true, delete elements.
 */
void CBFreeBTreeNode(CBBTreeNode * self, void (*onFree)(void *), bool onlyChildren);
/**
 @brief Initialises an empty associative array.
 @param self The array object
 @param compareFunc The comparison function to use.
 @param onFree Called for each element in the array when CBFreeAssociativeArray is called. The arguement is the element. If assigned to NULL, instead nothing will happen.
 @returns true on success and false on failure.
 */
bool CBInitAssociativeArray(CBAssociativeArray * self, CBCompare (*compareFunc)(void *, void *), void (*onFree)(void *));
/**
 @brief The default key comparison function. Reads the first byte in each key as the length, and then compares both keys fromt he remaining data.
 @param key1 The first key.
 @param key2 The second key.
 @returns If the first key is longer than the second key, or if the first key has data which is higher than the second key then CB_COMPARE_MORE_THAN is returned. If the keys are entirely equal thrn CB_COMPARE_EQUAL is returned. If the first key is shorter or has lower data than the second key CB_COMPARE_LESS_THAN is returned.
 */
CBCompare CBKeyCompare(void * key1, void * key2);

#endif
