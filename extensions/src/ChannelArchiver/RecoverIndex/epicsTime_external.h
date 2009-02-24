
typedef struct epicsTimeStamp {
    uint32_t secPastEpoch;   /* seconds since 0000 Jan 1, 1990 */
    uint32_t nsec;           /* nanoseconds within second */
} epicsTimeStamp;

struct local_tm_nano_sec {
    struct tm ansi_tm; /* ANSI C time details */
    unsigned long nSec; /* nano seconds extension */
};

struct timespec {
  unsigned long  tv_sec;         /* seconds */
  long    tv_nsec;        /* and nanoseconds */  
};/* POSIX real time */


struct timeval {
  long	tv_sec;		/* seconds */
  long	tv_usec;	/* and microseconds */
};/* BSD */

class  epicsTime{
 public:
  epicsTime ();
  epicsTime ( const epicsTime & t );
  
  /* convert to and from EPICS epicsTimeStamp format */
  epicsTime ( const epicsTimeStamp & ts );
  epicsTime ( const struct timespec & );
  epicsTime ( const struct timeval & );
  
  /* dump current state to standard out */
  void show ( unsigned interestLevel ) ;
};
