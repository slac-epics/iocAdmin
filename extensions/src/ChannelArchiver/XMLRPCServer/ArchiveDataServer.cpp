// System
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>
// XML-RPC
#include <xmlrpc.h>
// EPICS Base
#include <epicsVersion.h>
#include <alarm.h>
#include <epicsMath.h> // math.h + isinf + isnan
// Tools
#include <AutoPtr.h>
#include <MsgLogger.h>
#include <RegularExpression.h>
#include <BinaryTree.h>
// Storage
#include <SpreadsheetReader.h>
#include <LinearReader.h>
#include <PlotReader.h>
// XMLRPCServer
#include "ArchiveDataServer.h"

#define LOGFILE

/// Auto-Ptr for XmlRpcValue.
///
/// Will remove the reference to the value (DECREF)
/// unless the pointer is 'released' from the AutoXmlRpcValue.
class AutoXmlRpcValue
{
public:
    /// Constructor initializes with given pointer.
    AutoXmlRpcValue(xmlrpc_value *val = 0) 
            : val(val) 
    {
#ifdef DEBUG_AUTOXMLRPCVALUE
        LOG_MSG("AutoXmlRpcValue 0x%lX\n", (unsigned long)val);
#endif
    }

    /// Desctructor decrements the refererence.
    ~AutoXmlRpcValue()
    {
        set(0);
    }

    /// Set to (different) pointer.
    ///
    /// Will decrement reference to current one.
    void set(xmlrpc_value *nval)
    {
        if (val)
        {
#ifdef DEBUG_AUTOXMLRPCVALUE
            LOG_MSG("AutoXmlRpcValue 0x%lX released\n", (unsigned long)val);
#endif
            xmlrpc_DECREF(val);
        }
        val = nval;
#ifdef DEBUG_AUTOXMLRPCVALUE
        LOG_MSG("AutoXmlRpcValue 0x%lX\n", (unsigned long)val);
#endif
    }       

    /// @see set
    AutoXmlRpcValue & operator = (xmlrpc_value *nval)
    {
        set(nval);
        return *this;
    }

    /// @return True if current pointer is valid.
    operator bool () const
    { 
        return val != 0;
    }

    /// @return Return the current pointer.
    operator xmlrpc_value * () const
    {
        return val;
    }

    /// @return Return the current pointer.
    xmlrpc_value * get() const
    {
        return val;
    }

    /// Release the pointer so that it will <u>not</u> be auto-decremented.
    ///
    /// After this call, the AutoXmlRpcValue is empty (0, invalid).
    ///
    /// @return Return the current pointer.    
    xmlrpc_value * release()
    {
        xmlrpc_value *copy = val;
        val = 0;
        return copy;
    }
    
private:
    xmlrpc_value *val;
    AutoXmlRpcValue & operator = (AutoXmlRpcValue &rhs);
};

// Some compilers have "finite()",
// others have "isfinite()".
// This hopefully works everywhere:
static double make_finite(double value)
{
    if (isinf(value) ||  isnan(value))
        return 0;
    return value;
}

// Used by get_names to put info for channel into sorted tree & dump it
class ChannelInfo
{
public:
    stdString name;
    epicsTime start, end;

    // Required by BinaryTree for sorting. We sort by name.
    bool operator < (const ChannelInfo &rhs)   { return name < rhs.name; }
    bool operator == (const ChannelInfo &rhs)  { return name == rhs.name; }

    class UserArg
    {
    public:
        xmlrpc_env   *env;
        xmlrpc_value *result;
    };
    
    // "visitor" for BinaryTree of channel names
    static void add_name_to_result(const ChannelInfo &info, void *arg)
    {
        UserArg *user_arg = (UserArg *)arg;
        xmlrpc_int32 ss, sn, es, en;
        epicsTime2pieces(info.start, ss, sn);
        epicsTime2pieces(info.end, es, en);        

        LOG_MSG("Found name '%s'\n", info.name.c_str());

        AutoXmlRpcValue channel(xmlrpc_build_value(
                                    user_arg->env,
                                    "{s:s,s:i,s:i,s:i,s:i}",
                                    "name", info.name.c_str(),
                                    "start_sec", ss, "start_nano", sn,
                                    "end_sec", es,   "end_nano", en));
        if (user_arg->env->fault_occurred)
            return;
        xmlrpc_array_append_item(user_arg->env, user_arg->result, channel);
    }
};

