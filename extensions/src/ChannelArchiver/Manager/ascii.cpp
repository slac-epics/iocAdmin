// --------------------------------------------------------
// $Id: ascii.cpp,v 1.1.1.1 2005/11/01 17:28:52 luchini Exp $
//
// Please refer to NOTICE.txt,
// included as part of this distribution,
// for legal information.
//
// Kay-Uwe Kasemir, kasemir@lanl.gov
// --------------------------------------------------------

// ASCII read/write routines
//

#include "ArchiverConfig.h"
#include "BinArchive.h"
#include <ASCIIParser.h>
#include <stdlib.h>

#ifdef MANAGER_USE_MULTI
#   include "MultiArchive.h"
#   define ARCHIVE_TYPE MultiArchive
#else
#   define ARCHIVE_TYPE BinArchive
#endif

// -------------------------------------------------------------------
// write
// -------------------------------------------------------------------

static void output_header(const ValueIterator &value)
{
    stdString type;
    value->getType(type);

    printf("# ------------------------------------------------------\n");
    printf("Header\n");
    printf("{\n");
    printf("\tperiod=%g\n", value.getPeriod());
    printf("\ttype=%s\n", type.c_str());
    printf("\tcount=%d\n", value->getCount());
    printf("}\n");
    printf("# ------------------------------------------------------\n");
}

static void output_info(const CtrlInfo *info)
{
    printf("# ------------------------------------------------------\n");
    printf("CtrlInfo\n");
    printf("{\n");
    if (info->getType() == CtrlInfo::Numeric)
    {
        printf("\ttype=Numeric\n");
        printf("\tprecision=%ld\n",    info->getPrecision());
        printf("\tunits=%s\n",        info->getUnits());
        printf("\tdisplay_high=%g\n", info->getDisplayHigh());
        printf("\tdisplay_low=%g\n",  info->getDisplayLow());
        printf("\thigh_alarm=%g\n",   info->getHighAlarm());
        printf("\thigh_warning=%g\n", info->getHighWarning());
        printf("\tlow_warning=%g\n",  info->getLowWarning());
        printf("\tlow_alarm=%g\n",    info->getLowAlarm());
    }
    else if (info->getType() == CtrlInfo::Enumerated)
    {
        printf("\ttype=Enumerated\n");
        printf("\tstates=%ld\n", (long)info->getNumStates());
        size_t i;
        stdString state;
        for (i=0; i<info->getNumStates(); ++i)
        {
            info->getState(i, state);
            printf("\tstate=%s\n", state.c_str());
        }
    }
    else
    {
        printf("\ttype=Unknown\n");
    }
    printf("}\n");
    printf("# ------------------------------------------------------\n");
}

void output_ascii(const stdString &archive_name,
                  const stdString &channel_name,
                  const epicsTime &start, const epicsTime &end)
{
    Archive         archive(new ARCHIVE_TYPE(archive_name));
    ChannelIterator channel(archive);
    ValueIterator   value(archive);
    if (! archive.findChannelByName(channel_name, channel))
    {
        printf("# Channel not found: %s\n", channel_name.c_str());
        return;
    }

    printf("channel=%s\n", channel_name.c_str());

    if (! channel->getValueAfterTime (start, value))
    {
        printf("# no values\n");
        return;
    }

    CtrlInfo   info;
    double period=-1;
    epicsTime last_time = nullTime;
    while (value && (!isValidTime(end)  ||  value->getTime() < end))
    {
        if (period != value.getPeriod())
        {
            period = value.getPeriod();
            output_header(value);
        }
        if (info != *value->getCtrlInfo())
        {
            info = *value->getCtrlInfo();
            output_info(&info);
        }
        if (isValidTime(last_time) && value->getTime() < last_time)
            printf("Error: back in time:\n");
        value->show(stdout);
        fputs("\n", stdout);
        last_time = value->getTime();
        ++value;
    }
}

// -------------------------------------------------------------------
// read
// -------------------------------------------------------------------

class ArchiveParser : public ASCIIParser
{
public:
    ArchiveParser()
    {
        _value = 0;
        _buffer_alloc = 0;
        _new_ctrl_info = false;
    }
    ~ArchiveParser()
    {
        delete _value;
    }

