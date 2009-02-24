// ==============================================================================
//
//  Abs:  Class definition for cmlog throttling. Subclass of CWlogmsg_throttleABC.
//
//  Name: CWlogmsg_throttle.hh
//
//  Prev: CWlogmsg_throttleABC.hh
//
//  Rem:  Implement this class to use throttling for CWlogmsg plugins.
//
//  Auth: 21-Mar-2002, James Silva (jsilva@slac.stanford.edu):
//
//  Rev:  
//
// --------------------------------------------------------------------------------
//
//  Mod:  (newest to oldest):
//        10-Oct-2002, James Silva (V2.00)
//         Added declaration of checkThrottleStatus().
//
// ==============================================================================

#if !defined( CWlogmsg_throttle_hh )

class CWlogmsg_throttle : public CWlogmsg_throttleABC {

public:

  // initialize throttle using optional pointer to logging client
  void init (void * client = NULL);

  // set the string to be throttled(text), number of strings(limit) per time interval(time)
  int setThrottle (const char * token, const int limit, const double time);

  // implement this for special Channel throttling functionality
  int setThrottleChannel (const char * token, const int limit, const double time);

  // turn throttling on; only use after call to throttleOff()
  void throttleOn ();

  // turn throttling off
  void throttleOff ();

  // show queue of throttle strings
  void throttleShow ();

  // output throttle status messages, if necessary
  void checkThrottleStatus();

};


#define CWlogmsg_throttle_hh
#endif // guard
