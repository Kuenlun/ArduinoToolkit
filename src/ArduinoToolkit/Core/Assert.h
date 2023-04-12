#pragma once

#include "ArduinoToolkit/Core/Base.h"

#ifdef DEBUG
// Macro that disables scheduler and enters an infinite loop if the given condition 'x' is not met
#define ASSERT(x)          \
    if (!(x))              \
    {                      \
        vTaskSuspendAll(); \
        while (true)       \
            ;              \
    }
#else
#define ASSERT(x)
#endif
