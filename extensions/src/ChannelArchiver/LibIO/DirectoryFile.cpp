// DirectoryFile.cpp
//////////////////////////////////////////////////////////////////////

#include "MsgLogger.h"
#include "Filename.h"
#include "Conversions.h"
#include "DirectoryFile.h"
#include "ArchiveException.h"

//#define LOG_DIRFILE

//////////////////////////////////////////////////////////////////////
// DirectoryFile
//////////////////////////////////////////////////////////////////////

// Attach DiskBasedHashTable to disk file of given name.
// a) new file: setup Hash Table
// b) existing file for read-only: check HT
// c) existing file for read-write: check HT
DirectoryFile::DirectoryFile(const stdString &filename, bool for_write)
{
    _filename = filename;
    Filename::getDirname(_filename, _dirname);
    _file_for_write = for_write;
    _file = fopen(filename.c_str(), "r+b");
    if (_file==0 && for_write)
        _file = fopen(filename.c_str(), "w+b");
    if (_file == 0)
        throwDetailedArchiveException(OpenError, filename);

    // Does file contain HT?
    fseek(_file, 0, SEEK_END);
    _next_free_entry = ftell(_file);
    if (_next_free_entry < FirstEntryOffset)
    {
        if (!for_write) // ... but it should
            throwDetailedArchiveException(Invalid, "Missing HT");

        // Initialize HT:
        for (HashTable::HashValue entry = 0;
             entry < HashTable::HashTableSize; ++entry)
            writeHTEntry(entry, INVALID_OFFSET);
    
        // Next free entry = first entry after HT
        _next_free_entry = FirstEntryOffset;
    }
    
    // Check if file size = HT + N full entries
    FileOffset rest = (_next_free_entry - FirstEntryOffset)
        % BinChannel::getDataSize();
    if (rest)
        LOG_MSG("Suspicious directory file %s has a 'tail' of %d Bytes\n",
                filename.c_str(), rest);
    
#ifdef LOG_DIRFILE
    if (_file.isReadonly())
        LOG_MSG("(readonly) ");
    LOG_MSG("DirectoryFile %s\n", _filename);
#endif
}

DirectoryFile::~DirectoryFile()
{
#ifdef LOG_DIRFILE
    if (_file.isReadonly())
        LOG_MSG("(readonly) ");
    LOG_MSG("~DirectoryFile %s\n", _filename);
#endif
    if (_file)
        fclose(_file);
}

DirectoryFileIterator DirectoryFile::findFirst()
{
    DirectoryFileIterator       i(this);
    i.findValidEntry(0);

    return i;
}

// Try to locate entry with given name.
DirectoryFileIterator DirectoryFile::find(const stdString &name)
{
    DirectoryFileIterator i(this);

    i._hash   = HashTable::Hash(name.c_str());
    FileOffset offset = readHTEntry(i._hash);
    while (offset != INVALID_OFFSET)
    {
        i.getChannel()->read(_file, offset);
        if (name == i.getChannel()->getName())
            return i;
        offset = i.getChannel()->getNextEntryOffset();
    }
    i.getChannel()->clear();
    
    return i;
}

// Add a new entry to HT.
// Throws Invalid if that entry exists already.
//
// After calling this routine the current entry
// is undefined. It must be initialized and
// then written with saveEntry ().
DirectoryFileIterator DirectoryFile::add(const stdString &name)
{
    DirectoryFileIterator i(this);
	BinChannel *channel = i.getChannel();
    const char *cname = name.c_str();

    i._hash = HashTable::Hash(cname);
    FileOffset offset = readHTEntry(i._hash);

    if (offset == INVALID_OFFSET) // Empty HT slot:
        writeHTEntry(i._hash, _next_free_entry);
    else
    {       // Follow the entry chain that hashed to this value:
        FileOffset next = offset;
        while (next != INVALID_OFFSET)
        {
            channel->read(_file, next);
            if (name == channel->getName()) // already there?
                return i;
            next = channel->getNextEntryOffset();
        }
        // i._entry: last entry in chain.
        // make that one point to new entry:
        channel->setNextEntryOffset(_next_free_entry);
        channel->write(_file, channel->getOffset());
    }

    // Last entry points now to _next_free_entry.
    // Create the new entry there:
    channel->init(cname);
    channel->setNextEntryOffset(INVALID_OFFSET);
    channel->write(_file, _next_free_entry);
    fflush(_file);
    _next_free_entry += channel->getDataSize();
    
    return i;
}

