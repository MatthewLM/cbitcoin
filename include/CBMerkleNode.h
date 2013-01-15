//
//  CBMerkleNode.h
//  cbitcoin
//
//  Created by Matthew Mitchell on 28/08/2012.
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
 @brief A structure for a node in a merkle tree.
 */

#ifndef CBMERKLENODEH
#define CBMERKLENODEH

#include "CBByteArray.h"
#include "CBDependencies.h"

typedef struct CBMerkleNode CBMerkleNode;

/**
 @brief @see CBMerkleNode.h
 */
struct CBMerkleNode{
	uint8_t hash[32]; /**< The hash for this node. */
	CBMerkleNode * left; /**< The left child. NULL if leaf. */
	CBMerkleNode * right; /**< The right child. NULL if leaf. */
};

/**
 @brief Builds a Merkle tree from a list of hashes. In cases of duplication, "left" and "right" may refer to the same node.
 @param hashes A list of hashes as CBByteArrays to build the tree from.
 @param numHashes The number of hashes.
 @returns The root CBMerkleNode for the tree.
 */
CBMerkleNode * CBBuildMerkleTree(CBByteArray ** hashes, uint32_t numHashes);
/**
 @brief Frees a merkle tree from a given root.
 @param root The merkle tree root node.
 */
void CBFreeMerkleTree(CBMerkleNode * root);
/**
 @brief Gets a list of hashes for a level in a merkle tree. If the merkle tree's deepest level is smaller than specified by "level", the lowest level in the tree is returned.
 @param root The merkle tree root node.
 @param level The level to retrieve. Pass in a high number (use 255) to get the deepest level. Level 0 corresponds to the root which was passed in.
 @returns With nodes left to right, the memory block for this level of the merkle tree.
 */
CBMerkleNode * CBMerkleTreeGetLevel(CBMerkleNode * root, uint8_t level);

#endif
