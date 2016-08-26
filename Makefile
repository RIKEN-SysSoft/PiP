
.PHONY: all
all:
	( cd lib;         make )
	( cd preload;     make )
	( cd test/basics; make )
	( cd test/spawn;  make )

.PHONY: clean
clean:
	rm -f $(LIBRARY) *.o *~
	( cd lib;         make clean )
	( cd preload;     make clean )
	( cd include;     make clean )
	( cd test/basics; make clean )
	( cd test/spawn;  make clean )
