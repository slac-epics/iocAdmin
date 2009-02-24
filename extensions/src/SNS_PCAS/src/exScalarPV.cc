
#include <math.h>
#include <limits.h>
#include <stdlib.h>

#include "exServer.h"
#include "gddApps.h"

#define myPI 3.14159265358979323846

//
// SUN C++ does not have RAND_MAX yet
//
#if !defined(RAND_MAX)
//
// Apparently SUN C++ is using the SYSV version of rand
//
#if 0
#define RAND_MAX INT_MAX
#else
#define RAND_MAX SHRT_MAX
#endif
#endif

//
// exScalarPV::scan
//
void exScalarPV::scan()
{
    double          randVal;
    double          errLev;
    smartGDDPointer pDD;
    double          newValue;
    double          limit;
    int             gddStatus;

    //
    // update current time (so we are not required to do
    // this every time that we write the PV which impacts
    // throughput under sunos4 because gettimeofday() is
    // slow)
    //
    this->currentTime = epicsTime::getCurrent();

    pDD = new gddScalar (gddAppType_value, aitEnumFloat64);
    if ( ! pDD.valid () ) {
        return;
    }

    //
    // smart pointer class manages reference count after this point
    //
    gddStatus = pDD->unreference();
    assert (!gddStatus);

    randVal = randGauss();

    //get the value from the info
    newValue = this->info.getValue(0);
    errLev = this->info.getErrLev();

    newValue += newValue*errLev*randVal;

    limit = this->info.getHopr();
    newValue = tsMin (newValue, limit);
    limit = this->info.getLopr();
    newValue = tsMax (newValue, limit);
    *pDD = newValue;
    aitTimeStamp gddts = this->currentTime;
    pDD->setTimeStamp (&gddts);
    this->pValue = pDD;

    // post a value change event
    caServer *pCAS = this->getCAS();
    if (this->interest==true && pCAS!=NULL) {
        casEventMask select(pCAS->valueEventMask()|pCAS->logEventMask());
        this->postEvent (select, *this->pValue);
    }

}

//
// exScalarPV::updateValue ()
//
// NOTES:
// 1) This should have a test which verifies that the 
// incoming value in all of its various data types can
// be translated into a real number?
// 2) We prefer to unreference the old PV value here and
// reference the incomming value because this will
// result in each value change events retaining an
// independent value on the event queue.
//
caStatus exScalarPV::updateValue (smartConstGDDPointer pValueIn)
{
    if ( ! pValueIn.valid () ) {
        return S_casApp_undefined;
    }

    //
    // Really no need to perform this check since the
    // server lib verifies that all requests are in range
    //
    if (!pValueIn->isScalar()) {
        return S_casApp_outOfBounds;
    }

    this->pValue = pValueIn;
    return S_casApp_success;
}

caStatus exScalarPV::checkLimits()
{
    smartGDDPointer pDD;
    double          newValue;
    double          limit;
    int             gddStatus;

    this->currentTime = epicsTime::getCurrent();

    pDD = new gddScalar (gddAppType_value, aitEnumFloat64);
    if ( ! pDD.valid () ) {
        return S_casApp_undefined;
    }

    // smart pointer class manages reference count after this point
    gddStatus = pDD->unreference();
    assert (!gddStatus);

    if ( this->pValue.valid () ) {
        this->pValue->getConvert(newValue);
    }
    else {
        newValue = 0.0;
    }

    limit = (double) this->info.getHopr();
    newValue = tsMin (newValue, limit);
    limit = (double) this->info.getLopr();
    newValue = tsMax (newValue, limit);

    //remember the value in the info
    this->info.setValue(0,newValue);

    *pDD = newValue;
    aitTimeStamp gddts = this->currentTime;
    pDD->setTimeStamp (&gddts);

    this->pValue = pDD;

    return S_casApp_success;
}

