#
# Regular cron jobs for the cbitcoin package
#
0 4	* * *	root	[ -x /usr/bin/cbitcoin_maintenance ] && /usr/bin/cbitcoin_maintenance
