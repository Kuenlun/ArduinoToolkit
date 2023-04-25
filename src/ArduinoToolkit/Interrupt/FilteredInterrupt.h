#pragma once

#include <vector>

#include "ArduinoToolkit/Core.h"
#include "ArduinoToolkit/Interrupt/BasicInterrupt.h"

namespace AT
{

    class FilteredInterrupt : private BasicInterrupt
    {
    public:
        FilteredInterrupt(const uint8_t pin,
                          const uint8_t mode,
                          const uint32_t lowToHighTimeMs,
                          const uint32_t highToLowTimeMs,
                          const bool reverseLogic = false,
                          const uint32_t periodicCallToISRms = BasicInterrupt::s_DEFAULT_PERIODIC_CALL_ISR_MS);
        ~FilteredInterrupt();

        inline uint8_t getPin() const { return BasicInterrupt::getPin(); }
        inline uint8_t getMode() const { return BasicInterrupt::getMode(); }
        inline PinState getState() const { return BasicInterrupt::getState(); }

        PinState receiveInterrupt(const TickType_t xTicksToWait = portMAX_DELAY) const;
        PinState receiveInterruptDiscardIntermediate(const TickType_t xTicksToWait = portMAX_DELAY) const;
        PinState receiveLastInterrupt(const TickType_t xTicksToWait = portMAX_DELAY) const;

    public:
        static bool waitUntilAnyInterrupt(const TickType_t xTicksToWait = portMAX_DELAY);

    private:
        static void filteredStateChangeTimerCallback(const TimerHandle_t xTimer);
        static void deferredInterruptTask(void *const parameters);
        static void processInterrupt(FilteredInterrupt *const intPtr);

    private:
        const uint32_t m_lowToHighTimeMs;
        const uint32_t m_highToLowTimeMs;
        PinState m_state{PinState::Unknown};
        SemaphoreHandle_t m_interruptCountingSepmaphore{nullptr};
        TimerHandle_t m_changeFilteredStateTimer{nullptr};

    private:
        static UBaseType_t s_taskPriority;
        static TaskHandle_t s_deferredInterruptTaskHandle;
        static std::vector<FilteredInterrupt *> s_instances;
        static SemaphoreHandle_t s_interruptCountingSepmaphore;
    };

} // namespace AT
