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

void spvchild(mqd_t toparent,mqd_t fromparent) {
//    const char hello[] = "hello parent, I am child";
//	write(socket, hello, sizeof(hello)); /* NB. this includes nul */
    /* go forth and do childish things with this end of the pipe */
	// Get home directory and set the default data directory.
    // hi
	// create peer ip address
	CBByteArray * peeraddr = CBNewByteArrayWithDataCopy((uint8_t [16]){0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xFF, 0xFF, 10, 21, 0, 189}, 16);
	CBByteArray * selfaddr = CBNewByteArrayWithDataCopy((uint8_t [16]){0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xFF, 0xFF, 10, 21, 0, 67}, 16);
	CBNetworkCommunicator *self = SPVcreateSelf(CBNewNetworkAddress(0, (CBSocketAddress){selfaddr, 8333}, 0, false));
	self->inbound = fromparent;
	self->outbound = toparent;
	fprintf(stderr,"Help 1 \n");
	CBPeer * peer = SPVcreateNewPeer(CBNewNetworkAddress(0, (CBSocketAddress){peeraddr, 8333}, 0, false));
	//CBReleaseObject(peeraddr);
	//CBReleaseObject(selfaddr);
	fprintf(stderr,"Help 2 \n");
	CBVersion *version = SPVNetworkCommunicatorGetVersion(self,peer);
	CBMessage *msg = CBGetMessage(version);
	fprintf(stderr,"Help 3 \n");
	// bool SPVsendMessage(CBNetworkCommunicator * self, CBPeer * peer, CBMessage * message);
	char verstr[CBVersionStringMaxSize(CBGetVersion(msg))];
	CBVersionToString(CBGetVersion(msg), verstr);

	fprintf(stderr,"Help 4 \n");

	peer->sendQueue[0].message = msg;
	peer->sendQueueSize = 1;
	peer->sendQueueFront = 0;

	char output[CB_MESSAGE_TYPE_STR_SIZE];

/////////////////////////lib event//////////////////////////////////////////////////////
	int sfd, s;
	int w[2];
	int efd;
	// 0 for inbound, 1 for outbound
	struct epoll_event event[2];
	struct epoll_event *events;

	fprintf(stderr,"Help 5 \n");

	efd = epoll_create1 (0);
	if(efd == -1){
		fprintf(stderr,"failed to create epoll mechanism. \n");
		exit(1);
	}
	event[0].data.fd = self->inbound;
	event[0].events = EPOLLIN | EPOLLET;

	event[1].data.fd = self->outbound;
	event[1].events = EPOLLOUT | EPOLLET;

	w[0] = epoll_ctl (efd, EPOLL_CTL_ADD, self->inbound, &event[0]);
	w[1] = epoll_ctl (efd, EPOLL_CTL_ADD, self->outbound, &event[1]);


	if(w[0] == -1 || w[1] == -1){
		fprintf(stderr,"Problem creating epoll watcher \n");
		exit(1);
	}
	int maxevents = 10;
	events = calloc (maxevents, 2 * sizeof event[0]);
	fprintf(stderr,"Help 6 \n");
	int activeconnections = 2;
	while(activeconnections > 0){
		int n, i;
		fprintf(stderr,"Help 7 \n");
		n = epoll_wait (efd, events, maxevents, -1);
		for(i=0;i<n;i++){
			if(
					(events[i].events & EPOLLERR) ||
					(events[i].events & EPOLLHUP) ||
					(!(events[i].events & EPOLLIN) && !(events[i].events & EPOLLOUT))
			){
				fprintf(stderr,"There was an error on this pipe. \n");
				if(events[i].events & EPOLLERR){
					fprintf(stderr,"poll error. \n");
				}
				else if(events[i].events & EPOLLHUP){
					fprintf(stderr,"poll hup. \n");
				}
				close (events[i].data.fd);
				activeconnections -= 1;
				//exit(1);

			}
			else if(events[i].data.fd == self->inbound){
				// time to read
				fprintf(stderr,"Help read \n");
				SPVreceiveMessageHeader(self,peer);
				CBMessageTypeToString(peer->receive->type, output);
				fprintf(stderr,"Received message of type: %s \n",output);
				CBFreeMessage(peer->receive);
			}
			else if(events[i].data.fd == self->outbound && peer->sendQueueSize > 0){
				// time to write
				fprintf(stderr,"In Epoll, sending message. \n");
				SPVsendMessage(self,peer,peer->sendQueue[peer->sendQueueFront].message);

				peer->sendQueueSize -= 1;

				peer->sendQueueFront += 1;
				peer->sendQueueFront = peer->sendQueueFront % CB_SEND_QUEUE_MAX_SIZE;

				CBFreeMessage(peer->sendQueue[peer->sendQueueFront].message);
			}
			else{
				fprintf(stderr,"In Epoll, but nothing to do. \n");
			}

		}

	}
///////////////////////////////////////////////////////////////////////////////
}

