#ifndef SAVRES_HEADER_H
#define SAVRES_HEADER_H

#include <epicsThread.h>
#include <aaoRecord.h>

#ifdef __cplusplus
extern "C" {
#endif

/* write 'n' bytes in 'buf' to a binary file.
 * File is created (permissions: current umask) if necessary.
 * 'path' may be omitted (NULL).
 *
 * RETURNS: 0 on success, -1 on failure; an attempt is
 *          made to remove the file if the operation
 *          is unsuccessful.
 */
int
savresDumpData(char *path, char *fnam, char *buf, int n);

/* read up to 'n' bytes from a binary file into 'buf'.
 * 'path' may be omitted (NULL).
 *
 * RETURNS: number of bytes read (which may be less than n
 *          if file contains less data); -1 on failure.
 */
int
savresRstrData(char *path, char *fnam, char *buf, int n);

/* Notify a helper thread to dump the aao data
 * to a file. Can be called by aao record processing
 * (devsup 'write_aao').
 * The file name equals the record name. The path
 * is read from the environment variable "DATA_PATH"
 * or the default "/dat" is used if "DATA_PATH" is not
 * set.
 * Note that the write operation is asynchronous,
 * ie., the 'last' job will succeed. There is no
 * control over when the write is actually performed.
 * (AAO doesn't allow for async record processing :-( )
 * 
 * RETURNS: 0 on successful job queuing, -1 if queuing
 *          the job failed.
 *          There is no notification if the actual
 *          write operation fails.
 */

int
aaoDumpDataAsync(struct aaoRecord *paao);

/* Synchronously read data from a file.
 * The file name equals the record name. The path
 * is read from the environment variable "DATA_PATH"
 * or the default "/dat" is used if "DATA_PATH" is not
 * set.
 * This is usually called by 'init_record_aao' (devsup)
 * and hence it is OK to be synchronous.
 * 
 * RETURNS: 0 on success, -1 on failure.
 *
 * NOTE: paao->udf is set to FALSE and alarms are
 *       reset on success.
 */
int
aaoRstrData(struct aaoRecord *paao);

/* Initialize facility. Lazy init is also done by
 * aaoRstrData, i.e., if aaoRstrData is called
 * during iocInit() no further action is necessary.
 *
 * This routine must be executed once prior to
 * calling aaoDumpDataAsync(). 
 * It is not needed if only the low-level
 * routines (savresDumpData/saveresRstrData) are
 * used.
 *
 * Creates an epics message queue and a thread.
 */
int 
aaoSavResInit();

#ifdef __cplusplus
};
#endif
#endif

