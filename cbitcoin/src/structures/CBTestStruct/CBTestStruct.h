//
//  CBTestStruct.h
//  cbitcoin
//
//  Created by Matthew Mitchell on 29/04/2012.
//  Last modified by Matthew Mitchell on 29/04/2012.
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

//  Description of Structure
//  ========================
//  Testing... Inherits CBObject

//  Includes

#include "CBObject.h"
#include <stdbool.h>
#include <stdio.h>

//  Virtual table

typedef struct{
	CBObjectVT base;
	int (*add)(void *);
}CBTestStructVT;

//  Structure

typedef struct{
	CBObject base;
	int a;
	int b;
} CBTestStruct;

//  Constructor

CBTestStruct * CBNewTestStruct(int a, int b);

//  Virtual Table Creation

CBTestStructVT * CBCreateTestStructVT(void);
void CBSetTestStructVT(CBTestStructVT * VT);

//  Virtual Table Getter

CBTestStructVT * CBGetTestStructVT(void * self);

//  Object Getter

CBTestStruct * CBGetTestStruct(void * self);

//  Initialiser

bool CBInitTestStruct(CBTestStruct * self,int a, int b);

//  Destructor

void CBFreeTestStruct(CBTestStruct * self);
void CBFreeProcessTestStruct(CBTestStruct * self);

//  Functions

int CBTestStrustAdd(CBTestStruct * self);

