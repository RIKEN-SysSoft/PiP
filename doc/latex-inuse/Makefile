LIBPIP = libpip-manpages

all: $(LIBPIP).pdf

pdf: $(LIBPIP).pdf

$(LIBPIP).pdf: clean
	./man-pages.sh
	pdflatex $(LIBPIP)
	makeindex $(LIBPIP).idx
	pdflatex $(LIBPIP)
	latex_count=5 ; \
	while egrep -s 'Rerun (LaTeX|to get cross-references right)' $(LIBPIP).log && [ $$latex_count -gt 0 ] ;\
	    do \
	      echo "Rerunning latex...." ;\
	      pdflatex $(LIBPIP) ;\
	      latex_count=`expr $$latex_count - 1` ;\
	    done
	makeindex $(LIBPIP).idx
	pdflatex $(LIBPIP)


clean:
	rm -f *.ps *.dvi *.aux *.toc *.idx *.ind *.ilg *.log *.out *.brf *.blg *.bbl
	rm -f *-inputs.tex
	rm -f \#*\# *~

veryclean:
	rm -f $(LIBPIP).pdf
