// HashTable.h
//////////////////////////////////////////////////////////////////////

#if !defined(_HASHTABLE_H_)
#define _HASHTABLE_H_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class HashTable  
{
public:
	enum
	{
		HashTableSize = 256
	};

	typedef unsigned short HashValue;
	static HashValue Hash (const char *string);
};

#endif
