#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#include <dbEvent.h>
#include <dbDefs.h>
#include <dbCommon.h>
#include <registryFunction.h>
#include <epicsExport.h>
#include <recSup.h>
#include <genSubRecord.h>
#include <pinfo.h>

long setupTransmit( genSubRecord *pgsub )
{
  return( sizeof(struct pinfo) );
}

long setupLocalReceive( genSubRecord *pgsub )
{
  return( sizeof(struct pinfo) );
}

long transmit( genSubRecord *pgsub )
{
  char         sites[4][40];
  double       a[5];
  long         val;
  struct pinfo *ex;

/* Transmit 5 doubles through output link A */

  a[0] = 12345.678;
  a[1] = 12345.897;
  a[2] = 1.0;
  a[3] = 345.43;
  a[4] = 78.5;
  memcpy( pgsub->vala, a, 5*sizeof(double) );

/* Transmit a structure through output link B */

  ex      = (struct pinfo *)calloc(1, sizeof(struct pinfo));
  ex->age = 40;
  strcpy(ex->name, "Sam Jones");
  strcpy(ex->posn, "Senior Software Engineer at the Royal Greenwich Observatory, Cambridge");
  ex->salary = 32350.67;
  memcpy(pgsub->valb, ex, sizeof(struct pinfo));
  free(ex);

/* Transmit 4 strings through output link C */

  strcpy( sites[0], "Royal Greenwich Observatory");
  strcpy( sites[1], "Royal Observatory Edinburgh");
  strcpy( sites[2], "Rutherford Appleton Laboratory");
  strcpy( sites[3], "University College London");
  memcpy(pgsub->valc, sites, 4*40);

/* Transmit a single LONG value through output Link D */

  val = 17;
  memcpy(pgsub->vald, &val, sizeof(long) );

  return(0);
}

long localReceive( genSubRecord *pgsub )
{
  double       *ptr;
  long         *lptr;
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

  lptr = (long *)pgsub->d;
  printf("Receiving...through Link D\n");
  printf("%ld\n", *lptr);

  return(0);
}


/* Register these symbols for use by IOC code */

epicsRegisterFunction( transmit     );
epicsRegisterFunction( localReceive );
