// --------------------------------------------------------
// $Id: HTTPServer.cpp,v 1.1.1.1 2006/07/13 15:13:18 luchini Exp $
//
// Please refer to NOTICE.txt,
// included as part of this distribution,
// for legal information.
//
// Kay-Uwe Kasemir, kasemir@lanl.gov
// --------------------------------------------------------

#ifdef WIN32
#pragma warning (disable: 4786)
#endif

// System
#include <signal.h>
// Tools
#include <MsgLogger.h>
#include <ToolsConfig.h>
#include <ThrottledMsgLogger.h>
// Engine
#include "EngineLocks.h"
#include "HTTPServer.h"

#undef MOZILLA_HACK

#if defined(HTTPD_DEBUG)  && HTTPD_DEBUG > 1
// Throttle the log messages for accepted clients down to once every 10 minutes.
static ThrottledMsgLogger client_IP_log_throttle("HTTP clients", 10*60);
#endif

// The HTTPServer launches one HTTPClientConnection per
// web client, which runs until
// a) the request is understood and handled
// b) the server shuts down while we're in the middle of
//    a client request
// c) there's a timeout: Nothing from the client for some time
// Fine.
// 
// The first idea was then: Client cleans itself up,
// HTTPClientConnection::run() ends like this
// - remove itself from server's list of clients
// - delete this
// - return
// 
// Problem: That deletes the epicsThread before the
// thread routine (run) exists, resulting in messages
// "epicsThread::~epicsThread():
//  blocking for thread "HTTPClientConnection" to exit
//  Was epicsThread object destroyed before thread exit ?"
// 
// Therefore HTTPClientConnection::run() marks itself
// as "done" and returns, leaving it to the HTTPServer
// to delete the HTTPClientConnection (See client_cleanup()).
// 
// Similarly, HTTPServer cannot delete itself but
// depends on the Engine class to delete the HTTPServer.

