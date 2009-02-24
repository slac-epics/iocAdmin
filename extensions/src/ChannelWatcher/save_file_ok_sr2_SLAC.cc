//  =================================================================================
//
//   Abs: Used to determine whether a SLAC s/r V2.00 save file 1-exists and 2-has eof token
//
//   Name: save_file_ok_sr2_SLAC.cc
//
//   Static functions: none
//   Entry Points: int save_file_ok (name)
//
//   Proto: save_file_ok_sr2_SLAC.hh
//
//   Auth: 11-Nov-2002, Mike Zelazny (zelazny@slac.stanford.edu):
//   Rev:  DD-MMM-YYYY, Reviewer's Name (.NE. Author's Name)
//
// --------------------------------------------------------------------------------
//
//   Mod: (newest to oldest):
//
// =================================================================================

#include <stdio.h>                       // for FILE (linux)
#include <string.h>                      // for strstr

#include <iostream.h>                    // for FILE

#include "ChannelWatcher.hh"
#include "Debug.hh"
#include "save_file_ok_sr2_SLAC.hh"

#define file_ok_tag "FILE-WRITES-COMPLETED-SUCCESSFULLY"

// =============================================================================
//
//   Abs:  Used to determine whether a SLAC s/r V2.00 save file 1-exists and 2-has eof token
//
//   Name: save_file_ok
//
//   Args: name                   File name of default values
//          Use:  char-string
//          Type: char *
//          Acc:  read-only
//          Mech: reference
//
//   Rem:  Tries to open the given name; if successful looks for the file_ok_tag
//
//   Side: opens and closes a file, need file descriptor.
//
//   Ret:  0 when file is corrupt or missing; 1 when file seems ok.
//
// ================================================================================

int save_file_ok (char * name) {

  char line[130];
  CLEAR(line);
  int status = 0;

  // Make sure file exists
  FILE * file = fopen (name, "r");
  if (NULL == file) return 0;

  // Make sure it contains the EOF token
  while (fgets (line, sizeof(line), file)) {
    if (strstr (line, file_ok_tag)) status = 1;
    CLEAR (line);
    };

  fclose (file);

  return status;
  }
