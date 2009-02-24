#ifndef __BINVALUEITERATOR_H__
#define __BINVALUEITERATOR_H__

#include "ValueIteratorI.h"
#include "BinValue.h"
#include "DataFile.h"

//////////////////////////////////////////////////////////////////////
//CLASS BinValueIterator
// BinValueIterator is the fundamental implementation
// of an CLASS ValueIteratorI for Channel Archiver files.
// It holds a DataHeaderIterator to loop
// over the buffers in a DataFile.
// In addition it also reads the associated CLASS BinCtrlInfo
// for each buffer and allows to loop over each CLASS BinValue
// in a buffer, automatically switching to the
// following buffer.
//
// This iterator allows access to each Value as it's found
// in the archive, repeat counts for example are returned as such.
// Most clients might prefer using a filtering iterator like
// the CLASS ExpandingValueIterator in addition.
class BinValueIterator : public ValueIteratorI
{
public:
	BinValueIterator();
	BinValueIterator(const BinValueIterator &rhs);
	BinValueIterator & operator = (const BinValueIterator &rhs);
	~BinValueIterator();

	// Open ValueOterator for first value in header.
	// Will skip empty buffers.
	// With last_value==true it will position on the last value of that buffer.
	bool attach(class BinChannel *channel, const stdString &data_file,
				FileOffset header_offset, bool last_value=false);

	void clear();

	//* Implementation of pure virtuals from  CLASS ValueIteratorI
	bool isValid() const;
	size_t determineChunk(const epicsTime &until);
	double getPeriod() const;
	const ValueI *getValue() const;
	bool next();
	bool prev();

	// Low-level API ---------------------------------
	// Not to be used by general clients,
	// not available for ValueIteratorI.
	//
	// Get next buffer & CtrlInfo, does not get value
	bool nextBuffer();
	bool prevBuffer();
	const DataHeaderIterator &getHeader()	{ return _header; }

	// Read value from current buffer.
	bool readValue(size_t index);

	class BinArchive *getArchive();

protected:
	BinValueIterator (const DataHeaderIterator	&header);

	void buildFromHeader (bool last_value=false);

	class BinChannel	*_channel;
	DataHeaderIterator	_header;
	CtrlInfo			_ctrl_info;
	FileOffset			_ctrl_info_offset;
	BinValue			*_value;
	size_t				_value_index;	// Index of current value: 0 .. _header->getNumSamples ()-1

	// Seek & fetch CtrlInfo
	bool readCtrlInfo ();

	// Allocate Value class for current buffer.
	bool checkValueType ();


	// Get next value in current buffer.
	// Contrary to ++ this method does
	// not switch to the next data buffer
	// or next file.
	bool nextBufferValue ();
	bool prevBufferValue ();
};

#endif //__BINVALUEITERATOR_H__

