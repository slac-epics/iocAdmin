// --------------------------------------------------------
// $Id: MultiChannelIterator.h,v 1.1.1.1 2001/05/23 00:22:09 luchini Exp $
//
// Please refer to NOTICE.txt,
// included as part of this distribution,
// for legal information.
//
// Kay-Uwe Kasemir, kasemir@lanl.gov
// --------------------------------------------------------

#ifndef __MULTICHANNELITERATORI_H__
#define __MULTICHANNELITERATORI_H__

#include "ChannelIteratorI.h"
#include "MultiChannel.h"
#include "MultiArchive.h"
#include <RegularExpression.h>

class MultiValueIterator;

//CLASS MultiChannelIterator
// A CLASS ChannelIteratorI implementation for a CLASS MultiArchive
class MultiChannelIterator : public ChannelIteratorI
{
public:
    MultiChannelIterator(MultiArchive *archive);
    virtual ~MultiChannelIterator();

    virtual bool isValid() const;
    virtual ChannelI *getChannel();
    virtual bool next();

    // ---------------------------------------
    // To be called by MultiArchive:
    bool assign(MultiArchive *a, size_t channel_index)
    {
        _multi_archive = a;
        _channel_index = channel_index;
        _is_valid = _channel_index < _multi_archive->_channels.size();
        return _is_valid;
    }
    
    void clear();

    bool moveToMatchingChannel(const stdString &pattern);
    void position(ArchiveI *archive, ChannelIteratorI *channel_iterator);

private:
    friend class MultiArchive;
    friend class MultiChannel;
    friend class MultiValueIterator;
    
    MultiArchive *_multi_archive;       // Associated MultiArchive
    size_t _channel_index;              // Index of channel in that archive
    RegularExpression *_regex;

    bool _is_valid;                     // Rest only set if is_valid:
    ArchiveI *_base_archive;            // the current archive,
    ChannelIteratorI *_base_channel_iterator; // channeliterator and
    MultiChannel _channel;              // channel.
};

#endif //__MULTICHANNELITERATORI_H__
