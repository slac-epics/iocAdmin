#!/bin/csh
# THIS MAKES THE TEST PROGRAM FOR IOCLOGANDFWDSERVER.
# This is the version can be executed on lcls-daemon1.
# This is a dumb makefile for test program which is actually a script.
# FIX THIS LATER.  PUT IT IN REAL EPICS MAKEFILE.
#
mkdir -p O.linux-x86
cd O.linux-x86/
#
/usr/bin/gcc -c   -D_POSIX_C_SOURCE=199506L -D_POSIX_THREADS -D_XOPEN_SOURCE=500        -D_X86_  -DUNIX  -D_BSD_SOURCE -Dlinux  -D_REENTRANT -ansi  -O3  -Wall        -g  -I. -I.. -I/usr/local/lcls/epics/base/base-R3-14-8-2-lcls2/include/os/Linux -I/usr/local/lcls/epics/base/base-R3-14-8-2-lcls2/include       ../iocLogServerLogMsg.c 
#
#
/usr/bin/g++ -o iocLogServerLogMsg  -L../../../lib/linux-x86/   -L      -Wl,-rpath, -L/usr/local/lcls/epics/base/base-R3-14-8-2-lcls2/lib/linux-x86        iocLogServerLogMsg.o    -lrecIoc -lsoftDevIoc -liocsh -lmiscIoc -lrsrvIoc -ldbtoolsIoc -lasIoc -ldbIoc -lregistryIoc -ldbStaticIoc -lca -lCom  -lca -lCom 
#
