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

void onErrorReceived(char * format,...);
void onErrorReceived(char * format,...){
	va_list argptr;
    va_start(argptr, format);
    vfprintf(stderr, format, argptr);
    va_end(argptr);
	printf("\n");
}

int main(){
	/*remove("./validation.dat");
	remove("./branch0.dat");
	remove("./blocks0-0.dat");
	remove("./blocks1-0.dat");
	// Create validator
	CBFullValidator * validator = CBNewFullValidator("./", onErrorReceived);
	// Create initial data
	if (NOT CBFullValidatorLoadValidator(validator)){
		printf("VALIDATOR LOAD INIT FAIL\n");
		return 1;
	}
	if (NOT CBFullValidatorLoadBranchValidator(validator,0)){
		printf("VALIDATOR LOAD BRANCH INIT FAIL\n");
		return 1;
	}
	CBReleaseObject(validator);
	// Now create it again. It should load the data.
	validator = CBNewFullValidator("./", onErrorReceived);
	if (NOT CBFullValidatorLoadValidator(validator)){
		printf("VALIDATOR LOAD FROM FILE FAIL\n");
		return 1;
	}
	if (NOT CBFullValidatorLoadBranchValidator(validator,0)){
		printf("VALIDATOR LOAD BRANCH FROM FILE FAIL\n");
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
	if(validator->mainBranch != 0){
		printf("MAIN BRANCH FAIL\n");
		return 1;
	}
	if(validator->branches[0].lastRetargetTime != 1231006505){
		printf("LAST RETARGET FAIL\n");
		return 1;
	}
	if(validator->branches[0].numRefs != 1){
		printf("NUM REFS FAIL\n");
		return 1;
	}
	if(validator->branches[0].parentBranch){
		printf("PARENT BRANCH FAIL\n");
		return 1;
	}
	if(memcmp(validator->branches[0].referenceTable[0].blockHash,(uint8_t []){0x6F,0xE2,0x8C,0x0A,0xB6,0xF1,0xB3,0x72,0xC1,0xA6,0xA2,0x46,0xAE,0x63,0xF7,0x4F,0x93,0x1E,0x83,0x65,0xE1,0x5A,0x08,0x9C,0x68,0xD6,0x19,0x00,0x00,0x00,0x00,0x00},32)){
		printf("GENESIS REF HASH FAIL\n");
		return 1;
	}
	if(validator->branches[0].references[0].ref.fileID){
		printf("FILE ID FAIL\n");
		return 1;
	}
	if(validator->branches[0].references[0].ref.filePos){
		printf("FILE POS FAIL\n");
		return 1;
	}
	if(validator->branches[0].references[0].target != CB_MAX_TARGET){
		printf("REF TARGET FAIL\n");
		return 1;
	}
	if(validator->branches[0].references[0].time != 1231006505){
		printf("REF TIME FAIL\n");
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
	// Test unspent coinbase output
	if (validator->branches->numUnspentOutputs != 1){
		printf("NUM UNSPENT OUTPUTS FAIL\n");
		return 1;
	}
	if (validator->branches->unspentOutputs[0].branch) {
		printf("UNSPENT OUTPUT BRANCH FAIL\n");
		return 1;
	}
	if (NOT validator->branches->unspentOutputs[0].coinbase) {
		printf("UNSPENT OUTPUT COINBASE FAIL\n");
		return 1;
	}
	if (validator->branches->unspentOutputs[0].height) {
		printf("UNSPENT OUTPUT HEIGHT FAIL\n");
		return 1;
	}
	if (memcmp(validator->branches->unspentOutputs[0].outputHash,(uint8_t []){0x3b,0xa3,0xed,0xfd,0x7a,0x7b,0x12,0xb2,0x7a,0xc7,0x2c,0x3e,0x67,0x76,0x8f,0x61,0x7f,0xc8,0x1b,0xc3,0x88,0x8a,0x51,0x32,0x3a,0x9f,0xb8,0xaa,0x4b,0x1e,0x5e,0x4a},32)) {
		printf("UNSPENT OUTPUT HASH FAIL\n");
		return 1;
	}
	if (validator->branches->unspentOutputs[0].ref.fileID) {
		printf("UNSPENT OUTPUT FILE ID FAIL\n");
		return 1;
	}
	if (validator->branches->unspentOutputs[0].ref.filePos != 209) {
		printf("UNSPENT OUTPUT FILE POS FAIL\n");
		return 1;
	}
	// Verify unspent output is correct
	FILE * fd = CBFullValidatorGetBlockFile(validator, 0, 0);
	fseek(fd, 209, SEEK_SET);
	CBByteArray * outputBytes = CBNewByteArrayOfSize(76, onErrorReceived);
	if (fread(CBByteArrayGetData(outputBytes), 1, 76, fd) != 76){
		printf("UNSPENT OUTPUT READ FAIL\n");
		return 1;
	}
	CBTransactionOutput * output = CBNewTransactionOutputFromData(outputBytes, onErrorReceived);
	CBTransactionOutputDeserialise(output);
	if (output->value != 50 * CB_ONE_BITCOIN) {
		printf("UNSPENT OUTPUT VALUE FAIL\n");
		return 1;
	}
	if (output->scriptObject->length != 67) {
		printf("UNSPENT OUTPUT SCRIPT SIZE FAIL\n");
		return 1;
	}
	if (memcmp(CBByteArrayGetData(output->scriptObject),(uint8_t []){0x41,0x04,0x67,0x8A,0xFD,0xB0,0xFE,0x55,0x48,0x27,0x19,
		0x67,0xF1,0xA6,0x71,0x30,0xB7,0x10,0x5C,0xD6,0xA8,0x28,0xE0,0x39,0x09,0xA6,0x79,0x62,0xE0,0xEA,0x1F,0x61,0xDE,0xB6,0x49,0xF6,
		0xBC,0x3F,0x4C,0xEF,0x38,0xC4,0xF3,0x55,0x04,0xE5,0x1E,0xC1,0x12,0xDE,0x5C,0x38,0x4D,0xF7,0xBA,0x0B,0x8D,0x57,0x8A,0x4C,0x70,
		0x2B,0x6B,0xF1,0x1D,0x5F,0xAC},67)) {
			printf("UNSPENT OUTPUT SCRIPT FAIL\n");
			return 1;
	}
	CBReleaseObject(output);
	CBReleaseObject(outputBytes);
	// Try loading the genesis block
	CBBlock * block = CBFullValidatorLoadBlock(validator, validator->branches[0].references[0], 0);
	CBBlockDeserialise(block, true);
	if (NOT block) {
		printf("GENESIS RETRIEVE FAIL\n");
		return 1;
	}
	if (memcmp(CBBlockGetHash(block),(uint8_t []){0x6F,0xE2,0x8C,0x0A,0xB6,0xF1,0xB3,0x72,0xC1,0xA6,0xA2,0x46,0xAE,0x63,0xF7,0x4F,0x93,0x1E,0x83,0x65,0xE1,0x5A,0x08,0x9C,0x68,0xD6,0x19,0x00,0x00,0x00,0x00,0x00},32)) {
		printf("GENESIS HASH FAIL\n");
		return 1;
	}
	if (memcmp(CBByteArrayGetData(block->merkleRoot),(uint8_t []){0x3B,0xA3,0xED,0xFD,0x7A,0x7B,0x12,0xB2,0x7A,0xC7,0x2C,0x3E,0x67,0x76,0x8F,0x61,0x7F,0xC8,0x1B,0xC3,0x88,0x8A,0x51,0x32,0x3A,0x9F,0xB8,0xAA,0x4B,0x1E,0x5E,0x4A},32)) {
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
	CBByteArray * genesisInScript = CBNewByteArrayWithDataCopy((uint8_t [77]){0x04,0xFF,0xFF,0x00,0x1D,0x01,0x04,0x45,0x54,0x68,0x65,0x20,0x54,0x69,0x6D,0x65,0x73,0x20,0x30,0x33,0x2F,0x4A,0x61,0x6E,0x2F,0x32,0x30,0x30,0x39,0x20,0x43,0x68,0x61,0x6E,0x63,0x65,0x6C,0x6C,0x6F,0x72,0x20,0x6F,0x6E,0x20,0x62,0x72,0x69,0x6E,0x6B,0x20,0x6F,0x66,0x20,0x73,0x65,0x63,0x6F,0x6E,0x64,0x20,0x62,0x61,0x69,0x6C,0x6F,0x75,0x74,0x20,0x66,0x6F,0x72,0x20,0x62,0x61,0x6E,0x6B,0x73}, 77, onErrorReceived);
	CBByteArray * genesisOutScript = CBNewByteArrayWithDataCopy((uint8_t [67]){0x41,0x04,0x67,0x8A,0xFD,0xB0,0xFE,0x55,0x48,0x27,0x19,0x67,0xF1,0xA6,0x71,0x30,0xB7,0x10,0x5C,0xD6,0xA8,0x28,0xE0,0x39,0x09,0xA6,0x79,0x62,0xE0,0xEA,0x1F,0x61,0xDE,0xB6,0x49,0xF6,0xBC,0x3F,0x4C,0xEF,0x38,0xC4,0xF3,0x55,0x04,0xE5,0x1E,0xC1,0x12,0xDE,0x5C,0x38,0x4D,0xF7,0xBA,0x0B,0x8D,0x57,0x8A,0x4C,0x70,0x2B,0x6B,0xF1,0x1D,0x5F,0xAC}, 67, onErrorReceived);
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
	// Test validating a correct block onto the genesis block
	CBBlock * block1 = CBNewBlock(onErrorReceived);
	block1->version = 1;
	block1->prevBlockHash = CBNewByteArrayWithDataCopy((uint8_t []){0x6F,0xE2,0x8C,0x0A,0xB6,0xF1,0xB3,0x72,0xC1,0xA6,0xA2,0x46,0xAE,0x63,0xF7,0x4F,0x93,0x1E,0x83,0x65,0xE1,0x5A,0x08,0x9C,0x68,0xD6,0x19,0x00,0x00,0x00,0x00,0x00}, 32, onErrorReceived);
	block1->merkleRoot = CBNewByteArrayWithDataCopy((uint8_t []){0x98,0x20,0x51,0xfD,0x1E,0x4B,0xA7,0x44,0xBB,0xCB,0x68,0x0E,0x1F,0xEE,0x14,0x67,0x7B,0xA1,0xA3,0xC3,0x54,0x0B,0xF7,0xB1,0xCD,0xB6,0x06,0xE8,0x57,0x23,0x3E,0x0E}, 32, onErrorReceived);
	block1->time = 1231469665;
	block1->target = CB_MAX_TARGET;
	block1->nonce = 2573394689;
	block1->transactionNum = 1;
	block1->transactions = malloc(sizeof(*block1->transactions));
	block1->transactions[0] = CBNewTransaction(0, 1, onErrorReceived);
	CBByteArray * nullHash = CBNewByteArrayOfSize(32, onErrorReceived);
	memset(CBByteArrayGetData(nullHash), 0, 32);
	CBScript * script = CBNewScriptWithDataCopy((uint8_t []){0x04,0xff,0xff,0x00,0x1d,0x01,0x04}, 7, onErrorReceived);
	CBTransactionTakeInput(block1->transactions[0], CBNewTransactionInput(script, CB_TRANSACTION_INPUT_FINAL, nullHash, 0xFFFFFFFF, onErrorReceived));
	CBReleaseObject(script);
	script = CBNewScriptWithDataCopy((uint8_t []){0x41,0x04,0x96,0xb5,0x38,0xe8,0x53,0x51,0x9c,0x72,0x6a,0x2c,0x91,0xe6,0x1e,0xc1,0x16,0x00,0xae,0x13,0x90,0x81,0x3a,0x62,0x7c,0x66,0xfb,0x8b,0xe7,0x94,0x7b,0xe6,0x3c,0x52,0xda,0x75,0x89,0x37,0x95,0x15,0xd4,0xe0,0xa6,0x04,0xf8,0x14,0x17,0x81,0xe6,0x22,0x94,0x72,0x11,0x66,0xbf,0x62,0x1e,0x73,0xa8,0x2c,0xbf,0x23,0x42,0xc8,0x58,0xee,0xAC}, 67, onErrorReceived);
	CBTransactionTakeOutput(block1->transactions[0], CBNewTransactionOutput(5000000000, script, onErrorReceived));
	CBReleaseObject(script);
	CBGetMessage(block1)->bytes = CBNewByteArrayOfSize(CBBlockCalculateLength(block1, true), onErrorReceived);
	CBBlockSerialise(block1, true, false);
	// Made block now proceed with the test.
	printf("%lli\n",CB_MAX_MONEY);
	CBBlockStatus res = CBFullValidatorProcessBlock(validator, block1, 1349643202);
	if (res != CB_BLOCK_STATUS_MAIN) {
		printf("MAIN CHAIN ADD FAIL\n");
		return 1;
	}
	// Check validator data is correct, after closing and loading data
	CBReleaseObject(validator);
	validator = CBNewFullValidator("./", onErrorReceived);
	if (NOT CBFullValidatorLoadValidator(validator)){
		printf("BLOCK ONE LOAD FROM FILE FAIL\n");
		return 1;
	}
	if (NOT CBFullValidatorLoadBranchValidator(validator,0)){
		printf("BLOCK ONE LOAD BRANCH FROM FILE FAIL\n");
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
	if (validator->branches[0].numRefs != 2) {
		printf("BLOCK ONE NUM REFS FAIL\n");
		return 1;
	}
	if (validator->branches[0].numUnspentOutputs != 2) {
		printf("BLOCK ONE NUM UNSPENT OUTPUTS FAIL\n");
		return 1;
	}
	if (validator->branches[0].references[1].target != CB_MAX_TARGET) {
		printf("BLOCK ONE REF TARGET FAIL\n");
		return 1;
	}
	if (validator->branches[0].references[1].time != 1231469665) {
		printf("BLOCK ONE REF TIME FAIL\n");
		return 1;
	}
	if (NOT validator->branches[0].referenceTable[0].index || validator->branches[0].referenceTable[1].index) {
		printf("BLOCK ONE REF TABLE INDEXES FAIL\n");
		return 1;
	}
	if (memcmp(validator->branches[0].referenceTable[0].blockHash,CBBlockGetHash(block1),32)) {
		printf("BLOCK ONE REF TABLE FIRST HASH FAIL\n");
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
	if (memcmp(validator->branches[0].work.data, (uint8_t []){0x01,0x00,0x01,0x00,0x01}, 5)) {
		printf("BLOCK ONE WORK FAIL\n");
		return 1;
	}
	// Try to load block
	block1 = CBFullValidatorLoadBlock(validator, validator->branches[0].references[1], 0);
	CBBlockDeserialise(block1, true);
	if (NOT block1) {
		printf("BLOCK ONE LOAD FAIL\n");
		return 1;
	}
	if (memcmp(CBByteArrayGetData(block1->merkleRoot),(uint8_t []){0x98,0x20,0x51,0xfD,0x1E,0x4B,0xA7,0x44,0xBB,0xCB,0x68,0x0E,0x1F,0xEE,0x14,0x67,0x7B,0xA1,0xA3,0xC3,0x54,0x0B,0xF7,0xB1,0xCD,0xB6,0x06,0xE8,0x57,0x23,0x3E,0x0E},32)) {
		printf("BLOCK ONE MERKLE ROOT FAIL\n");
		return 1;
	}
	if (block1->nonce != 2573394689) {
		printf("BLOCK ONE NOUNCE FAIL\n");
		return 1;
	}
	if (memcmp(CBByteArrayGetData(block1->prevBlockHash),(uint8_t []){0x6F,0xE2,0x8C,0x0A,0xB6,0xF1,0xB3,0x72,0xC1,0xA6,0xA2,0x46,0xAE,0x63,0xF7,0x4F,0x93,0x1E,0x83,0x65,0xE1,0x5A,0x08,0x9C,0x68,0xD6,0x19,0x00,0x00,0x00,0x00,0x00},32)) {
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
	if (memcmp(CBByteArrayGetData(block1->transactions[0]->inputs[0]->scriptObject), (uint8_t []){0x04,0xff,0xff,0x00,0x1d,0x01,0x04}, 7)) {
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
	if (memcmp(CBByteArrayGetData(block1->transactions[0]->outputs[0]->scriptObject), (uint8_t []){0x41,0x04,0x96,0xb5,0x38,0xe8,0x53,0x51,0x9c,0x72,0x6a,0x2c,0x91,0xe6,0x1e,0xc1,0x16,0x00,0xae,0x13,0x90,0x81,0x3a,0x62,0x7c,0x66,0xfb,0x8b,0xe7,0x94,0x7b,0xe6,0x3c,0x52,0xda,0x75,0x89,0x37,0x95,0x15,0xd4,0xe0,0xa6,0x04,0xf8,0x14,0x17,0x81,0xe6,0x22,0x94,0x72,0x11,0x66,0xbf,0x62,0x1e,0x73,0xa8,0x2c,0xbf,0x23,0x42,0xc8,0x58,0xee,0xAC}, 67)) {
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
	if (validator->branches[0].unspentOutputs[1].branch != 0) {
		printf("BLOCK ONE UNSPENT OUTPUT BRANCH FAIL\n");
		return 1;
	}
	if (NOT validator->branches[0].unspentOutputs[1].coinbase) {
		printf("BLOCK ONE UNSPENT OUTPUT COINBASE FAIL\n");
		return 1;
	}
	if (NOT validator->branches[0].unspentOutputs[1].height != 1) {
		printf("BLOCK ONE UNSPENT OUTPUT HEIGHT FAIL\n");
		return 1;
	}
	if (memcmp(validator->branches[0].unspentOutputs[1].outputHash,(uint8_t []){0x98,0x20,0x51,0xfD,0x1E,0x4B,0xA7,0x44,0xBB,0xCB,0x68,0x0E,0x1F,0xEE,0x14,0x67,0x7B,0xA1,0xA3,0xC3,0x54,0x0B,0xF7,0xB1,0xCD,0xB6,0x06,0xE8,0x57,0x23,0x3E,0x0E},32)) {
		printf("BLOCK ONE UNSPENT OUTPUT HASH FAIL\n");
		return 1;
	}
	if (validator->branches[0].unspentOutputs[1].outputIndex) {
		printf("BLOCK ONE UNSPENT OUTPUT INDEX FAIL\n");
		return 1;
	}
	if (validator->branches[0].unspentOutputs[1].ref.fileID) {
		printf("BLOCK ONE UNSPENT OUTPUT FILE ID FAIL\n");
		return 1;
	}
	fd = CBFullValidatorGetBlockFile(validator, 0, 0);
	fseek(fd, validator->branches[0].unspentOutputs[1].ref.filePos, SEEK_SET);
	outputBytes = CBNewByteArrayOfSize(CBGetMessage(block1->transactions[0]->outputs[0])->bytes->length, onErrorReceived);
	if (fread(CBByteArrayGetData(outputBytes), 1, outputBytes->length, fd) != outputBytes->length){
		printf("BLOCK ONE UNSPENT OUTPUT READ FAIL\n");
		return 1;
	}
	// Verify same data
	if (CBByteArrayCompare(CBGetMessage(block1->transactions[0]->outputs[0])->bytes, outputBytes)) {
		printf("BLOCK ONE UNSPENT OUTPUT DATA CONSISTENCY FAIL\n");
		return 1;
	}
	CBReleaseObject(outputBytes);
	// Test duplicate add.
	res = CBFullValidatorProcessBlock(validator, block1, 1349643202);
	if (res != CB_BLOCK_STATUS_DUPLICATE) {
		printf("MAIN CHAIN ADD DUPLICATE FAIL\n");
		return 1;
	}
	// Add 100 blocks to test
	CBBlock * theBlocks[100];
	CBByteArray * prevHash = CBNewByteArrayWithDataCopy((uint8_t []){0x6F,0xE2,0x8C,0x0A,0xB6,0xF1,0xB3,0x72,0xC1,0xA6,0xA2,0x46,0xAE,0x63,0xF7,0x4F,0x93,0x1E,0x83,0x65,0xE1,0x5A,0x08,0x9C,0x68,0xD6,0x19,0x00,0x00,0x00,0x00,0x00}, 32, onErrorReceived);
	uint32_t time = 1231006506;
	for (uint8_t x = 0; x < 100; x++) {
		block = CBNewBlock(onErrorReceived);
		block->version = 1;
		if (x == 1 || x == 3 || NOT ((x-1) % 6))
			time++;
		block->time = time;
		block->transactionNum = 1;
		block->transactions = malloc(sizeof(*block->transactions));
		block->transactions[0] = CBNewTransaction(0, 1, onErrorReceived);
		CBScript * nullScript = CBNewScriptOfSize(0, onErrorReceived);
		CBScript * inScript = CBNewScriptOfSize(2, onErrorReceived);
		CBTransactionTakeInput(block->transactions[0],
							   CBNewTransactionInput(inScript, CB_TRANSACTION_INPUT_FINAL, nullHash, 0xFFFFFFFF, onErrorReceived));
		CBReleaseObject(inScript);
		CBTransactionTakeOutput(block->transactions[0], CBNewTransactionOutput(5000000000, nullScript, onErrorReceived));
		CBReleaseObject(nullScript);
		block->target = CB_MAX_TARGET;
		block->prevBlockHash = prevHash;
		CBGetMessage(block)->bytes = CBNewByteArrayOfSize(143, onErrorReceived);
		CBGetMessage(block->transactions[0])->bytes = CBNewByteArrayOfSize(62, onErrorReceived);
		CBByteArraySetByte(block->transactions[0]->inputs[0]->scriptObject, 0, blockInfo[x].extranonce);
		CBByteArraySetByte(block->transactions[0]->inputs[0]->scriptObject, 1, x);
		block->nonce = blockInfo[x].nonce;
		CBTransactionSerialise(block->transactions[0], true);
		block->merkleRoot = CBNewByteArrayWithDataCopy(CBTransactionGetHash(block->transactions[0]), 32, onErrorReceived);
		CBBlockSerialise(block, true, false);
		theBlocks[x] = block;
		prevHash = CBNewByteArrayWithDataCopy(CBBlockGetHash(block), 32, onErrorReceived);
	}
	for (uint8_t x = 0; x < 102; x++) {
		uint8_t y = x;
		if (x < 22)
			y += 3;
		else if (x < 27)
			y -= 22;
		else
			y -= 2;
		printf("PROCESSING BLOCK %u\n",y);
		res = CBFullValidatorProcessBlock(validator, theBlocks[y], 1230999326);
		if (x < 22){
			if (res != CB_BLOCK_STATUS_ORPHAN) {
				printf("ORPHAN FAIL AT %u\n",y);
				return 1;
			}
		}else if (x == 22){
			if (res != CB_BLOCK_STATUS_SIDE) {
				printf("SIDE FAIL AT %u\n",y);
				return 1;
			}
		}else if (res != CB_BLOCK_STATUS_MAIN) {
			// Reorg occurs at x == 1
			printf("MAIN FAIL AT %u\n",y);
			return 1;
		}
		if (x > 1)
			CBReleaseObject(theBlocks[y]);
	}
	// Free data
	CBReleaseObject(block1);
	CBReleaseObject(validator);
	CBReleaseObject(nullHash);*/
	return 0;
}
