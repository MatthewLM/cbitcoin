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

CBVersionChecksumBytes * CBNewVersionChecksumBytesFromString(CBString * string,bool cacheString,CBEvents * events){
	CBVersionChecksumBytes * self = malloc(sizeof(*self));
	CBGetObject(self)->free = CBFreeVersionChecksumBytes;
	bool ok = CBInitVersionChecksumBytesFromString(self,string,cacheString,events);
	if (!ok) {
		return NULL;
	}
	return self;
}
CBVersionChecksumBytes * CBNewVersionChecksumBytesFromBytes(u_int8_t * bytes,u_int32_t size,bool cacheString,CBEvents * events){
	CBVersionChecksumBytes * self = malloc(sizeof(*self));
	CBGetObject(self)->free = CBFreeVersionChecksumBytes;
	CBInitVersionChecksumBytesFromBytes(self,bytes,size,cacheString,events);
	return self;
}

//  Object Getter

CBVersionChecksumBytes * CBGetVersionChecksumBytes(void * self){
	return self;
}

//  Initialisers

bool CBInitVersionChecksumBytesFromString(CBVersionChecksumBytes * self,CBString * string,bool cacheString,CBEvents * events){
	// Cache string if needed
	if (cacheString) {
		self->cached = string;
		CBRetainObject(string);
	}else
		self->cached = NULL;
	self->cacheString = cacheString;
	// Get bytes from string conversion
	CBBigInt bytes = CBDecodeBase58Checked(string->string, events);
	if (bytes.length == 1) {
		return false;
	}
	// Take over the bytes with the CBByteArray
	if (!CBInitByteArrayWithData(CBGetByteArray(self), bytes.data, bytes.length, events))
		return false;
	CBByteArrayReverseBytes(CBGetByteArray(self)); // CBBigInt is in little-endian. Conversion needed to make bitcoin address the right way.
	return true;
}
bool CBInitVersionChecksumBytesFromBytes(CBVersionChecksumBytes * self,u_int8_t * bytes,u_int32_t size,bool cacheString,CBEvents * events){
	self->cacheString = cacheString;
	self->cached = NULL;
	if (!CBInitByteArrayWithData(CBGetByteArray(self), bytes, size, events))
		return false;
	return true;
}

//  Destructor

void CBFreeVersionChecksumBytes(void * vself){
	CBVersionChecksumBytes * self = vself;
	free(self->cached);
	CBFreeByteArray(CBGetByteArray(self));
}

//  Functions

u_int8_t CBVersionChecksumBytesGetVersion(CBVersionChecksumBytes * self){
	return CBByteArrayGetByte(CBGetByteArray(self), 0);
}
CBString * CBVersionChecksumBytesGetString(CBVersionChecksumBytes * self){
	if (self->cached) {
		// Return cached string
		CBRetainObject(self->cached);
		return self->cached;
	}else{
		// Make string
		CBByteArrayReverseBytes(CBGetByteArray(self)); // Make this into little-endian
		CBString * str = CBNewStringByTakingCString(CBEncodeBase58(CBByteArrayGetData(CBGetByteArray(self)), CBGetByteArray(self)->length));
		CBByteArrayReverseBytes(CBGetByteArray(self)); // Now the string is got, back to big-endian.
		if (self->cacheString) {
			self->cached = str;
			CBRetainObject(str); // Retain for this object.
		}
		return str; // No additional retain. Retained from constructor.
	}
}
