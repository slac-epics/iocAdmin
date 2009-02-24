/*
 * Miscellaneous utility routines to supplement EPICS base
 */

#include "iocsh.h"
#include "epicsExport.h"

#ifdef __rtems__
/*
 * RTEMS utility routines for EPICS
 *
 * Supplies routines that are present in vxWorks but missing in RTEMS.
 */

#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/route.h>
#include <arpa/inet.h>
#include <rtems/rtems_bsdnet.h>
/*
 * Add a route.  Destination and gateway must be in internet dot notation
 * (node names not supported). Type of service (tos) is not used.  
 */
int mRouteAdd (char * destination, char * gateway, long netmask,
               int tos, int flags)
{
    struct sockaddr_in destination_in;
    struct sockaddr_in netmask_in;
    struct sockaddr_in gateway_in;

    memset (&destination_in, '\0', sizeof destination_in);
    destination_in.sin_len = sizeof destination_in;
    destination_in.sin_family = AF_INET;
    /* Check for a default gateway */
    if (destination == 0) destination_in.sin_addr.s_addr = INADDR_ANY;
    else destination_in.sin_addr.s_addr = inet_addr(destination);
    
    memset (&gateway_in, '\0', sizeof gateway_in);
    gateway_in.sin_len = sizeof gateway_in;
    gateway_in.sin_family = AF_INET;
    gateway_in.sin_addr.s_addr = inet_addr(gateway);

    memset (&netmask_in, '\0', sizeof netmask_in);
    netmask_in.sin_len = sizeof netmask_in;
    netmask_in.sin_family = AF_INET;
    netmask_in.sin_addr.s_addr = netmask;

    if (rtems_bsdnet_rtrequest (
              RTM_ADD,
              (struct sockaddr *)&destination_in,
              (struct sockaddr *)&gateway_in,
              (struct sockaddr *)&netmask_in,
              flags, 0) < 0) {
          printf ("Can't add route using gateway %s: %s\n", gateway,
                  strerror (errno));
          return -1;
    }
    return 0;
}
/*
 * Delete a route.  Destination must be in internet dot notation
 * (node names not supported). Type of service (tos) is not used.  
 */
int mRouteDelete (char * destination, long netmask, int tos, int flags)
{
    struct sockaddr_in destination_in;
    struct sockaddr_in netmask_in;
    struct sockaddr_in gateway_in;

    memset (&destination_in, '\0', sizeof destination_in);
    destination_in.sin_len = sizeof destination_in;
    destination_in.sin_family = AF_INET;
    /* Check for a default gateway */
    if (destination == 0) destination_in.sin_addr.s_addr = INADDR_ANY;
    else destination_in.sin_addr.s_addr = inet_addr(destination);
    
    memset (&gateway_in, '\0', sizeof gateway_in);
    gateway_in.sin_len = sizeof gateway_in;
    gateway_in.sin_family = AF_INET;
    gateway_in.sin_addr.s_addr = INADDR_ANY;

    memset (&netmask_in, '\0', sizeof netmask_in);
    netmask_in.sin_len = sizeof netmask_in;
    netmask_in.sin_family = AF_INET;
    netmask_in.sin_addr.s_addr = netmask;

    if (rtems_bsdnet_rtrequest (
              RTM_DELETE,
              (struct sockaddr *)&destination_in,
              (struct sockaddr *)&gateway_in,
              (struct sockaddr *)&netmask_in,
              flags, 0) < 0) {
          printf ("Can't delete route for destination %s: %s\n", destination,
                  strerror (errno));
          return -1;
    }
    return 0;
}

/* Register for iocsh - mRouteAdd */
static const iocshArg mRouteAddArg0    = {"destination", iocshArgString};
static const iocshArg mRouteAddArg1    = {"gateway"    , iocshArgString};
static const iocshArg mRouteAddArg2    = {"netmask"    , iocshArgInt};
static const iocshArg mRouteAddArg3    = {"tos"        , iocshArgInt};
static const iocshArg mRouteAddArg4    = {"flags"      , iocshArgInt};
static const iocshArg * const mRouteAddArgs[5]    = {
  &mRouteAddArg0   , &mRouteAddArg1   , &mRouteAddArg2   , &mRouteAddArg3   ,
  &mRouteAddArg4};
static const iocshFuncDef mRouteAddFuncDef =
    {"mRouteAdd"   , 5, mRouteAddArgs};
static void mRouteAddCallFunc   (const iocshArgBuf *args)
{
  mRouteAdd(args[0].sval, args[1].sval,
            args[2].ival, args[3].ival, args[4].ival);
}

/* Register for iocsh - mRouteDelete */
static const iocshArg mRouteDeleteArg0 = {"destination", iocshArgString};
static const iocshArg mRouteDeleteArg1 = {"netmask"    , iocshArgInt};
static const iocshArg mRouteDeleteArg2 = {"tos"        , iocshArgInt};
static const iocshArg mRouteDeleteArg3 = {"flags"      , iocshArgInt};
static const iocshArg * const mRouteDeleteArgs[4] = {
  &mRouteDeleteArg0, &mRouteDeleteArg1, &mRouteDeleteArg2, &mRouteDeleteArg3};
static const iocshFuncDef mRouteDeleteFuncDef =
    {"mRouteDelete", 4, mRouteDeleteArgs};
static void mRouteDeleteCallFunc(const iocshArgBuf *args)
{
  mRouteDelete(args[0].sval, args[1].ival, args[2].ival, args[3].ival);
}
#endif  /* __rtems__ */

static void miscUtilsRegistrar(void) {
#ifdef __rtems__
    iocshRegister(&mRouteAddFuncDef   , mRouteAddCallFunc);
    iocshRegister(&mRouteDeleteFuncDef, mRouteDeleteCallFunc);
#endif
}
epicsExportRegistrar(miscUtilsRegistrar);