// HTTPServer ----------------------------------------------
HTTPServer::HTTPServer(short port, PathHandlerList *handlers, void *user_arg)
  : thread(*this, "HTTPD",
    epicsThreadGetStackSize(epicsThreadStackBig),
    epicsThreadPriorityMedium),
    go(true),
    socket(0),
    handlers(handlers),
    user_arg(user_arg),
    total_clients(0),
    client_duration(0.0),
    client_list_mutex("HTTPD Client List", EngineLocks::HTTPServer),
    clients(new AutoPtr<HTTPClientConnection> [MAX_NUM_CLIENTS])
{
    socket = epicsSocketCreate(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in  local;
    local.sin_family = AF_INET;
    local.sin_port = htons (port);
#   ifdef WIN32
    local.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
#   else
    local.sin_addr.s_addr = htonl(INADDR_ANY);
#   endif
    int val = 1;
    if (setsockopt(socket, SOL_SOCKET, SO_REUSEADDR,
                   (const char *)&val, sizeof(val)))
        LOG_MSG("setsockopt(SO_REUSEADDR) failed for HTTPServer::create\n");
    if (bind(socket, (const struct sockaddr *)&local, sizeof local) != 0)
        throw GenericException(__FILE__, __LINE__,
                               "HTTPServer cannot bind to port %d",
                               (int) port);
    listen(socket, 3);
}

HTTPServer::~HTTPServer()
{
    // Tell server and clients to stop.
    go = false;
    // Wait for clients to quit.
    while (client_cleanup() > 0)
    {
        LOG_MSG("HTTPServer waiting for clients to quit.\n");
        epicsThreadSleep(5.0);
        // We used to 'break' after some timeout, but then
        // the actual epicsThread will still hang around until
        // the client quits. So if a client is hung, for example
        // waiting for an Engine lock, there's no early way out
        // other than waiting or 'kill -9'.
        // So we wait.
    }
    // Wait for server to quit
    if (! thread.exitWait(5.0))
        LOG_MSG("HTTPServer: server thread does not exit.\n");
    epicsSocketDestroy(socket);
#ifdef HTTPD_DEBUG
    LOG_MSG("HTTPServer done.\n");
#endif
}

void HTTPServer::start()
{
    thread.start();
}

void HTTPServer::run()
{
#ifdef HTTPD_DEBUG
    LOG_MSG("HTTPServer thread 0x%08lX running.\n",
            (unsigned long)epicsThreadGetIdSelf());
#endif

    // Ignore SIGPIPE.
    struct sigaction action;
    memset(&action, 0, sizeof(struct sigaction));
    action.sa_handler = SIG_IGN;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    if (sigaction(SIGPIPE, &action, 0))
        LOG_MSG("Error setting SIGPIPE handler\n");

    try
    {
        // Wait for HTTP clients & spawn client handlers.
        bool overloaded = false;
        while (go)
        {
            // Don't hang in accept() but use select() so that
            // we have a chance to react to the main program
            // setting go == false
            fd_set fds;
            FD_ZERO(&fds);
            FD_SET(socket, &fds);
            struct timeval timeout;
            timeout.tv_sec = HTTPD_TIMEOUT;
            timeout.tv_usec = 0;
            if (select(socket+1, &fds, 0, 0, &timeout)!=1 ||
                !FD_ISSET(socket, &fds))
                continue;
            if (!go) // might have stopped while in select()
                break;
            struct sockaddr_in peername;
            socklen_t len = sizeof peername;
            SOCKET peer = accept(socket, (struct sockaddr *)&peername, &len);
            if (peer == INVALID_SOCKET)
                continue;
            // How to handle too many clients?
            // Certainly try to avoid overload, i.e. ignore them.
            // Accept, give error, and then close their connections?
            size_t num_clients = client_cleanup();
            if (num_clients >= MAX_NUM_CLIENTS)
            { 
                if (! overloaded)
                {
                    LOG_MSG("HTTPServer reached %zu concurrent clients.\n",
                            (size_t)num_clients);
                    overloaded = true;
                }
                reject(peer);
                // Wait: Don't allow 'attack' of clients to raise our CPU load.
                //epicsThreadSleep(HTTPD_TIMEOUT);
                continue;
            }
            overloaded = false;
                
    #if     defined(HTTPD_DEBUG)  && HTTPD_DEBUG > 1
            {
                stdString local_info, peer_info;
                GetSocketInfo(peer, local_info, peer_info);
                client_IP_log_throttle.LOG_MSG(
                    "HTTPServer thread 0x%08lX accepted %s/%s.\n",
                    (unsigned long)epicsThreadGetIdSelf(),
                    local_info.c_str(), peer_info.c_str());
            }
    #endif
            start_client(peer);
            // Allow a little runtime, so that it might already be done
            // when we perform the next client_cleanup()
            // epicsThreadSleep(HTTPD_TIMEOUT);
        }
    }
    catch (GenericException &e)
    {
        LOG_MSG("HTTPServer thread exception:\n%s\n", e.what());
    }
    catch (std::exception &e)
    {
        LOG_MSG("HTTPServer thread fatal exception:\n%s\n", e.what());
    }    
    catch (...)
    {
        LOG_MSG("HTTPServer thread: Unknown exception\n");
    }
#ifdef HTTPD_DEBUG
    LOG_MSG("HTTPServer thread 0x%08lX exiting.\n",
            (unsigned long)epicsThreadGetIdSelf());
#endif
}

void HTTPServer::start_client(SOCKET peer)
{
    Guard guard(__FILE__, __LINE__, client_list_mutex);
    size_t i;
    for (i = 0; i < MAX_NUM_CLIENTS; ++i)
    {
        if (clients[i])
            continue;
        try
        {   // Found empty slot, create new client.
            clients[i] = new HTTPClientConnection(this, peer, ++total_clients);
            clients[i]->start();
            return; // OK
        }
        catch (GenericException &e)
        {
            LOG_MSG("HTTPServer::start_client exception:\n%s\n", e.what());
        }
        catch (std::exception &e)
        {
            LOG_MSG("HTTPServer::start_client fatal exception:\n%s\n",
                    e.what());
        }    
        catch (...)
        {
            LOG_MSG("HTTPServer::start_client: Unknown exception\n");
        }
        clients[i] = 0;
        return;
    }
    LOG_MSG("HTTPServer client list is full\n");
}

size_t HTTPServer::client_cleanup()
{
    size_t i, num_clients = 0;
    Guard guard(__FILE__, __LINE__, client_list_mutex);

    for (i = 0; i < MAX_NUM_CLIENTS; ++i)
    {
        if (! clients[i])
            continue;
        if (clients[i]->isDone())
        {
            clients[i]->join();
            // Update runtime statistics
            client_duration = 0.99*client_duration + 0.01*clients[i]->getRuntime();
            // valgrind complains about access to undefined mem.
            // What?? Format string? num?
#           if defined(HTTPD_DEBUG) && HTTPD_DEBUG > 2
            size_t num = clients[i]->getNum();
            LOG_MSG("HTTPClientConnection %zu is done.\n", (size_t)num);
#           endif
            clients[i] = 0;
        }
        else
        {
#           if defined(HTTPD_DEBUG) && HTTPD_DEBUG > 2
            size_t num = clients[i]->getNum();
            LOG_MSG("HTTPClientConnection %zu is active\n", (size_t)num);
#           endif
            ++num_clients;
        }
    }
#   if defined(HTTPD_DEBUG) && HTTPD_DEBUG > 2
    if (num_clients > 0)
        LOG_MSG("%zu clients left.\n", (size_t)num_clients);
#   endif
    return num_clients;
}

void HTTPServer::reject(SOCKET socket)
{
    {
        HTMLPage page(socket, "Error");
        page.line("Too many clients");
    }
    shutdown(socket, 2);
    epicsSocketDestroy(socket);    
}

void HTTPServer::serverinfo(SOCKET socket)
{
    HTMLPage page(socket, "Server Info");    
    char line[100];
    
    page.openTable(1, "Client Number", 1, "Status", 1, "Age [secs]", 0);
    char num[20], age[50];
    epicsTime now = epicsTime::getCurrent();
    const char *status;
    {
        Guard guard(__FILE__, __LINE__, client_list_mutex);
        size_t i;
        for (i = 0; i < MAX_NUM_CLIENTS; ++i)
        {
            if (! clients[i])
                continue;
            sprintf(num, "%zu", (size_t)clients[i]->getNum());
            status = (clients[i]->isDone() ? "done" : "running");
            // The client's getRuntime() provides the active runtime,
            // updated by the client itself.
            // Here we show how long the client has been around even
            // if it didn't run.
            double uptime = now - clients[i]->getBirthTime();
            sprintf(age, "%.6f", uptime);
            page.tableLine(num, status, age, 0);
        }
    }
    page.closeTable();
    sprintf(line, "Average client runtime: %.6f seconds",
            client_duration);
    page.line(line);
}

// HTTPClientConnection -----------------------------------------

HTTPClientConnection::HTTPClientConnection(HTTPServer *server,
                                           SOCKET socket, int num)
  : thread(*this, "HTTPClientConnection",
           epicsThreadGetStackSize(epicsThreadStackBig),
           epicsThreadPriorityLow),
    server(server),
    birthtime(epicsTime::getCurrent()),
    num(num),
    done(false),
    socket(socket),
    dest(0),
    runtime(0.0)
{}

HTTPClientConnection::~HTTPClientConnection()
{
    if (! done)
    {
        LOG_MSG("HTTPClientConnection: Forced shutdown while # %zu still up\n",
                (size_t)num);
        epicsSocketDestroy(socket);
    }
#   if defined(HTTPD_DEBUG) && HTTPD_DEBUG > 2
    else
        LOG_MSG("~HTTPClientConnection: Graceful end of %zu\n", (size_t)num);
#   endif
}

void HTTPClientConnection::run()
{
    try
    {
#       if defined(HTTPD_DEBUG) && HTTPD_DEBUG > 2
        stdString local_info, peer_info;
        GetSocketInfo(socket, local_info, peer_info);
        LOG_MSG("HTTPClientConnection %zu thread 0x%08lX, handles %s/%s\n",
                (size_t)num, (unsigned long) epicsThreadGetIdSelf(),
                local_info.c_str(), peer_info.c_str());
#       endif
#       ifdef MOZILLA_HACK
        // Unclear:
        // The final 'socket_close' call should flush
        // queued write requests.
        // But especially with Mozilla, some pages show up
        // incomplete even though this thread has printed
        // all the data.
        // SO_LINGER makes us hang in the close call for 30 seconds.
        // Seems to help some but doesn't solve the problem 100% of the time.
        struct linger linger;
        linger.l_onoff = 1;
        linger.l_linger = 30;
        setsockopt(socket, SOL_SOCKET, SO_LINGER,
                   (const char *)&linger, sizeof(linger));
#       endif
        // This line seems to make the differenc between valgrind seeing
        // a memory leak in the first input_line.push_back() and no
        // memory leak:
        input_line.reserve(20);
        while (!handleInput())
        {
            if (server->isShuttingDown())
            {
                LOG_MSG("HTTPClientConnection %zu stopped; server's ending\n",
                        (size_t)num);
                break;
            }
            runtime = epicsTime::getCurrent() - birthtime;
            if (runtime > HTTPD_CLIENT_TIMEOUT)
            {
                LOG_MSG("HTTPClientConnection %zu timing out\n", (size_t)num);
                break;
            }
        }
#       ifdef MOZILLA_HACK
        // The delay and shutdown seem to help Mozilla
        // to read the web page before it stops because
        // the connection quits.
        epicsThreadSleep(2.0);
#       endif
        shutdown(socket, 2);
        epicsSocketDestroy(socket);
        runtime = epicsTime::getCurrent() - birthtime;
#       if HTTPD_DEBUG >= 3
        LOG_MSG("Closed client #%d, socket %d after %.3f seconds\n",
                num, socket, runtime);
#       endif
    }
    catch (GenericException &e)
    {
        LOG_MSG ("HTTPClient Exception:\n%s\n", e.what());
    }
    catch (std::exception &e)
    {
        LOG_MSG ("Fatal HTTPClient Problem:\n%s\n", e.what());
    }
    catch (...)
    {
        LOG_MSG ("HTTPClient: Unknown Exception\n");
    }
    done = true;
}

void HTTPClientConnection::join()
{
    if (! thread.exitWait(1.0))
    {
        LOG_MSG("HTTPClientConnection::join() failed for # %zu", (size_t)num);
    }
}

// Result: done, i.e. connection can be closed?
bool HTTPClientConnection::handleInput()
{
    // Check if there's anything to read
    struct timeval timeout;
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(socket, &fds);
    timeout.tv_sec = HTTPD_READ_TIMEOUT;
    timeout.tv_usec = 0;
    if (select(socket+1, &fds, 0, 0, &timeout)<1  ||  !FD_ISSET(socket, &fds))
        return false;
    // gather input into lines:
    char stuff[100];
    int end = recv(socket, stuff, (sizeof stuff)-1, 0);
    char *src;
    if (end <= 0) // client changed his mind?
        return true;
    stuff[end] = 0;
    for (src = stuff; *src; ++src)
    {
        switch (*src)
        {
            case 13:
                if (*(src+1) == 10)
                    continue; // handle within next loop
                // else: handle EOL now
            case 10:
                // complete current line
                line[dest] = 0;
                input_line.push_back(line);
                dest = 0;
                break;
            default:
                // avoid writing past end of _line!!!
                if (dest < (sizeof(line)-1))
                    line[dest++] = *src;
        }
    }
    return analyzeInput();
}

void HTTPClientConnection::dumpInput(HTMLPage &page)
{
    page.line("<OL>");
    for (size_t i=0; i<input_line.size(); ++i)
    {
        page.out ("<LI>");
        page.line (input_line[i]);
    }
    page.line ("</OL>");
}

// return: full request that I can handle?
bool HTTPClientConnection::analyzeInput()
{
    // format: GET <path> HTTP/1.1
    // need first line to determine this:
    if (input_line.size() <= 0)
        return false;
    if (input_line[0].substr(0,3) != "GET")
    {
        LOG_MSG("HTTP w/o GET line: '%s'\n", input_line[0].c_str());
        HTMLPage page (socket, "Error");
        page.line("<H1>Error</H1>");
        page.line("Cannot handle this input:");
        dumpInput(page);
        return true;
    }
    size_t pos = input_line[0].find("HTTP/");
    if (pos == input_line[0].npos)
    {
        LOG_MSG("pre-HTTP 1.0 GET line: '%s'\n", input_line[0].c_str());
        HTMLPage page(socket, "Error");
        page.line("<H1>Error</H1>");
        page.line("Pre-HTTP 1.0 GET is not supported:");
        dumpInput(page);
        return true;
    }
    // full requests ends with empty line:
    if (input_line.back().length() > 0)
        return false;
    // We have a full request.
    // Signal that we won't _read_ any more from this socket:
    shutdown(socket, 0);
#   if defined(HTTPD_DEBUG) && HTTPD_DEBUG > 4
    LOG_MSG("HTTPClientConnection::analyzeInput\n");
    for (size_t i=0; i<input_line.size(); ++i)
    {
        LOG_MSG("line %d: '%s'\n",
                i, input_line[i].c_str());
    }
#   endif
    stdString path = input_line[0].substr(4, pos-5);
    //stdString protocol = input_line[0].substr(pos+5);

    // Linear search ?!
    // Hash or map lookup isn't straightforward
    // because path has to be cut for parameters first...
    if (strcmp(path.c_str(), "/ping") == 0)
    {
        // No output at all.
        return true;
    }
    if (strcmp(path.c_str(), "/server") == 0)
    {
        server->serverinfo(socket);
        return true;
    }
    PathHandlerList *h = server->getHandlers();
    for (/* */; h && h->path; ++h)
    {
        if ((h->path_len > 0  &&
             strncmp(path.c_str(), h->path, h->path_len) == 0)
            ||
            path == h->path)
        {
#           if defined(HTTPD_DEBUG) && HTTPD_DEBUG > 4
            stdString peer;
            GetSocketPeer(socket, peer);
            LOG_MSG("HTTP get '%s' from %s\n", path.c_str(), peer.c_str());
#           endif
            h->handler(this, path, server->getUserArg());
#           if defined(HTTPD_DEBUG) && HTTPD_DEBUG > 4
            LOG_MSG("HTTP invoked handler for '%s'\n", path.c_str());
#           endif
            return true;
        }
    }
    pathError(path);
    return true;
}

void HTTPClientConnection::error(const stdString &message)
{
    HTMLPage page(socket, "Error");
    page.line(message);
}

void HTTPClientConnection::pathError (const stdString &path)
{
    error("The path <I>'" + path + "'</I> does not exist.");
}

