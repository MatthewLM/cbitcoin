//
//  CBAccounterStorage.h
//  cbitcoin
//
//  Created by Matthew Mitchell on 08/02/2013.
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

#ifndef CBACCOUNTERSTORAGEH
#define CBACCOUNTERSTORAGEH

#include "CBDatabase.h"
#include "CBTransaction.h"
#include "CBValidator.h"

// Constants and Macros

#define CB_UNUSED_SECTION 0
#define CBGetAccounterCursor(x) ((CBAccounterStorageCursor *)x)

/**
 @brief The offsets of accounter details data
 */
typedef enum{
	CB_ACCOUNTER_DETAILS_OUTPUT_ID = CB_BLOCK_CHAIN_EXTRA_SIZE, /**< The output ID to next use */
	CB_ACCOUNTER_DETAILS_TX_ID = CB_ACCOUNTER_DETAILS_OUTPUT_ID + 8, /**< The transaction ID to next use */
	CB_ACCOUNTER_DETAILS_ACCOUNT_ID = CB_ACCOUNTER_DETAILS_TX_ID + 8, /**< The account ID to next use */
	CB_ACCOUNTER_DETAILS_BRANCH_LAST_SECTION_IDS = CB_ACCOUNTER_DETAILS_ACCOUNT_ID + 8, /**< The IDs of the last sections for branches */
	CB_ACCOUNTER_DETAILS_BRANCH_SECTION_PARENTS = CB_ACCOUNTER_DETAILS_BRANCH_LAST_SECTION_IDS + CB_MAX_BRANCH_CACHE, /**< The parents for the last section IDs with offsets equaling the IDs */
	CB_ACCOUNTER_DETAILS_BRANCH_SECTION_HEIGHT_START = CB_ACCOUNTER_DETAILS_BRANCH_SECTION_PARENTS + CB_MAX_BRANCH_SECTIONS, /**< The parents for the last section IDs with offsets equaling the IDs */
	CB_ACCOUNTER_DETAILS_BRANCH_SECTION_NEXT_ORDER_NUM = CB_ACCOUNTER_DETAILS_BRANCH_SECTION_HEIGHT_START + CB_MAX_BRANCH_SECTIONS*4
} CBAccounterDetailsOffsets;

/**
 @brief The offsets of transactions details data
 */
typedef enum{
	CB_TX_DETAILS_BRANCH_INSTANCES = 0, /**< The number of branches owning this transaction or duplicates. */
	CB_TX_DETAILS_HASH = 1, /**< The transaction hash. */
	CB_TX_DETAILS_TIMESTAMP = 33 /**< The timestamp of the transaction. Duplicate transactions will have the same timestamp, but so what? */
} CBTransactionDetailsOffsets;

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
 @brief The data offsets for outputDetails
 */
typedef enum{
	CB_OUTPUT_DETAILS_VALUE = 0,
	CB_OUTPUT_DETAILS_TX_HASH = 8,
	CB_OUTPUT_DETAILS_INDEX = 40,
	CB_OUTPUT_DETAILS_ID_HASH = 44,
} CBOutputDetailsOffsets;

/**
 @brief The data offsets for branchSectionTxDetails
 */
typedef enum{
	CB_BRANCH_TX_DETAILS_HEIGHT = 0,
} CBBranchTxDetailsDataOffsets;

/**
 @brief The key offsets for outputHashAndIndexToID
 */
typedef enum{
	CB_OUTPUT_HASH_AND_INDEX_TO_ID_HASH = 0,
	CB_OUTPUT_HASH_AND_INDEX_TO_ID_INDEX = 32,
} CBOutputHashAndIndexToIDKeyOffsets;

/**
 @brief The key offsets for branchSectionAccountOutputs
 */
typedef enum{
	CB_ACCOUNT_OUTPUTS_BRANCH = 0,
	CB_ACCOUNT_OUTPUTS_ACCOUNT_ID = 1,
	CB_ACCOUNT_OUTPUTS_HEIGHT = 9,
	CB_ACCOUNT_OUTPUTS_OUTPUT_ID = 13,
} CBAccountUnspentOutputsKeyOffsets;

/**
 @brief The key offsets for outputAccounts
 */
typedef enum{
	CB_OUTPUT_ACCOUNTS_OUTPUT_ID = 0,
	CB_OUTPUT_ACCOUNTS_ACCOUNTS_ID = 8,
} CBOutputAccountsKeyOffsets;

/**
 @brief The key offsets for accountWatchedHashes
 */
typedef enum{
	CB_ACCOUNT_WATCHED_HASHES_ACCOUNT_ID = 0,
	CB_ACCOUNT_WATCHED_HASHES_HASH = 8,
} CBAccountWatchedHashesKeyOffsets;

