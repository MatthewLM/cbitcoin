typedef struct{
	uint8_t * addr;
	uint32_t height;
	int64_t amount;
	uint64_t timestamp;
	uint8_t * txHash;
} CBTestTxDetails;

void checkTransactions(CBDepObject cursor, CBTestTxDetails * txTestDetails, uint8_t numTxs);
void checkTransactions(CBDepObject cursor, CBTestTxDetails * txTestDetails, uint8_t numTxs){
	CBTransactionDetails txDetails;
	for (uint8_t x = 0; x < numTxs; x++) {
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
			printf("TX %u AMOUNT FAIL %lli != %lli\n", x, txDetails.accountTxDetails.amount, txTestDetails[x].amount);
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
	}
	if (CBAccounterGetNextTransaction(cursor, &txDetails) != CB_FALSE) {
		printf("TX END FAIL\n");
		exit(EXIT_FAILURE);
	}
	CBFreeAccounterCursor(cursor);
}
