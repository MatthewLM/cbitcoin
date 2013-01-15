//
//  CBFullNode.c
//  cbitcoin
//
//  Created by Matthew Mitchell on 14/09/2012.
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

#include "CBFullNode.h"

//  Constructor

/*CBFullNode * CBNewFullNode(){
	CBFullNode * self = malloc(sizeof(*self));
	if (NOT self) {
		CBLogError("Cannot allocate %i bytes of memory in CBNewFullNode\n", sizeof(*self));
		return NULL;
	}
	CBGetObject(self)->free = CBFreeFullNode;
	if (CBInitFullNode(self))
		return self;
	free(self);
	return NULL;
}

//  Object Getter

CBFullNode * CBGetFullNode(void * self){
	return self;
}

//  Initialiser

bool CBInitFullNode(CBFullNode * self){
	if (NOT CBInitNetworkCommunicator(CBGetNetworkCommunicator(self)))
		return false;
	// Set network communicator fields.
	CBGetNetworkCommunicator(self)->blockHeight = 0;
	CBGetNetworkCommunicator(self)->callbackHandler = self;
	CBGetNetworkCommunicator(self)->flags = CB_NETWORK_COMMUNICATOR_AUTO_DISCOVERY | CB_NETWORK_COMMUNICATOR_AUTO_HANDSHAKE | CB_NETWORK_COMMUNICATOR_AUTO_PING;
	CBGetNetworkCommunicator(self)->version = CB_PONG_VERSION;
	CBNetworkCommunicatorSetAlternativeMessages(CBGetNetworkCommunicator(self), NULL, NULL);
	// Find home directory.
	const char * homeDir;
	struct passwd * pwd = getpwuid(getuid());
	if (NOT pwd)
		return false;
	homeDir = pwd->pw_dir;
	unsigned long homeLen = strlen(homeDir);
	// Open or create a new address store
	unsigned long dataDirLen = strlen(CB_DATA_DIRECTORY);
	char * addressFilePath = malloc(homeLen + dataDirLen + strlen(CB_ADDRESS_DATA_FILE) + 1);
	memcpy(addressFilePath, homeDir, homeLen);
	memcpy(addressFilePath + homeLen, CB_DATA_DIRECTORY, strlen(CB_DATA_DIRECTORY));
	strcpy(addressFilePath + homeLen + dataDirLen, CB_ADDRESS_DATA_FILE);
	self->addressFile = fopen(addressFilePath, "rb+");
	if (self->addressFile) {
		// The address store exists.
		free(addressFilePath);
		// Get the file length
		fseek(self->addressFile, 0, SEEK_END);
		unsigned long fileLen = ftell(self->addressFile);
		fseek(self->addressFile, 0, SEEK_SET);
		// Read file into a CBByteArray
		CBByteArray * buffer = CBNewByteArrayOfSize((uint32_t)fileLen);
		if (NOT buffer) {
			fclose(self->addressFile);
			return false;
		}
		if(fread(CBByteArrayGetData(buffer), fileLen, 1, self->addressFile) != fileLen){
			CBReleaseObject(buffer);
			fclose(self->addressFile);
			return false;
		}
		// Create the CBAddressManager
		CBGetNetworkCommunicator(self)->addresses = CBNewAddressManagerFromData(buffer, CBFullNodeOnBadTime);
		CBReleaseObject(buffer);
		if (NOT CBAddressManagerDeserialise(CBGetNetworkCommunicator(self)->addresses)){
			fclose(self->addressFile);
			CBReleaseObject(CBGetNetworkCommunicator(self)->addresses);
			CBLogError("There was an error when deserialising the CBAddressManager for the CBFullNode.");
			return false;
		}
	}else{
		// The address store does not exist
		CBGetNetworkCommunicator(self)->addresses = CBNewAddressManager(CBLogError, CBFullNodeOnBadTime);
		if (NOT CBGetNetworkCommunicator(self)->addresses)
			return false;
		// Create the file
		self->addressFile = fopen(addressFilePath, "wb");
		free(addressFilePath);
		if (NOT self->addressFile){
			CBReleaseObject(CBGetNetworkCommunicator(self)->addresses);
			return false;
		}
	}
	// Create block validator
	
	return true;
}

//  Destructor

void CBFreeFullNode(void * self){
	fclose(CBGetFullNode(self)->addressFile);
	CBReleaseObject(CBGetNetworkCommunicator(self)->addresses);
	CBFreeNetworkCommunicator(self);
}

//  Functions

void CBFullNodeOnBadTime(void * self){
	
}*/
