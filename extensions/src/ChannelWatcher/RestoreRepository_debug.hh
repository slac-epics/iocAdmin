// ==============================================================================
//
//  Abs:  Class definition for the example Restore Repository plugin.
//
//  Name: RestoreRepository.hh
//
//  Rem:  Dump Restore Repository to the debug log.
//
//  Auth: 25-Jul-2002, Mike Zelazny (zelazny):
//  Rev:  
//
// ------------------------------------------------------------------------------
//
//  Mod:  (newest to oldest):
//
// ==============================================================================
//
#if !defined( RestoreRepository_debug_hh )

class RestoreRepository : public RestoreRepositoryABC {

public:

RestoreRepository (char * name);  // from -s on the command line
RestoreRepository (FILE * file);  // like cout for example

void put (RestoreRepositoryInfo_ts RestoreRepositoryInfo_s);

~RestoreRepository();

private:

// a copy of the requested name
char * argName;

// the number of times the method put is called
int putCount;

};

#define RestoreRepository_debug_hh
#endif // guard
