//
//  main.c
//  cbitcoin
//
//  Created by Matthew Mitchell on 11/12/2013.
//  Copyright (c) 2013 Matthew Mitchell
//
//  This file is part of cbitcoin. It is subject to the license terms
//  in the LICENSE file found in the top-level directory of this
//  distribution and at http://www.cbitcoin.com/license.html. No part of
//  cbitcoin, including this file, may be copied, modified, propagated,
//  or distributed except according to the terms contained in the
//  LICENSE file.

#include "main.h"

bool CBCheckNumber(uint32_t num, size_t lineNum){
	if (num == 0){
		if (errno == ERANGE){
			CBLogWarning("Number out of range for configuration option at line %u", lineNum);
			return false;
		}else if (errno == EINVAL){
			CBLogWarning("Invalid number for configuration option at line %u", lineNum);
			return false;
		}
	}
	return true;
}
uint64_t CBGetMilliseconds(void){
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}
void CBOnDoubleSpend(CBNode * node, uint8_t * txHash){
	
}
void CBOnNewBlock(CBNode * node, CBBlock * block, uint32_t forkPoint){
	
}
void CBOnNewTransaction(CBNode * node, CBTransaction * tx, uint64_t timestamp, uint32_t blockHeight, CBTransactionAccountDetailList * details){
	
}
void CBOnFatalNodeError(CBNode * node){
	
}
void CBOnTransactionConfirmed(CBNode * node, uint8_t * txHash, uint32_t blockHeight){
	
}
void CBOnTransactionUnconfirmed(CBNode * node, uint8_t * txHash){
	
}
CBNetworkAddress * CBReadNetworkAddress(char * ipStr, size_t lineNum, bool isPublic){
	uint16_t port = 8333;
	CBByteArray * ipBytes;
	char * portStart;
	// Determine if dealing with an IPv6 address
	if (ipStr[0] == '[') {
		struct in6_addr ip;
		if (inet_pton(AF_INET6, ipStr + 1, &ip) != 1) {
			CBLogWarning("Unable to obtain IPv6 address for the configuration option at line %u", lineNum);
			return NULL;
		}
		ipBytes = CBNewByteArrayWithDataCopy(ip.s6_addr, 16);
		portStart = strstr(ipStr, "]:");
		if (portStart) portStart += 2;
	}else{
		// Else IPv4
		struct in_addr ip;
		if (inet_pton(AF_INET, ipStr, &ip) != 1) {
			CBLogWarning("Unable to obtain IPv4 address for the configuration option at line %u", lineNum);
			return NULL;
		}
		ipBytes = CBNewByteArrayWithDataCopy((uint8_t [16]){0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xFF, 0xFF, 0, 0, 0, 0}, 16);
		CBInt32ToArrayBigEndian(CBByteArrayGetData(ipBytes), 12, ip.s_addr);
		portStart = strchr(ipStr, ':');
		if (portStart) portStart++;
	}
	if (portStart){
		port = strtoul(portStart, NULL, 10);
		if (!CBCheckNumber(port, lineNum)) {
			CBReleaseObject(ipBytes);
			return NULL;
		}
	}
	CBNetworkAddress * addr = CBNewNetworkAddress(0, ipBytes, port, 0, isPublic);
	CBReleaseObject(ipBytes);
	return addr;
}
void CBStartNode(void * comm){
	CBNetworkCommunicatorTryConnections(comm);
	if (CBGetNetworkCommunicator(comm)->addresses->addrNum == 0) {
		CBLogError("Unable to connect to any nodes.");
		exit(EXIT_SUCCESS);
	}
	if (CBGetNetworkCommunicator(comm)->maxIncommingConnections != 0)
		CBNetworkCommunicatorStartListening(comm);
}

