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

bool CBAddNetworkAddress(CBNetworkAddressLinkedList * nodes, char * ip, bool isPublic) {
	
	CBNetworkAddressLinkedListNode * node = malloc(sizeof(*node));
	
	node->addr = CBReadNetworkAddress(ip, isPublic);
	if (!node->addr)
		return false;
	
	node->next = NULL;
	
	if (nodes->last == NULL)
		nodes->start = nodes->last = node;
	else
		nodes->last = nodes->last->next = node;
	
	return true;
	
}

bool CBCheckNumber(uint32_t num) {
	
	if (num == 0){
		if (errno == ERANGE){
			CBLogWarning("Number out of range.");
			return false;
		}else if (errno == EINVAL){
			CBLogWarning("Invalid number.");
			return false;
		}
	}
	
	return true;
	
}

uint64_t CBGetMilliseconds(void) {
	
	struct timeval tv;
	
	gettimeofday(&tv, NULL);
	
	return tv.tv_sec * 1000 + tv.tv_usec / 1000;
	
}

void CBOnDoubleSpend(CBNode * node, uint8_t * txHash) {
	
}

void CBOnNewBlock(CBNode * node, CBBlock * block, uint32_t forkPoint) {
	
}

void CBOnNewTransaction(CBNode * node, CBTransaction * tx, uint64_t timestamp, uint32_t blockHeight, CBTransactionAccountDetailList * details) {
	
}

void CBOnFatalNodeError(CBNode * node, CBErrorReason reason) {
	
	if (reason == CB_ERROR_NO_PEERS)
		CBLogWarning("Could not connect to nodes. Trying again in 20 seconds (if not incoming only).");
	else{
		CBLogError("Fatal Node Error");
		exit(EXIT_FAILURE);
	}
	
}

void CBOnTransactionConfirmed(CBNode * node, uint8_t * txHash, uint32_t blockHeight) {
	
}

void CBOnTransactionUnconfirmed(CBNode * node, uint8_t * txHash) {
	
}

void CBUpToDate(CBNode * node, bool uptodate) {
	
}

CBNetworkAddress * CBReadNetworkAddress(char * ipStr, bool isPublic) {
	
	CBSocketAddress saddr = {NULL, 8333};
	char * portStart;
	
	// Determine if dealing with an IPv6 address
	if (ipStr[0] == '[') {
		struct in6_addr ip;
		char * bracket = strstr(ipStr, "]");
		if (!bracket){
			CBLogWarning("No closing bracket in IPv6 address");
			return NULL;
		}
		*bracket = '\0';
		if (inet_pton(AF_INET6, ipStr + 1, &ip) != 1) {
			CBLogWarning("Unable to obtain IPv6 address");
			return NULL;
		}
		*bracket = ']';
		saddr.ip = CBNewByteArrayWithDataCopy(ip.s6_addr, 16);
		portStart = strstr(ipStr, "]:");
		if (portStart) portStart += 2;
	}else{
		// Else IPv4
		struct in_addr ip;
		if (inet_pton(AF_INET, ipStr, &ip) != 1) {
			CBLogWarning("Unable to obtain IPv4 address");
			return NULL;
		}
		saddr.ip = CBNewByteArrayWithDataCopy((uint8_t [16]){0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xFF, 0xFF, 0, 0, 0, 0}, 16);
		CBInt32ToArrayBigEndian(CBByteArrayGetData(saddr.ip), 12, ip.s_addr);
		portStart = strchr(ipStr, ':');
		if (portStart) portStart++;
	}
	
	if (portStart){
		saddr.port = strtoul(portStart, NULL, 10);
		if (!CBCheckNumber(saddr.port)) {
			CBReleaseObject(saddr.ip);
			return NULL;
		}
	}
	
	CBNetworkAddress * addr = CBNewNetworkAddress(0, saddr, 0, isPublic);
	CBReleaseObject(saddr.ip);
	
	return addr;
	
}

void CBStartNode(void * vcomm) {
	
	CBNetworkCommunicator * comm = vcomm;
	
	// Use DNS if no addresses available.
	CBNetworkCommunicatorTryConnections(comm, comm->addresses->addrNum == 0);
	if (comm->maxIncommingConnections != 0)
		CBNetworkCommunicatorStartListening(comm);
	
}

