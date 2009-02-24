// ================================================================================
//
//   Abs:  Functions to format real numbers.
//
//   Name: format_real.cc
//         Entries: format_real given float
//                  format_real given double
//         Static:  format_real_prec which actually does the work
//
//   Proto: format_real.hh
//
//   Auth: 25-Jul-2002, Mike Zelazny (zelazny@slac.stanford.edu)
//   Rev:  DD-MMM-YYYY, Reviewer's Name (.NE. Author's Name)
//
// --------------------------------------------------------------------------------
//
//   Mod:  (newest to oldest)
//         08-Jan-2003, James Silva (V2.02)
//           Added ANSI C header includes to make code Linux compliant.
//         14-Oct-2002, Mike Zelazny (zelazny):
//           Doubles without decimal parts are now returned as whole numbers as 
//           per Stephanie Allison.
//
// ================================================================================

#include <string.h>      // for memset, memmove, strlen, memcpy
#include <stdio.h>       // for sprintf
#include <math.h>        // for log10

#include <iostream.h>    // for memset, fasb, sprintf, memmove, strlen, and memcpy
#include <db_access.h>   // for EPICS variable types

#include "Debug.hh"

#define MIN(a,b) (a)<(b)?(a):(b)

//=============================================================================
//
//  Abs:  Format the real given precision
//
//  Name: format_real_prec
//
//  Args: value              Real number to be formatted
//          Use:  double
//          Type: double
//          Acc:  read-only
//          Mech: value
//        string             Character representation of above value
//          Use:  char *
//          Type: char *
//          Acc:  write-only
//          Mech: reference
//        width              Number of characters the string has allocated.
//          Use:  size_t     Note that it's up to the caller to supply the
//          Type: size_t     memory for the formatted real number.
//          Acc:  read-only
//          Mech: value
//        prec               Number of significant digits your real has.
//          Use:  unsigned   Note that Ron Chestnut and Stephanie Allison
//          Type: unsigned   chose 7 digits for floats and 15 digits for
//          Acc:  read-only  doubles.  Ask them why.
//          Mech: value
//
//  Rem:  Our belief is that this function actually does a better job than "%g" format
//
//  Side: none.
//
//  Ret:  status from sprintf.
//
//==============================================================================

static int format_real_prec (double value, char * string, size_t width, unsigned short prec) {


  int status = 0;
  Debug debug; // if you remove this line, please be sure to re-test on Linux

  // make sure there's some place to write to
  if (!string) return status;

  dbr_string_t work;
  memset (work, 0, sizeof(work));
  memset (string, 0, (int)width);
  int exp = log10(fabs(value));
  int nexp = (exp > 0) ? ((exp > 9) ? 2 : 1) : ((exp >= -9) ? 1 : 2);
  int signSpace = (value < 0) ? 1 : 0;
  int lhs = 1; // number of significant digits to the left of the decimal point when using %*.*f format

  if (0 == value) {
    status = sprintf (string, "0");
    return status;
    };

  if ((exp > ((int)width-3-signSpace)) || (exp < 0)) {
    status = sprintf (work, "%-*.*e", (int)width, MIN((prec-1),(int)width-4-nexp-signSpace), value);
    if (1 == nexp) // remove extra 0 in e+0n
      for (int i=sizeof(work)-1; i >= 0; i=i-1) 
        if (('e' == work[i]) && ('0' == work[2+i])) {
          memmove (work+2+i, work+3+i, 2);
          break;
          };
    }
  else {
    if ((value < 1) && (value > -1)) lhs = 0;
    status = sprintf (work, "%-*.*f", (int)width, MIN((prec-exp-lhs),(int)width), value);
    };

  // replace trailing spaces and zeroes with null's
  if (strstr(work,"e")) {
    for (int i=sizeof(work)-1; i >= 0; i=i-1)
      if (0x20 == work[i]) work[i] = 0;
    }
  else {
    for (int i=sizeof(work)-1; i >= 0; i=i-1)
      if ((0 == work[i]) || (0x20 == work[i]) || ('0' == work[i])) work[i] = 0;
      else break;
    if ('.' == work[strlen(work)-1]) work[strlen(work)-1] = 0; // remove trailing `.', number is whole
    };;

  // get rid of extra zeroes before the e+nn
  for (int i=sizeof(work)-1; i > 0; i=i-1)
    if (('e' == work[i]) && ('0' == work[i-1]) && ('.' != work[i-2])) memmove (work+i-1, work+i, 1+strlen(work)-i); // found 0e+nn or 0e+n

  memcpy (string, work, width); 
  return status;
  }

//=============================================================================
//
//  Abs:  Format a double.
//
//  Name: format_real given dbr_double_t
//
//  Args: value              Double to be formatted
//          Use:  double
//          Type: dbr_double_t
//          Acc:  read-only
//          Mech: value
//        string             Character representation of above value
//          Use:  char *
//          Type: char *
//          Acc:  write-only
//          Mech: reference
//        width              Number of characters the string has allocated.
//          Use:  size_t     Note that it's up to the caller to supply the
//          Type: size_t     memory for the formatted double.
//          Acc:  read-only
//          Mech: value
//
//  Rem:  Our belief is that this function actually does a better job than "%g" format
//
//  Side: none.
//
//  Ret:  status from format_real_prec
//
//==============================================================================

int format_real (dbr_double_t value, char * string, size_t width) {
  return format_real_prec (value, string, width, 15);
  }

//=============================================================================
//
//  Abs:  Format a float.
//
//  Name: format_real given dbr_float_t
//
//  Args: value              float to be formatted
//          Use:  float
//          Type: dbr_float_t
//          Acc:  read-only
//          Mech: value
//        string             Character representation of above value
//          Use:  char *
//          Type: char *
//          Acc:  write-only
//          Mech: reference
//        width              Number of characters the string has allocated.
//          Use:  size_t     Note that it's up to the caller to supply the
//          Type: size_t     memory for the formatted float.
//          Acc:  read-only
//          Mech: value
//
//  Rem:  Our belief is that this function actually does a better job than "%g" format
//
//  Side: none.
//
//  Ret:  status from format_real_prec
//
//==============================================================================

int format_real (dbr_float_t value, char * string, size_t width) {
  return format_real_prec (value, string, width, 7);
  }
