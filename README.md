cbitcoin - A Bitcoin Library For The C Programming Language
===========================================================

cbitcoin is a bitcoin library in development that will use nothing but the standard C library. The aim for this project is to create a simple, portable and powerful library for bitcoin in C. The library should be easy to use but provide many configurable features. The library will be made purely for C99 with nothing but the standard library. This should make the library small, efficient and portable.

The library is being developed from the bitcoinj library. Once functionalities have been implemented from bitcoinj, remaining functionalities will be developed from the C++ bitcoin client. Network, threading and any requirements not available in the standard C library will be attached to cbitcoin using function pointers.

THE LIBRARY IS STILL IN DEVELOPMENT AND MOSTLY INCOMPLETE.

Making a Contribution
---------------------

1. Fork the project on github: https://github.com/MatthewLM/cbitcoin
2. Implement the changes.
3. Document the changes (See "Documenting" below)
4. Ensure the "Last modified" parts to each changed file are updated.
5. Send a pull request.
6. Send an email to cbitcoin@thelibertyportal.com notifying that a request has been made.
7. The changes will be pulled once approved.

Coding Guidelines
-----------------

* cbitcoin uses an object-orientated approach by implememting virtual function tables, reference counting and inheritance on structures. New code should be consistent with this approach. Use the supplied structure_maker.py to make a new structure which inherits CBObject. All structures inheriting CBObject should go into the structures directory as shown.
* The rule for memory management is to retain an object before returning it, to retain an object when giving it to another object, and to release an object once the object is no longer needed. When a new object is created it should be retained. Unless required for thread safety, objects don't need to be retained when passed into functions.
* Remember to send object pointers by reference when releasing.
* Filenames should begin with CB.
* Functions, types and variables with linker visibility outside the library should begin with CB.
* CamelCase should be used. lowerCamelCase should be used for structure fields and variables unless inappropriate.
* Constants should be ALL_UPPERCASE_WITH_UNDERSCORES.
* Constants made visible throughout the library should go in CBConstants.h
* Functions to be included in the virtual tables of structures, should be declared and implemented in the structure files. These must take the structure as the first argument.
* Destructors may be removed for structures that can use the inherited destructor. structure_maker.py includes a destructor by default for easy coding.
* CBGet functions should be prefered over type-casting. Type-casts should be used for virtual table casting when only the virtual table is available, else CBGet...VT can be used on the object.
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
	//  Last modified by Full Name on DD/MM/YYYY.
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

Header files should contain information for documentation. cbitcoin uses a DoxyGen syntax (See http://www.stack.nl/~dimitri/doxygen/manual.html). Please document all files as well as structures and functions that are exposed by the library. Brief descriptions should be included. Details can be added at a later date, especailly once code has been properly implemented. Files should be documented like this:

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
	int someFunc(int someParam);
 
It may be that other elements are documented and should be documented in a similar fashion.

Thank You!
==========