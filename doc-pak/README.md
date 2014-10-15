cbitcoin 2.0 - A Bitcoin Library In The C Programming Language
==========================================================

cbitcoin is a bitcoin library written in standard C99 and is licensed under the GPLv3 with additional permissions. It is currently in development stage and should only be used experimentally. The purpose of the library is to make it easier for developers to create bitcoin applications and the library should be versatile enough to be used in many bitcoin projects.

The features include:

* Bitcoin message structures including serialisation and deserialisation functions.
* A bitcoin script interpreter.
* Functions used for validation of POW, merkle trees, transactions etc.
* Automated bitcoin handshakes, peer discovery and pings.
* A network address manager.
* Network seeding, including DNS.
* Both full and headers only validation.
* Transaction accounting.
* A fully validating node.
* Base-58 encoding/decoding.
* Bitcoin address/WIF functions.
* Hierarchical Deterministic keys (BIP0032)
* Doxygen documentation and well-documented source code.
* Dependencies can easily be swapped and changed by implementing weakly linked functions.
* Implementations included that require libevent/libev, OpenSSL and POSIX.
* Very fast compilation. Much faster than the Satoshi client.
* Client implementing bitcoin RPC protocol (Incomplete)
* SPV validation (Planned)
* SPV node (Planned)
* Altcoin support (Planned)

If you wish to contact the project leader, Matthew Mitchell, about this project please email cbitcoin@thelibertyportal.com

Installation
------------

To build, type into your terminal:

    ./configure
    make

If you wish to test the library then type:

    make test

You can install individual tests such as `make bin/testCBAccounter`.

The configure script has the following additonal options:

 * `--disable-ec` cbitcoin produces two libraries, one for ordinary file IO and another for file IO with error correction. Both are built, but using this option means that the tests (except for testCBFile and testCBHamming72) and the cbitcoin client are linked without EC.
 * `--enable-debug` for debug builds, using `-g` instead of `-O3`.
 * `--disable-werror` disables the -Werror flag.
 * `--disable-stack-protector` disables stack protection in the form of `-fstack-protector-strong` or `-fstack-protector-all` if strong is not available.

You can also change CFLAGS, LFLAGS and CC before running ./configure to make changes to how cbitcoin is built.

The library will be built into a ./bin directory.

If you are able to get it to work for other systems then please submit the changes. If you think you have found a debug, then you may wish to submit an issue on the gitbub repository page (https://github.com/MatthewLM/cbitcoin/). Otherwise you may wish to try to fix the problem yourself, in which case please submit fixes. You should check to see that the issue is not being worked upon already.

Donations
---------

If you wish to support this project you can donate via the following methods:

Bitcoin: 1D5A1q5d192j5gYuWiP3CSE5fcaaZxe6E9  
Charitycoin: CSU54ZAa4VuhiVwzgyAudePmn7eJigkKU5  
Paypal: [Click here to donate](https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=YPB6N4BBZQD3N)

Contributors
------------

The following list is for all people that have contributed work that has been accepted into cbitcoin. Please consider making your own contribution to be added to the list.

Matthew Mitchell <matthewmitchell@thelibertyportal.com> - 1D5A1q5d192j5gYuWiP3CSE5fcaaZxe6E9  
Christian von Roques  
Andrew Miller <amiller@dappervision.com>
linuxdoctor - 1KB3RsW8H7TFV9awpt6MiXsjDqLD15oax3

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
	//
	//  Additional Permissions under GNU GPL version 3 section 7
	//
	//  Notwithstanding the terms of the license, when you distribute
	//  a covered work in non-source form, you are not required to provide
	//  source code corresponding to the covered work.
	//
	//  If you modify this Program, or any covered work, by linking or
	//  combining it with OpenSSL (or a modified version of that library),
	//  containing parts covered by the terms of the OpenSSL License, the
	//  licensors of this Program grant you additional permission to convey
	//  the resulting work.

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
