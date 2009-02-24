// --------------------------------------------------------
// $Id: RawValue.cpp,v 1.1.1.1 2006/02/15 18:20:33 luchini Exp $
//
// Please refer to NOTICE.txt,
// included as part of this distribution,
// for legal information.
//
// Kay-Uwe Kasemir, kasemir@lanl.gov
// --------------------------------------------------------

#include <stdlib.h>
#include <alarm.h>
#include <alarmString.h>
#include <ArrayTools.h>
#include <epicsTimeHelper.h>
#include <MsgLogger.h>
#include "RawValue.h"
#include "CtrlInfo.h"
#include "ArchiveException.h"
#include "Conversions.h"

RawValue::Data * RawValue::allocate(DbrType type, DbrCount count, size_t num)
{
    size_t size = num*getSize(type, count);
    return (Data *) malloc(size);
}

void RawValue::free(Data *value)
{
    ::free(value);
}

size_t RawValue::getSize(DbrType type, DbrCount count)
{
    // Need to make the buffer size be a properly structure aligned number
    // Not sure why, but this is the way the first chan_arch did it.
    size_t buf_size = dbr_size_n(type, count);
    if (buf_size % 8)
        buf_size += 8 - (buf_size % 8);

    return buf_size;
}

bool RawValue::hasSameValue(DbrType type, DbrCount count, size_t size,
                            const Data *lhs, const Data *rhs)
{
    size_t offset;

    switch (type)
    {
    case DBR_TIME_STRING: offset = offsetof (dbr_time_string, value); break;
    case DBR_TIME_SHORT:  offset = offsetof (dbr_time_short, value);  break;
    case DBR_TIME_FLOAT:  offset = offsetof (dbr_time_float, value);  break;
    case DBR_TIME_ENUM:   offset = offsetof (dbr_time_enum, value);   break;
    case DBR_TIME_CHAR:   offset = offsetof (dbr_time_char, value);   break;
    case DBR_TIME_LONG:   offset = offsetof (dbr_time_long, value);   break;
    case DBR_TIME_DOUBLE: offset = offsetof (dbr_time_double, value); break;
    default:
        LOG_MSG("RawValue::hasSameValue: cannot decode type %d\n", type);
        return false;
    }

    return memcmp(((const char *)lhs) + offset,
                  ((const char *)rhs) + offset, size - offset) == 0;
}

void RawValue::getStatus(const Data *value, stdString &result)
{
    char buf[200];

    short severity = short(value->severity & 0xfff);
    switch (severity)
    {
    case NO_ALARM:
        result = '\0';
        return;
    // Archiver specials:
    case ARCH_EST_REPEAT:
        sprintf(buf, "Est_Repeat %d", (int)value->status);
        result = buf;
        return;
    case ARCH_REPEAT:
        sprintf(buf, "Repeat %d", (int)value->status);
        result = buf;
        return;
    case ARCH_DISCONNECT:
        result = "Disconnected";
        return;
    case ARCH_STOPPED:
        result = "Archive_Off";
        return;
    case ARCH_DISABLED:
        result = "Archive_Disabled";
        return;
    case ARCH_CHANGE_PERIOD:
        result = "Change Sampling Period";
        return;
    }

    if (severity < (short)SIZEOF_ARRAY(alarmSeverityString)  &&
        (short)value->status < (short)SIZEOF_ARRAY(alarmStatusString))
    {
        result = alarmSeverityString[severity];
        result += " ";
        result += alarmStatusString[value->status];
    }
    else
    {
        sprintf(buf, "%d %d", severity, value->status);
        result = buf;
    }
}

bool RawValue::isZero(DbrType type, const Data *value)
{
    switch (type)
    {
        case DBR_TIME_STRING:
            return ((dbr_time_string *)value)->value[0] == '\0';
        case DBR_TIME_ENUM:
            return ((dbr_time_enum *)value)->value == 0;
        case DBR_TIME_CHAR:
            return ((dbr_time_char *)value)->value == 0;
        case DBR_TIME_SHORT:
            return ((dbr_time_short *)value)->value == 0;

        case DBR_TIME_LONG:
            return ((dbr_time_long *)value)->value == 0;
        case DBR_TIME_FLOAT:
            return ((dbr_time_float *)value)->value == (float)0.0;
        case DBR_TIME_DOUBLE:
            return ((dbr_time_double *)value)->value == (double)0.0;
    }
    return false;
}

