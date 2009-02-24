#!/usr/bin/env python
"""
Setup file for Ca-Python using distutils package.
use:
  env CC=g++ /usr/local/bin/python setup.py build
  for FileIndex module
"""
import os,platform

# Define defaults, or set the appropriate environment variables
EPICSROOT     = "/Users/kasemir/epics/R3.14.8.2"
EPICSBASE     = os.path.join(EPICSROOT,"base")
EPICSEXT      = os.path.join(EPICSROOT,"extensions")
EPICSHOSTARCH = "darwin-ppc"

if os.environ.has_key("EPICS_BASE"):
    EPICSBASE=os.environ["EPICS_BASE"]
    EPICSROOT=os.path.join(EPICSBASE,"..")
if os.environ.has_key("EPICS_EXTENSIONS"):
    EPICSEXT=os.environ["EPICS_EXTENSIONS"]

if os.environ.has_key("EPICS_HOST_ARCH"):
    EPICSHOSTARCH=os.environ["EPICS_HOST_ARCH"]

#
System=platform.system()
#
TKINC="/usr/local"
TKLIB="/usr/local"
CC="g++"

os.environ["CC"]="g++"

from distutils.core import setup,Extension
rev="$Revision: 1.1.1.1 $"
IndexFile_extension=Extension(
    "_IndexFile",
    ["IndexFile.i"],
    language="c++",
    swig_opts=["-importall",
               "-I%s"%"../Storage",
               "-c++",
               "-D__ppc__",
               #"-modern",
               "-apply",
               #"-classic",
               "-I%s"%os.path.join(EPICSEXT,"include"),
               "-I%s"%os.path.join(EPICSBASE,"include"),
               "-I%s"%os.path.join(EPICSBASE,"include/os"),
               "-I%s"%os.path.join(EPICSBASE,"include/os",System),
               "-I%s"%os.path.join("/usr","include"),
               "-I%s"%os.path.join("/usr","include","gcc/darwin/3.3"),
               "-I%s"%os.path.join("/usr","include","gcc/darwin/3.3/c++"),
               "-I%s"%os.path.join("/usr","include","gcc/darwin/3.3/c++/ppc-darwin"),
               ],
    include_dirs=["../Storage",
                  os.path.join(EPICSBASE,"include"),
                  os.path.join(EPICSBASE,"include/os"),
                  os.path.join(EPICSBASE,"include/os",System),
                  os.path.join(EPICSEXT,"include"),
                  ],
    define_macros=[("__ppc__",None)  ],
    undef_macros="",
    libraries=["Storage","Tools","ChanArchIO","Com","xerces-c"],
    library_dirs=[os.path.join(EPICSBASE,"lib",EPICSHOSTARCH),
                  os.path.join(EPICSEXT,"lib",EPICSHOSTARCH),
                  #os.path.join(EPICSEXT,"lib",System),
                  ],
    runtime_library_dirs=[os.path.join(EPICSBASE,"lib",EPICSHOSTARCH),
                          os.path.join(EPICSEXT,"lib",EPICSHOSTARCH),
                          #os.path.join(EPICSEXT,"lib",System),
                          ],
    )

setup(name="dbr_types",
      version=rev[11:-2],
      author="Noboru Yamamoto, KEK, JAPAN",
      author_email = "Noboru.YAMAMOTO@kek.jp",
      url="http://www-acc.kek.jp/",
      py_modules=[ #"dbr_types",
    "IndexFile",
    "ChanArchDataFile",
    "IndexRecovery"
    ],
      ext_modules=[
                   IndexFile_extension
                   ],
      )

