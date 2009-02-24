#
#  
#
# THIS BUILDS IOCLOGANDFWDSERVER AND STATICALLY LINKS TO LIBCMLOG.A
# Note, put the static library after the .o file in the g++ link.
# 
# THIS BUIDS WITH ORACLE (send to rdbSrv) SUPPORT and CMLOG.  
# IT LINS **STATICALLY** to libcmlog.a.  and libsendToRDBsrvr.a
#
mkdir -p O.linux-x86
#
/usr/bin/gcc -c   -D_POSIX_C_SOURCE=199506L -D_POSIX_THREADS -D_XOPEN_SOURCE=500        -D_X86_  -DUNIX  -D_BSD_SOURCE -Dlinux  -D_REENTRANT -ansi  -O3  -Wall     -D_CMLOG_BUILD_CLIENT -DCMLOG_FILELOG_OPTION -DCMLOG_SLAC_THROTTLING  -DLOG_TO_ORACLE   -g  -I. -I.. -I../../include/os/Linux -I../../include      -I/home/softegr/ronm/dev/cmlog/debug_throttle/cmlog/include    iocLogAndFwdServer.c 
#
/usr/bin/g++ -o iocLogAndFwdServer  -L../../lib/linux-x86/   /home/softegr/ronm/dev/cmlog/debug_throttle/cmlog/lib/Linux-i386/libcmlog.a   -Wl,-rpath,/usr/local/lcls/epics/base/base-R3-14-8-2-lcls2/lib/linux-x86  iocLogAndFwdServer.o  /home/softegr/ronm/dev/cmlog/debug_throttle/cmlog/lib/Linux-i386/libcmlog.a   /usr/local/lcls/tools/msglog/rdbSrv/libsendToRDBsrvr.a     -lrecIoc -lsoftDevIoc -liocsh -lmiscIoc -lrsrvIoc -ldbtoolsIoc -lasIoc -ldbIoc -lregistryIoc -ldbStaticIoc -lca -lCom -lca -lCom 

mv iocLogAndFwdServer O.linux-x86

exit


# THIS BUILDS WITH CMLOG SUPPORT ONLY.  IT LINKS **STATICALLY** to libcmlog.a
#

/usr/bin/gcc -c   -D_POSIX_C_SOURCE=199506L -D_POSIX_THREADS -D_XOPEN_SOURCE=500        -D_X86_  -DUNIX  -D_BSD_SOURCE -Dlinux  -D_REENTRANT -ansi  -O3  -Wall     -D_CMLOG_BUILD_CLIENT -DCMLOG_FILELOG_OPTION -DCMLOG_SLAC_THROTTLING       -g  -I. -I.. -I../../include/os/Linux -I../../include      -I/home/softegr/ronm/dev/cmlog/debug_throttle/cmlog/include    iocLogAndFwdServer.c 
#
/usr/bin/g++ -o iocLogAndFwdServer  -L../../lib/linux-x86/   /home/softegr/ronm/dev/cmlog/debug_throttle/cmlog/lib/Linux-i386/libcmlog.a   -Wl,-rpath,/usr/local/lcls/epics/base/base-R3-14-8-2-lcls2/lib/linux-x86  iocLogAndFwdServer.o  /home/softegr/ronm/dev/cmlog/debug_throttle/cmlog/lib/Linux-i386/libcmlog.a   -lrecIoc -lsoftDevIoc -liocsh -lmiscIoc -lrsrvIoc -ldbtoolsIoc -lasIoc -ldbIoc -lregistryIoc -ldbStaticIoc -lca -lCom -lca -lCom 
