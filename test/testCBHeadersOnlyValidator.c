//
//  testCBHeadersOnlyValidator.c
//  cbitcoin
//
//  Created by Matthew Mitchell on 06/02/2013.
//  Copyright (c) 2012 Matthew Mitchell
//
//  This file is part of cbitcoin. It is subject to the license terms
//  in the LICENSE file found in the top-level directory of this
//  distribution and at http://www.cbitcoin.com/license.html. No part of
//  cbitcoin, including this file, may be copied, modified, propagated,
//  or distributed except according to the terms contained in the
//  LICENSE file.

#include "CBValidator.h"
#include "CBBlockChainStorage.h"
#include <stdarg.h>
#include <stdio.h>

static struct {
    uint8_t extranonce;
    uint32_t nonce;
} blockInfo[100] = {
    {4, 0xa4a3e223}, {2, 0x15c32f9e}, {1, 0x0375b547}, {1, 0x7004a8a5}, 
    {2, 0xce440296}, {2, 0x52cfe198}, {1, 0x77a72cd0}, {2, 0xbb5d6f84}, 
    {2, 0x83f30c2c}, {1, 0x48a73d5b}, {1, 0xef7dcd01}, {2, 0x6809c6c4}, 
    {2, 0x0883ab3c}, {1, 0x087bbbe2}, {2, 0x2104a814}, {2, 0xdffb6daa}, 
    {1, 0xee8a0a08}, {2, 0xba4237c1}, {1, 0xa70349dc}, {1, 0x344722bb}, 
    {3, 0xd6294733}, {2, 0xec9f5c94}, {2, 0xca2fbc28}, {1, 0x6ba4f406}, 
    {2, 0x015d4532}, {1, 0x6e119b7c}, {2, 0x43e8f314}, {2, 0x27962f38}, 
    {2, 0xb571b51b}, {2, 0xb36bee23}, {2, 0xd17924a8}, {2, 0x6bc212d9}, 
    {1, 0x630d4948}, {2, 0x9a4c4ebb}, {2, 0x554be537}, {1, 0xd63ddfc7}, 
    {2, 0xa10acc11}, {1, 0x759a8363}, {2, 0xfb73090d}, {1, 0xe82c6a34}, 
    {1, 0xe33e92d7}, {3, 0x658ef5cb}, {2, 0xba32ff22}, {5, 0x0227a10c}, 
    {1, 0xa9a70155}, {5, 0xd096d809}, {1, 0x37176174}, {1, 0x830b8d0f}, 
    {1, 0xc6e3910e}, {2, 0x823f3ca8}, {1, 0x99850849}, {1, 0x7521fb81}, 
    {1, 0xaacaabab}, {1, 0xd645a2eb}, {5, 0x7aea1781}, {5, 0x9d6e4b78}, 
    {1, 0x4ce90fd8}, {1, 0xabdc832d}, {6, 0x4a34f32a}, {2, 0xf2524c1c}, 
    {2, 0x1bbeb08a}, {1, 0xad47f480}, {1, 0x9f026aeb}, {1, 0x15a95049}, 
    {2, 0xd1cb95b2}, {2, 0xf84bbda5}, {1, 0x0fa62cd1}, {1, 0xe05f9169}, 
    {1, 0x78d194a9}, {5, 0x3e38147b}, {5, 0x737ba0d4}, {1, 0x63378e10}, 
    {1, 0x6d5f91cf}, {2, 0x88612eb8}, {2, 0xe9639484}, {1, 0xb7fabc9d}, 
    {2, 0x19b01592}, {1, 0x5a90dd31}, {2, 0x5bd7e028}, {2, 0x94d00323}, 
    {1, 0xa9b9c01a}, {1, 0x3a40de61}, {1, 0x56e7eec7}, {5, 0x859f7ef6}, 
    {1, 0xfd8e5630}, {1, 0x2b0c9f7f}, {1, 0xba700e26}, {1, 0x7170a408},
    {1, 0x70de86a8}, {1, 0x74d64cd5}, {1, 0x49e738a1}, {2, 0x6910b602}, 
    {0, 0x643c565f}, {1, 0x54264b3f}, {2, 0x97ea6396}, {2, 0x55174459}, 
    {2, 0x03e8779a}, {1, 0x98f34d8f}, {1, 0xc07b2b07}, {1, 0xdfe29668}, 
};

