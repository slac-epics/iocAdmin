// ==============================================================================
//
//  Abs:  Abstract base class definition for Restore Repository plugins.
//
//  Name: RestoreRepositoryABC.hh
//
//  Rem:  Implement these functions to get the Channel Watcher to use your
//        specific Restore Repository format or database.
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

#if !defined( RestoreRepositoryABC_hh )

class RestoreRepositoryABC {

public:

//
// Your constructor will require a name, for example:
// RestoreRepository (char * name)  // from -s on the command line
//

// "put" gets called once for each channel in your restore repository
virtual void put (RestoreRepositoryInfo_ts RestoreRepositoryInfo_s) = 0;

// your destructor WILL get called for each new restore repository generation
// ~RestoreRepository() {...}

Debug debug;

};

#define RestoreRepositoryABC_hh
#endif // guard
