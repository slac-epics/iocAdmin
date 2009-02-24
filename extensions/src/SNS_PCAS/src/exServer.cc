// Example EPICS CA server

#include "exServer.h"

//random seed 
int INITIAL_SEED;

// static list of pre-created PVs
pvInfo** exServer::pvList;
unsigned exServer::pvListNElem = 0u;
unsigned exServer::pvListNElemCounter = 0u;

// exServer::exServer()
exServer::exServer (unsigned aliasCount_in, bool scanOnIn_in ) : 
    simultAsychIOCount (0u),
    scanOn (scanOnIn_in),
    scanOnIn (scanOnIn_in),
    aliasCount(aliasCount_in)
{
  pvListNElem = 0u;
  pvListNElemCounter = 0u;
}

void exServer::addPV(double scanPeriodIn, 
                     const char *pNameIn,
                     aitFloat64 errLevIn, 
                     aitFloat64 hoprIn, 
                     aitFloat64 loprIn,
                     excasIoType ioTypeIn,
                     unsigned countIn) 
{

  if( pvListNElemCounter > (pvListNElem-1) ) return;
  pvList[pvListNElemCounter] = new pvInfo(scanPeriodIn,
                                     pNameIn,errLevIn,hoprIn,
                                     loprIn,ioTypeIn,
                                     countIn);
  pvListNElemCounter++; 
}

void exServer::setNumbOfPV(int size)
{
  pvListNElem = (unsigned) size;
  pvListNElemCounter = 0u;
  pvList = new pvInfo*[pvListNElem];
}

void exServer::initialize() 
{
    char* pvPrefix = "";
    unsigned i,j;
    exPV *pPV;
    pvInfo *pPVI;
    char pvAlias[256];
    const char * const pNameFmtStr = "%.100s%.40s";
    const char * const pAliasFmtStr = "%.100s%.40s%u";

    exPV::initFT();

    // pre-create all of the simple PVs that this server will export
    for (j = 0u; j < pvListNElem; j++){
        pPVI = pvList[j];
        pPV = pPVI->createPV (*this, true, scanOnIn);
        if (!pPV) {
            fprintf(stderr, "Unable to create new PV \"%s\"\n",
                pPVI->getName());
        }

        // Install canonical (root) name
        sprintf(pvAlias, pNameFmtStr, pvPrefix, pPVI->getName());
        this->installAliasName(*pPVI, pvAlias);

        // Install numbered alias names
        for (i=0u; i<aliasCount; i++) {
            sprintf(pvAlias, pAliasFmtStr, pvPrefix,  
                    pPVI->getName(), i);
            this->installAliasName(*pPVI, pvAlias);
        }
    }
}

exServer::~exServer()
{
    int i;
    pvInfo *pPVI;

    // delete all pre-created PVs (eliminate bounds-checker warnings)
    for ( i = 0; i < ((int) pvListNElem); i++){
        pPVI = pvList[i];
        pPVI->deletePV ();
    }
    delete [] pvList;
    
    this->stringResTbl.traverse ( &pvEntry::destroy );
}

void exServer::installAliasName(pvInfo &info, const char *pAliasName)
{
    pvEntry *pEntry;

    pEntry = new pvEntry(info, *this, pAliasName);
    if (pEntry) {
        int resLibStatus;
        resLibStatus = this->stringResTbl.add(*pEntry);
        if (resLibStatus==0) {
            return;
        }
        else {
            delete pEntry;
        }
    }
    fprintf(stderr, 
      "Unable to enter PV=\"%s\" Alias=\"%s\" in PV name alias hash table\n",
      info.getName(), pAliasName);
}

pvExistReturn exServer::pvExistTest
    (const casCtx& ctxIn, const char *pPVName)
{
    // lifetime of id is shorter than lifetime of pName
    stringId id(pPVName, stringId::refString);
    pvEntry *pPVE;

    // Look in hash table for PV name (or PV alias name)
    pPVE = this->stringResTbl.lookup(id);
    if (!pPVE) {
        return pverDoesNotExistHere;
    }

    pvInfo &pvi = pPVE->getInfo();

    // Initiate async IO if this is an async PV
    if (pvi.getIOType() == excasIoSync) {
        return pverExistsHere;
    }
    else {
     fprintf(stderr,"This CAS supports only synchronous PV.\nStop.\n");
     exit(1);
    }
}

pvAttachReturn exServer::pvAttach(const casCtx &ctx, const char *pName)
{
    // lifetime of id is shorter than lifetime of pName
    stringId id(pName, stringId::refString); 
    exPV *pPV;
    pvEntry *pPVE;

    pPVE = this->stringResTbl.lookup(id);
    if (!pPVE) {
        return S_casApp_pvNotFound;
    }

    pvInfo &pvi = pPVE->getInfo();

    // If this is a synchronous PV create the PV now 
    if (pvi.getIOType() == excasIoSync) {
        pPV = pvi.createPV(*this, false, this->scanOn);
        if (pPV) {
            return *pPV;
        }
        else {
            return S_casApp_noMemory;
        }
    }
    // Initiate async IO if this is an async PV
    else {
     fprintf(stderr,"This CAS supports only synchronous PV.\nStop.\n");
     exit(1);
    }
}

exPV *pvInfo::createPV ( exServer & /*cas*/,
                         bool preCreateFlag, bool scanOn )
{
    if (this->pPV) {
        return this->pPV;
    }

    exPV *pNewPV;

    // create an instance of the appropriate class
    // depending on the io type and the number
    // of elements
    if (this->elementCount==1u) {
        switch (this->ioType){
        case excasIoSync:
            pNewPV = new exScalarPV ( *this, preCreateFlag, scanOn );
            break;
        default:
            pNewPV = NULL;
            break;
        }
    }
    else {
        if ( this->ioType == excasIoSync ) {
            pNewPV = new exVectorPV ( *this, preCreateFlag, scanOn );
        }
        else {
            pNewPV = NULL;
        }
    }
    
    //
    // load initial value (this is not done in
    // the constructor because the base class's
    // pure virtual function would be called)
    //
    // We always perform this step even if
    // scanning is disable so that there will
    // always be an initial value
    //
    if (pNewPV) {
        this->pPV = pNewPV;
        pNewPV->scan();
    }

    return pNewPV;
}

//
// exServer::show() 
//
void exServer::show (unsigned level) const
{
    //
    // server tool specific show code goes here
    //
    this->stringResTbl.show(level);

    //
    // print information about ca server libarary
    // internals
    //
    this->caServer::show(level);
}

double randGauss()
{
  double pi,r1,r2, random_value;
  srand(INITIAL_SEED);
  pi = 4*atan(1.0);
  r1 = ((double) rand())/RAND_MAX;
  r1 = -log(1-r1);
  r2 = (2*pi*rand())/RAND_MAX;
  r1 = sqrt(2*r1);
  random_value = r1*cos(r2);
  INITIAL_SEED=rand();
  return random_value;
}
