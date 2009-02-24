//stdString.h

class stdString
{
public:
	typedef size_t size_type;

	/// Create/delete string
	stdString();
	stdString(const stdString &rhs);
	stdString(const char *s);
	virtual ~stdString();

	/// Length information
	size_type length() const;

	/// Get C-type data which is always != NULL
	const char *c_str() const;


	/// Assignments
	///
	/// (assign (0, 0) implemented as means of "clear()")
	stdString & assign(const char *s, size_type len);

	/// Concatenations
	/// (prefer reserve() && += to + for performance)
	stdString & append(const char *s, size_type len);

	/// Comparisons
	///
	/// compare <  0: this <  rhs<br>
	/// compare >  0: this >  rhs<br>
	/// compare == 0: this == rhs
	int compare(const stdString &rhs) const;

	/// Reserve space for string of given max. length.
	///
	/// Call in advance to make assignments and concatenations
	/// more effective
	bool reserve(size_type len);

	/// Get position [0 .. length()-1] of first/last ch
	///
	/// Retuns npos if not found
	size_type find(char ch) const; 
	size_type find(const stdString &s) const; 
	size_type find(const char *) const; 
	size_type find_last_of(char ch) const; 

	static const size_type npos; 

	/// Extract sub-string from position <I>from</I>, up to n elements
	stdString substr(size_type from = 0, size_type n = npos) const;
};

//RTree
class FileAllocator;
// Beware: Check this on 64bit systems!
typedef unsigned int FileOffset;

class RTree
{ 
public:
    RTree(FileAllocator &fa, FileOffset anchor);

    bool insertDatablock(const epicsTime &start, const epicsTime &end,
                         FileOffset data_offset,
                         const stdString &data_filename);
  
    void makeDot(const char *filename);
  
    bool selfTest(unsigned long &nodes, unsigned long &records);
};

//Storage/IndexFile.h

class IndexFile 
{
public:
    IndexFile(int RTreeM);

    /// The hash table size used for new channel name tables.
    static uint32_t ht_size;
    
    void open(const stdString &filename, bool readonly=true);

    void close();
    
    class RTree *addChannel(const stdString &channel, stdString &directory);

    class RTree *getTree(const stdString &channel, stdString &directory);

    void showStats(FILE *f);   

    bool check(int level);
};

