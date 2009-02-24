// --------------------------------------------------------
// $Id: main.cpp,v 1.1.1.1 2006/02/15 15:03:41 luchini Exp $
//
// Please refer to NOTICE.txt,
// included as part of this distribution,
// for legal information.
//
// Kay-Uwe Kasemir, kasemir@lanl.gov
// --------------------------------------------------------
//
// ArchiveManager:
//
// Could end up being a command-line tool
// for simple ChannelArchive management:
// * List channels
// * List values
// * Extract subsection into new archive
// * ??
//
// Right now it's more a collection of API tests.
//

// Warning: names too long for debug info
#ifdef WIN32
#pragma warning (disable: 4786)
#endif

#include <signal.h>
#include <epicsVersion.h>
#include <ArgParser.h>
#include <Filename.h>
#include <epicsTimeHelper.h>
#include "ArchiverConfig.h"
#include "BinArchive.h"
#include "BinValueIterator.h"
#include "ExpandingValueIteratorI.h"
#include "ascii.h"
#include "DirectoryFile.h"

#ifdef MANAGER_USE_MULTI
#   include "MultiArchive.h"
#   define ARCHIVE_TYPE MultiArchive
#else
#   define ARCHIVE_TYPE BinArchive
#endif

// For communication sigint_handler -> main loop
static bool run = true;
static void signal_handler(int sig)
{
    run = false;
    printf("exiting on signal %d as soon as possible\n", sig);
}                                                        

// List all channel names for given pattern
// (leave empty to list all channels)
void list_channels(const stdString &archive_name, const stdString &pattern)
{
    Archive         archive(new ARCHIVE_TYPE(archive_name));
    ChannelIterator channel(archive);

    archive.findChannelByPattern(pattern, channel);
    while (run && channel)
    {
        printf("%s\n", channel->getName());
        ++ channel;
    }
}

// List all the values stampes start <= value.getTime() < end
// for a given channel,
// iterated version.
// start == end == 0 will list all values.
void list_values(const stdString &archive_name, const stdString &channel_name,
                 const epicsTime &start, const epicsTime &end, int repeat_floor)
{
    Archive         archive(new ARCHIVE_TYPE(archive_name));
    ChannelIterator channel(archive);
    ValueIterator   value(archive);
    stdString time_text, stat_text;

    if (! archive.findChannelByName(channel_name, channel))
    {
        fprintf(stderr, "Cannot find channel '%s'\n", channel_name.c_str());
        return;
    }

    printf("Channel '%s'\n", channel->getName());
    channel->getValueAfterTime(start, value);
    while (run && value &&(end == nullTime  ||  value->getTime() < end))
    {
        if ((repeat_floor < 0) ||
            (value->getSevr() != ARCH_REPEAT
             && value->getSevr() != ARCH_EST_REPEAT) ||
            (value->getStat() >= repeat_floor) )
        {
            value->show(stdout);
            fputs("\n", stdout);
        }
        ++ value;
    }
}

void dump(const stdString &archive_name, const stdString &channel_pattern,
          const epicsTime &start, const epicsTime &end, int repeat_floor)
{
    Archive         archive(new ARCHIVE_TYPE(archive_name));
    ChannelIterator channel(archive);
    ValueIterator   value(archive);

    archive.findChannelByPattern(channel_pattern, channel);
    while (run && channel)
    {
        printf("Channel '%s'\n", channel->getName());
        channel->getValueAfterTime(start, value);
        while (run && value && (end == nullTime  ||  value->getTime() < end))
        {
            if ((repeat_floor < 0) ||
                (value->getSevr() != ARCH_REPEAT &&
                 value->getSevr() != ARCH_EST_REPEAT) ||
                (value->getStat() >= repeat_floor) )
            {
                value->show(stdout);
                fputs("\n", stdout);
            }
            ++ value;
        }
        ++channel;
    }
}

