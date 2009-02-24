#include "gddAppFuncTable.h"
#include "epicsTimer.h"
#include "casdef.h"
#include "epicsAssert.h"
#include "resourceLib.h"
#include "tsMinMax.h"
#include "smartGDDPointer.h"

#include "envDefs.h"

#include "exServer.h"
#include "fdManager.h"

//
// main()
// (example single threaded ca server tool main loop)
//
int main (int argc, const char **argv)
{
    epicsTime   begin (epicsTime::getCurrent());
    exServer    *pCAS;
    casParser   *parser;
    unsigned    debugLevel = 0u;
    double      executionTime;
    char        nameOfFile[128] = "";
    char        nameOfFileOut[128] = "";
    unsigned    aliasCount = 1u;
    unsigned    scanOnAsUnsignedInt = false;
    char        arraySize[64] = "";
    bool        scanOn;
    bool        forever = true;
    int         i;

    for ( i = 1; i < argc; i++ ) {
        if (sscanf(argv[i], "-d %u", &debugLevel)==1) {
            continue;
        }
        if (sscanf(argv[i],"-t %lf", &executionTime)==1) {
            forever = false;
            continue;
        }
        if (sscanf(argv[i],"-c %u", &aliasCount)==1) {
            continue;
        }
        if (sscanf(argv[i],"-s %u", &scanOnAsUnsignedInt)==1) {
            continue;
        }
        if (sscanf(argv[i],"-a %63s", arraySize)==1) {
            continue;
        }
	if (sscanf(argv[i],"-f= %127s", nameOfFile)==1) {
	    continue;
	}
	if (sscanf(argv[i],"-fout= %127s", nameOfFileOut)==1) {
	    continue;
	}

        printf ("\"%s\"?\n", argv[i]);
        printf (
            "usage: %s [-d<debug level> -t<execution time> -f=<nameOfInpFile> " 
            "-fout=<nameOfOutPutFile> -c<numbered alias count> "
            "-s<1=scan on, 0=scan off (default)]> -a<max array size>]\n", 
            argv[0]);

        return (1);
    }

    if ( arraySize[0] != '\0' ) {
        epicsEnvSet ( "EPICS_CA_MAX_ARRAY_BYTES", arraySize );
    }

    //This Set defined in the parser
    //epicsEnvSet ( "EPICS_CA_ADDR_LIST", "160.91.225.149" );
    //epicsEnvSet ( "EPICS_CA_SERVER_PORT", "5080");
    //epicsEnvSet ( "EPICS_CA_AUTO_ADDR_LIST","NO");

    //The parser instance should be created before CAS to provide 
    //settings of epics environment variables
    parser = new casParser(nameOfFile,nameOfFileOut);    


    if (scanOnAsUnsignedInt) {
        scanOn = true;
    }
    else {
        scanOn = false;
    }

    pCAS = new exServer (aliasCount, scanOn );
    if ( ! pCAS ) {
        return (-1);
    }

    //create PVs for CAS
    parser->createPVs(pCAS);

    //initialize CAS
    pCAS->initialize();

    pCAS->setDebugLevel(debugLevel);

    if ( forever ) {
        //
        // loop here forever
        //
        while (true) {
            fileDescriptorManager.process(1000000.0);
        }
    }
    else {
        double delay = epicsTime::getCurrent() - begin;
        //
        // loop here untill the specified execution time
        // expires
        //
        while ( delay < executionTime ) {
            fileDescriptorManager.process ( delay );
            delay = epicsTime::getCurrent() - begin;
        }
    }
    pCAS->show(2u);
    delete pCAS;

    delete parser;

    return (0);
}

