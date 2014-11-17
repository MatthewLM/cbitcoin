//
//  CBDependencies.c
//  cbitcoin
//
//  Created by Matthew Mitchell on 28/09/2013.
//  Copyright (c) 2013 Matthew Mitchell
//  
//  This file is part of cbitcoin. It is subject to the license terms
//  in the LICENSE file found in the top-level directory of this
//  distribution and at http://www.cbitcoin.com/license.html. No part of
//  cbitcoin, including this file, may be copied, modified, propagated,
//  or distributed except according to the terms contained in the
//  LICENSE file.

//  SEE HEADER FILE FOR DOCUMENTATION 

#include "CBDependencies.h"
#include <stdlib.h>

void CBFreeTransactionAccountDetailList(CBTransactionAccountDetailList * list){
	do {
		CBTransactionAccountDetailList * temp = list;
		list = list->next;
		free(temp);
	} while (list != NULL);
}
