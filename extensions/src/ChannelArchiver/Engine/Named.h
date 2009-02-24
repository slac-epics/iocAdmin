#ifndef NAMED_H_
#define NAMED_H_

// Tools
#include <ToolsConfig.h>

/**\ingroup Engine
 *  Abstract base for a named thingy, does not actually contain the name.
 *  <p>
 *  Probably needs to be used as a 'virtual' base class,
 *  because several interfaces include 'Named', but a
 *  class that implements several such interfaces
 *  should still have only one name.
 */
class Named
{
public:
    virtual ~Named();
    /** @return Returns the name. */
    virtual const stdString &getName() const = 0;
};

/**\ingroup Engine
 *  A named thingy.
 */
class NamedBase : public Named
{
public:
    /** Create a Named thing with given name. */
    NamedBase(const char *name);
    
    /** @return Returns the name. */
    const stdString &getName() const;
private:
    stdString  name;
};

#endif /*NAMED_H_*/