void show_info(const stdString &archive_name, const stdString &pattern)
{
    Archive         archive(new ARCHIVE_TYPE(archive_name));
    ChannelIterator channel(archive);
    size_t channel_count = 0;
    epicsTime start, end, time;
    stdString s, e;

    for (archive.findChannelByPattern(pattern, channel); channel; ++channel)
    {
        ++channel_count;
        
        time = channel->getFirstTime();
        if (isValidTime(time) &&  (time < start  ||  start == nullTime))
            start = time;
        time = channel->getLastTime();
        if (time > end)
            end = time;
        if (pattern.empty())
            continue;
        epicsTime2string(channel->getFirstTime(), s);
        epicsTime2string(channel->getLastTime(), e);
        printf("%s\t%s\t%s\n", s.c_str(), e.c_str(), channel->getName());
    }
    epicsTime2string(start, s);
    epicsTime2string(end, e);
    printf("Channel count : %u\n", (unsigned)channel_count);
    printf("First sample  : %s\n", s.c_str());
    printf("Last  sample  : %s\n", e.c_str());
}

void show_channel_info(const stdString &archive_name,
                       const stdString &channel_name)
{
    Archive         archive(new ARCHIVE_TYPE(archive_name));
    ChannelIterator channel(archive);
    stdString       s, e;
    
    if (archive.findChannelByName(channel_name, channel))
    {
        epicsTime2string(channel->getFirstTime(), s);
        epicsTime2string(channel->getLastTime(), e);
        printf("start: %s\n", s.c_str());
        printf("end  : %s\n", e.c_str());
    }
    else
        printf("'%s': not found\n", channel_name.c_str());
}

void seek_time(const stdString &archive_name,
               const stdString &channel_name,
               const epicsTime &start)
{
    Archive         archive(new ARCHIVE_TYPE(archive_name));
    ChannelIterator channel(archive);
    ValueIterator   value(archive);

    if (! archive.findChannelByName(channel_name, channel))
    {
        fprintf(stderr, "Cannot find channel '%s'\n", channel_name.c_str());
        return;
    }

    printf("Channel '%s'\n", channel->getName());

    channel->getValueNearTime(start, value);
    if (value)
    {
        stdString s;
        epicsTime2string(value->getTime(), s);
        printf("Found: %s\n", s.c_str());
    }
    else
        printf("Not found\n");
}

