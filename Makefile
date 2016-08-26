
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

BACKUP = pip.tgz

.PHONY: backup
backup:
	( cd ..; \
	mv -f $(BACKUP).8 $(BACKUP).9; \
 	mv -f $(BACKUP).7 $(BACKUP).8; \
	mv -f $(BACKUP).6 $(BACKUP).7; \
	mv -f $(BACKUP).5 $(BACKUP).6; \
	mv -f $(BACKUP).4 $(BACKUP).5; \
	mv -f $(BACKUP).3 $(BACKUP).4; \
	mv -f $(BACKUP).2 $(BACKUP).3; \
	mv -f $(BACKUP).1 $(BACKUP).2; \
	mv -f $(BACKUP).0 $(BACKUP).1; \
	mv -f $(BACKUP)   $(BACKUP).0; \
	tar czvf $(BACKUP) PIP/;     \
	ls -l $(BACKUP); )