// Return CtrlInfo encoded a per "meta" returned by archiver.get_values
static xmlrpc_value *encode_ctrl_info(xmlrpc_env *env, const CtrlInfo *info)
{
    if (info && info->getType() == CtrlInfo::Enumerated)
    {
        AutoXmlRpcValue states(xmlrpc_build_value(env, "()"));
        stdString state_txt;
        size_t i, num = info->getNumStates();
        for (i=0; i<num; ++i)
        {
            info->getState(i, state_txt);
            AutoXmlRpcValue state(
                xmlrpc_build_value(env, "s#",
                                   state_txt.c_str(), state_txt.length()));
            xmlrpc_array_append_item(env, states, state);
        }
        xmlrpc_value *meta = xmlrpc_build_value(
            env, "{s:i,s:V}",
            "type", (xmlrpc_int32)META_TYPE_ENUM,
            "states", (xmlrpc_value *)states);
        return meta;
    }
    if (info && info->getType() == CtrlInfo::Numeric)
    {
        return xmlrpc_build_value(
            env, "{s:i,s:d,s:d,s:d,s:d,s:d,s:d,s:i,s:s}",
            "type", (xmlrpc_int32)META_TYPE_NUMERIC,
            "disp_high",  make_finite(info->getDisplayHigh()),
            "disp_low",   make_finite(info->getDisplayLow()),
            "alarm_high", make_finite(info->getHighAlarm()),
            "warn_high",  make_finite(info->getHighWarning()),
            "warn_low",   make_finite(info->getLowWarning()),
            "alarm_low",  make_finite(info->getLowAlarm()),
            "prec", (xmlrpc_int32)info->getPrecision(),
            "units", info->getUnits());
    }
    return xmlrpc_build_value(
            env, "{s:i,s:d,s:d,s:d,s:d,s:d,s:d,s:i,s:s}",
            "type", (xmlrpc_int32)META_TYPE_NUMERIC,
            "disp_high",  10.0,
            "disp_low",   0.0,
            "alarm_high", 0.0,
            "warn_high",  0.0,
            "warn_low",   0.0,
            "alarm_low",  0.0,
            "prec", (xmlrpc_int32) 1,
            "units", "<NO DATA>");
    /*
    return  xmlrpc_build_value(env, "{s:i,s:(s)}",
                               "type", (xmlrpc_int32)META_TYPE_ENUM,
                               "states", "<NO DATA>");
    */
}

void dbr_type_to_xml_type(DbrType dbr_type, DbrCount dbr_count,
                          xmlrpc_int32 &xml_type, xmlrpc_int32 &xml_count)
{
    switch (dbr_type)
    {
        case DBR_TIME_STRING: xml_type = XML_STRING; break;
        case DBR_TIME_ENUM:   xml_type = XML_ENUM;   break;
        case DBR_TIME_SHORT:
        case DBR_TIME_CHAR:
        case DBR_TIME_LONG:   xml_type = XML_INT;    break;
        case DBR_TIME_FLOAT:    
        case DBR_TIME_DOUBLE:
        default:              xml_type = XML_DOUBLE; break;
    }
    xml_count = dbr_count;
}

