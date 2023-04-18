#include "ArduinoToolkit/Interrupt/BasicInterrupt.h"

namespace AT
{

    // Globals
    static portMUX_TYPE spinlock = portMUX_INITIALIZER_UNLOCKED;

    // Interrupt service routine (ISR) function
    void IRAM_ATTR BasicInterrupt::intISR(void *const voidPtrInt)
    {
        BasicInterrupt *const &intPtr{static_cast<BasicInterrupt *>(voidPtrInt)};
        // Read the current state of the sensor pin
        const bool rawPinValue{digitalRead(intPtr->m_pin)};
        // Invert the logic if needed (Logic XOR between rawPinValue and intPtr->m_reverseLogic)
        const PinState newState{static_cast<PinState>(rawPinValue != intPtr->m_reverseLogic)};
        // Check if the sensor state has changed
        BaseType_t xHigherPriorityTaskWoken{pdFALSE};
        if (newState != intPtr->m_state)
        {
            // Update the current state of the pin
            intPtr->m_state = newState;
            // Send the interrupt to the interrupt queue
            xSemaphoreGiveFromISR(intPtr->m_interruptCountingSepmaphore, &xHigherPriorityTaskWoken);
            // Restart PeriodicCallToISR timer (also start it the first time)
            xTimerResetFromISR(intPtr->m_periodicCallToISRtimer, &xHigherPriorityTaskWoken);
        }
        // Did this action unblock a higher priority task?
        if (xHigherPriorityTaskWoken)
            portYIELD_FROM_ISR();
    }

    void BasicInterrupt::timerNoActivityCallback(const TimerHandle_t xTimer)
    {
        intISR(pvTimerGetTimerID(xTimer));
    }

    BasicInterrupt::BasicInterrupt(const uint8_t pin,
                                   const uint8_t mode,
                                   const bool reverseLogic,
                                   const uint32_t periodicCallToISRms)
        : m_pin(pin),
          m_mode(mode),
          m_reverseLogic(reverseLogic)
    {
        // Create a semaphore to count the number of interrupts that happens
        m_interruptCountingSepmaphore = xSemaphoreCreateCounting(-1, 0);
        ASSERT(m_interruptCountingSepmaphore);
        // Set up the pin mode and attach the interrupt
        pinMode(m_pin, m_mode);
        attachInterruptArg(m_pin, intISR, static_cast<void *>(this), CHANGE);
        // Create a timer to call the ISR (to catch missing interrupts)
        m_periodicCallToISRtimer = xTimerCreate("PeriodicCallToISR",
                                                pdMS_TO_TICKS(periodicCallToISRms),
                                                pdTRUE,
                                                static_cast<void *>(this),
                                                timerNoActivityCallback);
        ASSERT(m_periodicCallToISRtimer);
        AT_LOG_I("BasicInterrupt enabled on pin %u", m_pin);
    }

    BasicInterrupt::~BasicInterrupt()
    {
        // Dettach the interrupt from the pin
        detachInterrupt(m_pin);
        // Delete the timer
        xTimerDelete(m_periodicCallToISRtimer, portMAX_DELAY);
        // Delete the interrupt counting semaphore
        vSemaphoreDelete(m_interruptCountingSepmaphore);
        AT_LOG_D("BasicInterrupt disabled on pin %u", m_pin);
    }

    PinState BasicInterrupt::receiveInterrupt(const TickType_t xTicksToWait) const
    {
        // Block until an interrupt arrives from the ISR
        if (xSemaphoreTake(m_interruptCountingSepmaphore, xTicksToWait))
        {
            // Disable interrupts so m_state or the semaphore count is not updated if an interrupt happens here
            portENTER_CRITICAL(&spinlock);
            const UBaseType_t interruptsLeft{uxSemaphoreGetCount(m_interruptCountingSepmaphore)};
            const bool interruptState{static_cast<bool>(m_state)};
            portEXIT_CRITICAL(&spinlock);
            if (!(interruptsLeft % 2) != interruptState)
                return PinState::Low;
            else
                return PinState::High;
        }
        return PinState::Unknown;
    }

} // namespace AT