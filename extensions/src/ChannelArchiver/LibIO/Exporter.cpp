// $Id: Exporter.cpp,v 1.1.1.1 2004/04/01 20:49:43 luchini Exp $
//
// Exporter.cpp
//
// Base class for tools that export data from a ChannelArchive
//

#ifdef WIN32
#pragma warning (disable: 4786)
#endif

#include"Exporter.h"
#include"ArchiveException.h"
#include"epicsTimeHelper.h"
#include<cvtFast.h>

Exporter::Exporter(ArchiveI *archive)
{
    init(archive);
}

Exporter::Exporter(ArchiveI *archive, const stdString &filename)
{
    init(archive);
    _filename = filename;
}

void Exporter::init(ArchiveI *archive)
{
    _archive = archive;
    _undefined_value = "#N/A";
    _show_status = false;
    _linear_interpol_secs = 0.0;
    _gap_factor = 0;
    _fill = false;
    _be_verbose = false;
    _is_array = false;
    _max_channel_count = 100;
    _data_count = 0;
}

void Exporter::exportMatchingChannels(const stdString &channel_name_pattern)
{
    // Find all channels in archive that match a given pattern.
    // If pattern is empty,
    // all channels will be listed.
    stdVector<stdString> channel_names;

    ChannelIteratorI *channel = _archive->newChannelIterator();
    _archive->findChannelByPattern(channel_name_pattern, channel);
    while (channel->isValid())
    {
        channel_names.push_back(channel->getChannel()->getName ());
        channel->next();
    }
    delete channel;

    exportChannelList(channel_names);
}

void Exporter::printTime(FILE *f, const epicsTime &time)
{
    int year, month, day, hour, min, sec;
    unsigned long nano;
    epicsTime2vals (time, year, month, day, hour, min, sec, nano);

    fprintf(f, "%02d/%02d/%04d %02d:%02d:%02d.%09ld",
            month, day, year, hour, min, sec, nano);
}

void Exporter::printValue(FILE *f, const epicsTime &time, const ValueI *v)
{
    // skip values which are Archiver specials
    size_t ai;
    stdString txt;
    char buf[100];
    unsigned short precision;
    
    if (v->isInfo())
    {
        for (ai=0; ai<v->getCount(); ++ai)
            fprintf(f, "\t%s", _undefined_value.c_str());
    }
    else
    {
        const CtrlInfo *info = v->getCtrlInfo();
        
        // Format according to precision.
        // Unfortuately that is usually configured wrongly
        // and then people complain about not seeing their data...
        // -> use prec. if > 0
        if (v->getType() == DBR_TIME_STRING  ||
            (info && info->getType() == CtrlInfo::Enumerated))
        {
            v->getValue(txt);
            fprintf(f, "\t%s", txt.c_str());
        }
        else
        {
            if (info && info->getPrecision() > 0)
                precision = info->getPrecision();
            else
                precision = 6;
            for (ai=0; ai<v->getCount(); ++ai)
            {
                cvtDoubleToString(v->getDouble(ai), buf, precision);
                fprintf(f, "\t%s", buf);
            }
        }
    }
    
    if (_show_status)
    {
        v->getStatus(txt);
        if (txt.empty())
            fprintf(f, "\t ");
        else
            fprintf(f, "\t%s", txt.c_str());
    }
}

