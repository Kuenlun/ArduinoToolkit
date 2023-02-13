#include "Interrupt.h"

namespace AT
{

    const char *LogicStateToStr(const LogicState state)
    {
        switch (state)
        {
        case LogicState::low:
            return "LOW";
        case LogicState::high:
            return "HIGH";
        default:
            return "UNDEFINED";
        }
    }

    const char *InterruptToStr(const Interrupt interrupt)
    {
        switch (interrupt)
        {
        case Interrupt::falling:
            return "FALLING";
        case Interrupt::rising:
            return "RISING";
        default:
            return "NONE";
        }
    }

} // namespace AT