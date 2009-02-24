// ==============================================================================
//
//  Abs:  Abstract base class definition for the CWlogmsg_throttle_cmlog plugins. 
//        Subclass of CWlogmsg_throttleABC class.
//
//  Name: CWlogmsg_throttle_cmlogABC.hh
//
//  Prev: CWlogmsg_throttleABC.hh
//
//  Rem:  Inherit this class when creating your own CWlogmsg_cmlog_throttle plugin. 
//
//  Auth: 21-Mar-2002, James Silva (jsilva@slac.stanford.edu):
//
//  Rev:  
//
// --------------------------------------------------------------------------------
//
//  Mod:  (newest to oldest):
//
// ==============================================================================
#if !defined( CWlogmsg_throttle_cmlogABC_hh )


class CWlogmsg_throttle_cmlogABC : public CWlogmsg_throttleABC {

public:
  
  // initialize throttle using optional pointer to logging client
  virtual void init (void * client) = 0;

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

  // send throttle status messages to CMLOG, if necessary
  virtual void checkThrottleStatus() = 0;

};


#define CWlogmsg_throttle_cmlogABC_hh
#endif // guard
