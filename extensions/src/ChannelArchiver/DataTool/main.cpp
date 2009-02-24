// ArchiveDataTool ----------

// System
#include <stdio.h>
// Base
#include <epicsVersion.h>
// Tools
#include <AutoPtr.h>
#include <ArgParser.h>
#include <BenchTimer.h>
#include <epicsTimeHelper.h>
// Storage
#include <OldDirectoryFile.h>
#include <IndexFile.h>
#include <DataFile.h>
#include <RawDataReader.h>
#include <DataWriter.h>

int verbose;

bool do_enforce_off = false;

// TODO: export/import: ASCII or XML
// TODO: delete/rename channel

void show_hash_info(const stdString &index_name)
{
    IndexFile index(3);
    index.open(index_name, true);
    index.showStats(stdout);
}

void count_channel_values(RTree *tree, const stdString &directory,
                          DbrType &type, DbrCount &count, size_t &blocks, size_t &values)
{
    type   = 0;
    count  = 0;
    blocks = 0;
    values = 0;
    if (! tree)
        return;
    DataFile *datafile;
    AutoPtr<DataHeader> header;
    RTree::Datablock block;
    RTree::Node node(tree->getM(), true);
    stdString start, end;
    int idx;
    bool ok;
    for (ok = tree->getFirstDatablock(node, idx, block);
         ok;
         ok = tree->getNextDatablock(node, idx, block))
    {
        ++blocks;
        datafile = DataFile::reference(directory,
                                       block.data_filename,
                                       false);
        header = datafile->getHeader(block.data_offset);
        datafile->release();
        if (header)
        {
            if (blocks == 1)
            {
                type  = header->data.dbr_type;
                count = header->data.dbr_count;
            }
            values += header->data.num_samples;
            header = 0;
        }
        else
            printf("Cannot read header in data file.\n");
    }
}

void list_names(const stdString &index_name, bool channel_detail)
{
    IndexFile index(3);
    IndexFile::NameIterator names;
    epicsTime stime, etime, t0, t1;
    stdString directory, start, end;
    bool ok;
    size_t channels = 0, blocks = 0, values = 0;
    index.open(index_name, true);
    if (channel_detail)
        printf("# Name\tType\tCount\tStart\tEnd\tBlocks\tValues\n");
    for (ok = index.getFirstChannel(names);
         ok;
         ok = index.getNextChannel(names))
    {
        AutoPtr<RTree> tree(index.getTree(names.getName(), directory));
        tree->getInterval(stime, etime);
        if (channels == 0)
        {
            t0 = stime;
            t1 = etime;
        }
        else
        {
            if (t0 > stime)
                t0 = stime;
            if (t1 < etime)
                t1 = etime;
        }
	size_t chan_blocks, chan_values;
        DbrType  type;
        DbrCount count;
        count_channel_values(tree, directory, type, count, chan_blocks, chan_values);
        ++channels;
        blocks += chan_blocks;
        values += chan_values;
	if (channel_detail)
            printf("%s\t%d\t%d\t%s\t%s\t%zu\t%zu\n",
                   names.getName().c_str(),
                   (int) type, (int) count,
                   epicsTimeTxt(stime, start),
                   epicsTimeTxt(etime, end),
                   chan_blocks, chan_values);
    }
    printf("%zu channels, %zu values, %s - %s\n",
           channels, values,
           epicsTimeTxt(t0, start), epicsTimeTxt(t1, end));
}

DataHeader *get_dataheader(const stdString &dir, const stdString &file,
                           FileOffset offset)
{
    DataFile *datafile = DataFile::reference(dir, file, false);
    if (!datafile)
        return 0;
    DataHeader *header = datafile->getHeader(offset);
    datafile->release(); // ref'ed by header
    return header; // might be NULL
}

