#ifndef __ENGINESERVER_H__
#define __ENGINESERVER_H__

// Engine
#include <HTTPServer.h>

/**\ingroup Engine
 *  HTTP Server of the Engine.
 */
class EngineServer : public HTTPServer
{
public:
    /** Constructor creates and starts the HTTPServer for given engine. */
	EngineServer(short port, class Engine *engine);

    /** Stop and delete the HTTPServer. */
	~EngineServer();
};

#endif //__ENGINESERVER_H__
