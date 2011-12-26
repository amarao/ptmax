#
# Regular cron jobs for the ptmax package
#
0 4	* * *	root	[ -x /usr/bin/ptmax_maintenance ] && /usr/bin/ptmax_maintenance
