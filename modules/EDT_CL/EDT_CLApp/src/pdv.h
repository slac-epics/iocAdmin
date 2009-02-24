
#ifndef _PDV_H_
#define _PDV_H_

int pdv_device_ioctl(
    Edt_Dev *edt_p,
    u_int controlCode,
    u_int inSize,
    u_int outSize,
    void *argBuf,
    u_int *bytesReturned,
    void *pIrp);

int pdv_DeviceInt(Edt_Dev *edt_p);

void pdv_dep_init(Edt_Dev *edt_p);

int pdvdrv_serial_init(Edt_Dev *edt_p, int unit);

void pdv_dep_set(Edt_Dev *edt_p, u_int reg_def, u_int val);

u_int pdv_dep_get(Edt_Dev *edt_p, u_int reg_def);

int pdv_hwctl(int operation, void *context, Edt_Dev *edt_p);


void pdvdrv_set_fval_done(Edt_Dev * edt_p, int turnon);

#endif



