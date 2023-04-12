#pragma once

#include "ArduinoToolkit/Core/Base.h"

#ifdef DEBUG
#define AT_LOG_V(...) log_v(__VA_ARGS__)
#define AT_LOG_D(...) log_d(__VA_ARGS__)
#define AT_LOG_I(...) log_i(__VA_ARGS__)
#define AT_LOG_W(...) log_w(__VA_ARGS__)
#define AT_LOG_E(...) log_e(__VA_ARGS__)
#define LOG_V(...) log_v(__VA_ARGS__)
#define LOG_D(...) log_d(__VA_ARGS__)
#define LOG_I(...) log_i(__VA_ARGS__)
#define LOG_W(...) log_w(__VA_ARGS__)
#define LOG_E(...) log_e(__VA_ARGS__)
#define PRINT_TASK_INFO(xTask)                                       \
    LOG_D("Task: %s", pcTaskGetName(xTask));                         \
    LOG_D("\tPriority:\t%u", uxTaskPriorityGet(xTask));              \
    LOG_D("\tRunning Core:\t%d", xPortGetCoreID());                  \
    LOG_D("\tFree Memory:\t%u", uxTaskGetStackHighWaterMark(xTask)); \
    LOG_D("\tRunning Tasks:\t%u\n", uxTaskGetNumberOfTasks())
#else
#define AT_LOG_V(...)
#define AT_LOG_D(...)
#define AT_LOG_I(...)
#define AT_LOG_W(...)
#define AT_LOG_E(...)
#define LOG_V(...)
#define LOG_D(...)
#define LOG_I(...)
#define LOG_W(...)
#define LOG_E(...)
#define PRINT_TASK_INFO(xTask)
#endif
