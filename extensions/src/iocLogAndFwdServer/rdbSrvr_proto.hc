/*   **CMS**=C_INC   */

/*==============================================================================

  Abs:  protos for rdbServer.

  Name: rdbSrvr_proto.hc

  Auth: 01-Apr-1999   Ron MacKenzie (RONM)

  Prev: 
        All prevs are included within this module

--------------------------------------------------------------------------------

  Mod:  

7/2007 Ronm
Increase host of fwd_err_msg_ts to 16 from 8 bytes just for this app(fwdBro).
This struct is sent to fwdCliS on unix

==============================================================================*/



#ifndef RDB_SRVR_PROTO_HC
#define RDB_SRVR_PROTO_HC


int         fbro_vms_net (char * host_p,
                           int codeI,
                           char* dest_p,
                           int dest_provided,
                           char * vms_msg_p);

int fbro_fill_msg_to_unix (char * host_p,
                           int    codeI,
                           char*  dest_p,
                           int    dest_provided,
                           char * vms_msg_p,
                           int * const nbyte_msg_p,
                           char * const unix_err_msg_ps);

int         fbro_logMsg  (int messsage_code, char * err_msg, int my_errno,
                          int severity,
                          int log_to_vms);



#endif

































