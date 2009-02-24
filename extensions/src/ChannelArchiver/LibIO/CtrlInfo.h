
#ifndef __CTRLINFO_H__
#define __CTRLINFO_H__

#include <stdio.h>
#include "MemoryBuffer.h"
#include "StorageTypes.h"

// This is the CtrlInfo for numeric values
// as it's in the binary data files.
// So far, the structure's layout matches the binary
// layout (except for the byte order of the individual elements),
// so this strcuture must not be changed!
class NumericInfo
{
public:
	float	disp_high;	// high display range
	float	disp_low;	// low display range
	float	low_warn;
	float	low_alarm;
	float	high_warn;
	float	high_alarm;
	long	prec;		// display precision
	char	units[1];	// actually as long as needed,
};

// Similar to NumericInfo, this is for enumerated channels
class EnumeratedInfo
{
public:
	short	num_states;		// state_strings holds num_states strings
	short	pad;			// one after the other, separated by '\0'
	char	state_strings[1];
};

// A glorified union of NumericInfo and EnumeratedInfo.
//
// type == CtrlInfo::Type
// Info::size includes the "size" and "type" field.
// The original archiver read/wrote "Info" that way,
// but didn't properly initialize it:
// size excluded size/type and was then rounded up by 8 bytes... ?!
class CtrlInfoData
{
public:
	unsigned short	size;
	unsigned short	type;
	union
	{
		NumericInfo		analog;
		EnumeratedInfo	index;
	}				value;
	// class will be as long as necessary
	// to hold the units or all the state_strings
};

/// CtrlInfo holds the meta-information for values: Units, limits, ...

/// A value is archived with control information.
/// Several values might share the same control information
/// for efficiency.
/// The control information is variable in size
/// because it holds units or state strings.
class CtrlInfo
{
public:
	CtrlInfo();
	CtrlInfo(const CtrlInfo &rhs);
	CtrlInfo& operator = (const CtrlInfo &rhs);
	virtual ~CtrlInfo ();

	bool operator == (const CtrlInfo &rhs) const;
	bool operator != (const CtrlInfo &rhs) const
	{	return ! (*this == rhs);	}

	/// Type defines the type of value
	typedef enum
	{
		Invalid = 0,   ///< Undefined
		Numeric = 1,   ///< A numeric CtrlInfo: Limits, units, ...
		Enumerated = 2 ///< An enumerated CtrlInfo: Strings
	}	Type;

    /// Get the Type for this CtrlInfo
	Type getType() const;

	/// Read Control Information:
	/// Numeric precision, units,
	/// high/low limits for display etc.:
	long getPrecision() const;
	const char *getUnits() const;
	float getDisplayHigh() const;
	float getDisplayLow() const;
	float getHighAlarm() const;
	float getHighWarning() const;
	float getLowWarning() const;
	float getLowAlarm() const;

	/// Initialize a Numeric CtrlInfo
	/// (sets Type to Numeric and then sets fields)
	void setNumeric(long prec, const stdString &units,
					float disp_low, float disp_high,
					float low_alarm, float low_warn,
                    float high_warn, float high_alarm);

	/// Initialize an Enumerated CtrlInfo
	void setEnumerated(size_t num_states, char *strings[]);

	/// Alternative to setEnumerated:
	/// Call with total string length, including all the '\0's !
	void allocEnumerated(size_t num_states, size_t string_len);

	/// Must be called after allocEnumerated()
	/// AND must be called in sequence,
	/// i.e. setEnumeratedString (0, ..
	///      setEnumeratedString (1, ..
	void setEnumeratedString(size_t state, const char *string);

	/// After allocEnumerated() and a sequence of setEnumeratedString ()
	/// calls, this method recalcs the total size
	/// and checks if the buffer is sufficient (Debug version only)
	void calcEnumeratedSize();

	/// Format a double value according to precision<BR>
	/// Throws Invalid if CtrlInfo is not for Numeric
	void formatDouble(double value, stdString &result) const;

	/// Enumerated: state string
	size_t getNumStates() const;
    
    /// Get given state as text (also handles undefined states)
	void getState(size_t state, stdString &result) const;

	/// Like strtod, strtol: try to parse,
	/// position 'next' on character following the recognized state text
	bool parseState(const char *text, const char **next, size_t &state) const;

    /// Read/write a CtrlInfo from/to a binary data file
    void read(FILE *file, FileOffset offset);
	void write(FILE *file, FileOffset offset) const;

    void show(FILE *f) const;

protected:
	const char *getState(size_t state, size_t &len) const;

	MemoryBuffer<CtrlInfoData>	_infobuf;
};

inline CtrlInfo::Type CtrlInfo::getType() const
{	return (CtrlInfo::Type) (_infobuf.mem()->type);}

inline long CtrlInfo::getPrecision() const
{
	return (getType() == Numeric) ? _infobuf.mem()->value.analog.prec : 0;
}

inline const char *CtrlInfo::getUnits() const
{
    if (getType() == Numeric)
        return _infobuf.mem()->value.analog.units;
    return "";
}

inline float CtrlInfo::getDisplayHigh() const
{	return _infobuf.mem()->value.analog.disp_high; }

inline float CtrlInfo::getDisplayLow() const
{	return _infobuf.mem()->value.analog.disp_low; }

inline float CtrlInfo::getHighAlarm() const
{	return _infobuf.mem()->value.analog.high_alarm; }

inline float CtrlInfo::getHighWarning() const
{	return _infobuf.mem()->value.analog.high_warn; }

inline float CtrlInfo::getLowWarning() const
{	return _infobuf.mem()->value.analog.low_warn; }

inline float CtrlInfo::getLowAlarm() const
{	return _infobuf.mem()->value.analog.low_alarm; }

inline size_t CtrlInfo::getNumStates() const
{
	if (getType() == Enumerated)
		return _infobuf.mem()->value.index.num_states;
	return 0;
}

#endif


