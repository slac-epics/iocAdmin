#include "BinChannelIterator.h"
#include "ArchiveException.h"
#include "DataFile.h"

void BinChannelIterator::init ()
{
    _archive = 0;
    _regex = 0;
    _dir.getChannel()->setChannelIterator (this);
}

BinChannelIterator::BinChannelIterator ()
{
    init ();
}

BinChannelIterator & BinChannelIterator::operator = (const BinChannelIterator &rhs)
{
    _archive = rhs._archive;
    _dir = rhs._dir;
    _dir.getChannel()->setChannelIterator (this);
    delete _regex;
    if (rhs._regex)
        _regex = new RegularExpression(*rhs._regex);
    else
        _regex = 0;
    return *this;
}

void BinChannelIterator::attach (BinArchive *arch, const DirectoryFileIterator &dir,
                                 const char *regular_expression)
{
    clear ();
    _archive = arch;
    _dir = dir;
    _dir.getChannel()->setChannelIterator (this);
    // Set pattern and move on to first matching channel name:
    if (regular_expression)
        _regex = new RegularExpression(regular_expression);
    while (_dir.isValid()  &&  _regex  &&
           !_regex->doesMatch (_dir.getChannel()->getName()))
        _dir.next ();
}

void BinChannelIterator::clear ()
{
    delete _regex;
    _regex = 0;
    _archive = 0;
    _dir.getChannel()->setChannelIterator (0);
}

BinChannelIterator::~BinChannelIterator ()
{
    clear ();
}

bool BinChannelIterator::isValid () const
{
    return _dir.isValid();
}

ChannelI *BinChannelIterator::getChannel ()
{
    return _dir.getChannel ();
}

bool BinChannelIterator::next ()
{
    do
        _dir.next ();
    while (_dir.isValid() && _regex &&
           !_regex->doesMatch (_dir.getChannel()->getName()));

    return isValid();
}





