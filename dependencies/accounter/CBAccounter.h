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

#define CBDecKey(name, type) uint8_t name[cbKeySizes[type] + 2]; name[0] = cbKeySizes[type] + 1; name[1] = type;

typedef enum{
	CB_TYPE_ACCOUNTER_DETAILS,
	CB_TYPE_TX_DETAILS,
	CB_TYPE_BRANCH_ACCOUNT_DETAILS,
	CB_TYPE_OUTPUT_DETAILS,
	CB_TYPE_BRANCH_OUTPUT_DETAILS,
	CB_TYPE_ACCOUNT_TX_DETAILS,
	CB_TYPE_BRANCH_ACCOUNT_TIME_TX,
	CB_TYPE_BRANCH_TX_DETAILS,
	CB_TYPE_OUTPUT_ACCOUNTS,
	CB_TYPE_ACCOUNT_UNSPENT_OUTPUTS,
	CB_TYPE_TX_ACCOUNTS,
	CB_TYPE_TX_HASH_TO_ID,
	CB_TYPE_TX_HEIGHT_BRANCH_AND_ID,
	CB_TYPE_OUTPUT_HASH_AND_INDEX_TO_ID,
	CB_TYPE_WATCHED_HASHES,
} CBKeyTypes;

uint8_t cbKeySizes[] = {
	0, /**< CB_TYPE_ACCOUNTER_DETAILS : Null - Details for the accounter. */
	8, /**< CB_TYPE_TX_DETAILS : The 8 byte transaction ID - Details for a transaction independant of the account. */
	9, /**< CB_TYPE_BRANCH_ACCOUNT_DETAILS : 1 byte branch number and 8 byte account ID (CB_NO_BRANCH for unconfirmed) - Stores the number of transaction in the account and the balance */
	8, /**< CB_TYPE_OUTPUT_DETAILS : 8 byte output ID - Stores the value. */
	9, /**< CB_TYPE_BRANCH_OUTPUT_DETAILS : 1 byte branch number and 8 byte output ID - Stores the spent status */
	16, /**< CB_TYPE_ACCOUNT_TX_DETAILS : 8 byte account ID and 8 byte transaction ID - Transaction details for this account */
	16, /**< CB_TYPE_BRANCH_ACCOUNT_TIME_TX : 1 byte branch number, 8 byte account ID, 8 byte transaction timestamp and 8 byte transaction ID - Null */
	9, /**< CB_TYPE_BRANCH_TX_DETAILS : 1 byte branch number and 8 byte transaction ID - Details for a transaction on a branch, no information for unconfirmed transactions. (Not used for CB_NO_BRANCH) */
	16, /**< CB_TYPE_OUTPUT_ACCOUNTS : 8 byte output ID and 8 byte account ID - Null */
	17, /**< CB_TYPE_ACCOUNT_UNSPENT_OUTPUTS : 1 byte branch number, 8 byte account ID, 8 byte output ID - Null */
	16, /**< CB_TYPE_TX_ACCOUNTS : 8 byte transaction ID and 8 byte account ID - Null */
	32, /**< CB_TYPE_TX_HASH_TO_ID : 32 byte transaction hash - The transaction ID */
	13, /**< CB_TYPE_TX_HEIGHT_BRANCH_AND_ID : 1 byte branch number, 4 byte block height and 8 byte transaction ID - Null (Not used for CB_NO_BRANCH) */
	36, /**< CB_TYPE_OUTPUT_HASH_AND_INDEX_TO_ID : 32 byte transaction hash and 4 byte output transaction index - The output reference ID */
	28, /**< CB_TYPE_WATCHED_HASHES : 20 byte hash and 8 byte account ID - Null */
};


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
} CBAccountBranchDetailsOffsets;

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
	CB_ACCOUNT_TX_DETAILS_VALUE = 0, /**< The change in balance for this transaction */
	CB_ACCOUNT_TX_DETAILS_ADDR = 8, /**< The 20 byte hash for an address that has been detected to be sent to or received to, or all-zero in the case of an odd transaction. */
} CBTransactionAccountDetailsOffsets;

/**
 @brief The offsets of transactions with branch specific details data
 */
typedef enum{
	CB_TX_BRANCH_DETAILS_BLOCK_HEIGHT = 0, /**< The block height this transaction was found in or CB_TX_UNCONFIRMED for CB_NO_BRANCH */
} CBTransactionBranchDetailsOffsets;

/**
 @brief The key offsets for CB_TYPE_OUTPUT_HASH_AND_INDEX_TO_ID
 */
typedef enum{
	CB_OUTPUT_HASH_AND_INDEX_TO_ID_HASH = 2,
	CB_OUTPUT_HASH_AND_INDEX_TO_ID_INDEX = 34,
} CBOutputHashAndIndexToIDKeyOffsets;

/**
 @brief The key offsets for CB_TYPE_BRANCH_OUTPUT_DETAILS
 */
typedef enum{
	CB_BRANCH_OUTPUT_DETAILS_BRANCH = 2,
	CB_BRANCH_OUTPUT_DETAILS_OUPUT_ID = 3,
} CBBranchOutputDetailsKeyOffsets;

/**
 @brief The key offsets for CB_TYPE_ACCOUNT_UNSPENT_OUTPUTS
 */
typedef enum{
	CB_ACCOUNT_UNSPENT_OUTPUTS_BRANCH = 2,
	CB_ACCOUNT_UNSPENT_OUTPUTS_ACCOUNT_ID = 3,
	CB_ACCOUNT_UNSPENT_OUTPUTS_OUTPUT_ID = 11,
} CBAccountUnspentOutputsKeyOffsets;

