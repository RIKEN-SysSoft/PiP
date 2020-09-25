
#
# exit_code
#
EXIT_PASS=0
EXIT_FAIL=1
EXIT_XPASS=2		# passed, but it's unexpected. fixed recently?
EXIT_XFAIL=3		# failed, but it's expected. i.e. known bug
EXIT_UNRESOLVED=4	# cannot determine whether (X)?PASS or (X)?FAIL
EXIT_UNTESTED=5		# not tested, this test hasn't been written yet
EXIT_UNSUPPORTED=6	# not tested, this test cannot run in this environment
EXIT_KILLED=7		# killed by Control-C or something
