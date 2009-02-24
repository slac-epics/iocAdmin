// --------------------------------------------------------
// $Id: MultiValueIterator.h,v 1.1.1.1 2004/04/01 20:49:45 luchini Exp $
//
// Please refer to NOTICE.txt,
// included as part of this distribution,
// for legal information.
//
// Kay-Uwe Kasemir, kasemir@lanl.gov
// --------------------------------------------------------

#ifndef __MULTIVALUEITERATOR_H__
#define __MULTIVALUEITERATOR_H__

#include "ValueIteratorI.h"

class MultiChannelIterator;

//CLASS MultiValueIterator
// Implements the CLASS ValueIteratorI interface for the CLASS MultiArchive
class MultiValueIterator : public ValueIteratorI
{
public:
    MultiValueIterator();
    virtual ~MultiValueIterator();

    virtual bool isValid() const;
    virtual const ValueI * getValue() const;
    virtual bool next();
    virtual bool prev();

    virtual size_t determineChunk(const epicsTime &until);
    virtual double getPeriod() const;

private:
    friend class MultiArchive;
    friend class MultiChannel;
    // To be called by MultiArchive classes only:
    void clear();
    void position(MultiChannelIterator *channel, ValueIteratorI *value);
    
    bool _is_valid;
    MultiChannelIterator *_channel_iterator;
    ValueIteratorI *_base_value_iterator;
};

#endif // __MULTIVALUEITERATOR_H__
