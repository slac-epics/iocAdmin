// ==============================================================================
//
//  Abs:  Abstract base class definition for the CWlogmsg_throttle plugins.
//
//  Name: CWlogmsg_throttleABC.hh
//
//  Prev: none
//
//  Rem:  Inherit this class when creating your own CWlogmsg_throttle plugin. 
//
//  Auth: 29-Mar-2002, James Silva (jsilva@slac.stanford.edu):
//
//  Rev:  
//
// --------------------------------------------------------------------------------
//
//  Mod:  (newest to oldest):
//
//        27-Jan-2003, James Silva (V2.02)
//         Removed definition of NUM_MSGS_PER_THROTTLE_INTERVAL 
//        10-Oct-2002, James Silva (V2.00)
//         Added declaration of checkThrottleStatus().
// ==============================================================================
//
#if !defined( CWlogmsg_throttleABC_hh )



class CWlogmsg_throttleABC {

public:

 // initialize throttle using optional pointer to logging client
  virtual void init (void * client = NULL) = 0;

  // set the string to be throttled(text), number of strings(limit) per time interval(time)
  virtual int setThrottle (const char * token, const int limit, const double time) = 0;
  
  // special throttling functionlity for Channels
  virtual int setThrottleChannel (const char * text, 
                                  const int limit, 
                                  const double time) = 0;

  // turn throttling on; only use after call to throttleOff()
  virtual void throttleOn () = 0;

  // turn throttling off
  virtual void throttleOff () = 0;

  // show queue of throttle strings
  virtual void throttleShow () = 0;

  // force output of throttle status messages, if necessary
  virtual void checkThrottleStatus() = 0;


};


#define CWlogmsg_throttleABC_hh
#endif // guard