// Given a raw sample dbr_type/count/time/data,
// map it onto xml_type/count and add to values.
// Handles the special case data == 0,
// which happens for undefined cells in a SpreadsheetReader.
void encode_value(xmlrpc_env *env,
                  DbrType dbr_type, DbrCount dbr_count,
                  const epicsTime &time, const RawValue::Data *data,
                  xmlrpc_int32 xml_type, xmlrpc_int32 xml_count,
                  xmlrpc_value *values)
{
    if (xml_count > dbr_count)
        xml_count = dbr_count;
    AutoXmlRpcValue val_array(xmlrpc_build_value(env, "()"));
    if (env->fault_occurred)
        return;
    int i;
    switch (xml_type)
    {
        case XML_STRING:
        {
            stdString txt;
            if (data)
                RawValue::getValueString(txt, dbr_type, dbr_count, data, 0);
            AutoXmlRpcValue element(xmlrpc_build_value(env, "s#",
                                                       txt.c_str(), txt.length()));
            xmlrpc_array_append_item(env, val_array, element);
        }
        break;
        case XML_INT:
        case XML_ENUM:
        {
            long l;
            for (i=0; i < xml_count; ++i)
            {
                if (!data  ||
                    !RawValue::getLong(dbr_type, dbr_count, data, l, i))
                    l = 0;
                AutoXmlRpcValue element(xmlrpc_build_value(env, "i", (xmlrpc_int32)l));
                xmlrpc_array_append_item(env, val_array, element);
            }
        }
        break;
        case XML_DOUBLE:
        {
            double d;
            for (i=0; i < xml_count; ++i)
            {
                if (data  &&
                    RawValue::getDouble(dbr_type, dbr_count, data, d, i))
                
                {   /* XML-RPC, being XML, sends all numbers as strings.
                     * Unfortunately, XML-RPC also insists in the simple 'dot'
                     * notation and prohibits exponential notation.
                     * A number like 1e-300 would turn into
                     * "0.0000000..." with 300 zeroes,
                     * which is too long for the internal print buffer of
                     * the xml-rpc C library.
                     * Since such huge and tiny numbers can't be transferred,
                     * they are replaced by 0 with stat/sevr UDF/INVALID.
                     *
                     * The cut-off point is somewhat arbitrary.
                     * The XML-RPC library uses an internal print buffer
                     * of about 120 characters.
                     * Since PVs are usually scaled to be human-readable,
                     * with only vacuum readings using exp. notation for
                     * data like "1e-8 Torr", exponents of +-50 seemed
                     * reasonable.
                     */
                    if (d > 0.0)
                    {
                        if (d < 1e-50  ||  d > 1e50)   { d = 0.0; data = 0; }
                    }
                    else if (d < 0.0)
                    {  
                        if (d > -1e-50  ||  d < -1e50) { d = 0.0; data = 0; }
                    }
                } 
                else
                    d = 0.0;
                AutoXmlRpcValue element(xmlrpc_build_value(env, "d", d));
                xmlrpc_array_append_item(env, val_array, element);
            }
        }
    }
    xmlrpc_int32 secs, nano;
    epicsTime2pieces(time, secs, nano);
    AutoXmlRpcValue value;
    if (data)
        value.set(xmlrpc_build_value(
                      env, "{s:i,s:i,s:i,s:i,s:V}",
                      "stat", (xmlrpc_int32)RawValue::getStat(data),
                      "sevr", (xmlrpc_int32)RawValue::getSevr(data),
                      "secs", secs,
                      "nano", nano,
                      "value", (xmlrpc_value *)val_array));
    else
        value.set(xmlrpc_build_value(
                      env, "{s:i,s:i,s:i,s:i,s:V}",
                      "stat", (xmlrpc_int32)UDF_ALARM,
                      "sevr", (xmlrpc_int32)INVALID_ALARM,
                      "secs", secs,
                      "nano", nano,
                      "value",  (xmlrpc_value *)val_array));    
    xmlrpc_array_append_item(env, values, value);
}