// "Export", copy all values from [start...end[
// for all channels that match pattern
// into new archive.
//
// Start, end may be 0 to copy the whole archive.
//
// Certain repeat counts can be suppressed.
// (this makes the code more complicated to read...)
void do_export(const stdString &archive_name, 
               const stdString &channel_pattern,
               const epicsTime &start, const epicsTime &end,
               const stdString &new_dir_name,
               size_t repeat_limit,
               size_t repeat_floor,
               size_t days_per_file)
{
    size_t i, chunk, chunk_count=0, val_count=0;
    epicsTime next_file_time, last_stamp;

    Archive old_archive(new BinArchive(archive_name));
    BinArchive *new_archiveI = new BinArchive(new_dir_name, true);
    if (days_per_file > 0)
        new_archiveI->setSecsPerFile(BinArchive::SECS_PER_DAY*days_per_file);
    else
        new_archiveI->setSecsPerFile(BinArchive::SECS_PER_MONTH);
    Archive new_archive(new_archiveI);

    ChannelIterator old_channel(old_archive);
    ChannelIterator new_channel(new_archive);
    ValueIterator values(old_archive);

    for (old_archive.findChannelByPattern(channel_pattern, old_channel); 
         run && old_channel;
         ++old_channel)
    {
        printf("%s", old_channel->getName());
        fflush(stdout);
        chunk_count = val_count = 0;

            if (! old_channel->getValueAfterTime(start, values))
            {
                printf("\t0 chunks\t0 values\n");
                continue;
            }
            new_archiveI->calcNextFileTime(values->getTime(), next_file_time);
            if (isValidTime(next_file_time)  &&  next_file_time < end)
                chunk = values.determineChunk(next_file_time);
            else
                chunk = values.determineChunk(end);
            if (chunk <= 0)
            {
                printf("\t0 chunks\t0 values\n");
                continue;
            }

            if (new_archive.findChannelByName(old_channel->getName(),
                                              new_channel))
            {
                last_stamp = new_channel->getLastTime();
                if (isValidTime(start)  &&  last_stamp > start)
                {
                    stdString l, s;
                    epicsTime2string(last_stamp, l);
                    epicsTime2string(start, s);
                    printf("Error:\n"
                           "Archive '%s' does already contain\n"
                           "values for channel '%s'\n"
                           "and they are stamped %s, which is after\n"
                           "the start time of    %s\n",
                           new_dir_name.c_str(),
                           old_channel->getName(),
                           l.c_str(), s.c_str());
                    break;
                }
            }
            else
            {
                new_archive.addChannel(old_channel->getName(), new_channel);
                last_stamp = nullTime;
            }

            while (chunk > 0)
            {
                new_channel->lockBuffer(*values, values.getPeriod());
                for (i=0;   values && i<chunk;   ++i, ++values)
                {
                    if (! isValidTime(values->getTime()))
                    {
                        values->show(stdout);
                        printf("\tinvalid time stamp, skipped\n");
                        continue;
                    }
                    if (isValidTime(last_stamp))   // time stamp checks
                    {   
                        if (values->getTime() < last_stamp)
                        {
                            values->show(stdout);
                            printf("\tgoing back in time, skipped\n");
                            continue;
                        }
                    }
                    if (repeat_limit > 0 &&
                        (values->getSevr() == ARCH_REPEAT ||
                         values->getSevr() == ARCH_EST_REPEAT) &&
                        (size_t)values->getStat() >= repeat_limit)
                    {
                        values->show(stdout);
                        printf("\trepeat count beyond %u, skipped\n",
                               (unsigned)repeat_limit);
                        continue;
                    }
                    if (repeat_floor > 0 &&
                        (values->getSevr() == ARCH_REPEAT ||
                         values->getSevr() == ARCH_EST_REPEAT) &&
                        (size_t)values->getStat() <= repeat_floor)
                    {
                        values->show(stdout);
                        printf("\trepeat count below %u, skipped\n",
                               (unsigned)repeat_floor);
                        continue;
                    }
                    if (! new_channel->addValue(*values))
                    {
                        new_channel->addBuffer(*values, values.getPeriod(),
                                               chunk);
                        new_channel->addValue(*values);
                    }
                    last_stamp = values->getTime();
                }
                ++chunk_count;
                val_count += chunk;
                
                new_archiveI->calcNextFileTime(values->getTime(),
                                               next_file_time);
                if (isValidTime(next_file_time)  &&  next_file_time < end)
                    chunk = values.determineChunk(next_file_time);
                else
                    chunk = values.determineChunk(end);
            }
            new_channel->releaseBuffer();
        printf("\t%u chunks\t%u values\n",
               (unsigned)chunk_count, (unsigned)val_count);
    }
}

void compare(const stdString &archive_name, const stdString &target_name)
{
    Archive src(new ARCHIVE_TYPE(archive_name));
    Archive dst(new ARCHIVE_TYPE(target_name));
    ChannelIterator src_chan(src);
    ChannelIterator dst_chan(dst);
    ValueIterator src_val(src);
    ValueIterator dst_val(dst);

    for (src.findFirstChannel(src_chan); run && src_chan; ++src_chan)
    {
        printf("'%s' : ", src_chan->getName());
        if (! dst.findChannelByName(src_chan->getName(), dst_chan))
        {
            printf("not found\n");
            continue;
        }

        src_chan->getFirstValue(src_val);
        dst_chan->getFirstValue(dst_val);
	bool same = true;
        while (src_val && dst_val)
        {
            if (*src_val != *dst_val)
            {
                printf("difference:\n");
                printf("< ");
                src_val->show(stdout);
                printf("\n");
                printf("> ");
                dst_val->show(stdout);
                printf("\n");
		same = false;
            }
            ++src_val;
            ++dst_val;
        }
	while (src_val)
	{
	    printf("< ");
            src_val->show(stdout);
            printf("\n");
            ++src_val;
	    same = false;
	}
	while (dst_val)
	{
	    printf("> ");
            dst_val->show(stdout);
            printf("\n");
            ++dst_val;
	    same = false;
	}
        if (same)
            printf(" OK\n");
        fflush(stdout);
    }
}

