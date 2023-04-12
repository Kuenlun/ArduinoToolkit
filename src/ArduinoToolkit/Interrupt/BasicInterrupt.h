#pragma once

#include "ArduinoToolkit/Core.h"

namespace AT
{

    enum struct LogicState : uint8_t
    {
        low = 0,
        high = 1,
        undefined
    };

    const char *LogicStateToStr(const LogicState state);

    enum struct Interrupt : uint8_t
    {
        noInterrupt = 0b00,
        falling = 0b01,
        rising = 0b10
    };

    const char *InterruptToStr(const Interrupt interrupt);

} // namespace AT