bool RawValue::parseStatus(const stdString &text, short &stat, short &sevr)
{
    if (text.empty())
    {
        stat = sevr = 0;
        return true;
    }
    if (!strncmp(text.c_str(), "Est_Repeat ", 11))
    {
        sevr = ARCH_EST_REPEAT;
        stat = atoi(text.c_str()+11);
        return true;
    }
    if (!strncmp(text.c_str(), "Repeat ", 7))
    {
        sevr = ARCH_REPEAT;
        stat = atoi(text.c_str()+7);
        return true;
    }
    if (!strcmp(text.c_str(), "Disconnected"))
    {
        sevr = ARCH_DISCONNECT;
        stat = 0;
        return true;
    }
    if (!strcmp(text.c_str(), "Archive_Off"))
    {
        sevr = ARCH_STOPPED;
        stat = 0;
        return true;
    }
    if (!strcmp(text.c_str(), "Archive_Disabled"))
    {
        sevr = ARCH_DISABLED;
        stat = 0;
        return true;
    }
    if (!strcmp(text.c_str(), "Change Sampling Period"))
    {
        sevr = ARCH_CHANGE_PERIOD;
        stat = 0;
        return true;
    }

    short i, j;
    for (i=0; i<(short)SIZEOF_ARRAY(alarmSeverityString); ++i)
    {
        if (!strncmp(text.c_str(), alarmSeverityString[i],
                     strlen(alarmSeverityString[i])))
        {
            sevr = i;
            stdString status = text.substr(strlen(alarmSeverityString[i]));

            for (j=0; j<(short)SIZEOF_ARRAY(alarmStatusString); ++j)
            {
                if (status.find(alarmStatusString[j]) != stdString::npos)
                {
                    stat = j;
                    return true;
                }
            }
            return false;
        }
    }

    return false;
}

void RawValue::getTime(const Data *value, stdString &time)
{
    epicsTime et(value->stamp);
    epicsTime2string(et, time);
}

void RawValue::show(FILE *file,
                    DbrType type, DbrCount count, const Data *value,
                    const class CtrlInfo *info)
{
    stdString time, stat;
    int i;
    // Print time - status
    getTime(value, time);
    getStatus(value, stat);
    fprintf(file, "%s\t%s", time.c_str(), stat.c_str());
    if (isInfo(value))
    {   // done
        fputs("\n", file);
        return;
    }
    // print value
    switch (type)
    {
        case DBR_TIME_STRING:
            fprintf(file, "\t'%s'\n", ((dbr_time_string *)value)->value);
            return;
        case DBR_TIME_ENUM:
            i = ((dbr_time_enum *)value)->value;
            if (info)
            {
                stdString state;
                info->getState(i, state);
                fprintf(file, "%s (%d)\n", state.c_str(), i);
            }
            else
                fprintf(file, "%d\n", i);
            return;
        case DBR_TIME_CHAR:
        {
            dbr_char_t *val = &((dbr_time_char *)value)->value;
            for (i=0; i<count; ++i)
            {
                if (i+1 >= count)
                    fprintf(file, "%d\n", (int)*val);
                else
                    fprintf(file, "%d\t", (int)*val);
                ++val;
            }
            return;
        }
            
        case DBR_TIME_SHORT:
        {
            dbr_short_t *val = &((dbr_time_short *)value)->value;
            for (i=0; i<count; ++i)
            {
                if (i+1 >= count)
                    fprintf(file, "%d\n", (int)*val);
                else
                    fprintf(file, "%d\t", (int)*val);
                ++val;
            }
            return;
        }
        case DBR_TIME_LONG:
        {
            dbr_long_t *val = &((dbr_time_long *)value)->value;
            for (i=0; i<count; ++i)
            {
                if (i+1 >= count)
                    fprintf(file, "%ld\n", (long)*val);
                else
                    fprintf(file, "%ld\t", (long)*val);
                ++val;
            }
            return;
        }
        case DBR_TIME_FLOAT:
        {
            dbr_float_t *val = &((dbr_time_float *)value)->value;
            for (i=0; i<count; ++i)
            {
                if (i+1 >= count)
                    fprintf(file, "%f\n", (double)*val);
                else
                    fprintf(file, "%f\t", (double)*val);
                ++val;
            }
            return;
        }
        case DBR_TIME_DOUBLE:
        {
            dbr_double_t *val = &((dbr_time_double *)value)->value;
            for (i=0; i<count; ++i)
            {
                if (i+1 >= count)
                    fprintf(file, "%f\n", (double)*val);
                else
                    fprintf(file, "%f\t", (double)*val);
                ++val;
            }
            return;
        }
        default:
            fprintf(file, "<cannot decode>\n");
    }
}   

