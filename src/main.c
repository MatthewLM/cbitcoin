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






int main(int argc, char * argv[]) {
	fprintf(stderr, "Error: Hello\n");
	// Get home directory and set the default data directory.
    // hi
	CBNetworkAddress * peerAddress = CBReadNetworkAddress("10.21.0.189", false);
	CBNetworkAddress * selfAddress = CBReadNetworkAddress("10.21.0.67", false);
	CBNetworkCommunicator *self = createSelf();
	CBMessage *msg = CBFDgetVersion(self,peerAddress);


	//fprintf(stderr, "Error: Size(%d) 4\n",CBGetMessage(version)->bytes->length);

//CBNetworkCommunicatorSendMessage, then see CBNetworkCommunicatorOnCanSend

}
