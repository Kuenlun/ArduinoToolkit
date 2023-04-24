#pragma once

#include "ArduinoToolkit/Core/Base.h"

#ifdef DEBUG
// Macro that disables scheduler and enters an infinite loop if the given condition 'x' is not met
#define ASSERT(x)                                 \
    if (!(x))                                     \
    {                                             \
        AT_LOG_E("ASSERT not fulfilled: %s", #x); \
    }

// Macro that disables scheduler and enters an infinite loop if the given condition 'x' is not met
#define DEBUG_ASSERT(x)                                 \
    if (!(x))                                           \
    {                                                   \
        AT_LOG_E("DEBUG_ASSERT not fulfilled: %s", #x); \
    }
#else
#define ASSERT(x) // Show warning or error here
#define DEBUG_ASSERT(x)
#endif
