#!/usr/bin/env python
#
#  structure_maker.py
#  cbitcoin
#
#  Created by Matthew Mitchell on 28/04/2012.
#  Copyright (c) 2012 Matthew Mitchell
#  
#  This file is part of cbitcoin. It is subject to the license terms
#  in the LICENSE file found in the top-level directory of this
#  distribution and at http://www.cbitcoin.com/license.html. No part of
#  cbitcoin, including this file, may be copied, modified, propagated,
#  or distributed except according to the terms contained in the
#  LICENSE file.

#  This file makes a structure inheriting CBObject. May be improved to make structures inheriting a selected structure.

import os, sys
from datetime import datetime

name = raw_input("Enter the name of the new structure (Do not include CB): ")
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
 @brief "+desc+" Inherits CBObject\n\
*/\n\
\n\
#ifndef CB"+name.upper()+"H\n\
#define CB"+name.upper()+"H\n\
\n\
//  Includes\n\
\n\
#include \"CBObject.h\"\n\
\n\
/**\n\
 @brief Structure for CB"+name+" objects. @see CB"+name+".h\n\
*/\n\
typedef struct{\n\
\tCBObject base; /**< CBObject base structure */\n\
} CB"+name+";\n\
\n\
/**\n\
 @brief Creates a new CB"+name+" object.\n\
 @returns A new CB"+name+" object.\n\
 */\n\
CB"+name+" * CBNew"+name+"(void);\n\
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
//  Constructor\n\
\n\
CB"+name+" * CBNew"+name+"(){\n\
\tCB"+name+" * self = malloc(sizeof(*self));\n\
\tif (NOT self)\n\
\t\treturn NULL;\n\
\tCBGetObject(self)->free = CBFree"+name+";\n\
\tif(CBInit"+name+"(self))\n\
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
//  Initialiser\n\
\n\
bool CBInit"+name+"(CB"+name+" * self){\n\
\tif (NOT CBInitObject(CBGetObject(self)))\n\
\t\treturn false;\n\
\treturn true;\n\
}\n\
\n\
//  Destructor\n\
\n\
void CBDestroy"+name+"(void * self){\n\
\n\
}\n\
\n\
void CBFree"+name+"(void * self){\n\
\tCBDestroy"+name+"(self);\n\
\tfree(self);\n\
}\n\
\n\
//  Functions\n\
\n")
# Close files
header.close()
source.close()
