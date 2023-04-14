#include "ArduinoToolkit/Interrupt/BasicInterrupt.h"

namespace AT
{

    static void deferredInterrupt(void *const voidPtrInt, uint32_t ulParameter2)
    {
        const BasicInterrupt *const &intPtr{(BasicInterrupt *)voidPtrInt};
        if (intPtr->getState())
            log_i("Pin %u: Separado", intPtr->getPin());
        else
            log_i("Pin %u: Junto", intPtr->getPin());
    }

    static void IRAM_ATTR intISR(void *const voidPtrInt)
    {
        BasicInterrupt *const &intPtr{(BasicInterrupt *)voidPtrInt};
        // Leer el estado actual del pin del sensor
        const bool newPinState{digitalRead(intPtr->getPin())};
        // Verificar si el estado del sensor ha cambiado
        BaseType_t xHigherPriorityTaskWoken{pdFALSE};
        if (newPinState != intPtr->getState())
        {
            intPtr->getState() = newPinState;
            xTimerPendFunctionCallFromISR(deferredInterrupt, voidPtrInt, 0, &xHigherPriorityTaskWoken);
        }
        // Did this action unblock a higher priority task?
        if (xHigherPriorityTaskWoken)
            portYIELD_FROM_ISR();
    }

    BasicInterrupt::BasicInterrupt(const uint8_t pin, const uint8_t mode)
        : m_pin(pin), m_mode(mode)
    {
        pinMode(m_pin, m_mode);
        attachInterruptArg(m_pin, intISR, (void *)this, CHANGE);
        log_i("BasicInterrupt attached on pin %u", m_pin);
    }

    BasicInterrupt::~BasicInterrupt()
    {
        detachInterrupt(m_pin);
        // Make sure "deferredInterrupt" has processed all interrupts
        // before deleting a "BasicInterrupt" object because "this" is passed
        // to "deferredInterrupt". Waiting 100 ms should be enough.
        vTaskDelay(pdMS_TO_TICKS(100));
        log_i("BasicInterrupt detached on pin %u", m_pin);
    }

} // namespace AT