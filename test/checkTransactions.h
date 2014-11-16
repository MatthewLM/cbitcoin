#include <inttypes.h>

typedef struct{
	unsigned char * addr;
	int height;
	int64_t amount;
	long long int timestamp;
	unsigned char * txHash;
	long long int cumBalance;
	long long int cumUnconfBalance;
} CBTestTxDetails;

void checkTransactions(CBDepObject cursor, CBTestTxDetails * txTestDetails, int numTxs);
void checkTransactions(CBDepObject cursor, CBTestTxDetails * txTestDetails, int numTxs){
	CBTransactionDetails txDetails;
	for (int x = 0; x < numTxs; x++) {
		if (CBAccounterGetNextTransaction(cursor, &txDetails) != CB_TRUE) {
			printf("TX %u FAIL\n", x);
			exit(EXIT_FAILURE);
		}
		if (memcmp(txDetails.accountTxDetails.addrHash, txTestDetails[x].addr, 20)) {
			printf("TX %u ADDR HASH FAIL\n", x);
			exit(EXIT_FAILURE);
		}
		if (txDetails.height != txTestDetails[x].height) {
			printf("TX %u HEIGHT FAIL %u != %u\n", x, txDetails.height, txTestDetails[x].height);
			exit(EXIT_FAILURE);
		}
		if (txDetails.accountTxDetails.amount != txTestDetails[x].amount) {
			printf("TX %u AMOUNT FAIL %" PRId64 " != %" PRId64 "\n", x, txDetails.accountTxDetails.amount, txTestDetails[x].amount);
			exit(EXIT_FAILURE);
		}
		if (txDetails.timestamp != txTestDetails[x].timestamp){
			printf("TX %u TIMESTAMP FAIL\n", x);
			exit(EXIT_FAILURE);
		}
		if (memcmp(txDetails.txHash,  txTestDetails[x].txHash, 32)){
			printf("TX %u TX HASH FAIL\n", x);
			exit(EXIT_FAILURE);
		}
		if (txDetails.cumBalance != txTestDetails[x].cumBalance) {
			printf("TX %u CUM BALANCE FAIL\n", x);
			exit(EXIT_FAILURE);
		}
		if (txDetails.cumUnconfBalance != txTestDetails[x].cumUnconfBalance) {
			printf("TX %u CUM UNCONF BALANCE FAIL\n", x);
			exit(EXIT_FAILURE);
		}
	}
	if (CBAccounterGetNextTransaction(cursor, &txDetails) != CB_FALSE) {
		printf("TX END FAIL\n");
		exit(EXIT_FAILURE);
	}
	CBFreeAccounterCursor(cursor);
}
