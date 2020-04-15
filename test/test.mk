
all:
	make
	make -C scripts
	make -C util
	make -C prog test-progs
	make -C basics
	make -C compat
	make -C pthread
	make -C openmp
	make -C cxx
	make -C fortran
	make -C issues
	make -C blt
	./test.sh -A test.list
