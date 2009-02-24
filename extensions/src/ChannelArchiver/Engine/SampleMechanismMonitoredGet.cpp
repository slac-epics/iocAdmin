
// Tools
#include <MsgLogger.h>
// Engine
#include "SampleMechanismMonitoredGet.h"

SampleMechanismMonitoredGet::SampleMechanismMonitoredGet(
    EngineConfig &config, ProcessVariableContext &ctx,
    const char *name, double period)
    : SampleMechanism(config, ctx, name, period, &time_slot_filter),
      time_slot_filter(period, &repeat_filter),
      repeat_filter(config, &time_filter),
      time_filter(config, this)  
{
}

SampleMechanismMonitoredGet::~SampleMechanismMonitoredGet()
{
}

stdString SampleMechanismMonitoredGet::getInfo(Guard &guard)
{
    char per[10];
    snprintf(per, sizeof(per), "%.1f s", period);
    GuardRelease release(__FILE__, __LINE__, guard);

    stdString info;
    info.reserve(200);
    info = "Monitored get, ";
    info += per;
    info += ", PV ";

    Guard pv_guard(__FILE__, __LINE__, pv);
    info += pv.getStateStr(pv_guard);
    info += ", CA ";
    info += pv.getCAStateStr(pv_guard);
    return info;
}

void SampleMechanismMonitoredGet::stop(Guard &guard)
{
    repeat_filter.stop(pv);
    SampleMechanism::stop(guard);
}

void SampleMechanismMonitoredGet::pvConnected(
    ProcessVariable &pv, const epicsTime &when)
{   
    //LOG_MSG("SampleMechanismMonitoredGet(%s): connected\n",
    //        pv.getName().c_str());
    SampleMechanism::pvConnected(pv, when);

    Guard pv_guard(__FILE__, __LINE__, pv);
    if (!pv.isSubscribed(pv_guard))
        pv.subscribe(pv_guard);    
}

void SampleMechanismMonitoredGet::addToFUX(Guard &guard, FUX::Element *doc)
{
    new FUX::Element(doc, "period", "%g", period);
    new FUX::Element(doc, "scan");
}

