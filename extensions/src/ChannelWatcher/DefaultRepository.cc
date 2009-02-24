//  =================================================================================
//
//   Abs: Use this plug in when you don't require default values for your channels.
//
//   Name: DefaultRepository.cc
//
//   Static functions: none
//   Entry Points:     DefaultRepositoryInfo_ts DefaultRepository::value (char * ChannelName)
//
//   Proto: DefaultRepository.hh
//
//   Auth: 30-Sep-2002, Mike Zelazny (zelazny@slac.stanford.edu):
//   Rev:  DD-MMM-YYYY, Reviewer's Name (.NE. Author's Name)
//
// --------------------------------------------------------------------------------
//
//   Mod: (newest to oldest):
//
// =================================================================================

#include <cadef.h>

#include "Debug.hh"
#include "DefaultRepositoryInfo.hh"
#include "DefaultRepositoryABC.hh"
#include _CW_DEFAULT_REPOSITORY_DEF

DefaultRepository::DefaultRepository (char * name) { // file name for channel list's default values
  }

DefaultRepository::~DefaultRepository () {
  }


// Returns a DefaultRepositoryInfo_ts for the given channel name

DefaultRepositoryInfo_ts DefaultRepository::value (char * ChannelName) {

  DefaultRepositoryInfo_ts DefaultRepositoryInfo_s;
  DefaultRepositoryInfo_s.Value = NULL;
  DefaultRepositoryInfo_s.ValueNative = NULL;
  DefaultRepositoryInfo_s.type = -1;
  DefaultRepositoryInfo_s.count = -1;

  return DefaultRepositoryInfo_s;
  }
