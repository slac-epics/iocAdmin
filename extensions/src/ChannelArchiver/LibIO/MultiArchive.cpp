// --------------------------------------------------------
// $Id: MultiArchive.cpp,v 1.1.1.1 2005/12/21 16:26:13 luchini Exp $
//
// Please refer to NOTICE.txt,
// included as part of this distribution,
// for legal information.
//
// Kay-Uwe Kasemir, kasemir@lanl.gov
// --------------------------------------------------------

#include <math.h>
#include "MultiArchive.h"
#include "MultiChannelIterator.h"
#include "MultiValueIterator.h"
#include "BinArchive.h"
#include "ArchiveException.h"
#include <ASCIIParser.h>
#include <BinaryTree.h>

// Open a MultiArchive for the given master file
MultiArchive::MultiArchive(const stdString &master_file, 
                           const epicsTime &from, const epicsTime &to)
{
    _queriedAllArchives = false;
    if (! parseMasterFile(master_file, from, to))
        throwDetailedArchiveException(OpenError, master_file);
}

ChannelIteratorI *MultiArchive::newChannelIterator() const
{   return new MultiChannelIterator((MultiArchive *)this); }

ValueIteratorI *MultiArchive::newValueIterator() const
{   return new MultiValueIterator(); }

ValueI *MultiArchive::newValue(DbrType type, DbrCount count)
{   return 0; }

static bool does_file_exist(const stdString &filename)
{
    FILE *f = fopen(filename.c_str(), "rb");
    if (!f)
        return false;
    fclose(f);
    return true;
}

bool MultiArchive::parseMasterFile(const stdString &master_file,
                                   const epicsTime &from, const epicsTime &to)
{
#ifdef DEBUG_MULTIARCHIVE
    LOG_MSG("MultiArchive::parseMasterFile\n");
#endif
    stdString sub_archive;
    // Carefully check if "master_file" starts with the magic line:
    FILE *file = fopen(master_file.c_str(), "r");
    if (!file)
    {
        LOG_MSG("Cannot open master file '%s'\n",
                master_file.c_str());
        return false;
    }
    char line[14]; // master_version  : 14 chars
    if (fread(line, 14, 1, file) != 1)
    {
        LOG_MSG("Invalid master file '%s'\n", master_file.c_str());
        return false; // too small to be anything
    }
    fclose(file);
    if (strncmp(line, "master_version", 14) !=0)
    {
        // Could be Binary file:
        if (does_file_exist(master_file))
        {
            _archives.push_back(master_file);
            return true;
        }
        return false;
    }
    // OK, looks like a master file.
    // Now read it again as ASCII:
    ASCIIParser parser;
    if (! parser.open(master_file))
        return false;

    // Performance of MultiArchive:
    // Example:
    //     50 (weekly) Archives, each ~1GB / ~3000 channels
    //     All in 1 MultiArchive
    //     Command: ArchiveExport -start <T0> -end <T1> <MultiArchive> 
    //                            -match <any pattern>
    //
    //  1. plain, searching all archives in MultiArchive
    //     (master_version=1)                            ~90 seconds
    //  2. searching all archives that intersect timerange
    //     (exteded master_version=1)                    ~6 seconds
    //  3. searching all archives that intersect timerange
    //     (master_version=2)                            ~2 seconds
    stdString parameter, value;
    if (parser.nextLine()                      &&
        parser.getParameter(parameter, value)  &&
        parameter == "master_version")
    {
        if (value == "1")
        {
            // version 1 has one archive to search per line
            while (parser.nextLine())
            {
                sub_archive = parser.getLine();
                if (!does_file_exist(sub_archive))
                    continue;
                if (!isValidTime(from) && !isValidTime(to))
                {
                    // No timestamps
                    // -> we have to use every Archive in the Multi-Archive
#ifdef DEBUG_MULTIARCHIVE
                    LOG_MSG("sub-archive: %s\n", sub_archive.c_str());
#endif
                    _archives.push_back(sub_archive);
                }
                else
                {
                    // avoiding to search archives that can't contain queried
                    // values at all should save some time...
                    Archive archive(new BinArchive(sub_archive));
                    ChannelIterator channel(archive);
                    epicsTime start, end, time;
                    for (archive.findFirstChannel(channel); channel; ++channel)
                    {
                        time = channel->getFirstTime();
                        if (isValidTime(time) &&
                            (time < start  ||  start == nullTime))
                            start = time;
                        time = channel->getLastTime();
                        if (time > end)
                            end = time;
                    }
                    if ( (!isValidTime(from) || ( from < end ) ) &&
                         (!isValidTime(to)   || ( to >= start ) ) ) {
#ifdef DEBUG_MULTIARCHIVE
                        LOG_MSG("sub-archive: %s\n", sub_archive.c_str());
#endif
                        _archives.push_back(sub_archive);
                    }
                }
            }
        }
        else if (value == "2")
        {
            // version 2 has 2 timestamps (from, to) and an archive per line
            // the timestamps denote the times for which the archive contains
            // values
            while (parser.nextLine())
            {
                epicsTime f, t;
                string2epicsTime(parser.getLine().substr(0, 19), f);
                string2epicsTime(parser.getLine().substr(20, 19), t);
                if ( !isValidTime(f) || !isValidTime(t) )
                {
                    LOG_MSG("Invalid timestamp in master file '%s' line %zu\n",
                            master_file.c_str(), parser.getLineNo());
                    continue;
                }
                if ( (!isValidTime(from) || ( from < t ) ) &&
                     (!isValidTime(to)   || ( to >= f ) ) )
                {
                    sub_archive = parser.getLine().substr(40);
                    if (!does_file_exist(sub_archive))
                        continue;
#ifdef DEBUG_MULTIARCHIVE
                    LOG_MSG("sub-archive: %s\n", sub_archive.c_str());
#endif
                    _archives.push_back(sub_archive);
                }
            }
        }
        else
        {
            LOG_MSG("Invalid master file '%s' version %s\n",
                    master_file.c_str(), value.c_str());
            return false;
        }
    }
    else
    {
        LOG_MSG("Invalid master file '%s', maybe wrong version\n",
                master_file.c_str());
        return false;
    }
    return true;
}

