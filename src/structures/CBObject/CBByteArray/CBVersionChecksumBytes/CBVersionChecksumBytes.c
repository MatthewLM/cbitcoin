//
//  CBVersionChecksumBytes.c
//  cbitcoin
//
//  Created by Matthew Mitchell on 03/05/2012.
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

#include "CBVersionChecksumBytes.h"

//  Constructors

CBVersionChecksumBytes * CBNewVersionChecksumBytesFromString(CBByteArray * string,bool cacheString,void (*onErrorReceived)(CBError error,char *,...)){
	CBVersionChecksumBytes * self = malloc(sizeof(*self));
	if (NOT self) {
		onErrorReceived(CB_ERROR_OUT_OF_MEMORY,"Cannot allocate %i bytes of memory in CBNewVersionChecksumBytesFromString\n",sizeof(*self));
		return NULL;
	}
	CBGetObject(self)->free = CBFreeVersionChecksumBytes;
	if(CBInitVersionChecksumBytesFromString(self,string,cacheString,onErrorReceived))
		return self;
	free(self);
	return NULL;
}
CBVersionChecksumBytes * CBNewVersionChecksumBytesFromBytes(uint8_t * bytes,uint32_t size,bool cacheString,void (*onErrorReceived)(CBError error,char *,...)){
	CBVersionChecksumBytes * self = malloc(sizeof(*self));
	if (NOT self) {
		onErrorReceived(CB_ERROR_OUT_OF_MEMORY,"Cannot allocate %i bytes of memory in CBNewVersionChecksumBytesFromBytes\n",sizeof(*self));
		return NULL;
	}
	CBGetObject(self)->free = CBFreeVersionChecksumBytes;
	if(CBInitVersionChecksumBytesFromBytes(self,bytes,size,cacheString,onErrorReceived))
		return self;
	free(self);
	return NULL;
}

//  Object Getter

CBVersionChecksumBytes * CBGetVersionChecksumBytes(void * self){
	return self;
}

//  Initialisers

bool CBInitVersionChecksumBytesFromString(CBVersionChecksumBytes * self,CBByteArray * string,bool cacheString,void (*onErrorReceived)(CBError error,char *,...)){
	// Cache string if needed
	if (cacheString) {
		self->cachedString = string;
		CBRetainObject(string);
	}else
		self->cachedString = NULL;
	self->cacheString = cacheString;
	// Get bytes from string conversion
	CBBigInt bytes = CBDecodeBase58Checked((char *)CBByteArrayGetData(string), onErrorReceived);
	if (bytes.length == 1) {
		return false;
	}
	// Take over the bytes with the CBByteArray
	if (NOT CBInitByteArrayWithData(CBGetByteArray(self), bytes.data, bytes.length, onErrorReceived))
		return false;
	CBByteArrayReverseBytes(CBGetByteArray(self)); // CBBigInt is in little-endian. Conversion needed to make bitcoin address the right way.
	return true;
}
bool CBInitVersionChecksumBytesFromBytes(CBVersionChecksumBytes * self,uint8_t * bytes,uint32_t size,bool cacheString,void (*onErrorReceived)(CBError error,char *,...)){
	self->cacheString = cacheString;
	self->cachedString = NULL;
	if (NOT CBInitByteArrayWithData(CBGetByteArray(self), bytes, size, onErrorReceived))
		return false;
	return true;
}

//  Destructor

void CBFreeVersionChecksumBytes(void * vself){
	CBVersionChecksumBytes * self = vself;
	if (self->cachedString) CBReleaseObject(self->cachedString);
	CBFreeByteArray(CBGetByteArray(self));
}

//  Functions

uint8_t CBVersionChecksumBytesGetVersion(CBVersionChecksumBytes * self){
	return CBByteArrayGetByte(CBGetByteArray(self), 0);
}
CBByteArray * CBVersionChecksumBytesGetString(CBVersionChecksumBytes * self){
	if (self->cachedString) {
		// Return cached string
		CBRetainObject(self->cachedString);
		return self->cachedString;
	}else{
		// Make string
		CBByteArrayReverseBytes(CBGetByteArray(self)); // Make this into little-endian
		char * string = CBEncodeBase58(CBByteArrayGetData(CBGetByteArray(self)),CBGetByteArray(self)->length);
		CBByteArray * str = CBNewByteArrayFromString(string, true, CBGetByteArray(self)->onErrorReceived);
		if (NOT str) 
			return NULL;
		CBByteArrayReverseBytes(CBGetByteArray(self)); // Now the string is got, back to big-endian.
		if (self->cacheString) {
			self->cachedString = str;
			CBRetainObject(str); // Retain for this object.
		}
		return str; // No additional retain. Retained from constructor.
	}
}
