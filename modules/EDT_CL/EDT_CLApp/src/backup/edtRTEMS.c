#include <rtems.h>

void
osdThreadSetPriorityMax(int pri)
{
rtems_task_priority old;
	rtems_task_set_priority(RTEMS_SELF, 5 - pri, &old);
}