void CBLogError(char * format, ...){
	va_list argptr;
    va_start(argptr, format);
    vfprintf(stderr, format, argptr);
    va_end(argptr);
	printf("\n");
}

int main(){
	remove("./blk_log.dat");
	remove("./blk_0.dat");
	remove("./blk_1.dat");
	remove("./blk_2.dat");
	// Create validator
	uint64_t storage = CBNewBlockChainStorage("./");
	CBValidator * validator = CBNewValidator(storage, CB_VALIDATOR_HEADERS_ONLY);
	if (! validator) {
		printf("VALIDATOR INIT FAIL\n");
		return 1;
	}
	CBReleaseObject(validator);
	CBFreeBlockChainStorage(storage);
	// Now create it again. It should load the data.
	storage = CBNewBlockChainStorage("./");
	validator = CBNewValidator(storage, CB_VALIDATOR_HEADERS_ONLY);
	if (! validator) {
		printf("VALIDATOR LOAD FROM FILE FAIL\n");
		return 1;
	}
	// Now verify that the data is correct.
	if (validator->numOrphans > 0){
		printf("ORPHAN NUM FAIL\n");
		return 1;
	}
	if(validator->numBranches != 1){
		printf("BRANCH NUM FAIL\n");
		return 1;
	}
	if (validator->mainBranch != 0){
		printf("MAIN BRANCH FAIL\n");
		return 1;
	}
	if (validator->branches[0].lastRetargetTime != 1231006505){
		printf("LAST RETARGET FAIL\n");
		return 1;
	}
	if (validator->branches[0].numBlocks != 1){
		printf("NUM BLOCKS FAIL\n");
		return 1;
	}
	if (validator->branches[0].parentBranch != 0){
		printf("PARENT BRANCH FAIL\n");
		return 1;
	}
	if (validator->branches[0].startHeight != 0){
		printf("START HEIGHT FAIL\n");
		return 1;
	}
	if (validator->branches[0].work.length != 1){
		printf("WORK LENGTH FAIL\n");
		return 1;
	}
	if(validator->branches[0].work.data[0] != 0){
		printf("WORK VAL FAIL\n");
		return 1;
	}
	// Try loading the genesis block
	CBBlock * block = CBBlockChainStorageLoadBlock(validator, 0, 0);
	if (! block) {
		printf("GENESIS RETRIEVE FAIL\n");
		return 1;
	}
	CBBlockDeserialise(block, false);
	if (memcmp(CBBlockGetHash(block), (uint8_t []){0x6F, 0xE2, 0x8C, 0x0A, 0xB6, 0xF1, 0xB3, 0x72, 0xC1, 0xA6, 0xA2, 0x46, 0xAE, 0x63, 0xF7, 0x4F, 0x93, 0x1E, 0x83, 0x65, 0xE1, 0x5A, 0x08, 0x9C, 0x68, 0xD6, 0x19, 0x00, 0x00, 0x00, 0x00, 0x00}, 32)) {
		printf("GENESIS HASH FAIL\n");
		return 1;
	}
	if (memcmp(CBByteArrayGetData(block->merkleRoot), (uint8_t []){0x3B, 0xA3, 0xED, 0xFD, 0x7A, 0x7B, 0x12, 0xB2, 0x7A, 0xC7, 0x2C, 0x3E, 0x67, 0x76, 0x8F, 0x61, 0x7F, 0xC8, 0x1B, 0xC3, 0x88, 0x8A, 0x51, 0x32, 0x3A, 0x9F, 0xB8, 0xAA, 0x4B, 0x1E, 0x5E, 0x4A}, 32)) {
		printf("GENESIS MERKLE ROOT FAIL\n");
		return 1;
	}
	if (block->nonce != 2083236893) {
		printf("GENESIS NOUNCE FAIL\n");
		return 1;
	}
	if (block->target != CB_MAX_TARGET) {
		printf("GENESIS TARGET FAIL\n");
		return 1;
	}
	if (block->time != 1231006505) {
		printf("GENESIS TIME FAIL\n");
		return 1;
	}
	if (block->version != 1) {
		printf("GENESIS VERSION FAIL\n");
		return 1;
	}
	CBReleaseObject(block);
	// Test validating a correct block onto the genesis block (This one is taken off the main bitcoin production chain)
	CBBlock * block1 = CBNewBlock();
	block1->version = 1;
	block1->prevBlockHash = CBNewByteArrayWithDataCopy((uint8_t []){0x6F, 0xE2, 0x8C, 0x0A, 0xB6, 0xF1, 0xB3, 0x72, 0xC1, 0xA6, 0xA2, 0x46, 0xAE, 0x63, 0xF7, 0x4F, 0x93, 0x1E, 0x83, 0x65, 0xE1, 0x5A, 0x08, 0x9C, 0x68, 0xD6, 0x19, 0x00, 0x00, 0x00, 0x00, 0x00}, 32);
	block1->merkleRoot = CBNewByteArrayWithDataCopy((uint8_t []){0x98, 0x20, 0x51, 0xfD, 0x1E, 0x4B, 0xA7, 0x44, 0xBB, 0xBE, 0x68, 0x0E, 0x1F, 0xEE, 0x14, 0x67, 0x7B, 0xA1, 0xA3, 0xC3, 0x54, 0x0B, 0xF7, 0xB1, 0xCD, 0xB6, 0x06, 0xE8, 0x57, 0x23, 0x3E, 0x0E}, 32);
	block1->time = 1231469665;
	block1->target = CB_MAX_TARGET;
	block1->nonce = 2573394689;
	block1->transactionNum = 1;
	CBGetMessage(block1)->bytes = CBNewByteArrayOfSize(CBBlockCalculateLength(block1, false));
	CBBlockSerialise(block1, false, false);
	// Made block now proceed with the test.
	CBBlockProcessResult res = CBValidatorProcessBlock(validator, block1, 1349643202);
	if (res.status != CB_BLOCK_STATUS_MAIN) {
		printf("MAIN CHAIN ADD FAIL\n");
		return 1;
	}
	// Check validator data is correct, after closing and loading data
	CBReleaseObject(validator);
	CBFreeBlockChainStorage(storage);
	storage = CBNewBlockChainStorage("./");
	validator = CBNewValidator(storage, CB_VALIDATOR_HEADERS_ONLY);
	if (! validator){
		printf("BLOCK ONE LOAD FROM FILE FAIL\n");
		return 1;
	}
	if (validator->mainBranch != 0) {
		printf("BLOCK ONE MAIN BRANCH FAIL\n");
		return 1;
	}
	if (validator->numBranches != 1) {
		printf("BLOCK ONE MUM BRANCHES FAIL\n");
		return 1;
	}
	if (validator->numOrphans > 0) {
		printf("BLOCK ONE MUM ORPHANS FAIL\n");
		return 1;
	}
	if (validator->branches[0].lastRetargetTime != 1231006505) {
		printf("BLOCK ONE LAST RETARGET TIME FAIL\n");
		return 1;
	}
	if (validator->branches[0].numBlocks != 2) {
		printf("BLOCK ONE NUM REFS FAIL\n");
		return 1;
	}
	CBReleaseObject(block1);
	if (validator->branches[0].startHeight != 0) {
		printf("BLOCK ONE START HEIGHT FAIL\n");
		return 1;
	}
	if (validator->branches[0].work.length != 5) {
		printf("BLOCK ONE WORK LENGTH FAIL\n");
		return 1;
	}
	if (memcmp(validator->branches[0].work.data, (uint8_t []){0x01, 0x00, 0x01, 0x00, 0x01}, 5)) {
		printf("BLOCK ONE WORK FAIL\n");
		return 1;
	}
	// Try to load block
	block1 = CBBlockChainStorageLoadBlock(validator, 1, 0);
	CBBlockDeserialise(block1, false);
	if (! block1) {
		printf("BLOCK ONE LOAD FAIL\n");
		return 1;
	}
	if (memcmp(CBByteArrayGetData(block1->merkleRoot), (uint8_t []){0x98, 0x20, 0x51, 0xfD, 0x1E, 0x4B, 0xA7, 0x44, 0xBB, 0xBE, 0x68, 0x0E, 0x1F, 0xEE, 0x14, 0x67, 0x7B, 0xA1, 0xA3, 0xC3, 0x54, 0x0B, 0xF7, 0xB1, 0xCD, 0xB6, 0x06, 0xE8, 0x57, 0x23, 0x3E, 0x0E}, 32)) {
		printf("BLOCK ONE MERKLE ROOT FAIL\n");
		return 1;
	}
	if (block1->nonce != 2573394689) {
		printf("BLOCK ONE NOUNCE FAIL\n");
		return 1;
	}
	if (memcmp(CBByteArrayGetData(block1->prevBlockHash), (uint8_t []){0x6F, 0xE2, 0x8C, 0x0A, 0xB6, 0xF1, 0xB3, 0x72, 0xC1, 0xA6, 0xA2, 0x46, 0xAE, 0x63, 0xF7, 0x4F, 0x93, 0x1E, 0x83, 0x65, 0xE1, 0x5A, 0x08, 0x9C, 0x68, 0xD6, 0x19, 0x00, 0x00, 0x00, 0x00, 0x00}, 32)) {
		printf("BLOCK ONE PREV HASH FAIL\n");
		return 1;
	}
	if (block1->target != CB_MAX_TARGET) {
		printf("BLOCK ONE TARGET FAIL\n");
		return 1;
	}
	if (block1->time != 1231469665) {
		printf("BLOCK ONE TIME FAIL\n");
		return 1;
	}
	if (block1->transactionNum != 1) {
		printf("BLOCK ONE TX NUM FAIL\n");
		return 1;
	}
	if (block1->version != 1) {
		printf("BLOCK ONE VERSION FAIL\n");
		return 1;
	}
	// Test duplicate add.
	res = CBValidatorProcessBlock(validator, block1, 1349643202);
	if (res.status != CB_BLOCK_STATUS_DUPLICATE) {
		printf("MAIN CHAIN ADD DUPLICATE FAIL\n");
		return 1;
	}
	// Test block with bad PoW.
	CBBlock * testBlock = CBNewBlock();
	testBlock->version = 1;
	testBlock->prevBlockHash = CBNewByteArrayWithDataCopy((uint8_t []){0x6F, 0xE2, 0x8C, 0x0A, 0xB6, 0xF1, 0xB3, 0x72, 0xC1, 0xA6, 0xA2, 0x46, 0xAE, 0x63, 0xF7, 0x4F, 0x93, 0x1E, 0x83, 0x65, 0xE1, 0x5A, 0x08, 0x9C, 0x68, 0xD6, 0x19, 0x00, 0x00, 0x00, 0x00, 0x00}, 32);
	testBlock->merkleRoot = CBNewByteArrayWithDataCopy((uint8_t []){0x98, 0x20, 0x51, 0xfD, 0x1E, 0x4B, 0xA7, 0x44, 0xBB, 0xBE, 0x68, 0x0E, 0x1F, 0xEE, 0x14, 0x67, 0x7B, 0xA1, 0xA3, 0xC3, 0x54, 0x0B, 0xF7, 0xB1, 0xCD, 0xB6, 0x06, 0xE8, 0x57, 0x23, 0x3E, 0x0E}, 32);
	testBlock->time = 1231469664;
	testBlock->target = CB_MAX_TARGET;
	testBlock->nonce = 2573394689;
	testBlock->transactionNum = 1;
	CBGetMessage(testBlock)->bytes = CBNewByteArrayOfSize(CBBlockCalculateLength(testBlock, false));
	CBBlockSerialise(testBlock, false, false);
	// Made block now proceed with the test.
	res = CBValidatorProcessBlock(validator, testBlock, 1349643202);
	if (res.status != CB_BLOCK_STATUS_BAD) {
		printf("BAD POW ADD FAIL\n");
		return 1;
	}
	// Add 100 blocks to test
	CBBlock * theBlocks[100];
	CBByteArray * prevHash = CBNewByteArrayWithDataCopy((uint8_t []){0x6F, 0xE2, 0x8C, 0x0A, 0xB6, 0xF1, 0xB3, 0x72, 0xC1, 0xA6, 0xA2, 0x46, 0xAE, 0x63, 0xF7, 0x4F, 0x93, 0x1E, 0x83, 0x65, 0xE1, 0x5A, 0x08, 0x9C, 0x68, 0xD6, 0x19, 0x00, 0x00, 0x00, 0x00, 0x00}, 32);
	CBByteArray * nullHash = CBNewByteArrayOfSize(32);
	memset(CBByteArrayGetData(nullHash), 0, 32);
	uint32_t time = 1231006506;
	for (uint8_t x = 0; x < 100; x++) {
		block = CBNewBlock();
		block->version = 1;
		if (x == 1 || x == 3 || ! ((x-1) % 6))
			time++;
		block->time = time;
		block->transactionNum = 1;
		block->transactions = malloc(sizeof(*block->transactions));
		block->transactions[0] = CBNewTransaction(0, 1);
		CBScript * nullScript = CBNewScriptOfSize(0);
		CBScript * inScript = CBNewScriptOfSize(2);
		CBTransactionTakeInput(block->transactions[0], 
							   CBNewTransactionInput(inScript, CB_TX_INPUT_FINAL, nullHash, 0xFFFFFFFF));
		CBReleaseObject(inScript);
		CBTransactionTakeOutput(block->transactions[0], CBNewTransactionOutput(5000000000, nullScript));
		CBReleaseObject(nullScript);
		block->target = CB_MAX_TARGET;
		block->prevBlockHash = prevHash;
		CBGetMessage(block)->bytes = CBNewByteArrayOfSize(82);
		CBGetMessage(block->transactions[0])->bytes = CBNewByteArrayOfSize(62);
		CBByteArraySetByte(block->transactions[0]->inputs[0]->scriptObject, 0, blockInfo[x].extranonce);
		CBByteArraySetByte(block->transactions[0]->inputs[0]->scriptObject, 1, x);
		block->nonce = blockInfo[x].nonce;
		CBTransactionSerialise(block->transactions[0], true);
		block->merkleRoot = CBNewByteArrayWithDataCopy(CBTransactionGetHash(block->transactions[0]), 32);
		CBBlockSerialise(block, false, false);
		theBlocks[x] = block;
		prevHash = CBNewByteArrayWithDataCopy(CBBlockGetHash(block), 32);
	}
	for (uint8_t x = 0; x < 102; x++) {
		uint8_t y = x;
		if (x < 22)
			y += 3;
		else if (x < 27)
			y -= 22;
		else
			y -= 2;
		printf("PROCESSING BLOCK %u\n", y);
		res = CBValidatorProcessBlock(validator, theBlocks[y], 1230999326);
		if (x < 22){
			if (res.status != CB_BLOCK_STATUS_ORPHAN) {
				printf("ORPHAN FAIL AT %u\n", y);
				return 1;
			}
		}else if (x == 22){
			if (res.status != CB_BLOCK_STATUS_SIDE) {
				printf("SIDE FAIL AT %u\n", y);
				return 1;
			}
			if (res.data.sideBranch != 1) {
				printf("SIDE DATA FAIL AT %u\n", y);
				return 1;
			}
		}else if (x == 23){
			if (res.status != CB_BLOCK_STATUS_REORG) {
				printf("REORG FAIL AT %u\n", y);
				return 1;
			}
			if (res.data.reorgData.blockHeight != 1) {
				printf("REORG BLOCK HEIGHT FAIL AT %u\n", y);
				return 1;
			}
			if (res.data.reorgData.start.chainPathIndex != 0) {
				printf("REORG START CHAIN PATH INDEX FAIL AT %u\n", y);
				return 1;
			}
			if (res.data.reorgData.start.blockIndex != 0) {
				printf("REORG START BLOCK INDEX FAIL AT %u\n", y);
				return 1;
			}
			if (res.data.reorgData.newChain.numBranches != 2) {
				printf("REORG NEW CHAIN NUM BRANCHES FAIL AT %u\n", y);
				return 1;
			}
			if (res.data.reorgData.newChain.points[0].branch != 1) {
				printf("REORG NEW CHAIN POINT 0 BRANCH FAIL AT %u\n", y);
				return 1;
			}
			if (res.data.reorgData.newChain.points[0].blockIndex != 1) {
				printf("REORG NEW CHAIN POINT 0 BLOCK INDEX FAIL AT %u\n", y);
				return 1;
			}
			if (res.data.reorgData.newChain.points[1].branch != 0) {
				printf("REORG NEW CHAIN POINT 1 BRANCH FAIL AT %u\n", y);
				return 1;
			}
			if (res.data.reorgData.newChain.points[1].blockIndex != 0) {
				printf("REORG NEW CHAIN POINT 1 BLOCK INDEX FAIL AT %u\n", y);
				return 1;
			}
		}else if (x == 26){
			if (res.status != CB_BLOCK_STATUS_MAIN_WITH_ORPHANS) {
				printf("MAIN WITH ORPHANS FAIL AT %u\n", y);
				return 1;
			}
			if (res.data.numOrphansAdded != 20) {
				printf("MAIN WITH ORPHANS NUM ADDED FAIL AT %u\n", y);
				return 1;
			}
		}else if (res.status != CB_BLOCK_STATUS_MAIN) {
			// Reorg occurs at y == 1
			printf("MAIN FAIL AT %u\n", y);
			return 1;
		}
		if (x > 1)
			CBReleaseObject(theBlocks[y]);
	}
	// Now do tests without valid PoW
	validator->flags |= CB_VALIDATOR_DISABLE_POW_CHECK;
	// Test the disabler works
	res = CBValidatorProcessBlock(validator, testBlock, 1349643202);
	if (res.status != CB_BLOCK_STATUS_SIDE) {
		printf("POW CHECK DISABLE FAIL\n");
		return 1;
	}
	if (res.data.sideBranch != 2) {
		printf("POW CHECK DISABLE DATA FAIL\n");
		return 1;
	}
	// No transactions
	testBlock->transactionNum = 0;
	testBlock->time--;
	CBBlockSerialise(testBlock, false, false);
	res = CBValidatorProcessBlock(validator, testBlock, 1349643202);
	if (res.status != CB_BLOCK_STATUS_BAD) {
		printf("NO TXS FAIL\n");
		return 1;
	}
	// Bad target
	testBlock->transactionNum = 1;
	testBlock->target = CB_MAX_TARGET - 1;
	CBBlockSerialise(testBlock, false, false);
	if (CBValidatorProcessBlock(validator, testBlock, 1349643202).status != CB_BLOCK_STATUS_BAD) {
		printf("BAD TARGET FAIL\n");
		return 1;
	}
	// Test bad time against network time
	CBBlockSerialise(testBlock, false, false);
	if (CBValidatorProcessBlock(validator, testBlock, testBlock->time - 7201).status != CB_BLOCK_STATUS_BAD_TIME) {
		printf("NETWORK TIME BAD BLOCK TIME FAIL\n");
		return 1;
	}
	// Test bad time against median time
	testBlock->time--;
	CBBlockSerialise(testBlock, false, false);
	if (CBValidatorProcessBlock(validator, testBlock, 1349643202).status != CB_BLOCK_STATUS_BAD) {
		printf("MEDIAN TIME BAD BLOCK TIME FAIL\n");
		return 1;
	}
	return 0;
}
