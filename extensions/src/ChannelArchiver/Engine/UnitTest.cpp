// UnitTest Suite,
// created by makeUnitTestMain.pl.
// Do NOT modify!

// System
#include <stdio.h>
#include <string.h>
// Tools
#include <UnitTest.h>

// Unit CircularBufferTest:
extern TEST_CASE test_circular_buffer();
// Unit DisableFilterTest:
extern TEST_CASE test_disable_filter();
// Unit EngineConfigTest:
extern TEST_CASE engine_config();
// Unit HTMLPageTest:
extern TEST_CASE test_html_page();
// Unit HTTPServerTest:
extern TEST_CASE test_http_server();
// Unit ProcessVariableFilterTest:
extern TEST_CASE test_pv_filter();
// Unit ProcessVariableTest:
extern TEST_CASE process_variable();
extern TEST_CASE pv_lock_test();
// Unit RepeatFilterTest:
extern TEST_CASE test_repeat_filter();
// Unit SampleMechanismGetTest:
extern TEST_CASE test_sample_mechanism_get();
// Unit SampleMechanismMonitoredGetTest:
extern TEST_CASE test_sample_mechanism_monitored_get();
// Unit SampleMechanismMonitoredTest:
extern TEST_CASE test_sample_mechanism_monitored();
// Unit SampleMechanismTest:
extern TEST_CASE test_sample_mechanism();
// Unit ScanListTest:
extern TEST_CASE test_scan_list();
// Unit TimeFilterTest:
extern TEST_CASE test_time_filter();
// Unit TimeSlotFilterTest:
extern TEST_CASE test_time_slot_filter();

