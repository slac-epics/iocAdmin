//IndexFile.i
%module IndexFile
%include typemaps.i
%include cpointer.i
%include exception.i
%{
#include "Index.h"
#include "IndexFile.h"
extern "C"{
int foo(void){
	return 1;
}
long bah=0;
}
%}

%include "epicsTime_external.h"

%include "IndexFile_external.h"

int foo(void);
long bah=0;
