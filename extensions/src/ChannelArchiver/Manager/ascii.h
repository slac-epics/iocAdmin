// --------------------------------------------------------
// $Id: ascii.h,v 1.1.1.1 2004/04/01 20:49:47 luchini Exp $
//
// Please refer to NOTICE.txt,
// included as part of this distribution,
// for legal information.
//
// Kay-Uwe Kasemir, kasemir@lanl.gov
// --------------------------------------------------------

// ASCII read/write routines
//
// When there is more time,
// these should become part of an ASCIIArchive/ChannelIterator/..
// class collection like the BinArchive/... family.

void output_ascii (const stdString &archive_name, const stdString &channel_name,
	const epicsTime &start, const epicsTime &end);

void input_ascii (const stdString &archive_name, const stdString &file_name);

