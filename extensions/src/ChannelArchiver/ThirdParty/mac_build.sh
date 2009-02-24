# This is a log of what once worked:
# Mac OSX 'tiger' with gcc-3.3 selected via
#   sudo gcc_select 3.3
# Your milage will vary.

tar vzoxf w3c-libwww-5.4.0.tgz
cd w3c-libwww-5.4.0
patch <../w3c-libwww-5.4.0_osx_patch
./configure --enable-shared --enable-static     --with-zlib --with-ssl
make
sudo make install
cd ..

tar vzxf xmlrpc-c-0.9.10_darwin.tgz
cd xmlrpc-c-0.9.10
./configure
make
sudo make install
cd ..

tar vzxf xerces-c-current.tar.gz
cd xerces-c-src2_4_0
export XERCESCROOT=`pwd`
cd $XERCESCROOT/src/xercesc
autoconf
./runConfigure -p macosx -n native -P /usr/local
make
sudo make install

tar vzxf expat-1.95.8.tar.gz
cd expat-1.95.8
./configure
make
sudo make install
cd .. 

tar vzxf XML-Parser-2.34.tar.gz
cd XML-Parser-2.34
perl Makefile.PL
make
sudo make install
cd ..

tar vzoxf XML-Simple-2.09.tar.gz
cd XML-Simple-2.09
perl Makefile.PL
make
sudo make install
cd ..

tar vzoxf Frontier-RPC-0.07b4.tar.gz
cd Frontier-RPC-0.07b4
perl Makefile.PL
make
sodo make install
cd ..
