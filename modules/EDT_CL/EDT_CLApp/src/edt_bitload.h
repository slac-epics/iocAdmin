#ifndef _EDT_BITLOAD_H
#define _EDT_BITLOAD_H

#include "edtinc.h"

#define	CLKSAFTER	32

EDTAPI int edt_bitload(EdtDev *edt_p, char *basedir, char *fname, int flags, int skip);
EDTAPI int edt_bitload_loadit(EdtDev *edt_p, char *bitpath, int size, int skip, int flags, int alt, int alt_params, int which_try);
EDTAPI int edt_bitload_try_40xx_load(EdtDev *edt_p, char *basedir, char *devdir, char *fname, int skip, int flags);
void EDTAPI edt_bitload_devid_to_bitdir(EdtDev *edt_p, char *devdir);
EDTAPI int edt_get_x_header(FILE * xfile, char *header, int *size);
EDTAPI u_char *edt_get_xa_header(u_char *ba, char *header, int *size);
/* flag bits to use in flags param to edt_bitload */
#define BITLOAD_FLAGS_NOFS 0x1 /* embedded for vxworks */
#define BITLOAD_FLAGS_OVR  0x2 /* override gs/ss bitfile size constraint */
#define BITLOAD_FLAGS_OCM  0x4 /* changes programming alogorithm to program ocm channel xilinx */
#define BITLOAD_FLAGS_CH1  0x8 /* programs ocm channel 1  if ocm bit above is set */

/* program method alternatives */
#define	ALT_INTERFACE	0
#define	ALT_OCM			1

#endif /* _EDT_BITLOAD_H */
