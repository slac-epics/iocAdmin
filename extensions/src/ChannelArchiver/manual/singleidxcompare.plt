# GNUPlot command file
set xlabel 'RTree M (max. records / node)'
set key box
set grid
plot 'singleidxcompare' using 1:2 title 'Index Size [MB]' with linespoints,\
     'singleidxcompare' using 1:3 title 'Time to create [sec]' with linespoints
set term png small color xffffff x000000 x000000
set output 'singleidxcompare.png'
replot
!eog singleidxcompare.png
