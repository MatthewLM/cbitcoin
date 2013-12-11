//
//  CBAssociativeArray.c
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

//  SEE HEADER FILE FOR DOCUMENTATION ??? Many optimisations can be done here.

#include "CBAssociativeArray.h"
#include "CBDependencies.h"
#include <assert.h>

bool CBAssociativeArrayRangeIteratorLast(CBAssociativeArray * self, CBRangeIterator * it){
	// Try to find the maximum element.
	CBFindResult res = CBAssociativeArrayFind(self, it->maxElement);
	if (! res.found) {
		if (res.position.node->numElements == 0)
			// There are no elements
			return false;
		// We need to go backwards (Not found always goes to bottom of tree)
		if (res.position.index == 0) {
			// No child (always on bottom), try parent
			if (res.position.node == self->root)
				return false;
			if (res.position.parentIndex)
				res.position.node = res.position.parentNodes[res.position.parentIndex-1];
			else
				res.position.node = self->root;
			res.position.index = res.position.parentPositions[res.position.parentIndex--];
			if (res.position.index == 0)
				return false;
			res.position.index--;
		}else
			// Else we can just go back one in this node
			res.position.index--;
		// Check this element is above or equal to the minimum.
		if (self->compareFunc(self, it->minElement, CBFindResultToPointer(res)) == CB_COMPARE_MORE_THAN)
			return false;
	}
	it->pos = res.position;
	return true;
}
bool CBAssociativeArrayRangeIteratorNext(CBAssociativeArray * self, CBRangeIterator * it){
	// Try iterating
	if (CBAssociativeArrayIterate(self, &it->pos))
		// Couldn't iterate, meaning we have reach the end of the array already
		return true;
	// Check the range
	if (self->compareFunc(self, it->maxElement, CBRangeIteratorGetPointer(it)) == CB_COMPARE_LESS_THAN)
		// The next element goes out of range.
		return true;
	return false;
}
bool CBAssociativeArrayRangeIteratorPrev(CBAssociativeArray * self, CBRangeIterator * it){
	// Try iterating
	if (CBAssociativeArrayIterateBack(self, &it->pos))
		// Couldn't iterate, meaning we have reach the start of the array already
		return true;
	// Check the range
	if (self->compareFunc(self, it->minElement, CBRangeIteratorGetPointer(it)) == CB_COMPARE_MORE_THAN)
		// The next element goes out of range.
		return true;
	return false;
}
bool CBAssociativeArrayRangeIteratorStart(CBAssociativeArray * self, CBRangeIterator * it){
	// Try to find the minimum element
	CBFindResult res = CBAssociativeArrayFind(self, it->minElement);
	if (! res.found) {
		if (res.position.node->numElements == 0)
			// There are no elements
			return false;
		// If we are past the end of the node's elements, go back onto the previous element, and then iterate.
		if (res.position.index == res.position.node->numElements) {
			res.position.index--;
			if (CBAssociativeArrayIterate(self, &res.position))
				// Couldn't iterate, meaning there is no higher element than the minimum element.
				return false;
			// Iterated to the element in the array after the minimum element, so use it
		}
		// Else we have landed on the element after the minimum element.
		// Check that the element is below or equal to the maximum
		if (self->compareFunc(self, it->maxElement, CBFindResultToPointer(res)) == CB_COMPARE_LESS_THAN)
			// The element is above the maximum so return false
			return false;
	}
	it->pos = res.position;
	return true;
}
void CBAssociativeArrayClear(CBAssociativeArray * self){
	CBFreeBTreeNode(self->root, self->onFree, true);
}
void CBAssociativeArrayDelete(CBAssociativeArray * self, CBPosition pos, bool doFree){
	assert(self->root->numElements <= 32);
	if (doFree)
		self->onFree(pos.node->elements[pos.index]);
	if (! pos.node->children[0]) {
		// Leaf
		CBBTreeNode * parent = pos.parentIndex ? pos.parentNodes[pos.parentIndex-1] : self->root;
		if (pos.node->numElements > CB_BTREE_HALF_ELEMENTS || pos.node == self->root) {
			// Can simply remove this element. Nice and easy.
			if (--pos.node->numElements > pos.index)
				// Move everything down to overwrite the removed element.
				memmove(pos.node->elements + pos.index, pos.node->elements + pos.index + 1, (pos.node->numElements - pos.index) * sizeof(*pos.node->elements));
		}else{
			// Underflow... Where things get complicated. Loop through merges all the way to the root
			for (;;){
				// We have a parent so we can check siblings.
				uint8_t parentPosition = pos.parentPositions[pos.parentIndex];
				CBBTreeNode * left = NULL;
				CBBTreeNode * right = NULL;
				if (parentPosition) {
					// Create left sibling data
					left = parent->children[parentPosition - 1];
					// Can check left sibling
					if (left->numElements > CB_BTREE_HALF_ELEMENTS) {
						// Can take from left
						// Move elements up to overwrite the element we are deleting
						if (pos.index) {
							memmove(pos.node->elements + 1, pos.node->elements, pos.index * sizeof(*pos.node->elements));
							if (pos.node->children[0])
								memmove(pos.node->children + 1, pos.node->children, (pos.index + 1) * sizeof(*pos.node->children));
						}
						// Lower left parent element to this node
						pos.node->elements[0] = parent->elements[parentPosition - 1];
						// Now move left sibling's far right element to parent
						parent->elements[parentPosition - 1] = left->elements[left->numElements - 1];
						if (left->children[0])
							// Make left sibling's far right child the far left child of this node
							pos.node->children[0] = left->children[left->numElements];
						// Now remove left sibling's far right element and be done
						left->numElements--;
						assert(self->root->numElements <= 32);
						return;
					}
				}
				if (parentPosition < parent->numElements) {
					// Create right sibling data
					right = parent->children[parentPosition + 1];
					// Can check right sibling
					if (right->numElements > CB_BTREE_HALF_ELEMENTS) {
						// Can take from right
						// Move elements down to overwrite the element we are deleting
						if (pos.node->numElements > pos.index + 1) {
							memmove(pos.node->elements + pos.index, pos.node->elements + pos.index + 1, (pos.node->numElements - pos.index - 1) * sizeof(*pos.node->elements));
							if (pos.node->children[0])
								memmove(pos.node->children + pos.index, pos.node->children + pos.index + 1, (pos.node->numElements - pos.index) * sizeof(*pos.node->children));
						}
						// Lower right parent element to this node
						pos.node->elements[pos.node->numElements - 1] = parent->elements[parentPosition];
						// Now move right sibling's far left element to parent
						parent->elements[parentPosition] = right->elements[0];
						if (right->children[0]) {
							// Make right sibling's far left child the far right child of this node
							pos.node->children[pos.node->numElements] = right->children[0];
							// Move right siblings children down
							memmove(right->children, right->children + 1, right->numElements * sizeof(*right->children));
						}
						// Now remove right sibling's far left element and be done
						right->numElements--;
						memmove(right->elements, right->elements + 1, right->numElements * sizeof(*right->elements));
						assert(self->root->numElements <= 32);
						return;
					}
				}
				// Take away element num.
				pos.node->numElements--;
				// Could not take from siblings... now for the merging. :-(
				if (parentPosition == 0) {
					// We are merging right sibling into the node.
					// First move data down to overwrite deleted data o 0 i 1 ii
					if (pos.node->numElements > pos.index)
						memmove(pos.node->elements + pos.index, pos.node->elements + pos.index + 1, (pos.node->numElements - pos.index) * sizeof(*pos.node->elements));
					if (pos.node->children[0])
						memmove(pos.node->children + pos.index, pos.node->children + pos.index + 1, (pos.node->numElements - pos.index + 1) * sizeof(*pos.node->children));
					// Add parents mid value
					pos.node->elements[pos.node->numElements] = parent->elements[parentPosition];
					// Now merge right sibling
					pos.node->numElements++;
					memmove(pos.node->elements + CB_BTREE_HALF_ELEMENTS, right->elements, CB_BTREE_HALF_ELEMENTS * sizeof(*pos.node->elements));
					if (pos.node->children[0])
						memmove(pos.node->children + CB_BTREE_HALF_ELEMENTS, right->children, (CB_BTREE_HALF_ELEMENTS + 1) * sizeof(*right->children));
					// Adjust number of elements to full
					pos.node->numElements = CB_BTREE_ELEMENTS;
					// Free right node
					free(right);
					// Move node over to where the right sibling was so deleted part is on the left. ??? Change algorithm to merge differently.
					parent->children[1] = parent->children[0];
				}else{
					// We are merging the node into the left sibling.
					// Move over parent middle element
					left->elements[left->numElements] = parent->elements[parentPosition - 1];
					// Move over elements before deleted element.
					left->numElements++;
					if (pos.index){
						memcpy(left->elements + left->numElements, pos.node->elements, pos.index * sizeof(*left->elements));
						if (pos.node->children[0])
							memcpy(left->children + left->numElements, pos.node->children, pos.index * sizeof(*pos.node->children));
					}
					left->numElements += pos.index;
					// Move over elements after the deleted element to the left node
					if (pos.node->numElements > pos.index)
						memcpy(left->elements + left->numElements, pos.node->elements + pos.index + 1, (pos.node->numElements - pos.index) * sizeof(*left->elements));
					if (pos.node->children[0])
						memcpy(left->children + left->numElements, pos.node->children + pos.index + 1, (pos.node->numElements - pos.index + 1) * sizeof(*pos.node->children));
					// Adjust number of elements
					left->numElements += pos.node->numElements - pos.index;
					// Free the node
					free(pos.node);
					// Move left sibling over to where the node was so deleted part is on the left.
					parent->children[parentPosition] = parent->children[parentPosition - 1];
				}
				// If position is 0 then we are deleting to the right, else to the left.
				pos.index = parentPosition ? parentPosition - 1 : 0;
				// Now delete the parent element and continue merge upto root if neccesary...
				bool root = parent == self->root;
				if ((root && parent->numElements > 1)
					|| (! root && parent->numElements > CB_BTREE_HALF_ELEMENTS)) {
					// Either a root and with more than one element to still remove or not root and more than half the minimum elements.
					// Therefore remove the element and be done.
					parent->numElements--;
					if (parent->numElements > pos.index) {
						memmove(parent->elements + pos.index, parent->elements + pos.index + 1, (parent->numElements - pos.index) * sizeof(*parent->elements));
						memmove(parent->children + pos.index + 1, parent->children + pos.index + 2, (parent->numElements - pos.index) * sizeof(*parent->children));
					}
					assert(self->root->numElements <= 32);
					return;
				}else if (root){
					// The parent is the root and has 1 element. The root is now empty so we make it's newly merged children the root.
					self->root = parentPosition ? left : pos.node;
					free(parent);
					return;
				}else{
					// The parent is not root and has the minimum allowed elements. Therefore we need to merge again, going around the loop.
					pos.node = parent;
					pos.parentIndex--;
					parent = pos.parentIndex ? pos.parentNodes[pos.parentIndex-1] : self->root;
				}
			}
		}
	}else{
		// Not leaf data, insert successor then delete successor from/in the right child.
		CBBTreeNode * child = pos.node->children[pos.index + 1];
		// Set parent information ??? This repeats a lot: Add to function.
		if (pos.node != self->root){
			pos.parentNodes[pos.parentIndex++] = pos.node;
			pos.parentCursor++;
		}
		pos.parentPositions[pos.parentIndex] = pos.index + 1;
		// Now we have the right child, find left most element in the child.
		while (child->children[0]){
			pos.parentNodes[pos.parentIndex++] = child;
			pos.parentCursor++;
			pos.parentPositions[pos.parentIndex] = 0;
			child = child->children[0];
		}
		// Get left-most element and overwrite the element we are deleting with this left-most element.
		pos.node->elements[pos.index] = child->elements[0];
		// Now delete the successive element.
		pos.node = child;
		pos.index = 0;
		CBAssociativeArrayDelete(self, pos, false);
	}
	assert(self->root->numElements <= 32);
}
CBFindResult CBAssociativeArrayFind(CBAssociativeArray * self, void * element){
	CBFindResult result;
	CBBTreeNode * node = self->root;
	assert(node->numElements <= 32);
	result.position.parentIndex = 0;
	result.position.parentCursor = 0;
	for (;;) {
		// Do binary search on node
		CBBTreeNodeBinarySearch(self, node, element, &result);
		if (result.found){
			// Found the data on this node.
			result.position.node = node;
			if (result.position.node != self->root){
				CBBTreeNode * n = self->root;
				for (uint8_t x = 0; x < result.position.parentIndex; x++) {
					n = n->children[result.position.parentPositions[x]];
					if (n != result.position.parentNodes[x]) {
						exit(EXIT_FAILURE);
					}
				}
				n = n->children[result.position.parentPositions[result.position.parentIndex]];
				if (n != node) {
					exit(EXIT_FAILURE);
				}
			}
			return result;
		}else{
			// We have not found the data on this node, if there is a child go to the child.
			if (node->children[0]){
				// Set parent information
				if (node != self->root){
					result.position.parentNodes[result.position.parentIndex++] = node;
					result.position.parentCursor++;
				}
				result.position.parentPositions[result.position.parentIndex] = result.position.index;
				// Child exists, move to it
				node = node->children[result.position.index];
				assert(node != NULL);
			}else{
				// The child doesn't exist. Return as not found with position for insertion.
				result.position.node = node;
				return result;
			}
		}
	}
}
bool CBAssociativeArrayGetElement(CBAssociativeArray * self, CBPosition * it, uint32_t index){
	if (! CBAssociativeArrayGetFirst(self, it))
		return false;
	// ??? Lazy method of iteration.
	for (uint32_t x = 0; x < index; x++)
		if (CBAssociativeArrayIterate(self, it))
			return false;
	return true;
}
bool CBAssociativeArrayGetFirst(CBAssociativeArray * self, CBPosition * it){
	if (! self->root->numElements)
		return false;
	it->node = self->root;
	it->index = 0;
	it->parentIndex = 0;
	it->parentCursor = 0;
	if (it->node->children[0]) for (;;){
		it->node = it->node->children[0];
		it->parentPositions[it->parentIndex] = 0;
		if (it->node->children[0])
			it->parentNodes[it->parentIndex++] = it->node;
		else
			break;
	}
	return true;
}
bool CBAssociativeArrayGetLast(CBAssociativeArray * self, CBPosition * it){
	if (! self->root->numElements)
		return false;
	it->node = self->root;
	it->parentIndex = 0;
	it->parentCursor = 0;
	if (it->node->children[0]) for (;;){
		it->parentPositions[it->parentIndex] = it->node->numElements;
		it->node = it->node->children[it->node->numElements];
		if (it->node->children[0])
			it->parentNodes[it->parentIndex++] = it->node;
		else
			break;
	}
	it->index = it->node->numElements - 1;
	return true;
}
#include <stdio.h>
void CBAssociativeArrayInsert(CBAssociativeArray * self, void * element, CBPosition pos, CBBTreeNode * right){
	assert(self->root->numElements <= 32);
	// See if we can insert data in this node
	if (pos.node->numElements < CB_BTREE_ELEMENTS) {
		// Yes we can, do that.
		if (pos.node->numElements > pos.index){
			// Move up the keys, data and children
			memmove(pos.node->elements + pos.index + 1, pos.node->elements + pos.index, (pos.node->numElements - pos.index) * sizeof(*pos.node->elements));
			if (right)
				memmove(pos.node->children + pos.index + 2, pos.node->children + pos.index + 1, (pos.node->numElements - pos.index) * sizeof(*pos.node->children));
		}
		// Copy over new key, data and children
		pos.node->elements[pos.index] = element;
		if (right)
			pos.node->children[pos.index + 1] = right;
		// Increase number of elements
		pos.node->numElements++;
		// The element has been added, we can stop here.
		assert(self->root->numElements <= 32);
		return;
	}else{
		// Nope, we need to split this node into two.
		CBBTreeNode * new = malloc(sizeof(*new));
		// Make the leftmost child NULL so we know this is a leaf, if it is.
		new->children[0] = NULL;
		// Make both sides have half the order.
		new->numElements = CB_BTREE_HALF_ELEMENTS;
		pos.node->numElements = CB_BTREE_HALF_ELEMENTS;
		uint8_t * midKeyValue;
		if (pos.index >= CB_BTREE_HALF_ELEMENTS) {
			// Not in first child.
			if (pos.index == CB_BTREE_HALF_ELEMENTS) {
				// New key is median.
				memcpy(new->elements, pos.node->elements + CB_BTREE_HALF_ELEMENTS, CB_BTREE_HALF_ELEMENTS * sizeof(*new->elements));
				if (right) {
					// Copy children
					memcpy(new->children + 1, pos.node->children + CB_BTREE_HALF_ELEMENTS + 1, CB_BTREE_HALF_ELEMENTS * sizeof(*new->children));
					// First child in right of the split becomes the right input node
					new->children[0] = right;
				}
				// Middle value is the inserted value
				midKeyValue = element;
			}else{
				// In new child.
				// Copy over first part of the new child
				if (pos.index > CB_BTREE_HALF_ELEMENTS + 1)
					memcpy(new->elements, pos.node->elements + CB_BTREE_HALF_ELEMENTS + 1, (pos.index - CB_BTREE_HALF_ELEMENTS - 1) * sizeof(*new->elements));
				// Copy in data
				new->elements[pos.index - CB_BTREE_HALF_ELEMENTS - 1] = element;
				// Copy over the last part of the new child. It seems so simple to visualise but coding this is very hard...
				memcpy(new->elements + pos.index - CB_BTREE_HALF_ELEMENTS, pos.node->elements + pos.index, (CB_BTREE_ELEMENTS - pos.index) * sizeof(*new->elements));
				// Copy children (Can be the confusing part)
				if (right){
					memcpy(new->children, pos.node->children + CB_BTREE_HALF_ELEMENTS + 1, (pos.index - CB_BTREE_HALF_ELEMENTS) * sizeof(*new->children)); // Includes left as previously
					new->children[pos.index - CB_BTREE_HALF_ELEMENTS] = right;
					// Rest of the children.
					if (CB_BTREE_ELEMENTS > pos.index)
						memcpy(new->children + pos.index - CB_BTREE_HALF_ELEMENTS + 1, pos.node->children + pos.index + 1, (CB_BTREE_ELEMENTS - pos.index) * sizeof(*new->children));
				}
				// Middle value
				midKeyValue = pos.node->elements[CB_BTREE_HALF_ELEMENTS];
			}
		}else{
			// In first child. This is the (supposedly) easy one.
			// Copy data to new child
			memcpy(new->elements, pos.node->elements + CB_BTREE_HALF_ELEMENTS, CB_BTREE_HALF_ELEMENTS * sizeof(*new->elements));
			// Insert key and data
			memmove(pos.node->elements + pos.index + 1, pos.node->elements + pos.index, (CB_BTREE_HALF_ELEMENTS - pos.index) * sizeof(*pos.node->elements));
			pos.node->elements[pos.index] = element;
			// Children...
			if (right) {
				memcpy(new->children, pos.node->children + CB_BTREE_HALF_ELEMENTS, (CB_BTREE_HALF_ELEMENTS + 1) * sizeof(*new->children)); // OK
				if (CB_BTREE_HALF_ELEMENTS > 1 + pos.index)
					memmove(pos.node->children + pos.index + 2, pos.node->children + pos.index + 1, (CB_BTREE_HALF_ELEMENTS - pos.index - 1) * sizeof(*new->children));
				// Insert right to inserted area
				pos.node->children[pos.index + 1] = right;
			}
			// Middle value
			midKeyValue = pos.node->elements[CB_BTREE_HALF_ELEMENTS];
		}
		// Move middle value to parent, if parent does not exist (ie. root) create new root.
		if (pos.node == self->root) {
			// Create new root
			self->root = malloc(sizeof(*self->root));
			self->root->numElements = 0;
			self->root->children[0] = pos.node; // Make left the left split node.
			pos.node = self->root;
			pos.index = 0;
		}else{
			// Position to add middle key value in parent is the same position of the old node in the parent children.
			if (pos.parentCursor)
				pos.node = pos.parentNodes[pos.parentCursor-1];
			else
				pos.node = self->root;
			pos.index = pos.parentPositions[pos.parentCursor--];
		}
		CBAssociativeArrayInsert(self, midKeyValue, pos, new);
	}
	assert(self->root->numElements <= 32);
}
bool CBAssociativeArrayIsEmpty(CBAssociativeArray * self){
	return self->root->numElements == 0;
}
bool CBAssociativeArrayIterate(CBAssociativeArray * self, CBPosition * it){
	// Look for child
	if (it->node->children[0]) {
		// Go to left-most in child
		if (it->node != self->root){
			it->parentNodes[it->parentIndex++] = it->node;
			it->parentCursor++;
		}
		it->parentPositions[it->parentIndex] = it->index + 1;
		it->node = it->node->children[it->index + 1];
		while (it->node->children[0]){
			if (it->node != self->root){
				it->parentNodes[it->parentIndex++] = it->node;
				it->parentCursor++;
			}
			it->parentPositions[it->parentIndex] = 0;
			it->node = it->node->children[0];
		}
		it->index = 0;
	}else if (it->index == it->node->numElements - 1) {
		for(;;){
			// If root then it is the end
			if (it->node == self->root)
				return true;
			// Move to parent
			if (it->parentIndex)
				it->node = it->parentNodes[it->parentIndex-1];
			else
				it->node = self->root;
			it->index = it->parentPositions[it->parentIndex];
			if (it->parentIndex)
				it->parentIndex--;
			if (it->index < it->node->numElements)
				// We can use this
				break;
			// Else continue onto next parent and so on in the loop
		}
	}else
		// Move along one
		it->index++;
	return false;
}
bool CBAssociativeArrayIterateBack(CBAssociativeArray * self, CBPosition * it){
	// Look for child
	if (it->node->children[0]) {
		// Go to right-most in child
		if (it->node != self->root){
			it->parentNodes[it->parentIndex++] = it->node;
			it->parentCursor++;
		}
		it->parentPositions[it->parentIndex] = it->index;
		it->node = it->node->children[it->index];
		while (it->node->children[0]){
			if (it->node != self->root){
				it->parentNodes[it->parentIndex++] = it->node;
				it->parentCursor++;
			}
			it->parentPositions[it->parentIndex] = it->node->numElements;
			it->node = it->node->children[it->node->numElements];
		}
		it->index = it->node->numElements - 1;
	}else if (it->index == 0) {
		for(;;){
			// If root then it is the end
			if (it->node == self->root)
				return true;
			// Move to parent
			if (it->parentIndex)
				it->node = it->parentNodes[it->parentIndex-1];
			else
				it->node = self->root;
			it->index = it->parentPositions[it->parentIndex];
			if (it->parentIndex)
				it->parentIndex--;
			if (it->index-- != 0)
				// We can use this
				break;
			// Else continue onto next parent and so on in the loop
		}
	}else
		// Move down one
		it->index--;
	return false;
}
bool CBAssociativeArrayNotEmpty(CBAssociativeArray * self){
	return self->root->numElements != 0;
}
void CBBTreeNodeBinarySearch(CBAssociativeArray * array, CBBTreeNode * node, void * key, CBFindResult * result){
	result->found = false;
	if (node->numElements){
		uint8_t left = 0;
		uint8_t right = node->numElements - 1;
		CBCompare cmp;
		while (left <= right) {
			result->position.index = (right+left)/2;
			cmp = array->compareFunc(array, key, node->elements[result->position.index]);
			if (cmp == CB_COMPARE_EQUAL) {
				result->found = true;
				break;
			}else if (cmp == CB_COMPARE_LESS_THAN){
				if (! result->position.index)
					break;
				right = result->position.index - 1;
			}else
				left = result->position.index + 1;
		}
		if (cmp == CB_COMPARE_MORE_THAN)
			result->position.index++;
	}else
		result->position.index = 0;
}
void * CBFindResultToPointer(CBFindResult res){
	return res.position.node->elements[res.position.index];
}
void CBFreeAssociativeArray(CBAssociativeArray * self){
	CBFreeBTreeNode(self->root, self->onFree, false);
}
void CBFreeBTreeNode(CBBTreeNode * self, void (*onFree)(void *), bool onlyChildren){
	if (self->children[0])
		for (uint8_t x = 0; x < self->numElements + 1; x++)
			CBFreeBTreeNode(self->children[x], onFree, false);
	if (onFree)
		for (uint8_t x = 0; x < self->numElements; x++)
			onFree(self->elements[x]);
	if (onlyChildren){
		// We are only freeing the children so reset this node
		for (uint8_t x = 0; x < self->numElements + 1; x++)
			self->children[x] = NULL;
		self->numElements = 0;
	}else
		free(self);
}
void CBInitAssociativeArray(CBAssociativeArray * self, CBCompare (*compareFunc)(CBAssociativeArray *, void *, void *), void * compareObject, void (*onFree)(void *)){
	self->root = malloc(sizeof(*self->root));
	self->root->numElements = 0;
	for (uint8_t x = 0; x < CB_BTREE_ELEMENTS + 1; x++)
		self->root->children[x] = NULL;
	self->compareFunc = compareFunc;
	self->compareObject = compareObject;
	self->onFree = onFree;
}
CBCompare CBKeyCompare(CBAssociativeArray * self, void * key1, void * key2){
	if (*(uint8_t *)key1 > *(uint8_t *)key2)
		return CB_COMPARE_MORE_THAN;
	if (*(uint8_t *)key1 < *(uint8_t *)key2)
		return CB_COMPARE_LESS_THAN;
	int cmp = memcmp(((uint8_t *)key1) + 1, ((uint8_t *)key2) + 1, *(uint8_t *)key1);
	if (cmp > 0)
		return CB_COMPARE_MORE_THAN;
	if (! cmp)
		return CB_COMPARE_EQUAL;
	return CB_COMPARE_LESS_THAN;
}
void * CBRangeIteratorGetPointer(CBRangeIterator * it){
	return it->pos.node->elements[it->pos.index];
}
