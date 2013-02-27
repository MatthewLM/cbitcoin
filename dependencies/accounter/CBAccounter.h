//
//  CBAccounter.h
//  cbitcoin
//
//  Created by Matthew Mitchell on 08/02/2012.
//  Copyright (c) 2012 Matthew Mitchell
//
//  This file is part of cbitcoin. It is subject to the license terms
//  in the LICENSE file found in the top-level directory of this
//  distribution and at http://www.cbitcoin.com/license.html. No part of
//  cbitcoin, including this file, may be copied, modified, propagated,
//  or distributed except according to the terms contained in the
//  LICENSE file.

/**
 @file
 @brief Implements the account storage dependency with use of the CBDatabase.
 */

#ifndef CBACCOUNTERH
#define CBACCOUNTERH

#include "CBDatabase.h"
#include "CBTransaction.h"

// Constants

#define CBDecKey(name, type) uint8_t name[type + 1]; name[0] = type;

typedef enum{
	CB_TYPE_ACCOUNTER_DETAILS = 0, /**< Null - Details for the accounter. */
	CB_TYPE_TX_DETAILS = 8, /**< The 8 byte transaction ID - Details for a transaction independant of the account. */
	CB_TYPE_ACCOUNT_BRANCH_DETAILS = 9, /**< 8 byte account ID and 1 byte branch number (CB_NO_BRANCH for unconfirmed) - Stores the number of transaction in the account and the balance */
	CB_TYPE_OUTPUT_DETAILS = 8, /**< 8 byte output ID - Stores the value. */
	CB_TYPE_OUTPUT_BRANCH_DETAILS = 9, /**< 8 byte output ID and 1 byte branch number - Stores the spent status */
	CB_TYPE_ACCOUNT_TX_DETAILS = 16, /**< 8 byte account ID and 8 byte transaction ID - Transaction details for this account */
	CB_TYPE_BRANCH_TX_DETAILS = 9, /**< 1 byte branch number and 8 byte transaction ID - Details for a transaction on a branch, no information for unconfirmed transactions. (Not used for CB_NO_BRANCH) */
	CB_TYPE_OUTPUT_ACCOUNTS = 16, /**< 8 byte output ID and 8 byte account ID - Null */
	CB_TYPE_ACCOUNT_UNSPENT_OUTPUTS = 17, /**< 8 byte account ID, 1 byte branch number, 8 byte output ID - Null */
	CB_TYPE_TX_ACCOUNTS = 16, /**< 8 byte transaction ID and 8 byte account ID - Null */
	CB_TYPE_TX_HASH_TO_ID = 32, /**< 32 byte transaction hash - The transaction ID */
	CB_TYPE_TX_HEIGHT_BRANCH_AND_ID = 13, /**< 1 byte branch number, 4 byte block height and 8 byte transaction ID - Null (Not used for CB_NO_BRANCH) */
	CB_TYPE_OUTPUT_HASH_AND_INDEX_TO_ID = 36, /**< 32 byte transaction hash and 4 byte output transaction index - The output reference ID */
	CB_TYPE_WATCHED_HASHES = 28, /**< 20 byte hash and 8 byte account ID */
} CBKeyDataType;


/**
 @brief The offsets of accounter details data
 */
typedef enum{
	CB_ACCOUNTER_DETAILS_OUTPUT_ID = 0, /**< The output ID to next use */
	CB_ACCOUNTER_DETAILS_TX_ID = 8, /**< The transaction ID to next use */
	CB_ACCOUNTER_DETAILS_ACCOUNT_ID = 16, /**< The account ID to next use */
} CBAccounterDetailsOffsets;

/**
 @brief The offsets of transactions details data
 */
typedef enum{
	CB_TX_DETAILS_TIMESTAMP = 0, /**< The time this transaction was discovered if found unconfirmed or the timestamp of the block. */
	CB_TX_DETAILS_BRANCH_INSTANCES = 4, /**< The number of branches owning this transaction */
	CB_TX_DETAILS_HASH = 5, /**< The transaction hash. */
} CBTransactionDetailsOffsets;

/**
 @brief The offsets of account details data for a branch.
 */
typedef enum{
	CB_ACCOUNT_DETAILS_BALANCE = 0, /**< The balance for the account on this branch. */
} CBAccountDetailsOffsets;

/**
 @brief The offsets of output reference data.
 */
typedef enum{
	CB_OUTPUT_REF_DATA_VALUE = 0, /**< The value of this output */
} CBOutputRefDataOffsets;

/**
 @brief The offsets of output reference branch data.
 */
typedef enum{
	CB_OUTPUT_REF_BRANCH_DATA_SPENT = 0,
} CBOutputRefBranchDataOffsets;


/**
 @brief The offsets of transactions account specific details data
 */
typedef enum{
	CB_TX_ACCOUNT_DETAILS_VALUE = 0, /**< The change in balance for this transaction */
	CB_TX_ACCOUNT_DETAILS_ADDR = 8, /**< The 20 byte hash for an address that has been detected to be sent to or received to, or all-zero in the case of an odd transaction. */
} CBTransactionAccountDetailsOffsets;

/**
 @brief The offsets of transactions with branch specific details data
 */
typedef enum{
	CB_TX_BRANCH_DETAILS_BLOCK_HEIGHT = 0, /**< The block height this transaction was found in or CB_TX_UNCONFIRMED for CB_NO_BRANCH */
} CBTransactionBranchDetailsOffsets;

// Structures

/**<
 @brief Counts the credit and debit for an account when processing a transaction.
 */
typedef struct{
	uint64_t accountID;
	uint64_t creditAmount;
	uint64_t debitAmount;
	bool foundCreditAddr;
	bool creditAddrIndexIsZero;
	uint8_t creditAddr[20];
} CBTransactionAccountCreditDebit;

typedef struct{
	CBDatabase base;
	uint64_t nextAccountID;
	uint64_t nextTxID;
	uint64_t nextOutputRefID;
} CBAccounter;

// Functions

/**
 @brief Compares 32-bit integers.
 @param int1 A pointer to the first 32-bit integer.
 @param int2 A pointer to the second 32-bit integer.
 @returns CB_COMPARE_MORE_THAN if the first integer is bigger than the second and likewise.
 */
CBCompare CBCompareUInt32(void * int1, void * int2);

#endif
