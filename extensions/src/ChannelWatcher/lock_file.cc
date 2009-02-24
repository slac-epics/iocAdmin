// ================================================================================
//
//   Abs: Function for locking a file by an application on NFS.
//
//   Name: lock_file.cc
//         Entries: lock_file
//
//   Proto: lock_file.hh
//
//   Auth: 02-Apr-2003, James Silva (jsilva@slac.stanford.edu)
//   Rev:  DD-MMM-YYYY, Reviewer's Name (.NE. Author's Name)
//
// --------------------------------------------------------------------------------
//
//   Mod:  (newest to oldest)
//         
//
// ================================================================================

#include <stdio.h>             // for FILE, fopen, fclose
#include <unistd.h>            // for lockf
#include <fcntl.h>             // for O_RDWR
#include <stdlib.h>            // for atof, atoi, getenv



//=============================================================================
//
//  Abs:  Create a file lock given a lock file name
//
//  Name: lock_file
//
//  Args:   lock_file_name            Name of file to be locked
//          Use:  char *
//          Type: const char *
//          Acc:  read-only
//          Mech: reference

//
//  Rem:  Locks a file so that only the calling application can use it. All other 
//        calls to this function will wait on lockf() until the locking applicaiton
//        has released the file. Note that this lock only works on NFS; it will not 
//        work on AFS. 
//
//  Side: none.
//
//  Ret:  status from lockf
//
//==============================================================================

int lock_file (const char * lock_file_name)
{
	FILE *fp;
        int lockFileDescriptor;
 
        if (!(fp=fopen(lock_file_name,"a"))) {
	  perror("Can't open locking file for a");
	  exit(1);
	};

        fclose(fp); 

        if((lockFileDescriptor=open(lock_file_name,O_RDWR,0644)) == 0) { 
	  perror("Can't open locking file for rw");
	  exit(1);
	}

        return lockf(lockFileDescriptor,F_LOCK, 0L);

}

