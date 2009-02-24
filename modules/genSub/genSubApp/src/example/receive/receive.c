#include <stdio.h>
#include <math.h>
#include <time.h>
#include <stdlib.h>

#include <dbEvent.h>
#include <dbDefs.h>
#include <dbCommon.h>
#include <registryFunction.h>
#include <epicsExport.h>
#include <recSup.h>
#include <genSubRecord.h>
#include <pinfo.h>

static int messcounter = 0;

long setupRemoteReceive( genSubRecord *pgsub )
{
  return( sizeof(struct pinfo) );
}

long remoteReceive( genSubRecord *pgsub )
{
  double       *ptr;
  struct pinfo *pex;
  int          i;
  char         (*cptr)[40];

  ptr = (double *)pgsub->a;
  printf("Receiving...through Link A\n");
  for(i=0; i<5; i++ )
    printf("%14.7f\n", *(ptr++));

  pex = (struct pinfo *)pgsub->b;
  printf("Receiving...through Link B\n");
  printf("Name = %s, Age = %d, Salary = %f\n", 
          pex->name, pex->age, pex->salary);
  printf("Position = %s\n", pex->posn);

  cptr = pgsub->c;
  printf("Receiving...through Link C\n");
  for(i=0; i<4; i++)
    printf("Site: %s\n", *(cptr++));
  printf("\n");

  printf("\nReceiving...through Link D\n");
  printf("%ld\n", *((long *)pgsub->d));

  /* This shows that events are posted on VAL */

  messcounter++;
  return(messcounter);
}


/* Register these symbols for use by IOC code */

epicsRegisterFunction( remoteReceive );
