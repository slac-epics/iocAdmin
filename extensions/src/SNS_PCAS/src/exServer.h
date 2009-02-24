//
//  Example EPICS CA server
//
//
//  caServer
//  |
//  exServer
//
//  casPV
//  |
//  exPV-----------
//  |             |
//  exScalarPV    exVectorPV
//
//  casChannel
//  |
//  exChannel
//

// ANSI C
#include <string.h>
#include <stdio.h>
#include <cmath>

// EPICS
#include "gddAppFuncTable.h"
#include "epicsTimer.h"
#include "casdef.h"
#include "epicsAssert.h"
#include "resourceLib.h"
#include "tsMinMax.h"
#include "smartGDDPointer.h"

#ifndef NELEMENTS
#   define NELEMENTS(A) (sizeof(A)/sizeof(A[0]))
#endif

//-----------------------------------------------
//Gauss random generartor
//-----------------------------------------------

double randGauss();

// info about all pv in this server
enum excasIoType {excasIoSync};

class exPV;
class exServer;

// pvInfo
class pvInfo {
public:
    pvInfo (double scanPeriodIn, const char *pNameIn,
            aitFloat64 errValIn, 
            aitFloat64 hoprIn, aitFloat64 loprIn, 
            excasIoType ioTypeIn, unsigned countIn) :
            scanPeriod(scanPeriodIn), pName(pNameIn),
            errVal(errValIn),
            hopr(hoprIn), lopr(loprIn), ioType(ioTypeIn), 
            elementCount(countIn), pPV(0)
    {
      d_value = new double[countIn];
    }

    ~pvInfo ();

    //
    // for use when MSVC++ will not build a default copy constructor 
    // for this class
    //
    pvInfo (const pvInfo &copyIn) :
        scanPeriod(copyIn.scanPeriod), pName(copyIn.pName), 
        hopr(copyIn.hopr), lopr(copyIn.lopr), 
        ioType(copyIn.ioType), elementCount(copyIn.elementCount),
        pPV(copyIn.pPV)
    {
      d_value = new double[copyIn.elementCount];
      unsigned i;
      for( i=0; i < copyIn.elementCount; i++){
        d_value[i] = copyIn.getValue(i);
      }      
    }

    const double getScanPeriod () const { return this->scanPeriod; }
    const char *getName () const { return this->pName; }
    const double getHopr () const { return this->hopr; }
    const double getLopr () const { return this->lopr; }
    const excasIoType getIOType () const { return this->ioType; }
    const unsigned getElementCount() const 
        { return this->elementCount; }
    void unlinkPV() { this->pPV=NULL; }
    double getValue(int i) const {
      if(i < 0 || i >  ((int) (elementCount - 1))){
        printf("pvInfo getValue method error: i= %d",i);
        exit(-1);
      }
      return this->d_value[i];
    }
    void setValue(int i, double d) {
      if(i < 0 || i >  ((int) (elementCount - 1))){
        printf("pvInfo setValue method error: i= %d",i);
        exit(-1);
      }
      this->d_value[i] = d;
    }
    double getErrLev() { return errVal;}

    exPV *createPV (exServer &exCAS, bool preCreateFlag, bool scanOn);
    void deletePV ();

private:
    const double    scanPeriod;
    const char      *pName;
    double    errVal;
    const double    hopr;
    const double    lopr;
    const excasIoType   ioType;
    const unsigned  elementCount;
    exPV            *pPV;
    double*         d_value;
};

//
// pvEntry 
//
// o entry in the string hash table for the pvInfo
// o Since there may be aliases then we may end up
// with several of this class all referencing 
// the same pv info class (justification
// for this breaking out into a seperate class
// from pvInfo)
//
class pvEntry // X aCC 655
    : public stringId, public tsSLNode<pvEntry> {
public:
    pvEntry (pvInfo  &infoIn, exServer &casIn, 
            const char *pAliasName) : 
        stringId(pAliasName), info(infoIn), cas(casIn) 
    {
        assert(this->stringId::resourceName()!=NULL);
    }

    inline ~pvEntry();

    pvInfo &getInfo() const { return this->info; }
    
    inline void destroy ();

private:
    pvInfo &info;
    exServer &cas;
};


//
// exPV
//
class exPV : public casPV, public epicsTimerNotify, public tsSLNode<exPV> {
public:
    exPV ( pvInfo &setup, bool preCreateFlag, bool scanOn );
    virtual ~exPV();

    void show ( unsigned level ) const;

    //
    // Called by the server libary each time that it wishes to
    // subscribe for PV the server tool via postEvent() below.
    //
    caStatus interestRegister();

    //
    // called by the server library each time that it wishes to
    // remove its subscription for PV value change events
    // from the server tool via caServerPostEvents()
    //
    void interestDelete();

    aitEnum bestExternalType() const;

    //
    // chCreate() is called each time that a PV is attached to
    // by a client. The server tool must create a casChannel object
    // (or a derived class) each time that this routine is called
    //
    // If the operation must complete asynchronously then return
    // the status code S_casApp_asyncCompletion and then
    // create the casChannel object at some time in the future
    //
    //casChannel *createChannel ();

    //
    // This gets called when the pv gets a new value
    //
    caStatus update (smartConstGDDPointer pValue);

    //
    //This method should be overridden in the subclasses
    //to check that the value is inside the limits
    //
    virtual caStatus checkLimits();

    //
    // Gets called when we add noise to the current value
    //
    virtual void scan() = 0;
    
    //
    // If no one is watching scan the PV with 10.0
    // times the specified period
    //
    const double getScanPeriod()
    {
        double curPeriod;

        curPeriod = this->info.getScanPeriod ();
        if ( ! this->interest ) {
            curPeriod *= 10.0L;
        }
        return curPeriod;
    }

