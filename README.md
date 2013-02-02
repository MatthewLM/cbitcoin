cbitcoin 2.0 - A Bitcoin Library In The C Programming Language
==========================================================

cbitcoin is a bitcoin library written in standard C99. It is currently in development stage and should only be used experimentally. The purpose of the library is to make it easier for developers to create bitcoin applications and the library should be versatile enough to be used in many bitcoin projects. 

The features includes:

* Bitcoin message structures including serialisation and deserialisation functions.
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
* Purely standard C99 with weakly linked function prototypes for cryptography, PRNG, file-IO and network dependencies.
* Implementations of the dependencies using libevent, OpenSSL and POSIX.
* Full block-chain validation (Incomplete)
* Fully validating node (Planned)
* SPV validation (Planned)
* SPV node (Planned)

If you wish to contact the project leader, Matthew Mitchell, about this project please email cbitcoin@thelibertyportal.com

**To help support cbitcoin financially, please send donations to: 1D5A1q5d192j5gYuWiP3CSE5fcaaZxe6E9 Thank you**

Contributors
------------

The following list is for all people that have contributed work that has been accepted into cbitcoin. Please consider making your own contribution to be added to the list.

Matthew Mitchell - 1D5A1q5d192j5gYuWiP3CSE5fcaaZxe6E9  
Christian von Roques

Installation
------------

To build, type into your terminal:

    ./configure
    make

If you wish to test the library then type:

    make test

You can install a debug version of cbitcoin with:

    make debug-all

And you can build debug versions of the test executables with:

    make debug-test

The library will be built into a ./bin directory; there is no install target yet.

If you are able to get it to work for other systems then please submit the changes. If you think you have found a debug, then you may wish to submit an issue on the gitbub repository page (https://github.com/MatthewLM/cbitcoin/). Otherwise you may wish to try to fix the problem yourself, in which case please submit fixes. You should check to see that the issue is not being worked upon already.

Making a Contribution
---------------------

If you wish to contibute feedback please email Matthew Mitchell at cbitcoin@thelibertyportal.com.

If you wish to contribute code:

1. Fork the project on github: https://github.com/MatthewLM/cbitcoin
2. Decide how you'd wish to contribute. You can search for occurances of "???" in the sourcecode. Each time you see comment with "???" it will describe any issues or potential improvements that can be made to the code. Look at the issues on the github repository page and you may find something that you can do from there.
3. Submit what you intend to do on the github repository page as an issue.
4. Implement your changes.
5. Document the changes if possible (See "Documenting" below)
6. Make a pull request.
7. Send an email to cbitcoin@thelibertyportal.com notifying that a request has been made.
8. The changes will be pulled once approved.

Easy! Please email Matthew Mitchell (cbitcoin@thelibertyportal.com) if you have any queries.

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
	typedef struct {
		int someInt; /**< Description of data field. */
	} CBSomeStruct;

Functions should be documented like this:

	/**
	 @brief Brief decription of function.
	 @param someParam Brief decription of the function parameter.
	 @returns What the function returns.
	 */
	int CBSomeFunc(int someParam);
 
It may be that other elements are documented and should be documented in a similar fashion and please leave helpful comments in the source code. Type "???" if you wish to provide any details that should be brought forward to other contributors.

Thank You!
==========
