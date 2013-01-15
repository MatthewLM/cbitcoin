//
//  testCBAlert.c
//  cbitcoin
//
//  Created by Matthew Mitchell on 14/07/2012.
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

#include <stdio.h>
#include "CBAlert.h"
#include <time.h>
#include "stdarg.h"
#include "openssl/ssl.h"
#include "CBDependencies.h"

void CBLogError(char * format, ...);
void CBLogError(char * format, ...){
	va_list argptr;
    va_start(argptr, format);
    vfprintf(stderr, format, argptr);
    va_end(argptr);
	printf("\n");
}

int main(){
	unsigned int s = (unsigned int)time(NULL);
	s = 1337544566;
	printf("Session = %ui\n", s);
	srand(s);
	// Test deserialisation of real alert
	uint8_t data[188] = {
		0x73, // Length of payload
		0x01, 0x00, 0x00, 0x00, // Version 1
		0x37, 0x66, 0x40, 0x4F, 0x00, 0x00, 0x00, 0x00, // Relay until 1329620535
		0xB3, 0x05, 0x43, 0x4F, 0x00, 0x00, 0x00, 0x00, // Expires at 1329792435
		0xF2, 0x03, 0x00, 0x00, // ID 1010
		0xF1, 0x03, 0x00, 0x00, // Cancel < 1009
		0x00, // No more IDs
		0x10, 0x27, 0x00, 0x00, // Min version 10000
		0x48, 0xee, 0x00, 0x00, // Max version 61000
		0x00, // No user agents
		0x64, 0x00, 0x00, 0x00, // Priority 100
		0x00, // Empty hidden comment
		0x46, // Displayed comment is 70 characters long
		// "See bitcoin.org/feb20 if you have trouble connecting after 20 February"
		0x53, 0x65, 0x65, 0x20, 0x62, 0x69, 0x74, 0x63, 0x6F, 0x69, 0x6E, 0x2E, 0x6F, 0x72, 0x67, 0x2F, 0x66, 0x65, 0x62, 0x32, 0x30, 0x20, 0x69, 0x66, 0x20, 0x79, 0x6F, 0x75, 0x20, 0x68, 0x61, 0x76, 0x65, 0x20, 0x74, 0x72, 0x6F, 0x75, 0x62, 0x6C, 0x65, 0x20, 0x63, 0x6F, 0x6E, 0x6E, 0x65, 0x63, 0x74, 0x69, 0x6E, 0x67, 0x20, 0x61, 0x66, 0x74, 0x65, 0x72, 0x20, 0x32, 0x30, 0x20, 0x46, 0x65, 0x62, 0x72, 0x75, 0x61, 0x72, 0x79, 
		0x00, // No reserved
		// Signature
		0x47, // Signature is 71 bytes long
		0x30, 0x45, 0x02, 0x21, 0x00, 0x83, 0x89, 0xdf, 0x45, 0xF0, 0x70, 0x3F, 0x39, 0xEC, 0x8C, 0x1C, 0xC4, 0x2C, 0x13, 0x81, 0x0F, 0xFC, 0xAE, 0x14, 0x99, 0x5B, 0xB6, 0x48, 0x34, 0x02, 0x19, 0xE3, 0x53, 0xB6, 0x3B, 0x53, 0xEB, 0x02, 0x20, 0x09, 0xEC, 0x65, 0xE1, 0xC1, 0xAA, 0xEE, 0xC1, 0xFD, 0x33, 0x4C, 0x6B, 0x68, 0x4B, 0xDE, 0x2B, 0x3F, 0x57, 0x30, 0x60, 0xD5, 0xb7, 0x0C, 0x3A, 0x46, 0x72, 0x33, 0x26, 0xE4, 0xE8, 0xA4, 0xF1
	};
	CBByteArray * bytes = CBNewByteArrayWithDataCopy(data, 188);
	CBAlert * alert = CBNewAlertFromData(bytes);
	if(CBAlertDeserialise(alert) != 188){
		printf("DESERIALISATION LEN FAIL\n");
		return 1;
	}
	if (alert->version != 1) {
		printf("DESERIALISATION VERSION FAIL\n");
		return 1;
	}
	if (alert->relayUntil != 1329620535) {
		printf("DESERIALISATION RELAY UNTIL FAIL\n");
		return 1;
	}
	if (alert->expiration != 1329792435) {
		printf("DESERIALISATION EXPIRATION FAIL\n");
		return 1;
	}
	if (alert->ID != 1010) {
		printf("DESERIALISATION ID FAIL\n");
		return 1;
	}
	if (alert->cancel != 1009) {
		printf("DESERIALISATION CANCEL FAIL\n");
		return 1;
	}
	if (alert->setCancelNum != 0) {
		printf("DESERIALISATION SET CANCEL NUM FAIL\n");
		return 1;
	}
	if (alert->setCancel != NULL) {
		printf("DESERIALISATION SET CANCEL FAIL\n");
		return 1;
	}
	if (alert->minVer != 10000) {
		printf("DESERIALISATION MIN VERSION FAIL\n");
		return 1;
	}
	if (alert->maxVer != 61000) {
		printf("DESERIALISATION MAX VERSION FAIL\n");
		return 1;
	}
	if (alert->userAgentNum != 0) {
		printf("DESERIALISATION USER AGENT NUM FAIL\n");
		return 1;
	}
	if (alert->userAgents != NULL) {
		printf("DESERIALISATION USER AGENTS FAIL\n");
		return 1;
	}
	if (alert->priority != 100) {
		printf("DESERIALISATION PRIORITY FAIL\n");
		return 1;
	}
	if (alert->hiddenComment != NULL) {
		printf("DESERIALISATION HIDDEN COMMENT FAIL\n");
		return 1;
	}
	if (memcmp(CBByteArrayGetData(alert->displayedComment), "See bitcoin.org/feb20 if you have trouble connecting after 20 February", 70)) {
		printf("DESERIALISATION DISPLAYED COMMENT FAIL\n0x");
		char * d = (char *)CBByteArrayGetData(alert->displayedComment);
		for (int x = 0; x < 70; x++) {
			printf("%c", d[x]);
		}
		printf("\n!=\nSee bitcoin.org/feb20 if you have trouble connecting after 20 February\n");
		return 1;
	}
	if (alert->reserved != NULL) {
		printf("DESERIALISATION RESERVED FAIL\n");
		return 1;
	}
	// Check signature
	CBByteArray * payload = CBAlertGetPayload(alert);
	uint8_t hash1[32];
	CBSha256(CBByteArrayGetData(payload), payload->length, hash1);
	uint8_t hash2[32];
	CBSha256(hash1, 32, hash2);
	if (NOT CBEcdsaVerify(CBByteArrayGetData(alert->signature), alert->signature->length, hash2, (uint8_t [65]){0x04, 0xFC, 0x97, 0x02, 0x84, 0x78, 0x40, 0xAA, 0xF1, 0x95, 0xDE, 0x84, 0x42, 0xEB, 0xEC, 0xED, 0xF5, 0xB0, 0x95, 0xCD, 0xBB, 0x9B, 0xC7, 0x16, 0xBD, 0xA9, 0x11, 0x09, 0x71, 0xB2, 0x8A, 0x49, 0xE0, 0xEA, 0xD8, 0x56, 0x4F, 0xF0, 0xDB, 0x22, 0x20, 0x9E, 0x03, 0x74, 0x78, 0x2C, 0x09, 0x3B, 0xB8, 0x99, 0x69, 0x2D, 0x52, 0x4E, 0x9D, 0x6A, 0x69, 0x56, 0xE7, 0xC5, 0xEC, 0xBC, 0xD6, 0x82, 0x84}, 65)) {
		printf("DESERIALISATION SIG FAIL\n");
		return 1;
	}
	CBReleaseObject(payload);
	// Test serialisation
	memset(CBByteArrayGetData(bytes), 0, 188);
	CBReleaseObject(alert->displayedComment);
	alert->displayedComment = CBNewByteArrayWithDataCopy((uint8_t *)"See bitcoin.org/feb20 if you have trouble connecting after 20 February", 70);
	CBReleaseObject(alert->signature);
	alert->signature = CBNewByteArrayWithDataCopy((uint8_t []){0x30, 0x45, 0x02, 0x21, 0x00, 0x83, 0x89, 0xdf, 0x45, 0xF0, 0x70, 0x3F, 0x39, 0xEC, 0x8C, 0x1C, 0xC4, 0x2C, 0x13, 0x81, 0x0F, 0xFC, 0xAE, 0x14, 0x99, 0x5B, 0xB6, 0x48, 0x34, 0x02, 0x19, 0xE3, 0x53, 0xB6, 0x3B, 0x53, 0xEB, 0x02, 0x20, 0x09, 0xEC, 0x65, 0xE1, 0xC1, 0xAA, 0xEE, 0xC1, 0xFD, 0x33, 0x4C, 0x6B, 0x68, 0x4B, 0xDE, 0x2B, 0x3F, 0x57, 0x30, 0x60, 0xD5, 0xb7, 0x0C, 0x3A, 0x46, 0x72, 0x33, 0x26, 0xE4, 0xE8, 0xA4, 0xF1}, 71);
	CBAlertSerialisePayload(alert);
	if(CBAlertSerialiseSignature(alert, 116) != 188){
		printf("SERIALISATION LEN FAIL\n");
		return 1;
	}
	if (memcmp(data, CBByteArrayGetData(bytes), 188)) {
		printf("SERIALISATION FAIL\n0x");
		uint8_t * d = CBByteArrayGetData(bytes);
		for (int x = 0; x < 188; x++) {
			printf("%.2X", d[x]);
		}
		printf("\n!=\n0x");
		for (int x = 0; x < 188; x++) {
			printf("%.2X", data[x]);
		}
		return 1;
	}
	CBReleaseObject(alert);
	CBReleaseObject(bytes);
	return 0;
}
