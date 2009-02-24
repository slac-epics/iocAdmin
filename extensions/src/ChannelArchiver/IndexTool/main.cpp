// System
#include <stdio.h>
#include <string.h>
#include <signal.h>
// Base
#include <epicsVersion.h>
// Tools
#include <AutoPtr.h>
#include <BenchTimer.h>
#include <stdString.h>
#include <Filename.h>
#include <ArgParser.h>
#include <epicsTimeHelper.h>
#include <IndexConfig.h>
#include <Lockfile.h>
// Index
#include <IndexFile.h>

int verbose;

bool run = true;

static void signal_handler(int sig)
{
    run = false;
    fprintf(stderr, "Exiting on signal %d\n", sig);
}

void add_tree_to_master(const stdString &index_name,
                        const stdString &sub_name,
                        const stdString &dirname,
                        const stdString &channel,
                        const RTree *subtree,
                        RTree *index)
{
    try
    {
        RTree::Datablock block;
        RTree::Node node(subtree->getM(), true);
        int idx;
        bool ok;
        stdString datafile, start, end;
        // Xfer data blocks from the end on, going back to the start.
        // Stop when finding a block that was already in the master index.
        for (ok = subtree->getLastDatablock(node, idx, block);
             ok && run;
             ok = subtree->getPrevDatablock(node, idx, block))
        {
            // Data files in master need a full path ....
            if (Filename::containsFullPath(block.data_filename))
                datafile = block.data_filename;
            else // or are written relative to the dir. of the index file
                Filename::build(dirname, block.data_filename, datafile);
            if (verbose > 3)
                printf("Inserting '%s' as '%s'\n",
                       block.data_filename.c_str(), datafile.c_str());
            if (verbose > 2)
                printf("'%s' @ 0x%lX: %s - %s\n",
                       datafile.c_str(),
                       (unsigned long)block.data_offset,
                       epicsTimeTxt(node.record[idx].start, start),
                       epicsTimeTxt(node.record[idx].end, end));
            // Note that there's no inner loop over the 'chained'
            // blocks, we only handle the main blocks of each sub-tree.
            if (!index->updateLastDatablock(node.record[idx].start,
                                            node.record[idx].end,
                                            block.data_offset, datafile))
            {
                if (verbose > 2)
                     printf("Block already existed, skipping the rest\n");
                return;
            }
        }
    }
    catch (GenericException &e)
    {
        fprintf(stderr,
                "Error adding channel '%s' from '%s' to '%s':\n%s\n",
                channel.c_str(), sub_name.c_str(), index_name.c_str(), e.what());
    }
}

void create_masterindex(int RTreeM,
                        const stdString &config_name,
                        const stdString &index_name,
                        bool reindex)
{
    IndexConfig config;
    if (reindex)
        config.subarchives.push_back(config_name);
    else
        if (!config.parse(config_name))
            throw GenericException(__FILE__, __LINE__,
                                   "File '%s' is no XML indexconfig",
                                   config_name.c_str());
    IndexFile::NameIterator names;
    stdString index_directory, sub_directory;
    IndexFile index(RTreeM), subindex(RTreeM);
    bool ok;
    index.open(index_name, false);
    if (verbose)
        printf("Opened master index '%s'.\n", index_name.c_str());
    stdList<stdString>::const_iterator subs;
    for (subs  = config.subarchives.begin();
         subs != config.subarchives.end(); ++subs)
    {
        const stdString &sub_name = *subs;
        try
        {
            subindex.open(sub_name);
            if (verbose)
            {
                printf("Adding sub-index '%s'\n", sub_name.c_str());
                fflush(stdout);
            }
            for (ok = subindex.getFirstChannel(names);
                 ok && run;
                 ok = subindex.getNextChannel(names))
            {
                const stdString &channel = names.getName();
                if (verbose > 1)
                    printf("'%s'\n", channel.c_str());
                AutoPtr<RTree> subtree(subindex.getTree(channel, sub_directory));
                if (!subtree)
                {
                    fprintf(stderr, "Cannot get tree for '%s' from '%s'\n",
                            channel.c_str(), sub_name.c_str());
                    continue;
                }
                AutoPtr<RTree> tree(index.addChannel(channel, index_directory));
                if (!tree)
                {
                    fprintf(stderr, "Cannot add '%s' to '%s'\n",
                            channel.c_str(), index_name.c_str());
                    continue;
                }
                add_tree_to_master(index_name, sub_name, sub_directory,
                                   channel, subtree, tree);
            }
            subindex.close();
            if (verbose > 2)
                index.showStats(stdout);
        }
        catch (GenericException &e)
        {
            fprintf(stderr, "Error while processing sub-index '%s':\n%s\n",
                    sub_name.c_str(), e.what());
        }
    }
    index.close();
    if (verbose)
        printf("Closed master index '%s'.\n", index_name.c_str());
}

int main(int argc, const char *argv[])
{
    signal(SIGINT,  signal_handler);
    signal(SIGTERM, signal_handler);
    CmdArgParser parser(argc, argv);
    parser.setHeader("ArchiveIndexTool version " ARCH_VERSION_TXT ", "
                     EPICS_VERSION_STRING
                      ", built " __DATE__ ", " __TIME__ "\n\n"
                     );
    parser.setArgumentsInfo("<archive list file/old index> <output index>");
    CmdArgFlag help   (parser, "help", "Show Help");
    CmdArgInt  RTreeM (parser, "M", "<3-100>", "RTree M value");
    CmdArgFlag reindex(parser, "reindex", "Build new index from old index (no list file)");
    CmdArgInt  verbose_flag (parser, "verbose", "<level>", "Show more info");
    RTreeM.set(50);
    if (!(parser.parse()  &&  parser.getArguments().size() == 2))
    {
        parser.usage();
        return -1;
    }
    verbose = verbose_flag;
    stdString old_index_name = parser.getArgument(0);
    stdString new_index_name = parser.getArgument(1);
    if (Filename::containsPath(new_index_name))
    {
        fprintf(stderr, "The new index file name, '%s',"
                " must not contain any path information,\n",
                new_index_name.c_str());
        fprintf(stderr, "it has to refer to a file in the current directory.\n");
        return -1;
    }
    if (parser.getArguments().size() != 2  ||  help)
    {
        parser.usage();
        printf("\nThis tools reads an archive list file, ");
        printf("that is a file listing sub-archives,\n");
        printf("and generates a master index for all of them ");
        printf("in the output index.\n");
        printf("With the -reindex flag, it creates a new index ");
        printf("from an existing index\n");
        printf("(shortcut for creating a list file that contains only ");
        printf(" a single old index).\n");
        return -1;
    }

    try
    {
        Lockfile lock_file("indextool_active.lck", argv[0]);
        BenchTimer timer;
        create_masterindex(RTreeM, old_index_name, new_index_name, reindex);
        timer.stop();
        if (verbose > 0)
            printf("Time: %s\n", timer.toString().c_str());
    }
    catch (GenericException &e)
    {
        printf("Error:\n%s\n", e.what());
        return -1;
    }
    return 0;
}

