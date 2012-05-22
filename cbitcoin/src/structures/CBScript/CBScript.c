//
//  CBScript.c
//  cbitcoin
//
//  Created by Matthew Mitchell on 02/05/2012.
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

#include "CBScript.h"

//  Virtual Table Store

static void * VTStore = NULL;
static int objectNum = 0;

//  Constructor

CBScript * CBNewScript(CBNetworkParameters * params,CBByteArray * program,CBEvents * events){
	CBScript * self = malloc(sizeof(*self));
	objectNum++;
	CBAddVTToObject(CBGetObject(self), &VTStore, CBCreateScriptVT);
	CBInitScript(self,params,program,events);
	return self;
}

//  Virtual Table Creation

CBScriptVT * CBCreateScriptVT(){
	CBScriptVT * VT = malloc(sizeof(*VT));
	CBSetScriptVT(VT);
	return VT;
}
void CBSetScriptVT(CBScriptVT * VT){
	CBSetObjectVT((CBObjectVT *)VT);
	((CBObjectVT *)VT)->free = (void (*)(void *))CBFreeScript;
	VT->execute = (bool (*)(void *, CBScriptStack *))CBScriptExecute;
}

//  Virtual Table Getter

CBScriptVT * CBGetScriptVT(void * self){
	return ((CBScriptVT *)(CBGetObject(self))->VT);
}

//  Object Getter

CBScript * CBGetScript(void * self){
	return self;
}

//  Initialiser

bool CBInitScript(CBScript * self,CBNetworkParameters * params,CBByteArray * program,CBEvents * events){
	if (!CBInitObject(CBGetObject(self)))
		return false;
	self->params = params;
	self->program = program;
	self->events = events;
	// Retain objects now held
	CBGetObjectVT(self->params)->retain(self->params);
	CBGetObjectVT(self->program)->retain(self->program);
	self->cursor = 0;
	return true;
}

//  Destructor

void CBFreeScript(CBScript * self){
	CBFreeProcessScript(self);
	CBFree();
}
void CBFreeProcessScript(CBScript * self){
	CBGetObjectVT(self->params)->release(&self->params);
	CBGetObjectVT(self->program)->release(&self->program);
	CBFreeProcessObject(CBGetObject(self));
}

//  Functions

