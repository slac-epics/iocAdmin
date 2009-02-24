/*****************************************
 *
 *
 * edt_error.c - generalized error handling for edt and pdv libraries
 *
 * Copyright 2000 Engineering Design Team
 *
 ****************************************/


#include "edtdef.h"
#include "edt_error.h"
#include <string.h>
#include <sys/errno.h>

#define MAX_MESSAGE 512

#ifdef PDV
#define EDT_DEFAULT_MSG_LEVEL     EDTAPP_MSG_FATAL \
				| EDTAPP_MSG_WARNING \
				| EDTLIB_MSG_FATAL \
				| EDTLIB_MSG_WARNING \
				| PDVLIB_MSG_FATAL \
				| PDVLIB_MSG_WARNING
#else
#define EDT_DEFAULT_MSG_LEVEL     EDTAPP_MSG_FATAL \
				| EDTAPP_MSG_WARNING \
				| EDTLIB_MSG_FATAL \
				| EDTLIB_MSG_WARNING 
#endif
static char edt_last_error_message[MAX_MESSAGE];

char *edt_msg_last_error()
{
    return edt_last_error_message;
}


static EdtMsgHandler edt_default_msg;
static int edt_default_msg_initialized = 0;

static int edt_msg_printf(void *f_p, int level, char *message)

{
	FILE *f = (FILE *) f_p;


	if (f)
		return fprintf(f, message);
	
	else
		return -1;

}

/* default initialization - uses stdout and stderr */

void edt_msg_init(EdtMsgHandler *msg_p)

{
	if (msg_p)
	{
		msg_p->file = stderr;
		msg_p->own_file = 0;
		msg_p->level = EDT_DEFAULT_MSG_LEVEL;
		msg_p->func = edt_msg_printf;
		msg_p->target = msg_p->file;
	}

	if (msg_p == &edt_default_msg)
	    edt_default_msg_initialized = 1;
}

void edt_msg_init_names(EdtMsgHandler *msg_p, char *file, int level)

{
	edt_msg_init(msg_p);
	edt_msg_set_name(msg_p, file);
	edt_msg_set_level(msg_p, level);

}

void edt_msg_init_files(EdtMsgHandler *msg_p, FILE *file, int level)

{
	edt_msg_init(msg_p);
	edt_msg_set_file(msg_p, file);
	edt_msg_set_level(msg_p, level);
  
}

int
edt_msg_output(EdtMsgHandler *msg_p,  int level, char *format, ...)

{

    va_list	stack;


    if (msg_p == &edt_default_msg)
    {
	if (!edt_default_msg_initialized)
	    edt_msg_init(msg_p);
    }

    if (!(level & msg_p->level))
	    return 0;

    va_start(stack,	format);

    vsprintf(edt_last_error_message, format, stack);

    if (msg_p->func)
	return msg_p->func(msg_p->target, level, edt_last_error_message);

    return -1;
}


static
void edt_msg_close_file(EdtMsgHandler *msg_p)

{
    if (msg_p->own_file && msg_p->file)
	    fclose(msg_p->file);

    if (msg_p->target == msg_p->file)
	    msg_p->target = NULL;

    msg_p->file = NULL;
    msg_p->own_file = 0;

}

void edt_msg_close(EdtMsgHandler *msg_p)

{
	
    edt_msg_close_file(msg_p);
		
}


void edt_msg_set_level(EdtMsgHandler *msg_p, int newlevel)

{
    if ((msg_p == &edt_default_msg) && !edt_default_msg_initialized)
	edt_msg_init(&edt_default_msg);

    msg_p->level  = newlevel;

}

void edt_msg_set_function(EdtMsgHandler *msg_p, EdtMsgFunction f)

{
    if ((msg_p == &edt_default_msg) && !edt_default_msg_initialized)
	edt_msg_init(&edt_default_msg);

	msg_p->func = f;
}


void edt_msg_set_file(EdtMsgHandler *msg_p, FILE *f)

{
    if ((msg_p == &edt_default_msg) && !edt_default_msg_initialized)
	edt_msg_init(&edt_default_msg);

    edt_msg_close_file(msg_p);

    msg_p->file = f;

    msg_p->own_file = 0;
}


void edt_msg_set_name(EdtMsgHandler *msg_p, char *name)

{
    if ((msg_p == &edt_default_msg) && !edt_default_msg_initialized)
	edt_msg_init(&edt_default_msg);

    edt_msg_close_file(msg_p);

    msg_p->file = fopen(name, "wb");

    msg_p->own_file = (msg_p->file != NULL);
    edt_msg_set_target(msg_p, msg_p->file);

}

void edt_msg_set_target(EdtMsgHandler *msg_p, void *target)

{
    if ((msg_p == &edt_default_msg) && !edt_default_msg_initialized)
	edt_msg_init(&edt_default_msg);

    msg_p->target = target;
}


int
edt_msg(int level, char *format, ...)

{
    va_list	stack;

    if (!edt_default_msg_initialized)
	edt_msg_init(&edt_default_msg);

    if (!(level & edt_default_msg.level))
	    return 0;

    va_start(stack,	format);

    vsprintf(edt_last_error_message, format, stack);

    if (edt_default_msg.func)
	return edt_default_msg.func(edt_default_msg.target, level, edt_last_error_message);
    return -1;

}

int edt_msg_output_perror(EdtMsgHandler *msg_p, int level, char *msg)

{
    char message[MAX_MESSAGE];

    if ((msg_p == &edt_default_msg) && !edt_default_msg_initialized)
	edt_msg_init(&edt_default_msg);

    sprintf(message, "%s: %s\n", msg, strerror(errno));

    return edt_msg_printf(msg_p, level, message);
}

int
edt_msg_perror(int level, char *msg)

{
    char message[MAX_MESSAGE];

    if (!edt_default_msg_initialized)
	edt_msg_init(&edt_default_msg);

    if (!(level & edt_default_msg.level))
	    return 0;

    sprintf(message, "%s: %s\n", msg, strerror(errno));
    return edt_msg(level, message);
}

EdtMsgHandler * edt_msg_default_handle()
{
    if (!edt_default_msg_initialized)
	edt_msg_init(&edt_default_msg);

    return &edt_default_msg;
}

int edt_msg_default_level()
{
    if (!edt_default_msg_initialized)
	edt_msg_init(&edt_default_msg);

    return edt_default_msg.level;
}