// Try to determine the period and a good guess
// for the number of samples for a channel.
void determine_period_and_samples(IndexFile &index,
                                  const stdString &channel,
                                  double &period, size_t &num_samples)
{
    period      = 1.0; // initial guess
    num_samples = 10;
    // Whatever fails only means that there is no data,
    // so the initial guess remains OK.
    // Otherwise we peek into the last datablock.
    stdString directory;
    AutoPtr<RTree> tree(index.getTree(channel, directory));
    if (!tree)
        return;
    RTree::Node node(tree->getM(), false);
    RTree::Datablock block;
    int i;
    if (!tree->getLastDatablock(node, i, block))
        return;
    AutoPtr<DataHeader> header;
    if (!(header = get_dataheader(directory,
                                  block.data_filename, block.data_offset)))
        return;
    period = header->data.period;
    num_samples = header->data.num_samples;
    if (verbose > 2)
        printf("Last source buffer: Period %g, %lu samples.\n",
               period, (unsigned long) num_samples);
}

// Helper for copy: Handles samples of a single channel
void copy_channel(const stdString &channel_name,
                  const epicsTime *start, const epicsTime *end,
                  IndexFile &index, RawDataReader &reader,
                  IndexFile &new_index,
                  size_t &channel_count, size_t &value_count, size_t &back_count)
{
    double          period = 1.0;
    size_t          num_samples = 4096;    
    AutoPtr<DataWriter> writer;
    RawValue::Data *last_value = 0;
    bool            last_value_set = false;
    size_t          count = 0, back = 0;
    if (verbose > 1)
    {
        printf("Channel '%s': ", channel_name.c_str());
        if (verbose > 3)
            printf("\n");
        fflush(stdout);
    }
    determine_period_and_samples(index, channel_name, period, num_samples);
    const RawValue::Data *value = reader.find(channel_name, start);
    while (value && start && RawValue::getTime(value) < *start)
    {   // Correct for "before-or-at" idea of find()
        if (verbose > 2)
            printf("Skipping sample before start time\n");
        value = reader.next();
    }
    for (/**/; value; value = reader.next())
    {
        if (end  &&  RawValue::getTime(value) >= *end)
            break; // Reached or exceeded end time
        if (!writer || reader.changedType() || reader.changedInfo())
        {   // Need new or different writer
            writer = new DataWriter(
                new_index, channel_name,
                reader.getInfo(), reader.getType(), reader.getCount(),
                period, num_samples);
            RawValue::free(last_value);
            last_value_set = false;
            last_value = RawValue::allocate(reader.getType(),
                                            reader.getCount(), 1);
            if (!writer  ||  !last_value)
            {
                printf("Cannot allocate DataWriter/RawValue\n");
                return;
            }
        }
        if (RawValue::getTime(value) < writer->getLastStamp())
        {
            ++back;
            if (verbose > 2)
                printf("Skipping %lu back-in-time values\r",
                       (unsigned long) back);
            continue;
        }
        if (! writer->add(value))
        {
            printf("DataWriter::add claims back-in-time\n");
            continue;
        }
        // Keep track of last time stamp and status (not value!)
        RawValue::setStatus(last_value,
                            RawValue::getStat(value),
                            RawValue::getSevr(value));
        RawValue::setTime(last_value, RawValue::getTime(value));
        last_value_set = true;
        ++count;
        if (verbose > 3 && (count % 1000 == 0))
            printf("Copied %lu values\r", (unsigned long) count);
        
    }
    if (do_enforce_off && last_value_set && count > 0 &&
        RawValue::getSevr(last_value) != ARCH_STOPPED)
    {   // Try to add an "Off" Sample.
        RawValue::setStatus(last_value, 0, ARCH_STOPPED);
        writer->add(last_value);        
    }
    writer = 0;
    if (verbose > 1)
    {
        if (back)
            printf("%lu values, %lu back-in-time                       \n",
                   (unsigned long) count, (unsigned long) back);
        else
            printf("%lu values                                         \n",
                   (unsigned long) count);
    }
    ++channel_count;
    value_count += count;
    back_count += back;
}

