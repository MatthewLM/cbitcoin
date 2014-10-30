//
//  CBMerkleNode.h
//  cbitcoin
//
//  Created by Matthew Mitchell on 28/08/2012.
//  Copyright (c) 2012 Matthew Mitchell
//  
//  This file is part of cbitcoin. It is subject to the license terms
//  in the LICENSE file found in the top-level directory of this
//  distribution and at http://www.cbitcoin.com/license.html. No part of
//  cbitcoin, including this file, may be copied, modified, propagated,
//  or distributed except according to the terms contained in the
//  LICENSE file.

//  SEE HEADER FILE FOR DOCUMENTATION

#include "CBMerkleNode.h"

CBMerkleNode * CBBuildMerkleTree(CBByteArray ** hashes, uint32_t numHashes){
	CBMerkleNode * level = malloc(numHashes * sizeof(*level)); // Nodes on a level of the tree for processing.
	// Create nodes from the CBByteArray hashes
	for (uint32_t x = 0; x < numHashes; x++) {
		memcpy(level[x].hash, CBByteArrayGetData(hashes[x]), 32);
		level[x].left = NULL;
		level[x].right = NULL;
	}
	// Build each level upwards to the root
	uint8_t hash[32];
	uint8_t cat[64];
	CBMerkleNode * nextLevel = malloc((numHashes + 1)/2 * sizeof(*level));
	for (uint32_t x = 0;;) {
		nextLevel[x/2].left = level + x;
		if (x == numHashes - 1)
			nextLevel[x/2].right = level + x;
		else
			nextLevel[x/2].right = level + x + 1;
		memcpy(cat, nextLevel[x/2].left->hash, 32);
		memcpy(cat + 32, nextLevel[x/2].right->hash, 32);
		// Double SHA256
		CBSha256(cat, 64, hash);
		CBSha256(hash, 32, nextLevel[x/2].hash);
		x += 2;
		if (x >= numHashes) {
			// Finished level
			if (x > numHashes)
				// The number of hashes was odd. Increment to even
				numHashes++;
			numHashes /= 2;
			// Move to next level
			level = nextLevel;
			if (numHashes == 1)
				// Done, got the single root hash
				break;
			x = 0;
			nextLevel = malloc((numHashes + 1)/2 * sizeof(*level));
		}
	}
	// Return last level which contains only the root node.
	return level;
}
void CBFreeMerkleTree(CBMerkleNode * root){
	// Free all levels along the far left
	CBMerkleNode * next;
	for (CBMerkleNode * node = root; node != NULL; node = next) {
		next = node->left;
		free(node);
	}
}
CBMerkleNode * CBMerkleTreeGetLevel(CBMerkleNode * root, uint8_t level){
	for (uint8_t x = 0; x < level; x++) {
		if (root->left == NULL)
			return root;
		root = root->left;
	}
	return root;
}
