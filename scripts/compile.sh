#!/bin/sh

# transactions
basedir="/home/joeldejesus/Workspace/cbitcoin/scripts"

target="TransactionInput"
(cd ${basedir}/${target} && make clean && perl coinx2.pl && cp old-config/* ./ && perl Makefile.PL && make && sudo make install && sudo chown -R joeldejesus:joeldejesus ./)
target="TransactionOutput"
(cd ${basedir}/${target} && make clean && perl coinx2.pl && cp old-config/* ./ && perl Makefile.PL && make && sudo make install && sudo chown -R joeldejesus:joeldejesus ./)
target="Transaction"
(cd ${basedir}/${target} && make clean && perl coinx2.pl && cp old-config/* ./ && perl Makefile.PL && make && sudo make install && sudo chown -R joeldejesus:joeldejesus ./)
