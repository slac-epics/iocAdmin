// MatlabExporter.h

#ifndef __MATLAB_EXPORTER__
#define __MATLAB_EXPORTER__

#include "Exporter.h"

//CLASS MatlabExporter
// Generate a text file that's readable
// by Matlab (version 4.2 or newer).
//
// Implements CLASS Exporter.
class MatlabExporter : public Exporter
{
public:
    MatlabExporter(ArchiveI *archive);
    MatlabExporter(ArchiveI *archive, const stdString &filename);

    virtual void exportChannelList(const stdVector<stdString> &channel_names);

    // convert osiTime into a Matlab 'datestr' that 'datenum' can handle
    enum { DATESTR_LEN=30 };
    static bool epicsTime2datestr(const epicsTime &time, char *text);

    // The other way round.
    // Does only work for datestr of the format
    //     mm/dd/yyyy hh:mm:ss.nano
    // In Matlab, this can be created as:
    // [datestr(n,'mm') '/' datestr(n,'dd') '/' datestr(n,'yyyy')
    //  ' ' datestr(n,'HH:MM:SS')]
    static bool datestr2epicsTime(const char *text, epicsTime &time);

  protected:
};

#endif