// Copy samples from archive with index_name
// to new index copy_name.
// Uses all samples in source archive or  [start ... end[ .
void copy(const stdString &index_name, const stdString &copy_name,
          int RTreeM, const epicsTime *start, const epicsTime *end,
          const stdString &single_name)
{
    IndexFile               index(RTreeM), new_index(RTreeM);
    IndexFile::NameIterator names;
    size_t                  channel_count = 0, value_count = 0, back_count = 0;
    BenchTimer              timer;
    stdString               dir1, dir2;
    Filename::getDirname(index_name, dir1);
    Filename::getDirname(copy_name, dir2);
    if (dir1 == dir2)
    {
        printf("You have to assert that the new index (%s)\n"
               "is in a  directory different from the old index\n"
               "(%s)\n", copy_name.c_str(), index_name.c_str());
        return;
    }
    index.open(index_name, true);
    new_index.open(copy_name, false);
    if (verbose)
        printf("Copying values from '%s' to '%s'\n",
               index_name.c_str(), copy_name.c_str());
    RawDataReader reader(index);
    if (single_name.empty())
    {
        bool ok = index.getFirstChannel(names);
        while (ok)
        {
            copy_channel(names.getName(), start, end, index, reader,
                         new_index, channel_count, value_count, back_count);
            ok = index.getNextChannel(names);
        }    
    }
    else
        copy_channel(single_name, start, end, index, reader,
                     new_index, channel_count, value_count, back_count);
    new_index.close();
    index.close();
    timer.stop();
    if (verbose)
    {
        printf("Total: %lu channels, %lu values\n",
               (unsigned long) channel_count, (unsigned long) value_count);
        printf("Skipped %lu back-in-time values\n",
               (unsigned long) back_count);
        printf("Runtime: %s\n", timer.toString().c_str());
    }
}

void convert_dir_index(int RTreeM,
                       const stdString &dir_name, const stdString &index_name)
{
    OldDirectoryFile dir;
    if (!dir.open(dir_name))
        return;
    
    OldDirectoryFileIterator channels = dir.findFirst();
    IndexFile index(RTreeM);
    stdString index_directory;
    if (verbose)
        printf("Opened directory file '%s'\n", dir_name.c_str());
    index.open(index_name, false);
    if (verbose)
        printf("Created index '%s'\n", index_name.c_str());
    for (/**/;  channels.isValid();  channels.next())
    {
        if (verbose)
            printf("Channel '%s':\n", channels.entry.data.name);
        if (! Filename::isValid(channels.entry.data.first_file))
        {
            if (verbose)
                printf("No values\n");
            continue;
        }
        AutoPtr<RTree> tree(index.addChannel(channels.entry.data.name, index_directory));
        if (!tree)
        {
            fprintf(stderr, "Cannot add channel '%s' to index '%s'\n",
                    channels.entry.data.name, index_name.c_str());
            continue;
        }   
        DataFile *datafile =
            DataFile::reference(dir.getDirname(),
                                channels.entry.data.first_file, false);
        AutoPtr<DataHeader> header(
            datafile->getHeader(channels.entry.data.first_offset));
        datafile->release();
        while (header && header->isValid())
        {
            if (verbose)
            {
                stdString start, end;
                epicsTime2string(header->data.begin_time, start);
                epicsTime2string(header->data.end_time, end);    
                printf("'%s' @ 0x0%lX: %s - %s\n",
                       header->datafile->getBasename().c_str(),
                       (unsigned long)header->offset,
                       start.c_str(),
                       end.c_str());
            }
            if (!tree->insertDatablock(
                    header->data.begin_time, header->data.end_time,
                    header->offset,  header->datafile->getBasename()))
            {
                fprintf(stderr, "insertDatablock failed for channel '%s'\n",
                        channels.entry.data.name);
                break;
            }
            header->read_next();
        }
    }
    DataFile::close_all();
}


