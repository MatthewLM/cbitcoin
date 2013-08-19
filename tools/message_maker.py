#!/usr/bin/env python
#
#  structure_maker.py
#  cbitcoin
#
#  Created by Matthew Mitchell on 03/07/2012.
#  Copyright (c) 2012 Matthew Mitchell
#  
#  This file is part of cbitcoin. It is subject to the license terms
#  in the LICENSE file found in the top-level directory of this
#  distribution and at http://www.cbitcoin.com/license.html. No part of
#  cbitcoin, including this file, may be copied, modified, propagated,
#  or distributed except according to the terms contained in the
#  LICENSE file.

#  This file makes a structure inheriting CBMessage.

import os, sys
from datetime import datetime

name = raw_input("Enter the name of the new message structure (Do not include CB): ")
author = raw_input("Enter your first and last name: ")
desc = raw_input("Enter a description for the structure: ")
d = datetime.utcnow()
# Make files
header = open(os.path.dirname(sys.argv[0]) + "/../include/CB" + name + ".h", "w")
source = open(os.path.dirname(sys.argv[0]) + "/../src/CB" + name + ".c", "w")
# Header
header.write("//\n\
//  CB"+name+".h\n\
//  cbitcoin\n\
//\n\
//  Created by "+author+" on "+d.strftime("%d/%m/%Y")+".\n\
//  Copyright (c) 2012 Matthew Mitchell\n\
//  \n\
//  This file is part of cbitcoin. It is subject to the license terms\n\
//  in the LICENSE file found in the top-level directory of this\n\
//  distribution and at http://www.cbitcoin.com/license.html. No part of\n\
//  cbitcoin, including this file, may be copied, modified, propagated,\n\
//  or distributed except according to the terms contained in the\n\
//  LICENSE file.\n\
\n\
/**\n\
 @file\n\
 @brief "+desc+" Inherits CBMessage\n\
*/\n\
\n\
#ifndef CB"+name.upper()+"H\n\
#define CB"+name.upper()+"H\n\
\n\
//  Includes\n\
\n\
#include \"CBMessage.h\"\n\
\n\
/**\n\
 @brief Structure for CB"+name+" objects. @see CB"+name+".h\n\
*/\n\
typedef struct{\n\
\tCBMessage base; /**< CBMessage base structure */\n\
} CB"+name+";\n\
\n\
/**\n\
 @brief Creates a new CB"+name+" object.\n\
 @returns A new CB"+name+" object.\n\
*/\n\
CB"+name+" * CBNew"+name+"();\n\
/**\n\
@brief Creates a new CB"+name+" object from serialised data.\n\
 @param data Serialised CB"+name+" data.\n\
 @returns A new CB"+name+" object.\n\
*/\n\
CB"+name+" * CBNew"+name+"FromData(CBByteArray * data);\n\
\n\
/**\n\
 @brief Gets a CB"+name+" from another object. Use this to avoid casts.\n\
 @param self The object to obtain the CB"+name+" from.\n\
 @returns The CB"+name+" object.\n\
*/\n\
CB"+name+" * CBGet"+name+"(void * self);\n\
\n\
/**\n\
 @brief Initialises a CB"+name+" object\n\
 @param self The CB"+name+" object to initialise\n\
 @returns true on success, false on failure.\n\
*/\n\
bool CBInit"+name+"(CB"+name+" * self);\n\
/**\n\
 @brief Initialises a CB"+name+" object from serialised data\n\
 @param self The CB"+name+" object to initialise\n\
 @param data The serialised data.\n\
 @returns true on success, false on failure.\n\
*/\n\
bool CBInit"+name+"FromData(CB"+name+" * self, CBByteArray * data);\n\
\n\
/**\n\
@brief Release and free the objects stored by the CB"+name+" object.\n\
@param self The CB"+name+" object to destroy.\n\
*/\n\
void CBDestroy"+name+"(void * self);\n\
\n\
/**\n\
 @brief Frees a CB"+name+" object and also calls CBDestroy"+name+".\n\
 @param self The CB"+name+" object to free.\n\
 */\n\
void CBFree"+name+"(void * self);\n\
 \n\
//  Functions\n\
\n\
/**\n\
 @brief Deserialises a CB"+name+" so that it can be used as an object.\n\
 @param self The CB"+name+" object\n\
 @returns The length read on success, 0 on failure.\n\
*/\n\
uint32_t CB"+name+"Deserialise(CB"+name+" * self);\n\
/**\n\
 @brief Serialises a CB"+name+" to the byte data.\n\
 @param self The CB"+name+" object\n\
 @returns The length written on success, 0 on failure.\n\
*/\n\
uint32_t CB"+name+"Serialise(CB"+name+" * self);\n\
\n\
#endif\n");
# Source
source.write("//\n\
//  CB"+name+".c\n\
//  cbitcoin\n\
//\n\
//  Created by "+author+" on "+d.strftime("%d/%m/%Y")+".\n\
//  Copyright (c) 2012 Matthew Mitchell\n\
//  \n\
//  This file is part of cbitcoin. It is subject to the license terms\n\
//  in the LICENSE file found in the top-level directory of this\n\
//  distribution and at http://www.cbitcoin.com/license.html. No part of\n\
//  cbitcoin, including this file, may be copied, modified, propagated,\n\
//  or distributed except according to the terms contained in the\n\
//  LICENSE file.\n\
\n\
//  SEE HEADER FILE FOR DOCUMENTATION \n\
\n\
#include \"CB"+name+".h\"\n\
\n\
//  Constructors\n\
\n\
CB"+name+" * CBNew"+name+"(){\n\
\tCB"+name+" * self = malloc(sizeof(*self));\n\
\tif (! self) { \n\
\t\tCBLogError(\"Cannot allocate %i bytes of memory in CBNew"+name+"\n\", sizeof(*self));\n\
\t\treturn NULL;\n\
\t}\n\
\tCBGetObject(self)->free = CBFree"+name+";\n\
\tif(CBInit"+name+"(self))\n\
\t\treturn self;\n\
\tfree(self);\n\
\treturn NULL;\n\
}\n\
CB"+name+" * CBNew"+name+"FromData(CBByteArray * data){\n\
\tCB"+name+" * self = malloc(sizeof(*self));\n\
\tif (! self) { \n\
\t\tCBLogError(\"Cannot allocate %i bytes of memory in CBNew"+name+"FromData\n\", sizeof(*self));\n\
\t\treturn NULL;\n\
\t}\n\
\tCBGetObject(self)->free = CBFree"+name+";\n\
\tif(CBInit"+name+"FromData(self, data))\n\
\t\treturn self;\n\
\tfree(self);\n\
\treturn NULL;\n\
}\n\
\n\
//  Object Getter\n\
\n\
CB"+name+" * CBGet"+name+"(void * self){\n\
\treturn self;\n\
}\n\
\n\
//  Initialisers\n\
\n\
bool CBInit"+name+"(CB"+name+" * self){\n\
\tif (! CBInitMessageByObject(CBGetMessage(self)))\n\
\t\treturn false;\n\
\treturn true;\n\
}\n\
bool CBInit"+name+"FromData(CB"+name+" * self, CBByteArray * data){\n\
\tif (! CBInitMessageByData(CBGetMessage(self), data))\n\
\t\treturn false;\n\
\treturn true;\n\
}\n\
\n\
//  Destructor\n\
 \n\
 void CBDestroy"+name+"(void * self){\n\
 \tCBDestroyMessage(self);\n\
 }\n\
\n\
void CBFree"+name+"(void * self){\n\
\tCBDestroy"+name+"(self);\n\
\tfree(self);\n\
}\n\
\n\
//  Functions\n\
\n\
uint32_t CB"+name+"Deserialise(CB"+name+" * self){\n\
\tCBByteArray * bytes = CBGetMessage(self)->bytes;\n\
\tif (! bytes) {\n\
\t\tCBLogError(\"Attempting to deserialise a CB"+name+" with no bytes.\");\n\
\t\treturn 0;\n\
\t}\n\
}\n\
uint32_t CB"+name+"Serialise(CB"+name+" * self){\n\
\tCBByteArray * bytes = CBGetMessage(self)->bytes;\n\
\tif (! bytes) {\n\
\t\tCBLogError(\"Attempting to serialise a CB"+name+" with no bytes.\");\n\
\t\treturn 0;\n\
\t}\n\
}\n\
\n")
# Close files
header.close()
source.close()
