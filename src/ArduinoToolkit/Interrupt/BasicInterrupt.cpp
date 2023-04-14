#include "ArduinoToolkit/Interrupt/BasicInterrupt.h"

namespace AT
{

    // Deferred interrupt handler function
    void BasicInterrupt::deferredInterrupt(void *const voidPtrInt, uint32_t ulParameter2)
    {
        const BasicInterrupt *const &intPtr{(BasicInterrupt *)voidPtrInt};
        // Log the current state of the sensor pin
        if (intPtr->m_state)
            AT_LOG_I("Pin %u: Separado", intPtr->m_pin);
        else
            AT_LOG_I("Pin %u: Junto", intPtr->m_pin);
        BaseType_t xHigherPriorityTaskWoken{pdFALSE};
        // Decrease the counting semaphore
        xSemaphoreTakeFromISR(intPtr->m_countingSemaphore, &xHigherPriorityTaskWoken);
        // Check if the semaphore count is zero and give the all done semaphore if it is
        if (uxSemaphoreGetCount(intPtr->m_countingSemaphore) == 0)
            xSemaphoreGiveFromISR(intPtr->m_allDoneSemaphore, &xHigherPriorityTaskWoken);
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
            if (xTimerPendFunctionCallFromISR(deferredInterrupt, voidPtrInt, 0, &xHigherPriorityTaskWoken))
            {
                xSemaphoreTakeFromISR(intPtr->m_allDoneSemaphore, &xHigherPriorityTaskWoken);
                xSemaphoreGiveFromISR(intPtr->m_countingSemaphore, &xHigherPriorityTaskWoken);
            }
        }
        // Did this action unblock a higher priority task?
        if (xHigherPriorityTaskWoken)
            portYIELD_FROM_ISR();
    }

    BasicInterrupt::BasicInterrupt(const uint8_t pin, const uint8_t mode)
        : m_pin(pin), m_mode(mode)
    {
        // Create semaphores to protect the destruction
        m_allDoneSemaphore = xSemaphoreCreateBinary();
        ASSERT(m_allDoneSemaphore);
        m_countingSemaphore = xSemaphoreCreateCounting(-1, 0);
        ASSERT(m_countingSemaphore);
        // Set up the pin mode and attach the interrupt
        pinMode(m_pin, m_mode);
        attachInterruptArg(m_pin, intISR, (void *)this, CHANGE);
        AT_LOG_I("BasicInterrupt attached on pin %u", m_pin);
    }

    BasicInterrupt::~BasicInterrupt()
    {
        detachInterrupt(m_pin);
        // Make sure that "deferredInterrupt" has finished processing
        // all interrupts before deleting a "BasicInterrupt" object.
        xSemaphoreTake(m_allDoneSemaphore, portMAX_DELAY);
        AT_LOG_I("BasicInterrupt detached on pin %u", m_pin);
    }

} // namespace AT