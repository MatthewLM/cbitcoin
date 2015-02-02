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

#include "spv.h"

CBPeer * SPVcreateNewPeer(CBNetworkAddress *addr){
	CBPeer *peer = CBNewPeer(addr);
	peer->sendQueueSize = 1;
	peer->connectionWorking = 1;

	return peer;
}
/*
CBMessage * CBFDgetVersion(CBNetworkCommunicator *self, CBNetworkAddress * peer){

}
*/

CBNetworkCommunicator * SPVcreateSelf(CBNetworkAddress * selfAddress){
	CBNetworkCommunicator * self;
	CBNetworkCommunicatorCallbacks callbacks = {
		onPeerWhatever,
		acceptType,
		onMessageReceived,
		onNetworkError
	};
	self = CBNewNetworkCommunicator(0, callbacks);
	self->networkID = CB_PRODUCTION_NETWORK_BYTES;
	self->flags = CB_NETWORK_COMMUNICATOR_AUTO_HANDSHAKE | CB_NETWORK_COMMUNICATOR_AUTO_PING | CB_NETWORK_COMMUNICATOR_AUTO_DISCOVERY;
	self->version = CB_PONG_VERSION;
	self->maxConnections = 2;
	self->maxIncommingConnections = 0;
	self->heartBeat = 2000;
	self->timeOut = 3000;
	self->recvTimeOut = 1000;
	self->services = CB_SERVICE_NO_FULL_BLOCKS;
	self->nonce = rand();

	self->blockHeight = 0;


	//CBNetworkAddressManager * addrManager = CBNewNetworkAddressManager(onBadTime);
	//addrManager->maxAddressesInBucket = 2;
	CBByteArray * userAgent = CBNewByteArrayFromString(CB_USER_AGENT_SEGMENT, false);

	CBNetworkCommunicatorSetReachability(self, CB_IP_IP4 | CB_IP_LOCAL, true);
	CBNetworkCommunicatorSetAlternativeMessages(self, NULL, NULL);
	//CBNetworkCommunicatorSetNetworkAddressManager(self, addrManager);
	CBNetworkCommunicatorSetUserAgent(self, userAgent);
	CBNetworkCommunicatorSetOurIPv4(self, selfAddress);

	self->ipData[CB_TOR_NETWORK].isSet = false;
	self->ipData[CB_I2P_NETWORK].isSet = false;
	self->ipData[CB_IP6_NETWORK].isSet = false;

	//self->ipData[CB_IP4_NETWORK].isSet = true;
	//self->ipData[CB_IP4_NETWORK].isListening = false;
	//self->ipData[CB_IP4_NETWORK].ourAddress = selfAddress;

	//CBReleaseObject(userAgent);

	return self;

}

CBVersion * SPVNetworkCommunicatorGetVersion(CBNetworkCommunicator * self,CBPeer *peer){
	CBNetworkAddress * addRecv = peer->addr;
	CBNetworkAddress * sourceAddr = CBNetworkCommunicatorGetOurMainAddress(self, addRecv->type);
	//CBNetworkAddress * sourceAddr = self->ipData[CB_IP4_NETWORK].ourAddress;
	char x[300];
	CBNetworkAddressToString(addRecv,x);
	fprintf(stderr,"Peer Address: %s \n",x);
	CBNetworkAddressToString(sourceAddr,x);
	fprintf(stderr,"Our Address: %s \n",x);


	// If the peer's address is local give a null address
	if (addRecv->type & CB_IP_LOCAL) {
		CBByteArray * ip = CBNewByteArrayWithDataCopy(CB_NULL_ADDRESS, 16);
		addRecv = CBNewNetworkAddress(0, (CBSocketAddress){ip, 0}, 0, true);
		CBReleaseObject(ip);
	}else
		CBRetainObject(addRecv);


	CBVersion * version = CBNewVersion(
			self->version, self->services, time(NULL),
			sourceAddr, addRecv , self->nonce, self->userAgent,
			self->blockHeight
	);

	//CBReleaseObject(addRecv);
	//CBVersion *version;
	return version;
}