void RawValue::read(DbrType type, DbrCount count, size_t size, Data *value,
                    FILE *file, FileOffset offset)
{
    if (fseek(file, offset, SEEK_SET) != 0 ||
        (FileOffset) ftell(file) != offset   ||
        fread(value, size, 1, file) != 1)
        throwArchiveException(ReadError);
    
    SHORTFromDisk(value->status);
    SHORTFromDisk(value->severity);
    epicsTimeStampFromDisk(value->stamp);

    // nasty: cannot use inheritance in lightweight RawValue,
    // so we have to switch on the type here:
    switch (type)
    {
    case DBR_TIME_CHAR:
    case DBR_TIME_STRING:
        break;
        /* The following might generate 'unused variable dbr_...'
         * warnings on systems where no byte swapping is required
         */
#define FROM_DISK(DBR, TYP, TIMETYP, MACRO)                                   \
    case DBR:                                                                 \
        {                                                                     \
            TYP *data __attribute__ ((unused)) = & ((TIMETYP *)value)->value; \
            for (size_t i=0; i<count; ++i)  MACRO(data[i]);                   \
        }                                                                     \
        break;
        FROM_DISK(DBR_TIME_DOUBLE, dbr_double_t, dbr_time_double, DoubleFromDisk)
        FROM_DISK(DBR_TIME_FLOAT,  dbr_float_t,  dbr_time_float,  FloatFromDisk)
        FROM_DISK(DBR_TIME_SHORT,  dbr_short_t,  dbr_time_short,  SHORTFromDisk)
        FROM_DISK(DBR_TIME_ENUM,   dbr_enum_t,   dbr_time_enum,   USHORTFromDisk)
        FROM_DISK(DBR_TIME_LONG,   dbr_long_t,   dbr_time_long,   LONGFromDisk)
    default:
        throwDetailedArchiveException(Invalid, "Unknown DBR_xx");
#undef FROM_DISK
    }
}

void RawValue::write(DbrType type, DbrCount count, size_t size, const Data *value,
                     MemoryBuffer<dbr_time_string> &cvt_buffer,
                     FILE *file, FileOffset offset)
{
    cvt_buffer.reserve(size);
    dbr_time_string *buffer = cvt_buffer.mem();

    memcpy(buffer, value, size);
    SHORTToDisk(buffer->status);
    SHORTToDisk(buffer->severity);
    epicsTimeStampToDisk(buffer->stamp);

    switch (type)
    {
#define TO_DISK(DBR, TYP, TIMETYP, CVT_MACRO)                                  \
    case DBR:                                                                  \
        {                                                                      \
            TYP *data __attribute__ ((unused)) = & ((TIMETYP *)buffer)->value; \
            for (size_t i=0; i<count; ++i)  CVT_MACRO (data[i]);               \
        }                                                                      \
        break;

    case DBR_TIME_CHAR:
    case DBR_TIME_STRING:
        // no conversion necessary
        break;
        TO_DISK(DBR_TIME_DOUBLE, dbr_double_t, dbr_time_double, DoubleToDisk)
        TO_DISK(DBR_TIME_FLOAT,  dbr_float_t,  dbr_time_float,  FloatToDisk)
        TO_DISK(DBR_TIME_SHORT,  dbr_short_t,  dbr_time_short,  SHORTToDisk)
        TO_DISK(DBR_TIME_ENUM,   dbr_enum_t,   dbr_time_enum,   USHORTToDisk)
        TO_DISK(DBR_TIME_LONG,   dbr_long_t,   dbr_time_long,   LONGToDisk)
    default:
        throwDetailedArchiveException (Invalid, "Unknown DBR_xx");
#undef TO_DISK
    }

    if (fseek (file, offset, SEEK_SET) != 0 ||
        (FileOffset) ftell(file) != offset  ||
        fwrite(buffer, size, 1, file) != 1)
        throwArchiveException (WriteError);
}

