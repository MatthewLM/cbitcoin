//
//  testCBFullValidator.c
//  cbitcoin
//
//  Created by Matthew Mitchell on 17/09/2012.
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

#include "CBFullValidator.h"
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
	bool bad;
	CBFullValidator * validator = CBNewFullValidator(storage, &bad, 0);
	if (NOT validator || bad) {
		printf("VALIDATOR INIT FAIL\n");
		return 1;
	}
	CBReleaseObject(validator);
	CBFreeBlockChainStorage(storage);
	// Now create it again. It should load the data.
	storage = CBNewBlockChainStorage("./");
	validator = CBNewFullValidator(storage, &bad, 0);
	if (NOT validator || bad) {
		printf("VALIDATOR LOAD FROM FILE FAIL\n");
		return 1;
	}
	// Now verify that the data is correct.
	if(validator->numOrphans){
		printf("ORPHAN NUM FAIL\n");
		return 1;
	}
	if(validator->numBranches != 1){
		printf("BRANCH NUM FAIL\n");
		return 1;
	}
	if(validator->mainBranch){
		printf("MAIN BRANCH FAIL\n");
		return 1;
	}
	if(validator->branches[0].lastRetargetTime != 1231006505){
		printf("LAST RETARGET FAIL\n");
		return 1;
	}
	if(validator->branches[0].numBlocks != 1){
		printf("NUM BLOCKS FAIL\n");
		return 1;
	}
	if(validator->branches[0].parentBranch){
		printf("PARENT BRANCH FAIL\n");
		return 1;
	}
	if(validator->branches[0].startHeight){
		printf("START HEIGHT FAIL\n");
		return 1;
	}
	if(validator->branches[0].work.length != 1){
		printf("WORK LENGTH FAIL\n");
		return 1;
	}
	if(validator->branches[0].work.data[0]){
		printf("WORK VAL FAIL\n");
		return 1;
	}
	// Try loading the genesis block
	CBBlock * block = CBBlockChainStorageLoadBlock(validator, 0, 0);
	if (NOT block) {
		printf("GENESIS RETRIEVE FAIL\n");
		return 1;
	}
	CBBlockDeserialise(block, true);
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
	if (block->transactionNum != 1) {
		printf("GENESIS TRANSACTION NUM FAIL\n");
		return 1;
	}
	if (block->version != 1) {
		printf("GENESIS VERSION FAIL\n");
		return 1;
	}
	if (block->transactions[0]->inputNum != 1) {
		printf("GENESIS COIN BASE INPUT NUM FAIL\n");
		return 1;
	}
	if (block->transactions[0]->outputNum != 1) {
		printf("GENESIS COIN BASE INPUT NUM FAIL\n");
		return 1;
	}
	if (block->transactions[0]->version != 1) {
		printf("GENESIS COIN BASE VERSION FAIL\n");
		return 1;
	}
	if (block->transactions[0]->lockTime) {
		printf("GENESIS COIN BASE LOCK TIME FAIL\n");
		return 1;
	}
	if (block->transactions[0]->lockTime) {
		printf("GENESIS COIN BASE LOCK TIME FAIL\n");
		return 1;
	}
	if (block->transactions[0]->inputs[0]->scriptObject->length != 0x4D) {
		printf("GENESIS TRANSACTION INPUT SCRIPT LENGTH FAIL\n0x");
		return 1;
	}
	if (block->transactions[0]->outputs[0]->scriptObject->length != 0x43) {
		printf("GENESIS TRANSACTION OUTPUT SCRIPT LENGTH FAIL\n0x");
		return 1;
	}
	for (int x = 0; x < 32; x++) {
		if(CBByteArrayGetByte(block->transactions[0]->inputs[0]->prevOut.hash, x) != 0){
			printf("GENESIS TRANSACTION INPUT OUT POINTER HASH FAIL\n");
			return 1;
		}
	}
	if (block->transactions[0]->inputs[0]->prevOut.index != 0xFFFFFFFF) {
		printf("GENESIS TRANSACTION INPUT OUT POINTER INDEX FAIL\n0x");
		return 1;
	}
	if (block->transactions[0]->inputs[0]->sequence != CB_TRANSACTION_INPUT_FINAL) {
		printf("GENESIS TRANSACTION INPUT SEQUENCE FAIL\n0x");
		return 1;
	}
	CBByteArray * genesisInScript = CBNewByteArrayWithDataCopy((uint8_t [77]){0x04, 0xFF, 0xFF, 0x00, 0x1D, 0x01, 0x04, 0x45, 0x54, 0x68, 0x65, 0x20, 0x54, 0x69, 0x6D, 0x65, 0x73, 0x20, 0x30, 0x33, 0x2F, 0x4A, 0x61, 0x6E, 0x2F, 0x32, 0x30, 0x30, 0x39, 0x20, 0x43, 0x68, 0x61, 0x6E, 0x63, 0x65, 0x6C, 0x6C, 0x6F, 0x72, 0x20, 0x6F, 0x6E, 0x20, 0x62, 0x72, 0x69, 0x6E, 0x6B, 0x20, 0x6F, 0x66, 0x20, 0x73, 0x65, 0x63, 0x6F, 0x6E, 0x64, 0x20, 0x62, 0x61, 0x69, 0x6C, 0x6F, 0x75, 0x74, 0x20, 0x66, 0x6F, 0x72, 0x20, 0x62, 0x61, 0x6E, 0x6B, 0x73}, 77);
	CBByteArray * genesisOutScript = CBNewByteArrayWithDataCopy((uint8_t [67]){0x41, 0x04, 0x67, 0x8A, 0xFD, 0xB0, 0xFE, 0x55, 0x48, 0x27, 0x19, 0x67, 0xF1, 0xA6, 0x71, 0x30, 0xB7, 0x10, 0x5C, 0xD6, 0xA8, 0x28, 0xE0, 0x39, 0x09, 0xA6, 0x79, 0x62, 0xE0, 0xEA, 0x1F, 0x61, 0xDE, 0xB6, 0x49, 0xF6, 0xBC, 0x3F, 0x4C, 0xEF, 0x38, 0xC4, 0xF3, 0x55, 0x04, 0xE5, 0x1E, 0xC1, 0x12, 0xDE, 0x5C, 0x38, 0x4D, 0xF7, 0xBA, 0x0B, 0x8D, 0x57, 0x8A, 0x4C, 0x70, 0x2B, 0x6B, 0xF1, 0x1D, 0x5F, 0xAC}, 67);
	if (CBByteArrayCompare(block->transactions[0]->inputs[0]->scriptObject, genesisInScript)) {
		printf("GENESIS IN SCRIPT FAIL\n0x");
		return 1;
	}
	if (block->transactions[0]->outputs[0]->value != 5000000000) {
		printf("GENESIS TRANSACTION OUTPUT VALUE FAIL\n0x");
		return 1;
	}
	if (CBByteArrayCompare(block->transactions[0]->outputs[0]->scriptObject, genesisOutScript)) {
		printf("GENESIS OUT SCRIPT FAIL\n0x");
		return 1;
	}
	CBReleaseObject(genesisInScript);
	CBReleaseObject(genesisOutScript);
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
	block1->transactions = malloc(sizeof(*block1->transactions));
	block1->transactions[0] = CBNewTransaction(0, 1);
	CBByteArray * nullHash = CBNewByteArrayOfSize(32);
	memset(CBByteArrayGetData(nullHash), 0, 32);
	CBScript * script = CBNewScriptWithDataCopy((uint8_t []){0x04, 0xff, 0xff, 0x00, 0x1d, 0x01, 0x04}, 7);
	CBTransactionTakeInput(block1->transactions[0], CBNewTransactionInput(script, CB_TRANSACTION_INPUT_FINAL, nullHash, 0xFFFFFFFF));
	CBReleaseObject(script);
	script = CBNewScriptWithDataCopy((uint8_t []){0x41, 0x04, 0x96, 0xb5, 0x38, 0xe8, 0x53, 0x51, 0x9c, 0x72, 0x6a, 0x2c, 0x91, 0xe6, 0x1e, 0xc1, 0x16, 0x00, 0xae, 0x13, 0x90, 0x81, 0x3a, 0x62, 0x7c, 0x66, 0xfb, 0x8b, 0xe7, 0x94, 0x7b, 0xe6, 0x3c, 0x52, 0xda, 0x75, 0x89, 0x37, 0x95, 0x15, 0xd4, 0xe0, 0xa6, 0x04, 0xf8, 0x14, 0x17, 0x81, 0xe6, 0x22, 0x94, 0x72, 0x11, 0x66, 0xbf, 0x62, 0x1e, 0x73, 0xa8, 0x2c, 0xbf, 0x23, 0x42, 0xc8, 0x58, 0xee, 0xAC}, 67);
	CBTransactionTakeOutput(block1->transactions[0], CBNewTransactionOutput(5000000000, script));
	CBReleaseObject(script);
	CBGetMessage(block1)->bytes = CBNewByteArrayOfSize(CBBlockCalculateLength(block1, true));
	CBBlockSerialise(block1, true, false);
	// Made block now proceed with the test.
	CBBlockStatus res = CBFullValidatorProcessBlock(validator, block1, 1349643202);
	if (res != CB_BLOCK_STATUS_MAIN) {
		printf("MAIN CHAIN ADD FAIL\n");
		return 1;
	}
	// Check validator data is correct, after closing and loading data
	CBReleaseObject(validator);
	CBFreeBlockChainStorage(storage);
	storage = CBNewBlockChainStorage("./");
	validator = CBNewFullValidator(storage, &bad, 0);
	if (NOT validator || bad){
		printf("BLOCK ONE LOAD FROM FILE FAIL\n");
		return 1;
	}
	if (validator->mainBranch) {
		printf("BLOCK ONE MAIN BRANCH FAIL\n");
		return 1;
	}
	if (validator->numBranches != 1) {
		printf("BLOCK ONE MUM BRANCHES FAIL\n");
		return 1;
	}
	if (validator->numOrphans) {
		printf("BLOCK ONE MUM ORPHANS FAIL\n");
		return 1;
	}
	if (validator->branches[0].lastRetargetTime != 1231006505) {
		printf("BLOCK ONE LAST RETARGET TIME FAIL\n");
		return 1;
	}
	if (validator->branches[0].lastValidation != 1) {
		printf("BLOCK ONE LAST VALIDATION FAIL\n");
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
	CBBlockDeserialise(block1, true);
	if (NOT block1) {
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
		printf("BLOCK ONE TRANSACTION NUM FAIL\n");
		return 1;
	}
	if (block1->version != 1) {
		printf("BLOCK ONE VERSION FAIL\n");
		return 1;
	}
	if (block1->transactions[0]->inputNum != 1) {
		printf("BLOCK ONE COINBASE INPUT NUM FAIL\n");
		return 1;
	}
	if (block1->transactions[0]->inputs[0]->prevOut.index != 0xFFFFFFFF) {
		printf("BLOCK ONE COINBASE INPUT PREV OUT INDEX FAIL\n");
		return 1;
	}
	if (CBByteArrayCompare(nullHash, block1->transactions[0]->inputs[0]->prevOut.hash)) {
		printf("BLOCK ONE COINBASE INPUT PREV OUT HASH FAIL\n");
		return 1;
	}
	if (block1->transactions[0]->inputs[0]->scriptObject->length != 7) {
		printf("BLOCK ONE COINBASE INPUT SCRIPT SIZE FAIL\n");
		return 1;
	}
	if (memcmp(CBByteArrayGetData(block1->transactions[0]->inputs[0]->scriptObject), (uint8_t []){0x04, 0xff, 0xff, 0x00, 0x1d, 0x01, 0x04}, 7)) {
		printf("BLOCK ONE COINBASE INPUT SCRIPT FAIL\n");
		return 1;
	}
	if (block1->transactions[0]->inputs[0]->sequence != CB_TRANSACTION_INPUT_FINAL) {
		printf("BLOCK ONE COINBASE INPUT SEQUENCE FAIL\n");
		return 1;
	}
	if (block1->transactions[0]->lockTime) {
		printf("BLOCK ONE COINBASE LOCK TIME FAIL\n");
		return 1;
	}
	if (block1->transactions[0]->outputNum != 1) {
		printf("BLOCK ONE COINBASE OUTPUT NUM FAIL\n");
		return 1;
	}
	if (block1->transactions[0]->outputs[0]->scriptObject->length != 67) {
		printf("BLOCK ONE COINBASE OUTPUT SCRIPT SIZE FAIL\n");
		return 1;
	}
	if (memcmp(CBByteArrayGetData(block1->transactions[0]->outputs[0]->scriptObject), (uint8_t []){0x41, 0x04, 0x96, 0xb5, 0x38, 0xe8, 0x53, 0x51, 0x9c, 0x72, 0x6a, 0x2c, 0x91, 0xe6, 0x1e, 0xc1, 0x16, 0x00, 0xae, 0x13, 0x90, 0x81, 0x3a, 0x62, 0x7c, 0x66, 0xfb, 0x8b, 0xe7, 0x94, 0x7b, 0xe6, 0x3c, 0x52, 0xda, 0x75, 0x89, 0x37, 0x95, 0x15, 0xd4, 0xe0, 0xa6, 0x04, 0xf8, 0x14, 0x17, 0x81, 0xe6, 0x22, 0x94, 0x72, 0x11, 0x66, 0xbf, 0x62, 0x1e, 0x73, 0xa8, 0x2c, 0xbf, 0x23, 0x42, 0xc8, 0x58, 0xee, 0xAC}, 67)) {
		printf("BLOCK ONE COINBASE INPUT SCRIPT FAIL\n");
		return 1;
	}
	if (block1->transactions[0]->outputs[0]->value != 50 * CB_ONE_BITCOIN) {
		printf("BLOCK ONE COINBASE OUTPUT VALUE FAIL\n");
		return 1;
	}
	if (block1->transactions[0]->version != 1) {
		printf("BLOCK ONE COINBASE VERSION FAIL\n");
		return 1;
	}
	// Check coinbase output reference data
	uint8_t outputHash[32] = {0x98, 0x20, 0x51, 0xfD, 0x1E, 0x4B, 0xA7, 0x44, 0xBB, 0xBE, 0x68, 0x0E, 0x1F, 0xEE, 0x14, 0x67, 0x7B, 0xA1, 0xA3, 0xC3, 0x54, 0x0B, 0xF7, 0xB1, 0xCD, 0xB6, 0x06, 0xE8, 0x57, 0x23, 0x3E, 0x0E};
	bool coinbase;
	uint32_t outputHeight;
	CBTransactionOutput * output = CBBlockChainStorageLoadUnspentOutput(validator, outputHash, 0, &coinbase, &outputHeight);
	if (outputHeight != 1) {
		printf("BLOCK ONE UNSPENT OUTPUT HEIGHT FAIL\n");
		return 1;
	}
	if (NOT coinbase) {
		printf("BLOCK ONE UNSPENT OUTPUT COINBASE FAIL\n");
		return 1;
	}
	// Verify same data
	if (memcmp(CBByteArrayGetData(CBGetMessage(output)->bytes), (uint8_t [76]){
		0x00, 0xF2, 0x05, 0x2A, 0x01, 0x00, 0x00, 0x00, 
		0x43, 
		0x41, 0x04, 0x96, 0xb5, 0x38, 0xe8, 0x53, 0x51, 0x9c, 0x72, 0x6a, 0x2c, 0x91, 0xe6, 0x1e, 0xc1, 0x16, 0x00, 0xae, 0x13, 0x90, 0x81, 0x3a, 0x62, 0x7c, 0x66, 0xfb, 0x8b, 0xe7, 0x94, 0x7b, 0xe6, 0x3c, 0x52, 0xda, 0x75, 0x89, 0x37, 0x95, 0x15, 0xd4, 0xe0, 0xa6, 0x04, 0xf8, 0x14, 0x17, 0x81, 0xe6, 0x22, 0x94, 0x72, 0x11, 0x66, 0xbf, 0x62, 0x1e, 0x73, 0xa8, 0x2c, 0xbf, 0x23, 0x42, 0xc8, 0x58, 0xee, 0xAC
	}, 76)) {
		printf("BLOCK ONE UNSPENT OUTPUT DATA CONSISTENCY FAIL\n");
		return 1;
	}
	CBReleaseObject(output);
	// Test duplicate add.
	res = CBFullValidatorProcessBlock(validator, block1, 1349643202);
	if (res != CB_BLOCK_STATUS_DUPLICATE) {
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
	testBlock->transactions = malloc(sizeof(*testBlock->transactions));
	testBlock->transactions[0] = CBNewTransaction(0, 1);
	memset(CBByteArrayGetData(nullHash), 0, 32);
	script = CBNewScriptWithDataCopy((uint8_t []){0x04, 0xff, 0xff, 0x00, 0x1d, 0x01, 0x04}, 7);
	CBTransactionTakeInput(testBlock->transactions[0], CBNewTransactionInput(script, CB_TRANSACTION_INPUT_FINAL, nullHash, 0xFFFFFFFF));
	CBReleaseObject(script);
	script = CBNewScriptWithDataCopy((uint8_t []){0x41, 0x04, 0x96, 0xb5, 0x38, 0xe8, 0x53, 0x51, 0x9c, 0x72, 0x6a, 0x2c, 0x91, 0xe6, 0x1e, 0xc1, 0x16, 0x00, 0xae, 0x13, 0x90, 0x81, 0x3a, 0x62, 0x7c, 0x66, 0xfb, 0x8b, 0xe7, 0x94, 0x7b, 0xe6, 0x3c, 0x52, 0xda, 0x75, 0x89, 0x37, 0x95, 0x15, 0xd4, 0xe0, 0xa6, 0x04, 0xf8, 0x14, 0x17, 0x81, 0xe6, 0x22, 0x94, 0x72, 0x11, 0x66, 0xbf, 0x62, 0x1e, 0x73, 0xa8, 0x2c, 0xbf, 0x23, 0x42, 0xc8, 0x58, 0xee, 0xAC}, 67);
	CBTransactionTakeOutput(testBlock->transactions[0], CBNewTransactionOutput(5000000000, script));
	CBReleaseObject(script);
	CBGetMessage(testBlock)->bytes = CBNewByteArrayOfSize(CBBlockCalculateLength(testBlock, true));
	CBBlockSerialise(testBlock, true, false);
	// Made block now proceed with the test.
	res = CBFullValidatorProcessBlock(validator, testBlock, 1349643202);
	if (res != CB_BLOCK_STATUS_BAD) {
		printf("BAD POW ADD FAIL\n");
		return 1;
	}
	// Add 100 blocks to test
	CBBlock * theBlocks[100];
	CBByteArray * prevHash = CBNewByteArrayWithDataCopy((uint8_t []){0x6F, 0xE2, 0x8C, 0x0A, 0xB6, 0xF1, 0xB3, 0x72, 0xC1, 0xA6, 0xA2, 0x46, 0xAE, 0x63, 0xF7, 0x4F, 0x93, 0x1E, 0x83, 0x65, 0xE1, 0x5A, 0x08, 0x9C, 0x68, 0xD6, 0x19, 0x00, 0x00, 0x00, 0x00, 0x00}, 32);
	uint32_t time = 1231006506;
	for (uint8_t x = 0; x < 100; x++) {
		block = CBNewBlock();
		block->version = 1;
		if (x == 1 || x == 3 || NOT ((x-1) % 6))
			time++;
		block->time = time;
		block->transactionNum = 1;
		block->transactions = malloc(sizeof(*block->transactions));
		block->transactions[0] = CBNewTransaction(0, 1);
		CBScript * nullScript = CBNewScriptOfSize(0);
		CBScript * inScript = CBNewScriptOfSize(2);
		CBTransactionTakeInput(block->transactions[0], 
							   CBNewTransactionInput(inScript, CB_TRANSACTION_INPUT_FINAL, nullHash, 0xFFFFFFFF));
		CBReleaseObject(inScript);
		CBTransactionTakeOutput(block->transactions[0], CBNewTransactionOutput(5000000000, nullScript));
		CBReleaseObject(nullScript);
		block->target = CB_MAX_TARGET;
		block->prevBlockHash = prevHash;
		CBGetMessage(block)->bytes = CBNewByteArrayOfSize(143);
		CBGetMessage(block->transactions[0])->bytes = CBNewByteArrayOfSize(62);
		CBByteArraySetByte(block->transactions[0]->inputs[0]->scriptObject, 0, blockInfo[x].extranonce);
		CBByteArraySetByte(block->transactions[0]->inputs[0]->scriptObject, 1, x);
		block->nonce = blockInfo[x].nonce;
		CBTransactionSerialise(block->transactions[0], true);
		block->merkleRoot = CBNewByteArrayWithDataCopy(CBTransactionGetHash(block->transactions[0]), 32);
		CBBlockSerialise(block, true, false);
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
		res = CBFullValidatorProcessBlock(validator, theBlocks[y], 1230999326);
		if (x < 22){
			if (res != CB_BLOCK_STATUS_ORPHAN) {
				printf("ORPHAN FAIL AT %u\n", y);
				return 1;
			}
		}else if (x == 22){
			if (res != CB_BLOCK_STATUS_SIDE) {
				printf("SIDE FAIL AT %u\n", y);
				return 1;
			}
		}else if (res != CB_BLOCK_STATUS_MAIN) {
			// Reorg occurs at x == 1
			printf("MAIN FAIL AT %u\n", y);
			return 1;
		}
		if (x > 1)
			CBReleaseObject(theBlocks[y]);
	}
	// Now do tests without valid PoW
	validator->flags |= CB_FULL_VALIDATOR_DISABLE_POW_CHECK;
	// Test the disabler works
	res = CBFullValidatorProcessBlock(validator, testBlock, 1349643202);
	if (res != CB_BLOCK_STATUS_SIDE) {
		printf("POW CHECK DISABLE FAIL\n");
		return 1;
	}
	// No transactions
	testBlock->transactionNum = 0;
	testBlock->time--;
	CBBlockSerialise(testBlock, true, false);
	res = CBFullValidatorProcessBlock(validator, testBlock, 1349643202);
	if (res != CB_BLOCK_STATUS_BAD) {
		printf("NO TRANSACTIONS FAIL\n");
		return 1;
	}
	// Incorrect merkle root
	testBlock->transactionNum = 1;
	CBByteArrayGetData(testBlock->merkleRoot)[0]--;
	CBGetMessage(testBlock)->bytes->length = CBBlockCalculateLength(testBlock, true);
	CBBlockSerialise(testBlock, true, false);
	res = CBFullValidatorProcessBlock(validator, testBlock, 1349643202);
	if (res != CB_BLOCK_STATUS_BAD) {
		printf("INCORRECT MERKLE ROOT FAIL\n");
		return 1;
	}
	// First tx not a coinbase
	CBReleaseObject(testBlock->prevBlockHash);
	testBlock->prevBlockHash = prevHash;
	testBlock->transactions[0]->inputs[0]->prevOut.index = 2;
	CBTransactionSerialise(testBlock->transactions[0], true);
	memcpy(CBByteArrayGetData(testBlock->merkleRoot), CBTransactionGetHash(testBlock->transactions[0]), 32);
	CBBlockSerialise(testBlock, true, false);
	res = CBFullValidatorProcessBlock(validator, testBlock, 1349643202);
	if (res != CB_BLOCK_STATUS_BAD) {
		printf("TX NOT COINBASE FAIL\n");
		return 1;
	}
	// Non final coinbase
	testBlock->transactions[0]->inputs[0]->prevOut.index = 0xFFFFFFFF;
	testBlock->transactions[0]->inputs[0]->sequence = 1;
	testBlock->transactions[0]->lockTime = testBlock->time;
	CBTransactionSerialise(testBlock->transactions[0], true);
	memcpy(CBByteArrayGetData(testBlock->merkleRoot), CBTransactionGetHash(testBlock->transactions[0]), 32);
	CBBlockSerialise(testBlock, true, false);
	res = CBFullValidatorProcessBlock(validator, testBlock, 1349643202);
	if (res != CB_BLOCK_STATUS_BAD) {
		printf("COINBASE NOT FINAL FAIL\n");
		return 1;
	}
	// Bad coinbase script size 1
	testBlock->transactions[0]->inputs[0]->sequence = CB_TRANSACTION_INPUT_FINAL;
	testBlock->transactions[0]->lockTime = 0;
	CBReleaseObject(testBlock->transactions[0]->inputs[0]->scriptObject);
	testBlock->transactions[0]->inputs[0]->scriptObject = CBNewByteArrayOfSize(1);
	CBByteArraySetByte(testBlock->transactions[0]->inputs[0]->scriptObject, 0, 0);
	CBTransactionSerialise(testBlock->transactions[0], true);
	memcpy(CBByteArrayGetData(testBlock->merkleRoot), CBTransactionGetHash(testBlock->transactions[0]), 32);
	CBBlockSerialise(testBlock, true, false);
	res = CBFullValidatorProcessBlock(validator, testBlock, 1349643202);
	if (res != CB_BLOCK_STATUS_BAD) {
		printf("COINBASE SCRIPT SIZE 1 FAIL\n");
		return 1;
	}
	// Bad coinbase script size 101
	CBReleaseObject(testBlock->transactions[0]->inputs[0]->scriptObject);
	testBlock->transactions[0]->inputs[0]->scriptObject = CBNewByteArrayOfSize(101);
	memset(CBByteArrayGetData(testBlock->transactions[0]->inputs[0]->scriptObject), 0, 101);
	CBReleaseObject(CBGetMessage(testBlock->transactions[0])->bytes);
	CBGetMessage(testBlock->transactions[0])->bytes = CBNewByteArrayOfSize(228);
	CBTransactionSerialise(testBlock->transactions[0], true);
	memcpy(CBByteArrayGetData(testBlock->merkleRoot), CBTransactionGetHash(testBlock->transactions[0]), 32);
	CBReleaseObject(CBGetMessage(testBlock)->bytes);
	CBGetMessage(testBlock)->bytes = CBNewByteArrayOfSize(309);
	CBBlockSerialise(testBlock, true, false);
	if (CBFullValidatorProcessBlock(validator, testBlock, 1349643202) != CB_BLOCK_STATUS_BAD) {
		printf("COINBASE SCRIPT SIZE 101 FAIL\n");
		return 1;
	}
	// Bad target
	testBlock->transactions[0]->inputs[0]->scriptObject->length = 2;
	testBlock->target = CB_MAX_TARGET - 1;
	CBTransactionSerialise(testBlock->transactions[0], true);
	memcpy(CBByteArrayGetData(testBlock->merkleRoot), CBTransactionGetHash(testBlock->transactions[0]), 32);
	CBBlockSerialise(testBlock, true, false);
	if (CBFullValidatorProcessBlock(validator, testBlock, 1349643202) != CB_BLOCK_STATUS_BAD) {
		printf("BAD TARGET FAIL\n");
		return 1;
	}
	// Make 100 blocks
	// Prepare
	testBlock->target = CB_MAX_TARGET;
	CBReleaseObject(testBlock->transactions[0]->outputs[0]->scriptObject);
	testBlock->transactions[0]->outputs[0]->scriptObject = CBNewScriptOfSize(3);
	CBByteArraySetByte(testBlock->transactions[0]->outputs[0]->scriptObject, 0, 1);
	CBByteArraySetByte(testBlock->transactions[0]->outputs[0]->scriptObject, 2, CB_SCRIPT_OP_EQUAL);
	testBlock->transactions[0]->outputs[0]->value = 20 * CB_ONE_BITCOIN;
	CBRetainObject(testBlock->transactions[0]->outputs[0]->scriptObject);
	CBTransactionTakeOutput(testBlock->transactions[0], 
							CBNewTransactionOutput(30 * CB_ONE_BITCOIN, testBlock->transactions[0]->outputs[0]->scriptObject));
	// Create and add
	CBByteArray * txHashes[2];
	uint8_t x = 0;
	for (; x < 100; x++) {
		testBlock->time++;
		CBByteArraySetByte(testBlock->transactions[0]->outputs[0]->scriptObject, 1, x);
		CBTransactionSerialise(testBlock->transactions[0], true);
		memcpy(CBByteArrayGetData(testBlock->merkleRoot), CBTransactionGetHash(testBlock->transactions[0]), 32);
		if (x < 2)
			txHashes[x] = CBNewByteArrayWithDataCopy(CBTransactionGetHash(testBlock->transactions[0]), 32);
		CBBlockSerialise(testBlock, true, false);
		// Add
		printf("100 TEST BLOCKS PROCESSING %u\n", x);
		if (CBFullValidatorProcessBlock(validator, testBlock, 1349643202) != CB_BLOCK_STATUS_MAIN) {
			printf("100 TEST BLOCKS NUM %u FAIL\n", x);
			return 1;
		}
		// Change previous block to the one before it
		memcpy(CBByteArrayGetData(testBlock->prevBlockHash), CBBlockGetHash(testBlock), 32);
	}
	uint8_t endOf100BlocksHash[32];
	memcpy(endOf100BlocksHash, CBBlockGetHash(testBlock), 32);
	// Test a block spending from block 102 FAIL
	testBlock->time++;
	CBByteArraySetByte(testBlock->transactions[0]->outputs[0]->scriptObject, 1, x);
	CBTransactionSerialise(testBlock->transactions[0], true);
	// Make "normal" transaction
	CBTransaction * normal = CBNewTransaction(0, 1);
	CBScript * normalInScript = CBNewScriptWithDataCopy((uint8_t []){1, 1}, 2);
	CBTransactionTakeInput(normal, CBNewTransactionInput(normalInScript, CB_TRANSACTION_INPUT_FINAL, txHashes[1], 0));
	CBReleaseObject(normalInScript);
	CBByteArray * emptyScript = CBNewByteArrayOfSize(0);
	CBTransactionTakeOutput(normal, CBNewTransactionOutput(20 * CB_ONE_BITCOIN, emptyScript));
	CBGetMessage(normal)->bytes = CBNewByteArrayOfSize(65);
	CBTransactionSerialise(normal, true);
	testBlock->transactionNum = 2;
	testBlock->transactions = realloc(testBlock->transactions, sizeof(*testBlock->transactions) * 2);
	testBlock->transactions[1] = normal;
	CBBlockCalculateAndSetMerkleRoot(testBlock);
	CBReleaseObject(CBGetMessage(testBlock)->bytes);
	CBGetMessage(testBlock)->bytes = CBNewByteArrayOfSize(374);
	CBBlockSerialise(testBlock, true, false);
	if (CBFullValidatorProcessBlock(validator, testBlock, 1349643202) != CB_BLOCK_STATUS_BAD) {
		printf("COINBASE NOT MATURE FAIL\n");
		return 1;
	}
	// Test a block spending from block 101 SUCCESS
	CBByteArraySetByte(normalInScript, 1, 0);
	normal->inputs[0]->prevOut.hash = txHashes[0];
	CBByteArraySetByte(testBlock->transactions[0]->outputs[0]->scriptObject, 1, ++x);
	CBTransactionSerialise(testBlock->transactions[0], true);
	CBTransactionSerialise(normal, true);
	CBBlockCalculateAndSetMerkleRoot(testBlock);
	CBBlockSerialise(testBlock, true, false);
	if (CBFullValidatorProcessBlock(validator, testBlock, 1349643202) != CB_BLOCK_STATUS_MAIN) {
		printf("COINBASE IS MATURE FAIL\n");
		return 1;
	}
	// Test non-final normal transaction
	memcpy(CBByteArrayGetData(testBlock->prevBlockHash), CBBlockGetHash(testBlock), 32);
	normal->inputs[0]->prevOut.index = 1;
	normal->lockTime = ++testBlock->time;
	normal->inputs[0]->sequence = 0;
	normal->outputs[0]->value = 30 * CB_ONE_BITCOIN;
	CBTransactionSerialise(normal, true);
	CBBlockCalculateAndSetMerkleRoot(testBlock);
	CBBlockSerialise(testBlock, true, false);
	if (CBFullValidatorProcessBlock(validator, testBlock, 1349643202) != CB_BLOCK_STATUS_BAD) {
		printf("NON FINAL NORMAL TRANSACTION FAIL\n");
		return 1;
	}
	// Try spending transaction later in block
	testBlock->time++;
	CBByteArraySetByte(testBlock->transactions[0]->outputs[0]->scriptObject, 1, ++x);
	CBTransactionSerialise(testBlock->transactions[0], true);
	CBTransactionSerialise(normal, true);
	CBTransaction * normal2 = CBNewTransaction(0, 1);
	CBScript * inScript = CBNewScriptWithDataCopy((uint8_t []){CB_SCRIPT_OP_TRUE}, 1);
	CBByteArray * txHash = CBNewByteArrayWithDataCopy(CBTransactionGetHash(normal), 32);
	CBTransactionTakeInput(normal2, CBNewTransactionInput(inScript, CB_TRANSACTION_INPUT_FINAL, txHash, 0));
	CBReleaseObject(inScript);
	CBReleaseObject(txHash);
	CBTransactionTakeOutput(normal2, CBNewTransactionOutput(30 * CB_ONE_BITCOIN, emptyScript));
	CBGetMessage(normal2)->bytes = CBNewByteArrayOfSize(65);
	CBTransactionSerialise(normal2, false);
	testBlock->transactionNum = 3;
	testBlock->transactions = realloc(testBlock->transactions, sizeof(*testBlock->transactions) * 3);
	testBlock->transactions[1] = normal2;
	testBlock->transactions[2] = normal;
	CBBlockCalculateAndSetMerkleRoot(testBlock);
	CBReleaseObject(CBGetMessage(testBlock)->bytes);
	CBGetMessage(testBlock)->bytes = CBNewByteArrayOfSize(439);
	CBBlockSerialise(testBlock, true, false);
	if (CBFullValidatorProcessBlock(validator, testBlock, 1349643202) != CB_BLOCK_STATUS_BAD) {
		printf("LATER SPEND FAIL\n");
		return 1;
	}
	// Spend transaction earlier in block SUCCESS
	testBlock->transactions[1] = normal;
	testBlock->transactions[2] = normal2;
	CBBlockCalculateAndSetMerkleRoot(testBlock);
	CBBlockSerialise(testBlock, true, false);
	if (CBFullValidatorProcessBlock(validator, testBlock, 1349643202) != CB_BLOCK_STATUS_MAIN) {
		printf("EARLIER SPEND FAIL\n");
		return 1;
	}
	// Transaction spending output already spent in another block
	memcpy(CBByteArrayGetData(testBlock->prevBlockHash), CBBlockGetHash(testBlock), 32);
	testBlock->time++;
	normal->inputs[0]->prevOut.hash = CBNewByteArrayWithDataCopy(CBTransactionGetHash(normal), 32);
	normal->inputs[0]->prevOut.index = 0;
	CBByteArraySetByte(testBlock->transactions[0]->outputs[0]->scriptObject, 1, ++x);
	CBByteArraySetByte(normal->inputs[0]->scriptObject, 1, x - 1);
	CBReleaseObject(normal2->inputs[0]->prevOut.hash);
	normal2->inputs[0]->prevOut.hash = txHashes[1];
	normal2->inputs[0]->prevOut.index = 0;
	CBByteArraySetByte(normal2->inputs[0]->scriptObject, 1, 1);
	normal2->outputs[0]->value = 20 * CB_ONE_BITCOIN;
	CBTransactionSerialise(testBlock->transactions[0], true);
	CBTransactionSerialise(normal, true);
	CBTransactionSerialise(normal2, true);
	CBBlockCalculateAndSetMerkleRoot(testBlock);
	CBBlockSerialise(testBlock, true, false);
	if (CBFullValidatorProcessBlock(validator, testBlock, 1349643202) != CB_BLOCK_STATUS_BAD) {
		printf("DOUBLE SPEND IN SAME BLOCK FAIL\n");
		return 1;
	}
	// Two transactions spending same output
	CBReleaseObject(normal->inputs[0]->prevOut.hash);
	normal->inputs[0]->prevOut.hash = txHashes[1];
	normal->inputs[0]->prevOut.index = 0;
	CBByteArraySetByte(normal->inputs[0]->scriptObject, 1, 1);
	normal->outputs[0]->value = 20 * CB_ONE_BITCOIN;
	CBTransactionSerialise(normal, true);
	CBBlockCalculateAndSetMerkleRoot(testBlock);
	CBBlockSerialise(testBlock, true, false);
	if (CBFullValidatorProcessBlock(validator, testBlock, 1349643202) != CB_BLOCK_STATUS_BAD) {
		printf("DOUBLE SPEND IN SAME BLOCK FAIL\n");
		return 1;
	}
	// Invalid input script
	normal->inputs[0]->prevOut.index = 1;
	normal->outputs[0]->value = 30 * CB_ONE_BITCOIN;
	CBByteArraySetByte(normal->inputs[0]->scriptObject, 1, 0);
	CBTransactionSerialise(normal, true);
	CBBlockCalculateAndSetMerkleRoot(testBlock);
	CBBlockSerialise(testBlock, true, false);
	if (CBFullValidatorProcessBlock(validator, testBlock, 1349643202) != CB_BLOCK_STATUS_BAD) {
		printf("BAD INPUT SCRIPT FAIL\n");
		return 1;
	}
	// Three transactions with different fees. Coinbase equal to reward plus fees plus one.
	normal->outputs[0]->value = 29 * CB_ONE_BITCOIN;
	CBByteArraySetByte(normal->inputs[0]->scriptObject, 1, 1);
	CBTransactionSerialise(normal, true);
	CBTransaction * normal3 = CBNewTransaction(0, 1);
	inScript = CBNewScriptWithDataCopy((uint8_t []){1, CBByteArrayGetByte(normal->outputs[0]->scriptObject, 1)}, 2);
	CBByteArray * prevOutHash = CBNewByteArrayWithDataCopy(CBTransactionGetHash(normal), 32);
	CBTransactionTakeInput(normal3, CBNewTransactionInput(inScript, CB_TRANSACTION_INPUT_FINAL, prevOutHash, 0));
	CBReleaseObject(inScript);
	CBReleaseObject(prevOutHash);
	CBScript * outScript = CBNewScriptWithDataCopy((uint8_t []){CB_SCRIPT_OP_TRUE}, 1);
	CBTransactionTakeOutput(normal3, CBNewTransactionOutput(20 * CB_ONE_BITCOIN, outScript));
	CBReleaseObject(outScript);
	normal2->outputs[0]->value = 15 * CB_ONE_BITCOIN;
	testBlock->transactions[0]->outputs[0]->value = 35 * CB_ONE_BITCOIN + 1;
	CBTransactionSerialise(testBlock->transactions[0], true);
	CBTransactionSerialise(normal2, true);
	CBGetMessage(normal3)->bytes = CBNewByteArrayOfSize(63);
	CBTransactionSerialise(normal3, true);
	testBlock->transactionNum = 4;
	testBlock->transactions = realloc(testBlock->transactions, sizeof(*testBlock->transactions) * 4);
	testBlock->transactions[3] = normal3;
	CBReleaseObject(CBGetMessage(testBlock)->bytes);
	CBGetMessage(testBlock)->bytes = CBNewByteArrayOfSize(502);
	CBBlockCalculateAndSetMerkleRoot(testBlock);
	CBBlockSerialise(testBlock, true, false);
	if (CBFullValidatorProcessBlock(validator, testBlock, 1349643202) != CB_BLOCK_STATUS_BAD) {
		printf("COINBASE OUTPUT TOO HIGH FAIL\n");
		return 1;
	}
	// Three transactions with different fees. Coinbase equal to reward plus fees. SUCCESS
	testBlock->transactions[0]->outputs[0]->value = 35 * CB_ONE_BITCOIN;
	CBTransactionSerialise(testBlock->transactions[0], true);
	CBBlockCalculateAndSetMerkleRoot(testBlock);
	CBBlockSerialise(testBlock, true, false);
	if (CBFullValidatorProcessBlock(validator, testBlock, 1349643202) != CB_BLOCK_STATUS_MAIN) {
		printf("COINBASE OUTPUT IS OK FAIL\n");
		return 1;
	}
	// Reference to output index that does not exist
	memcpy(CBByteArrayGetData(testBlock->prevBlockHash), CBBlockGetHash(testBlock), 32);
	testBlock->time++;
	CBByteArraySetByte(testBlock->transactions[0]->outputs[0]->scriptObject, 1, ++x);
	testBlock->transactionNum = 2;
	normal->inputs[0]->prevOut.index = 1;
	normal->inputs[0]->prevOut.hash = CBNewByteArrayWithDataCopy(CBTransactionGetHash(normal3), 32);
	CBTransactionSerialise(normal, true);
	CBBlockCalculateAndSetMerkleRoot(testBlock);
	CBBlockSerialise(testBlock, true, false);
	if (CBFullValidatorProcessBlock(validator, testBlock, 1349643202) != CB_BLOCK_STATUS_BAD) {
		printf("PREV OUT INDEX NOT EXIST FAIL\n");
		return 1;
	}
	// Reference to output transaction that does not exist
	normal->inputs[0]->prevOut.index = 0;
	CBByteArraySetByte(normal->inputs[0]->prevOut.hash, 0, CBByteArrayGetByte(normal->inputs[0]->prevOut.hash, 0) + 1);
	CBTransactionSerialise(normal, true);
	CBBlockCalculateAndSetMerkleRoot(testBlock);
	CBBlockSerialise(testBlock, true, false);
	if (CBFullValidatorProcessBlock(validator, testBlock, 1349643202) != CB_BLOCK_STATUS_BAD) {
		printf("PREV OUT TX NOT EXIST FAIL\n");
		return 1;
	}
	// Duplicate coinbase
	CBByteArraySetByte(normal->inputs[0]->prevOut.hash, 0, CBByteArrayGetByte(normal->inputs[0]->prevOut.hash, 0) - 1);
	CBTransactionSerialise(normal, true);
	normal->outputs[0]->value = 20 * CB_ONE_BITCOIN;
	CBByteArraySetByte(testBlock->transactions[0]->inputs[0]->scriptObject, 1, 0);
	CBByteArraySetByte(testBlock->transactions[0]->outputs[0]->scriptObject, 1, 0);
	CBByteArraySetByte(testBlock->transactions[0]->outputs[1]->scriptObject, 1, 0);
	testBlock->transactions[0]->outputs[0]->value = 20 * CB_ONE_BITCOIN;
	CBTransactionSerialise(testBlock->transactions[0], true);
	CBBlockCalculateAndSetMerkleRoot(testBlock);
	CBBlockSerialise(testBlock, true, false);
	if (CBFullValidatorProcessBlock(validator, testBlock, 1349643202) != CB_BLOCK_STATUS_BAD) {
		printf("DUPLICATE COINBASE FAIL\n");
		return 1;
	}
	// Block with 20001 sigops
	prevOutHash = CBNewByteArrayWithDataCopy(CBTransactionGetHash(normal3), 32);
	// 420 sigops per outScript
	outScript = CBNewScriptOfSize(201);
	CBByteArraySetByte(outScript, 200, CB_SCRIPT_OP_TRUE);
	for (uint16_t x = 0; x < 200; x += 10) {
		CBByteArraySetByte(outScript, x + 0, CB_SCRIPT_OP_TRUE);
		CBByteArraySetByte(outScript, x + 1, CB_SCRIPT_OP_TRUE);
		CBByteArraySetByte(outScript, x + 2, CB_SCRIPT_OP_CHECKSIG);
		CBByteArraySetByte(outScript, x + 3, CB_SCRIPT_OP_TRUE);
		CBByteArraySetByte(outScript, x + 4, CB_SCRIPT_OP_1);
		CBByteArraySetByte(outScript, x + 5, CB_SCRIPT_OP_TRUE);
		CBByteArraySetByte(outScript, x + 6, CB_SCRIPT_OP_TRUE);
		CBByteArraySetByte(outScript, x + 7, CB_SCRIPT_OP_2);
		CBByteArraySetByte(outScript, x + 8, CB_SCRIPT_OP_CHECKMULTISIG);
		CBByteArraySetByte(outScript, x + 9, CB_SCRIPT_OP_DROP);
	}
	// Each p2sh input contains 3 sigops.
	CBScript * p2shInput = CBNewScriptOfSize(12);
	CBByteArraySetByte(p2shInput, 0, CB_SCRIPT_OP_TRUE);
	CBByteArraySetByte(p2shInput, 1, 10);
	CBByteArrayCopySubByteArray(p2shInput, 2, outScript, 0, 10);
	CBScript * p2shOutput = CBNewScriptOfSize(23);
	CBByteArraySetByte(p2shOutput, 0, CB_SCRIPT_OP_HASH160);
	CBByteArraySetByte(p2shOutput, 1, 20);
	uint8_t hash[32];
	CBSha256(CBByteArrayGetData(outScript), 10, hash);
	CBRipemd160(hash, 32, CBByteArrayGetData(p2shOutput) + 2);
	CBByteArraySetByte(p2shOutput, 22, CB_SCRIPT_OP_EQUAL);
	CBReleaseObject(normal);
	CBReleaseObject(normal2);
	CBReleaseObject(normal3);
	testBlock->transactionNum = 4;
	testBlock->transactions = realloc(testBlock->transactions, sizeof(*testBlock->transactions) * 4);
	CBTransaction * tx = CBNewTransaction(0, 1);
	CBTransactionTakeInput(tx, CBNewTransactionInput(emptyScript, CB_TRANSACTION_INPUT_FINAL, prevOutHash, 0));
	// Add outputs
	CBTransactionTakeOutput(tx, CBNewTransactionOutput(20 * CB_ONE_BITCOIN, outScript));
	for (uint8_t y = 0; y < 46; y++)
		CBTransactionTakeOutput(tx, CBNewTransactionOutput(0, outScript));
	// Outputs equals 420 bytes times 47 = 19740
	CBGetMessage(tx)->bytes = CBNewByteArrayOfSize(CBTransactionCalculateLength(tx));
	CBTransactionSerialise(tx, false);
	testBlock->transactions[1] = tx;
	CBReleaseObject(prevOutHash);
	prevOutHash = CBNewByteArrayWithDataCopy(CBTransactionGetHash(tx), 32);
	// 86 p2sh inputs = 258 sigops (19998 total)
	tx = CBNewTransaction(0, 1);
	CBTransactionTakeInput(tx, CBNewTransactionInput(emptyScript, CB_TRANSACTION_INPUT_FINAL, prevOutHash, 0));
	// Add p2sh outputs
	for (uint8_t y = 0; y < 86; y++)
		CBTransactionTakeOutput(tx, CBNewTransactionOutput(CB_ONE_BITCOIN / 10, p2shOutput));
	CBGetMessage(tx)->bytes = CBNewByteArrayOfSize(CBTransactionCalculateLength(tx));
	CBTransactionSerialise(tx, false);
	testBlock->transactions[2] = tx;
	CBReleaseObject(prevOutHash);
	prevOutHash = CBNewByteArrayWithDataCopy(CBTransactionGetHash(tx), 32);
	// p2sh inputs
	tx = CBNewTransaction(0, 1);
	for (uint8_t y = 0; y < 86; y++)
		CBTransactionTakeInput(tx, CBNewTransactionInput(p2shInput, CB_TRANSACTION_INPUT_FINAL, prevOutHash, y));
	for (uint8_t y = 0; y < 86; y++)
		CBTransactionTakeOutput(tx, CBNewTransactionOutput(CB_ONE_BITCOIN / 10, emptyScript));
	CBGetMessage(tx)->bytes = CBNewByteArrayOfSize(CBTransactionCalculateLength(tx));
	CBTransactionSerialise(tx, false);
	testBlock->transactions[3] = tx;
	CBReleaseObject(prevOutHash);
	prevOutHash = CBNewByteArrayWithDataCopy(CBTransactionGetHash(tx), 32);
	// Need 3 more sigops. Add to coinbase
	// Add 3 outputs to coinbase.
	CBReleaseObject(outScript);
	outScript = CBNewByteArrayOfSize(3);
	CBByteArraySetByte(outScript, 0, CB_SCRIPT_OP_TRUE);
	CBByteArraySetByte(outScript, 1, CB_SCRIPT_OP_TRUE);
	CBByteArraySetByte(outScript, 2, CB_SCRIPT_OP_CHECKSIG);
	for (uint8_t x = 0; x < 3; x++)
		CBTransactionTakeOutput(testBlock->transactions[0], CBNewTransactionOutput(0, outScript));
	// Make block
	CBReleaseObject(CBGetMessage(testBlock->transactions[0])->bytes);
	CBGetMessage(testBlock->transactions[0])->bytes = CBNewByteArrayOfSize(CBTransactionCalculateLength(testBlock->transactions[0]));
	CBTransactionSerialise(testBlock->transactions[0], true);
	CBBlockCalculateAndSetMerkleRoot(testBlock);
	CBReleaseObject(CBGetMessage(testBlock)->bytes);
	CBGetMessage(testBlock)->bytes = CBNewByteArrayOfSize(CBBlockCalculateLength(testBlock, true));
	CBBlockSerialise(testBlock, true, false);
	if (CBFullValidatorProcessBlock(validator, testBlock, 1349643202) != CB_BLOCK_STATUS_BAD) {
		printf("BLOCK TOO MANY SIGOPS FAIL\n");
		return 1;
	}
	// Block with 20000 sigops SUCCESS
	// Remove one output from coinbase
	testBlock->transactions[0]->outputNum--;
	CBTransactionSerialise(testBlock->transactions[0], true);
	CBBlockCalculateAndSetMerkleRoot(testBlock);
	CBBlockSerialise(testBlock, true, false);
	CBBlockCalculateLength(testBlock, true);
	if (CBFullValidatorProcessBlock(validator, testBlock, 1349643202) != CB_BLOCK_STATUS_MAIN) {
		printf("BLOCK SIGOPS OK FAIL\n");
		return 1;
	}
	uint8_t savedHash[32];
	memcpy(savedHash, CBBlockGetHash(testBlock), 32);
	// Test reorganisation with spend on other branch.
	testBlock->transactionNum = 2;
	CBReleaseObject(testBlock->transactions[1]);
	testBlock->transactions[1] = CBNewTransaction(0, 1);
	inScript = CBNewScriptWithDataCopy((uint8_t []){CB_SCRIPT_OP_TRUE}, 1);
	CBTransactionTakeInput(testBlock->transactions[1], CBNewTransactionInput(inScript, CB_TRANSACTION_REF_BLOCK_INDEX, prevOutHash, 0));
	CBReleaseObject(inScript);
	CBTransactionTakeOutput(testBlock->transactions[1], CBNewTransactionOutput(CB_ONE_BITCOIN / 10, emptyScript));
	CBGetMessage(testBlock->transactions[1])->bytes = CBNewByteArrayOfSize(CBTransactionCalculateLength(testBlock->transactions[1]));
	CBTransactionSerialise(testBlock->transactions[1], false);
	CBBlockCalculateAndSetMerkleRoot(testBlock);
	CBBlockSerialise(testBlock, true, false);
	if (CBFullValidatorProcessBlock(validator, testBlock, 1349643202) != CB_BLOCK_STATUS_SIDE) {
		printf("SPEND ON OTHER BRANCH FIRST FAIL\n");
		return 1;
	}
	memcpy(CBByteArrayGetData(testBlock->prevBlockHash), CBBlockGetHash(testBlock), 32);
	testBlock->time++;
	testBlock->transactionNum = 1;
	testBlock->transactions[0]->outputNum--;
	CBTransactionSerialise(testBlock->transactions[0], false);
	CBBlockCalculateAndSetMerkleRoot(testBlock);
	CBBlockSerialise(testBlock, true, false);
	if (CBFullValidatorProcessBlock(validator, testBlock, 1349643202) != CB_BLOCK_STATUS_BAD) {
		printf("SPEND ON OTHER BRANCH SECOND FAIL\n");
		return 1;
	}
	// Test adding to main branch after failed reorg
	memcpy(CBByteArrayGetData(testBlock->prevBlockHash), savedHash, 32);
	testBlock->transactionNum = 2;
	testBlock->transactions[1] = CBNewTransaction(0, 1);
	inScript = CBNewScriptWithDataCopy((uint8_t []){CB_SCRIPT_OP_TRUE,CB_SCRIPT_OP_TRUE}, 2);
	CBTransactionTakeInput(testBlock->transactions[1], CBNewTransactionInput(inScript, CB_TRANSACTION_INPUT_FINAL, prevOutHash, 2));
	CBTransactionTakeOutput(testBlock->transactions[1], CBNewTransactionOutput(CB_ONE_BITCOIN / 10, emptyScript));
	CBGetMessage(testBlock->transactions[1])->bytes = CBNewByteArrayOfSize(CBTransactionCalculateLength(testBlock->transactions[1]));
	CBTransactionSerialise(testBlock->transactions[1], false);
	prevOutHash = CBNewByteArrayWithDataCopy(CBTransactionGetHash(testBlock->transactions[1]), 32);
	CBBlockCalculateAndSetMerkleRoot(testBlock);
	CBReleaseObject(CBGetMessage(testBlock)->bytes);
	CBGetMessage(testBlock)->bytes = CBNewByteArrayOfSize(CBBlockCalculateLength(testBlock, true));
	CBBlockSerialise(testBlock, true, false);
	if (CBFullValidatorProcessBlock(validator, testBlock, 1349643202) != CB_BLOCK_STATUS_MAIN) {
		printf("ADD TO MAIN BRANCH AFTER FAILED REORG FAIL\n");
		return 1;
	}
	// Test spend on other branch when adding block to new branch
	testBlock->time++;
	CBByteArraySetByte(testBlock->transactions[0]->inputs[0]->scriptObject, 0, CBByteArrayGetByte(testBlock->transactions[0]->inputs[0]->scriptObject, 0) + 1);
	CBTransactionSerialise(testBlock->transactions[0], true);
	testBlock->transactions[1]->inputs[0]->prevOut.index = 1;
	CBTransactionSerialise(testBlock->transactions[1], true);
	CBByteArray * prevOutHash2 = CBNewByteArrayWithDataCopy(CBTransactionGetHash(testBlock->transactions[1]), 32);
	CBBlockCalculateAndSetMerkleRoot(testBlock);
	CBBlockSerialise(testBlock, true, false);
	if (CBFullValidatorProcessBlock(validator, testBlock, 1349643202) != CB_BLOCK_STATUS_SIDE) {
		printf("SPEND FROM OTHER BRANCH ON LAST BLOCK FIRST FAIL\n");
		return 1;
	}
	memcpy(CBByteArrayGetData(testBlock->prevBlockHash), CBBlockGetHash(testBlock), 32);
	CBByteArraySetByte(testBlock->transactions[0]->inputs[0]->scriptObject, 0, CBByteArrayGetByte(testBlock->transactions[0]->inputs[0]->scriptObject, 0) + 1);
	CBTransactionSerialise(testBlock->transactions[0], true);
	testBlock->transactions[1]->inputs[0]->prevOut.hash = prevOutHash;
	CBTransactionSerialise(testBlock->transactions[1], true);
	CBBlockCalculateAndSetMerkleRoot(testBlock);
	CBBlockSerialise(testBlock, true, false);
	if (CBFullValidatorProcessBlock(validator, testBlock, 1349643202) != CB_BLOCK_STATUS_BAD) {
		printf("SPEND FROM OTHER BRANCH ON LAST BLOCK SECOND FAIL\n");
		return 1;
	}
	// Test successful reorganisation
	testBlock->transactions[1]->inputs[0]->prevOut.hash = prevOutHash2;
	testBlock->transactions[1]->inputs[0]->prevOut.index = 0;
	CBTransactionSerialise(testBlock->transactions[1], true);
	CBBlockCalculateAndSetMerkleRoot(testBlock);
	CBBlockSerialise(testBlock, true, false);
	if (CBFullValidatorProcessBlock(validator, testBlock, 1349643202) != CB_BLOCK_STATUS_MAIN) {
		printf("SUCESSFUL REORG WITH TRANSACTIONS FAIL\n");
		return 1;
	}
	prevOutHash = CBNewByteArrayWithDataCopy(CBTransactionGetHash(testBlock->transactions[1]), 32);
	// Test transaction is coinbase
	memcpy(CBByteArrayGetData(testBlock->prevBlockHash), CBBlockGetHash(testBlock), 32);
	testBlock->time++;
	CBByteArraySetByte(testBlock->transactions[0]->inputs[0]->scriptObject, 0, CBByteArrayGetByte(testBlock->transactions[0]->inputs[0]->scriptObject, 0) + 1);
	CBTransactionSerialise(testBlock->transactions[0], true);
	memset(CBByteArrayGetData(testBlock->transactions[1]->inputs[0]->prevOut.hash), 0, 32);
	testBlock->transactions[1]->inputs[0]->prevOut.index = 0xFFFFFFFF;
	CBTransactionSerialise(testBlock->transactions[1], true);
	CBBlockCalculateAndSetMerkleRoot(testBlock);
	CBBlockSerialise(testBlock, true, false);
	if (CBFullValidatorProcessBlock(validator, testBlock, 1349643202) != CB_BLOCK_STATUS_BAD) {
		printf("NORMAL TX IS COINBASE FAIL\n");
		return 1;
	}
	// Test transaction outputs too big.
	testBlock->transactions[1]->inputs[0]->prevOut.hash = prevOutHash;
	testBlock->transactions[1]->inputs[0]->prevOut.index = 0;
	CBTransactionTakeOutput(testBlock->transactions[1], CBNewTransactionOutput(CB_ONE_BITCOIN / 10, emptyScript));
	CBGetMessage(testBlock->transactions[1])->bytes = CBNewByteArrayOfSize(CBTransactionCalculateLength(testBlock->transactions[1]));
	CBTransactionSerialise(testBlock->transactions[1], true);
	CBBlockCalculateAndSetMerkleRoot(testBlock);
	CBGetMessage(testBlock)->bytes = CBNewByteArrayOfSize(CBBlockCalculateLength(testBlock, true));
	CBBlockSerialise(testBlock, true, false);
	if (CBFullValidatorProcessBlock(validator, testBlock, 1349643202) != CB_BLOCK_STATUS_BAD) {
		printf("TX OUTPUTS TOO BIG FAIL\n");
		return 1;
	}
	// Test referencing output index in earlier transaction in the same block that does not exist in that transaction.
	testBlock->transactions[1]->outputs[0]->value = CB_ONE_BITCOIN / 20;
	testBlock->transactions[1]->outputs[1]->value = CB_ONE_BITCOIN / 20;
	testBlock->transactions[2] = CBNewTransaction(0, 1);
	CBTransactionTakeInput(testBlock->transactions[2], CBNewTransactionInput(inScript, CB_TRANSACTION_INPUT_FINAL, CBNewByteArrayWithDataCopy(CBTransactionGetHash(testBlock->transactions[1]), 32), 3));
	CBTransactionTakeOutput(testBlock->transactions[2], CBNewTransactionOutput(CB_ONE_BITCOIN / 20, emptyScript));
	CBGetMessage(testBlock->transactions[2])->bytes = CBNewByteArrayOfSize(CBTransactionCalculateLength(testBlock->transactions[2]));
	CBTransactionSerialise(testBlock->transactions[2], false);
	testBlock->transactionNum = 3;
	CBBlockCalculateAndSetMerkleRoot(testBlock);
	CBGetMessage(testBlock)->bytes = CBNewByteArrayOfSize(CBBlockCalculateLength(testBlock, true));
	CBBlockSerialise(testBlock, true, false);
	if (CBFullValidatorProcessBlock(validator, testBlock, 1349643202) != CB_BLOCK_STATUS_BAD) {
		printf("OUTPUT REFENCE TOO HIGH INDEX FAIL\n");
		return 1;
	}
	// Test invalid input script
	testBlock->transactions[2]->inputs[0]->prevOut.index = 0;
	CBByteArraySetByte(inScript, 0, CB_SCRIPT_OP_RETURN);
	CBTransactionSerialise(testBlock->transactions[2], true);
	CBBlockCalculateAndSetMerkleRoot(testBlock);
	CBBlockSerialise(testBlock, true, false);
	if (CBFullValidatorProcessBlock(validator, testBlock, 1349643202) != CB_BLOCK_STATUS_BAD) {
		printf("INVALID INPUT SCRIPT FAIL\n");
		return 1;
	}
	// Test P2SH has non-push in input
	CBByteArraySetByte(inScript, 0, CB_SCRIPT_OP_TRUE);
	testBlock->transactions[1]->outputs[0]->scriptObject = CBNewByteArrayOfSize(23);
	CBByteArraySetByte(testBlock->transactions[1]->outputs[0]->scriptObject, 0, CB_SCRIPT_OP_HASH160);
	CBByteArraySetByte(testBlock->transactions[1]->outputs[0]->scriptObject, 1, 20);
	CBSha256((uint8_t []){CB_SCRIPT_OP_TRUE}, 1, hash);
	CBRipemd160(hash, 20, CBByteArrayGetData(testBlock->transactions[1]->outputs[0]->scriptObject) + 2);
	CBByteArraySetByte(testBlock->transactions[1]->outputs[0]->scriptObject, 22, CB_SCRIPT_OP_EQUAL);
	CBGetMessage(testBlock->transactions[1])->bytes = CBNewByteArrayOfSize(CBTransactionCalculateLength(testBlock->transactions[1]));
	CBTransactionSerialise(testBlock->transactions[1], true);
	testBlock->transactions[2]->inputs[0]->scriptObject = CBNewScriptWithDataCopy((uint8_t []){CB_SCRIPT_OP_TRUE, CB_SCRIPT_OP_DROP, 1, CB_SCRIPT_OP_TRUE}, 4);
	CBGetMessage(testBlock->transactions[2])->bytes = CBNewByteArrayOfSize(CBTransactionCalculateLength(testBlock->transactions[2]));
	testBlock->transactions[2]->inputs[0]->prevOut.hash = CBNewByteArrayWithDataCopy(CBTransactionGetHash(testBlock->transactions[1]), 32);
	CBTransactionSerialise(testBlock->transactions[2], true);
	CBBlockCalculateAndSetMerkleRoot(testBlock);
	CBGetMessage(testBlock)->bytes = CBNewByteArrayOfSize(CBBlockCalculateLength(testBlock, true));
	CBBlockSerialise(testBlock, true, false);
	if (CBFullValidatorProcessBlock(validator, testBlock, 1349643202) != CB_BLOCK_STATUS_BAD) {
		printf("P2SH INPUT PUSH FAIL\n");
		return 1;
	}
	// Test bad time against network time
	testBlock->transactionNum = 1;
	CBBlockCalculateAndSetMerkleRoot(testBlock);
	CBBlockSerialise(testBlock, true, false);
	if (CBFullValidatorProcessBlock(validator, testBlock, testBlock->time - 7201) != CB_BLOCK_STATUS_BAD_TIME) {
		printf("NETWORK TIME BAD BLOCK TIME FAIL\n");
		return 1;
	}
	// Test bad time against median time
	testBlock->time -= 7;
	CBBlockSerialise(testBlock, true, false);
	if (CBFullValidatorProcessBlock(validator, testBlock, 1349643202) != CB_BLOCK_STATUS_BAD) {
		printf("MEDIAN TIME BAD BLOCK TIME FAIL\n");
		return 1;
	}
	// Test bad coinbase
	memset(CBByteArrayGetData(testBlock->transactions[0]->inputs[0]->prevOut.hash), 1, 32);
	CBTransactionSerialise(testBlock->transactions[0], true);
	CBBlockCalculateAndSetMerkleRoot(testBlock);
	CBBlockSerialise(testBlock, true, false);
	if (CBFullValidatorProcessBlock(validator, testBlock, 1349643202) != CB_BLOCK_STATUS_BAD) {
		printf("BAD COINBASE FAIL\n");
		return 1;
	}
	memcpy(savedHash, CBByteArrayGetData(testBlock->prevBlockHash), 32);
	// Test reorganisation through entire branch
	memcpy(CBByteArrayGetData(testBlock->prevBlockHash), endOf100BlocksHash, 32);
	CBReleaseObject(testBlock->transactions[0]->outputs[0]->scriptObject);
	testBlock->transactions[0]->outputs[0]->scriptObject = CBNewScriptOfSize(3);
	CBByteArraySetByte(testBlock->transactions[0]->outputs[0]->scriptObject, 0, 1);
	CBByteArraySetByte(testBlock->transactions[0]->outputs[0]->scriptObject, 2, CB_SCRIPT_OP_EQUAL);
	memset(CBByteArrayGetData(testBlock->transactions[0]->inputs[0]->prevOut.hash), 0, 32);
	// Create and add
	for (x = 100; x < 107; x++) {
		testBlock->time++;
		CBByteArraySetByte(testBlock->transactions[0]->outputs[0]->scriptObject, 1, x);
		CBTransactionSerialise(testBlock->transactions[0], true);
		memcpy(CBByteArrayGetData(testBlock->merkleRoot), CBTransactionGetHash(testBlock->transactions[0]), 32);
		CBBlockSerialise(testBlock, true, false);
		// Add
		// At 106
		//
		// 0: 0 --> 1
		// 3:  \                               ,---> 204
		// 1:   `-> 1 -> ... -> 200 -> ... -> 203 -> 204 -> 205
		// 4:                     \                     `-> 205 -> 206  MAIN CHAIN
		// 2:                      `--------> 201 -> ... -> 205 -> 206  NEW CHAIN
		//
		// Chain path stack creation.         OK
		// Go back 2 blocks on branch 4.      OK
		// Go back 4 blocks on branch 1.      OK
		// Go forward 6 blocks on branch 2.   OK
		// Validate new block on branch 2.    OK
		// Commit.                            FAIL
		//
		if (CBFullValidatorProcessBlock(validator, testBlock, 1349643202) != ((x == 106)? CB_BLOCK_STATUS_MAIN : CB_BLOCK_STATUS_SIDE)) {
			printf("REORG OVER BRANCH NUM %u FAIL\n", x);
			return 1;
		}
		// Change previous block to the one before it
		memcpy(CBByteArrayGetData(testBlock->prevBlockHash), CBBlockGetHash(testBlock), 32);
	}
	// Test block back on the other branch
	memcpy(CBByteArrayGetData(testBlock->prevBlockHash), savedHash, 32);
	CBBlockSerialise(testBlock, true, false);
	if (CBFullValidatorProcessBlock(validator, testBlock, 1349643202) != CB_BLOCK_STATUS_SIDE) {
		printf("ADD ON OTHER BRANCH AFTER REORG OVER BRANCH FAIL\n");
		return 1;
	}
	// Test bad orphan ???
	memset(CBByteArrayGetData(testBlock->prevBlockHash), 1, 32);
	CBByteArrayGetData(testBlock->merkleRoot)[0]++;
	CBBlockSerialise(testBlock, true, false);
	if (CBFullValidatorProcessBlock(validator, testBlock, 1349643202) != CB_BLOCK_STATUS_BAD) {
		printf("BAD ORPHAN FAIL\n");
		return 1;
	}
	// Test loading orphans
	memset(CBByteArrayGetData(testBlock->prevBlockHash), 1, 32);
	CBByteArrayGetData(testBlock->merkleRoot)[0]--;
	CBBlockSerialise(testBlock, true, false);
	if (CBFullValidatorProcessBlock(validator, testBlock, 1349643202) != CB_BLOCK_STATUS_ORPHAN) {
		printf("ADD ORPHAN FAIL\n");
		return 1;
	}
	CBReleaseObject(validator);
	CBFreeBlockChainStorage(storage);
	storage = CBNewBlockChainStorage("./");
	validator = CBNewFullValidator(storage, &bad, 0);
	if (NOT validator || bad) {
		printf("LOAD VALIDATOR WITH ORPHAN FAIL\n");
		return 1;
	}
	if (validator->numOrphans != 1) {
		printf("ORPHAN NUM FAIL\n");
		return 1;
	}
	if (memcmp(CBBlockGetHash(validator->orphans[0]), CBBlockGetHash(testBlock), 32)) {
		printf("ORPHAN DATA FAIL\n");
		return 1;
	}
	return 0;
}
