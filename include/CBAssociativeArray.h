//
//  CBAssociativeArray.h
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

// Constants and Macros

#define CB_BTREE_ELEMENTS 32 // Algorithm only works with even values. Best with powers of 2. This refers to the number of elements and not children.
#define CB_BTREE_HALF_ELEMENTS (CB_BTREE_ELEMENTS/2)
#define CBAssociativeArrayForEach(el, arr) \
	for (struct CBAssociativeArrayForEachData s = {.first = true, .enterNested = true}; \
		s.enterNested && ((s.first && CBAssociativeArrayGetFirst(arr, &s.it)) || (!s.first && !CBAssociativeArrayIterate(arr, &s.it))) ; \
		s.first = false, s.enterNested = !s.enterNested) \
		for (el = s.it.node->elements[s.it.index];s.enterNested;s.enterNested = !s.enterNested)
#define CBCurrentPosition s.it

typedef struct CBBTreeNode CBBTreeNode;

/**
 @brief A node for a B-Tree. After this data should come the keys and the data elements.
 */
struct CBBTreeNode{
	CBBTreeNode * children[CB_BTREE_ELEMENTS + 1]; /**< Children nodes */
	uint8_t numElements; /**< The number of elements */
	void * elements[CB_BTREE_ELEMENTS]; /**< The elements cointaining the actual data, including the key information to be compared with the compareFunc function. */
};

/**
 @brief References an element in an array or a position for insertion.
 */
typedef struct{
	CBBTreeNode * node; /**< The node for the element */
	uint8_t index; /**< The element index in the node */
	CBBTreeNode * parentNodes[7];
	uint8_t parentPositions[8];
	uint8_t parentIndex;
	uint8_t parentCursor;
} CBPosition;

struct CBAssociativeArrayForEachData{
	CBPosition it;
	bool first;
	bool enterNested;
};

/**
 @brief Describes the result of a find operation
 */
typedef struct{
	CBPosition position; /**< If found, this is the position of the data, otherwise it is the position where the data should be inserted. */
	bool found; /**< True if found, false otherwise. */
} CBFindResult;

/**
 @brief Allows iteration between a min element and max element in the array.
 */
typedef struct{
	void * minElement;
	void * maxElement;
	CBPosition pos;
} CBRangeIterator;

typedef struct CBAssociativeArray CBAssociativeArray;

/**
 @brief @see CBAssociativeArray.h
 */
struct CBAssociativeArray{
	CBBTreeNode * root; /**< The root of the B-tree */
	CBCompare (*compareFunc)(CBAssociativeArray *, void *, void *); /**< The function pointer for comparing two keys, to see if the first key is higher, equal or lower to the second. The first parameter is the array object and the next two are the keys. */
	void (*onFree)(void *); /**< Called for each element in the array when CBFreeAssociativeArray is called. The arguement is the element. If assigned to NULL, instead nothing will happen. */
	void * compareObject; /**< Allows a pointer to be stored for reference in the comparison function. */
};

/**
 @brief Places the iterator at the end.
 @param self The array object
 @param it The iterator object.
 @returns true if the final element has been found, or false if there are no elements to be found.
 */
bool CBAssociativeArrayRangeIteratorLast(CBAssociativeArray * self, CBRangeIterator * it);
/**
 @brief Starts the iteration between two elements
 @param self The array object
 @param it The iterator object.
 @returns true if an initial element has been found, or false if there are no elements to be found.
 */
bool CBAssociativeArrayRangeIteratorStart(CBAssociativeArray * self, CBRangeIterator * it);
/**
 @brief Iterates to the next element.
 @param self The array object
 @param it The iterator object.
 @returns false if an element has been found during iteration, or true if there are no more elements to be found.
 */
bool CBAssociativeArrayRangeIteratorNext(CBAssociativeArray * self, CBRangeIterator * it);
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
 @brief Gets the last element (highest key) in the array.
 @param self The array object
 @param it The CBPosition object to be set to the last element.
 @returns true if there is at least one element, or false.
 */
bool CBAssociativeArrayGet(CBAssociativeArray * self, CBPosition * it);
/**
 @brief Inserts an element into an array.
 @param self The array object.
 @param element The key-value pair to insert.
 @param pos The position to insert data.
 @oaram right The child to the right of the value we are inserting, which will be a new child from a split or NULL for a new value.
 */
void CBAssociativeArrayInsert(CBAssociativeArray * self, void * element, CBPosition pos, CBBTreeNode * right);
bool CBAssociativeArrayIsEmpty(CBAssociativeArray * self);
/**
 @brief Iterates to the next element, in order.
 @param self The array object.
 @param it The CBFindResult to be iterated.
 @returns true if the end of the array has been reached and no iteration could take place. false if the end has not been reached.
 */
bool CBAssociativeArrayIterate(CBAssociativeArray * self, CBPosition * it);
/**
 @brief Determines if an associative array is empty or not.
 @returns true if not empty and false if empty.
 */
bool CBAssociativeArrayNotEmpty(CBAssociativeArray * self);
/**
 @brief Does a binary search on a B-tree node.
 @param array The array object.
 @param node The node
 @param key Key to search for.
 @returns The position and wether or not the key exists at this position.
 */
void CBBTreeNodeBinarySearch(CBAssociativeArray * array, CBBTreeNode * node, void * key, CBFindResult * result);
/**
 @brief Gets the pointer to an element from a CBFindResult
 @param res The CBFindResult.
 @returns The element pointer.
 */
void * CBFindResultToPointer(CBFindResult res);
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
 @param compareObject The object which is passed into the compare function.
 @param onFree Called for each element in the array when CBFreeAssociativeArray is called. The argument is the element. If assigned to NULL, instead nothing will happen.
 */
void CBInitAssociativeArray(CBAssociativeArray * self, CBCompare (*compareFunc)(CBAssociativeArray *, void *, void *), void * compareObject, void (*onFree)(void *));
/**
 @brief Reads the first byte in each key as the length, and then compares both keys from the remaining data.
 @param self The array object
 @param key1 The first key.
 @param key2 The second key.
 @returns If the first key is longer than the second key, or if the first key has data which is higher than the second key then CB_COMPARE_MORE_THAN is returned. If the keys are entirely equal thrn CB_COMPARE_EQUAL is returned. If the first key is shorter or has lower data than the second key CB_COMPARE_LESS_THAN is returned.
 */
CBCompare CBKeyCompare(CBAssociativeArray * self, void * key1, void * key2);
/**
 @brief Gets the element pointer from a CBRangeIterator
 @param it The iterator object.
 @returns The element pointer.
 */
void * CBRangeIteratorGetPointer(CBRangeIterator * it);

#endif