void convert_index_dir(const stdString &index_name, const stdString &dir_name)
{
    IndexFile::NameIterator names;
    IndexFile index(50);
    stdString index_directory;
    RTree::Datablock block;
    stdString start_file, end_file;
    FileOffset start_offset, end_offset;
    epicsTime start_time, end_time;
    bool ok;
    index.open(index_name.c_str(), true);
    OldDirectoryFile dir;
    if (!dir.open(dir_name, true))
    {
        fprintf(stderr, "Cannot create  '%s'\n", dir_name.c_str());
        return;
    }
    for (ok=index.getFirstChannel(names); ok; ok=index.getNextChannel(names))
    {
        if (verbose)
            printf("Channel '%s':\n", names.getName().c_str());
        AutoPtr<const RTree> tree(index.getTree(names.getName(),
                                                index_directory));
        RTree::Node node(tree->getM(), false);
        int i;
        if (!tree->getFirstDatablock(node, i, block))
        {
            printf("No first datablock for channel '%s'\n",
                   names.getName().c_str());
            continue;
        }
        start_file   = block.data_filename;
        start_offset = block.data_offset;
        if (!tree->getLastDatablock(node, i, block))
        {
            printf("No last datablock for channel '%s'\n",
                   names.getName().c_str());
            continue;
        }
        end_file   = block.data_filename;
        end_offset = block.data_offset;
        // Check by reading first+last buffer & getting times
        bool times_ok = false;
        AutoPtr<DataHeader> header(
            get_dataheader(index_directory, start_file, start_offset));
        if (header)
        {
            start_time = header->data.begin_time;
            header = get_dataheader(index_directory, end_file, end_offset);
            if (header)
            {
                end_time = header->data.end_time;
                header = 0;
                times_ok = true;
            }
        }
        if (times_ok)
        {
            stdString start, end;
            if (verbose)
                printf("%s - %s\n",
                       epicsTimeTxt(start_time, start),
                       epicsTimeTxt(end_time, end));
            OldDirectoryFileIterator dfi;
            dfi = dir.find(names.getName());
            if (!dfi.isValid())
                dfi = dir.add(names.getName());
            dfi.entry.setFirst(start_file, start_offset);
            dfi.entry.setLast(end_file, end_offset);
            dfi.entry.data.create_time = start_time;
            dfi.entry.data.first_save_time = start_time;
            dfi.entry.data.last_save_time = end_time;
            dfi.save();
        }
    }
}

unsigned long dump_datablocks_for_channel(IndexFile &index,
                                          const stdString &channel_name,
                                          unsigned long &direct_count,
                                          unsigned long &chained_count)
{
    DataFile *datafile;
    AutoPtr<DataHeader> header;
    direct_count = chained_count = 0;
    stdString directory;
    AutoPtr<RTree> tree(index.getTree(channel_name, directory));
    if (! tree)
        return 0;
    RTree::Datablock block;
    RTree::Node node(tree->getM(), true);
    stdString start, end;
    int idx;
    bool ok;
    if (verbose > 1)
        printf("RTree M for channel '%s': %d\n", channel_name.c_str(),
               tree->getM());
    if (verbose > 2)
        printf("Datablocks for channel '%s':\n", channel_name.c_str());
    for (ok = tree->getFirstDatablock(node, idx, block);
         ok;
         ok = tree->getNextDatablock(node, idx, block))
    {
        ++direct_count;
        if (verbose > 2)
            printf("'%s' @ 0x%lX: Indexed range %s - %s\n",
                   block.data_filename.c_str(),
                   (unsigned long)block.data_offset,
                   epicsTimeTxt(node.record[idx].start, start),
                   epicsTimeTxt(node.record[idx].end, end));
        if (verbose > 3)
        {
            datafile = DataFile::reference(directory,
                                           block.data_filename,
                                           false);
            header = datafile->getHeader(block.data_offset);
            datafile->release();
            if (header)
            {
                header->show(stdout, true);
                header = 0;
            }
            else
                printf("Cannot read header in data file.\n");
        }
        bool first_hidden_block = true;
        while (tree->getNextChainedBlock(block))
        {
            if (first_hidden_block && verbose > 2)
            {
                first_hidden_block = false;
                printf("Hidden blocks with smaller time range:\n");
            }
            ++chained_count;
            if (verbose > 2)
            {
                printf("---  '%s' @ 0x%lX\n",
                       block.data_filename.c_str(),
                       (unsigned long)block.data_offset);
                if (verbose > 3)
                {
                    datafile = DataFile::reference(directory,
                                                   block.data_filename,
                                                   false);
                    header = datafile->getHeader(block.data_offset);
                    if (header)
                    {
                        header->show(stdout, false);
                        header = 0;
                    }
                    else
                        printf("Cannot read header in data file.\n");
                    datafile->release();
                }   
            }
        }
        printf("\n");
    }
    return direct_count + chained_count;
}

