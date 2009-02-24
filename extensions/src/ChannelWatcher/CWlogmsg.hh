// ==============================================================================
//
//  Abs:  Class definition for Channel Watcher message logging.
//
//  Name: CWlogmsg.hh
//
//  Rem:  Prototypes for implementing CWlogmsg abstract base class.
//
//  Auth: 13-Dec-2001, James Silva (jsilva@slac.stanford.edu):
//  Rev:  
//
// --------------------------------------------------------------------------------
//
//  Mod: (newest to oldest):
//        11-Oct-2002, James Silva (V2.00)
//         Added declaration of setThrottleLimit(int).
//        10-Oct-2002, James Silva (V2.00)
//         Added declaration of checkThrottleStatus().
//        03-Jan-2002, James Silva (V1.10):
//          Changed constructors for CWlogmsg to "log" methods
//
// ==============================================================================
//
#if !defined( CWlogmsg_hh )

class CWlogmsg : public CWlogmsgABC {

public:

CWlogmsg();

// log a message give code from CWlogmsgABC.hh and text, usually for informational messages
int log (const int code, const char * text); 

// for messages about channels
int log (const int code,                 // from CWlogmsgABC.hh
         const char * text,              //
         const char * host,              // IOC name
         const char * device,            // channel name
         const char * alias,             // optional channel name alias from your Channel Group
         const TS_STAMP * tstamp = NULL  // optional time stamp, will use current time if not supplied
         );

// for messages about channels and their values
int log (const int code, 
         const char * text, 
         const char * host, 
         const char * device, 
         const char * alias, 
         const dbr_short_t severity, // from channel access
         const dbr_short_t stat,     // from channel access
         const dbr_string_t value,   // from channel access
         const TS_STAMP * tstamp
         );

// optional tag placed at the beginning of all of your log "text"s
// useful when running multiple Channel Watchers and logging to the
// same repository such as cmlog.

void setTag(char * argTag);

// if throttling is used or implemented, use this function to set 
// the throttling time interval for messages. Longer times are useful
// for limiting output by "noisy" channels.

void setThrottleTime(double argTime);


// if throttling is used or implemented, use this function to set 
// the throttling limit of messages per time interval. 

void setThrottleLimit(int argLimit);

// force output of throttle status messages

void checkThrottleStatus();


~CWlogmsg();

};

#define CWlogmsg_hh
#endif // guard
