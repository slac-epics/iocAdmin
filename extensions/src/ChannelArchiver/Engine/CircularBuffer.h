// -*- c++ -*-
#ifndef __CIRCULARBUFFER_H__
#define __CIRCULARBUFFER_H__

// Storage
#include <RawValue.h>

/** \ingroup Engine
 *  In-Memory buffer for values.
 *  <p>
 *  Each SampleMechanism has one to buffer the incoming values
 *  until they are written to the disk.
 */
class CircularBuffer
{
public:
    CircularBuffer();
 
    /** Allocate buffer for num*(type,count) values. */
    void allocate(DbrType type, DbrCount count, size_t num);

    /** @return Returns the data type. */
    DbrType getDbrType() const
    { return type; }
   
    /** @return Returns the data size. */
    DbrCount getDbrCount() const
    { return count; }
    
    /** @return Returns the capacity.
     *  <p>
     *  That is: max. number of elements.
     */
    size_t getCapacity() const
    {   return max_index-1; }

    /** Number of values in the buffer, 0...(capacity-1) */
    size_t getCount() const;

    /** Keep memory as is, but reset to have 0 entries */
    void reset();
    
    /** Number of samples that had to be dropped */
    size_t getOverwrites() const
    {   return overwrites; }

    /** Advance pointer to the next element and return it.
     * 
     *  This allows you to fiddle with that element yourself,
     *  otherwise see addRawValue.
     */
    RawValue::Data *getNextElement();
    
    /** Copy a raw value into the buffer */
    void addRawValue(const RawValue::Data *raw_value);

    /** Get a pointer to value number i without removing it.
     * 
     * @return Returns 0 if i is invalid.
     */
    const RawValue::Data *getRawValue(size_t i) const;
    
    /** Return pointer to the oldest value in the buffer and remove it.
     * 
     * @return Returns 0 of there's nothing more to remove.
     */
    const RawValue::Data *removeRawValue();

    void dump() const;
    
private:
    CircularBuffer(const CircularBuffer &); // not impl.
    CircularBuffer & operator = (const CircularBuffer &); // not impl.

    RawValue::Data    *getElement(size_t i) const;

    DbrType            type;        // dbr_time_xx
    DbrCount           count;       // array size of type
    MemoryBuffer<char> buffer;     // the buffer
    size_t             element_size;// == RawValue::getSize (_type, _count)
    size_t             max_index;   // max. number of elements in buffer
    size_t             head;        // index of current element
    size_t             tail;        // before(!) last element, _tail==_head: empty
    size_t             overwrites;  // # of elements we had to drop
};

#endif //__CIRCULARBUFFER_H__

