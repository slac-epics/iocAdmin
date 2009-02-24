// -*- c++ -*-
// $Id: HTTPServer.h,v 1.1.1.1 2006/05/01 18:10:32 luchini Exp $
//
// Please refer to NOTICE.txt,
// included as part of this distribution,
// for legal information.
//
// kasemir@lanl.gov

#if !defined(HTTP_SERVER_H_)
#define HTTP_SERVER_H_

// Base
#include <epicsTime.h>
#include <epicsThread.h>
// Tools
#include <ToolsConfig.h>
#include <NetTools.h>
#include <Guard.h>
#include <AutoPtr.h>
// Engine
#include "HTMLPage.h"

// undef: Nothing
//     1: Start/Stop
//     2: Show Client IPs
//     3: Connect & cleanup
//     4: Requests
//     5: whatever
#define HTTPD_DEBUG 2

// Maximum number of clients that we accept.
// This includes connections that are "done"
// and need to be cleaned up
// (which happens every HTTPD_TIMEOUT seconds).
// Since most clients lock the engine to get the
// list of channels,
// really only one client can run at a time.
// Allowing a few more to queue up is OK,
// but a long list doesn't make any sense.
#define MAX_NUM_CLIENTS 3

// These timeouts influence how quickly the server reacts,
// including how fast the whole engine can be shut down.

// Timeout for server to check for new client
#define HTTPD_TIMEOUT 1

// Client uses timeout for each read
#define HTTPD_READ_TIMEOUT 1

// HTTP clients older than this total timeout are killed
#define HTTPD_CLIENT_TIMEOUT 10

/**\ingroup Engine
 * Used by HTTPClientConnection to dispatch client requests
 *
 * Terminator: entry with path = 0.
 */
typedef struct
{
    typedef void (*PathHandler) (class HTTPClientConnection *connection,
                                 const stdString &full_path,
                                 void *user_arg);
    const char  *path;      // Path for this handler
    size_t      path_len;   // _Relevant_ portion of path to check (if > 0)
    PathHandler handler;    // Handler to call
}   PathHandlerList;

/**\ingroup Engine
 * An in-memory web server.
 *
 * Waits for connections on the given TCP port,
 * creates an HTTPClientConnection
 * for each incoming, well, client.
 *
 * The HTTPClientConnection then needs to handle
 * the incoming requests and produce appropriate
 * and hopefully strikingly beautiful HTML pages.
 */
class HTTPServer : public epicsThreadRunable
{
public:
    /** Create a HTTPServer.
      * 
      * @parm port The TCP port number where the server listens.
      * @exception GenericException when port unavailable.
      * @see start()
      */
    HTTPServer(short port, PathHandlerList *handlers, void *user_arg);
    
    virtual ~HTTPServer();

    /** Start accepting connections (launch thread). */
    void start();

    /** Part of the epicsThreadRunable interface. Do not call directly! */
    void run();

    /** Dump HTML page with server info to socket. */
    void serverinfo(SOCKET socket);

    /** @return Returns the path handlers for this HTTPServer. */
    PathHandlerList *getHandlers() const
    {   return handlers; }
            
    /** @return Returns the user arg passed to the constructor. */    
    void *getUserArg() const
    {   return user_arg; }

    /** @return Returns true if the server wants to shut down. */
    bool isShuttingDown() const
    {   return go == false; }    

private:
    epicsThread                           thread;
    bool                                  go;
    SOCKET                                socket;
    PathHandlerList                       *handlers;
    void                                  *user_arg;
    size_t                                total_clients;
    double                                client_duration; // seconds; averaged
    OrderedMutex                          client_list_mutex;
    AutoArrayPtr< AutoPtr<class HTTPClientConnection> > clients;

    // Create a new HTTPClientConnection, add to 'clients'.
    void start_client(SOCKET peer);

    // returns # of clients that are still active
    size_t client_cleanup();
    
    void reject(SOCKET socket);    
};

/**\ingroup Engine
 * Handler for a HTTPServer's client.
 *
 * Handles input and dispatches
 * to a PathHandler from PathList.
 * It's deleted when the connection is closed.
 */
class HTTPClientConnection : public epicsThreadRunable
{
public:
    HTTPClientConnection(HTTPServer *server, SOCKET socket, int num);
    virtual ~HTTPClientConnection();

    HTTPServer *getServer()
    {   return server; }
    
    SOCKET getSocket()
    {   return socket; }
    
    size_t getNum()
    {   return num; }
    
    bool isDone()
    {   return done; }    

    /** Predefined PathHandlers. */
    void error(const stdString &message);
    void pathError(const stdString &path);
 
    void start()
    {  thread.start(); }

    void run();

    const epicsTime &getBirthTime()
    {   return birthtime; }

    /** @returns The runtime in seconds. */
    double getRuntime() const
    {   return runtime; }
    
    void join();

private:
    epicsThread              thread; // .. that handles this connection
    HTTPServer              *server;
    epicsTime                birthtime;
    size_t                   num;    // unique sequence number of this conn.
    bool                     done;   // has run() finished running?
    SOCKET                   socket;
    stdVector<stdString>     input_line;
    char                     line[2048];
    unsigned int             dest;  // index of next unused char in _line
    double                   runtime;

    // Result: done, i.e. connection can be closed?
    bool handleInput();

    // return: full request that I can handle?
    bool analyzeInput();

    void dumpInput(HTMLPage &page);
};

#endif // !defined(HTTP_SERVER_H_)
