// ==============================================================================
//
//  Abs:  Class definition for the basic Restore Repository plugin.
//
//  Name: RestoreRepository.hh
//
//  Rem:  For use when your derived class generally involves writing files.
//
//  Auth: 08-Jul-2002, Mike Zelazny (zelazny):
//  Rev:  
//
// ------------------------------------------------------------------------------
//
//  Mod:  (newest to oldest):
//
// ==============================================================================
//
#if !defined( RestoreRepository_file_hh )

class RestoreRepository : public RestoreRepositoryABC {

public:

RestoreRepository (char * name);    // from -s on the command line
RestoreRepository (FILE * argFile); // for say, cout

void put (RestoreRepositoryInfo_ts RestoreRepositoryInfo_s);

~RestoreRepository();

private:

// file descriptor
FILE * File;

// a copy of the requested name
char * argName;

// File Name actually used.  May be different than the argName.
char * FileName;

};

#define RestoreRepository_file_hh
#endif // guard
