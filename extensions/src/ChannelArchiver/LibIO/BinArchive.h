// --------------------------------------------------------
// $Id: BinArchive.h,v 1.1.1.1 2004/04/01 20:49:41 luchini Exp $
//
// Please refer to NOTICE.txt,
// included as part of this distribution,
// for legal information.
//
// Kay-Uwe Kasemir, kasemir@lanl.gov
// --------------------------------------------------------

#ifndef __BINARCHIVE_H__
#define __BINARCHIVE_H__

#include "ArchiveI.h"

//CLASS BinArchive
//
// BinArchive is the implementation of the CLASS ArchiveI interface
// for the binary data file format.
class BinArchive : public ArchiveI
{
public:
	//* Open or create archive.
	// Will fail with for_write==false if there is no existing archive.
	BinArchive (const stdString &archive_name, bool for_write=false);
	~BinArchive ();

	//* ArchiveI interface
	bool findFirstChannel (ChannelIteratorI *channel);
	bool findChannelByName (const stdString &name, ChannelIteratorI *channel);
	bool findChannelByPattern (const stdString &regular_expression, ChannelIteratorI *channel);
	bool addChannel (const stdString &name, ChannelIteratorI *channel);
	ChannelIteratorI *newChannelIterator () const;
	ValueIteratorI *newValueIterator () const;
	ValueI *newValue (DbrType type, DbrCount count);         

	enum {
		SECS_PER_DAY = 60*60*24,
		SECS_PER_MONTH = 60*60*24*31 // depends on month, ... this is a magic number
	};        

	// each data file holds data for this period
	unsigned long getSecsPerFile ()
	{	return _secs_per_file;	}

	void setSecsPerFile (unsigned long seconds)
	{	_secs_per_file = seconds;	}

	void makeDataFileName (const ValueI &value, stdString &data_file_name);

	void calcNextFileTime (const epicsTime &time, epicsTime &next_file_time);

	bool makeFullFileName (const stdString &basename, stdString &full_name);

private:
	BinArchive (const BinArchive &); // not impl.
	BinArchive & operator = (const BinArchive &); // not impl.

	class DirectoryFile	*_dir;
	unsigned long _secs_per_file;		// each data file holds data for this period
};

#endif //__BINARCHIVE_H__
