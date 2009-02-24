
all:  show

dots: driv_dev_rec.pdf tagtable.pdf

driv_dev_rec.pdf: driv_dev_rec.dot

tagtable.pdf: tagtable.dot

slides.pdf: dots slides.tex
	pdflatex slides
	while egrep -s 'Rerun to get cross-references right' slides.log ;\
        do \
           echo "Re-texing" ;\
           pdflatex slides ;\
        done

show: slides.pdf
	xpdf slides.pdf

%.ps2: %.dot
	dot -Tps2 $< -o $@

%.pdf: %.ps2
	ps2pdf $< $@

clean:
	-rm *.log *.toc *.aux *.idx *.ilg *.ind *.out *~