unsigned long dump_datablocks(const stdString &index_name,
                              const stdString &channel_name)
{
    unsigned long direct_count, chained_count;
    IndexFile index(3);
    index.open(index_name, true);
    dump_datablocks_for_channel(index, channel_name,
                                direct_count, chained_count);
    printf("%ld data blocks, %ld hidden blocks, %ld total\n",
           direct_count, chained_count,
           direct_count + chained_count);
    return direct_count + chained_count;
}

unsigned long dump_all_datablocks(const stdString &index_name)
{
    unsigned long total = 0, direct = 0, chained = 0, channels = 0;
    IndexFile index(3);
    bool ok;
    index.open(index_name, true);
    IndexFile::NameIterator names;
    for (ok = index.getFirstChannel(names);
         ok;  ok = index.getNextChannel(names))
    {
        ++channels;
        unsigned long direct_count, chained_count;
        total += dump_datablocks_for_channel(index, names.getName(),
                                             direct_count, chained_count);
        direct += direct_count;
        chained += chained_count;
    }
    printf("Total: %ld channels, %ld datablocks\n", channels, total);
    printf("       %ld visible datablocks, %ld hidden\n", direct, chained);
    return total;
}

void dot_index(const stdString &index_name, const stdString channel_name,
               const stdString &dot_name)
{
    IndexFile index(3);
    index.open(index_name, true);
    stdString directory;
    AutoPtr<RTree> tree(index.getTree(channel_name, directory));
    if (!tree)
    {
        fprintf(stderr, "Cannot find '%s' in index '%s'.\n",
                channel_name.c_str(), index_name.c_str());
        return;
    }
    tree->makeDot(dot_name.c_str());
}

bool seek_time(const stdString &index_name,
               const stdString &channel_name,
               const stdString &start_txt)
{
    epicsTime start;
    if (!string2epicsTime(start_txt, start))
    {
        fprintf(stderr, "Cannot convert '%s' to time stamp\n",
                start_txt.c_str());
        return false;
    }
    IndexFile index(3);
    index.open(index_name, true);
    stdString directory;
    AutoPtr<RTree> tree(index.getTree(channel_name, directory));
    if (tree)
    {
        RTree::Datablock block;
        RTree::Node node(tree->getM(), true);
        int idx;
        if (tree->searchDatablock(start, node, idx, block))
        {
            stdString s, e;
            printf("Found block %s - %s\n",
                   epicsTimeTxt(node.record[idx].start, s),
                   epicsTimeTxt(node.record[idx].end, e));
        }
        else
            printf("Nothing found\n");
    }
    else
        fprintf(stderr, "Cannot find channel '%s'\n", channel_name.c_str());
    return true;
}

bool check (const stdString &index_name)
{
    IndexFile index(3);
    index.open(index_name, true);
    return index.check(verbose);
}

