// SpreadSheetExporter.cpp

#include "SpreadSheetExporter.h"
#include "ArchiveException.h"
#include "LinInterpolValueIteratorI.h"
#include "ExpandingValueIteratorI.h"
#include "epicsTimeHelper.h"

SpreadSheetExporter::SpreadSheetExporter(ArchiveI *archive)
        : Exporter(archive)
{
    _use_matlab_format = false;
}

SpreadSheetExporter::SpreadSheetExporter(ArchiveI *archive,
                                         const stdString &filename)
        : Exporter(archive, filename)
{
    _use_matlab_format = false;
}

// Loop over current values and find oldest
static epicsTime findOldestValue(ValueIteratorI *values[], size_t num)
{
    size_t i;
    epicsTime first, t;
    for (i=0; i<num; ++i) // get first valid time
    {
        if (values[i]->isValid())
        {
            first = values[i]->getValue()->getTime();
            break;
        }
    }
    for (++i; i<num; ++i) // see if anything is older
    {
        if (values[i]->isValid())
        {
            t = values[i]->getValue()->getTime();
            if (first > t)
                first = t;
        }
    }

    return first;
}

void SpreadSheetExporter::exportChannelList(
    const stdVector<stdString> &channel_names)
{
    size_t i, ai, num = channel_names.size();
    ChannelIteratorI **channels = 0;
    ValueIteratorI   **base = 0;
    ValueIteratorI   **values = 0;
    ValueI           **prev_values = 0;
    CtrlInfo         **infos = 0;
    const ValueI *v;
    char info[300];

    if (_use_matlab_format)
    {
        _undefined_value = "NaN";
        _comment = "% ";
    }
    else
    {
        _undefined_value = "#N/A";
        _comment = "; ";
    }
    
    if (num > _max_channel_count)
    {
        sprintf(info,
                "You tried to export %ld channels.\n"
                "For performance reason you are limited "
                "to export %ld channels at once",
                (long)num, (long)_max_channel_count);
        throwDetailedArchiveException(Invalid, info);
        return;
    }

    channels = new ChannelIteratorI *[num];
    base = new ValueIteratorI *[num];
    values = new ValueIteratorI *[num];
    prev_values = new ValueI *[num];
    infos = new CtrlInfo *[num];
    // Open Channel & ValueIterator
    for (i=0; i<num; ++i)
    {
        channels[i] = _archive->newChannelIterator();
        if (! _archive->findChannelByName(channel_names[i], channels[i]))
        {
            sprintf(info, "Cannot find channel '%s' in archive",
                    channel_names[i].c_str());
            throwDetailedArchiveException (ReadError, info);
            return;
        }
        base[i] = _archive->newValueIterator();
        prev_values[i] = 0;
        infos[i] = new CtrlInfo();
        values[i] = 0;

        if (! channels[i]->getChannel()->getValueBeforeTime(_start, base[i]))
            channels[i]->getChannel()->getValueAfterTime(_start, base[i]);

        if (_linear_interpol_secs > 0.0)
        {
            LinInterpolValueIteratorI *interpol =
                new LinInterpolValueIteratorI(base[i], _linear_interpol_secs);
            interpol->setMaxDeltaT(_linear_interpol_secs * _gap_factor);
            values[i] = interpol;
        }
        else
            values[i] = new ExpandingValueIteratorI(base[i]);

        if (values[i]->isValid() && values[i]->getValue()->getCount() > 1)
        {
            _is_array = true;
            if (num > 1)
            {
                sprintf(info,
                        "Array channels like '%s' can only be exported "
                        "on their own, "
                        "not together with another channel",
                        channel_names[i].c_str());
                throwDetailedArchiveException(Invalid, info);
            }
        }
    }

    FILE *f = stdout; // default: stdout
    if (! _filename.empty())
    {
        f = fopen(_filename.c_str(), "wt");
        if (! f)
        {
            throwDetailedArchiveException(WriteError, _filename);
        }
    }

    if (_use_matlab_format)
    {
        fputs("% Generated from archive data\n", f);
        fputs("%\n", f);
        fputs("% To plot this data into MatLab:\n", f);
        fputs("%\n", f);
        fputs("% [date, time, ch1, ch2]=textread('data.txt', '%s %s %f %f', "
              "'commentstyle', 'matlab', f);\n", f);
        fputs("%\n", f);
        fputs("% (replace ch1, ch2 by the names you like, "
              "check for a matching number of %f format arguments)\n", f);
        fputs("%\n", f);
        fputs("% You will obtain arrays date, time, ch1, ....\n", f);
        fputs("% To plot one of them over time:\n", f);
        fputs("% times=datenum(date) + datenum(time, f);\n", f);
        fputs("% plot(times, ch1, f); datetick('x', f);\n", f);
        fputs("\n", f);
    }
    else
    {
        fputs("; Generated from archive data\n", f);
        fputs(";\n", f);
        fputs("; To plot this data in MS Excel:\n", f);
        fputs("; 1) choose a suitable format for the Time colunm (1st col)\n",
              f);
        fputs(";    Excel can display up to the millisecond level\n", f);
        fputs(";    if you choose a custom cell format like\n", f);
        fputs(";              mm/dd/yy hh:mm:ss.000\n", f);
        fputs("; 2) Select the table and generate a plot,\n", f);
        fputs(";    a useful type is the 'XY (Scatter) plot'.\n", f);
        fputs(";\n", f);
        fprintf(f,
                "; %s is used to indicate that there is no valid data for\n",
                _undefined_value.c_str());
        fputs("; this point in time, e.g. the channel was disconnected.\n", f);
        fputs("; A seperate 'status' column - if included - shows details.\n",
              f);
        fputs(";\n", f);
    }

    if (_linear_interpol_secs > 0.0)
	{
		fprintf(f, "%sNOTE:\n", _comment);
		fprintf(f, "%sThe values in this table were interpolated within %g "
                "seconds\n", _comment, _linear_interpol_secs);
		fprintf(f, "%s(linear interpolation), so that the values for "
                "different channels\n", _comment);
		fprintf(f, "%scan be written on lines for the same time stamp.\n",
                _comment);
		fprintf(f, "%sIf you prefer to look at the exact time stamps "
                "for each value\n", _comment);
		fprintf(f, "%sexport the data without interpolation.\n", _comment);
	}
	else if (_fill)
	{
		fprintf(f, "%sNOTE:\n", _comment);
		fprintf(f, "%sThe values in this table were filled, i.e. repeated\n",
                _comment);
		fprintf(f, "%susing staircase interpolation.\n", _comment);
		fprintf(f, "%sIf you prefer to look at the exact time stamps\n",
                _comment);
		fprintf(f, "%sfor all channels, export the data without rounding.\n",
                _comment);
	}
	else
	{
		fprintf(f,
                "%sThis table holds the raw data as found in the archive.\n",
                _comment);
		fprintf(f, "%sSince Channels are not always scanned at exactly "
                "the same time,\n", _comment);
		fprintf(f, "%smany '%s' may appear when looking at more than "
                " one channel like this.\n", _comment,
                _undefined_value.c_str());
	}

    // Headline: "Time" and channel names
    if (_use_matlab_format)
        fprintf(f, "%sDate / Time", _comment);
    else
        fprintf(f, "Time");
    for (i=0; i<num; ++i)
    {
        fprintf(f, "\t%s", channels[i]->getChannel()->getName());
        if (! values[i]->isValid())
            continue;
        if (values[i]->getValue()->getCtrlInfo()->getType()
            == CtrlInfo::Numeric)
            fprintf(f, " [%s]",
                    values[i]->getValue()->getCtrlInfo()->getUnits());
        // Array columns
        for (ai=1; ai<values[i]->getValue()->getCount(); ++ai)
            fprintf(f, "\t[%zu]", ai);
        if (_show_status)
            fprintf(f, "\tStatus");
    }
    fprintf(f, "\n");

    // Find first time stamp
    epicsTime time = findOldestValue(values, num);
    while (time != nullTime)
    {
        // One line: time and all values for that time
        printTime(f, time);
        for (i=0; i<num; ++i)
        {
            // print all valid values that match the current time stamp:
            if (values[i]->isValid()  &&
                (v=values[i]->getValue())->getTime() == time)
            {
                ++_data_count;
                printValue(f, time, v);
                if (_fill) // keep copy of this value?
                {
                    if (v->isInfo () && prev_values[i])
                    {
                        delete prev_values[i];
                        prev_values[i] = 0;
                    }
                    /* would be faster but leads to crashes:
                     * Have to figure out when to copy the CtrlInfo and when
                     * the pointer can be kept. Pointer changes when we switch
                     * data files or even cross archive boundaries
                    else if (prev_values[i] && prev_values[i]->hasSameType(*v))
                        prev_values[i]->copyValue(*v);
                        */
                    else
                    {
                        delete prev_values[i];
                        prev_values[i] = v->clone();
                        *infos[i] = *v->getCtrlInfo();
                        prev_values[i]->setCtrlInfo(infos[i]);
                    }
                }
                values[i]->next();
            }
            else
            {   // no valid value for current time stamp:
                if (prev_values[i])
                    printValue(f, time, prev_values[i]);
                else
                    fprintf(f, "\t%s", _undefined_value.c_str());
            }
        }
        fprintf(f, "\n");
        // Print one value beyond time, then quit:
        if (isValidTime(_end) && time >= _end)
            break;
        // Find time stamp for next line
        time = findOldestValue(values, num);
    }

    if (f != stdout)
        fclose(f);
    for (i=0; i<num; ++i)
    {
        delete infos[i];
        delete prev_values[i];
        delete values[i];
        delete base[i];
        delete channels[i];
    }
    delete [] infos;
    delete [] prev_values;
    delete [] values;
    delete [] base;
    delete [] channels;
}








