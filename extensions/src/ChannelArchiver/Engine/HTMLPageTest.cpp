// System
#include <stdio.h>
// Tools
#include <AutoPtr.h>
#include <UnitTest.h>
// Engine
#include "HTMLPage.h"

TEST_CASE test_html_page()
{
    // This is mostly about 32 vs. 64 bit.
    // On 64 bit, the original 'colspan = va_arg(size_t)'
    // caused core dumps, because the colspan was
    // really passed as an int.
    TEST(sizeof(int) == 4);
    if (sizeof(size_t) == 4)
    {
        PASS("size_t uses 4 bytes, Looks like 32 bit system");
        // Here's the thing: When passing just a number to
        // a var-arg routine, that's 4 bytes long.
        // Not sure if int or size_t:
        TEST(sizeof(42) == 4);
    }
    else  if (sizeof(size_t) == 8)
    {
        PASS("size_t uses 8 bytes, Looks like 64 bit system");
        // On 64bit OS, that simple number is still passed in 4 bytes,
        // so we must not use "va_arg(size_t)" to fetch it!
        TEST(sizeof(42) == 4);
    }
    else
    {   FAIL("Unknown system"); }

    SOCKET s = fileno(stdout); // Not really a socket... Using stdout
    {
        HTMLPage p(s, "Test Page");
        p.line("<H1>Hello</H1>");
        p.openTable(1, "Column 1", 1, "Columns 2",  2, "Double-wide column", 0);
        p.tableLine("A", "B", "C 1", "C 2", 0);
        p.tableLine("A", "B", "C 1", "C 2", 0);
        p.tableLine("A", "B", "C 1", "C 2", 0);
        p.tableLine("A", "B", "C 1", "C 2", 0);
        p.tableLine("A", "B", "C 1", "C 2", 0);
        p.closeTable();
    }
    PASS("^^^^ There should be some HTML text ^^^^");
    TEST_OK;
}