int main(int argc, const char *argv[])
{
    CmdArgParser parser(argc, argv);
    parser.setHeader("Archive Data Tool version " ARCH_VERSION_TXT ", "
                     EPICS_VERSION_STRING
                      ", built " __DATE__ ", " __TIME__ "\n\n"
                     );
    parser.setArgumentsInfo("<index-file>");
    CmdArgFlag help          (parser, "help", "Show help");
    CmdArgInt  verbosity     (parser, "verbose", "<level>", "Show more info");
    CmdArgFlag info          (parser, "info", "Simple archive info");
    CmdArgFlag list_index    (parser, "list", "List channel name info");
    CmdArgString copy_index  (parser, "copy", "<new index>", "Copy channels");
    CmdArgString start_time  (parser, "start", "<time>",
                              "Format: \"mm/dd/yyyy[ hh:mm:ss[.nano-secs]]\"");
    CmdArgString end_time    (parser, "end", "<time>", "(exclusive)");
    CmdArgDouble file_limit  (parser, "file_limit", "<MB>", "File Size Limit");
    CmdArgString basename    (parser, "basename", "<string>", "Basename for new data files");
    CmdArgFlag enforce_off   (parser, "append_off", "Enforce a final 'Archive_Off' value when copying data");
    CmdArgString dir2index   (parser, "dir2index", "<dir. file>",
                              "Convert old directory file to index");
    CmdArgString index2dir   (parser, "index2dir", "<dir. file>",
                              "Convert index to old directory file");
    CmdArgInt RTreeM         (parser, "M", "<1-100>", "RTree M value");
    CmdArgFlag dump_blocks   (parser, "blocks", "List channel's data blocks");
    CmdArgFlag all_blocks    (parser, "Blocks", "List all data blocks");
    CmdArgString dotindex    (parser, "dotindex", "<dot filename>",
                              "Dump contents of RTree index into dot file");
    CmdArgString channel_name(parser, "channel", "<name>", "Channel name");
    CmdArgFlag hashinfo      (parser, "hashinfo", "Show Hash table info");
    CmdArgString seek_test   (parser, "seek", "<time>", "Perform seek test");
    CmdArgFlag test          (parser, "test", "Perform some consistency tests");

    try
    {
        // Defaults
        RTreeM.set(50);
        file_limit.set(100.0);
        // Get Arguments
        if (! parser.parse())
            return -1;
        if (help   ||   parser.getArguments().size() != 1)
        {
            parser.usage();
            return -1;
        }
        // Consistency checks
        if ((dump_blocks ||
             dotindex.get().length() > 0  ||
             seek_test.get().length() > 0)
            && channel_name.get().length() <= 0)
        {
            fprintf(stderr,
                    "Options 'blocks' and 'dotindex' require 'channel'.\n");
            return -1;
        }
        verbose = verbosity;
        stdString index_name = parser.getArgument(0);
        DataWriter::file_size_limit = (unsigned long)(file_limit*1024*1024);
        if (file_limit < 10.0)
            fprintf(stderr, "file_limit values under 10.0 MB are not useful\n");
        // Start/end time
        epicsTime *start = 0, *end = 0;
        stdString txt;
        if (start_time.get().length() > 0)
        {
            start = new epicsTime;
            string2epicsTime(start_time.get(), *start);
            if (verbose > 1)
                printf("Using start time %s\n", epicsTimeTxt(*start, txt));
        }
        if (end_time.get().length() > 0)
        {
            end = new epicsTime();
            string2epicsTime(end_time.get(), *end);
            if (verbose > 1)
                printf("Using end time   %s\n", epicsTimeTxt(*end, txt));
        }
        // Base name
        if (basename.get().length() > 0)
    	    DataWriter::data_file_name_base = basename.get();
        if (enforce_off)
    	    do_enforce_off = true;
        // What's requested?
        if (info)
    	    list_names(index_name, false);
        else if (list_index)
            list_names(index_name, true);
        else if (copy_index.get().length() > 0)
        {
            copy(index_name, copy_index, RTreeM, start, end, channel_name);
            return 0;
        }
        else if (hashinfo)
            show_hash_info(index_name);
        else if (dir2index.get().length() > 0)
        {
            convert_dir_index(RTreeM, dir2index, index_name);
            return 0;
        }
        else if (index2dir.get().length() > 0)
        {
            convert_index_dir(index_name, index2dir);
            return 0;
        }
        else if (all_blocks)
        {
            dump_all_datablocks(index_name);
            return 0;
        }
        else if (dump_blocks)
        {
            dump_datablocks(index_name, channel_name);
            return 0;
        }
        else if (dotindex.get().length() > 0)
        {
            dot_index(index_name, channel_name, dotindex);
            return 0;
        }
        else if (seek_test.get().length() > 0)
        {
            seek_time(index_name, channel_name, seek_test);
            return 0;
        }
        else if (test)
        {
            return check(index_name) ? 0 : -1;
        }
        else
        {
            parser.usage();
            return -1;
        }
    }
    catch (GenericException &e)
    {
        fprintf(stderr, "Error:\n%s\n", e.what());
    }
        
    return 0;
}