// Remove name from directory file.
// Will not remove data but only "pointers" to the data!
bool DirectoryFile::remove(const stdString &name)
{
    DirectoryFileIterator i(this);
	BinChannel *channel = i.getChannel();
    HashTable::HashValue hash = HashTable::Hash(name.c_str());
    FileOffset prev=0, offset = readHTEntry(hash);

    // Follow the channel chain that hashes to this value:
    while (offset != INVALID_OFFSET)
    {
        channel->read(_file, offset);
        if (name == channel->getName())
        {
            // unlink this entry from list of names that share 'hash'
            if (prev == 0) // first entry in list?
            {
                // Make hash table point to the next channel,
                // skipping this one
                writeHTEntry(hash, channel->getNextEntryOffset());
                return true;
            }
            else
            {
                // Make previous entry skip this one
                offset = channel->getNextEntryOffset();
                channel->read(_file, prev);
                channel->setNextEntryOffset(offset);
                channel->write(_file, prev);
                return true;
            }
        }
        prev = offset;
        offset = channel->getNextEntryOffset();
    }
    return false;
}

FileOffset DirectoryFile::readHTEntry(HashTable::HashValue entry) const
{
    FileOffset offset;
    FileOffset pos = entry * sizeof(FileOffset);
    
    if (fseek(_file, pos, SEEK_SET) != 0 ||
        (FileOffset) ftell(_file) != pos   ||
        fread(&offset, sizeof(FileOffset), 1, _file) != 1)
        throwArchiveException(ReadError);
    FileOffsetFromDisk(offset);
    return offset;
}

void DirectoryFile::writeHTEntry(HashTable::HashValue entry, FileOffset offset)
{       // offset is value parm -> safe to convert in place
    FileOffsetToDisk (offset);
    FileOffset pos = entry * sizeof(FileOffset);
    if (fseek(_file, pos, SEEK_SET) != 0 ||
        (FileOffset) ftell(_file) != pos   ||
        fwrite(&offset, sizeof(FileOffset), 1, _file) != 1)
        throwArchiveException(WriteError);
}

//////////////////////////////////////////////////////////////////////
// DirectoryFileIterator
//////////////////////////////////////////////////////////////////////

void DirectoryFileIterator::clear()
{
    _dir = 0;
    _hash = HashTable::HashTableSize;
    _entry.clear();
}

DirectoryFileIterator::DirectoryFileIterator()
{
    clear();
}

DirectoryFileIterator::DirectoryFileIterator(DirectoryFile *dir)
{
    clear();
    _dir = dir;
}

DirectoryFileIterator::DirectoryFileIterator(const DirectoryFileIterator &dir)
{
    clear();
    *this = dir;
}

DirectoryFileIterator & DirectoryFileIterator::operator = (const DirectoryFileIterator &rhs)
{
    // Right now, this is actually what the default copy op. would do:
    _dir = rhs._dir;
    _entry = rhs._entry;
    _hash = rhs._hash;
    return *this;
}

bool DirectoryFileIterator::next()
{
    if (_hash >= HashTable::HashTableSize ||
        _entry.getOffset() == INVALID_OFFSET ||
        _dir == 0)
        return false;
    
    // Have a current entry.
    // Ask it for pointer to next entry:
    FileOffset next = _entry.getNextEntryOffset();
    if (next != INVALID_OFFSET)
    {
        _entry.read(_dir->_file, next);
        return isValid();
    }
    // End of entries that relate to current _hash value,
    // switch to next value:
    return findValidEntry(_hash + 1);
}

// Search HT for the first non-empty entry:
bool DirectoryFileIterator::findValidEntry(HashTable::HashValue start)
{
    _entry.clear();
    if (!_dir)
        return false;
    
    // Loop HT from 'start'
    FileOffset tmp;
    for (_hash = start; _hash < HashTable::HashTableSize; ++_hash)
    {
        // Get first entry's position from HT
        tmp = _dir->readHTEntry(_hash);
        // If valid, read that entry
        if (tmp != INVALID_OFFSET)
        {
            _entry.read(_dir->_file, tmp);
            return isValid();
        }
    }
    return false;
}

void DirectoryFileIterator::save()
{
    if (_dir)
        _entry.write(_dir->_file, _entry.getOffset());
}

