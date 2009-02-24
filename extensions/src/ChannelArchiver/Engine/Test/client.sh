while true
do
	lynx -dump http://localhost:4812/group/test.lst | fgrep [1]ramp1
	sleep 0.1
done
