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
        BasicInterrupt(const uint8_t pin, const uint8_t mode);
        ~BasicInterrupt();

        inline uint8_t getPin() const { return m_pin; }
        inline uint8_t getMode() const { return m_mode; }
        inline PinState getState() const { return m_state; }

    private:
        static void deferredInterrupt(void *const pvParameter1, uint32_t ulParameter2);
        static void IRAM_ATTR intISR(void *const voidPtrInt);

    private:
        const uint8_t m_pin;
        const uint8_t m_mode;
        PinState m_state{PinState::Unknown};
    };

} // namespace AT
