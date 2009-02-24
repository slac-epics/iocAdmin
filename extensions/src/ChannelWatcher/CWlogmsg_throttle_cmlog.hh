// ==============================================================================
//
//  Abs:  Class definition for cmlog throttling. Subclass of 
//        CWlogmsg_throttle_cmlogABC.
//
//  Name: CWlogmsg_throttle_cmlog.hh
//
//  Prev: CWlogmsg_throttleABC.hh
//        CWlogmsg_throttle_cmlogABC.hh
//
//  Rem:  Implement this class to use cmlog throttling for the CWlogmsg_cmlog plugin.
//
//  Auth: 21-Mar-2002, James Silva (jsilva@slac.stanford.edu):
//
//  Rev:  
//        10-Oct-2002, James Silva (V2.00)
//         Added declaration of checkThrottleStatus().
//
// --------------------------------------------------------------------------------
//
//  Mod:  (newest to oldest):
//
// ==============================================================================

#if !defined( CWlogmsg_throttle_cmlog_hh )

// structure for managing linked list of throttle strings
typedef struct {
   char * name;
   void * next_p;
   } ThrottleStringList_ts;

class CWlogmsg_throttle_cmlog : public CWlogmsg_throttle_cmlogABC {

public:  

  CWlogmsg_throttle_cmlog(); 

// initialize throttle using optional pointer to logging client
  void init(void * client);

// set the string to be throttled(text), number of strings(limit) per time interval(time)
  int setThrottle(const char * text, const int limit, const double time);

// implement this for special Channel throttling functionality
  int setThrottleChannel(const char * text, 
                         const int limit, 
                         const double time);

// turn throttling on; only use after call to throttleOff()
  void throttleOn();

// turn throttling off
  void throttleOff();

// show queue of throttle strings
  void throttleShow();

// send throttle status messages to CMLOG, if necessary
  void checkThrottleStatus();

  ~CWlogmsg_throttle_cmlog();

// throttle string list management methods

private:

// checks whether a given string token exists in the throttle list  
  int stringInThrottleList(const char * token); 

// adds a string to the list of previously entered throttle strings
  void addStringToThrottleList(const char * token);

};


#define CWlogmsg_throttle_cmlog_hh
#endif // guard