void spvchild(int socket) {
//    const char hello[] = "hello parent, I am child";
//	write(socket, hello, sizeof(hello)); /* NB. this includes nul */
    /* go forth and do childish things with this end of the pipe */
	// Get home directory and set the default data directory.
    // hi
	// create peer ip address
	CBByteArray * peeraddr = CBNewByteArrayWithDataCopy((uint8_t [16]){0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xFF, 0xFF, 10, 21, 0, 189}, 16);
	CBByteArray * selfaddr = CBNewByteArrayWithDataCopy((uint8_t [16]){0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xFF, 0xFF, 10, 21, 0, 67}, 16);
	CBNetworkCommunicator *self = SPVcreateSelf(CBNewNetworkAddress(0, (CBSocketAddress){selfaddr, 8333}, 0, false));
	CBPeer * peer = SPVcreateNewPeer(CBNewNetworkAddress(0, (CBSocketAddress){peeraddr, 8333}, 0, false));
	//CBReleaseObject(peeraddr);
	//CBReleaseObject(selfaddr);

	CBVersion *version = SPVNetworkCommunicatorGetVersion(self,peer);
	CBMessage *msg = CBGetMessage(version);

	// bool SPVsendMessage(CBNetworkCommunicator * self, CBPeer * peer, CBMessage * message);
	char verstr[CBVersionStringMaxSize(CBGetVersion(msg))];
	CBVersionToString(CBGetVersion(msg), verstr);
	fprintf(stderr,"main hello 3\n---\n%s\n---\n",verstr);

	SPVsendMessage(self,peer,msg);

	//fprintf(stderr, "Error: Size(%d) 4\n",msg->bytes->length);
	//return 0;
	//CBNetworkCommunicatorSendMessage, then see CBNetworkCommunicatorOnCanSend
	int x;
	uint8_t buf[24];
	fprintf(stderr,"main hello outside forza \n");
	if(SPVreceiveMessageHeader(self,peer)){
		fprintf(stderr,"main hello inside\n");
		return;
	}
	fprintf(stderr,"main hello 4\n");
}

void child(mqd_t toparent,mqd_t fromparent) {
	//fprintf(stderr, "Forking process child\n");
	const char hello[] = "hello parent, I am child";
	uint16_t len = strlen(hello);
	uint8_t buffer[len + 1];
	strcpy (buffer,hello);
	CHECK(0 <= mq_send(toparent, buffer, len, 0)); /* NB. this includes nul */
	/* go forth and do childish things with this end of the pipe */
	CHECK((mqd_t)-1 != mq_close(toparent));
	CHECK((mqd_t)-1 != mq_close(fromparent));
}

void parent(mqd_t tochild,mqd_t fromchild) {
	//fprintf(stderr, "Forking process parent\n");
	/* do parental things with this end, like reading the child's message */
	uint8_t buffer[MAX_SIZE + 1];
	ssize_t bytes_read;
	/* receive the message */
	bytes_read = mq_receive(fromchild, buffer, MAX_SIZE, NULL);

	printf("parent received mqueue '%.*s'\n", bytes_read, buffer);

	CHECK((mqd_t)-1 != mq_close(fromchild));
	CHECK((mqd_t)-1 != mq_close(tochild));
	CHECK((mqd_t)-1 != mq_unlink("/morning_up"));
	CHECK((mqd_t)-1 != mq_unlink("/morning_down"));
}