// Return the data for all the names[], start .. end etc.
// as get_values() is supposed to return them.
// Returns 0 on error.
xmlrpc_value *get_channel_data(xmlrpc_env *env,
                               int key,
                               const stdVector<stdString> names,
                               const epicsTime &start, const epicsTime &end,
                               long count,
                               ReaderFactory::How how, double delta)
{
    AutoXmlRpcValue results;
    try
    {
#ifdef LOGFILE
        stdString txt;
        LOG_MSG("get_channel_data\n");
        LOG_MSG("Method: %s\n", ReaderFactory::toString(how, delta));
        LOG_MSG("Start:  %s\n", epicsTimeTxt(start, txt));
        LOG_MSG("End  :  %s\n", epicsTimeTxt(end, txt));
#endif
        AutoPtr<Index> index(open_index(env, key));
        if (env->fault_occurred)
            return 0;
        AutoPtr<DataReader> reader(ReaderFactory::create(*index, how, delta));
        results = xmlrpc_build_value(env, "()");
        if (env->fault_occurred)
            return 0;
        long i, name_count = names.size();
        for (i=0; i<name_count; ++i)
        {
#ifdef LOGFILE
            LOG_MSG("Handling '%s'\n", names[i].c_str());
#endif
            long num_vals = 0;            
            AutoXmlRpcValue values(xmlrpc_build_value(env, "()"));
            if (env->fault_occurred)
                return 0;
            const RawValue::Data *data = reader->find(names[i], &start);
            AutoXmlRpcValue meta;
            xmlrpc_int32 xml_type, xml_count;
            if (data == 0)
            {   // No exception from file error etc., just no data.
                meta = encode_ctrl_info(env, 0);
                xml_type = XML_ENUM;
                xml_count = 1;
            }
            else
            {
                // Fix meta/type/count based on first value
                meta = encode_ctrl_info(env, &reader->getInfo());
                dbr_type_to_xml_type(reader->getType(), reader->getCount(),
                                     xml_type, xml_count);
                while (num_vals < count
                       && data
                       && RawValue::getTime(data) < end)
                {
                    encode_value(env, reader->getType(), reader->getCount(),
                                 RawValue::getTime(data), data,
                                 xml_type, xml_count, values);
                    ++num_vals;
                    data = reader->next();
                }
            }
            // Assemble result = { name, meta, type, count, values }
            AutoXmlRpcValue result(xmlrpc_build_value(
                                       env, "{s:s,s:V,s:i,s:i,s:V}",
                                       "name",   names[i].c_str(),
                                       "meta",   (xmlrpc_value *)meta,
                                       "type",   xml_type,
                                       "count",  xml_count,
                                       "values", (xmlrpc_value *)values));
            // Add to result array
            xmlrpc_array_append_item(env, results, result);
#ifdef LOGFILE
            LOG_MSG("%ld values\n", num_vals);
#endif
        } // for ( .. name .. )
    }
    catch (GenericException &e)
    {
#ifdef LOGFILE
        LOG_MSG("Error:\n%s\n", e.what());
#endif
        xmlrpc_env_set_fault_formatted(env, ARCH_DAT_DATA_ERROR,
                                       "%s", e.what());
        return 0;
    }
    return results.release();
}

// Return the data for all the names[], start .. end etc.
// as get_values() is supposed to return them.
//
// Returns raw values if interpol <= 0.0.
// Returns 0 on error.
xmlrpc_value *get_sheet_data(xmlrpc_env *env,
                             int key,
                             const stdVector<stdString> names,
                             const epicsTime &start, const epicsTime &end,
                             long count,
                             ReaderFactory::How how, double delta)
{
    try
    {
#ifdef LOGFILE
        stdString txt;
        LOG_MSG("get_sheet_data\n");
        LOG_MSG("Start : %s\n", epicsTimeTxt(start, txt));
        LOG_MSG("End   : %s\n", epicsTimeTxt(end, txt));
        LOG_MSG("Method: %s\n", ReaderFactory::toString(how, delta));
 #endif
        AutoPtr<Index> index(open_index(env, key));
        if (env->fault_occurred)
            return 0;

        AutoPtr<SpreadsheetReader> sheet(new SpreadsheetReader(*index, how, delta));
    
        AutoXmlRpcValue results(xmlrpc_build_value(env, "()"));
        if (env->fault_occurred)
            return 0;    

        long name_count = names.size();
        AutoArrayPtr<AutoXmlRpcValue> meta     (new AutoXmlRpcValue[name_count]);
        AutoArrayPtr<AutoXmlRpcValue> values   (new AutoXmlRpcValue[name_count]);
        AutoArrayPtr<xmlrpc_int32>    xml_type (new xmlrpc_int32[name_count]);
        AutoArrayPtr<xmlrpc_int32>    xml_count(new xmlrpc_int32[name_count]);
        AutoArrayPtr<size_t>          ch_vals  (new size_t[name_count]);
        bool ok = sheet->find(names, &start);
        long i;
        // Per-channel meta-info.
        for (i=0; i<name_count; ++i)
        {
#ifdef LOGFILE
            LOG_MSG("Handling '%s'\n", names[i].c_str());
#endif
            ch_vals[i] = 0;
            values[i] = xmlrpc_build_value(env, "()");            
            if (env->fault_occurred)
                return 0;
            if (sheet->found(i))
            {   // Fix meta/type/count based on first value
                meta[i] = encode_ctrl_info(env, &sheet->getInfo(i));
                dbr_type_to_xml_type(sheet->getType(i), sheet->getCount(i),
                                     xml_type[i], xml_count[i]);
#if 0
                LOG_MSG("Ch %lu: type, count = %d, %d\n", i, (int)xml_type[i], (int)xml_count[i]);
#endif
            }
            else
            {   // Channel exists, but has no data
                meta[i] = encode_ctrl_info(env, 0);
                xml_type[i] = XML_ENUM;
                xml_count[i] = 1;
            }
        }
        // Collect values
        long num_vals = 0;
        while (ok && num_vals < count && sheet->getTime() < end)
        {
            for (i=0; i<name_count; ++i)
            {
                if (sheet->get(i))
                {
                    ++ch_vals[i];
                    encode_value(env,
                                 sheet->getType(i), sheet->getCount(i),
                                 sheet->getTime(), sheet->get(i),
                                 xml_type[i], xml_count[i], values[i]);
                }
                else
                {   // Encode as no value, but one of them ;-)
                    // to avoid confusion w/ viewers that like to see some data in any case.
                    encode_value(env, 0, 1, sheet->getTime(), 0,
                                 xml_type[i], xml_count[i], values[i]);
                }
                
            }
            ++num_vals;
            ok = sheet->next();
        }
        // Assemble result = { name, meta, type, count, values }
        for (i=0; i<name_count; ++i)
        {
            AutoXmlRpcValue result(
                xmlrpc_build_value(env, "{s:s,s:V,s:i,s:i,s:V}",
                                   "name",   names[i].c_str(),
                                   "meta",   meta[i].get(),
                                   "type",   xml_type[i],
                                   "count",  xml_count[i],
                                   "values", values[i].get()));    
            // Add to result array
            xmlrpc_array_append_item(env, results, result);
#ifdef LOGFILE
            LOG_MSG("Ch %lu: %zu values\n", i, ch_vals[i]);
#endif
        }
#ifdef LOGFILE
        LOG_MSG("%ld values total\n", num_vals);
#endif
        return results.release();
    }
    catch (GenericException &e)
    {
#ifdef LOGFILE
        LOG_MSG("Error:\n%s\n", e.what());
#endif
        xmlrpc_env_set_fault_formatted(env, ARCH_DAT_DATA_ERROR,
                                       "%s", e.what());
    }
    return 0;
}