int main(int argc, char * argv[]) {
	
	// Get home directory and set the default data directory.
	struct passwd * pwd = getpwuid(getuid());
	char defaultDataDir[strlen(pwd->pw_dir) + strlen(CB_DEFUALT_DATA_DIR) + 1];
	strcpy(defaultDataDir, pwd->pw_dir);
	strcat(defaultDataDir, CB_DEFUALT_DATA_DIR);
	char * dataDir = defaultDataDir;
	CBNetworkAddressLinkedList extraNodes = {NULL};
	bool incomingOnly = false;
	
	// Loop through arguments and set data
	for (int x = 1; x < argc; x++) {
		if (strcmp(argv[x], "--datadir") == 0)
			dataDir = argv[++x];
		else if (strcmp(argv[x], "--addnode") == 0){
			if (!CBAddNetworkAddress(&extraNodes, argv[++x], true))
				CBLogWarning("Bad IP (%s) for --addnode", argv[x]);
		}else if (strcmp(argv[x], "--connect") == 0){
			if (!CBAddNetworkAddress(&extraNodes, argv[++x], false))
				CBLogWarning("Bad IP (%s) for --connect", argv[x]);
		}else if (strcmp(argv[x], "--inonly") == 0){
			incomingOnly = true;
		}else if (strcmp(argv[x], "--version") == 0){
			puts("cbitcoin client-server " CB_LIBRARY_VERSION_STRING " (Built on " __DATE__ ")");
			puts("Copyright 2013 Matthew Mitchell");
			printf("This program is part of cbitcoin. It is subject to the license terms in the LICENSE file found in the top-level directory of the source distribution and at http://www.cbitcoin.com/license.html. No part of cbitcoin, including this file, may be copied, modified, propagated, or distributed except according to the terms contained in the LICENSE file. Run %s --license to show the license terms.\n", argv[0]);
			return 0;
		}else if (strcmp(argv[x], "--license") == 0){
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
	uint32_t commitGap = 30000;
	uint32_t cacheLimit = 10000000;
	CBNodeFlags nodeFlags = CB_NODE_CHECK_STANDARD | CB_NODE_BOOTSTRAP;
	uint32_t otherTxsSizeLimit = 10000000;
	uint32_t maxConnections = 30;
	uint32_t maxIncommingConnections = 20;
	char userAgentCustom[400];
	char * userAgentString = CB_CLIENT_USER_AGENT;
	CBNetworkAddress * explicitIPv4 = NULL;
	CBNetworkAddress * explicitIPv6 = NULL;
	
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
			if (strncmp(linePtr, "checkstandard=", 14) == 0) {
				if (linePtr[14] == '0')
					nodeFlags &= ~CB_NODE_CHECK_STANDARD;
				else if (linePtr[14] == '1')
					nodeFlags |= CB_NODE_CHECK_STANDARD;
				else
					CBLogWarning("Error with boolean 1/0 value for configuration option at line %u", lineNum);
			}else if (strncmp(linePtr, "inonly=", 7) == 0){
				if (linePtr[7] == '1')
					incomingOnly = true;
				else if (linePtr[7] != '0')
					CBLogWarning("Error with boolean 1/0 value for configuration option at line %u", lineNum);
			}else if (strncmp(linePtr, "commitgap=", 10) == 0) {
				uint32_t num = (uint32_t)strtoul(linePtr + 10, NULL, 10);
				if (CBCheckNumber(num))
					commitGap = num;
			}else if (strncmp(linePtr, "cachelim=", 9) == 0) {
				uint32_t num = (uint32_t)strtoul(linePtr + 9, NULL, 10);
				if (CBCheckNumber(num))
					cacheLimit = num;
			}else if (strncmp(linePtr, "otherlim=", 9) == 0) {
				uint32_t num = (uint32_t)strtoul(linePtr + 9, NULL, 10);
				if (CBCheckNumber(num))
					otherTxsSizeLimit = num;
			}else if (strncmp(linePtr, "maxconnections=", 15) == 0) {
				uint32_t num = (uint32_t)strtoul(linePtr + 15, NULL, 10);
				if (CBCheckNumber(num))
					maxConnections = num;
			}else if (strncmp(linePtr, "maxinconnections=", 17) == 0) {
				uint32_t num = (uint32_t)strtoul(linePtr + 17, NULL, 10);
				if (CBCheckNumber(num))
					maxIncommingConnections = num;
			}else if (strncmp(linePtr, "useragent=", 10) == 0) {
				char * spacePos = strchr(linePtr + 10, ' ');
				char * newLinePos = strchr(linePtr + 10, '\n');
				char * lowest = spacePos ? spacePos : strchr(linePtr + 10, '\0');
				if (newLinePos && newLinePos < lowest)
					lowest = newLinePos;
				size_t userAgentSize = lowest - (linePtr + 10) - 1;
				if (userAgentSize == 0 || userAgentSize > 400)
					CBLogWarning("Bad string for user agent configuration option at line %u", lineNum);
				else{
					memcpy(userAgentCustom, linePtr + 10, userAgentSize);
					userAgentString = userAgentCustom;
				}
			}else if (strncmp(linePtr, "ourip4=", 7) == 0) {
				explicitIPv4 = CBReadNetworkAddress(linePtr + 7, false);
			}else if (strncmp(linePtr, "ourip6=", 7) == 0) {
				explicitIPv6 = CBReadNetworkAddress(linePtr + 7, false);
			}else if (strncmp(linePtr, "addnode=", 8) == 0) {
				if (!CBAddNetworkAddress(&extraNodes, linePtr + 8, true))
					CBLogWarning("Bad IP for addnode configuration option at line %u", lineNum);
			}else if (strncmp(linePtr, "connect=", 8) == 0) {
				if (!CBAddNetworkAddress(&extraNodes, linePtr + 8, false)) 
					CBLogWarning("Bad IP for connect configuration option at line %u", lineNum);
			}
		}
		free(line);
		fclose(config);
	}
	
	// Create the data directory if needed.
	if (access(dataDir, F_OK)
		&& (mkdir(dataDir, S_IRWXU | S_IRWXG)
			|| chmod(dataDir, S_IRWXU | S_IRWXG))){
			CBLogError("Unable to create the data directory.");
			return EXIT_FAILURE;
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
		CBOnTransactionUnconfirmed,
		CBUpToDate
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
	while (extraNodes.start) {
		CBNetworkAddressLinkedListNode * node = extraNodes.start;
		CBNetworkAddressManagerAddAddress(comm->addresses, node->addr);
		CBReleaseObject(node->addr);
		extraNodes.start = extraNodes.start->next;
		free(node);
	}
	if (incomingOnly)
		comm->flags |= CB_NETWORK_COMMUNICATOR_INCOMING_ONLY;
	
	// Start the node
	CBRunOnEventLoop(comm->eventLoop, CBStartNode, comm, false);
	
	// Start the HTTP JSON-RPC server
	CBRPCServer * server = malloc(sizeof(*server));
	if (!CBStartRPCServer(server, node, 8332)) {
		CBLogError("Could not start RPC server.");
		return EXIT_FAILURE;
	}
	
	pthread_exit(NULL);
	
}
