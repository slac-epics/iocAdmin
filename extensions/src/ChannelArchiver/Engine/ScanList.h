#ifndef __SCANLIST_H__
#define __SCANLIST_H__

// Base
#include <epicsTime.h>
// Tools
#include <ConcurrentList.h>
// Engine
#include "Named.h"

/**\ingroup Engine
 *  Interface for something that can be placed on a ScanList.
 *  <p>
 *  Uses 'virtual Named' so that the user can implement this
 *  interface together with other 'Named' interfaces and still
 *  only get one 'Named' base.
 * 
 *  @see ScanList
 */
class Scannable : public virtual Named
{
public:
    /** Invoked whenever a scan is due. */
    virtual void scan() = 0;
};

/**\ingroup Engine
 *  A ScanList keeps track of Scannable items.
 *  <p>
 *  It does not spawn new threads, somebody else needs to check
 *  when the next scan is due and then invoke scan() in time.
 *  <p>
 *  The ScanList is self-protecting against concurrent access,
 *  based on the ConcurrentList used to maintain items to scan.
 */
class ScanList
{
public:
    ScanList();
    ~ScanList();

    /** Add an item to the scan list.
     *  @param item The item to scan
     *  @param period The requested scan period in seconds.
     */
    void add(Scannable *item, double period);

    /** Does the scan list contain anyting? */
    bool isDueAtAll()
    {   return ! lists.isEmpty(); }
    
    /** When should scan() be called ? */
    const epicsTime &getDueTime();

    /** Remove an item from the ScanList */
    void remove(Scannable *item);

    /** Scan all channels that are due at/after deadline. */
    void scan(const epicsTime &deadline);

    void dump();

private:
    ConcurrentList<class SinglePeriodScanList> lists;
    epicsTime next_list_scan;
};

#endif //__SCANLIST_H__
