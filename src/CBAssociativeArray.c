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

//  SEE HEADER FILE FOR DOCUMENTATION

#include "CBAssociativeArray.h"

CBFindResult CBAssociativeArrayFind(CBAssociativeArray * self, uint8_t * key){
	CBFindResult result;
	CBBTreeNode * node = self->root;
	for (;;) {
		// Do binary search on node
		result = CBBTreeNodeBinarySearch(node, key, self->keySize);
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
bool CBAssociativeArrayDelete(CBAssociativeArray * self, CBFindResult pos){
	uint8_t * keys = (uint8_t *)(pos.node + 1);
	uint8_t * data = keys + self->keySize * CB_BTREE_ORDER;
	if (NOT pos.node->children[0]) {
		// Leaf
		CBBTreeNode * parent = pos.node->parent;
		if (pos.node->numElements > CB_BTREE_HALF_ORDER || NOT parent) {
			// Can simply remove this element. Nice and easy.
			if (--pos.node->numElements > pos.pos){
				// Move everything down to overwrite the removed element.
				memmove(pos.node->children + pos.pos, pos.node->children + pos.pos + 1, (pos.node->numElements - pos.pos + 1) * sizeof(pos.node->children));
				memmove(keys + pos.pos * self->keySize, keys + (pos.pos + 1) * self->keySize, (pos.node->numElements - pos.pos) * self->keySize);
				memmove(data + pos.pos * self->dataSize, data + (pos.pos + 1) * self->dataSize, (pos.node->numElements - pos.pos) * self->dataSize);
			}
			return true;
		}else{
			// Underflow... Where things get complicated
			// We have a parent so we can check siblings.
			// First find the position of this node in the parent. ??? Worth additional pointers?
			CBFindResult res = CBBTreeNodeBinarySearch(parent, keys, self->keySize);
			uint8_t * parentKeys = (uint8_t *)(parent + 1);
			uint8_t * parentData = parentKeys + CB_BTREE_ORDER * self->keySize;
			CBBTreeNode * left = parent->children[res.pos-1];
			uint8_t * leftKeys = (uint8_t *)(left + 1);
			uint8_t * leftData = leftKeys + CB_BTREE_ORDER * self->keySize;
			CBBTreeNode * right = parent->children[res.pos+1];
			uint8_t * rightKeys = (uint8_t *)(right + 1);
			uint8_t * rightData = rightKeys + CB_BTREE_ORDER * self->keySize;
			if (res.pos) {
				// Can check left sibling
				if (left->numElements > CB_BTREE_HALF_ORDER) {
					// Can take from left
					// Move elements up to overwrite the element we are deleting
					if (pos.pos) {
						memmove(keys + self->keySize, keys, pos.pos * self->keySize);
						memmove(data + self->dataSize, data, pos.pos * self->dataSize);
					}
					// Lower left parent element to this node
					memcpy(keys, parentKeys + res.pos * self->keySize, self->keySize);
					memcpy(data, parentData + res.pos * self->dataSize, self->dataSize);
					// Now move left sibling's far right element to parent
					memcpy(parentKeys + res.pos * self->keySize, leftKeys + (left->numElements - 1) * self->keySize, self->keySize);
					memcpy(parentData + res.pos * self->dataSize, leftData + (left->numElements - 1) * self->dataSize, self->dataSize);
					// Now remove left sibling's far right element
					pos.node = left;
					pos.pos = left->numElements - 1;
					return CBAssociativeArrayDelete(self, pos);
				}
			}
			if (res.pos < parent->numElements) {
				// Can check right sibling
				if (right->numElements > CB_BTREE_HALF_ORDER) {
					// Can take from right
					// Move elements down to overwrite the element we are deleting
					if (pos.node->numElements > pos.pos + 1) {
						memmove(keys + pos.pos * self->keySize, keys + (pos.pos + 1) * self->keySize, (pos.node->numElements - pos.pos - 1) * self->keySize);
						memmove(data + pos.pos * self->dataSize, data + (pos.pos + 1) * self->dataSize, (pos.node->numElements - pos.pos - 1) * self->dataSize);
					}
					// Lower right parent element to this node
					memcpy(keys + (pos.node->numElements - 1) * self->keySize, parentKeys + (res.pos + 1) * self->keySize, self->keySize);
					memcpy(data + (pos.node->numElements - 1) * self->dataSize, parentData + (res.pos + 1) * self->dataSize, self->dataSize);
					// Now move right sibling's far left element to parent
					memcpy(parentKeys + (res.pos + 1) * self->keySize, rightKeys, self->keySize);
					memcpy(parentData + (res.pos + 1) * self->dataSize, rightData, self->dataSize);
					// Now remove right sibling's far left element
					pos.node = right;
					pos.pos = 0;
					return CBAssociativeArrayDelete(self, pos);
				}
			}
			// Could not take from siblings... now for the merging. :-(
			if (NOT res.pos) {
				// We are merging right sibling into the node, Make modifications so that this node is known as the left and the right is thought as the node.
				leftKeys = keys;
				leftData = data;
				left = pos.node;
				keys = rightKeys;
				data = rightData;
				pos.node = right;
			}else{
				// We are merging the node into the left sibling. Change parent position for node to now be used for the parent element that is copied into the merged node...
				res.pos--;
			}
			// Merge
			// Move over parent middle element
			memcpy(leftKeys + left->numElements * self->keySize, parentKeys + res.pos * self->keySize, self->keySize);
			memcpy(leftData + left->numElements * self->dataSize, parentData + res.pos * self->dataSize, self->dataSize);
			// Move over this node up to before the deleted element to the left node
			left->numElements++;
			if (pos.pos) {
				memcpy(leftKeys + left->numElements * self->keySize, keys, pos.pos * self->keySize);
				memcpy(leftData + left->numElements * self->dataSize, data, pos.pos * self->dataSize);
			}
			left->numElements += pos.pos;
			// Move over this node after the deleted element to the left node
			if (pos.node->numElements > pos.pos + 1) {
				memcpy(leftKeys + left->numElements * self->keySize, keys + (pos.pos + 1) * self->keySize, (pos.node->numElements - pos.pos - 1) * self->keySize);
				memcpy(leftData + left->numElements * self->dataSize, data + (pos.pos + 1) * self->dataSize, (pos.node->numElements - pos.pos - 1) * self->dataSize);
			}
			left->numElements += pos.node->numElements - pos.pos - 1;
			// Free right node
			free(pos.node);
			// Now delete the parent element and continue merge upto root if neccesary...
			IMPLEMENT THIS! ???
		}
	}else{
		// Not leaf data, insert successor then delete successor from/in the right child. This is the easiest bit of all!
		CBBTreeNode * rightChild = pos.node->children[pos.pos + 1];
		uint8_t * rightChildKeys = (uint8_t *)(rightChild + 1);
		memcpy(keys + pos.pos * self->keySize, rightChildKeys, self->keySize);
		memcpy(data + pos.pos * self->dataSize, rightChildKeys + CB_BTREE_ORDER * self->keySize, self->dataSize);
		// Now delete the successive element.
		pos.node = rightChild;
		pos.pos = 0;
		return CBAssociativeArrayDelete(self, pos);
	}
}
void * CBAssociativeArrayGetData(CBAssociativeArray * self, CBFindResult res){
	return (uint8_t *)(res.node + 1) + self->keySize * CB_BTREE_ORDER + res.pos * self->dataSize;
}
bool CBAssociativeArrayInsert(CBAssociativeArray * self, uint8_t * key, void * data, CBFindResult pos, CBBTreeNode * right){
	// See if we can insert data in this node
	uint8_t * keys = (uint8_t *)(pos.node + 1);
	uint8_t * dataElements = keys + self->keySize * CB_BTREE_ORDER;
	if (pos.node->numElements < CB_BTREE_ORDER) {
		// Yes we can, do that.
		if (pos.node->numElements > pos.pos){
			// Move up the keys, data and children
			// . 0 . x . 1 . 2 . 3 .
			memmove(keys + (pos.pos + 1) * self->keySize, keys + pos.pos * self->keySize, (pos.node->numElements - pos.pos) * self->keySize);
			memmove(dataElements + (pos.pos + 1) * self->dataSize, dataElements + pos.pos * self->dataSize, (pos.node->numElements - pos.pos) * self->dataSize);
			memmove(pos.node->children + pos.pos + 2, pos.node->children + pos.pos + 1, (pos.node->numElements - pos.pos) *  sizeof(*pos.node->children));
		}
		// Copy over new key, data and children
		memcpy(keys + pos.pos * self->keySize, key, self->keySize);
		memcpy(dataElements + pos.pos * self->dataSize, data, self->dataSize);
		pos.node->children[pos.pos + 1] = right;
		// Increase number of elements
		pos.node->numElements++;
		// The element has been added, we can stop here.
		return true;
	}else{
		// Nope, we need to split this node into two.
		CBBTreeNode * new = malloc(self->nodeSize);
		if (NOT new)
			return false;
		uint8_t * newKeys = (uint8_t *)(new + 1);
		uint8_t * newData = newKeys + self->keySize * CB_BTREE_ORDER;
		// Make both sides have half the order.
		new->numElements = CB_BTREE_HALF_ORDER;
		pos.node->numElements = CB_BTREE_HALF_ORDER;
		uint8_t * midKey;
		uint8_t * midVal;
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
				//  .D.    .F.
				//
				//         . - E - .
				//        /         \
				//      .C.         .G.
				//     /   \       /   \
				//  .A.B.  .D.   .F.  .H.I.
				//
				memcpy(newKeys, keys + CB_BTREE_HALF_ORDER * self->keySize, CB_BTREE_HALF_ORDER * self->keySize);
				memcpy(newData, dataElements + CB_BTREE_HALF_ORDER * self->dataSize, CB_BTREE_HALF_ORDER * self->dataSize);
				// Copy children
				memcpy(new->children + 1, pos.node->children + CB_BTREE_HALF_ORDER + 1, CB_BTREE_HALF_ORDER * sizeof(*new->children));
				// First child in right of the split becomes the right input node
				new->children[0] = right;
				// Middle value is the inserted value
				midKey = key;
				midVal = data;
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
				if (pos.pos > CB_BTREE_HALF_ORDER + 1){
					memcpy(newKeys, keys + (CB_BTREE_HALF_ORDER + 1) * self->keySize, (pos.pos - CB_BTREE_HALF_ORDER - 1) * self->keySize);
					memcpy(newData, dataElements + (CB_BTREE_HALF_ORDER + 1) * self->dataSize, (pos.pos - CB_BTREE_HALF_ORDER - 1)  * self->dataSize);
				}
				// Copy in data
				memcpy(newKeys + (pos.pos - CB_BTREE_HALF_ORDER - 1) * self->keySize, key, self->keySize);
				memcpy(newData + (pos.pos - CB_BTREE_HALF_ORDER - 1) * self->dataSize, data, self->dataSize);
				// Copy over the last part of the new child. It seems so simple to visualise but coding this is very hard...
				memcpy(newKeys + (pos.pos - CB_BTREE_HALF_ORDER) * self->keySize, keys + pos.pos * self->keySize, (CB_BTREE_ORDER - pos.pos) * self->keySize);
				memcpy(newData + (pos.pos - CB_BTREE_HALF_ORDER) * self->dataSize, dataElements + pos.pos * self->dataSize, (CB_BTREE_ORDER - pos.pos) * self->dataSize);
				// Copy children (Can be the confusing part)
				memcpy(new->children, pos.node->children + CB_BTREE_HALF_ORDER + 1, (pos.pos - CB_BTREE_HALF_ORDER) * sizeof(*new->children)); // Includes left as previously
				new->children[pos.pos - CB_BTREE_HALF_ORDER] = right;
				if (CB_BTREE_ORDER > pos.pos)
					memcpy(new->children, pos.node->children + pos.pos + 1, (CB_BTREE_ORDER - pos.pos) * sizeof(*new->children));
				// Middle value
				midKey = keys + CB_BTREE_HALF_ORDER * self->keySize;
				midVal = dataElements + CB_BTREE_HALF_ORDER * self->dataSize;
			}
		}else{
			// In first child. This is the easy one.
			// . 0 . 1 .  . 2 . 3 .
			// Copy data to new child
			memcpy(newKeys, keys + CB_BTREE_HALF_ORDER * self->keySize, CB_BTREE_HALF_ORDER * self->keySize);
			memcpy(newData, dataElements + CB_BTREE_HALF_ORDER * self->dataSize, CB_BTREE_HALF_ORDER * self->dataSize);
			memcpy(new->children, pos.node->children + CB_BTREE_HALF_ORDER, (CB_BTREE_HALF_ORDER + 1) * sizeof(*new->children));
			// Insert key and data
			memmove(keys + (pos.pos + 1) * self->keySize, keys + pos.pos * self->keySize, (CB_BTREE_HALF_ORDER - pos.pos) * self->keySize);
			memmove(dataElements + (pos.pos + 1) * self->dataSize, dataElements + pos.pos * self->dataSize, (CB_BTREE_HALF_ORDER - pos.pos) * self->dataSize);
			memmove(pos.node->children + pos.pos + 2, pos.node->children + pos.pos + 1, (CB_BTREE_HALF_ORDER - pos.pos) * self->dataSize);
			memcpy(keys + pos.pos * self->keySize, key, self->keySize);
			memcpy(dataElements + pos.pos * self->dataSize, data, self->dataSize);
			// Insert right to inserted area
			pos.node->children[pos.pos + 1] = right;
			// Middle value
			midKey = keys + CB_BTREE_HALF_ORDER * self->keySize;
			midVal = dataElements + CB_BTREE_HALF_ORDER * self->dataSize;
		}
		// Move middle value to parent, if parent does not exist (ie. root) create new root.
		if (NOT pos.node->parent) {
			// Create new root
			self->root = malloc(self->nodeSize);
			if (NOT self->root)
				return false;
			self->root->numElements = 0;
			self->root->parent = NULL;
			pos.node->parent = self->root;
			self->root->children[0] = pos.node; // Make left the left split node.
		}
		// New node requires that the parent be set
		new->parent = pos.node->parent;
		// Find position of value in parent and then insert.
		CBFindResult res = CBBTreeNodeBinarySearch(pos.node->parent, midKey, self->keySize);
		res.node = pos.node->parent;
		return CBAssociativeArrayInsert(self, midKey, midVal, res, new);
	}
}
CBFindResult CBBTreeNodeBinarySearch(CBBTreeNode * self, uint8_t * key, uint8_t keySize){
	CBFindResult res;
	res.found = false;
	if (NOT self->numElements) {
		res.pos = 0;
		return res;
	}
	uint8_t left = 0;
	uint8_t right = self->numElements - 1;
	uint8_t * keys = (uint8_t *)(self + 1);
	int cmp;
	while (left <= right) {
		res.pos = (right+left)/2;
		cmp = memcmp(key, keys + res.pos * keySize, keySize);
		if (cmp == 0) {
			res.found = true;
			break;
		}else if (cmp < 0){
			if (NOT res.pos)
				break;
			right = res.pos - 1;
		}else
			left = res.pos + 1;
	}
	if (cmp > 0)
		res.pos++;
	return res;
}
void CBFreeAssociativeArray(CBAssociativeArray * self){
	CBFreeBTreeNode(self->root);
}
void CBFreeBTreeNode(CBBTreeNode * self){
	for (uint8_t x = 0; x < self->numElements + 1; x++)
		if (self->children[x])
			CBFreeBTreeNode(self->children[x]);
	free(self);
}
bool CBInitAssociativeArray(CBAssociativeArray * self, uint8_t keySize, uint8_t dataSize){
	self->keySize = keySize;
	self->dataSize = dataSize;
	self->nodeSize = sizeof(*self->root) + (keySize + dataSize) * CB_BTREE_ORDER;
	self->root = malloc(self->nodeSize);
	if (NOT self->root)
		return false;
	self->root->parent = NULL;
	self->root->numElements = 0;
	for (uint8_t x = 0; x < CB_BTREE_ORDER + 1; x++)
		self->root->children[x] = NULL;
	return true;
}
