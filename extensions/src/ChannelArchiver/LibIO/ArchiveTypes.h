// --------------------------------------------------------
// $Id: ArchiveTypes.h,v 1.1.1.1 2004/04/01 20:49:40 luchini Exp $
//
// Please refer to NOTICE.txt,
// included as part of this distribution,
// for legal information.
//
// Kay-Uwe Kasemir, kasemir@lanl.gov
// --------------------------------------------------------

#ifndef __ARCHIVE_TYPES_H__
#define __ARCHIVE_TYPES_H__

#include <math.h>
#include <db_access.h>
#include <epicsTime.h>
#include <MsgLogger.h>
#include "ArchiverConfig.h"

typedef char Byte;
typedef unsigned long FileOffset;
typedef unsigned short DbrType;
typedef unsigned short DbrCount;

#ifdef CONVERSION_REQUIRED
#include<osiSock.h>

inline void ULONGFromDisk(unsigned long &item)
{	item = ntohl (item);	}

inline void ULONGToDisk(unsigned long &item)
{	item = htonl (item);	}

inline void USHORTFromDisk(unsigned short &item)
{	item = ntohs (item);	}

inline void USHORTToDisk(unsigned short &item)
{	item = htons (item);	}

inline void DoubleFromDisk(double &d)
{
    dbr_long_t  cvrt_tmp = ntohl(((dbr_long_t *)&d)[0]);
    ((dbr_long_t *)&d)[0] = ntohl(((dbr_long_t *)&d)[1]);
    ((dbr_long_t *)&d)[1] = cvrt_tmp;
}

inline void DoubleToDisk(double &d)
{
    dbr_long_t  cvrt_tmp = htonl(((dbr_long_t *)&d)[0]);
    ((dbr_long_t *)&d)[0] = htonl(((dbr_long_t *)&d)[1]);
    ((dbr_long_t *)&d)[1] = cvrt_tmp;
}

inline void FloatFromDisk(float &d)
{   *((dbr_long_t *)&d) = ntohl(*((dbr_long_t *)&d)); }

inline void FloatToDisk(float &d)
{   *((dbr_long_t *)&d) = htonl(*((dbr_long_t *)&d)); }

inline void epicsTimeStampFromDisk(epicsTimeStamp &ts)
{
	ts.secPastEpoch = ntohl(ts.secPastEpoch);
	ts.nsec = ntohl(ts.nsec);
}

inline void epicsTimeStampToDisk(epicsTimeStamp &ts)
{
	ts.secPastEpoch = ntohl(ts.secPastEpoch);
	ts.nsec = ntohl(ts.nsec);
}

#define SHORTFromDisk(s)	USHORTFromDisk((unsigned short &)s)
#define SHORTToDisk(s)		USHORTToDisk((unsigned short &)s)
#define LONGFromDisk(l)		ULONGFromDisk((unsigned long &)l)
#define LONGToDisk(l)		ULONGToDisk((unsigned long &)l)

#else

#define ULONGFromDisk(s) {}
#define ULONGToDisk(i)   {}
#define USHORTFromDisk(i) {}
#define USHORTToDisk(i)   {}
#define DoubleFromDisk(i) {}
#define DoubleToDisk(i)   {}
#define FloatFromDisk(i)   {}
#define FloatToDisk(i)   {}
#define epicsTimeStampFromDisk(i) {}
#define epicsTimeStampToDisk(i) {}
#define SHORTFromDisk(i) {}
#define SHORTToDisk(i) {}
#define LONGFromDisk(i) {}
#define LONGToDisk(i) {}

#endif // CONVERSION_REQUIRED

// used internally for offsets inside files:
const FileOffset INVALID_OFFSET = 0xffffffff;
#define FileOffsetFromDisk ULONGFromDisk
#define FileOffsetToDisk ULONGToDisk

// Safely (i.e. w/o overruns and '\0'-limited)
// copy a std-string into a char [].
//
// string::copy isn't available on all platforms, so strncpy is used.
inline void string2cp(char *dest, const stdString &src, size_t maxlen)
{
	if (src.length() >= maxlen)
	{
		LOG_MSG("string2cp: Truncating '%s' to %d chars.\n",
                src.c_str(), maxlen);
		strncpy(dest, src.c_str(), maxlen);
		dest[maxlen-1] = '\0';
	}
	else
		strncpy(dest, src.c_str(), maxlen);
}

inline bool isValidFilename(const stdString &name)
{	return ! name.empty();	}

inline bool isValidFilename(const char *name)
{	return name[0] != '\0';	}

#endif















