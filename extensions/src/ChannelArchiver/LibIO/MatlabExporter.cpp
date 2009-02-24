// $Id: MatlabExporter.cpp,v 1.1.1.1 2005/12/21 16:26:13 luchini Exp $
//
// MatlabExporter.cpp

#ifdef WIN32
#pragma warning (disable: 4786)
#endif

#include"MatlabExporter.h"
#include"ArchiveException.h"
#include"epicsTimeHelper.h"

bool MatlabExporter::epicsTime2datestr(const epicsTime &time, char *text)
{
    int year, month, day, hour, min, sec;
    unsigned long nano;
    
    epicsTime2vals (time, year, month, day, hour, min, sec, nano);
    sprintf(text, "%02d/%02d/%04d %02d:%02d:%02d.%09ld",
            month, day, year, hour, min, sec, nano);
    return true;
}

bool MatlabExporter::datestr2epicsTime(const char *text, epicsTime &time)
{
    int     year, month, day, hour, min;
    double  second;

    if (sscanf(text, "%02d/%02d/%04d %02d:%02d:%lf",
               &month, &day, &year, &hour, &min, &second) != 6)
        return false;

    int secs = (int)second;
    unsigned long nano = (unsigned long) ((second - secs) * 1000000000L);

    vals2epicsTime(year, month, day, hour, min, secs, nano, time);

    return true;
}

MatlabExporter::MatlabExporter(ArchiveI *archive)
        : Exporter(archive)
{
}

MatlabExporter::MatlabExporter(ArchiveI *archive, const stdString &filename)
        : Exporter(archive, filename)
{
}

void MatlabExporter::exportChannelList(
    const stdVector<stdString> &channel_names)
{
    size_t i, ai, count, line, num = channel_names.size();
    Archive         archive(_archive);
    ChannelIterator channel(archive);
    ValueIterator   value(archive);
    epicsTime         time;
    stdString       txt;
    char            datestr[DATESTR_LEN];
    char            info[300];

    if (channel_names.size() > _max_channel_count)
    {
        archive.detach();
        sprintf(info,
                "You tried to export %ld channels.\n"
                "For performance reason you are limited to export %ld "
                "channels at once",
                (long)channel_names.size(), (long)_max_channel_count);
        throwDetailedArchiveException(Invalid, info);
        return;
    }

    // Redirection of output
    FILE *f = stdout; // default: stdout
    if (! _filename.empty())
    {
        f = fopen(_filename.c_str(), "wt");
        if (! f)
        {
            archive.detach();
            throwDetailedArchiveException(WriteError, _filename);
        }
    }

    fputs("% MatLab data file, created by ChannelArchiver.\n", f);
    fputs("% Channels: "  "\n", f);
    for (i=0; i<num; ++i)
        fprintf(f, "%%  %s\n", channel_names[i].c_str());
    fputs("%\n", f);
    fputs("% Struct: t - time string\n", f);
    fputs("%         v - value\n", f);
    if (_show_status)
        fputs("%         s - status\n", f);
    fputs("%         d - date number\n", f);
    fputs("%         l - length of data\n", f);
    fputs("%         n - name\n", f);
    fputs("%\n", f);
    fputs("% Example for generic plot func. that can handle this data:\n", f);
    fputs("%\n", f);
    fputs("%  function archdataplot(data)\n", f);
    fputs("%\n", f);
    fputs("%  plot(data.d, data.v), f);\n", f);
    fputs("%  datetick('x');\n", f);
    fputs("%  xlabel([data.t{1} ' - ' data.t{data.l}]);\n", f);
    fputs("%  title(data.n);\n", f);
    fputs("%\n", f);
    fputs("% For the of data.v being an array, try the 'mesh' function.\n", f);
    
    for (i=0; i<num; ++i)
    {
        if (! archive.findChannelByName(channel_names[i], channel))
        {
            fprintf(f, "%% Cannot find channel '%s' in archive\n",
                    channel_names[i].c_str());
            continue;
        }
        if (! channel->getValueBeforeTime(_start, value) &&
            ! channel->getValueAfterTime(_start, value))
        {
            fprintf(f, "%% No values found for channel '%s'\n",
                    channel_names[i].c_str());
            continue;
        }
        
        char *p, *variable = strdup(channel_names[i].c_str());
        bool fixed_name = false;
        while ((p=strchr(variable, ':')) != 0)
        {
            *p = '_';
            fixed_name = true;
        }
        if (fixed_name)
            fprintf(f, "%% Used '%s' for channel '%s'\n",
                    variable, channel_names[i].c_str());
        
        // Value loop per channel
        line = 0;
        count = value->getCount();
        while (value)
        {
            time=value->getTime();
            ++line;
            ++_data_count;
            epicsTime2datestr(time, datestr);
            fprintf(f, "%s.t(%zu)={'%s'};\n", variable, line, datestr);

            if (value->isInfo())
            {
                if (count > 1)
                {
                    fprintf(f, "%s.v(:,%zu)=[", variable, line);
                    for (ai=0; ai<count; ++ai)
                        fprintf(f, "nan ");
                    fprintf(f, "];\n");
                }
                else
                    fprintf(f, "%s.v(%zu)=nan;\n", variable, line);
            }
            else
            {
                if (count == 1)
                    fprintf(f, "%s.v(%zu)=%g;\n",
                            variable, line, value->getDouble());
                else
                {
                    fprintf(f, "%s.v(:,%zu)=[", variable, line);
                    for (ai=0; ai<count; ++ai)
                        fprintf(f, "%g ", value->getDouble(ai));
                    fprintf(f, "];\n");
                }
            }
            
            if (_show_status)
            {
                value->getStatus (txt);
                fprintf(f, "%s.s(%zu)={'%s'}",
                        variable, line, txt.c_str());
            }
            // Show one value after _end, then quit:
            if (isValidTime(_end) && time >= _end)
                break;
            ++value;
        }

        fprintf(f, "%s.d=datenum(char(%s.t));\n", variable, variable);
        fprintf(f, "%s.l=size(%s.v, 2);\n", variable, variable);
        fprintf(f, "%s.n='%s';\n", variable, variable);
        free(variable);
    }

    if (f != stdout)
        fclose(f);
    archive.detach();
}

