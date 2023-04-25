#pragma once

#include "ArduinoToolkit/Core.h"

namespace AT
{

    enum class PinState : int8_t
    {
        Unknown = -1,
        Low = 0,
        High = 1
    };

    class BasicInterrupt
    {
    public:
        BasicInterrupt(const uint8_t pin,
                       const uint8_t mode,
                       const bool reverseLogic = false,
                       const uint32_t periodicCallToISRms = s_DEFAULT_PERIODIC_CALL_ISR_MS);
        ~BasicInterrupt();

        inline uint8_t getPin() const { return m_pin; }
        inline uint8_t getMode() const { return m_mode; }
        inline PinState getState() const { return m_state; }

        PinState receiveInterrupt(const TickType_t xTicksToWait = portMAX_DELAY) const;
        PinState receiveInterruptDiscardIntermediate(const TickType_t xTicksToWait = portMAX_DELAY) const;
        PinState receiveLastInterrupt(const TickType_t xTicksToWait = portMAX_DELAY) const;

    public:
        static bool waitUntilAnyInterrupt(const TickType_t xTicksToWait = portMAX_DELAY);

    public:
        static constexpr uint32_t s_DEFAULT_PERIODIC_CALL_ISR_MS{100};

    private:
        static void IRAM_ATTR intISR(void *const voidPtrInt);
        static void timerNoActivityCallback(const TimerHandle_t xTimer);

    private:
        const uint8_t m_pin;
        const uint8_t m_mode;
        const bool m_reverseLogic;
        PinState m_state{PinState::Unknown};
        SemaphoreHandle_t m_interruptCountingSepmaphore{nullptr};
        TimerHandle_t m_periodicCallToISRtimer{nullptr};

    private:
        static SemaphoreHandle_t s_interruptCountingSepmaphore;
    };

} // namespace AT
