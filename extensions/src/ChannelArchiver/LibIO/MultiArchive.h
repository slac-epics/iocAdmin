// -*- c++ -*-
// --------------------------------------------------------
// $Id: MultiArchive.h,v 1.1.1.1 2004/04/01 20:49:44 luchini Exp $
//
// Please refer to NOTICE.txt,
// included as part of this distribution,
// for legal information.
//
// Kay-Uwe Kasemir, kasemir@lanl.gov
// --------------------------------------------------------

#ifndef __MULTI_ARCHIVEI_H__
#define __MULTI_ARCHIVEI_H__

#include "ArchiveI.h"
#include "epicsTimeHelper.h"

//#define DEBUG_MULTIARCHIVE

class MultiChannelIterator;
class MultiValueIterator;

//CLASS MultiArchive
// The MultiArchive class implements a CLASS ArchiveI interface
// for accessing more than one archive.
// <P>
// The MultiArchive is configured via a "master" archive file,
// an ASCII file with the following format:
//
// <UL>
// <LI>Comments (starting with a number-sign) and empty lines are ignored
// <LI>The very first line must be<BR>
//     <PRE>master_version=1</PRE>
//     There must be no comments preceding this line!
// <LI>All remaining lines list one archive name per line
// </UL>
// <H2>Example</H2>
// <PRE>
//  master_version=1
//  # First check the "fast" archive
//  /archives/fast/dir
//  # Then check the "main" archive
//  /archives/main/2001_05/dir
//  /archives/main/2001_04/dir
//  /archives/main/2001_03/dir
//  /archives/main/2001_02/dir
//  /archives/main/2001/dir
// </PRE>
// <P>
// The most recent sub-archive has to be listed first,
// followed by the next one back in time and so on
// to the oldest sub-archive.
// <P>
// This type of archive is read-only!
// <P>
// For now, each individual archive is in the binary data format
// (CLASS BinArchive).
// Later, it might be necessary to specify the type together with the name
// for each archive. The master_version will then be incremented.
// <BR>
// If the "master" file is invalid, it is considered an ordinary BinArchive
// directory file,
// i.e. Tools based on the MultiArchive should work just like BinArchive-based
// Tools when operating on a single archive.
//
// <H2>Details</H2>
// No sophisticated merging technique is used.
// Given a channel and point in time,
// the MultiArchive will try to read this from the
// first archive in the multi archive list,
// then the next archive and so on until it succeeds.
//
// When combining archives with disjunct channel sets,
// a read request for a channel will yield data
// from the single archive that holds that channel.
//
// When channels are present in multiple archives,
// a read request for a given point in time will
// return values from the <I>first archive listed</I> in the master file
// that has values for that channel and point in time.
//
// Consequently, one should avoid archives with overlapping time ranges.
// If this cannot be avoided, the "most important" archive should be
// listed first.

class MultiArchive : public ArchiveI
{
public:
    class ChannelInfo
    {
    public:
        stdString	_name;			// name of the channel
        epicsTime		_first_time;	// Time stamp of first value
        epicsTime		_last_time;		// Time stamp of last value
    };

    //* Open a MultiArchive for the given master file
    MultiArchive(const stdString &master_file, 
		 const epicsTime &from = nullTime,
		 const epicsTime &to = nullTime);

    //* All virtuals from CLASS ArchiveI are implemented,
    // except that the "write" routines fail for this read-only archive type.

    virtual ChannelIteratorI *newChannelIterator() const;
    virtual ValueIteratorI *newValueIterator() const;
    virtual ValueI *newValue(DbrType type, DbrCount count);
    virtual bool findFirstChannel(ChannelIteratorI *channel);
    virtual bool findChannelByName(const stdString &name,
                                   ChannelIteratorI *channel);
    virtual bool findChannelByPattern(const stdString &regular_expression,
                                      ChannelIteratorI *channel);
    virtual bool addChannel(const stdString &name, ChannelIteratorI *channel);

    // debugging only
    void log() const;

    // For given channel, set value_iterator to value at-or-after time.
    // For result=false, value_iterator could not be set.
    // These routines will not clear() the value_iterator
    // to allow stepping back when used from within next()/prev()!
    bool getValueAfterTime(MultiChannelIterator &channel_iterator,
                           const epicsTime &time,
                           MultiValueIterator &value_iterator) const;

    bool getValueAtOrBeforeTime(MultiChannelIterator &channel_iterator,
                                const epicsTime &time, bool exact_time_ok,
                                MultiValueIterator &value_iterator) const;

    bool getValueNearTime(MultiChannelIterator &channel_iterator,
                          const epicsTime &time,
                          MultiValueIterator &value_iterator) const;

private:
    friend class MultiChannelIterator;
    friend class MultiChannel;
    
    bool parseMasterFile(const stdString &master_file, 
                         const epicsTime &from, const epicsTime &to);

    // Fill _channels from _archives
    void queryAllArchives();
    // Get/insert ChannelInfo for given name
    size_t getChannelInfoIndex(const stdString &name);

    stdList<stdString>     _archives; // names of archives
    bool _queriedAllArchives;
    stdVector<ChannelInfo> _channels; // summarized over all archives
};

#endif
