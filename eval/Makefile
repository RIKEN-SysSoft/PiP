
.PHONY: all
all:
	( cd mmap;  make )
	( cd spawn; make )

.PHONY: clean
clean:
	rm -f $(LIBRARY) *.o *~
	( cd mmap;  make clean )
	( cd spawn; make clean )
