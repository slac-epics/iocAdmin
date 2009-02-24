set timefmt "%m/%d/%Y %H:%M:%S"
set xdata time
set xlabel "Time"
set format x "%m/%d/%Y\n%H:%M:%S"
set ylabel "Data"
set grid
set mxtics 2
set pointsize 0.4
plot 'test/dtl' index 0 using 1:3 title 'DTL_HPRF:Tnk1:T [F]' with lines, 'test/dtl_pb' index 0 using 1:3 title 'plot-binned' with points, 'test/dtl' index 1 using 1:3 title 'DTL_HPRF:Tnk2:T [F]' with lines, 'test/dtl_pb' index 1 using 1:3 title 'plot-binned' with points

pause 5 "Please check picture"

set output '/tmp/plotbin.png'
set terminal png
replot

