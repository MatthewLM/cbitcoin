cbitcoin - A Bitcoin Library In The C Programming Language
==========================================================

cbitcoin is a bitcoin library written in standard C99. It is currently in alpha-stage and should only be used experimentally. The purpose of the library is to make it easier for developers to create bitcoin applications and the library should be versatile enough to be used in many bitcoin projects. It is not a client library, rather it provides the basic building blocks of the bitcoin protocol.

The features includes:

* Bitcoin message structures including serailisation and deserialisation functions.
* A bitcoin script interpreter.
* Functions for building and verifying Merkle trees.
* Basic functions which can be used in various validation models (full nodes, pruning nodes, SPV nodes or whatever).
* Asynchronous networking code which provides a simple interface to the bitcoin network for sending and receiving messages.
* A network address manager
* Automatic bitcoin handshakes.
* Automatic peer discovery.
* Automatic pings.
* Base-58 bitcoin address encoding.
* Simple reference counting memory management.
* Doxygen documentation and well-documented source code.
* Purely standard C99 with weakly linked function prototypes for cryptography, PRNG and network dependencies.
* Implementations of the dependencies using libevent and OpenSSL.

If you wish to contact the project leader, Matthew Mitchell, about this project please email cbitcoin@thelibertyportal.com

**To help support cbitcoin financially, please send donations to: 1D5A1q5d192j5gYuWiP3CSE5fcaaZxe6E9 Thank you**

Contributors
------------

The following list is for all people that have contributed work that has been accepted into cbitcoin. Please consider making your own contribution to be added to the list.

Matthew Mitchell - 1D5A1q5d192j5gYuWiP3CSE5fcaaZxe6E9

Installation
------------

cbitcoin comes with an installation script which has been tested on OSX Mountain Lion and Linux Mint 13. To use you should have python installed. Run the BUILD.py file with a python 2 (tested 2.5 to 2.7) interpreter. The library will be built into a ./build/bin directory. The BUILD.py script takes the following parameters:

**--all** Compile all the source files, even for source files which are older than any existing object files.  
**--test** Build and run the test programs.
**--universal** Attempt to build a universal library when building for OSX.
**--debug** Compile with the "-g" flag and do not use optimizations.  

The top of the BUILD.py file contains a configuration section. This section allows you to add your own linker and compilation flags and also provide the locations of libraries for building the tests if you have passed "--test". By default the script will use settings depending on whether you are on OSX or Linux but you can modify the configuration for your own platform. Ensure AUTO_CONFIG is False if you are doing this.

The script has only been tested on OSX Mountain Lion 10.8.1 and Linux Mint 13 Maya. Contributions for successful builds on other systems are welcome.

Making a Contribution
---------------------

1. Fork the project on github: https://github.com/MatthewLM/cbitcoin
2. Implement the changes.
3. Document the changes (See "Documenting" below)
4. Make a pull request.
5. Send an email to cbitcoin@thelibertyportal.com notifying that a request has been made.
6. The changes will be pulled once approved.

Coding Guidelines
-----------------

* cbitcoin uses an object-orientated approach by implementing reference counting and inheritance on structures. New code should be consistent with this approach. Use the supplied structure_maker.py to make a new structure which inherits CBObject. All structures inheriting CBObject should go into the structures directory as shown. message_maker.py is similar to structure_maker but is for CBMessage structures.
* The rule for memory management is to retain an object before returning it, to retain an object when giving it to another object, and to release an object once the object is no longer needed by an object or function. When a new object is created it should be retained. There are some exceptions to the rules, such as functions which take a reference from the caller.
* Filenames should begin with CB.
* Functions, types and variables with linker visibility outside the library should begin with CB.
* CamelCase should be used. lowerCamelCase should be used for structure fields and variables except where inappropriate.
* Constants should be ALL_UPPERCASE_WITH_UNDERSCORES.
* Constants made visible throughout the library should go in CBConstants.h
* CBGet functions should be preferred over type-casting.
* Use comments where appropriate, especially in obscure sections.
* In structure files, please order functions under "//  Functions" alphabetically.
* Use "???" in comments for areas of confusion or uncertainty where the code may need improving or changed. Look for "???" to find parts in the code that may need work.

Documenting
-----------

cbitcoin should contain the following header for each file:

	//
	//  CBFileName
	//  cbitcoin
	//
	//  Created by Full Name on DD/MM/YYYY.
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

Header files should contain information for documentation. cbitcoin uses a Doxygen syntax (See http://www.stack.nl/~dimitri/doxygen/manual.html). Please document all files as well as structures and functions that are exposed by the library. Brief descriptions should be included. Details can be added at a later date, especially once code has been properly implemented. Files should be documented like this:

	/**
	 @file
	 @brief Brief description of file.
	 @details More in depth description of file.
	 */

Structures should be documented like this:

	/**
	 @brief Brief decription of structure.
	 */
	typedef struct CBSomeStruct{
		int someInt; /**< Description of data field. */
	} CBSomeStruct;

Functions should be documented like this:

	/**
	 @brief Brief decription of function.
	 @param someParam Brief decription of the function parameter.
	 @returns What the function returns.
	 */
	int CBSomeFunc(int someParam);
 
It may be that other elements are documented and should be documented in a similar fashion and please leave helpful comments in the source code.

Thank You!
==========
