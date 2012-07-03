#!/usr/bin/env python
#
#  BUILD.py
#  cbitcoin
#
#  Created by Matthew Mitchell on 26/06/2012.
#  Copyright (c) 2012 Matthew Mitchell
#  
#  This file is part of cbitcoin.
#
#  cbitcoin is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#  
#  cbitcoin is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#  
#  You should have received a copy of the GNU General Public License
#  along with cbitcoin.  If not, see <http://www.gnu.org/licenses/>.

#  Builds cbitcoin.

import os,sys,fnmatch,subprocess,shutil
currentDir = os.path.dirname(sys.argv[0])
binPath = os.path.join(currentDir,"build","bin")
targetPath = os.path.join(binPath,"libcbitcoin.dylib")
objPath = os.path.join(currentDir,"build","obj")
incPath = os.path.join(currentDir,"build","include")
examplesPath = os.path.join(currentDir,"examples")
srcPath = os.path.join(currentDir,"src")
testPath = os.path.join(currentDir,"test")
if not os.path.exists(binPath):
    os.makedirs(binPath)
if not os.path.exists(objPath):
    os.makedirs(objPath)
if not os.path.exists(incPath):
    os.makedirs(incPath)
clean = False
test = False
for	arg in sys.argv:
	if arg == "--all":
		clean = True
	if arg == "--test":
		test = True
cflags = "-02 -Wall -pedantic -std=c99 -I" + incPath
lflags = "-Wl -flat_namespace -undefined dynamic_lookup"
# Copy Headers
for root, dirnames, filenames in os.walk(srcPath):
	for filename in fnmatch.filter(filenames, '*.h'):
		header = os.path.join(root,filename)
		headerLoc = os.path.join(incPath,filename)
		shutil.copyfile(header,headerLoc)
# Compile object files
objects = ""
for root, dirnames, filenames in os.walk(srcPath):
	for filename in fnmatch.filter(filenames, '*.c'):
		source = os.path.join(root,filename)
		objectFile = filename[:-1] + "o"
		object = os.path.join(objPath,objectFile)
		objects += " " + object
		if not os.path.exists(object) or os.path.getmtime(source) > os.path.getmtime(object) or clean:
			subprocess.check_call("gcc -c " + cflags + " -o " + object + " " + source, shell=True)
# Link object files into dynamic library
subprocess.check_call("gcc -dynamiclib " + lflags + " -o " + targetPath + objects, shell=True)
execflags = cflags + " -lssl -lcrypto -L" + binPath + " -lcbitcoin"
# Build and run tests if specified
if test:
	for root, dirnames, filenames in os.walk(testPath):
		for filename in fnmatch.filter(filenames, '*.c'):
			source = os.path.join(root,filename)
			output = os.path.join(binPath,filename[:-1])
			if not os.path.exists(output) or os.path.getmtime(source) > os.path.getmtime(output) or clean:
				subprocess.check_call("gcc " + execflags + " -o " + output + " " + source, shell=True)
				subprocess.check_call(output)
# Build example if specified
if sys.argv[-1] == "addressGenerator":
	subprocess.check_call("gcc " + execflags + " -o " + os.path.join(binPath,"addressGenerator") + " " + os.path.join(examplesPath,"addressGenerator.c"), shell=True)
