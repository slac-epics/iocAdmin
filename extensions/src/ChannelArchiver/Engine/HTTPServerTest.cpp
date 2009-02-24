// Tools
#include <AutoPtr.h>
#include <UnitTest.h>
// Engine
#include "HTTPServer.h"

static void close_test(HTTPClientConnection *connection,
                       const stdString &path, void *user_arg)
{
    close(connection->getSocket());
    HTMLPage page(connection->getSocket(), "Close");
    page.line("<H3>Echo</H3>");
    page.line("<PRE>");
    page.line(path);
    page.line("</PRE>");
}

static void echo_test(HTTPClientConnection *connection,
                      const stdString &path, void *user_arg)
{
    HTMLPage page(connection->getSocket(), "Echo");
    page.line("<H3>Echo</H3>");
    page.line("<PRE>");
    page.line(path);
    page.line("</PRE>");
}

static PathHandlerList  handlers[] =
{
    //  URL, substring length?, handler. The order matters!
    { "/close",  6, close_test },
    { "/",  0, echo_test },
    { 0,    0,  },
};

TEST_CASE test_http_server()
{
    try
    {
        AutoPtr<HTTPServer> server(new HTTPServer(4812, handlers, 0));
        server->start();
        printf("Server is running, try\n");
        printf("  lynx -dump http://localhost:4812\n");
        epicsThreadSleep(20.0);
    }
    catch (GenericException &e)
    {
        FAIL(e.what());
    }
    TEST_OK;
}

