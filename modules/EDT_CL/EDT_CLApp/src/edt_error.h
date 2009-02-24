#ifndef _edt_msg_H
#define _edt_msg_H

#include <stdarg.h>
#include <stdio.h>

#include "edtdef.h" /* this is where _NT_ would be defined */

/*****************************************
 *
 *
 * edt_error.h - generalized error handling for edt and pdv libraries
 *
 * Copyright 2000 Engineering Design Team
 *
 ****************************************/

/*
 * define EDTAPI instead of EDTAPI which is in libedt.h -- not needed
 * or included here.
 */

#ifndef EDTAPI

#ifdef _NT_

#define EDTAPI __declspec(dllexport)

#else

#define EDTAPI

#endif

#endif

typedef int (*EdtMsgFunction)(void *target, int level, char *message);


typedef struct _edt_msg_handler {

  EdtMsgFunction func;
  int level;
  FILE * file;
  unsigned char own_file;
  void * target;

} EdtMsgHandler;



/* default initialization - uses stdout and stderr */

EDTAPI void edt_msg_init(EdtMsgHandler *msg_p);

EDTAPI void edt_msg_init_names(EdtMsgHandler *msg_p, char *file, int level);

EDTAPI void edt_msg_init_files(EdtMsgHandler *msg_p, FILE *file, int level);

EDTAPI void edt_msg_close(EdtMsgHandler *msg_p);

EDTAPI void edt_msg_set_level(EdtMsgHandler *msg_p, int newlevel);

EDTAPI void edt_msg_set_function(EdtMsgHandler *msg_p, EdtMsgFunction f);


EDTAPI void edt_msg_set_file(EdtMsgHandler *msg_p, FILE *f);

EDTAPI void edt_msg_set_target(EdtMsgHandler *msg_p, void *t);

EDTAPI void edt_msg_set_name(EdtMsgHandler *msg_p, char *f);

EDTAPI int edt_msg_output(EdtMsgHandler *msg_p, int level, char *format, ...);

EDTAPI int edt_msg_output_perror(EdtMsgHandler *msg_p, int level, char *message);

/*
 * predefined message flags. EDTAPP_MSG are for general purpose application
 * use EDTLIB_MSG are for libedt messages. PDVLIB are for libpdv messages.
 * Application programmers can define other flags in the 0x1000 to
 * 0x1000000 range
 */
#define EDTAPP_MSG_FATAL   	0x1
#define EDTAPP_MSG_WARNING  	0x2
#define EDTAPP_MSG_INFO_1   	0x4
#define EDTAPP_MSG_INFO_2   	0x8

#define EDTLIB_MSG_FATAL	0x10
#define EDTLIB_MSG_WARNING	0x20
#define EDTLIB_MSG_INFO_1	0x40
#define EDTLIB_MSG_INFO_2	0x80

#define PDVLIB_MSG_FATAL	0x100
#define PDVLIB_MSG_WARNING	0x200
#define PDVLIB_MSG_INFO_1	0x400
#define PDVLIB_MSG_INFO_2	0x800

#define EDT_MSG_FATAL		  EDTAPP_MSG_FATAL \
				| EDTLIB_MSG_FATAL \
				| PDVLIB_MSG_FATAL

#define EDT_MSG_WARNING		  EDTAPP_MSG_WARNING \
				| EDTLIB_MSG_WARNING \
				| PDVLIB_MSG_WARNING

#define EDT_MSG_INFO_1		  EDTAPP_MSG_INFO_1 \
				| EDTLIB_MSG_INFO_1 \
				| PDVLIB_MSG_INFO_1

#define EDT_MSG_INFO_2		  EDTAPP_MSG_INFO_2 \
				| EDTLIB_MSG_INFO_2 \
				| PDVLIB_MSG_INFO_2

EDTAPI int edt_msg(int level, char *format, ...);
EDTAPI int edt_msg_perror(int level, char *msg);
EDTAPI EdtMsgHandler *edt_msg_default_handle(void);
EDTAPI int edt_msg_default_level(void);
EDTAPI char *edt_msg_last_error(void); /* returns pointer to most recent 
				      msg string */

#endif
