// -*- c++ -*-

// ArchiveDataServer.h

// XML-RPC
#include <xmlrpc.h>
// EPICS Base
#include <epicsTime.h>
// Storage
#include <Index.h>
// XMPRPCServer
#include "ServerConfig.h"

/// \defgroup DataServer
/// Code related to the network data server

// The version of this server
#define ARCH_VER 0

// Code numbers for 'how'
// Raw data, channel by channel
#define HOW_RAW      0
// Raw data in 'filled' spreadsheet
#define HOW_SHEET    1
// Averaged spreadsheet
#define HOW_AVERAGE  2
// Plot-binned, channel by channel
#define HOW_PLOTBIN  3
// Linear interpolation spreadsheet
#define HOW_LINEAR   4

// XML-RPC does not define fault codes.
// The xml-rpc-c library uses -500, -501, ... (up to -510)
#define ARCH_DAT_SERV_FAULT -600
#define ARCH_DAT_NO_INDEX -601        
#define ARCH_DAT_ARG_ERROR -602
#define ARCH_DAT_DATA_ERROR -603

// Type codes returned by archiver.get_values
#define XML_STRING 0
#define XML_ENUM 1
#define XML_INT 2
#define XML_DOUBLE 3

// meta.type as returned by archiver.get_values
#define META_TYPE_ENUM    0
#define META_TYPE_NUMERIC 1

void epicsTime2pieces(const epicsTime &t,
                      xmlrpc_int32 &secs, xmlrpc_int32 &nano);

// Inverse to epicsTime2pieces
void pieces2epicsTime(xmlrpc_int32 secs, xmlrpc_int32 nano, epicsTime &t);

// NOTE: User of the ArchiverDataServer.cpp code must provide
// implementations of these routines!
// ---------------------------------------------
const char *get_config_name(xmlrpc_env *env);
bool get_config(xmlrpc_env *env, ServerConfig &config);
// Return open index for given key or 0
Index *open_index(xmlrpc_env *env, int key);
// ---------------------------------------------

// { int32  ver, string desc } = archiver.info()
xmlrpc_value *get_info(xmlrpc_env *env, xmlrpc_value *args, void *user);

// {int32 key, string name, string path}[] = archiver.archives()
xmlrpc_value *get_archives(xmlrpc_env *env, xmlrpc_value *args, void *user);

// {string name, int32 start_sec, int32 start_nano,
//               int32 end_sec,   int32 end_nano}[]
// = archiver.names(int32 key, string pattern)
xmlrpc_value *get_names(xmlrpc_env *env, xmlrpc_value *args, void *user);

// very_complex_array = archiver.values(key, names[], start, end, ...)
xmlrpc_value *get_values(xmlrpc_env *env, xmlrpc_value *args, void *user);