/**
 @brief The key offsets for watchedHashes
 */
typedef enum{
	CB_WATCHED_HASHES_HASH = 0,
	CB_WATCHED_HASHES_ACCOUNT_ID = 20,
} CBWatchedHashesKeyOffsets;

/**
 @brief The key offsets for branchSectionTxDetails
 */
typedef enum{
	CB_BRANCH_TX_DETAILS_BRANCH = 0,
	CB_BRANCH_TX_DETAILS_ORDER_NUM = 1,
	CB_BRANCH_TX_DETAILS_TX_ID = 9,
} CBBranchTxDetailsKeyOffsets;

/**
 @brief The key offsets for txBranchSectionHeightAndID
 */
typedef enum{
	CB_TX_HEIGHT_BRANCH_AND_ID_BRANCH = 0,
	CB_TX_HEIGHT_BRANCH_AND_ID_HEIGHT = 1,
	CB_TX_HEIGHT_BRANCH_AND_ID_ORDER_NUM = 5,
	CB_TX_HEIGHT_BRANCH_AND_ID_TX_ID = 13,
} CBTxHeightBranchAndIDKeyOffsets;

/**
 @brief The key offsets for txAccounts
 */
typedef enum{
	CB_TX_ACCOUNTS_TX_ID = 0,
	CB_TX_ACCOUNTS_ACCOUNT_ID = 8,
} CBTxAccountsKeyOffsets;

/**
 @brief The key offsets for accountTxDetails
 */
typedef enum{
	CB_ACCOUNT_TX_DETAILS_ACCOUNT_ID = 0,
	CB_ACCOUNT_TX_DETAILS_TX_ID = 8,
} CBAccountTxDetailsKeyOffsets;

/**
 @brief The key offsets for branchSectionAccountTimeTx
 */
typedef enum{
	CB_BRANCH_ACCOUNT_TIME_TX_BRANCH = 0,
	CB_BRANCH_ACCOUNT_TIME_TX_ACCOUNT_ID = 1,
	CB_BRANCH_ACCOUNT_TIME_TX_TIMESTAMP = 9,
	CB_BRANCH_ACCOUNT_TIME_TX_ORDERNUM = 17, /**< This is important. 8 bytes for allowing ordering of transactions with the same timestamp by the order they were found. */
	CB_BRANCH_ACCOUNT_TIME_TX_TX_ID = 25,
} CBBranchAccountTimeTxKeyOffsets;

// Structures

/**<
 @brief Counts the in and outflows for an account when processing a transaction.
 */
typedef struct{
	uint64_t accountID;
	uint64_t inAmount;
	uint64_t outAmount;
	bool foundInAddr;
	bool inAddrIndexIsZero;
	uint8_t inAddr[20];
} CBTransactionAccountFlow;

typedef struct{
	CBDatabase * database;
	uint64_t lastAccountID;
	uint64_t nextTxID;
	uint64_t nextOutputRefID;
	CBDatabaseIndex * txDetails;
	CBDatabaseIndex * accountUnconfBalance; /**< Contains the total of all the values of unconfirmed transactions. */
	CBDatabaseIndex * outputDetails; /**< Contains the value, txHash, index and hash identifier of outputs. */
	CBDatabaseIndex * accountTxDetails; /**< Contains the value of the transction for the account and the in/out address. */
	CBDatabaseIndex * branchSectionAccountTimeTx; /**< Entries for the transactions in each accounts ordered by time and 32bit integer that insures transactions with the same time get ordered by the order found. Contains details for on-chain transactions with the balance upto that transaction. Used to fetch balances for accounts and making reorgs easier. */
	CBDatabaseIndex * branchSectionTxDetails; /**< Contains the height and timestamp of transactions in the branches. */
	CBDatabaseIndex * outputAccounts; /**< Entries linking outputs with accounts that own them. */
	CBDatabaseIndex * branchSectionAccountOutputs; /**< Has a value of true when the output is unspent and false when an output is spent. Ordered by height descending. To find unspent outputs go through and find first value for an output, if the first value is unspent then it can be used. */
	CBDatabaseIndex * txAccounts; /**< Entries linking transactions to accounts that own them. */
	CBDatabaseIndex * txHashToID; /**< Contains transaction hashes as keys with the transaction IDs as data. */
	CBDatabaseIndex * txBranchSectionHeightAndID; /**< Entries that list transaction IDs in order of the height and ID */
	CBDatabaseIndex * outputHashAndIndexToID; /**< Contains transaction hashes and output indexes as keys with the output IDs as data. */
	CBDatabaseIndex * watchedHashes; /**< Entries of output identification hashes with account IDs that watch them. */
	CBDatabaseIndex * accountWatchedHashes; /**< Opposite of watchedHashes, linking account IDs with watched hashes. */
} CBAccounterStorage;

