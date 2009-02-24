// --------------------------------------------------------
// $Id: ValueI.cpp,v 1.1.1.1 2004/04/01 20:49:45 luchini Exp $
//
// Please refer to NOTICE.txt,
// included as part of this distribution,
// for legal information.
//
// Kay-Uwe Kasemir, kasemir@lanl.gov
// --------------------------------------------------------

#include<alarm.h>
#include<ArrayTools.h>
#include<epicsTimeHelper.h>
#include<stdlib.h>
#include"ValueI.h"

//////////////////////////////////////////////////////////////////////
// ValueI
//////////////////////////////////////////////////////////////////////

ValueI::ValueI (DbrType type, DbrCount count)
{
    _type  = type;
    _count = count;
    _size  = RawValue::getSize(type, count);
    _value = RawValue::allocate(type, count, 1);
}

ValueI::~ValueI ()
{   RawValue::free(_value);   }

void ValueI::setCtrlInfo (const CtrlInfo *info)
{}

bool ValueI::parseStatus (const stdString &text)
{
    short  stat, sevr;
    if (RawValue::parseStatus (text, stat, sevr))
    {
        setStatus (stat, sevr);
        return true;
    }
    return false;
}

void ValueI::getType(stdString &text) const
{
    if (getType() < dbr_text_dim)
        text = dbr_text[getType()];
    else
        text = dbr_text_invalid;
}

bool ValueI::parseType (const stdString &text, DbrType &type)
{
    // db_access.h::dbr_text_to_type() doesn't handle invalid types,
    // so it's duplicated here:
    for (type=0; type<dbr_text_dim; ++type)
    {
        if (strcmp(text.c_str(), dbr_text[type]) == 0)
            return true;
    }
    return false;
}

void ValueI::show(FILE *f) const
{
    stdString time_text, stat_text;
    getTime(time_text);
    getStatus(stat_text);
    fprintf(f, "%s RawValue (type %d, count %d) %s",
            time_text.c_str(), getType(), getCount(),
            stat_text.c_str());
}