    caStatus read (const casCtx &, gdd &protoIn);

    caStatus readNoCtx (smartGDDPointer pProtoIn)
    {
        return this->ft.read (*this, *pProtoIn);
    }

    caStatus write (const casCtx &, const gdd &value);

    void destroy();

    const pvInfo &getPVInfo()
    {
        return this->info;
    }

    const char *getName() const
    {
        return this->info.getName();
    }

    static void initFT();

    //
    // for access control - optional
    //
    casChannel *createChannel (const casCtx &ctx,
        const char * const pUserName, const char * const pHostName);

protected:
    smartConstGDDPointer    pValue;
    epicsTimer              &timer;
    pvInfo &                info; 
    bool                    interest;
    bool                    preCreate;
    bool                    scanOn;
    static epicsTime        currentTime;

    virtual caStatus updateValue (smartConstGDDPointer pValue) = 0;

private:

    //
    // scan timer expire
    //
    expireStatus expire ( const epicsTime & currentTime );

    //
    // Std PV Attribute fetch support
    //
    gddAppFuncTableStatus getPrecision(gdd &value);
    gddAppFuncTableStatus getHighLimit(gdd &value);
    gddAppFuncTableStatus getLowLimit(gdd &value);
    gddAppFuncTableStatus getUnits(gdd &value);
    gddAppFuncTableStatus getValue(gdd &value);
    gddAppFuncTableStatus getEnums(gdd &value);

    //
    // static
    //
    static gddAppFuncTable<exPV> ft;
    static char hasBeenInitialized;
};

//
// exScalarPV
//
class exScalarPV : public exPV {
public:
    exScalarPV ( pvInfo &setup, bool preCreateFlag, bool scanOnIn ) :
            exPV ( setup, preCreateFlag, scanOnIn) {}
    void scan();

    //This method should be overridden in the subclasses
    //to check that the value is inside the limits
    caStatus checkLimits();

private:
    caStatus updateValue (smartConstGDDPointer pValue);

};

//
// exVectorPV
//
class exVectorPV : public exPV {
public:
    exVectorPV ( pvInfo &setup, bool preCreateFlag, bool scanOnIn ) :
            exPV ( setup, preCreateFlag, scanOnIn) {}
    void scan();

    unsigned maxDimension() const;
    aitIndex maxBound (unsigned dimension) const;

    //check limits
    caStatus checkLimits();

private:
    caStatus updateValue (smartConstGDDPointer pValue);
};

//
// exServer
//
class exServer : public caServer {
public:
    exServer (unsigned aliasCount, bool scanOn );
    void initialize();
    ~exServer();

    void show (unsigned level) const;
    pvExistReturn pvExistTest (const casCtx&, const char *pPVName);
    pvAttachReturn pvAttach (const casCtx &ctx, const char *pPVName);

    void installAliasName(pvInfo &info, const char *pAliasName);
    inline void removeAliasName(pvEntry &entry);

    void addPV(double scanPeriodIn, 
               const char *pNameIn,
               aitFloat64 errLevIn, 
               aitFloat64 hoprIn, 
               aitFloat64 loprIn,
               excasIoType ioTypeIn,
	       unsigned countIn);

    void setNumbOfPV(int size);

private:
    resTable<pvEntry,stringId> stringResTbl;
    unsigned simultAsychIOCount;
    bool scanOn;
    bool scanOnIn;
    unsigned aliasCount;

    // list of pre-created PVs
    static pvInfo** pvList;
    static unsigned pvListNElem;
    static unsigned pvListNElemCounter;

};


// exChannel
class exChannel : public casChannel{
public:
    exChannel(const casCtx &ctxIn) : casChannel(ctxIn) {}

    virtual void setOwner(const char * const pUserName, 
        const char * const pHostName);

    virtual bool readAccess () const;
    virtual bool writeAccess () const;

private:
};


// exServer::removeAliasName()
inline void exServer::removeAliasName ( pvEntry &entry )
{
        pvEntry *pE;
        pE = this->stringResTbl.remove ( entry );
        assert ( pE == &entry );
}

//
// pvEntry::~pvEntry()
//
inline pvEntry::~pvEntry()
{
    this->cas.removeAliasName ( *this );
}

//
// pvEntry:: destroy()
//
inline void pvEntry::destroy ()
{
    delete this;
}

inline pvInfo::~pvInfo ()
{
    if ( this->pPV != NULL ) {
        delete this->pPV;
    }
    delete []d_value;
}

inline void pvInfo::deletePV ()
{
    if ( this->pPV != NULL ) {
        delete this->pPV;
    }
}

//----------------------------------------------------
//The casParser class parses input file and create PVs
//---------------------------------------------------- 
class casParser
{
public:
  /// Constructor
  casParser(const char* fl, const char* fl_out);

  /// Destructor
  virtual ~casParser();

  ///Returns number of PVs
  int getNumbOfPVs();

  ///Creates PVs inside the CAS
  void createPVs(exServer *pCAS);

  /// Names of the PVs (we have to keep them for pvInfo)
  /// or we can rewrite constructor for pvInfo (not done yet)
  char** namesOfPVs;

  //protected methods
protected:

  ////define number of PVs and define EPICS variables
  void numbOfPVsDef();

  //protected data
protected:

  //name of the file to parse
  const char* name_fl;

  //name of the file for output information
  const char* name_fl_out;

  //maximal length of string in the input file
  static int const MAX_LENGTH_STRING = 320;

  // maximal length of PV's name
  static int const MAX_LENGTH_PV_NAME = 40;

  // number of PVs defined by the input file
  int numberOfPVs;

};
