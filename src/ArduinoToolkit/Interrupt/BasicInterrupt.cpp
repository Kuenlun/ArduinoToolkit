#include "ArduinoToolkit/Interrupt/BasicInterrupt.h"

namespace AT
{

    static_assert(sizeof(BasicInterrupt) <= sizeof(uint32_t));

    // Deferred interrupt handler function
    void BasicInterrupt::deferredInterrupt(void *const pvParameter1, uint32_t ulParameter2)
    {
        const BasicInterrupt *const &intPtr{(BasicInterrupt*)&ulParameter2};
        // Log the current state of the sensor pin
        if (intPtr->m_state)
            AT_LOG_I("Pin %u: Separado", intPtr->m_pin);
        else
            AT_LOG_I("Pin %u: Junto", intPtr->m_pin);
        BaseType_t xHigherPriorityTaskWoken{pdFALSE};
        // Did this action unblock a higher priority task?
        if (xHigherPriorityTaskWoken)
            portYIELD_FROM_ISR();
    }

    // Interrupt service routine (ISR) function
    void IRAM_ATTR BasicInterrupt::intISR(void *const voidPtrInt)
    {
        BasicInterrupt *const &intPtr{(BasicInterrupt *)voidPtrInt};
        // Read the current state of the sensor pin
        const bool newPinState{digitalRead(intPtr->m_pin)};
        // Check if the sensor state has changed
        BaseType_t xHigherPriorityTaskWoken{pdFALSE};
        if (newPinState != intPtr->m_state)
        {
            intPtr->m_state = newPinState;
            // Call the deferred interrupt handler function
            xTimerPendFunctionCallFromISR(deferredInterrupt, nullptr, *(uint32_t*)intPtr, &xHigherPriorityTaskWoken);
        }
        // Did this action unblock a higher priority task?
        if (xHigherPriorityTaskWoken)
            portYIELD_FROM_ISR();
    }

    BasicInterrupt::BasicInterrupt(const uint8_t pin, const uint8_t mode)
        : m_pin(pin), m_mode(mode)
    {
        // Set up the pin mode and attach the interrupt
        pinMode(m_pin, m_mode);
        attachInterruptArg(m_pin, intISR, (void *)this, CHANGE);
        AT_LOG_I("BasicInterrupt attached on pin %u", m_pin);
    }

    BasicInterrupt::~BasicInterrupt()
    {
        detachInterrupt(m_pin);
        AT_LOG_I("BasicInterrupt detached on pin %u", m_pin);
    }

} // namespace AT