    void run(Archive &archive);
private:
    bool handleHeader(Archive &archive);
    bool handleCtrlInfo(ChannelIterator &channel);
    bool handleValue(ChannelIterator &channel);

    double _period;
    ValueI *_value;
    epicsTime _last_time;
    CtrlInfo _info;
    bool _new_ctrl_info;
    size_t  _buffer_alloc;
    enum
    {
        INIT_VALS_PER_BUF = 16,
        BUF_GROWTH_RATE = 4,
        MAX_VALS_PER_BUF = 1000
    };          
};

// Implementation:
// all handlers "handle" the current line
// and may move on to digest further lines,
// but will stay on their last line
// -> call nextLine again for following item

bool ArchiveParser::handleHeader(Archive &archive)
{
    _period=-1;
    DbrType type = 999;
    DbrCount count = 0;

    stdString parameter, value;
    
    if (!nextLine()  ||
        getLine () != "{")
    {
        printf("Line %zd: missing start of Header\n", getLineNo());
        return false;
    }

    while (nextLine())
    {
        if (getLine() == "}")
            break;
        if (getParameter(parameter, value))
        {
            if (parameter == "period")
                _period = atof(value.c_str());
            else if (parameter == "type")
            {
                if (! ValueI::parseType(value, type))
                {
                    printf("Line %zd: invalid type %s\n",
                           getLineNo(), value.c_str());
                    return false;
                }
            }
            else if (parameter == "count")
                count = atoi(value.c_str());
        }
        else
            printf("Line %zd skipped\n", getLineNo());
    }

    if (_period < 0.0 || type >= LAST_BUFFER_TYPE || count <= 0)
    {
        printf("Line %zd: incomplete header\n", getLineNo());
        return false;
    }
    if (_value)
    {
        if (_value->getType() != type  || _value->getCount() != count)
        {
            delete _value;
            _value = 0;
        }
    }
    if (!_value)
        _value = archive.newValue(type, count);
    _buffer_alloc = INIT_VALS_PER_BUF;
    _new_ctrl_info = false;

    return true;
}

bool ArchiveParser::handleCtrlInfo(ChannelIterator &channel)
{
    if (!_value)
    {
        printf("Line %zd: no header, yet\n", getLineNo());
        return false;
    }

    long prec=2;
    stdString units;
    float disp_low=0, disp_high=0;
    float low_alarm=0, low_warn=0, high_warn=0, high_alarm=0;   
    stdString parameter, value;
    CtrlInfo::Type type;
    size_t states = 0;
    stdVector<stdString> state;
    
    if (!nextLine()  ||
        getLine() != "{")
    {
        printf("Line %zd: missing start of CtrlInfo\n", getLineNo());
        return false;
    }
    
    while (nextLine())
    {
        if (getLine() == "}")
            break;
        if (getParameter (parameter, value))
        {
            if (parameter == "type")
            {
                if (value == "Numeric")
                    type = CtrlInfo::Numeric;
                else if (value == "Enumerated")
                    type = CtrlInfo::Enumerated;
                else
                {
                    printf("Line %zd: Unknown type %s\n",
                           getLineNo(), value.c_str());
                    return false;
                }
            }
            if (parameter == "display_high")
                disp_high = atof(value.c_str());
            else if (parameter == "display_low")
                disp_low = atof(value.c_str());
            else if (parameter == "high_alarm")
                high_alarm = atof(value.c_str());
            else if (parameter == "high_warning")
                high_warn = atof(value.c_str());
            else if (parameter == "low_warning")
                low_warn = atof(value.c_str());
            else if (parameter == "low_alarm")
                low_alarm = atof(value.c_str());
            else if (parameter == "precision")
                prec = atoi(value.c_str());
            else if (parameter == "units")
                units = value;
            else if (parameter == "states")
                states = atoi(value.c_str());
            else if (parameter == "state")
                state.push_back (value);
        }
        else
            printf("Line %zd skipped\n", getLineNo());
    }

    if (type == CtrlInfo::Numeric)
        _info.setNumeric (prec, units, disp_low, disp_high,
            low_alarm, low_warn, high_warn, high_alarm);                     
    else if (type == CtrlInfo::Enumerated)
    {
        if (state.size() != states)
        {
            printf("Line %zd: Asked for %zd states but provided %zd\n",
                   getLineNo(), states, state.size());
            return false;
        }
        size_t i, len = 0;
        for (i=0; i<states; ++i)
            len += state[i].length();
        _info.allocEnumerated (states, len);
        for (i=0; i<states; ++i)
            _info.setEnumeratedString (i, state[i].c_str());
    }
    else
    {
        printf("Line %zd: Invalid CtrlInfo\n", getLineNo());
        return false;
    }

    _value->setCtrlInfo(&_info);
    _new_ctrl_info = true;

    return true;
}

