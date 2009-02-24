
#ifndef __STORAGE_TYPES_H
#define __STORAGE_TYPES_H

#include <stdint.h>

typedef uint32_t FileOffset;

// used internally for offsets inside files:
const FileOffset INVALID_OFFSET = 0xffffffff;

#endif
