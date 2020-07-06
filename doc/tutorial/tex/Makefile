TARGET = ge-pip
BIBS   = ref.bib hori.bib
LATEX  = pdflatex

PDF    = $(TARGET).pdf

SRCS = $(shell find .       -name '*.tex' -print)
FIGS = $(shell find ../figs -name '*.pdf' -print)
PRGS = $(shell find ../prgs -name '*.c'   -print)

all : $(PDF)

$(PDF): $(SRCS) $(FIGS) $(PRGS) $(BIBS) Makefile
	$(LATEX) $(TARGET);
	$(LATEX) $(TARGET);	
	$(LATEX) $(TARGET);	
	grep -e $(LATEX) -e "LaTeX\ Warning" -e "Package\ natbib\ Warning" -e "undefined" -e "multiply\ defined" $(TARGET).log;

clean:
	rm -f $(PDF) *.lol *.lof *.lot *.toc *.bbl *.blg *.out *.aux *.log *~ *.dvi ../figs/*.bb ../progs/*~ *~

veryclean: clean
	rm -f $(PDF)
