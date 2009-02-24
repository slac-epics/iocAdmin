/* Bad code, don't use,
 * only intended for performance tests
 */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <xmlrpc.h>
#include <xmlrpc_client.h>

#define URL "http://localhost/cgi-bin/xmlrpc/DummyServer.cgi"
//#define URL "http://localhost/cgi-bin/xmlrpc/ArchiveServer.cgi"

void die_if_fault_occurred(xmlrpc_env *env)
{
    if (env->fault_occurred)
    {
        fprintf(stderr, "XML-RPC Fault: %s (%d)\n",
                env->fault_string, env->fault_code);
        exit(1);
    }
}

// Invoke archiver.info
void get_version(xmlrpc_env *env)
{
    xmlrpc_value *result = xmlrpc_client_call(env, URL, "archiver.info", "()");
    die_if_fault_occurred(env);
    
    xmlrpc_int32 ver;
    char *desc;
    size_t len;
    xmlrpc_parse_value(env, result, "{s:i,s:s#,*}",
                       "ver", &ver,
                       "desc", &desc, &len);
    die_if_fault_occurred(env);
    printf("Version %d, Desc. '%s' (%d)\n", ver, desc, len);
    xmlrpc_DECREF(result);
}

// Dump 'meta' part of returned value
void show_meta(xmlrpc_env *env, xmlrpc_value *meta)
{
    xmlrpc_int32 type;
    xmlrpc_parse_value(env, meta, "{s:i,*}", "type", &type);
    if (type == 1)
    {
        double disp_high, disp_low, alarm_high, alarm_low, warn_high, warn_low;
        xmlrpc_int32 prec;
        char *units;   
        xmlrpc_parse_value(env, meta, "{s:d,s:d,s:d,s:d,s:d,s:d,s:i,s:s,*}",
                           "disp_high", &disp_high,
                           "disp_low", &disp_low,
                           "alarm_high", &alarm_high,
                           "alarm_low", &alarm_low,
                           "warn_high", &warn_high,
                           "warn_low", &warn_low,
                           "prec", &prec,
                           "units", &units);
        die_if_fault_occurred(env);
        printf("CtrlInfo: display %g ... %g\n",
               disp_low, disp_high);
        printf("          alarm   %g ... %g\n",
               alarm_low, alarm_high);
        printf("          Warning %g ... %g\n",
               warn_low, warn_high);
        printf("          prec    %d, units '%s'\n",
               prec, units);
    }
    else
    {
        printf("unknown meta type %d\n", type);
    }
}

// Dump 'values' part of returned value
void show_data(xmlrpc_env *env, xmlrpc_int32 type,
               xmlrpc_int32 count, xmlrpc_value *data_array)
{
    int i, num;
    int v, v_num;
    xmlrpc_int32 stat, sevr, secs, nano;
    xmlrpc_value *data, *value_array, *value;
    double dbl_val;

    switch (type)
    {
        case 3:
            num = xmlrpc_array_size(env, data_array);
            for (i=0; i<num; ++i)
            {
                data = xmlrpc_array_get_item(env, data_array, i);
                xmlrpc_parse_value(env, data, "{s:i,s:i,s:i,s:i,s:A,*}",
                                   "stat", &stat, "sevr", &sevr,
                                   "secs", &secs, "nano", &nano,
                                   "value", &value_array);
                die_if_fault_occurred(env);
                struct tm *tm = localtime((time_t *)&secs);
                printf("%02d/%02d/%04d %02d:%02d:%02d.%09ld %d/%d",
                       tm->tm_mon + 1,
                       tm->tm_mday,
                       tm->tm_year + 1900,
                       tm->tm_hour,
                       tm->tm_min,
                       tm->tm_sec,
                       (long)nano, stat, sevr);
                v_num = xmlrpc_array_size(env, value_array);
                for (v=0; v<v_num; ++v)
                {
                    value = xmlrpc_array_get_item(env, value_array, v);
                    xmlrpc_parse_value(env, value, "d", &dbl_val);
                    printf("\t%g", dbl_val);
                }
                printf("\n");
            }
            break;
    }
}

// Dump the values for one channel as returned by archiver.get_values
void show_values(xmlrpc_env *env, xmlrpc_value *value)
{
    char *name;
    xmlrpc_value *meta, *data;
    xmlrpc_int32 type, count;
    xmlrpc_parse_value(env, value, "{s:s,s:V,s:i,s:i,s:A,*}",
                       "name", &name,
                       "meta", &meta,
                       "type", &type,
                       "count", &count,
                       "values", &data);
    die_if_fault_occurred(env);
    
    printf("%s:     type/count %d/%d\n", name, type, count);
    show_meta(env, meta);
    show_data(env, type, count, data);
}

// Test archiver.get_values
void get_values(xmlrpc_env *env, xmlrpc_int32 count_to_get)
{
    xmlrpc_int32 end = time(0);
    xmlrpc_int32 start = end - count_to_get;
    xmlrpc_value *result = xmlrpc_client_call(env, URL, "archiver.values",
                                              "(i(ssss)iiiiii)",
                                              (xmlrpc_int32)42,
                                              "fred","freddy","Jimmy","James",
                                              start, (xmlrpc_int32) 2,
                                              end, (xmlrpc_int32) 4,
                                              count_to_get, (xmlrpc_int32)0);
    die_if_fault_occurred(env);
    int channel_count = xmlrpc_array_size(env, result);
    int i;
    xmlrpc_value *channel_result;
    for (i=0; i<channel_count; ++i)
    {
        channel_result = xmlrpc_array_get_item(env, result, i);
        show_values(env, channel_result);
    }    
}

int main (int argc, char** argv)
{
    int num;

    if (argc != 2)
    {
        fprintf(stderr, "Usage: bench count\n");
        return -1;
    }
    num = atoi(argv[1]);
    
    xmlrpc_env env;

    // W/o this call, we can't get more than ~ 4 x 300 values
    xmlrpc_limit_set(XMLRPC_XML_SIZE_LIMIT_ID, 10000*1024);
    
    xmlrpc_client_init(XMLRPC_CLIENT_NO_FLAGS, "archiver test", "1");
    xmlrpc_env_init(&env);

    get_version(&env);
    get_values(&env, num);
    
    xmlrpc_env_clean(&env);
    xmlrpc_client_cleanup();
        
    return 0;
}



