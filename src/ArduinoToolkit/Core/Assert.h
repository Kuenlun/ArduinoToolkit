#pragma once

#ifdef AT_DEBUG
// Macro that disables scheduler and enters an infinite loop if the given condition 'x' is not met
#define AT_ASSERT(x)       \
    if (!(x))              \
    {                      \
        vTaskSuspendAll(); \
        while (true)       \
            ;              \
    }
#else
#define AT_ASSERT(x)
#endif
