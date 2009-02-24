* xmlrpc-c-0.9.9.tar.gz
XML-RPC library for C/C++, see "Setup, Installation" in manual.

* xmlrpc-c-0.9.10_darwin.tgz
Similar, but patched such that it compiles for Mac OS X.

** patch_xmlrpc-c-0.9.9
Patch file for xmlrpc-c-0.9.9.tar.gz, see "Setup, Installation" in manual.

* xerces-c-current.tar.gz
Xerces XML Library, see "Setup, Installation" in manual.

* Frontier-RPC-0.07b4.tar.gz
An XML-RPC library for perl, see guess what.

* expat-1.95.8.tar.gz
Another XML parser. If you don't already have it, you 
might need it for the other packages.

* XML-Simple-2.09.tar.gz, XML-Parser-2.34.tar.gz,
  libwww-perl-5.800.tar.gz, HTML-Parser-3.36.tar.gz, HTML-Tagset-3.03.tar.gz,
  URI-1.31.tar.gz
More perl libraries that you might need to make
Frontier work.
On Mac OSX, Frontier didn't complain during the installation,
but to actually run, this was needed as well:
  tar vzxf libwww-perl-5.800.tar.gz 
  cd libwww-perl-5.800
  perl Makefile.PL 
  sudo make install

