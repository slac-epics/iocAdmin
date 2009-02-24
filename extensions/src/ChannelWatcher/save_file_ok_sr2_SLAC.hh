// ==============================================================================
//
//  Abs:  Function prototype for save_file_ok for SLAC's s/r V2.00 file format
//
//  Name: save_file_ok_sr2_SLAC.hh
//
//  Rem:  Used by ChannelGroup_sr2_SLAC.cc to determine where to get default
//        channel values; and used by RestoreRepository_sr2_SLAC.cc to determine
//        whether the .savB file is good enough to become the .sav file.
//
//  Auth: 11-Nov-2002, Mike Zelazny (zelazny@slac.stanford.edu):
//  Rev:
//
// --------------------------------------------------------------------------------
//
//  Mod: (newest to oldest):
//
// ==============================================================================
//
#if !defined( save_file_ok_hh )

int save_file_ok (char * name);

#define save_file_ok_hh
#endif // guard
