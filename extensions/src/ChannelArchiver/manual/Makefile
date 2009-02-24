all: tex show

tex: manual.tex
	pdflatex manual.tex
	makeindex manual.idx
	while egrep -s 'Rerun to get cross-references right' manual.log ;\
        do \
           echo "Re-texing" ;\
           pdflatex manual.tex ;\
        done

show:
	open manual.pdf

clean:
	-rm *.log *.toc *.aux *.idx *.ilg *.ind *.out *~
