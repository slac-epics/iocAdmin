all:    octave matlab

OUT=O.$(EPICS_HOST_ARCH)

octave:	$(OUT) $(OUT)/ArchiveData.oct

matlab: $(OUT) $(OUT)/ArchiveData.mexglx

# Both Octave and Matlab include
# compilation tools mkoctfile resp. mex
# which handle all the magic.
# In theory, we only provide the include & link
# directives for the archiver & xml-rpc.
# Even better: At least on Unix, both mkoctfile & mex
# understand the same -v, -I, -L & -l syntax for that.

# Do you want verbose compilation?
# FLAGS = -v
# Includes -------------------------------------------
# Archiver
FLAGS += -I../../../include
# EPICS Base
FLAGS += -I$(EPICS_BASE_RELEASE)/include
FLAGS += -I$(EPICS_BASE_RELEASE)/include/os/$(HOST_ARCH)
# xml-rpc
FLAGS += -I/usr/local/include
# Libraries ------------------------------------------
FLAGS += -L../../../lib/$(EPICS_HOST_ARCH) -lDataServer -lStorage -lTools 
# EPICS Base
FLAGS += -L$(EPICS_BASE_RELEASE)/lib/$(EPICS_HOST_ARCH) -ldbStaticHost -lCom -lca
# XML-RPC library
# TODO: This is not portable to Win32
FLAGS += `xmlrpc-c-config libwww-client --libs  | sed -e 's/-Wl,--rpath.*//'`

$(OUT):
	mkdir $(OUT)

$(OUT)/ArchiveData.oct: ArchiveData.cpp
	mkoctfile -DOCTAVE $(FLAGS) ArchiveData.cpp -o $(OUT)/ArchiveData.oct

$(OUT)/ArchiveData.mexglx: ArchiveData.cpp
	mex $(FLAGS) ArchiveData.cpp -o $(OUT)/ArchiveData.mexglx

# In reality, your milage will vary.
# Matlab 6p1 under Linux gave this runtime error:
# "...undefined symbol: __gxx_personality_v0".
# Fix: Compile like this from within matlab:
#   eval(['mex -v -f ' matlabroot '/bin/cxxopts.sh ArchiveData.cpp'])
# ... but then you'd have to add all the other FLAGS into that.

clean:
	-rm -rf $(OUT)

rebuild: clean all