// ---------------------------------------------------------------------
// XML-RPC callbacks
// ---------------------------------------------------------------------

// { int32  ver, string desc } = archiver.info()
xmlrpc_value *get_info(xmlrpc_env *env, xmlrpc_value *args, void *user)
{
    extern const char *alarmStatusString[];
    extern const char *alarmSeverityString[];
#ifdef LOGFILE
    LOG_MSG("archiver.info\n");
#endif
    int i;
    const char *config = get_config_name(env);
    if (!config)
        return 0;
#ifdef LOGFILE
    LOG_MSG("config: '%s'\n", config);
#endif
    char description[500];
    sprintf(description,
            "Channel Archiver Data Server V%d,\n"
            "for " EPICS_VERSION_STRING ",\n"
            "built " __DATE__ ", " __TIME__ "\n"
            "from sources for version " ARCH_VERSION_TXT "\n"
            "Config: '%s'\n",
            ARCH_VER, config);
    AutoXmlRpcValue how(xmlrpc_build_value(env, "(sssss)",
                                           "raw",
                                           "spreadsheet",
                                           "average",
                                           "plot-binning",
                                           "linear"));
    if (env->fault_occurred)
        return 0;
    // 'status': array of all status string.
    AutoXmlRpcValue element;
    AutoXmlRpcValue status(xmlrpc_build_value(env, "()"));
    if (env->fault_occurred)
        return 0;
    for (i=0; i<=lastEpicsAlarmCond; ++i)
    {
        element = xmlrpc_build_value(env, "s", alarmStatusString[i]);
        xmlrpc_array_append_item(env, status, element);
        if (env->fault_occurred)
            return 0;
    }
    // 'severity': array of all severity strings.
    AutoXmlRpcValue severity(xmlrpc_build_value(env, "()"));
    for (i=0; i<=lastEpicsAlarmSev; ++i)
    {
        element = xmlrpc_build_value(env, "{s:i,s:s,s:b,s:b}",
                                     "num", (xmlrpc_int32)i,
                                     "sevr", alarmSeverityString[i],
                                     "has_value", (xmlrpc_bool) 1,
                                     "txt_stat", (xmlrpc_bool) 1);
        xmlrpc_array_append_item(env, severity, element);
        if (env->fault_occurred)
            return 0;
    }
    // ... including the archive-specific ones.
    element = xmlrpc_build_value(env, "{s:i,s:s,s:b,s:b}",
                                 "num", (xmlrpc_int32)ARCH_EST_REPEAT,
                                 "sevr", "Est_Repeat",
                                 "has_value", (xmlrpc_bool) 1,
                                 "txt_stat", (xmlrpc_bool) 0);
    xmlrpc_array_append_item(env, severity, element);
    element = xmlrpc_build_value(env, "{s:i,s:s,s:b,s:b}",
                                 "num", (xmlrpc_int32)ARCH_REPEAT,
                                 "sevr", "Repeat",
                                 "has_value", (xmlrpc_bool) 1,
                                 "txt_stat", (xmlrpc_bool) 0);
    xmlrpc_array_append_item(env, severity, element);
    element = xmlrpc_build_value(env, "{s:i,s:s,s:b,s:b}",
                                 "num", (xmlrpc_int32)ARCH_DISCONNECT,
                                 "sevr", "Disconnected",
                                 "has_value", (xmlrpc_bool) 0,
                                 "txt_stat", (xmlrpc_bool) 1);
    xmlrpc_array_append_item(env, severity, element);
    element = xmlrpc_build_value(env, "{s:i,s:s,s:b,s:b}",
                                 "num", (xmlrpc_int32)ARCH_STOPPED,
                                 "sevr", "Archive_Off",
                                 "has_value", (xmlrpc_bool) 0,
                                 "txt_stat", (xmlrpc_bool) 1);
    xmlrpc_array_append_item(env, severity, element);
    element = xmlrpc_build_value(env, "{s:i,s:s,s:b,s:b}",
                                 "num", (xmlrpc_int32)ARCH_DISABLED,
                                 "sevr", "Archive_Disabled",
                                 "has_value", (xmlrpc_bool) 0,
                                 "txt_stat", (xmlrpc_bool) 1);
    xmlrpc_array_append_item(env, severity, element);
    // Overall result.
    return xmlrpc_build_value(env, "{s:i,s:s,s:V,s:V,s:V}",
                              "ver", ARCH_VER,
                              "desc", description,
                              "how", how.get(),
                              "stat", status.get(),
                              "sevr", severity.get());
}