/**
 @brief The key offsets for CB_TYPE_OUTPUT_ACCOUNTS
 */
typedef enum{
	CB_OUTPUT_ACCOUNTS_OUTPUT_ID = 2,
	CB_OUTPUT_ACCOUNTS_ACCOUNTS_ID = 10,
} CBOutputAccountsKeyOffsets;

/**
 @brief The key offsets for CB_TYPE_OUTPUT_DETAILS
 */
typedef enum{
	CB_OUTPUT_DETAILS_OUTPUT_ID = 2,
} CBOutputDetailsKeyOffsets;

/**
 @brief The key offsets for CB_TYPE_WATCHED_HASHES
 */
typedef enum{
	CB_WATCHED_HASHES_HASH = 2,
	CB_WATCHED_HASHES_ACCOUNT_ID = 22,
} CBWatchedHashesKeyOffsets;

/**
 @brief The key offsets for CB_TYPE_TX_HASH_TO_ID
 */
typedef enum{
	CB_TX_HASH_TO_ID_HASH = 2,
} CBTxHashToIDKeyOffsets;

/**
 @brief The key offsets for CB_TYPE_TX_DETAILS
 */
typedef enum{
	CB_TX_DETAILS_TX_ID = 2,
} CBTxDetailsKeyOffsets;

/**
 @brief The key offsets for CB_TYPE_BRANCH_TX_DETAILS
 */
typedef enum{
	CB_BRANCH_TX_DETAILS_BRANCH = 2,
	CB_BRANCH_TX_DETAILS_TX_ID = 3,
} CBBranchTxDetailsKeyOffsets;

/**
 @brief The key offsets for CB_TYPE_TX_HEIGHT_BRANCH_AND_ID
 */
typedef enum{
	CB_TX_HEIGHT_BRANCH_AND_ID_BRANCH = 2,
	CB_TX_HEIGHT_BRANCH_AND_ID_HEIGHT = 3,
	CB_TX_HEIGHT_BRANCH_AND_ID_TX_ID = 7,
} CBTxHeightBranchAndIDKeyOffsets;

/**
 @brief The key offsets for CB_TYPE_TX_ACCOUNTS
 */
typedef enum{
	CB_TX_ACCOUNTS_TX_ID = 2,
	CB_TX_ACCOUNTS_ACCOUNT_ID = 10,
} CBTxAccountsKeyOffsets;

/**
 @brief The key offsets for CB_TYPE_ACCOUNT_TX_DETAILS
 */
typedef enum{
	CB_ACCOUNT_TX_DETAILS_ACCOUNT_ID = 2,
	CB_ACCOUNT_TX_DETAILS_TX_ID = 10,
} CBAccountTxDetailsKeyOffsets;

/**
 @brief The key offsets for CB_TYPE_BRANCH_ACCOUNT_DETAILS
 */
typedef enum{
	 CB_BRANCH_ACCOUNT_DETAILS_BRANCH = 2,
	CB_BRANCH_ACCOUNT_DETAILS_ACCOUNT_ID = 3,
} CBBranchAccountDetailsKeyOffsets;

/**
 @brief The key offsets for CB_TYPE_BRANCH_ACCOUNT_TIME_TX
 */
typedef enum{
	CB_BRANCH_ACCOUNT_TIME_TX_BRANCH = 2,
	CB_BRANCH_ACCOUNT_TIME_TX_ACCOUNT_ID = 3,
	CB_BRANCH_ACCOUNT_TIME_TX_TIMESTAMP = 11,
	CB_BRANCH_ACCOUNT_TIME_TX_TX_ID = 19,
} CBBranchAccountTimeTxKeyOffsets;
 
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
	uint64_t lastAccountID;
	uint64_t nextTxID;
	uint64_t nextOutputRefID;
} CBAccounter;

// Functions

/**
 @brief Adjusts the balance of an account by a transaction.
 @param database The database object.
 @param accountTxDetails The CB_TYPE_ACCOUNT_TX_DETAILS key.
 @param accountDetailsKey The CB_TYPE_BRANCH_ACCOUNT_DETAILS key.
 @returns true on success, false on failure.
 */
bool CBAccounterAdjustAccountBalanceByTx(CBDatabase * database, uint8_t * accountTxDetails, uint8_t * accountDetailsKey);
/**
 @brief Changes the spent status of an output reference on a branch.
 @param self The database object.
 @param prevOut The output hash and index.
 @param branch The branch of the output reference.
 @param spent The new spent status.
 @param txInfo A pointer to the transaction information, to be updated.
 @returns true on success, false on failure.
 */
bool CBAccounterChangeOutputReferenceSpentStatus(CBDatabase * self, CBPrevOut * prevOut, uint8_t branch, bool spent, CBAssociativeArray * txInfo);
/**
 @brief Compares 32-bit integers.
 @param int1 A pointer to the first 32-bit integer.
 @param int2 A pointer to the second 32-bit integer.
 @returns CB_COMPARE_MORE_THAN if the first integer is bigger than the second and likewise.
 */
CBCompare CBCompareUInt32(void * int1, void * int2);
/**
 @brief Removes a transaction's account details from a branch.
 @param database The database object.
 @param txDetailsKey The CB_TYPE_TX_DETAILS key.
 @param txHashToIDKey The CB_TYPE_TX_HASH_TO_ID key.
 @param branch The branch to remove the transaction details from.
 @returns true on success, false on failure.
 */
bool CBAccounterRemoveTransactionFromBranch(CBDatabase * database, uint8_t * txDetailsKey, uint8_t * txHashToIDKey, uint8_t branch);

#endif