void MultiArchive::log() const
{
    LOG_MSG("MultiArchive:\n");
    stdList<stdString>::const_iterator archs = _archives.begin();
    while (archs != _archives.end())
    {
        LOG_MSG("Archive file: %s\n", archs->c_str());
        ++archs;
    }
    
    LOG_MSG("Channels:\n");
    stdVector<ChannelInfo>::const_iterator chans = _channels.begin();
    stdString start, end;
    while (chans != _channels.end())
    {
        epicsTime2string(chans->_first_time, start);
        epicsTime2string(chans->_last_time, end);
        LOG_MSG("%s: %s, %s\n",
                chans->_name.c_str(), start.c_str(), end.c_str());
        ++chans;
    }
}

// Get/insert index of ChannelInfo for given name
size_t MultiArchive::getChannelInfoIndex(const stdString &name)
{
    size_t i;
    for (i=0; i < _channels.size(); ++i)
        if (_channels[i]._name == name)   // found
            return i;
    ChannelInfo new_info;
    new_info._name = name;
    _channels.push_back(new_info);
    return _channels.size()-1;
}

// Fill _channels from _archives
void MultiArchive::queryAllArchives()
{
    if (_queriedAllArchives)
        return;
#ifdef DEBUG_MULTIARCHIVE
    LOG_MSG("MultiArchive::queryAllArchives\n");
#endif
    
    stdString name;
    size_t i;
    ChannelInfo *info;
    epicsTime time;

    stdList<stdString>::iterator archs = _archives.begin();
    for (/**/; archs != _archives.end(); ++archs)
    {
        Archive archive (new BinArchive (*archs));
        ChannelIterator channel (archive);
        archive.findFirstChannel (channel);
        while (channel)
        {
            name = channel->getName();
            i = getChannelInfoIndex(name);
            info = &_channels[i];
            // check bounds
            time = channel->getFirstTime();
            if (isValidTime(time) &&
                (!isValidTime(info->_first_time) ||
                 info->_first_time > time))
                info->_first_time = time;
            time = channel->getLastTime();
            if (isValidTime(time) &&
                info->_last_time < time)
                info->_last_time = time;
            ++ channel;
        }
    }
    _queriedAllArchives = true;
}

bool MultiArchive::findFirstChannel(ChannelIteratorI *channel)
{
    MultiChannelIterator *multi_channel_iter =
        dynamic_cast<MultiChannelIterator *>(channel);
    queryAllArchives();
    return multi_channel_iter->assign(this, 0);
}

bool MultiArchive::findChannelByName(const stdString &name,
                                     ChannelIteratorI *channel)
{
    MultiChannelIterator *multi_channel_iter =
        dynamic_cast<MultiChannelIterator *>(channel);
    size_t i;

    if (_queriedAllArchives)
    {
        for (i=0; i < _channels.size(); ++i)
            if (_channels[i]._name == name)
                return multi_channel_iter->assign(this, i);
    }
    else
    {
        stdList<stdString>::iterator archs = _archives.begin();
        for (/**/; archs != _archives.end(); ++archs)
        {
            Archive archive (new BinArchive(*archs));
            ChannelIterator channel(archive);
            if (archive.findChannelByName(name, channel))
            {
                i = getChannelInfoIndex(name);
                return multi_channel_iter->assign(this, i);
            }
        }         
    }
    multi_channel_iter->clear();
    return false;
}

bool MultiArchive::findChannelByPattern(const stdString &regular_expression,
                                        ChannelIteratorI *channel)
{
    MultiChannelIterator *multi_channel =
        dynamic_cast<MultiChannelIterator *>(channel);
    queryAllArchives();
    return findFirstChannel(channel) &&
        multi_channel->moveToMatchingChannel(regular_expression);
}

bool MultiArchive::addChannel(const stdString &name,
                              ChannelIteratorI *channel)
{
    throwDetailedArchiveException(Invalid,
                                  "Cannot write, MultiArchive is read-only");
    return false;
}

