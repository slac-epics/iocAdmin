// System
#include <math.h>
// Base
#include <epicsThread.h>
// Tools
#include <MsgLogger.h>
#include <UnitTest.h>
// Engine
#include "ScanList.h"

class Thing : public Scannable
{
public:
    Thing(const char *name);
    const stdString &getName() const;
    void scan();
    
    size_t scans;
private:
    stdString name;
};

Thing::Thing(const char *name)
    : name(name)
{
    scans = 0;
}

const stdString &Thing::getName() const
{
    return name;
}

void Thing::scan()
{
    ++scans;
    printf("%s gets scanned\n", getName().c_str());
}

TEST_CASE test_scan_list()
{
    ScanList list;
    Thing a("0.5");
    Thing b("1.0");
    Thing c("1.5");
    
    list.add(&a, 0.5);
    list.add(&b, 1.0);
    list.add(&c, 1.5);
    TEST(list.isDueAtAll() == true);
    
    size_t run;
    for (run = 0; run < 150/2; ++run)
    {
        epicsThreadSleep(0.1);
        epicsTime now = epicsTime::getCurrent();
        if (list.getDueTime() < now)
        {
            LOG_MSG("Scanning!\n");
            list.scan(now);
        }
    }
    printf("'%s' got scanned %zu times\n", a.getName().c_str(), a.scans);
    printf("'%s' got scanned %zu times\n", b.getName().c_str(), b.scans);
    printf("'%s' got scanned %zu times\n", c.getName().c_str(), c.scans);
    TEST(fabs(a.scans - 15.0)   < 4);
    TEST(fabs(b.scans - 15.0/2) < 3);
    TEST(fabs(c.scans - 15.0/3) < 2);
    
    list.remove(&c);
    list.remove(&b);
    
    a.scans = 0;
    for (run = 0; run < 10; ++run)
    {
        epicsThreadSleep(0.1);
        epicsTime now = epicsTime::getCurrent();
        if (list.getDueTime() < now)
        {
            LOG_MSG("Scanning!\n");
            list.scan(now);
        }
    }
    TEST(a.scans > 0);
        
    list.remove(&a);
    TEST(list.isDueAtAll() == false);
    TEST_OK;
}

