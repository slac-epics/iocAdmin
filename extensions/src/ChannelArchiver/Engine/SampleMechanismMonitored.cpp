
// Tools
#include <MsgLogger.h>
// Engine
#include "SampleMechanismMonitored.h"

SampleMechanismMonitored::SampleMechanismMonitored(
    EngineConfig &config, ProcessVariableContext &ctx,
    const char *name, double period_estimate)
    : SampleMechanism(config, ctx, name, period_estimate, &time_filter),
      time_filter(config, this)
{
}

SampleMechanismMonitored::~SampleMechanismMonitored()
{
}

stdString SampleMechanismMonitored::getInfo(Guard &guard)
{
    char per[10];
    snprintf(per, sizeof(per), "%.1f s", period);

    GuardRelease release(__FILE__, __LINE__, guard);
    stdString info;
    info.reserve(200);
    info = "Monitored, max period ";
    info += per;
    info += ", PV ";

    Guard pv_guard(__FILE__, __LINE__, pv);
    info += pv.getStateStr(pv_guard);
    info += ", CA ";
    info += pv.getCAStateStr(pv_guard);
    return info;
}

void SampleMechanismMonitored::pvConnected(ProcessVariable &pv,
                                           const epicsTime &when)
{
    // LOG_MSG("SampleMechanismMonitored(%s): connected\n", pv.getName().c_str());
    SampleMechanism::pvConnected(pv, when);
    Guard pv_guard(__FILE__, __LINE__, pv);
    if (!pv.isSubscribed(pv_guard))
        pv.subscribe(pv_guard);
}

void SampleMechanismMonitored::addToFUX(Guard &guard, class FUX::Element *doc)
{
    new FUX::Element(doc, "period", "%g", period);
    new FUX::Element(doc, "monitor");
}