void socketfork() {
	mqd_t fd[4];
	static const int toparentsocket = 0; // for child /morning_parent
	static const int tochildsocket = 1; // for parent /morning_child
	static const int fromparentsocket = 2; // for child
	static const int fromchildsocket = 3; // for parent
	pid_t pid;

	mqd_t mq;
	struct mq_attr attr;
	char buffer[MAX_SIZE + 1];
	attr.mq_flags = 0;
	attr.mq_maxmsg = 10;
	attr.mq_msgsize = MAX_SIZE;
	attr.mq_curmsgs = 0;

	// sockets for parent
	fd[fromchildsocket] = mq_open("/morning_up", O_CREAT | O_RDONLY, 0644, &attr);
	CHECK((mqd_t)-1 != fd[fromchildsocket]);
	fd[tochildsocket] = mq_open("/morning_down", O_CREAT | O_WRONLY, 0644, &attr);
	CHECK((mqd_t)-1 != fd[tochildsocket]);
	// sockets for child
	fd[fromparentsocket] = mq_open("/morning_down", O_CREAT | O_RDONLY, 0644, &attr);
	CHECK((mqd_t)-1 != fd[fromparentsocket]);
	fd[toparentsocket] = mq_open("/morning_up", O_CREAT | O_WRONLY, 0644, &attr);
	CHECK((mqd_t)-1 != fd[toparentsocket]);
	//fprintf(stderr, "Forking process 2\n");
	/* 2. call fork ... */
	pid = fork();
	if (pid == 0) { /* 2.1 if fork returned zero, you are the child */
		CHECK((mqd_t)-1 != mq_close(fd[tochildsocket])); /* Close the parent file descriptor */
		CHECK((mqd_t)-1 != mq_close(fd[fromchildsocket])); /* Close the parent file descriptor */
		child(fd[toparentsocket],fd[fromparentsocket]);
	} else { /* 2.2 ... you are the parent */
		CHECK((mqd_t)-1 != mq_close(fd[toparentsocket])); /* Close the child file descriptor */
		CHECK((mqd_t)-1 != mq_close(fd[fromparentsocket])); /* Close the child file descriptor */
		parent(fd[tochildsocket],fd[fromchildsocket]);
	}
	exit(0); /* do everything in the parent and child functions */
}


typedef struct  {
	int fd; /*socket descriptor*/
	uint8_t *buffer;
	ssize_t buffsize;
} btcpeer;

// Using http://www.binarytides.com/tcp-connect-port-scanner-c-code-linux-sockets/
btcpeer * createbtcpeer(char *hostname,char *portNum){
	int sock, err, port;                        /* Socket descriptor */
	struct sockaddr_in sa;
	memset(&sa, 0, sizeof(sa));

	struct hostent *host;
	fprintf(stderr,"create btc peer 1\n");
	port = atoi(portNum); // for testing, use nc -l 48333 on the local host
	//strncpy((char*)&sa , "" , sizeof sa);
	sa.sin_family = AF_INET;
	fprintf(stderr,"create btc peer 2\n");
	//direct ip address, use it
	memset(&sa, 0, sizeof(sa));                /* zero the struct */
	sa.sin_family = AF_INET;
	sa.sin_addr.s_addr = htonl(INADDR_LOOPBACK); /* set destination IP number - localhost, 127.0.0.1*/
	sa.sin_port = htons(48333);                /* set destination port number */
	sock = socket(AF_INET , SOCK_STREAM , 0);
	fprintf(stderr,"create btc peer 3\n");
	if(sock < 0) 
	{
		fprintf(stderr,"\nSocket");
		exit(1);
	}
	//Connect using that socket and sockaddr structure
	//err = connect(sock , (struct sockaddr*)&sa , sizeof sa);
	if((err = connect(sock, (struct sockaddr *)&sa, sizeof(sa))) < 0)
	{
		 fprintf(stderr,"Client-connect() error");
		 close(sock);
		 exit(-1);
	}
	fprintf(stderr,"create btc peer 4\n");
	btcpeer *peer = (btcpeer*) calloc(1, sizeof(btcpeer));
	fprintf(stderr,"create btc peer 5\n");
	peer->fd = sock;
	fprintf(stderr,"create btc peer 6\n");
	peer->buffsize = 8192;
	fprintf(stderr,"create btc peer 7\n");
	return peer;
}


int main(int argc, char * argv[]) {
	fprintf(stderr, "Forking process\n");
	//socketfork();
	btcpeer *peer = createbtcpeer("127.0.0.1","48333");
	peer->buffer = "Hi, I am awesome.\n";
	write(peer->fd,peer->buffer,strlen(peer->buffer));
	close(peer->fd);
	socketfork();
}
