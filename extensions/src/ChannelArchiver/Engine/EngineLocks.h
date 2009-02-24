#ifndef ENGINELOCKS_H_
#define ENGINELOCKS_H_

// System
#include <stdlib.h>

/** \defgroup Engine Engine
 *  Lock order constants.
 *  <p>
 *  When locking more than
 *  one object from the following list, they have to be taken
 *  in this order, for example: First lock the Engine, then an ArchiveChannel,
 *  then the ProcessVariableContext.
 *  <p>
 *  Classes in Tools like the ThrottledMsgLogger use locks with an order
 *  of 100 or higher.
 *  <p>
 *  Beyond the reach of this code are locks internal to the CA client library.
 *  Possible lock chains:
 *  <ul>
 *  <li>Start the engine: Engine, ArchiveChannel, ProcessVariable, ca_...<br>
 *      Has to lock the Engine to prevent concurrent stop().
 *      This initial scenario created the lock order.
 *  <li>CA connect or control info callback: CA, PV, channel, group.<br>
 *      This is basically the reversed lock order!
 *      To preserve the lock order, the channel will release the PV lock
 *      before locking itself, and unlock itself before locking the group.
 *      This does assume that the configuration (group membership etc.)
 *      does not change!<br>
 *      The engine used to maintain a connection count that was updated
 *      in here as well, but locking the engine in here results in deadlocks
 *      with case 6.
 *  <li>CA value arrives: CA,  PV, channel.<br>
 *      Maybe enable/disable group, which locks group and other channels.
 *      As in 2, the lock order must be preserved.
 *  <li>HTTP client lists group or channel info:<br>
 *      Lock Engine, group or channel, maybe channel's PV and ca_state().<br>
 *      Must lock the engine to prevent concurrent engine changes.
 *      DEADLOCK with 2 or 3.
 *      <br>
 *      TODO: Handled by not keeping any locks?
 *  <li>Stop a channel (including from HTTPClient config update):
 *      channel, PV, then ca_*<br>
 *      Handled as part of 6.
 *  <li>Stop engine (including ...): engine, channel, PV, ca_<br>
 *      DEADLOCK with 2 or 3.
 *      <br>
 *      Must lock the engine to prevent concurrent engine changes.
 *  <li>Change channel or overall config<br>
 *      Handled after stopping the Engine, reload, then start.
 *      No new problems.
 *  </ul>
 *  The basic PV/CA conflicts need to be handled by PV lock releases
 *  whenever calling CA.
 *  <p>
 */
 
/** \ingroup Engine
 *  The archive engine.
 */
class EngineLocks
{
public:
    /** Lock order constant for the Engine. */
    static const size_t Engine = 10;

    /** Lock order constant for the GroupInfo. */
    static const size_t GroupInfo = 20;

    /** Lock order constant for the ArchiveChannel. */
    static const size_t ArchiveChannel = 30;

    /** Lock order constant for the SampleMechanism. */
    static const size_t SampleMechanism = 40;

    /** Lock order constant for the RepeatFilter. */
    static const size_t RepeatFilter = 50;

    /** Lock order constant for the DisableFilter. */
    static const size_t DisableFilter = 51;

    /** Lock order constant for the ProcessVariable. */
    static const size_t ProcessVariable = 60;

    /** Lock order constant for the ProcessVariableContext. */
    static const size_t ProcessVariableContext = 70;

    /** Lock order constant for the client list of the HTTPServer. */
    static const size_t HTTPServer = 80;
};

#endif
