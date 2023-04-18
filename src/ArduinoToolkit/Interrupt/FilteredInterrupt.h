#pragma once

#include "ArduinoToolkit/Core.h"
#include "ArduinoToolkit/Interrupt/BasicInterrupt.h"

namespace AT
{

    class FilteredInterrupt : public BasicInterrupt
    {
    public:
        FilteredInterrupt(const uint8_t pin,
                          const uint8_t mode,
                          const uint32_t lowToHighTimeMs,
                          const uint32_t highToLowTimeMs,
                          const bool reverseLogic = false,
                          const uint32_t periodicCallToISRms = BasicInterrupt::s_DEFAULT_PERIODIC_CALL_ISR_MS);
        ~FilteredInterrupt();

    private:
        const uint32_t m_lowToHighTimeMs;
        const uint32_t m_highToLowTimeMs;
        PinState m_filteredState{PinState::Unknown};
    };

} // namespace AT

/* HEADER
private:
    static void deferredInterruptTask(void *const parameters);

private:
    static UBaseType_t s_taskPriority;
    static TaskHandle_t s_deferredInterruptTaskHandle;
    static size_t s_numInterruptsUsed;
    static constexpr UBaseType_t s_INTERRUPT_QUEUE_LENGTH{100};
*/