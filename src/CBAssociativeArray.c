//
//  CBAssociativeArray.c
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
//

//  SEE HEADER FILE FOR DOCUMENTATION ??? Many optimisations can be done here.

#include "CBAssociativeArray.h"
#include <assert.h>

void CBAssociativeArrayDelete(CBAssociativeArray * self, CBFindResult pos){
	if (NOT pos.node->children[0]) {
		// Leaf
		CBBTreeNode * parent = pos.node->parent;
		if (pos.node->numElements > CB_BTREE_HALF_ORDER || NOT parent) {
			// Can simply remove this element. Nice and easy.
			if (--pos.node->numElements > pos.pos){
				// Move everything down to overwrite the removed element.
				memmove(pos.node->children + pos.pos, pos.node->children + pos.pos + 1, (pos.node->numElements - pos.pos + 1) * sizeof(*pos.node->children));
				memmove(pos.node->elements + pos.pos, pos.node->elements + pos.pos + 1, (pos.node->numElements - pos.pos) * sizeof(*pos.node->elements));
			}
		}else{
			// Underflow... Where things get complicated. Loop through merges all the way to the root
			for (;;){
				// We have a parent so we can check siblings.
				// First find the position of this node in the parent. ??? Worth additional pointers?
				// Search for second key in case the first has been taken by the parent.
				CBFindResult res = CBBTreeNodeBinarySearch(parent, pos.node->elements[1], self->compareFunc);
				CBBTreeNode * left;
				CBBTreeNode * right;
				if (res.pos) {
					// Create left sibling data
					left = parent->children[res.pos-1];
					// Can check left sibling
					if (left->numElements > CB_BTREE_HALF_ORDER) {
						// Can take from left
						// Move elements up to overwrite the element we are deleting
						if (pos.pos) { // o 0 ii 1 iii
							memmove(pos.node->elements + 1, pos.node->elements, pos.pos * sizeof(*pos.node->elements));
							memmove(pos.node->children + 1, pos.node->children, (pos.pos + 1) * sizeof(*pos.node->children));
						}
						// Lower left parent element to this node
						pos.node->elements[0] = parent->elements[res.pos-1];
						// Now move left sibling's far right element to parent
						parent->elements[res.pos-1] = left->elements[left->numElements-1];
						// Make left sibling's far right child the far left child of this node
						pos.node->children[0] = left->children[left->numElements];
						// Make this child's parent this node
						if (pos.node->children[0])
							((CBBTreeNode *)(pos.node->children[0]))->parent = pos.node;
						// Now remove left sibling's far right element and be done
						left->numElements--;
						return;
					}
				}
				if (res.pos < parent->numElements) {
					// Create right sibling data
					right = parent->children[res.pos+1];
					// Can check right sibling
					if (right->numElements > CB_BTREE_HALF_ORDER) {
						// Can take from right
						// Move elements down to overwrite the element we are deleting
						if (pos.node->numElements > pos.pos + 1) {
							memmove(pos.node->elements + pos.pos, pos.node->elements + pos.pos + 1, (pos.node->numElements - pos.pos - 1) * sizeof(*pos.node->elements));
							memmove(pos.node->children + pos.pos, pos.node->children + (pos.pos + 1), (pos.node->numElements - pos.pos) * sizeof(*pos.node->children));
						}
						// Lower right parent element to this node
						pos.node->elements[pos.node->numElements-1] = parent->elements[res.pos];
						// Now move right sibling's far left element to parent
						parent->elements[res.pos] = right->elements[0];
						// Make right sibling's far left child the far right child of this node
						pos.node->children[pos.node->numElements] = right->children[0];
						// Make this child's parent this node
						if (pos.node->children[0])
							((CBBTreeNode *)(pos.node->children[pos.node->numElements]))->parent = pos.node;
						// Now remove right sibling's far left element and be done
						memmove(right->children, right->children + 1, right->numElements * sizeof(*right->children));
						right->numElements--;
						memmove(right->elements, right->elements + 1, right->numElements * sizeof(*right->elements));
						return;
					}
				}
				// Take away element num.
				pos.node->numElements--;
				// Could not take from siblings... now for the merging. :-(
				if (NOT res.pos) {
					// We are merging right sibling into the node.
					// First move data down to overwrite deleted data o 0 i 1 ii
					if (pos.node->numElements > pos.pos)
						memmove(pos.node->elements + pos.pos, pos.node->elements + pos.pos + 1, (pos.node->numElements - pos.pos) * sizeof(*pos.node->elements));
					memmove(pos.node->children + pos.pos, pos.node->children + pos.pos + 1, (pos.node->numElements + 1) * sizeof(*pos.node->children));
					// Add parents mid value
					pos.node->elements[pos.node->numElements] = parent->elements[res.pos];
					// Now merge right sibling
					pos.node->numElements++;
					memmove(pos.node->elements + pos.node->numElements, right->elements, right->numElements * sizeof(*pos.node->elements));
					memmove(pos.node->children + pos.node->numElements, right->children, (right->numElements + 1) * sizeof(*right->children));
					// Change children parent pointers
					if (pos.node->children[0])
						for (uint8_t x = CB_BTREE_HALF_ORDER; x < CB_BTREE_ORDER + 1; x++)
							((CBBTreeNode *)(pos.node->children[x]))->parent = pos.node;
					// Adjust number of elements
					pos.node->numElements += right->numElements;
					// Free right node
					free(right);
					// Move node over to where the right sibling was so deleted part is on the left. ??? Change algorithm to merge differently.
					parent->children[res.pos+1] = parent->children[res.pos];
				}else{
					// We are merging the node into the left sibling.
					// Move over parent middle element
					left->elements[left->numElements] = parent->elements[res.pos-1];
					// Move over elements before deleted element.
					left->numElements++;
					if (pos.pos){
						memcpy(left->elements + left->numElements, pos.node->elements, pos.pos * sizeof(*left->elements));
						memcpy(left->children + left->numElements, pos.node->children, pos.pos * sizeof(*pos.node->children));
					}
					left->numElements += pos.pos;
					// Move over elements after the deleted element to the left node
					if (pos.node->numElements > pos.pos)
						memcpy(left->elements + left->numElements, pos.node->elements + pos.pos + 1, (pos.node->numElements - pos.pos) * sizeof(*left->elements));
					memcpy(left->children + left->numElements, pos.node->children + pos.pos + 1, (pos.node->numElements - pos.pos + 1) * sizeof(*pos.node->children));
					// Change children parent pointers
					if (left->children[0])
						for (uint8_t x = CB_BTREE_HALF_ORDER; x < CB_BTREE_ORDER + 1; x++)
							((CBBTreeNode *)(left->children[x]))->parent = left;
					// Adjust number of elements
					left->numElements += pos.node->numElements - pos.pos;
					// Free the node
					free(pos.node);
					// Move left sibling over to where the node was so deleted part is on the left.
					parent->children[res.pos] = parent->children[res.pos-1];
				}
				// If position is 0 then we are deleting to the right, else to the left.
				pos.pos = res.pos ? res.pos - 1 : res.pos;
				// Now delete the parent element and continue merge upto root if neccesary...
				if ((NOT parent->parent && parent->numElements > 1)
					|| (parent->parent && parent->numElements > CB_BTREE_HALF_ORDER)) {
					// Either a root and with more than one element to still remove or not root and more than half the minimum elements.
					// Therefore remove the element and be done.
					parent->numElements--;
					if (parent->numElements > pos.pos) {
						memmove(parent->elements + pos.pos, parent->elements + pos.pos + 1, (parent->numElements - pos.pos) * sizeof(*parent->elements));
						memmove(parent->children + pos.pos + 1, parent->children + pos.pos + 2, (parent->numElements - pos.pos) * sizeof(*parent->children));
					}
					return;
				}else if (NOT parent->parent){
					// The parent is the root and has 1 element. The root is now empty so we make it's newly merged children the root.
					self->root = res.pos ? left : pos.node;
					self->root->parent = NULL;
					free(parent);
					return;
				}else{
					// The parent is not root and has the minimum allowed elements. Therefore we need to merge again, going around the loop.
					pos.node = parent;
					parent = pos.node->parent;
				}
			}
		}
	}else{
		// Not leaf data, insert successor then delete successor from/in the right child.
		CBBTreeNode * child = pos.node->children[pos.pos + 1];
		// Now we have the right child, find left most element in the child.
		while (child->children[0])
			child = child->children[0];
		// Get left-most element and overwrite the element we are deleting with this left-most element.
		pos.node->elements[pos.pos] = child->elements[0];
		// Now delete the successive element.
		pos.node = child;
		pos.pos = 0;
		CBAssociativeArrayDelete(self, pos);
	}
}
CBFindResult CBAssociativeArrayFind(CBAssociativeArray * self, void * element){
	CBFindResult result;
	CBBTreeNode * node = self->root;
	for (;;) {
		// Do binary search on node
		result = CBBTreeNodeBinarySearch(node, element, self->compareFunc);
		if (result.found){
			// Found the data on this node.
			result.node = node;
			return result;
		}else{
			// We have not found the data on this node, if there is a child go to the child.
			if (node->children[result.pos])
				// Child exists, move to it
				node = node->children[result.pos];
			else{
				// The child doesn't exist. Return as not found with position for insertion.
				result.node = node;
				return result;
			}
		}
	}
}
bool CBAssociativeArrayGetElement(CBAssociativeArray * self, CBIterator * it, uint32_t index){
	if (NOT CBAssociativeArrayGetFirst(self, it))
		return false;
	// ??? Lazy method of iteration.
	for (uint32_t x = 0; x < index; x++)
		if (CBAssociativeArrayIterate(self, it))
			return false;
	return true;
}
bool CBAssociativeArrayGetFirst(CBAssociativeArray * self, CBIterator * it){
	if (NOT self->root->numElements)
		return false;
	it->node = self->root;
	it->pos = 0;
	while (it->node->children[0])
		it->node = it->node->children[0];
	return true;
}
bool CBAssociativeArrayInsert(CBAssociativeArray * self, void * element, CBFindResult pos, CBBTreeNode * right){
	// See if we can insert data in this node
	assert(NOT pos.found);
	if (pos.node->numElements < CB_BTREE_ORDER) {
		// Yes we can, do that.
		if (pos.node->numElements > pos.pos){
			// Move up the keys, data and children
			// . 0 . x . 1 . 2 . 3 .
			memmove(pos.node->elements + pos.pos + 1, pos.node->elements + pos.pos, (pos.node->numElements - pos.pos) * sizeof(*pos.node->elements));
			memmove(pos.node->children + pos.pos + 2, pos.node->children + pos.pos + 1, (pos.node->numElements - pos.pos) * sizeof(*pos.node->children));
		}
		// Copy over new key, data and children
		pos.node->elements[pos.pos] = element;
		pos.node->children[pos.pos + 1] = right;
		// Increase number of elements
		pos.node->numElements++;
		// The element has been added, we can stop here.
		return true;
	}else{
		// Nope, we need to split this node into two.
		CBBTreeNode * new = malloc(sizeof(*new));
		if (NOT new)
			return false;
		// Make both sides have half the order.
		new->numElements = CB_BTREE_HALF_ORDER;
		pos.node->numElements = CB_BTREE_HALF_ORDER;
		uint8_t * midKeyValue;
		if (pos.pos >= CB_BTREE_HALF_ORDER) {
			// Not in first child.
			if (pos.pos == CB_BTREE_HALF_ORDER) {
				// New key is median. Visualisation of median insertion:
				//
				//      . C  .  G .
				//     /     |     \
				//  .A.B.  .D.F.  .H.I.
				//
				// . C . E . G . = Too many when order is 2, therefore split again
				//    /     \
				//  .D.     .F.
				//
				//         . - E - .
				//        /         \
				//      .C.         .G.
				//     /   \       /   \
				//  .A.B.  .D.   .F.  .H.I.
				//
				memcpy(new->elements, pos.node->elements + CB_BTREE_HALF_ORDER, CB_BTREE_HALF_ORDER * sizeof(*new->elements));
				// Copy children
				memcpy(new->children + 1, pos.node->children + CB_BTREE_HALF_ORDER + 1, CB_BTREE_HALF_ORDER * sizeof(*new->children));
				// First child in right of the split becomes the right input node
				new->children[0] = right;
				// Middle value is the inserted value
				midKeyValue = element;
			}else{
				// In new child. Visualisation of order of 4:
				//
				//        . - E - . -- J -- . -- O -- . - T - .
				//       /        |         |         |        \
				//  .A.B.C.D. .F.G.H.I. .K.L.M.N. .P.Q.R.S. .U.V.W.X.
				//
				//  Add Y
				//
				//        . - E - . -- J -- . -- O -- . - T - .
				//       /        |         |         |        \
				//  .A.B.C.D. .F.G.H.I. .K.L.M.N. .P.Q.R.S. .U.V.W.X.Y. == Too many
				//
				//        . - E - . -- J -- . -- O -- . - T - . W .     == Too many
				//       /        |         |         |       |    \
				//  .A.B.C.D. .F.G.H.I. .K.L.M.N. .P.Q.R.S. .U.V. .X.Y.
				//
				//                  . ---------- O -------- .
				//                 /                         \
				//        . - E - . -- J -- .         . - T - . W .
				//       /        |         |         |       |    \
				//  .A.B.C.D. .F.G.H.I. .K.L.M.N. .P.Q.R.S. .U.V. .X.Y.
				//
				// 
				//
				// Copy over first part of the new child
				if (pos.pos > CB_BTREE_HALF_ORDER + 1)
					memcpy(new->elements, pos.node->elements + CB_BTREE_HALF_ORDER + 1, (pos.pos - CB_BTREE_HALF_ORDER - 1) * sizeof(*new->elements));
				// Copy in data
				new->elements[pos.pos - CB_BTREE_HALF_ORDER - 1] = element;
				// Copy over the last part of the new child. It seems so simple to visualise but coding this is very hard...
				memcpy(new->elements + pos.pos - CB_BTREE_HALF_ORDER, pos.node->elements + pos.pos, (CB_BTREE_ORDER - pos.pos) * sizeof(*new->elements));
				// Copy children (Can be the confusing part) o 0 i 1 ii 3 iii 4 iv
				memcpy(new->children, pos.node->children + CB_BTREE_HALF_ORDER + 1, (pos.pos - CB_BTREE_HALF_ORDER) * sizeof(*new->children)); // Includes left as previously
				new->children[pos.pos - CB_BTREE_HALF_ORDER] = right;
				// Rest of the children.
				if (CB_BTREE_ORDER > pos.pos)
					memcpy(new->children + pos.pos - CB_BTREE_HALF_ORDER + 1, pos.node->children + pos.pos + 1, (CB_BTREE_ORDER - pos.pos) * sizeof(*new->children));
				// Middle value
				midKeyValue = pos.node->elements[CB_BTREE_HALF_ORDER];
			}
		}else{
			// In first child. This is the (supposedly) easy one.
			// a 0 b 1 c 2 d 3 e
			// a 0 b I r  c 2 d 3 e
			// Copy data to new child
			memcpy(new->elements, pos.node->elements + CB_BTREE_HALF_ORDER, CB_BTREE_HALF_ORDER * sizeof(*new->elements));
			// Insert key and data
			memmove(pos.node->elements + pos.pos + 1, pos.node->elements + pos.pos, (CB_BTREE_HALF_ORDER - pos.pos) * sizeof(*pos.node->elements));
			pos.node->elements[pos.pos] = element;
			// Children...
			memcpy(new->children, pos.node->children + CB_BTREE_HALF_ORDER, (CB_BTREE_HALF_ORDER + 1) * sizeof(*new->children)); // OK
			if (CB_BTREE_HALF_ORDER > 1 + pos.pos)
				memmove(pos.node->children + pos.pos + 2, pos.node->children + pos.pos + 1, (CB_BTREE_HALF_ORDER - pos.pos - 1) * sizeof(*new->children));
			// Insert right to inserted area
			pos.node->children[pos.pos + 1] = right; // OK
			// Middle value
			midKeyValue = pos.node->elements[CB_BTREE_HALF_ORDER];
		}
		// Make the new node's children's parents reflect this new node
		if (new->children[0])
			for (uint8_t x = 0; x < CB_BTREE_HALF_ORDER + 1; x++) {
				CBBTreeNode * child = new->children[x];
				child->parent = new;
			}
		// Move middle value to parent, if parent does not exist (ie. root) create new root.
		if (NOT pos.node->parent) {
			// Create new root
			self->root = malloc(sizeof(*self->root));
			if (NOT self)
				return false;
			self->root->numElements = 0;
			self->root->parent = NULL;
			pos.node->parent = self->root;
			self->root->children[0] = pos.node; // Make left the left split node.
		}
		// New node requires that the parent be set
		new->parent = pos.node->parent;
		// Find position of value in parent and then insert.
		CBFindResult res = CBBTreeNodeBinarySearch(pos.node->parent, midKeyValue, self->compareFunc);
		res.node = pos.node->parent;
		return CBAssociativeArrayInsert(self, midKeyValue, res, new);
	}
}
bool CBAssociativeArrayIterate(CBAssociativeArray * self, CBIterator * it){
	// Look for child
	if (it->node->children[it->pos + 1]) {
		// Go to left-most in child
		it->node = it->node->children[it->pos + 1];
		while (it->node->children[0])
			it->node = it->node->children[0];
		it->pos = 0;
	}else{
		if (it->pos == it->node->numElements - 1) {
			// Get key to find next key for
			uint8_t * key = it->node->elements[it->pos];
			for(;;){
				// If root then it is the end
				if (NOT it->node->parent)
					return true;
				// Find position in parent
				CBFindResult res;
				res = CBBTreeNodeBinarySearch(it->node->parent, key, self->compareFunc);
				// Move to parent
				it->node = it->node->parent;
				it->pos = res.pos;
				if (it->pos < it->node->numElements)
					// We can use this
					break;
				// Else continue onto next parent and so on in the loop
			}
		}else
			// Move along one
			it->pos++;
	}
	return false;
}
CBFindResult CBBTreeNodeBinarySearch(CBBTreeNode * self, void * key, CBCompare (*compareFunc)(void *, void *)){
	CBFindResult res;
	res.found = false;
	if (NOT self->numElements){
		res.pos = 0;
		return res;
	}
	uint8_t left = 0;
	uint8_t right = self->numElements - 1;
	CBCompare cmp;
	while (left <= right) {
		res.pos = (right+left)/2;
		cmp = compareFunc(key,self->elements[res.pos]);
		if (cmp == CB_COMPARE_EQUAL) {
			res.found = true;
			break;
		}else if (cmp == CB_COMPARE_LESS_THAN){
			if (NOT res.pos)
				break;
			right = res.pos - 1;
		}else
			left = res.pos + 1;
	}
	if (cmp == CB_COMPARE_MORE_THAN)
		res.pos++;
	return res;
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
bool CBInitAssociativeArray(CBAssociativeArray * self, CBCompare (*compareFunc)(void *, void *), void (*onFree)(void *)){
	self->root = malloc(sizeof(*self->root));
	if (NOT self)
		return false;
	self->root->parent = NULL;
	self->root->numElements = 0;
	for (uint8_t x = 0; x < CB_BTREE_ORDER + 1; x++)
		self->root->children[x] = NULL;
	self->compareFunc = compareFunc;
	self->onFree = onFree;
	return true;
}
CBCompare CBKeyCompare(void * key1, void * key2){
	if (*(uint8_t *)key1 > *(uint8_t *)key2)
		return CB_COMPARE_MORE_THAN;
	if (*(uint8_t *)key1 < *(uint8_t *)key2)
		return CB_COMPARE_LESS_THAN;
	int cmp = memcmp(((uint8_t *)key1) + 1, ((uint8_t *)key2) + 1, *(uint8_t *)key1);
	if (cmp > 0)
		return CB_COMPARE_MORE_THAN;
	if (NOT cmp)
		return CB_COMPARE_EQUAL;
	return CB_COMPARE_LESS_THAN;
}