int main(int argc, const char *argv[])
{
    size_t units = 0, run = 0, passed = 0;
    const char *single_unit = 0;
    const char *single_case = 0;

    if (argc > 3  || (argc > 1 && argv[1][0]=='-'))
    {
        printf("USAGE: UnitTest { Unit { case } }\n");
        printf("\n");
        printf("Per default, all test cases in all units are executed.\n");
        printf("\n");
        return -1;
    }
    if (argc >= 2)
        single_unit = argv[1];
    if (argc == 3)
        single_case = argv[2];

    if (single_unit==0  ||  strcmp(single_unit, "CircularBufferTest")==0)
    {
        printf("======================================================================\n");
        printf("Unit CircularBufferTest:\n");
        printf("----------------------------------------------------------------------\n");
        ++units;
       if (single_case==0  ||  strcmp(single_case, "test_circular_buffer")==0)
       {
            ++run;
            printf("\ntest_circular_buffer:\n");
            if (test_circular_buffer())
                ++passed;
            else
                printf("THERE WERE ERRORS!\n");
       }
    }
    if (single_unit==0  ||  strcmp(single_unit, "DisableFilterTest")==0)
    {
        printf("======================================================================\n");
        printf("Unit DisableFilterTest:\n");
        printf("----------------------------------------------------------------------\n");
        ++units;
       if (single_case==0  ||  strcmp(single_case, "test_disable_filter")==0)
       {
            ++run;
            printf("\ntest_disable_filter:\n");
            if (test_disable_filter())
                ++passed;
            else
                printf("THERE WERE ERRORS!\n");
       }
    }
    if (single_unit==0  ||  strcmp(single_unit, "EngineConfigTest")==0)
    {
        printf("======================================================================\n");
        printf("Unit EngineConfigTest:\n");
        printf("----------------------------------------------------------------------\n");
        ++units;
       if (single_case==0  ||  strcmp(single_case, "engine_config")==0)
       {
            ++run;
            printf("\nengine_config:\n");
            if (engine_config())
                ++passed;
            else
                printf("THERE WERE ERRORS!\n");
       }
    }
    if (single_unit==0  ||  strcmp(single_unit, "HTMLPageTest")==0)
    {
        printf("======================================================================\n");
        printf("Unit HTMLPageTest:\n");
        printf("----------------------------------------------------------------------\n");
        ++units;
       if (single_case==0  ||  strcmp(single_case, "test_html_page")==0)
       {
            ++run;
            printf("\ntest_html_page:\n");
            if (test_html_page())
                ++passed;
            else
                printf("THERE WERE ERRORS!\n");
       }
    }
    if (single_unit==0  ||  strcmp(single_unit, "HTTPServerTest")==0)
    {
        printf("======================================================================\n");
        printf("Unit HTTPServerTest:\n");
        printf("----------------------------------------------------------------------\n");
        ++units;
       if (single_case==0  ||  strcmp(single_case, "test_http_server")==0)
       {
            ++run;
            printf("\ntest_http_server:\n");
            if (test_http_server())
                ++passed;
            else
                printf("THERE WERE ERRORS!\n");
       }
    }
    if (single_unit==0  ||  strcmp(single_unit, "ProcessVariableFilterTest")==0)
    {
        printf("======================================================================\n");
        printf("Unit ProcessVariableFilterTest:\n");
        printf("----------------------------------------------------------------------\n");
        ++units;
       if (single_case==0  ||  strcmp(single_case, "test_pv_filter")==0)
       {
            ++run;
            printf("\ntest_pv_filter:\n");
            if (test_pv_filter())
                ++passed;
            else
                printf("THERE WERE ERRORS!\n");
       }
    }
    if (single_unit==0  ||  strcmp(single_unit, "ProcessVariableTest")==0)
    {
        printf("======================================================================\n");
        printf("Unit ProcessVariableTest:\n");
        printf("----------------------------------------------------------------------\n");
        ++units;
       if (single_case==0  ||  strcmp(single_case, "process_variable")==0)
       {
            ++run;
            printf("\nprocess_variable:\n");
            if (process_variable())
                ++passed;
            else
                printf("THERE WERE ERRORS!\n");
       }
       if (single_case==0  ||  strcmp(single_case, "pv_lock_test")==0)
       {
            ++run;
            printf("\npv_lock_test:\n");
            if (pv_lock_test())
                ++passed;
            else
                printf("THERE WERE ERRORS!\n");
       }
    }
    if (single_unit==0  ||  strcmp(single_unit, "RepeatFilterTest")==0)
    {
        printf("======================================================================\n");
        printf("Unit RepeatFilterTest:\n");
        printf("----------------------------------------------------------------------\n");
        ++units;
       if (single_case==0  ||  strcmp(single_case, "test_repeat_filter")==0)
       {
            ++run;
            printf("\ntest_repeat_filter:\n");
            if (test_repeat_filter())
                ++passed;
            else
                printf("THERE WERE ERRORS!\n");
       }
    }
    if (single_unit==0  ||  strcmp(single_unit, "SampleMechanismGetTest")==0)
    {
        printf("======================================================================\n");
        printf("Unit SampleMechanismGetTest:\n");
        printf("----------------------------------------------------------------------\n");
        ++units;
       if (single_case==0  ||  strcmp(single_case, "test_sample_mechanism_get")==0)
       {
            ++run;
            printf("\ntest_sample_mechanism_get:\n");
            if (test_sample_mechanism_get())
                ++passed;
            else
                printf("THERE WERE ERRORS!\n");
       }
    }
    if (single_unit==0  ||  strcmp(single_unit, "SampleMechanismMonitoredGetTest")==0)
    {
        printf("======================================================================\n");
        printf("Unit SampleMechanismMonitoredGetTest:\n");
        printf("----------------------------------------------------------------------\n");
        ++units;
       if (single_case==0  ||  strcmp(single_case, "test_sample_mechanism_monitored_get")==0)
       {
            ++run;
            printf("\ntest_sample_mechanism_monitored_get:\n");
            if (test_sample_mechanism_monitored_get())
                ++passed;
            else
                printf("THERE WERE ERRORS!\n");
       }
    }
    if (single_unit==0  ||  strcmp(single_unit, "SampleMechanismMonitoredTest")==0)
    {
        printf("======================================================================\n");
        printf("Unit SampleMechanismMonitoredTest:\n");
        printf("----------------------------------------------------------------------\n");
        ++units;
       if (single_case==0  ||  strcmp(single_case, "test_sample_mechanism_monitored")==0)
       {
            ++run;
            printf("\ntest_sample_mechanism_monitored:\n");
            if (test_sample_mechanism_monitored())
                ++passed;
            else
                printf("THERE WERE ERRORS!\n");
       }
    }
    if (single_unit==0  ||  strcmp(single_unit, "SampleMechanismTest")==0)
    {
        printf("======================================================================\n");
        printf("Unit SampleMechanismTest:\n");
        printf("----------------------------------------------------------------------\n");
        ++units;
       if (single_case==0  ||  strcmp(single_case, "test_sample_mechanism")==0)
       {
            ++run;
            printf("\ntest_sample_mechanism:\n");
            if (test_sample_mechanism())
                ++passed;
            else
                printf("THERE WERE ERRORS!\n");
       }
    }
    if (single_unit==0  ||  strcmp(single_unit, "ScanListTest")==0)
    {
        printf("======================================================================\n");
        printf("Unit ScanListTest:\n");
        printf("----------------------------------------------------------------------\n");
        ++units;
       if (single_case==0  ||  strcmp(single_case, "test_scan_list")==0)
       {
            ++run;
            printf("\ntest_scan_list:\n");
            if (test_scan_list())
                ++passed;
            else
                printf("THERE WERE ERRORS!\n");
       }
    }
    if (single_unit==0  ||  strcmp(single_unit, "TimeFilterTest")==0)
    {
        printf("======================================================================\n");
        printf("Unit TimeFilterTest:\n");
        printf("----------------------------------------------------------------------\n");
        ++units;
       if (single_case==0  ||  strcmp(single_case, "test_time_filter")==0)
       {
            ++run;
            printf("\ntest_time_filter:\n");
            if (test_time_filter())
                ++passed;
            else
                printf("THERE WERE ERRORS!\n");
       }
    }
    if (single_unit==0  ||  strcmp(single_unit, "TimeSlotFilterTest")==0)
    {
        printf("======================================================================\n");
        printf("Unit TimeSlotFilterTest:\n");
        printf("----------------------------------------------------------------------\n");
        ++units;
       if (single_case==0  ||  strcmp(single_case, "test_time_slot_filter")==0)
       {
            ++run;
            printf("\ntest_time_slot_filter:\n");
            if (test_time_slot_filter())
                ++passed;
            else
                printf("THERE WERE ERRORS!\n");
       }
    }

    printf("======================================================================\n");
    size_t failed = run - passed;
    printf("Tested %zu unit%s, ran %zu test%s, %zu passed, %zu failed.\n",
           units,
           (units > 1 ? "s" : ""),
           run,
           (run > 1 ? "s" : ""),
           passed, failed);
    printf("Success rate: %.1f%%\n", 100.0*passed/run);

    printf("==================================================\n");
    printf("--------------------------------------------------\n");
    if (failed != 0)
    {
        printf("THERE WERE ERRORS!\n");
        return -1;
    }
    printf("All is OK\n");
    printf("--------------------------------------------------\n");
    printf("==================================================\n");

    return 0;
}

