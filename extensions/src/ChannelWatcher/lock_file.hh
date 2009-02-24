// ==============================================================================
//
//  Abs:  Function prototype for file locking mechanism.
//
//  Name: lock_file.hh
//
//  Rem:  Function prototype for locking of a file by an application. Used 
//        to allow for multiple processes to wait on a file lock in case 
//        the locking application dies. Currently only supports locking on 
//        NFS.
//
//  Auth: 02-Apr-2003, James Silva (jsilva@slac.stanford.edu)
//  Rev:  
//
// --------------------------------------------------------------------------------
//
//  Mod: (newest to oldest):
//
// ==============================================================================
//
#if !defined( lock_file_hh )


int lock_file(const char * lock_file_name);


#define lock_file_hh
#endif // guard
