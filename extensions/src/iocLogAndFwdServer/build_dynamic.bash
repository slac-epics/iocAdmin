#
# DNAMIC BUILD!!
# This builds against my DEV version of sendRDBsrvr.so and libcmlog.so
# It is the output of gmake (standard makefile) with includes and libs
# coming from my DEV dirs instead.
#
# After I deliver those two to production (cvs and build) then I can
# just use gmake and regular makefile to make instead of this beast.
#
#
mkdir -p O.linux-x86
cd O.linux-x86
#
# stick my development lib directories on the front of LD_LIB...
#
export LD_LIBRARY_PATH=/home/softegr/ronm/dev/cmlog/debug_throttle/cmlog/lib/Linux-i386/home/softegr/ronm/dev/tools/msglog/rdbSrv:${LD_LIBRARY_PATH}
#
/usr/bin/gcc -c   -D_POSIX_C_SOURCE=199506L -D_POSIX_THREADS -D_XOPEN_SOURCE=500        -D_X86_  -DUNIX  -D_BSD_SOURCE -Dlinux  -D_REENTRANT -ansi  -O3  -Wall     -D_CMLOG_BUILD_CLIENT -DCMLOG_FILELOG_OPTION -DCMLOG_SLAC_THROTTLING  -DLOG_TO_ORACLE      -g  -I/home/softegr/ronm/dev/tools/msglog/rdbSrv -I/home/softegr/ronm/dev/cmlog/debug_throttle/cmlog/include -I. -I.. -I../../../include/os/Linux -I../../../include    ../iocLogAndFwdServer.c 
#.
# 2. take out -L to production cmlog library or it would use the .a file in #1.
#
/usr/bin/g++ -o iocLogAndFwdServer  -L/home/softegr/ronm/dev/tools/msglog/rdbSrv -L/home/softegr/ronm/dev/cmlog/debug_throttle/cmlog/lib/Linux-i386 -L../../../lib/linux-x86/    -L      -Wl,-rpath,/usr/local/lcls/epics/base/base-R3-14-8-2-lcls2/lib/linux-x86        iocLogAndFwdServer.o    -lrecIoc -lsoftDevIoc -liocsh -lmiscIoc -lrsrvIoc -ldbtoolsIoc -lasIoc -ldbIoc -lregistryIoc -ldbStaticIoc -lca -lCom  -lca -lCom -lcmlog -lsendToRDBsrvr 
#
# this uses big and little L for cmlog. Wrong.  We want .so files to be used.
#
#/usr/bin/g++ -o iocLogAndFwdServer  -L/home/softegr/ronm/dev/tools/msglog/rdbSrv -L/home/softegr/ronm/dev/cmlog/debug_throttle/cmlog/lib/Linux-i386 -L../../../lib/linux-x86/ -L/usr/local/lcls/package/cmlog/lib/Linux-i386/    -L      -Wl,-rpath,/usr/local/lcls/epics/base/base-R3-14-8-2-lcls2/lib/linux-x86        iocLogAndFwdServer.o    -lrecIoc -lsoftDevIoc -liocsh -lmiscIoc -lrsrvIoc -ldbtoolsIoc -lasIoc -ldbIoc -lregistryIoc -ldbStaticIoc -lca -lCom  -lca -lCom -lsendToRDBsrvr 
