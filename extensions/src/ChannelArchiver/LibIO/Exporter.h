// Exporter.h -*- c++ -*-

#ifndef __EXPORTER_H__
#define __EXPORTER_H__

#include"ArchiveI.h"
#include<stdio.h>

//CLASS Exporter
// Exporter is the virtual base class for
// tools that export data from a ChannelArchive:
// <UL>
// <LI>CLASS SpreadSheetExporter
// <LI>CLASS GNUPlotExporter
// <LI>CLASS MatlabExporter
// </UL>
//
// These classes export the values of selected channels
// as text files.
// Each derived class generates a slightly different
// type of text file for the specific target application.
class Exporter
{
public:
    //* All Exporters work on an open Archive.
    // <I>filename</I> is the (base)name
    // for the generated file.
    // When left empty, <I>cout</I> is used.
    //
    // See also: CLASS FilenameTool
    Exporter(ArchiveI *archive);
    Exporter(ArchiveI *archive, const stdString &filename);

    virtual ~Exporter() {}

    //* Allowed number of channels to export
    // (limited for performance reasons)
    void setMaxChannelCount(size_t limit);

    //* Set start/end time. Default: dump from whole archive
    void setStart(const epicsTime &start);
    void setEnd(const epicsTime &end);

    //* Switch on linear interpolation,
    // generating a value every "secs".
    // Gaps bigger than "secs * gap" will be
    // shown as "Archive_Off"
    // to avoid endless interpolation.
    void setLinearInterpolation(double secs, size_t gap = 0);

    //* When using filled values,
    // missing entries (when the value has not changed since the last entry)
    // will be filled in by repeating the last value
    void useFilledValues();

    void setVerbose(bool verbose = true)
    {   _be_verbose = verbose; }

    //* Generate extra column for each channel
    // with status information?
    void enableStatusText(bool yesno = true);

    //* Export channels that match a name pattern
    void exportMatchingChannels(const stdString &channel_name_pattern);

    //* Export channels from provided list
    // Derived class has to implement this.
    // The protected members can be used
    // but it's up to the implementation as to how.
    virtual void exportChannelList(
        const stdVector<stdString> &channel_names) = 0;

    size_t getDataCount();
    
protected:
    ArchiveI *_archive;
    stdString _filename;
    epicsTime _start, _end;
    double _linear_interpol_secs;
    size_t _gap_factor;
    bool _fill;
    bool _be_verbose;

    // String to be used for undefined values
    stdString _undefined_value;
    // Show status text in seperate column
    bool _show_status;

    bool _is_array;
    size_t _max_channel_count;
    size_t _data_count;

    // Helpers for derived classes:
    // Print time in some human readable format
    void printTime(FILE *f, const epicsTime &time);
    // Print value, handling status values (_show_status)
    // as well as arrays.
    // Uses CtrlInfoI->getPrecision() if > 0.
    void printValue(FILE *f, const epicsTime &time, const ValueI *v);

private:
    void init(ArchiveI *archive);
};

inline void Exporter::setStart(const epicsTime &start) { _start = start; }
inline void Exporter::setEnd(const epicsTime &end)     { _end = end; }
inline void Exporter::enableStatusText(bool yesno)   { _show_status = yesno; }

inline void Exporter::setLinearInterpolation(double secs, size_t gap)
{
    _fill = false;
    _linear_interpol_secs = secs;
    _gap_factor = gap;
}

inline void Exporter::useFilledValues()
{   
    _fill = true;
    _linear_interpol_secs = 0.0;
}

inline void Exporter::setMaxChannelCount(size_t limit)
{   _max_channel_count = limit; }

inline size_t Exporter::getDataCount()
{   return _data_count; }

#endif //__EXPORTER_H__
