// ==============================================================================
//
//  Abs:  Converts a string stored in a Repository to its Channel Access data 
//        type and vice versa.
//
//  Name: ChannelType.cc
//
//  Proto: ChannelType.hh
//
//  Auth: 30-Sep-2002, Mike Zelazny (zelazny@slac.stanford.edu):
//  Rev:  dd-mmm-19yy, Tom Slick (TXS):
//
// --------------------------------------------------------------------------------
//
//  Mod:
//
// ==============================================================================
//

#include <string.h>

#include <db_access.h>   // for DBR_*
#include <cadef.h>       // for chtype
#include <iostream.h>    // for strstr

#include "ChannelType.hh"

// ==============================================================================
//
//  Abs:  Converts Channel Access data type to a string.
//
//  Name: ChannelType
//
//  Args: type                       Channel Access Data Type
//        Use:  chtype
//        Type: chtype
//        Acc:  read-only
//        Mech: value
//
//  Rem:  A string is stored in the Repositories instead of an int so that a 
//        real person can read the repository without having a CA data type
//        decoder ring handy. People occassionally edit their Repositories to
//        reboot their IOC with different running conditions (like a config).
//
//  Side: none.
//  
//  Ret:  char * string to put into a Repository.
//
// ==============================================================================
//

char * ChannelType (chtype type) {
  if (DBR_STRING == type) return ((char *)"STRING");
  if (DBR_INT == type) return ((char *)"INT");
  if (DBR_SHORT == type) return ((char *)"SHORT");
  if (DBR_FLOAT == type) return ((char *)"FLOAT");
  if (DBR_ENUM == type) return ((char *)"ENUM");
  if (DBR_CHAR == type) return ((char *)"CHAR");
  if (DBR_LONG == type) return ((char *)"LONG");
  if (DBR_DOUBLE == type) return ((char *)"DOUBLE");
  return ((char *)"UNKNOWN");
  }

// ==============================================================================
//
//  Abs:  Converts a string to a Channel Access data type.
//
//  Name: ChannelType
//
//  Args: type                       string returned from function above.
//        Use:  char *
//        Type: char *
//        Acc:  read-only
//        Mech: reference
//
//  Rem:  A string is stored in the Repositories instead of an int so that a 
//        real person can read the repository without having a CA data type
//        decoder ring handy. People occassionally edit their Repositories to
//        reboot their IOC with different running conditions (like a config).
//
//  Side: none.
//  
//  Ret:  chtype from db_access.h
//
// ==============================================================================
//

chtype ChannelType (char * type) {
  if (strstr (type, "STRING")) return DBR_STRING;
  if (strstr (type, "INT")) return DBR_INT;
  if (strstr (type, "SHORT")) return DBR_SHORT;
  if (strstr (type, "FLOAT")) return DBR_FLOAT;
  if (strstr (type, "ENUM")) return DBR_ENUM;
  if (strstr (type, "CHAR")) return DBR_CHAR;
  if (strstr (type, "LONG")) return DBR_LONG;
  if (strstr (type, "DOUBLE")) return DBR_DOUBLE;
  return DBF_NO_ACCESS;
  }