// Test ExpandingValueIteratorI
void expand(const stdString &archive_name,
            const stdString &channel_name,
            const epicsTime &start,
            const epicsTime &end)
{
    Archive         archive(new ARCHIVE_TYPE(archive_name));
    ChannelIterator channel(archive);

    if (! archive.findChannelByName(channel_name, channel))
    {
        fprintf(stderr, "Cannot find channel '%s'\n", channel_name.c_str());
        return;
    }

    printf("Channel '%s'\n", channel->getName());
    ValueIterator   raw_values(archive);
    channel->getValueAfterTime(start, raw_values);
    ExpandingValueIteratorI *expand = new ExpandingValueIteratorI(raw_values);
    ValueIterator value(expand);
    while (value && (end == nullTime  ||  value->getTime() < end))
    {
        value->show(stdout);
        if (expand->isExpanded())
            printf("\t(expanded)");
        printf("\n");
        ++value;
    }
}

// Interpolation Test
void interpolate(const stdString &directory, const stdString &channel_name,
                 const epicsTime &start, const epicsTime &end,
                 double interpol)
{
#if 0
    Archive a(directory);
    ChannelIterator channel = a.findChannelByName(channel_name);
    if (! channel)
    {
        cerr << "Cannot find channel '" << channel_name << "':\n";
        return;
    }

    printf("Channel '" << channel->getName() << "\n");
    
    ValueIterator vi = channel.getValueAfterTime(start);
    ExpandingValueIterator evi(&vi);
    LinInterpolValueIterator livi(&evi, interpol);
    InfoFilterValueIterator value(&livi);
    while (value && (end == nullTime  ||  value->getTime() < end))
    {
        printf(*value << "\n");
        ++value;
    }
#endif
}

// This is low-level.
// The API shown in here should not be used by
// general Archive clients
void headers(const stdString &directory, const stdString &channel_name)
{
    Archive archive(new BinArchive(directory));
    ChannelIterator channel(archive);
    
    if (! archive.findChannelByName(channel_name, channel))
    {
        fprintf(stderr, "Cannot find channel '%s'\n", channel_name.c_str());
        return;
    }

    ValueIterator value(archive);
    if (! channel->getFirstValue(value))
    {
        fprintf(stderr, "No value for '%s'\n", channel_name.c_str());
        return;
    }

    printf("Channel '%s'\n", channel->getName());

    BinValueIterator *bvi = dynamic_cast<BinValueIterator *>(value.getI());
    const CtrlInfo *info;
    stdString t;
    do
    {
        printf("Buffer  : '%s' @ 0x%lX\n",
               bvi->getHeader().getFilename().c_str(),
               (unsigned long)bvi->getHeader().getOffset());
        printf("Prev    : '%s' @ 0x%lX\n",
               bvi->getHeader()->getPrevFile(),
               (unsigned long)bvi->getHeader()->getPrev());
        epicsTime2string(bvi->getHeader()->getBeginTime(), t);
        printf("Time    : %s\n", t.c_str());
        epicsTime2string(bvi->getHeader()->getEndTime(), t);
        printf("...     : %s\n", t.c_str());
        epicsTime2string(bvi->getHeader()->getNextTime(), t);
        printf("New File: %s\n", t.c_str());

        printf("Samples : %ld\n", bvi->getHeader()->getNumSamples());
        printf("Size    : %ld bytes, free: %ld bytes\n",
               bvi->getHeader()->getBufSize(), bvi->getHeader()->getBufFree());
        printf("Period  : %g\n", value.getPeriod());

        info = value->getCtrlInfo();
        if (info)
            info->show(stdout);
        else
            printf("No CtrlInfo\n");

        printf("Next    : '%s' @ 0x%lX\n",
               bvi->getHeader()->getNextFile(),
               (unsigned long)bvi->getHeader()->getNext());
        printf("\n");
    }
    while (run && bvi->nextBuffer());
}

