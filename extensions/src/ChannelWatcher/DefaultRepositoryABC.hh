// ==============================================================================
//
//  Abs:  Abstract base class definition for the Default Channel Values plugins.
//
//  Name: DefaultRepositoryABC.hh
//
//  Rem:  Implement these functions to get the chosen Channel Group to use 
//        your specific list of channel's default values.
//
//  Auth: 07-Aug-2002, Mike Zelazny (zelazny):
//  Rev:  
//
// ------------------------------------------------------------------------------
//
//  Mod:  (newest to oldest):
//        10-Jul-2002, Mike Zelazny (V1.16)
//          Convert channel value to native data type
//
// ==============================================================================
//
#if !defined( DefaultRepositoryABC_hh )

class DefaultRepositoryABC {

public:

//
// Your constructor will require a name, for example:
// DefaultRepository (char * name)        // from -s on the command line
//                       

// Returns a DefaultRepositoryInfo_ts for the given channel name
virtual DefaultRepositoryInfo_ts value (char * ChannelName) = 0;

Debug debug;

};

#define DefaultRepositoryABC_hh
#endif // guard
