/* memDrv.h - header file for memDrv.c */

/* Copyright 1984-1992 Wind River Systems, Inc. */

/*
modification history
--------------------
01f,13aug98,rlp  added memDevDelete function.
01e,22sep92,rrr  added support for c++
01d,04jul92,jcf  cleaned up.
01c,26may92,rrr  the tree shuffle
01b,04oct91,rrr  passed through the ansification filter
		  -fixed #else and #endif
		  -changed copyright notice
01a,05oct90,shl created.
*/

#ifndef __INCedtDrvh
#define __INCedtDrvh

#ifdef __cplusplus
extern "C" {
#endif


/* function declarations */

#if defined(__STDC__) || defined(__cplusplus)

extern void edtInstall(int board);
extern void edtRemove(int board);
extern int  edtDevCreate (char *name, char *base, int board, int channel);
extern int  edtDevDelete (char *name);

extern HANDLE edtOpen (char * chnlname, int mode, int flag);
extern int edtRead (HANDLE fd, char * user_buf, int count);
int edtWrite (HANDLE fd, char * user_buf, int count);
int edtIoctl (HANDLE fd, int function , int arg);
int edtClose (HANDLE fd);

#else

extern void edtInstall();
extern void edtRemove();
extern int  edtDevCreate ();
extern int  edtDevDelete ();

extern HANDLE edtOpen ();
extern int edtRead ();
extern int edtWrite ();
extern int edtClose ();
extern int edtIoctl ();

#endif	/* __STDC__ */

#ifdef __cplusplus
}
#endif

#endif /* __INCedtDrvh */
