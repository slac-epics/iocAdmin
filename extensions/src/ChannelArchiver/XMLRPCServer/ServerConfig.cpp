// System
#include <stdio.h>
// Tools
#include <MsgLogger.h>
#include <FUX.h>
// Local
#include "ServerConfig.h"

bool ServerConfig::read(const char *filename)
{
    // We depend on the XML parser to validate.
    // The asserts are only the ultimate way out.
    FUX fux;
    FUX::Element *arch, *doc = fux.parse(filename);
    if (!doc)
    {
        LOG_MSG("ServerConfig: Cannot parse '%s'\n", filename);
        return false;
    }
    LOG_ASSERT(doc->getName() == "serverconfig");
    Entry entry;
    stdList<FUX::Element *>::const_iterator archs, e;
    for (archs=doc->getChildren().begin(); archs!=doc->getChildren().end(); ++archs)
    {
        arch = *archs;
        LOG_ASSERT(arch->getName() == "archive");
        e = arch->getChildren().begin();
        LOG_ASSERT((*e)->getName() == "key");
        entry.key = atoi((*e)->getValue().c_str());
        ++e;
        LOG_ASSERT((*e)->getName() == "name");
        entry.name = (*e)->getValue();
        ++e;
        LOG_ASSERT((*e)->getName() == "path");
        entry.path = (*e)->getValue();
        config.push_back(entry);
        entry.clear();
    }
    return true;
}

void ServerConfig::dump()
{
    stdList<Entry>::const_iterator i;
    for (i=config.begin(); i!=config.end(); ++i)
        printf("key %d: '%s' in '%s'\n",
               i->key, i->name.c_str(), i->path.c_str());
}

bool ServerConfig::find(int key, stdString &path)
{
    stdList<Entry>::const_iterator i;
    for (i=config.begin(); i!=config.end(); ++i)
        if (i->key == key)
        {
            path = i->path;
            return true;
        }
    return false;
}


