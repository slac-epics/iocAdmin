// System
#include <math.h>
// Tools
#include <epicsTimeHelper.h>
#include <MsgLogger.h>
// Engine
#include "ScanList.h"

// #define DEBUG_SCANLIST

/*  List of channels to scan for one period.
 *  <p>
 *  Used internally by the ScanList.
 *  <p>
 *  Uses a ConcurrentList, also uses its lock.
 */
class SinglePeriodScanList
{
public:
    SinglePeriodScanList(double period);
    ~SinglePeriodScanList();
    
    double getPeriod() const
    {
        return period;
    }
    
    epicsTime getNextScantime();

    void add(Scannable *item)
    {
        items.add(item);
    }

    void remove(Scannable *item)
    {
        items.removeIfFound(item);
    }
    
    void scan();

    bool isEmpty()
    {
        return items.isEmpty();
    }

    void dump();
    
private:
    double                    period;    // Scan period in seconds
    ConcurrentList<Scannable> items;     // Items to scan every 'period'
    epicsTime                 next_scan; // Next time this list is due
    void computeNextScantime();
};


SinglePeriodScanList::SinglePeriodScanList(double period)
        : period(period)
{
#ifdef DEBUG_SCANLIST
    LOG_MSG("new SinglePeriodScanList(%g seconds)\n", period);
#endif
    computeNextScantime();
}

SinglePeriodScanList::~SinglePeriodScanList()
{
#ifdef DEBUG_SCANLIST
    LOG_MSG("delete SinglePeriodScanList(%g seconds)\n", period);
#endif
}

epicsTime SinglePeriodScanList::getNextScantime()
{
    Guard guard(__FILE__, __LINE__, items);
    return next_scan;
}

void SinglePeriodScanList::scan()
{
    ConcurrentListIterator<Scannable> i = items.iterator();
    Scannable *scannable;
    while ((scannable = i.next()) != 0)
        scannable->scan();
    computeNextScantime();
}

void SinglePeriodScanList::dump()
{
    printf("Scan List %g sec\n", period);
    ConcurrentListIterator<Scannable> i = items.iterator();
    Scannable *scannable;
    while ((scannable = i.next()) != 0)
        printf("'%s'\n", scannable->getName().c_str());
}

void SinglePeriodScanList::computeNextScantime()
{
    epicsTime now = epicsTime::getCurrent();
    Guard guard(__FILE__, __LINE__, items);
    // Start with simple calculation of next time.
    next_scan += period;
    // Use the more elaborate rounding if that time already passed.
    if (next_scan < now)
        next_scan = roundTimeUp(now, period);
#ifdef DEBUG_SCANLIST
    stdString time;
    epicsTime2string(next_scan, time);
    LOG_MSG("Next due time for %g sec list: %s\n", period, time.c_str());
#endif
}

// --------------------------------------

ScanList::ScanList()
{
    next_list_scan = nullTime;
}

ScanList::~ScanList()
{
    if (!lists.isEmpty())
    {
        LOG_MSG("ScanList not empty while destructed\n");
    }
}

void ScanList::add(Scannable *item, double period)
{
    bool first_item = lists.isEmpty();
    // Check it the channel is already on some
    // list where it needs to be removed
    remove(item);
    // Find a scan list with suitable period,
    // meaning: a scan period that matches the requested
    // period to some epsilon.
    SinglePeriodScanList *list = 0;
    ConcurrentListIterator<SinglePeriodScanList> i = lists.iterator();
    SinglePeriodScanList *spl;
    while ((spl = i.next()) != 0)
    {
        if (fabs(spl->getPeriod() - period) < 0.05)
        {
            list = spl; // found one!
            break;
        }
    }
    if (list == 0) // create new list for this period
    {
        list = new SinglePeriodScanList(period);
        lists.add(list);
    }
    // Add item to list; an existing one or a newly created one.
    list->add(item);
    // Update the next scan time
    epicsTime due = list->getNextScantime();
    Guard guard(__FILE__, __LINE__, lists);
    if (first_item  ||  due < next_list_scan)
    {
        next_list_scan = due;
#ifdef  DEBUG_SCANLIST
        LOG_MSG("Updated the overall due time.\n");
#endif   
    }
}

const epicsTime &ScanList::getDueTime()
{
    Guard guard(__FILE__, __LINE__, lists);
    return next_list_scan;
}

void ScanList::remove(Scannable *item)
{
    ConcurrentListIterator<SinglePeriodScanList> i = lists.iterator();
    SinglePeriodScanList *spl;
    while ((spl = i.next()) != 0)
    {
        // Remove item from every SinglePeriodScanList
        // (should really be at most on one)
        spl->remove(item);
        // In case that shrunk a S.P.S.L to zero, drop that list.
        if (spl->isEmpty())
        {
            lists.remove(spl);
            delete spl;
        }
    }
}

void ScanList::scan(const epicsTime &deadline)
{
    ConcurrentListIterator<SinglePeriodScanList> i = lists.iterator();
#ifdef DEBUG_SCANLIST
    LOG_MSG("ScanList::scan\n");
#endif
    // Reset: Next time any list is due
    epicsTime next_due = nullTime;
    SinglePeriodScanList *spl;
    while ((spl = i.next()) != 0)
    {
        if (deadline >= spl->getNextScantime())
            spl->scan();
        // Update earliest scan time
        if (next_due == nullTime ||
            next_due > spl->getNextScantime())
            next_due = spl->getNextScantime();
    }

    Guard guard(__FILE__, __LINE__, lists);
    next_list_scan = next_due;
#ifdef DEBUG_SCANLIST
    stdString time;
    epicsTime2string(next_list_scan, time);            
    LOG_MSG("Next due '%s'\n", time.c_str());    
#endif
}

void ScanList::dump()
{
    ConcurrentListIterator<SinglePeriodScanList> i = lists.iterator();
    SinglePeriodScanList *spl;
    while ((spl = i.next()) != 0)
        spl->dump();
}

// EOF ScanList.cpp
