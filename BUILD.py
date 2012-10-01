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

#  Builds cbitcoin. Anyone is welcome to implement a makefile to substitute this and submit a pull request to https://github.com/MatthewLM/cbitcoin

#############################################################################

########################    CONFIGURATION SECTION    ########################

AUTO_CONFIG = True # True for automatic configuration of the following variables. Does not override OPTIMISATION_LEVEL
LIBEVENT_LOCATION = "" # Needed for tests
OPENSSL_LOCATION = "" # Needed for tests
LIBEV_LOCATION = "" # Needed for tests
OPTIMISATION_LEVEL = "2" # No optimisation with the "--debug" option
ADDITIONAL_CFLAGS = ""
ADDITIONAL_LFLAGS = ""
ADDITIONAL_OPENSSL_LFLAGS = "" # For tests

#############################################################################
import os,sys,fnmatch,subprocess,shutil,platform
def compile(flags,output,source,clean):
	if not os.path.exists(output) or os.path.getmtime(source) > os.path.getmtime(output) or clean:
		print "COMPILING " + output
		subprocess.check_call("gcc -c " + flags + " -o " + output + " " + source, shell=True)
	else:
		print "USING PREVIOUS " + output
# Auto config
if AUTO_CONFIG:
	# At the moment it just assumes certain things about different OSes.
	if platform.system() == 'Darwin':
		# Assuming macports installations or other installations in /opt/local/
		LIBEVENT_LOCATION = "/opt/local/"
		OPENSSL_LOCATION = "/opt/local/"
		LIBEV_LOCATION = "/opt/local/"
	else:
		# Tested for Linux Mint 13 only with the libraries installed from source with no modifications and standard configuration.
		LIBEVENT_LOCATION = "/usr/local/"
		OPENSSL_LOCATION = "/usr/local/ssl/"
		LIBEV_LOCATION = "/usr/local/"
		ADDITIONAL_OPENSSL_LFLAGS = "-ldl -L/lib/x86_64-linux-gnu/"
currentDir = os.path.dirname(sys.argv[0])
binPath = os.path.join(currentDir,"build","bin")
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
universal = False
debug = False
for	arg in sys.argv:
	if arg == "--all":
		clean = True
	if arg == "--test":
		test = True
	if arg == "--universal":
		universal = True
	if arg == "--debug":
		debug = True;
cflags = "-Wall -Wno-overflow -Wno-uninitialized -pedantic -std=c99 -I" + incPath + " " + ADDITIONAL_CFLAGS
libflags = ""
if platform.system() == 'Linux':
     libflags += " -fpic" # Required for compiling shared libraries in Linux
if universal and platform.system() == 'Darwin':
	cflags += " -arch i386 -arch x86_64"
else:
	cflags += " -m64"
if debug:
	cflags += " -g"
else:
	cflags += " -O" + OPTIMISATION_LEVEL
lflags = ADDITIONAL_LFLAGS
if platform.system() == 'Darwin':
    lflags += " -flat_namespace -dynamiclib -undefined dynamic_lookup"
    targetPath = os.path.join(binPath,"libcbitcoin.dylib")
else:
    lflags += " -shared"
    targetPath = os.path.join(binPath,"libcbitcoin.so")
# Copy Headers
for root, dirnames, filenames in os.walk(srcPath):
	for filename in fnmatch.filter(filenames, '*.h'):
		header = os.path.join(root,filename)
		headerLoc = os.path.join(incPath,filename)
		shutil.copyfile(header,headerLoc)
shutil.copyfile(os.path.join(currentDir,"dependencies","sockets","CBLibEventSockets.h"),os.path.join(incPath,"CBLibEventSockets.h"))
#shutil.copyfile(os.path.join(currentDir,"dependencies","sockets","CBLibevSockets.h"),os.path.join(incPath,"CBLibevSockets.h"))
# Compile object files
objects = ""
for root, dirnames, filenames in os.walk(srcPath):
	for filename in fnmatch.filter(filenames, '*.c'):
		source = os.path.join(root,filename)
		objectFile = filename[:-1] + "o"
		object = os.path.join(objPath,objectFile)
		objects += " " + object
		compile(cflags + libflags,object,source,clean)
