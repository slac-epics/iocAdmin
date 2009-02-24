# GNUPlot command file for comparing
# some export methods

!ArchiveExport /mnt/bogart_home/snsdoc/RF/HPRF/LANLXmtrData/index Test_HPRF:Kly1:Pwr_Fwd_Out -s "01/19/2003 07:30:00" -e "01/20/2003" -o /tmp/raw -gnu

!ArchiveExport /mnt/bogart_home/snsdoc/RF/HPRF/LANLXmtrData/index Test_HPRF:Kly1:Pwr_Fwd_Out -s "01/19/2003 07:30:00" -e "01/20/2003" -o /tmp/plt -gnu -plotbin 600

set timefmt "%m/%d/%Y %H:%M:%S"
set xdata time
set xlabel "Time"
set format x "%H:%M"
set ylabel "Klystron Output [kW]"
# X axis tick control:
# One tick per hour:
#set xtics 30
#set mxtics 3
#set grid xtics noytics
set term png small color xffffff x000000 x000000
set output 'plotbin.png'
plot '/tmp/raw' using 1:3 title 'Raw Data' with points lt -1 , '/tmp/plt' using 1:3 title 'Plot-Binned' with lines lt 1


!wc /tmp/raw /tmp/plt
!eog plotbin.png