void test(const stdString &directory, const epicsTime &start, const epicsTime &end)
{
    Archive archive (new ARCHIVE_TYPE(directory));
    ChannelIterator channel(archive);
    ValueIterator value(archive);
    size_t chan_errors, errors = 0, channels = 0;
    stdString t;
    
    archive.findFirstChannel (channel);
    while (run && channel)
    {
        printf(".");
        fflush(stdout);
        ++channels;
        channel->getValueAfterTime(start, value);
        chan_errors = 0;
        epicsTime last;
            while (run && value &&
                   (!isValidTime(end) || value->getTime() < end))
            {
                if (isValidTime(last))
                {
                    if (value->getTime() < last)
                    {
                        printf("\n'%s' back in time\n", channel->getName());
                        epicsTime2string(last, t);
                        printf("%s   is followed by:\n", t.c_str());
                        value->show(stdout);
                        printf("\n");
                        ++chan_errors;
                    }
                }
                last = value->getTime();
                ++value;
            }
        
        if (chan_errors)
        {
            errors += chan_errors;
            printf("\n'%s' : %u errors\n",
                   channel->getName(), (unsigned)chan_errors);
        }
        ++channel;
    }
    printf("\n");
    printf("%u channels\n", (unsigned)channels);
    printf("%u errors\n",   (unsigned)errors);
}

// Copy one channel name onto another new name
static void copy(const stdString &archive_name,
                 const stdString &channel_name, const stdString &new_name)
{
    DirectoryFile dir(archive_name, true /* for write */);
    DirectoryFileIterator o_entry = dir.find(channel_name);
    if (o_entry.isValid())
    {
        printf("Found %s\n", channel_name.c_str());
        DirectoryFileIterator n_entry = dir.add(new_name);
        BinChannel *o_ch = o_entry.getChannel();
        BinChannel *n_ch = n_entry.getChannel();
        
        if (n_entry.isValid())
            printf("Copied as '%s'\n", new_name.c_str());
        n_ch->setFirstTime(o_ch->getFirstTime());
        n_ch->setFirstFile(o_ch->getFirstFile());
        n_ch->setFirstOffset(o_ch->getFirstOffset());
        n_ch->setLastTime(o_ch->getLastTime());
        n_ch->setLastFile(o_ch->getLastFile());
        n_ch->setLastOffset(o_ch->getLastOffset());
        n_entry.save();
    }
    else
    {
        printf("Cannot find '%s'\n", channel_name.c_str());
    }
}

void delete_name(const stdString &archive_name, const stdString &channel_name)
{
    DirectoryFile dir(archive_name, true /* for write */);
    if (dir.remove(channel_name))
    {
        printf("Removed '%s' from index file\n", channel_name.c_str());
    }
    else
    {
        printf("Cannot remove '%s'\n", channel_name.c_str());
    }
}

#define EXPERIMENT
#ifdef EXPERIMENT
// Ever changing experimental routine,
// usually empty in "useful" versions of the ArchiveManager
void experiment(const stdString &archive_name,
                const stdString &channel_name,
                const epicsTime &start)
{
}
#endif

