# GNUPlot command file
set xlabel 'RTree M (max. records / node)'
set key box
set key 90, 15
set grid
plot 'mastercompare' using 1:3 title 'Size of new master index [MB]' with linespoints,\
     'mastercompare' using 1:2 title 'Creation time from M=10 indices [min]' with linespoints, \
     'mastercompare' using 1:4 title '...  M=50 indices [min]' with linespoints

set term png small color xffffff x000000 x000000
set output 'masteridxcompare.png'
replot
!eog masteridxcompare.png
