//==============================================================================
//
//  Abs:  Contains definitions used by all Channel Watcher files.
//
//  Name: ChannelWatcher.hh
//
//  Rem:  All Channel Watcher files should include this file.
//        See http://www.slac.stanford.edu/comp/unix/package/epics/extensions/ChannelWatcher/ChannelWatcher.html
//
//  Auth: 05-Nov-2001, Mike Zelazny (zelazny@slac.stanford.edu):
//  Rev:  dd-mmm-19yy, Tom Slick (TXS):
//
//--------------------------------------------------------------------------------
//
//  Mod:
//        13-Jan-2007, Mike Zelazny (V2.14)
//          Fix bugs in POSTERROR & LASTMSG pv processing and "flag" processing, 
//          such as /log.  Faster waveform default values.
//        22-Jan-2007, Mike Zelazny (V2.13)
//          Ability to log char waveforms.
//        07-Dec-2006, Mike Zelazny (V2.12)
//          Changes for RHEL4.
//        23-May-2006, Mike Zelazny (V2.11)
//          Support "" ca_host_name.
//        03-Apr-2005, Mike Zelaznt (V2.10)
//          uwd option for CA monitor callbacks.
//        04-Aug-2005, Mike Zelazny (V2.09)
//          Additional cmlog options
//        07-Apr-2005, Mike Zelazny (V2.08)
//          Add /anyChange flag (modifier on /log flag) for spear.
//        05-Apr-2005, Mike Zelazny (V2.07)
//          Fix bug in /NOWRITE vs /NOSCPCFG flag.
//        14-Oct-2004, Mike Zelazny (V2.06)
//          Mods for soft IOCs.
//        28-Apr-2004, Mike Zelazny (V2.05)
//          Mods for R3.14.5 build.
//        11-Oct-2002, James Silva (V2.00)
//          Added DefaultThrottleMessageLimit definintion.
//        08-Jul-2002, Mike Zelazny (V1.15)
//          Convert to use Restore Repository plug-in.
//        26-Jun-2002, Mike Zelazny (V1.14)
//          Convert to use Channel Group plug-in
//        28-Jan-2002, James Silva (V1.10)
//          Removed .hh includes and moved them to ChannelWatcher.cc
//        09-Jan-2002, James Silva, (V1.10)
//          Changed declaration of static char variables to remove warnings. 
//        08-Jan-2002, James Silva (jsilva@slac.stanford.edu) (V1.10)
//          Added static message variable for CWlogmsg function calls. 
//        15-Nov-2001, Mike Zelazny (V1.06)
//          Only try to connect to channel once.
//        15-Nov-2001, Mike Zelazny (V1.05)
//          Added REQconnectSeconds and MAXconnectAttempts
//
//==============================================================================

#if !defined(ChannelWatcher_hh)

// Channel Watcher facility
#define FACILITY (char *) "Channel Watcher"

// Channel Watcher version
static char * CWversion = (char*) "V2.14";

static char message[256];

// Classic MIN and MAX macros
#if !defined(MIN)
#define MIN(a,b) (a)<(b)?(a):(b)
#endif

#if !defined(MAX)
#define MAX(a,b) (a)<(b)?(b):(a)
#endif

// For clearing out allocated structures
#define CLEAR(a) { memset (a, 0, sizeof(a)); }

#define NEW_LINE 0x0A
#define CLEAR_NL(a) { if (NEW_LINE == a[strlen(a)-1]) a[strlen(a)-1] = 0; }

// For cleaning out blanks at the end of a token
#define CLEAR_BL(a) { while (' ' == a[strlen(a)-1]) a[strlen(a)-1] = 0; }

// Required minimum number of seconds between Restore Repository Generation
#define REQminSeconds 10.0

// Default throttle message limit per time interval
#define DefaultThrottleMessageLimit 2

// Channel Log flags
#define NO_LOGGING 0
#define LOG_ON_VALUE_CHANGE 1
#define LOG_ANY_CHANGE 3

#define ChannelWatcher_hh
#endif /* guard */
