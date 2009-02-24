#ifndef ENGINECONFIG_H_
#define ENGINECONFIG_H_

// Tools
#include <ToolsConfig.h>
#include <FUX.h>

/**\ingroup Engine
 *  Global engine configuration parameters.
 *  <p>
 *  Does not use a mutex, assuming that the engine is stopped while
 *  parsing a new configuration and thus changing EngineConfig.
 */
class EngineConfig
{
public:
    EngineConfig()
    {
        write_period = 30.0;
        buffer_reserve = 3;
        max_repeat_count = 100;
        ignored_future = 60.0;
        get_threshold = 20.0;
        file_size_limit = 100*1024*1024; // 100MB Default
        disconnect_on_disable = false;
    }
    
    /** @return Returns the period in seconds between 'writes' to Storage. */
    double getWritePeriod() const       { return write_period; }

    /** @return Returns how many times more buffer is reserved. */
    size_t getBufferReserve() const     { return buffer_reserve; }
    
    /** @return Returns suggested buffer for given scan period. */
    size_t getSuggestedBufferSpace(double scan_period) const
    {
        if (scan_period <= 0)
            return 1;
        size_t num = (size_t)(write_period * buffer_reserve / scan_period);
        if (num < 3)
            return 3;
        return num;
    }
    
    /** @return Returns the max. repeat count.
     *  @see SampleMechanismGet
     */
    size_t getMaxRepeatCount() const    { return max_repeat_count; }
    
    /** @return Returns the seconds into the future considered too futuristic.
     */
    double getIgnoredFutureSecs() const { return ignored_future; }

    /** @return Returns the threshold between 'Get' and 'MonitoredGet'. */
    double getGetThreshold() const      { return get_threshold; }

    /** @return Returns the file size limit (Bytes). */
    size_t getFileSizeLimit() const     { return file_size_limit; }
    
    /** @return Returns true if engine disconnects disabled channels. */
    bool getDisconnectOnDisable() const { return disconnect_on_disable; }
    
    /** Append this config to a FUX document. */ 
    void addToFUX(class FUX::Element *doc);
    
protected:
    double write_period;
    size_t buffer_reserve;
    size_t max_repeat_count;
    double ignored_future;
    double get_threshold;
    size_t file_size_limit;
    bool   disconnect_on_disable;
};

/**\ingroup Engine
 *  Modifyable global engine configuration parameters.
 */
class WritableEngineConfig : public EngineConfig
{
public:    
    /** Set the period in seconds between 'writes' to Storage. */
    void setWritePeriod(double secs)    { write_period = secs; }

    /** Set how many times more buffer is reserved. */
    void setBufferReserve(size_t times) { buffer_reserve = times; }

    /** Set the max. repeat count.
     *  @see SampleMechanismGet
     */
    void setMaxRepeatCount(size_t count) { max_repeat_count = count; }

    /** Set which data gets ignored because time stamp is too far
     *  ahead of host clock.
     */
    void setIgnoredFutureSecs(double secs) { ignored_future = secs; }

    /** Set threshold between 'Get' and 'MonitoredGet'. */
    void setGetThreshold(double secs) { get_threshold = secs; }
    
    /** Set the file size limit (Bytes). */
    void setFileSizeLimit(size_t bytes) { file_size_limit = bytes; }
    
    /** Should engine disconnect channels that are disabled? */
    void setDisconnectOnDisable(bool yes_no) { disconnect_on_disable = yes_no;}    
};

/** \ingroup Engine
 *  Listener to EngineConfigParser
 */
class EngineConfigListener
{
public:
    /** Invoked for each channel that the EngineConfigParser found. */
    virtual void addChannel(const stdString &group_name,
                            const stdString &channel_name,
                            double scan_period,
                            bool disabling, bool monitor) = 0;
};

/** \ingroup Engine
 *
 * Reads the config from an XML file
 * that should conform to engineconfig.dtd.
 */
class EngineConfigParser : public WritableEngineConfig
{
public:
    EngineConfigParser();
    
    /** Reads the config from an XML file.
     * 
     *  @exception GenericException on error.
     */
     void read(const char *filename, EngineConfigListener *listener);

private:
    EngineConfigListener *listener;
    void handle_group(FUX::Element *group);
    void handle_channel(const stdString &group_name,
                        FUX::Element *channel);
};

#endif /*ENGINECONFIG_H_*/