bool ArchiveParser::handleValue(ChannelIterator &channel)
{
    if (!_value)
    {
        printf("Line %zd: no header, yet\n", getLineNo());
        return false;
    }
    // Format of line:   time\tvalue\tstatus
    const stdString &line = getLine();
    size_t valtab = line.find('\t');
    if (valtab == stdString::npos)
    {
        printf("Line %zd: expected time stamp of value\n%s\n",
               getLineNo(), line.c_str());
        return false;
    }

    // Time:
    stdString text = line.substr(0, valtab);
    epicsTime time;
    if (! string2epicsTime(text, time))
    {
        printf("Line %zd: invalid time '%s'\n",
               getLineNo(), text.c_str());
        return false;
    }
    _value->setTime(time);

    stdString status;
    stdString value = line.substr(valtab+1);
    size_t stattab = value.find('\t');
    if (stattab != stdString::npos)
    {
        status = value.substr(stattab+1);
        value = value.substr(0, stattab);
    }
        
    // Value:
    if (! _value->parseValue(value))
    {
        printf("Line %zd: invalid value '%s'\n%s\n",
               getLineNo(), value.c_str(), line.c_str());
        return false;
    }

    // Status:
    if (status.empty())
        _value->setStatus(0, 0);
    else
    {
        if (!_value->parseStatus(status))
        {
            printf("Line %zd: invalid status '%s'\n%s\n",
                   getLineNo(), status.c_str(), line.c_str());
            return false;
        }
    }

    if (isValidTime(_last_time)  && _value->getTime() < _last_time)
    {
        printf("Line %zd: back in time\n", getLineNo());
        return false;
    }

    size_t avail = channel->lockBuffer(*_value, _period);
    if (avail <= 0 || _new_ctrl_info)
    {
        channel->addBuffer(*_value, _period, _buffer_alloc);
        if (_buffer_alloc < MAX_VALS_PER_BUF)
            _buffer_alloc *= BUF_GROWTH_RATE;
        _new_ctrl_info = false;
    }
    channel->addValue(*_value);
    channel->releaseBuffer();

    _last_time = _value->getTime();

    return true;
}

void ArchiveParser::run (Archive &archive)
{
    ChannelIterator channel (archive);
    ValueIterator value (archive);
    stdString parameter, arg;
    bool go = true;

    while (go && nextLine ())
    {
        if (getParameter (parameter, arg))
        {
            if (parameter == "channel")
            {
                if (archive.findChannelByName (arg, channel))
                {
                    _last_time = channel->getLastTime ();
                }
                else
                {
                    if (! archive.addChannel (arg, channel))
                    {
                        printf("Cannot add channel '%s' to archive\n",
                               arg.c_str());
                        return;
                    }
                    _last_time = nullTime;
                }
            }
        }
        else
        {
            if (getLine() == "Header")
                go = handleHeader(archive);
            else if (getLine() == "CtrlInfo")
                go = handleCtrlInfo(channel);
            else
                go = handleValue(channel);
        }
    }
}

void input_ascii(const stdString &archive_name, const stdString &file_name)
{
    ArchiveParser parser;
    if (! parser.open(file_name))
    {
        printf("Cannot open '%s'\n", file_name.c_str());
    }
    Archive archive(new BinArchive(archive_name, true));
    parser.run(archive);
}