// See comment in header file
bool MultiArchive::getValueAfterTime(
    MultiChannelIterator &channel_iterator,
    const epicsTime &time,
    MultiValueIterator &value_iterator) const
{
#ifdef DEBUG_MULTIARCHIVE
    LOG_MSG("MultiArchive::parseMasterFile\n");
#endif
    if (channel_iterator._channel_index >= _channels.size())
        return false;

    stdString last_containing_archive;
    const ChannelInfo &info = _channels[channel_iterator._channel_index];
    stdList<stdString>::const_iterator archs = _archives.begin();
    // Find archive with first stamp <= time <= last stamp
    for (/**/; archs != _archives.end(); ++archs)
    {
        Archive archive (new BinArchive(*archs));
        ChannelIterator channel(archive);
        if (archive.findChannelByName(info._name, channel))
        {
            if (channel->getLastTime() < time)
                continue; // can't be in here
            if (channel->getFirstTime() >= time)
            {
                // ALL values in here are "after" time,
                // but maybe there is a better archive
                // that contains values close to given time
                last_containing_archive = *archs;
                continue;
            }
            ValueIterator value(archive);
            // Does this archive have values after "time"?
            if (! channel->getValueAfterTime(time, value))
                continue;
            // position() will delete ref's to previous
            // ArchiveI, ChannelIteratorI, ...
            // -> remove ref. to values first, then channeliterator/archive
            value_iterator.position(&channel_iterator, value.getI());
            channel_iterator.position(archive.getI(), channel.getI());

            value.detach();    // Now ref'd by MultiValueIterator
            archive.detach();  // Now ref'd by MultiChannelIterator
            channel.detach();  // dito
            return value_iterator.isValid();
        }                    
    }
    // Was there an archive where all is after time?
    if (!last_containing_archive.empty())
    {
        Archive archive(new BinArchive(last_containing_archive));
        ChannelIterator channel(archive);
        if (!archive.findChannelByName(info._name, channel))
            return false;
        ValueIterator value(archive);
        // Does this archive have values after "time"?
        if (! channel->getValueAfterTime(time, value))
            return false;
        value_iterator.position(&channel_iterator, value.getI());
        channel_iterator.position(archive.getI(), channel.getI());
        value.detach();    // Now ref'd by MultiValueIterator
        archive.detach();   // Now ref'd by MultiChannelIterator
        channel.detach();   // dito
        return value_iterator.isValid();
    }                    
        
    return false;
}

bool MultiArchive::getValueAtOrBeforeTime(
    MultiChannelIterator &channel_iterator,
    const epicsTime &time, bool exact_time_ok,
    MultiValueIterator &value_iterator) const
{
    if (channel_iterator._channel_index >= _channels.size())
        return false;

    const ChannelInfo &info = _channels[channel_iterator._channel_index];
    stdList<stdString>::const_iterator archs = _archives.begin();
    for (/**/; archs != _archives.end(); ++archs)
    {
        Archive archive (new BinArchive(*archs));
        ChannelIterator channel(archive);
        if (archive.findChannelByName(info._name, channel))
        {
            // getValueBeforeTime() could succeed for '=='.
            // Do we allow '=='?
            if (!exact_time_ok && channel->getFirstTime() >= time)
                continue;
            ValueIterator value(archive);
            // Does this archive have values before "time"?
            if (! channel->getValueBeforeTime(time, value))
                continue;
            // position() will delete ref's to previous
            // ArchiveI, ChannelIteratorI, ...
            // -> remove ref. to values first, then channeliterator/archive
            value_iterator.position(&channel_iterator, value.getI());
            channel_iterator.position(archive.getI(), channel.getI());

            value.detach();    // Now ref'd by MultiValueIterator
            archive.detach();   // Now ref'd by MultiChannelIterator
            channel.detach();   // dito
            return value_iterator.isValid();
        }                    
    }
    return false;
}

bool MultiArchive::getValueNearTime(
    MultiChannelIterator &channel_iterator,
    const epicsTime &time, MultiValueIterator &value_iterator) const
{
    if (channel_iterator._channel_index >= _channels.size())
        return false;

    // Query all archives in MultiArchive for value nearest "time"
    double best_bet = -1.0; // negative == invalid

    const ChannelInfo &info = _channels[channel_iterator._channel_index];
    stdList<stdString>::const_iterator archs = _archives.begin();
    for (/**/; archs != _archives.end(); ++archs)
    {
        Archive archive (new BinArchive(*archs));
        ChannelIterator channel(archive);
        if (archive.findChannelByName(info._name, channel))
        {
            ValueIterator value(archive);
            if (! channel->getValueNearTime(time, value))
                continue;

            double distance = fabs(value->getTime() - time);  
            if (best_bet >= 0  &&  best_bet <= distance)
                continue; // worse than what we found before

            best_bet = distance;
            value_iterator.position(&channel_iterator, value.getI());
            channel_iterator.position(archive.getI(), channel.getI());
            value.detach ();    // Now ref'd by MultiValueIterator
            archive.detach();   // Now ref'd by MultiChannelIterator
            channel.detach();   // dito
        }                    
    }

    if (best_bet >= 0)
        return value_iterator.isValid ();
    return false;
}