int main(int argc, const char *argv[])
{
    CmdArgParser parser(argc, argv);
    parser.setHeader("Archive Manager version " VERSION_TXT ", "
                     EPICS_VERSION_STRING
                      ", built " __DATE__ ", " __TIME__ "\n\n"
                     "You should not run the this tool on a live archive\n"
                     "Stop the ArchiveEngine before operating on an archive.\n"
                     "Create backups in case this tool damages your archive.\n"
                     "\n"
                     );
    parser.setArgumentsInfo("<archive index file>");

    CmdArgFlag   do_show_info   (parser, "info", "Show archive information");
    CmdArgFlag   do_test        (parser, "test", "Test archive for errors");
    CmdArgString channel_name   (parser, "channel", "<channel>", "Specify channel name");
    CmdArgString channel_pattern(parser, "match", "<regular expression>", "List matching channels");
    CmdArgString dump_channels  (parser, "Match", "<regular expression>", "Dump values for matching channels");
    CmdArgString start_text     (parser, "start", "<time>", "Start time as mm/dd/yyyy hh:mm:ss[.nano-secs]");
    CmdArgString end_text       (parser, "end", "<time>", "End time (exclusive)");
    CmdArgString export_archive (parser, "xport", "<new archive>", "export data into new archive");
    CmdArgInt    repeat_limit   (parser, "repeat_limit", "<seconds>", "remove 'repeat' entries beyond limit (export)");
    CmdArgInt    repeat_floor   (parser, "Repeat_floor", "<seconds>", "remove 'repeat' entries below limit (list/dump/export)");
    CmdArgInt    days_per_file  (parser, "FileSize", "<days>", "Days per binary data file (export, binary file format detail)");
    CmdArgString show_headers   (parser, "headers", "<channel>", "show headers for channel");
    CmdArgString ascii_output   (parser, "Output", "<channel>", "output ASCII dump for channel");
    CmdArgString ascii_input    (parser, "Input", "<ascii file>", "read ASCII dump for channel into archive");
    CmdArgString compare_target (parser, "Compare", "<target archive>", "Compare with target archive");
    CmdArgFlag   do_seek_test   (parser, "Seek", "Seek test (use with -start)");
    CmdArgString rename_channel (parser, "Rename", "<new name>", "Rename channel name, requires -c for old channel");
    CmdArgString delete_channel (parser, "DELETE", "<channel>", "Delete channel from index file");
#ifdef EXPERIMENT
    CmdArgFlag   do_experiment  (parser, "Experiment", "Perform experiment (temporary option)");
#endif

    if (! parser.parse())
        return -1;

    if (parser.getArguments().size() != 1)
    {
        parser.usage();
        return -1;
    }
    stdString archive_name = parser.getArgument(0);

    epicsTime start, end;
    string2epicsTime(start_text, start);
    string2epicsTime(end_text.get(), end);
    
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

        if (do_show_info)
        {
            if (channel_name.get().length() > 0)
                show_channel_info(archive_name, channel_name);
            else
                show_info(archive_name, channel_pattern);
        }
#ifdef EXPERIMENT
        else if (do_experiment)
            experiment(archive_name, channel_name, start);
#endif
        else if (do_test)
            test(archive_name, start, end);
        else if (ascii_output.get().length() > 0)
            output_ascii(archive_name, ascii_output.get(), start, end);
        else if (ascii_input.get().length() > 0)
            input_ascii(archive_name, ascii_input);
        else if (compare_target.get().length() > 0)
            compare(archive_name, compare_target);
        else if (do_seek_test)
            seek_time(archive_name, channel_name, start);
        else if (show_headers.get().length() > 0)
            headers(archive_name, show_headers);
        else if (rename_channel.get().length() > 0)
        {
            if (channel_name.get().length() <= 0)
            {
                fprintf(stderr, "Need channel name (option -c) for rename\n");
                return -1;
            }
            copy(archive_name, channel_name.get(), rename_channel.get());
            delete_name(archive_name, channel_name.get());
        }
        else if (delete_channel.get().length() > 0)
        {
            delete_name(archive_name, delete_channel.get());
        }
        else if (dump_channels.get().length() > 0)
            dump(archive_name, dump_channels, start, end, repeat_floor.get());
        else if (export_archive.get().length() > 0)
        {
            // Export matching channels, generate pattern for single channel
            // if only one channel was given
            if (channel_pattern.get().empty()  &&
                channel_name.get().length() > 0)
            {
                stdString pattern;
                pattern.reserve(channel_name.get().length() + 2);
                pattern += '^';
                pattern += channel_name.get();
                pattern += '$';
                channel_pattern.set(pattern);
            }
            do_export(archive_name, channel_pattern,
                      start, end, export_archive, 
		      (size_t) repeat_limit.get(), (size_t) repeat_floor.get(),
		      days_per_file.get());
        }
        else if (channel_name.get().empty())
            list_channels(archive_name, channel_pattern);
        else
            list_values(archive_name, channel_name, start, end, repeat_floor.get());
    printf("\n");

    return 0;
}
