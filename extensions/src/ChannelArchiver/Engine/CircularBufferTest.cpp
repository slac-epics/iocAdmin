// Tools
#include <UnitTest.h>
// Engine
#include "CircularBuffer.h"

TEST_CASE test_circular_buffer()
{
    DbrType type = DBR_TIME_DOUBLE;
    DbrCount count = 1;
    CircularBuffer buffer;
    long N, i;

    // Tabula Rasa
    TEST(buffer.getCapacity() == 0);
    
    // Allocated but empty
    buffer.allocate(type, count, 8);
    puts("--- Empty");
    TEST(buffer.getCapacity() == 8);
    TEST(buffer.getCount() == 0);

    RawValueAutoPtr data(RawValue::allocate(type, count, 1));
    RawValue::setStatus(data, 0, 0);

    N = 2;
    printf("--- Adding %d values\n", (int)N);
    for (i=0; i<N; ++i)
    {
        RawValue::setTime(data, epicsTime::getCurrent());
        data->value = 3.14 + i;
        buffer.addRawValue(data);
    }
    TEST(buffer.getCapacity() == 8);
    TEST((int)buffer.getCount() == N);

    printf("--- Removing all values\n");
    const RawValue::Data *p;
    while ((p=buffer.removeRawValue()) != 0)
        RawValue::show(stdout, type, count, p);

    TEST(buffer.getCapacity() == 8);
    TEST(buffer.getCount() == 0);
    
    N = 20;
    printf("--- Adding %d values\n", (int)N);
    for (i=0; i<N; ++i)
    {
        RawValue::setTime(data, epicsTime::getCurrent());
        data->value = i;
        buffer.addRawValue(data);
    }
    printf("--- Overwrites: %zu\n", buffer.getOverwrites());
    TEST(buffer.getOverwrites() == N-buffer.getCapacity());
    puts("--- Peeking the values out");
    for (i=0; i<(long)buffer.getCount(); ++i)
        RawValue::show(stdout, type, count, buffer.getRawValue(i));
    TEST(buffer.getCapacity() == 8);
    TEST(buffer.getCount() == 8);
    
    puts("--- Removing the values");
    while ((p=buffer.removeRawValue()) != 0)
        RawValue::show(stdout, type, count, p);
    TEST(buffer.getCapacity() == 8);
    TEST(buffer.getCount() == 0);

    // Buffer remembers overwrites. 
    TEST(buffer.getOverwrites() == N-buffer.getCapacity());
    buffer.reset();
    TEST(buffer.getOverwrites() == 0);


    TEST_OK;
}

