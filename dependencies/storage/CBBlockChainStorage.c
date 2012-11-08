//
//  CBBlockChainStorage.c
//  cbitcoin
//
//  Created by Matthew Mitchell on 03/11/2012.
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

//  SEE HEADER FILE FOR DOCUMENTATION

#include "CBBlockChainStorage.h"

uint64_t CBNewBlockChainStorage(char * dataDir, void (*logError)(char *,...)){
	// Try to create the object
	CBBlockChainStorage * self = malloc(sizeof(*self));
	if (NOT self) {
		logError("Could not create the block chain storage object.");
		return 0;
	}
	leveldb_options_t * options;
	options = leveldb_options_create();
	leveldb_options_set_compression(options, leveldb_no_compression);
	char * err = NULL;
	self->db = leveldb_open(options, dataDir, &err);
	if (err != NULL) {
		logError("LevelDB Open Error: %s", err);
		return 0;
	}
	leveldb_options_destroy(options);
	self->writeOptions = leveldb_writeoptions_create();
	leveldb_writeoptions_set_sync(self->writeOptions, 1);
	self->readOptions = leveldb_readoptions_create();
	leveldb_readoptions_set_verify_checksums(self->readOptions, 1);
	self->batch = NULL;
	self->logError = logError;
	return 0;
}
void CBFreeBlockChainStorage(uint64_t iself){
	CBBlockChainStorage * self = (CBBlockChainStorage *)iself;
	leveldb_close(self->db);
	leveldb_writeoptions_destroy(self->writeOptions);
	leveldb_readoptions_destroy(self->readOptions);
}
bool CBBlockChainStorageWriteValue(uint64_t iself, uint8_t * key, uint8_t keySize, uint8_t * data, uint32_t dataSize){
	CBBlockChainStorage * self = (CBBlockChainStorage *)iself;
	if (NOT self->batch) {
		// Create batch
		self->batch = leveldb_writebatch_create();
	}
	leveldb_writebatch_put(self->batch, (const char *)key, keySize, (const char *)data, dataSize);
	return true;
}
bool CBBlockChainStorageReadValue(uint64_t iself, uint8_t * key, uint8_t keySize, uint8_t * data, uint32_t * dataSize){
	CBBlockChainStorage * self = (CBBlockChainStorage *)iself;
	char * err;
	data = (uint8_t *)leveldb_get(self->db, self->readOptions, (char *)key, keySize, (size_t *)dataSize, &err);
	if (err) {
		self->logError("LevelDB Read Error: %s", err);
		return false;
	}
	return true;
}
bool CBBlockChainStorageCommitData(uint64_t iself){
	CBBlockChainStorage * self = (CBBlockChainStorage *)iself;
	char * err;
	leveldb_write(self->db, self->writeOptions, self->batch, &err);
	leveldb_writebatch_destroy(self->batch);
	if (err) {
		self->logError("LevelDB Write Error: %s", err);
		return false;
	}
	return true;
}