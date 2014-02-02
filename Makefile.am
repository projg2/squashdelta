bin_PROGRAMS = squashdelta

squashdelta_SOURCES = \
	src/squashdelta.cxx src/squashfs.cxx src/util.cxx

EXTRA_DIST = NEWS
NEWS: configure.ac Makefile.am
	git for-each-ref refs/tags --sort '-*committerdate' \
		--format '# %(tag) (%(*committerdate:short))%0a%(contents:body)' \
		> NEWS
