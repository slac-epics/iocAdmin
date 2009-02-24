#ifndef __SPREADSHEETEXPORTER_H__
#define __SPREADSHEETEXPORTER_H__

#include "Exporter.h"

//CLASS SpreadSheetExporter
// Generate a text file that's readable
// by SpreadSheet programs.
//
// Implements CLASS Exporter.
class SpreadSheetExporter : public Exporter
{
public:
    SpreadSheetExporter(ArchiveI *archive);
    SpreadSheetExporter(ArchiveI *archive, const stdString &filename);

    void useMatlabFormat()       { _use_matlab_format = true; }

    void exportChannelList(const stdVector<stdString> &channel_names);
private:
    const char *_comment;
    bool _use_matlab_format;
};


#endif //__SPREADSHEETEXPORTER_H__