# Link object files into dynamic library
print "LINKING " + targetPath
subprocess.check_call("gcc " + lflags + " -o " + targetPath + objects, shell=True)
# OpenSSL information
opensslFlags = " -lssl -lcrypto -L" + OPENSSL_LOCATION + "lib/ " + ADDITIONAL_OPENSSL_LFLAGS
opensslCFlags = " -I" + OPENSSL_LOCATION + "include/"
opensslReq = ["testCBAddress", "testCBAddressManager", "testCBAlert", "testCBTransaction", "testCBBase58", "testCBBlock", "testCBNetworkCommunicator", "testCBNetworkCommunicatorLibEv","testCBScript","testValidation"]
opensslOutput = os.path.join(objPath,"CBOpenSSLCrypto.o")
opensslSource = os.path.join(currentDir,"dependencies", "crypto", "CBOpenSSLCrypto.c")
# Build and run tests if specified
if test:
	# Script tests
	shutil.copyfile(os.path.join(testPath,"scriptCases.txt"),os.path.join(binPath,"scriptCases.txt"))
	os.chdir(binPath)
	# OpenSSL dependency
	compile(cflags + opensslCFlags,opensslOutput,opensslSource,clean)
	# PRNG dependency
	randReq = ["testCBAddressManager", "testCBNetworkCommunicator", "testCBNetworkCommunicatorLibEv"]
	source = os.path.join(currentDir,"dependencies","random","CBRand.c")
	randOutput = os.path.join(objPath,"CBRand.o")
	compile(cflags,randOutput,source,clean)
	# LibEvent
	libeventFlags = " -L" + LIBEVENT_LOCATION + "lib/ -levent_core -levent_pthreads"
	libeventCFlags = " -I" + LIBEVENT_LOCATION + "include/"
	if platform.system() == 'Linux':
	    libeventCFlags += " -D_POSIX_SOURCE"
	libeventReq = ["testCBNetworkCommunicator"]
	source = os.path.join(currentDir,"dependencies","sockets","CBLibEventSockets.c")
	libeventOutput = os.path.join(objPath,"CBLibEventSockets.o")
	compile(cflags + libeventCFlags,libeventOutput,source,clean)
	# Libev - Not working, hence commented out. Use libevent for now
	'''libevFlags = " -L" + LIBEV_LOCATION + "lib/ -lev"
	libevCFlags = " -I" + LIBEV_LOCATION + "include/"
	if platform.system() == 'Linux':
	    libevCFlags += " -D_POSIX_SOURCE"
	libevReq = ["testCBNetworkCommunicatorLibEv"]
	source = os.path.join(currentDir,"dependencies","sockets","CBLibevSockets.c")
	libevOutput = os.path.join(objPath,"CBLibevSockets.o")
	compile(cflags + libevCFlags,libevOutput,source,clean)'''
	for root, dirnames, filenames in os.walk(testPath):
		for filename in fnmatch.filter(filenames, '*.c'):
			source = os.path.join(root,filename)
			output = os.path.join(objPath,filename[:-1] + "o")
			targetPath = os.path.join(binPath,filename[:-2])
			objFiles = output
			linkerFlags = " -L" + binPath + " -lcbitcoin"
			if platform.system() == 'Linux':
				linkerFlags += " -Wl,-rpath," + binPath
				if filename[:-2] in opensslReq:
					linkerFlags += ":" + OPENSSL_LOCATION + "lib/"
				if filename[:-2] in libeventReq:
					linkerFlags += ":" + LIBEVENT_LOCATION + "lib/"
			additionalCFlags = ""
			if filename[:-2] in opensslReq:
				linkerFlags += opensslFlags
				objFiles += " " + opensslOutput
				additionalCFlags += opensslCFlags
			if filename[:-2] in randReq:
				objFiles += " " + randOutput
			if filename[:-2] in libeventReq:
				linkerFlags += libeventFlags
				objFiles += " " + libeventOutput
				additionalCFlags += libeventCFlags
			'''if filename[:-2] in libevReq:
				linkerFlags += libevFlags
				objFiles += " " + libevOutput
				additionalCFlags += libevCFlags'''
			compile(cflags + additionalCFlags,output,source,clean)
			print "LINKING " + targetPath
			subprocess.check_call("gcc " + objFiles + linkerFlags + " -o " + targetPath, shell=True)
			print "TESTING " + targetPath
			subprocess.check_call(targetPath,shell=True)
# Build example if specified
if sys.argv[-1] == "addressGenerator":
	compile(cflags + opensslCFlags,opensslOutput,opensslSource,clean)
	output = os.path.join(objPath,"addressGenerator.o")
	compile(cflags + opensslCFlags,output,os.path.join(examplesPath,"addressGenerator.c"),clean)
	targetPath = os.path.join(binPath,"addressGenerator")
	print "LINKING " + targetPath
	linkerFlags = " -L" + binPath + " -lcbitcoin"
	if platform.system() == 'Linux':
		linkerFlags += " -Wl,-rpath," + binPath
		linkerFlags += ":" + OPENSSL_LOCATION + "lib/"
	linkerFlags += opensslFlags
	subprocess.check_call("gcc " + output + " " + opensslOutput + linkerFlags + " -o " + targetPath, shell=True)
