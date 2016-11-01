
.PHONY: all
all:
	( cd lib;          make )
	( cd preload;      make )
	( cd test/basics;  make )
	( cd test/spawn;   make )
	( cd test/compat;  make )
	( cd test/openmp;  make )
	( cd test/fortran; make )
	( cd eval;         make )

.PHONY: clean
clean:
	rm -f $(LIBRARY) *.o *~
	( cd lib;          make clean )
	( cd preload;      make clean )
	( cd include;      make clean )
	( cd test/basics;  make clean )
	( cd test/spawn;   make clean )
	( cd test/compat;  make clean )
	( cd test/openmp;  make clean )
	( cd test/fortran; make clean )
	( cd eval;         make clean )
