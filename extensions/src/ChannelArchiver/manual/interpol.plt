# GNUPlot command file for comparing
# some export methods

!ArchiveExport ../DemoData/index fred -s "03/23/2004 08:48:30" -gnu -o /tmp/raw
!ArchiveExport ../DemoData/index fred -s "03/23/2004 08:48:30"  -gnu -o /tmp/pb -plotbin 5
!ArchiveExport ../DemoData/index fred -s "03/23/2004 08:48:30"  -gnu -o /tmp/av -average 10
!ArchiveExport ../DemoData/index fred -s "03/23/2004 08:48:30"  -gnu -o /tmp/lin -linear 10

set timefmt "%m/%d/%Y %H:%M:%S"
set xdata time
set xlabel "Time"
set format x "%H:%M:%S"
set ylabel "Data"
# X axis tick control:
# One tick per hour:
set xtics 30
set mxtics 3
set grid xtics noytics
set term png small color xffffff x000000 x000000 x000000 xff0000 x0000ff
set output 'interpol.png'
plot '/tmp/raw' using 1:3 title 'Raw Data' with lines , '/tmp/av' using 1:3 title 'Averages' with points pt 9 ps 1.5, '/tmp/lin' using 1:3 title 'Linear' with points pt 7 ps 1.5
!eog interpol.png

