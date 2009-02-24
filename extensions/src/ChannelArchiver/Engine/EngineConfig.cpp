// System
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
// Tools
#include <MsgLogger.h>
#include <FUX.h>
// Engine
#include "EngineConfig.h"

void EngineConfig::addToFUX(FUX::Element *doc)
{
    new FUX::Element(doc, "write_period", "%g", getWritePeriod());    
    new FUX::Element(doc, "get_threshold", "%g", getGetThreshold());
    new FUX::Element(doc, "file_size", "%g",
                     getFileSizeLimit()/1024.0/1024.0);
    new FUX::Element(doc, "ignored_future", "%g",
                     getIgnoredFutureSecs()/60.0/60.0);
    new FUX::Element(doc, "buffer_reserve", "%zu",
                     (size_t)getBufferReserve());
    new FUX::Element(doc, "max_repeat_count", "%zu",
                     (size_t)getMaxRepeatCount());
    if (getDisconnectOnDisable())
        new FUX::Element(doc, "disconnect");
}

EngineConfigParser::EngineConfigParser()
    : listener(0)
{
}

static bool parse_double(const stdString &t, double &n)
{
    const char *s = t.c_str();
    char *e;
    n = strtod(s, &e);
    return e != s  &&  n != HUGE_VAL  && n != -HUGE_VAL;
}

void EngineConfigParser::read(const char *filename,
                              EngineConfigListener *listener)
{
    this->listener = listener;
    // We depend on the XML parser to validate.
    // The asserts are only the ultimate way out.
    FUX fux;
    FUX::Element *e, *doc = fux.parse(filename);
    if (!(doc && doc->getName() == "engineconfig"))
        throw GenericException(__FILE__, __LINE__,
                               "Cannot parse '%s'\n", filename);
    stdList<FUX::Element *>::const_iterator els;
    double d;
    for (els=doc->getChildren().begin(); els!=doc->getChildren().end(); ++els)
    {
        e = *els;
        if (e->getName() == "write_period")
        {
            if (!parse_double(e->getValue(), d))
                throw GenericException(__FILE__, __LINE__,
                                       "'%s': Error in write_period\n",
                                       filename);
            setWritePeriod(d < 0 ? 1.0 : d);
        }
        else if (e->getName() == "get_threshold")
        {
            if (!parse_double(e->getValue(), d))
                throw GenericException(__FILE__, __LINE__,
                                       "'%s': Error in get_threshold\n",
                                       filename);
            setGetThreshold(d);
        }
        else if (e->getName() == "file_size")
        {
            if (!parse_double(e->getValue(), d))
                throw GenericException(__FILE__, __LINE__,
                                       "'%s': Error in file_size\n",
                                       filename);
            setFileSizeLimit((size_t)(d*1024*1024));
        }
        else if (e->getName() == "ignored_future")
        {
            if (!parse_double(e->getValue(), d))
                throw GenericException(__FILE__, __LINE__,
                                       "'%s': Error in ignored_future\n",
                                       filename);
            setIgnoredFutureSecs(d*60*60);
        }
        else if (e->getName() == "buffer_reserve")
        {
            if (!parse_double(e->getValue(), d))
                throw GenericException(__FILE__, __LINE__,
                                       "'%s': Error in buffer_reserve\n",
                                       filename);
            setBufferReserve((size_t)d);
        }
        else if (e->getName() == "max_repeat_count")
        {
            if (!parse_double(e->getValue(), d))
                throw GenericException(__FILE__, __LINE__,
                                       "'%s': Error in max_repeat_count\n",
                                       filename);
            setMaxRepeatCount((size_t) d);
        }
        else if (e->getName() == "disconnect")
            setDisconnectOnDisable(true);
        else if (e->getName() == "group")
            handle_group(e);
    }
}

void EngineConfigParser::handle_group(FUX::Element *group)
{
    stdList<FUX::Element *>::const_iterator els;
    els=group->getChildren().begin();
    LOG_ASSERT((*els)->getName() == "name");
    const stdString &group_name = (*els)->getValue();
    ++els;
    while (els!=group->getChildren().end())
    {
        LOG_ASSERT((*els)->getName() == "channel");
        handle_channel(group_name, (*els));
        ++els;
    }
}

void EngineConfigParser::handle_channel(const stdString &group_name,
                                        FUX::Element *channel)
{
    stdList<FUX::Element *>::const_iterator els;
    els = channel->getChildren().begin();
    LOG_ASSERT((*els)->getName() == "name");
    const stdString &channel_name = (*els)->getValue();
    ++els;
    LOG_ASSERT((*els)->getName() == "period");
    double period;
    if (!parse_double((*els)->getValue(), period))
        throw GenericException(__FILE__, __LINE__,
                               "Group '%s': Error in period for channel '%s'\n",
                               group_name.c_str(), channel_name.c_str());
    ++els;
    bool monitor = (*els)->getName() == "monitor";
    bool disabling = false;
    ++els;
    while (els != channel->getChildren().end())
    {
        if ((*els)->getName() == "disable")
            disabling = true;
        ++els;
    }
    if (listener)
        listener->addChannel(group_name, channel_name,
                             period, disabling, monitor);
}
