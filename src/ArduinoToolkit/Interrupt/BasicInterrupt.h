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
                       const bool reverseLogic = false);
        ~BasicInterrupt();

        inline uint8_t getPin() const { return m_pin; }
        inline uint8_t getMode() const { return m_mode; }
        inline PinState getState() const { return m_state; }

        PinState receiveInterrupt(const TickType_t xTicksToWait = portMAX_DELAY) const;

    private:
        static void deferredInterruptTask(void *const parameters);
        static void IRAM_ATTR intISR(void *const voidPtrInt);

    private:
        const uint8_t m_pin;
        const uint8_t m_mode;
        const bool m_reverseLogic;
        PinState m_state{PinState::Unknown};

    private:
        static UBaseType_t s_taskPriority;
        static TaskHandle_t s_deferredInterruptTaskHandle;
        static size_t s_numInterruptsUsed;
        static QueueHandle_t s_interruptQueue;
        static constexpr UBaseType_t s_INTERRUPT_QUEUE_LENGTH{100};
    };

} // namespace AT
