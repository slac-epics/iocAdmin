#!/bin/csh
#
# This is a dumb makefile for test program which is actually a script.
# FIX THIS LATER.  PUT IT IN REAL EPICS MAKEFILE.
#
cd O.linux-x86/
#
/usr/bin/gcc -c   -D_POSIX_C_SOURCE=199506L -D_POSIX_THREADS -D_XOPEN_SOURCE=500        -D_X86_  -DUNIX  -D_BSD_SOURCE -Dlinux  -D_REENTRANT -ansi  -O3  -Wall        -g  -I. -I.. -I/afs/slac/g/lcls/epics/base/include/os/Linux -I/afs/slac/g/lcls/epics/base/include       ../iocLogServerLogMsg.c 
#
#
/usr/bin/g++ -o iocLogServerLogMsg  -L../../../lib/linux-x86/   -L      -Wl,-rpath, -L/afs/slac.stanford.edu/g/lcls/epics/base/lib/linux-x86        iocLogServerLogMsg.o    -lrecIoc -lsoftDevIoc -liocsh -lmiscIoc -lrsrvIoc -ldbtoolsIoc -lasIoc -ldbIoc -lregistryIoc -ldbStaticIoc -lca -lCom  -lca -lCom 
#
