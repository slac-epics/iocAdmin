// Local
#include "Named.h"

Named::~Named()
{}

NamedBase::NamedBase(const char *name) : name(name)
{
}

const stdString &NamedBase::getName() const
{
    return name;
}