// Using http://www.binarytides.com/tcp-connect-port-scanner-c-code-linux-sockets/
btcpeer * createbtcpeer(char *hostname,char *portNum){
	int sock, err, port;                        /* Socket descriptor */
	struct sockaddr_in sa;
	memset(&sa, 0, sizeof(sa));

	struct hostent *host;
	//fprintf(stderr,"create btc peer 1\n");
	//port = atoi(portNum); // for testing, use nc -l 48333 on the local host
	//strncpy((char*)&sa , "" , sizeof sa);
	//sa.sin_family = AF_INET;
	//fprintf(stderr,"create btc peer 2\n");
	//direct ip address, use it
	memset(&sa, 0, sizeof(sa));                /* zero the struct */
	sa.sin_family = AF_INET;
	//sa.sin_addr.s_addr = htonl(INADDR_LOOPBACK); /* set destination IP number - localhost, 127.0.0.1*/
	sa.sin_addr.s_addr = inet_addr(hostname);
	//sa.sin_port = htons(48333);                /* set destination port number */
	sa.sin_port = htons(atoi(portNum));
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
	//fprintf(stderr,"create btc peer 4\n");
	btcpeer *peer = (btcpeer*) calloc(1, sizeof(btcpeer));
	//fprintf(stderr,"create btc peer 5\n");
	peer->fd = sock;
	//fprintf(stderr,"create btc peer 6\n");
	peer->buffsize = 8192;
	//fprintf(stderr,"create btc peer 7\n");
	return peer;
}

void start_service() {
	// tcp connection
	btcpeer *peer = createbtcpeer("10.21.0.189","8333");



	mqd_t fd[2];
	static const int toparentsocket = 0; // for child /morning_parent
	static const int fromparentsocket = 1; // for child

	mqd_t mq;
	struct mq_attr attr;
	char buffer[MAX_SIZE + 1];
	attr.mq_flags = 0;
	attr.mq_maxmsg = 10;
	attr.mq_msgsize = MAX_SIZE;
	attr.mq_curmsgs = 0;

	// sockets for child
	fd[fromparentsocket] = mq_open("/morning_down", O_CREAT | O_RDONLY, 0644, &attr);
	CHECK((mqd_t)-1 != fd[fromparentsocket]);
	fd[toparentsocket] = mq_open("/morning_up", O_CREAT | O_WRONLY, 0644, &attr);
	CHECK((mqd_t)-1 != fd[toparentsocket]);

	//child(fd[toparentsocket],fd[fromparentsocket],peer);
	spvchild(fd[toparentsocket],fd[fromparentsocket]);

	CHECK((mqd_t)-1 != mq_close(fd[toparentsocket]));
	CHECK((mqd_t)-1 != mq_close(fd[fromparentsocket]));
	CHECK((mqd_t)-1 != mq_unlink("/morning_up"));
	CHECK((mqd_t)-1 != mq_unlink("/morning_down"));

	exit(0);
}


int main(int argc, char * argv[]) {

	//socketfork();
	//peer->buffer = "Hi, I am awesome.\n";
	//write(peer->fd,peer->buffer,strlen(peer->buffer));
	start_service();
	//socketfork();
}
