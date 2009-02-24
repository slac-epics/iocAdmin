// Base
#include <cadef.h>
// Tools
#include <GenericException.h>
#include <MsgLogger.h>
// Local
#include "EngineLocks.h"
#include "ProcessVariableContext.h"

// #define DEBUG_PV_CONTEXT

static void caException(struct exception_handler_args args)
{
    const char *pName;
    
    if (args.chid)
        pName = ca_name(args.chid);
    else
        pName = "?";
    LOG_MSG("CA Exception %s - with request "
            "chan=%s op=%d type=%s count=%d:\n%s\n", 
            args.ctx, pName, (int)args.op, dbr_type_to_text(args.type),
            (int)args.count,
            ca_message(args.stat));
}

ProcessVariableContext::ProcessVariableContext()
    : mutex("ProcessVariableContext", EngineLocks::ProcessVariableContext),
      ca_context(0), refs(0), flush_requested(false)
{
    LOG_MSG("Creating ChannelAccess Context.\n");
    if (ca_context_create(ca_enable_preemptive_callback) != ECA_NORMAL ||
        ca_add_exception_event(caException, 0) != ECA_NORMAL)
        throw GenericException(__FILE__, __LINE__,
                               "CA client initialization failed.");
    ca_context = ca_current_context();
}

ProcessVariableContext::~ProcessVariableContext()
{
    if (refs > 0)
    {
        LOG_MSG("ProcessVariableContext has %zu references "
                "left on destruction\n", (size_t)refs);
        return;
    }
    LOG_MSG("Stopping ChannelAccess.\n");
    ca_context_destroy();
    ca_context = 0;
}

OrderedMutex &ProcessVariableContext::getMutex()
{
    return mutex;
}

void ProcessVariableContext::attach(Guard &guard)
{
    guard.check(__FILE__, __LINE__, mutex);
    int result = ca_attach_context(ca_context);
    if (result == ECA_ISATTACHED)
        throw GenericException(__FILE__, __LINE__,
            "thread 0x%08lX (%s) tried to attach more than once\n",
                (unsigned long)epicsThreadGetIdSelf(),
                epicsThreadGetNameSelf());
    if (result != ECA_NORMAL)
        throw GenericException(__FILE__, __LINE__,
            "ca_attach_context failed for thread 0x%08lX (%s)\n",
                (unsigned long)epicsThreadGetIdSelf(),
                epicsThreadGetNameSelf());
}

bool ProcessVariableContext::isAttached(Guard &guard)
{
    guard.check(__FILE__, __LINE__, mutex);
    int result = ca_attach_context(ca_context);
    return result == ECA_ISATTACHED;
}

void ProcessVariableContext::incRef(Guard &guard)
{
    guard.check(__FILE__, __LINE__, mutex);
    LOG_ASSERT(isAttached(guard));
    ++refs;
#   ifdef DEBUG_PV_CONTEXT
    printf("ProcessVariableContext::incRef: %zu refs\n", (size_t)refs);
#   endif
}
        
void ProcessVariableContext::decRef(Guard &guard)
{
    guard.check(__FILE__, __LINE__, mutex);
    LOG_ASSERT(isAttached(guard));
    if (refs <= 0)
        throw GenericException(__FILE__, __LINE__,
                               "RefCount goes negative");
    --refs;
#   ifdef DEBUG_PV_CONTEXT
    printf("ProcessVariableContext::decRef: %zu refs\n", (size_t)refs);
#   endif
}

size_t ProcessVariableContext::getRefs(Guard &guard)
{
    guard.check(__FILE__, __LINE__, mutex);
    LOG_ASSERT(isAttached(guard));
    return refs;
}

void ProcessVariableContext::requestFlush(Guard &guard)
{
#   ifdef DEBUG_PV_CONTEXT
    printf("ProcessVariableContext::requestFlush\n");
#   endif
    guard.check(__FILE__, __LINE__, mutex);
    LOG_ASSERT(isAttached(guard));
    flush_requested = true;
}
    
bool ProcessVariableContext::isFlushRequested(Guard &guard)
{
    guard.check(__FILE__, __LINE__, mutex);
    LOG_ASSERT(isAttached(guard));
    return flush_requested;
}

void ProcessVariableContext::flush(Guard &guard)
{
    guard.check(__FILE__, __LINE__, mutex);
    LOG_ASSERT(isAttached(guard));
    if (flush_requested)
    {
        flush_requested = false;
#       ifdef DEBUG_PV_CONTEXT
        printf("ProcessVariableContext::flush\n");
#       endif
        {
            GuardRelease release(__FILE__, __LINE__, guard);
            ca_flush_io();
        }
    }
}

#if 0
//  TODO: Dump CA client lib info to file on demand.
    if (info_dump_file.length() > 0)
    {
        int out = open(info_dump_file.c_str(), O_CREAT|O_WRONLY, 0x777);
        info_dump_file.assign(0, 0);
        if (out >= 0)
        {
            int oldout = dup(1);
            dup2(out, 1);
            ca_client_status(10);
            dup2(oldout, 1);
        }
    }
#endif

