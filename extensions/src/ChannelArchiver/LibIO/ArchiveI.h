// ------------- -*- c++ -*- ------------------------------
// $Id: ArchiveI.h,v 1.1.1.1 2001/04/09 17:48:28 luchini Exp $ 
//
// Please refer to NOTICE.txt,
// included as part of this distribution,
// for legal information.
//
// Kay-Uwe Kasemir, kasemir@lanl.gov
// --------------------------------------------------------

#ifndef __ARCHIVEI_H__
#define __ARCHIVEI_H__

#include "ChannelIteratorI.h"

//CLASS ArchiveI
// The ArchiveI class is the top-level interface
// to the ChannelArchiver data files.
// <P>
// Like CLASS ChannelIteratorI,
// CLASS ValueIteratorI etc.
// this class defines an <I>interface</I>.
//
// You cannot instantiate an ArchiveI class but only obtain
// a pointer to one from a factory class that
// implements the actual routines.
// <P>
// <UL>
// <LI>CLASS BinArchive handles access to the original, binary data format.
// <LI>CLASS MultiArchive allows simultaneous read-access to multiple archives
// </UL>
// <P>
// Client programs might prefer to use the "smart pointer" CLASS Archive
// instead of the raw ArchiveI interface pointer.
// <P>
// <H2>Multiple, Simultaneous Access</H2>
// The ChannelArchiver API is not thread-safe,
// additional semaphores are needed for multi-threaded access.
//
// Within a single thread, multiple archives can be used.
// For each CLASS Archive, more than one CLASS ChannelIterator can be obtained.
// For each ChannelIterator, a single CLASS ValueIterator can be used.
// This allows to read multiple channels from many archives at once.
// To access values for the <I>same</I> channel from different points in time,
// multiple ChannelIterators (each set to the same channel) must be used.
//
class ArchiveI
{
public:
	virtual ~ArchiveI ();

	//* Factory methods to create archive-specific iterators and values,
	//
	// See CLASS ChannelIteratorI,
	// CLASS ValueIteratorI,
	// CLASS ValueI.
	virtual ChannelIteratorI *newChannelIterator () const = 0;
	virtual ValueIteratorI *newValueIterator () const = 0;
	virtual ValueI *newValue (DbrType type, DbrCount count) = 0;

	//* Make Channel iterator point to first channel.
	// (ChannelIteratorI has to exist,
	//  for creation see newChannelIterator() below)
	virtual bool findFirstChannel (ChannelIteratorI *channel) = 0;

	//* Find channel by exact name or regular expression pattern
	virtual bool findChannelByName (const stdString &name, ChannelIteratorI *channel) = 0;
	virtual bool findChannelByPattern (const stdString &regular_expression, ChannelIteratorI *channel) = 0;

	//* Add new channel to archive
	// (undefined behaviour if channel exists before,
	// check with findChannelByName() to avoid problems)
	virtual bool addChannel (const stdString &name, ChannelIteratorI *channel) = 0;
};

//CLASS Archive
// The Archive class is a "smart pointer" wrapper
// for an CLASS ArchiveI  interface pointer.
//
// <H2>Examples</H2>
// <UL>
// <LI> List all the channels in an archive (BinArchive in this case):
//	<PRE>
//  #include&lt;BinArchive.h&gt;
//  #include&lt;ArchiveException.h&gt;
//  int main(int argc, const char argv[])
//  {
//      try
//      {
//          Archive archive(new BinArchive(argv[1]));
//          ChannelIterator channel(archive);
//          archive.findFirstChannel(channel);
//          while (channel)
//          {
//              cout << channel->getName() << endl;
//              ++ channel;
//          }
//      }
//      catch (ArchiveException &e)
//      {
//          cout << "Error: " << e.what() << "\n";
//      }
//      return 0;
//  }
//	</PRE>
// <LI> Find channels that match a given regular expression:
//	<PRE>
//	ChannelIterator channel(archive);
//	archive.findChannelByPattern ("Input[0-9]", channel);
//	...
//	</PRE>
// </UL>
//
// See CLASS ChannelIterator
// about access to values.
//
// All methods might throw an CLASS ArchiveException.
class Archive
{
public:
	Archive (ArchiveI *archive);
	~Archive ();

	void detach ();

	//* Set CLASS ChannelIterator on first channel.
	// Iterator allows access to next channel,
	// but not necessarily sorted (Hash Table).
	bool findFirstChannel (ChannelIterator &channel);

	//* Find channel by exact name (fast since using Hash Table)
	bool findChannelByName (const stdString &name, ChannelIterator &channel);

	//* Find first channel that matches given pattern.
	// If pattern is empty, findFirstChannel() will be called.
	bool findChannelByPattern (const stdString &regular_expression, ChannelIterator &channel);

	// Write access ------------------------------------------------------

	// Add a new channel to this archive and make ChannelIteratorI point to it.
	// The channel must not exist when calling addChannel,
	// use findChannelByName to check!
	bool addChannel (const stdString &name, ChannelIterator &channel);

	//* Create a value suitable for the given DbrType/Count
	ValueI *newValue (DbrType type, DbrCount count);

	// After transfering "ownership", remember to detach() Archive from ArchiveI !
	ArchiveI *getI()				{ return _ptr; }
	const ArchiveI *getI() const	{ return _ptr; }

private:
	Archive (const Archive &rhs); // not impl.
	Archive & operator = (const Archive &rhs); // not impl.

	ArchiveI *_ptr;
};

inline Archive::Archive (ArchiveI *archive)
{	_ptr = archive; }

inline Archive::~Archive ()
{
	if (_ptr)
	{
		delete _ptr;
		_ptr=0;
	}
}

inline void Archive::detach ()
{	_ptr = 0;	}

inline bool Archive::findFirstChannel (ChannelIterator &channel)
{	return _ptr->findFirstChannel (channel.getI()); }

inline bool Archive::findChannelByName (const stdString &name, ChannelIterator &channel)
{	return _ptr->findChannelByName (name, channel.getI()); }

inline bool Archive::findChannelByPattern (const stdString &regular_expression, ChannelIterator &channel)
{	return _ptr->findChannelByPattern (regular_expression, channel.getI()); }

inline bool Archive::addChannel (const stdString &name, ChannelIterator &channel)
{	return _ptr->addChannel (name, channel.getI()); }

inline ValueI *Archive::newValue (DbrType type, DbrCount count)
{	return _ptr->newValue (type, count); }

#endif //__ARCHIVEI_H__