int main(int argc, char * argv[]){
	// Get home directory and set the default data directory.
	struct passwd * pwd = getpwuid(getuid());
	char defaultDataDir[strlen(pwd->pw_dir) + strlen(CB_DEFUALT_DATA_DIR) + 1];
	strcpy(defaultDataDir, pwd->pw_dir);
	strcat(defaultDataDir, CB_DEFUALT_DATA_DIR);
	char * dataDir = defaultDataDir;
	// Loop through arguments and set data
	for (int x = 1; x < argc; x += 2) {
		if (strcmp(argv[x], "--datadir")){
			dataDir = argv[x+1];
			if (access(dataDir, F_OK))
				CBInvalidArg("Directory cannot be accessed.");
		}else if (strcmp(argv[x], "--version")){
			puts("cbitcoin client-server " CB_LIBRARY_VERSION_STRING " (Built on " __DATE__ ")");
			puts("Copyright 2013 Matthew Mitchell");
			printf("This program is part of cbitcoin. It is subject to the license terms in the LICENSE file found in the top-level directory of the source distribution and at http://www.cbitcoin.com/license.html. No part of cbitcoin, including this file, may be copied, modified, propagated, or distributed except according to the terms contained in the LICENSE file. Run %s --license to show the license terms.\n", argv[0]);
			return 0;
		}else if (strcmp(argv[x], "--license")){
			puts(
				"\n"
				"LICENSE:\n"
				"\n"
				" Copyright (c) 2013  Matthew Mitchell\n"
				"\n"
				"  cbitcoin is free software: you can redistribute it and/or modify\n"
				"  it under the terms of the GNU General Public License as published by\n"
				"  the Free Software Foundation, either version 3 of the License, or\n"
				"  (at your option) any later version. The full text of version 3 of the GNU\n"
				"  General Public License can be found in the file LICENSE-GPLV3.\n"
				"\n"
				"  cbitcoin is distributed in the hope that it will be useful,\n"
				"  but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
				"  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
				"  GNU General Public License for more details.\n"
				"\n"
				"  You should have received a copy of the GNU General Public License\n"
				"  along with cbitcoin.  If not, see <http://www.gnu.org/licenses/>.\n"
				"\n"
				"  Additional Permissions under GNU GPL version 3 section 7\n"
				"\n"
				"  When you convey a covered work in non-source form, you are not\n"
				"  required to provide source code corresponding to the covered work.\n"
				"\n"
				"  If you modify this Program, or any covered work, by linking or\n"
				"  combining it with OpenSSL (or a modified version of that library),\n"
				"  containing parts covered by the terms of the OpenSSL License, the\n"
				"  licensors of this Program grant you additional permission to convey\n"
				"  the resulting work."
			);
			return 0;
		}else{
			printf("Unknown argument: %s\n", argv[x]);
			return 1;
		}
	}
	// Read the configuration file or use default values
	uint32_t commitGap = 600;
	uint32_t cacheLimit = 10000000;
	CBNodeFlags nodeFlags = CB_NODE_CHECK_STANDARD;
	uint32_t otherTxsSizeLimit = 10000000;
	uint32_t maxConnections = 30;
	uint32_t maxIncommingConnections = 20;
	char userAgentCustom[400];
	char * userAgentString = CB_CLIENT_USER_AGENT;
	CBNetworkAddress * explicitIPv4 = NULL;
	CBNetworkAddress * explicitIPv6 = NULL;
	CBNetworkAddressLinkedList * extraNodes = NULL, * extraNodesEnd = NULL;
	// Create filename and open file if we can
	char filename[strlen(dataDir) + 13];
	strcpy(filename, dataDir);
	strcat(filename, "bitcoin.conf");
	FILE * config = fopen(filename, "r");
	if (config) {
		// bitcoin.conf exists
		char * line, * linePtr;
		size_t lineLen = 0, lineNum = 0;
		while (getline(&line, &lineLen, config) > 0) {
			lineNum++;
			// Ignore leading white-space
			linePtr = line;
			while (linePtr[0] == ' ')
				linePtr++;
			if (linePtr[0] == '#')
				continue;
			bool addNode = false, isPublic;
			if (strncmp(linePtr, "checkstandard=", 14) == 0) {
				if (linePtr[14] == '0')
					nodeFlags &= ~CB_NODE_CHECK_STANDARD;
				else if (linePtr[14] == '1')
					nodeFlags |= CB_NODE_CHECK_STANDARD;
				else
					CBLogWarning("Error with boolean 1/0 value for configuration option at line %u", lineNum);
			}else if (strncmp(linePtr, "commitgap=", 10) == 0) {
				uint32_t num = (uint32_t)strtoul(linePtr + 10, NULL, 10);
				if (CBCheckNumber(num, lineNum))
					commitGap = num;
			}else if (strncmp(linePtr, "cachelim=", 9) == 0) {
				uint32_t num = (uint32_t)strtoul(linePtr + 9, NULL, 10);
				if (CBCheckNumber(num, lineNum))
					cacheLimit = num;
			}else if (strncmp(linePtr, "otherlim=", 9) == 0) {
				uint32_t num = (uint32_t)strtoul(linePtr + 9, NULL, 10);
				if (CBCheckNumber(num, lineNum))
					otherTxsSizeLimit = num;
			}else if (strncmp(linePtr, "maxconnections=", 15) == 0) {
				uint32_t num = (uint32_t)strtoul(linePtr + 15, NULL, 10);
				if (CBCheckNumber(num, lineNum))
					maxConnections = num;
			}else if (strncmp(linePtr, "maxinconnections=", 17) == 0) {
				uint32_t num = (uint32_t)strtoul(linePtr + 17, NULL, 10);
				if (CBCheckNumber(num, lineNum))
					maxIncommingConnections = num;
			}else if (strncmp(linePtr, "useragent=", 10) == 0) {
				char * spacePos = strchr(linePtr + 10, ' ');
				char * newLinePos = strchr(linePtr + 10, '\n');
				char * lowest = spacePos ? spacePos : strchr(linePtr + 10, '\0');
				if (newLinePos && newLinePos < lowest)
					lowest = newLinePos;
				size_t userAgentSize = lowest - (linePtr + 10) - 1;
				if (userAgentSize == 0 || userAgentSize > 400){
					CBLogWarning("Bad string for user agent configuration option at line %u", lineNum);
					continue;
				}
				memcpy(userAgentCustom, linePtr + 10, userAgentSize);
				userAgentString = userAgentCustom;
			}else if (strncmp(linePtr, "ourip4=", 7) == 0) {
				explicitIPv4 = CBReadNetworkAddress(linePtr + 7, lineNum, false);
			}else if (strncmp(linePtr, "ourip6=", 7) == 0) {
				explicitIPv6 = CBReadNetworkAddress(linePtr + 7, lineNum, false);
			}else if (strncmp(linePtr, "addnode=", 8) == 0) {
				addNode = true;
				isPublic = true;
			}else if (strncmp(linePtr, "connect=", 8) == 0) {
				addNode = true;
				isPublic = false;
			}
			if (addNode) {
				CBNetworkAddressLinkedList * node = malloc(sizeof(*node));
				node->addr = CBReadNetworkAddress(linePtr + 8, lineNum, isPublic);
				node->next = NULL;
				if (extraNodesEnd == NULL)
					extraNodes = extraNodesEnd = node;
				else
					extraNodesEnd = extraNodesEnd->next = node;
			}
		}
		free(line);
		fclose(config);
	}
	// Create the full node
	CBDepObject database;
	if (!CBNewStorageDatabase(&database, dataDir, commitGap, cacheLimit)){
		CBLogError("Could not open database.\n");
		return 1;
	}
	CBNodeCallbacks callbacks = {
		CBOnFatalNodeError,
		CBOnNewBlock,
		CBOnNewTransaction,
		CBOnTransactionConfirmed,
		CBOnDoubleSpend,
		CBOnTransactionUnconfirmed
	};
	CBNodeFull * node = CBNewNodeFull(database, nodeFlags, otherTxsSizeLimit, callbacks);
	if (!node) {
		CBLogError("Could not create the full node object.\n");
		return 1;
	}
	CBNetworkCommunicator * comm = CBGetNetworkCommunicator(node);
	comm->maxConnections = maxConnections;
	comm->maxIncommingConnections = maxIncommingConnections;
	CBByteArray * userAgent = CBNewByteArrayFromString(userAgentString, false);
	CBNetworkCommunicatorSetUserAgent(comm, userAgent);
	CBReleaseObject(userAgent);
	// Assume IPv4 and IPv6 support to start.
	CBNetworkCommunicatorSetReachability(comm, CB_IP_IP4 | CB_IP_IP6 | CB_IP_LOCAL, true);
	if (explicitIPv4) {
		explicitIPv4->isPublic = maxIncommingConnections != 0;
		CBNetworkCommunicatorSetOurIPv4(comm, explicitIPv4);
		CBReleaseObject(explicitIPv4);
	}else
		comm->flags |= CB_NETWORK_COMMUNICATOR_DETERMINE_IP4;
	if (explicitIPv6) {
		explicitIPv6->isPublic = maxIncommingConnections != 0;
		CBNetworkCommunicatorSetOurIPv4(comm, explicitIPv6);
		CBReleaseObject(explicitIPv6);
	}else
		comm->flags |= CB_NETWORK_COMMUNICATOR_DETERMINE_IP6;
	// Add the nodes
	while (extraNodes) {
		CBNetworkAddressLinkedList * node = extraNodes;
		CBNetworkAddressManagerAddAddress(comm->addresses, node->addr);
		CBReleaseObject(node->addr);
		extraNodes = extraNodes->next;
		free(node);
	}
	// Use seed nodes if no nodes available
	if (comm->addresses->addrNum == 0) {
		// Use seed nodes...
		uint8_t failures = 0;
		for (uint8_t x = 0; x < CB_SEED_DOMAIN_NUM; x++) {
			struct addrinfo * addrs;
			if (getaddrinfo(CB_SEED_DOMAINS[x], NULL, NULL, &addrs)){
				CBLogWarning("Unable to get address information from %s", CB_SEED_DOMAINS[x]);
				failures++;
				if (failures == CB_SEED_DOMAIN_NUM) {
					CBLogError("Failed to get seed nodes.");
					return EXIT_FAILURE;
				}
				continue;
			}
			for (;addrs; addrs = addrs->ai_next) {
				CBByteArray * ipBytes;
				if (addrs->ai_family == AF_INET) {
					uint32_t ipInt = ((struct sockaddr_in *)addrs->ai_addr)->sin_addr.s_addr;
					ipBytes = CBNewByteArrayWithDataCopy((uint8_t [16]){0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xFF, 0xFF, 0, 0, 0, 0}, 16);
					CBInt32ToArrayBigEndian(CBByteArrayGetData(ipBytes), 12, ipInt);
				}else
					ipBytes = CBNewByteArrayWithDataCopy(((struct sockaddr_in6 *)addrs->ai_addr)->sin6_addr.s6_addr, 16);
				CBNetworkAddress * addr = CBNewNetworkAddress(0, ipBytes, 8333, CB_SERVICE_FULL_BLOCKS, false);
				CBReleaseObject(ipBytes);
				CBNetworkAddressManagerAddAddress(comm->addresses, addr);
				CBReleaseObject(addr);
			}
		}
	}
	// Start the node
	CBRunOnEventLoop(comm->eventLoop, CBStartNode, comm, false);
	// Start the RPC server
	pthread_exit(NULL);
}
