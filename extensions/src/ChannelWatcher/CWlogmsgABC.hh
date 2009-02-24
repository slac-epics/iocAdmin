// ==============================================================================
//
//  Abs:  Abstract base class definition for the CWlogmsg plugins.
//
//  Name: CWlogmsgABC.hh
//
//  Prev: tsDefs.h for time format, db_access.h for sb string variables
//
//  Rem:  Choose between these defines when calling CWlogmsg.  
//        Implement this class when creating your own CWlogmsg plugin.
//
//  Auth: 13-Dec-2001, James Silva (jsilva@slac.stanford.edu):
//
//  Rev:  
//
// --------------------------------------------------------------------------------
//
//  Mod:  (newest to oldest):
//
//        12-Dec-2002, Mike Zelazny (V2.01)
//          Added IOC vs UNIX time message
//        11-Oct-2002, James Silva (V2.00)
//         Added declaration of setThrottleLimit(int).
//        10-Oct-2002, James Silva (V2.00)
//         Added declaration of checkThrottleStatus().
// ==============================================================================
//
#if !defined( CWlogmsgABC_hh )

#define INFO 1               // to log informational messages
#define VALUE_CHANGE 2       // Channel value has changed
#define CONNECTION_CHANGE 3  // Channel connection status has changed for a channel
#define LOG_CHANGE 4         // Logging a channel has started or stopped
#define WRITE_CHANGE 5       // Restore Repository generation has changed for a channel
#define AVAIL_CHANGE 6       // A channel has been added or removed from a Channel List
#define RATE_CHANGE 7        // Min time between restore repository generation has changed for a channel
#define IO_ERROR 8           // File read/write error  
#define FILE_NAME_ERROR 9    // Unable to find file
#define CA_ERROR 10          // Fatal Channel Access error
#define THROTTLE_ERROR 11    // Message throttle setup errors
#define TIME_ERROR 12        // IOC time and UNIX time are too different!

#define LAST_MESSAGE 13      // Make this one more than the last message code.
                             // And please don't skip any numbers.

// Choose your EPICS time format for message logging, if any.
#define TIME_FORMAT TS_TEXT_MONDDYYYY

class CWlogmsgABC {

public:

// log a message give code from above and text, usually for informational messages

virtual int log (const int code, const char * text) = 0; 


// for messages about channels

virtual int log (const int code,                 // from CWlogmsgABC.hh
                 const char * text,              //
                 const char * host,              // optional IOC name
                 const char * device,            // channel name
                 const char * alias,             // optional channel name alias from your Channel Group
                 const TS_STAMP * tstamp = NULL  // optional time stamp, will use current time if not supplied
                 ) = 0;


// for messages about channels and their values

virtual int log (const int code,
                 const char * text,
                 const char * host,
                 const char * device,
                 const char * alias,
                 const dbr_short_t severity, // from channel access
                 const dbr_short_t stat,     // from channel access
                 const dbr_string_t value,   // from channel access
                 const TS_STAMP * tstamp
                 ) = 0;

// optional tag placed at the beginning of all of your log "text"s
// useful when running multiple Channel Watchers and logging to the
// same repository such as cmlog.

virtual void setTag (char * argTag) = 0;

// if throttling is used or implemented, use this function to set 
// the throttling time interval for messages. Longer times are useful
// for limiting output by "noisy" channels.

virtual void setThrottleTime(double argTime) = 0;

// if throttling is used or implemented, use this function to set 
// the throttling limit of messages per time interval. 

virtual void setThrottleLimit(int argLimit) = 0;

// force output of throttle status messages, if necessary

virtual void checkThrottleStatus() = 0;

Debug debug;

};

#define CWlogmsgABC_hh
#endif // guard
