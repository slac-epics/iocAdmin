// Tools
#include <MsgLogger.h>
#include <epicsTimeHelper.h>
// Engine
#include "CircularBuffer.h"

#undef DEBUG_CIRCBUF

// The Circular buffer implementation:
//
// Initial: tail = head = 0.
//
// valid entries:
// tail+1, tail+2 .... head

CircularBuffer::CircularBuffer()
  : type(0),
    count(0),
    element_size(0),
    max_index(1),
    head(0),
    tail(0),
    overwrites(0)
{}

void CircularBuffer::allocate(DbrType new_type, DbrCount new_count, size_t num)
{
    element_size = RawValue::getSize(new_type, new_count);
    type = new_type;
    count = new_count;
    // Since head == tail indicates empty, we can only
    // hold max_index-1 elements. Inc num by one to account for that:
    max_index = num+1;
    buffer.reserve(element_size * max_index);
    reset();
}

size_t CircularBuffer::getCount() const
{
    size_t count;
    if (head >= tail)
        count = head - tail;
    else    
        //     #(tail .. end)      + #(start .. head)
        count = (max_index - tail - 1) + (head + 1);
    return count;
}

void CircularBuffer::reset()
{
	head = 0;
	tail = 0;
	overwrites = 0;
}

RawValue::Data *CircularBuffer::getNextElement()
{
	// compute the place in the circular queue
	if (++head >= max_index)
		head = 0;
	// here is the over write of the queue
	if (head == tail)
	{
		++overwrites;
		if (++tail >= max_index)
			tail = 0;
	}
	return getElement(head);
}

void CircularBuffer::addRawValue(const RawValue::Data *value)
{
#ifdef DEBUG_CIRCBUF
    printf("CircularBuffer(0x%lX)::add: ", (unsigned long)this);
    RawValue::show(stdout, type, count, value); 
#endif
    if (!isValidTime(RawValue::getTime(value)))
    {
        LOG_MSG("CircularBuffer added bad time stamp!\n");
    }
    memcpy(getNextElement(), value, element_size);
}

const RawValue::Data *CircularBuffer::getRawValue(size_t i) const
{
    if (i<0 || i >= getCount())
        return 0;
    i = (tail + 1 + i) % max_index;
    return getElement(i);
}

const RawValue::Data *CircularBuffer::removeRawValue()
{
    RawValue::Data *val;

    if (tail == head)
        val = 0;
    else
    {
        if (++tail >= max_index)
            tail = 0;

        val = getElement(tail);
    }
    if (val && !isValidTime(RawValue::getTime(val)))
    {
        LOG_MSG("CircularBuffer returns bad time stamp!\n");
    }

#ifdef DEBUG_CIRCBUF
    printf("CircularBuffer(0x%lX)::remove: ", (unsigned long)this);
    RawValue::show(stdout, type, count, val); 
#endif
    return val;
}

void CircularBuffer::dump() const
{
    size_t i, num = getCount();
    printf("CircularBuffer type %d, count %d, capacity %d:\n",
           (int)type, (int)count, (int)getCapacity());
    for (i=0; i<num; ++i)
    {
        printf("#%3u: ", (unsigned int)i);
        RawValue::show(stdout, type, count, getRawValue(i));
    }
}

RawValue::Data *CircularBuffer::getElement(size_t i) const
{
    return (RawValue::Data *) (buffer.mem() + i * element_size);
}

