// $id$ -*- c++ -*-
#ifndef __GNUPLOTEXPORTER_H__
#define __GNUPLOTEXPORTER_H__

#include "SpreadSheetExporter.h"

//CLASS GNUPlotExporter
// GNUPlotExporter generates a text file for GNUPlot
// together with a simple plotting script.
//
// The data file will be called "<filename>",
// the script (unless using pipe) will be called "<filename>.plt".
//
// Implements CLASS Exporter.
class GNUPlotExporter : public SpreadSheetExporter
{
public:
    //* Contrary to the simple CLASS Exporter,
    // <I>filename</I> has to be defined
    // for GNUPlotExporter because two files are created
    // and <I>filename</I> is used as a base name.
    // reduce: # of buckets to reduce data into, 0 -> don't reduce
    GNUPlotExporter(Archive &archive, const stdString &filename,
                     int reduce = 0);
    GNUPlotExporter(ArchiveI *archive, const stdString &filename,
                    int reduce = 0);

    void exportChannelList(const stdVector<stdString> &channel_names);

    //* Issue GNUPlot commands to generate an image file
    // (<filename>.png)
    void makeImage()      { _make_image = true; }
    static const char *imageExtension();

    //* Call GNUPlot and run the command script via pipe
    // instead of dumping script to disk
    void usePipe()        { _use_pipe = true; }

    //* Set Y scale limits. Default: auto
    void setY0(double y0) { _y0 = y0; }
    void setY1(double y1) { _y1 = y1; }

    //* Use log scale for y axes
    void useLogscale()    { _use_logscale = true; }
        
private:
    bool _make_image;
    bool _use_pipe;

    //* # of buckets to reduce data into, 0 -> don't reduce
    int _reduce;

    double _y0, _y1;
    bool _use_logscale;
};

#endif //__GNUPLOTEXPORTER_H__
