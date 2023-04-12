#pragma once

#ifdef AT_DEBUG
#define AT_PRINT_TASK_INFO(xTask)                                    \
    log_d("Task: %s", pcTaskGetName(xTask));                         \
    log_d("\tPriority:\t%u", uxTaskPriorityGet(xTask));              \
    log_d("\tRunning Core:\t%d", xPortGetCoreID());                  \
    log_d("\tFree Memory:\t%u", uxTaskGetStackHighWaterMark(xTask)); \
    log_d("\tRunning Tasks:\t%u\n", uxTaskGetNumberOfTasks())
#else
#define AT_PRINT_TASK_INFO(xTask)
#endif
