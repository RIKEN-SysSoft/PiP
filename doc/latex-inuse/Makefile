LIBPIP = libpip-manpages

all: $(LIBPIP).pdf

pdf: $(LIBPIP).pdf

libpip-manpages.tex:
	echo "Rerun configure"
	exit 1

$(LIBPIP).pdf: clean libpip-manpages.tex
	./man-pages.sh
	pdflatex $(LIBPIP) > /dev/null 2>&1
	makeindex $(LIBPIP).idx > /dev/null 2>&1
	pdflatex $(LIBPIP) > /dev/null 2>&1
	latex_count=5 ; \
	while egrep -s 'Rerun (LaTeX|to get cross-references right)' $(LIBPIP).log && [ $$latex_count -gt 0 ] ;\
	    do \
	      echo "Rerunning latex...." ;\
	      pdflatex $(LIBPIP) > /dev/null 2>&1 ;\
	      latex_count=`expr $$latex_count - 1` ;\
	    done
	makeindex $(LIBPIP).idx > /dev/null 2>&1
	pdflatex $(LIBPIP) > /dev/null 2>&1
	grep -e "LaTeX\ Warning" -e "Package\ natbib\ Warning" -e "undefined" -e "multiply\ defined" $(LIBPIP).log | cat

clean:
	rm -f *.ps *.dvi *.aux *.toc *.idx *.ind *.ilg *.log *.out *.brf *.blg *.bbl
	rm -f *-inputs.tex
	rm -f \#*\# *~

veryclean:
	rm -f $(LIBPIP).pdf