// {int32 key, string name, string path}[] = archiver.archives()
xmlrpc_value *get_archives(xmlrpc_env *env, xmlrpc_value *args, void *user)
{
#ifdef LOGFILE
    LOG_MSG("archiver.archives\n");
#endif
    // Get Configuration
    ServerConfig config;
    if (!get_config(env, config))
        return 0;
    // Create result
    AutoXmlRpcValue result(xmlrpc_build_value(env, "()"));
    if (env->fault_occurred)
        return 0;
    stdList<ServerConfig::Entry>::const_iterator i;
    for (i=config.config.begin(); i!=config.config.end(); ++i)
    {
        AutoXmlRpcValue archive(
            xmlrpc_build_value(env, "{s:i,s:s,s:s}",
                               "key",  i->key,
                               "name", i->name.c_str(),
                               "path", i->path.c_str()));
        xmlrpc_array_append_item(env, result, archive);
    }
    return result.release();
}

// {string name, int32 start_sec, int32 start_nano,
//               int32 end_sec,   int32 end_nano}[]
// = archiver.names(int32 key, string pattern)
xmlrpc_value *get_names(xmlrpc_env *env, xmlrpc_value *args, void *user)
{
#ifdef LOGFILE
    LOG_MSG("archiver.names\n");
#endif
    // Get args, maybe setup pattern
    AutoPtr<RegularExpression> regex;
    xmlrpc_int32 key;
    char *pattern = 0;
    size_t pattern_len = 0; 
    xmlrpc_parse_value(env, args, "(is#)", &key, &pattern, &pattern_len);
    if (env->fault_occurred)
        return 0;
    // Create result
    AutoXmlRpcValue result(xmlrpc_build_value(env, "()"));
    if (env->fault_occurred)
        return 0;
    try
    {
        if (pattern_len > 0)
            regex.assign(new RegularExpression(pattern));
        // Open Index
        stdString directory;
        AutoPtr<Index> index(open_index(env, key));
        if (env->fault_occurred)
            return 0;
        // Put all names in binary tree
        Index::NameIterator ni;
        BinaryTree<ChannelInfo> channels;
        ChannelInfo info;
        bool ok;
        for (ok = index->getFirstChannel(ni);
             ok; ok = index->getNextChannel(ni))
        {
            if (regex && !regex->doesMatch(ni.getName()))
                continue; // skip what doesn't match regex
            info.name = ni.getName();
            AutoPtr<RTree> tree(index->getTree(info.name, directory));
            if (tree)
                tree->getInterval(info.start, info.end);
            else // Is this an error?
                info.start = info.end = nullTime;
            channels.add(info);
        }
        index->close();
        // Sorted dump of names
        ChannelInfo::UserArg user_arg;
        user_arg.env = env;
        user_arg.result = result;
        channels.traverse(ChannelInfo::add_name_to_result, (void *)&user_arg);
#ifdef LOGFILE
        LOG_MSG("get_names(%d, '%s') -> %d names\n",
                key,
                (pattern ? pattern : "<no pattern>"),
                xmlrpc_array_size(env, result));
#endif
    }
    catch (GenericException &e)
    {
        xmlrpc_env_set_fault_formatted(env, ARCH_DAT_SERV_FAULT,
                                       (char *) e.what());
        return 0;
    }
    return result.release();
}