void CBFreeScriptStack(CBScriptStack stack){
	for (int x = 0; x < stack.length; x++) {
		free(stack.elements[x].data);
	}
	free(stack.elements);
}
CBScriptStack CBNewEmptyScriptStack(){
	CBScriptStack stack;
	stack.elements = NULL;
	stack.length = 0;
	return stack;
}
bool CBScriptStackEvalBool(CBScriptStack * stack){
	CBScriptStackItem item = stack->elements[stack->length-1];
	for (u_int16_t x = 0; x < item.length; x++)
        if (item.data[x] != 0)
            // Can be negative zero
            if (x == item.length - 1 && item.data[x] == 0x80)
                return false;
			else
				return true;
    return false;
}
bool CBScriptExecute(CBScript * self,CBScriptStack * stack){
	CBScriptStack altStack = CBNewEmptyScriptStack();
	u_int16_t skipIfElseBlock = 0xffff; // Skips all instructions on or over this if/else level.
	u_int16_t ifElseSize = 0; // Amount of if/else block levels
	if (CBGetByteArray(self->program)->length > 10000)
		return false; // Script is an illegal size.
	for (u_int8_t opCount = 0;;opCount++) {
		if (!(CBGetByteArray(self->program)->length - self->cursor))
			break; // Reached end of program
		if (opCount == 201) {
			return false; // Too many Op codes
		}
		u_int8_t byte = CBGetByteArrayVT(self->program)->getByte(self->program,self->cursor);
		self->cursor++;
		// Control management for skipping
		if (ifElseSize >= skipIfElseBlock) { // Skip when "ifElseSize" level is over or at "skipIfElseBlock"
			if (byte == CB_SCRIPT_OP_ELSE && ifElseSize == skipIfElseBlock) {
				skipIfElseBlock = 0xffff; // No more skipping
			}else if (byte == CB_SCRIPT_OP_ENDIF){
				if (ifElseSize == skipIfElseBlock) {
					skipIfElseBlock = 0xffff; // No more skipping
				}
				ifElseSize--;
			}else if (byte == CB_SCRIPT_OP_IF){
				ifElseSize++;
			}
		}else{ // Execution for no skipping
			if (!byte) {
				// Push 0 onto stack
				CBScriptStackItem item;
				item.data = malloc(1);
				item.length = 1;
				item.data[0] = 0;
				CBScriptStackPushItem(stack, item);
			}else if (byte < 76){
				// Check size
				if ((CBGetByteArray(self->program)->length - self->cursor) < byte)
					return false; // Not enough space.
				// Push data the size of the value of the byte
				CBScriptStackItem item;
				item.data = malloc(byte);
				item.length = byte;
				memmove(item.data, CBGetByteArrayVT(self->program)->getData(self->program) + self->cursor, byte);
				CBScriptStackPushItem(stack, item);
				self->cursor += byte;
			}else if (byte < 79){
				// Push data with the length of bytes represented by the next bytes
				u_int32_t amount;
				if (byte == CB_SCRIPT_OP_PUSHDATA1){
					amount = CBGetByteArrayVT(self->program)->getByte(self->program,self->cursor);
					self->cursor++;
				}else if (byte == CB_SCRIPT_OP_PUSHDATA2){
					amount = CBGetByteArrayVT(self->program)->readUInt16(self->program,self->cursor);
					self->cursor += 2;
				}else{
					amount = CBGetByteArrayVT(self->program)->readUInt32(self->program,self->cursor);
					self->cursor += 4;
				}
				// Check limitation
				if (amount > 520)
					return false; // Size of data to push is illegal.
				// Check size
				if ((CBGetByteArray(self->program)->length - self->cursor) < amount)
					return false; // Not enough space.
				CBScriptStackItem item;
				item.data = malloc(amount);
				item.length = amount;
				memmove(item.data, CBGetByteArrayVT(self->program)->getData(self->program) + self->cursor, amount);
				CBScriptStackPushItem(stack, item);
				self->cursor += amount;
			}else if (byte == CB_SCRIPT_OP_1NEGATE){
				// Push -1 onto the stack
				CBScriptStackItem item;
				item.data = malloc(1);
				item.length = 1;
				item.data[0] = 0x81; // 10000001 Not like normal signed integers, most significant bit applies sign, making the rest of the bits take away from zero.
				CBScriptStackPushItem(stack, item);
			}else if (byte < 97){
				// Push a number onto the stack
				CBScriptStackItem item;
				item.data = malloc(1);
				item.length = 1;
				item.data[0] = byte - CB_SCRIPT_OP_1 + 1;
				CBScriptStackPushItem(stack, item);
			}else if (byte == CB_SCRIPT_OP_NOP){
				// Nothing...
			}else if (byte == CB_SCRIPT_OP_IF || byte == CB_SCRIPT_OP_NOTIF){
				// If top of stack is true, continue, else goto OP_ELSE or OP_ENDIF.
				ifElseSize++;
				if (!stack->length)
					return false; // Stack empty
				bool res = CBScriptStackEvalBool(stack);
				if ((res && byte == CB_SCRIPT_OP_IF) || (!res && byte == CB_SCRIPT_OP_NOTIF))
					skipIfElseBlock = 0xffff;
				else
					skipIfElseBlock = ifElseSize; // Is skipping on this level until OP_ELSE or OP_ENDIF is reached on this level
				// Remove top stack item
				CBScriptStackRemoveItem(stack);
			}else if (byte == CB_SCRIPT_OP_ELSE){
				if (!ifElseSize)
					return false; // OP_ELSE on lowest level not possible
				skipIfElseBlock = ifElseSize; // Begin skipping
			}else if (byte == CB_SCRIPT_OP_ENDIF){
				if (!ifElseSize)
					return false; // OP_ENDIF on lowest level not possible
				ifElseSize--; // Lower level
			}else if (byte == CB_SCRIPT_OP_VERIFY){
				if (!stack->length)
					return false; // Stack empty
				if (CBScriptStackEvalBool(stack))
					// Remove top stack item
					CBScriptStackRemoveItem(stack);
				else
					return false; // Failed verification
			}else if (byte == CB_SCRIPT_OP_RETURN){
				return false; // Failed verification with OP_RETURN.
			}else if (byte == CB_SCRIPT_OP_TOALTSTACK){
				if (!stack->length)
					return false; // Stack empty
				CBScriptStackPushItem(&altStack, CBScriptStackPopItem(stack));
			}else if (byte == CB_SCRIPT_OP_FROMALTSTACK){
				if (!altStack.length)
					return false; // Alternative stack empty
				CBScriptStackPushItem(stack, CBScriptStackPopItem(&altStack));
			}else if (byte == CB_SCRIPT_OP_IFDUP){
				if (!stack->length)
					return false; // Stack empty
				if (CBScriptStackEvalBool(stack))
					//Duplicate top stack item
					CBScriptStackPushItem(stack, CBScriptStackCopyItem(stack,0));
			}else if (byte == CB_SCRIPT_OP_DEPTH){
				CBScriptStackItem item;
				item.data = malloc(2);
				item.length = 2;
				item.data[0] = stack->length >> 8;
				item.data[1] = stack->length;
				CBScriptStackPushItem(stack, item);
			}else if (byte == CB_SCRIPT_OP_DROP){
				if (!stack->length)
					return false; // Stack empty
				CBScriptStackRemoveItem(stack);
			}else if (byte == CB_SCRIPT_OP_DUP){
				if (!stack->length)
					return false; // Stack empty
				//Duplicate top stack item
				CBScriptStackPushItem(stack, CBScriptStackCopyItem(stack,0));
			}else if (byte == CB_SCRIPT_OP_NIP){
				if (stack->length < 2)
					return false; // Stack needs 2 or more elements.
				// Remove second from top item.
				stack->length--;
				free(stack->elements[stack->length-1].data);
				stack->elements[stack->length-1] = stack->elements[stack->length]; // Top item moves down
				stack->elements = realloc(stack->elements, sizeof(*stack->elements)*stack->length);
			}else if (byte == CB_SCRIPT_OP_OVER){
				if (stack->length < 2)
					return false; // Stack needs 2 or more elements.
				CBScriptStackPushItem(stack, CBScriptStackCopyItem(stack,1)); // Copies second from top and pushes it on the top.
			}else if (byte == CB_SCRIPT_OP_PICK || byte == CB_SCRIPT_OP_ROLL){
				if (stack->length < 2)
					return false; // Stack needs 2 or more elements.
				CBScriptStackItem item = CBScriptStackPopItem(stack);
				if (item.length > 4)
					return false; // Protocol does not except integers more than 32 bits.
				if (item.data[item.length-1] > 0x80) // Negative
					return false; // Must be positive
				u_int32_t i = item.data[0] & (item.data[1] << 8) & (item.data[2] << 16) & (item.data[3] << 24);
				if (i >= stack->length)
					return false; // Must be smaller than stack size
				if (byte == CB_SCRIPT_OP_PICK) {
					// Copy element
					CBScriptStackPushItem(stack, CBScriptStackCopyItem(stack,i));
				}else{ // CB_SCRIPT_OP_ROLL
					// Move element.
					CBScriptStackItem temp = stack->elements[stack->length-i-1];
					for (u_int32_t x = 0; x < i; x++) // Move other elements down
						stack->elements[stack->length-i+x-1] = stack->elements[stack->length-i+x];
					stack->elements[stack->length] = temp;
				}
			}else if (byte == CB_SCRIPT_OP_ROT){
				if (stack->length < 3)
					return false; // Stack needs 3 or more elements.
				// Rotate top three elements to the left.
				CBScriptStackItem temp = stack->elements[stack->length-3];
				stack->elements[stack->length-3] = stack->elements[stack->length-2];
				stack->elements[stack->length-2] = stack->elements[stack->length-1];
				stack->elements[stack->length-1] = temp;
			}else if (byte == CB_SCRIPT_OP_SWAP){
				if (stack->length < 2)
					return false; // Stack needs 2 or more elements.
				CBScriptStackItem temp = stack->elements[stack->length-2];
				stack->elements[stack->length-2] = stack->elements[stack->length-1];
				stack->elements[stack->length-1] = temp;
			}else if (byte == CB_SCRIPT_OP_TUCK){
				if (stack->length < 2)
					return false; // Stack needs 2 or more elements.
				CBScriptStackItem item = CBScriptStackCopyItem(stack, 0);
				// New copy three down.
				stack->length++;
				stack->elements = realloc(stack->elements, sizeof(*stack->elements)*stack->length);
				stack->elements[stack->length-1] = stack->elements[stack->length-2];
				stack->elements[stack->length-2] = stack->elements[stack->length-3];
				stack->elements[stack->length-3] = item;
			}else if (byte == CB_SCRIPT_OP_2DROP){
				if (stack->length < 2)
					return false; // Stack needs 2 or more elements.
				CBScriptStackRemoveItem(stack);
				CBScriptStackRemoveItem(stack);
			}else if (byte == CB_SCRIPT_OP_2DUP){
				if (stack->length < 2)
					return false; // Stack needs 2 or more elements.
				CBScriptStackPushItem(stack, CBScriptStackCopyItem(stack,1));
				CBScriptStackPushItem(stack, CBScriptStackCopyItem(stack,1));
			}else if (byte == CB_SCRIPT_OP_3DUP){
				if (stack->length < 3)
					return false; // Stack needs 3 or more elements.
				CBScriptStackPushItem(stack, CBScriptStackCopyItem(stack,2));
				CBScriptStackPushItem(stack, CBScriptStackCopyItem(stack,2));
				CBScriptStackPushItem(stack, CBScriptStackCopyItem(stack,2));
			}else if (byte == CB_SCRIPT_OP_2OVER){
				if (stack->length < 4)
					return false; // Stack needs 4 or more elements.
				CBScriptStackPushItem(stack, CBScriptStackCopyItem(stack,3));
				CBScriptStackPushItem(stack, CBScriptStackCopyItem(stack,3));
			}else if (byte == CB_SCRIPT_OP_2ROT){
				if (stack->length < 6)
					return false; // Stack needs 6 or more elements.
				// Rotate top three pairs of elements to the left.
				CBScriptStackItem temp = stack->elements[stack->length-6];
				CBScriptStackItem temp2 = stack->elements[stack->length-5];
				stack->elements[stack->length-6] = stack->elements[stack->length-4];
				stack->elements[stack->length-5] = stack->elements[stack->length-3];
				stack->elements[stack->length-4] = stack->elements[stack->length-2];
				stack->elements[stack->length-3] = stack->elements[stack->length-1];
				stack->elements[stack->length-2] = temp;
				stack->elements[stack->length-1] = temp2;
			}else if (byte == CB_SCRIPT_OP_2SWAP){
				if (stack->length < 4)
					return false; // Stack needs 4 or more elements.
				CBScriptStackItem temp = stack->elements[stack->length-4];
				CBScriptStackItem temp2 = stack->elements[stack->length-3];
				stack->elements[stack->length-4] = stack->elements[stack->length-2];
				stack->elements[stack->length-3] = stack->elements[stack->length-1];
				stack->elements[stack->length-2] = temp;
				stack->elements[stack->length-1] = temp2;
			}else if (byte == CB_SCRIPT_OP_SIZE){
				// ??? Does this match the protocol?
				if (!stack->length)
					return false; // Stack empty
				u_int16_t len = stack->elements[stack->length-1].length;
				CBScriptStackItem item;
				if (len > 0x80) { // More than 0x80 so use two bytes as on a single byte it would be negative.
					item.data = malloc(2);
					item.length = 2;
					item.data[0] = len >> 8;
					item.data[1] = len;
				}else{
					item.data = malloc(1);
					item.length = 1;
					item.data[0] = len;
				}
				CBScriptStackPushItem(stack, item);
			}else if (byte == CB_SCRIPT_OP_EQUAL || byte == CB_SCRIPT_OP_EQUALVERIFY){
				if (stack->length < 2)
					return false; // Stack needs 2 or more elements.
				CBScriptStackItem i1 = stack->elements[stack->length-1];
				CBScriptStackItem i2 = stack->elements[stack->length-2];
				bool ok = true;
				if (i1.length != i2.length)
					ok = false;
				else for (u_int16_t x = 0; x < i1.length; x++)
					if (i1.data[x] != i2.data[x]) {
						ok = false;
						break;
					}
				// Push result onto stack
				CBScriptStackItem item;
				item.data = malloc(1);
				item.length = 1;
				item.data[0] = ok;
				CBScriptStackPushItem(stack, item);
				if (byte == CB_SCRIPT_OP_EQUALVERIFY)
					if (ok)
						CBScriptStackRemoveItem(stack);
					else
						return false; // Failed verification
			}
		}
	}
	if (ifElseSize) {
		return false; // If/Else Block(s) not terminated.
	}
	if (!stack->length) {
		return false; // Stack empty.
	}
	if (CBScriptStackEvalBool(stack)) {
		return true;
	}else{
		return false;
	}
}
CBScriptStackItem CBScriptStackCopyItem(CBScriptStack * stack,u_int8_t fromTop){
	CBScriptStackItem oldItem = stack->elements[stack->length - fromTop - 1];
	CBScriptStackItem newItem;
	newItem.data = malloc(oldItem.length);
	newItem.length = oldItem.length;
	memmove(newItem.data, oldItem.data, newItem.length);
	return newItem;
}
CBScriptStackItem CBScriptStackPopItem(CBScriptStack * stack){
	stack->length--;
	CBScriptStackItem item = stack->elements[stack->length];
	stack->elements = realloc(stack->elements, sizeof(*stack->elements)*stack->length);
	return item;
}
void CBScriptStackPushItem(CBScriptStack * stack,CBScriptStackItem item){
	stack->length++;
	stack->elements = realloc(stack->elements, sizeof(*stack->elements)*stack->length);
	stack->elements[stack->length-1] = item;
}
void CBScriptStackRemoveItem(CBScriptStack * stack){
	stack->length--;
	free(stack->elements[stack->length].data);
	stack->elements = realloc(stack->elements, sizeof(*stack->elements)*stack->length);
}