typedef struct{
	CBAccounterStorage * storage;
	uint8_t branchSections[CB_MAX_BRANCH_CACHE];
	uint8_t currentSection;
	CBDatabaseRangeIterator it;
	bool started;
} CBAccounterStorageCursor;

typedef struct{
	CBAccounterStorageCursor base;
	uint8_t branchSectionAccountOutputsMin[21];
	uint8_t branchSectionAccountOutputsMax[21];
	CBAssociativeArray spentOutputs; /**< Those outputs marked as spent. */
	uint8_t numSections;
} CBAccounterStorageUnspentOutputCursor;

typedef struct{
	CBAccounterStorageCursor base;
	uint8_t branchSectionAccountTimeTxMin[33];
	uint8_t branchSectionAccountTimeTxMax[33];
} CBAccounterStorageTxCursor;

// Functions

bool CBAccounterAdjustBalances(CBAccounterStorage * storage, uint8_t * low, uint8_t * high, uint8_t section, uint64_t accountID, int64_t adjustment, uint64_t * last);
/**
 @brief Adjusts the balance for an account of th unconfirmed transactions.
 @param self The accounter storage object.
 @param accountID The ID of the account.
 @param value The amount to adjust the balance by.
 @returns true on success, false on failure.
 */
bool CBAccounterAdjustUnconfBalance(CBAccounterStorage * self, uint64_t accountID, uint64_t value);
/**
 @brief Changes the spent status of an output reference on a branch to spent
 @param self The accounter storage object.
 @param prevOut The output hash and index.
 @param branch The branch of the output reference.
 @param blockHeight The height of the output reference.
 @param txInfo A pointer to the transaction information, to be updated unless NULL.
 @returns true on success, false on failure.
 */
bool CBAccounterMakeOutputSpent(CBAccounterStorage * self, CBPrevOut * prevOut, uint8_t branch, uint32_t blockHeight, CBAssociativeArray * txInfo);
/**
 @brief Initialises an accounter storage cursor.
 */
void CBAccounterCursorInit(CBAccounterStorageCursor * cursor, CBAccounterStorage * storage, uint8_t branch);
/**
 @brief Gets the last cumulative balance for a branch section.
 @param self The accounter storage object.
 @param accountID The ID of the account
 @param branchSection the branch section Id.
 @param balance Will be set to the balance
 @returns true on success, false on failure.
 */
bool CBAccounterGetLastAccountBranchSectionBalance(CBAccounterStorage * self, uint64_t accountID, uint8_t branchSection, uint64_t maxTime, uint64_t * balance);
/**
 @brief Gets the ID of the branch data for the last section in the branch, after any forks.
 @param self The accounter storage object.
 @param branch The branch ID.
 @returns The ID.
 */
uint8_t CBAccounterGetLastBranchSectionID(CBAccounterStorage * self, uint8_t branch);
/**
 @brief Gets the ID of the branch data for the parent section of the provided branch section ID.
 @param self The accounter storage object.
 @param branchSectionID The branch section ID.
 @returns CB_NO_PARENT if the section has no parent or the parent ID.
 */
uint8_t CBAccounterGetParentBranchSection(CBAccounterStorage * self, uint8_t branchSectionID);
/**
 @brief Gets the value of a transction for an account.
 @param self The accounter storage object.
 @param txID The transction ID.
 @param accountID The account ID.
 @param value The value to set.
 @returns true on success, false on failure. 
 */
bool CBAccounterGetTxAccountValue(CBAccounterStorage * self, uint64_t txID, uint64_t accountID, int64_t * value);
uint64_t CBAccounterInt64ToUInt64(int64_t value);
/**
 @brief Removes a transaction's account details from a branch.
 @param storage The accounter storage object.
 @param txID The transaction ID.
 @param txHash The transaction hash.
 @param branch The branch to remove the transaction details from.
 @param orderNum The data for branch ordernums.
 @param height The data for branch heights.
 @returns true on success, false on failure.
 */
bool CBAccounterRemoveTransactionFromBranch(CBAccounterStorage * storage, uint64_t txID, uint8_t * txHash, uint8_t branch, uint8_t * orderNum, uint8_t * height);
void CBAccounterUInt64ToInt64(uint64_t raw, int64_t * value);
/**
 @brief Compares 32-bit integers.
 @param int1 A pointer to the first 32-bit integer.
 @param int2 A pointer to the second 32-bit integer.
 @returns CB_COMPARE_MORE_THAN if the first integer is bigger than the second and likewise.
 */
CBCompare CBCompareUInt32(CBAssociativeArray * foo, void * int1, void * int2);

#endif