// very_complex_array = archiver.values(key, names[], start, end, ...)
xmlrpc_value *get_values(xmlrpc_env *env, xmlrpc_value *args, void *user)
{
#ifdef LOGFILE
    LOG_MSG("archiver.get_values\n");
#endif
    xmlrpc_value *names;
    xmlrpc_int32 key, start_sec, start_nano, end_sec, end_nano, count, how;
    xmlrpc_int32 actual_count;
    // Extract arguments
    xmlrpc_parse_value(env, args, "(iAiiiiii)",
                       &key, &names,
                       &start_sec, &start_nano, &end_sec, &end_nano,
                       &count, &how);    
    if (env->fault_occurred)
        return 0;
#ifdef LOGFILE
    LOG_MSG("how=%d, count=%d\n", (int) how, (int) count);
#endif
    if (count <= 1)
        count = 1;
    actual_count = count;
    if (count > 10000) // Upper limit to avoid outrageous requests.
        actual_count = 10000;
    // Build start/end
    epicsTime start, end;
    pieces2epicsTime(start_sec, start_nano, start);
    pieces2epicsTime(end_sec, end_nano, end);    
    // Pull names into vector for SpreadsheetReader and just because.
    xmlrpc_value *name_val;
    char *name;
    int i;
    xmlrpc_int32 name_count = xmlrpc_array_size(env, names);
    stdVector<stdString> name_vector;
    name_vector.reserve(name_count);
    for (i=0; i<name_count; ++i)
    {   // no new ref!
        name_val = xmlrpc_array_get_item(env, names, i);
        if (env->fault_occurred)
            return 0;
        xmlrpc_parse_value(env, name_val, "s", &name);
        if (env->fault_occurred)
            return 0;
        name_vector.push_back(stdString(name));
    }
    // Build results
    switch (how)
    {
        case HOW_RAW:
            return get_channel_data(env, key, name_vector, start, end,
                                    actual_count, ReaderFactory::Raw, 0.0);
        case HOW_SHEET:
            return get_sheet_data(env, key, name_vector, start, end,
                                  actual_count, ReaderFactory::Raw, 0.0);
        case HOW_AVERAGE:
            return get_sheet_data(env, key, name_vector, start, end,
                                  actual_count,
                                  ReaderFactory::Average, (end-start)/count);
        case HOW_PLOTBIN:
            // 'count' = # of bins, resulting in up to 4*count samples
            return get_channel_data(env, key, name_vector, start, end,
                                    actual_count*4,
                                    ReaderFactory::Plotbin, (end-start)/count);
        case HOW_LINEAR:
            return get_sheet_data(env, key, name_vector, start, end,
                                  actual_count,
                                  ReaderFactory::Linear, (end-start)/count);
    }
    xmlrpc_env_set_fault_formatted(env, ARCH_DAT_ARG_ERROR,
                                   "Invalid how=%d", how);
    return 0;
}

