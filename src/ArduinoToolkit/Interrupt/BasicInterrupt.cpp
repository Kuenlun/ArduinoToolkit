#include "ArduinoToolkit/Interrupt/BasicInterrupt.h"

namespace AT
{

    // Static class members
    SemaphoreHandle_t BasicInterrupt::s_interruptCountingSepmaphore{nullptr};

    // Interrupt service routine (ISR) function
    void IRAM_ATTR BasicInterrupt::intISR(void *const voidPtrInt)
    {
        BasicInterrupt *const &intPtr{static_cast<BasicInterrupt *>(voidPtrInt)};
        // Read the current state of the sensor pin
        const bool rawPinValue{digitalRead(intPtr->m_pin)};
        // Invert the logic if needed (Logic XOR between rawPinValue and intPtr->m_reverseLogic)
        const PinState newState{static_cast<PinState>(rawPinValue != intPtr->m_reverseLogic)};
        BaseType_t xHigherPriorityTaskWoken{pdFALSE};
        // Start the PeriodicCallToISR timer the first time the ISR is called
        static bool firstInterrupt{true};
        if (firstInterrupt)
        {
            if (xTimerResetFromISR(intPtr->m_periodicCallToISRtimer, &xHigherPriorityTaskWoken))
                firstInterrupt = false;
        }
        // Check if the sensor state has changed
        if (newState != intPtr->m_state)
        {
            // Update the current state of the pin
            intPtr->m_state = newState;
            // Increment the object interrupt semaphore counter
            xSemaphoreGiveFromISR(intPtr->m_interruptCountingSepmaphore, &xHigherPriorityTaskWoken);
            // Increment the class interrupt semaphore counter
            xSemaphoreGiveFromISR(s_interruptCountingSepmaphore, &xHigherPriorityTaskWoken);
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
        // Create a semaphore to count the number of interrupts that happens on this object
        m_interruptCountingSepmaphore = xSemaphoreCreateCounting(-1, 0);
        ASSERT(m_interruptCountingSepmaphore);
        // Create a semaphore to count the number of interrupts that happens for all objects
        if (!s_interruptCountingSepmaphore)
        {
            s_interruptCountingSepmaphore = xSemaphoreCreateCounting(-1, 0);
            ASSERT(s_interruptCountingSepmaphore);
        }
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

    /**
     * @brief Receive an interrupt from the ISR using a semaphore.
     * This function blocks until an interrupt is received or the specified timeout elapses.
     * It then determines the state of the past interrupts by examining the semaphore counter
     * and the current interrupt state.
     *
     * @param xTicksToWait The maximum time to wait for an interrupt.
     * @return PinState::Low if the interrupt state is low, PinState::High if the interrupt state is high,
     *         or PinState::Unknown if no interrupt was received within the specified timeout.
     */
    PinState BasicInterrupt::receiveInterrupt(const TickType_t xTicksToWait) const
    {
        // Block until an interrupt arrives from the ISR
        if (xSemaphoreTake(m_interruptCountingSepmaphore, xTicksToWait))
        {
            // Take also the class semaphore
            xSemaphoreTake(s_interruptCountingSepmaphore, portMAX_DELAY);
            // Disable interrupts so m_state or the semaphore count is not updated
            // from the ISR if an interrupt happens here.
            portENTER_CRITICAL(&spinlock);
            const UBaseType_t interruptsLeft{uxSemaphoreGetCount(m_interruptCountingSepmaphore)};
            const bool interruptState{static_cast<bool>(m_state)};
            portEXIT_CRITICAL(&spinlock);
            // Depending on the semaphore counter (interrupts that remain to be processed)
            // and the current state of the interrupt, determine which state the past interrupts
            // correspond to (by doing an XOR operation).
            return ((interruptsLeft % 2) != interruptState) ? PinState::High : PinState::Low;
        }
        return PinState::Unknown;
    }

    PinState BasicInterrupt::receiveInterruptDiscardIntermediate(const TickType_t xTicksToWait) const
    {
        if (xSemaphoreTake(m_interruptCountingSepmaphore, xTicksToWait))
        {
            // Take also the class semaphore
            xSemaphoreTake(s_interruptCountingSepmaphore, portMAX_DELAY);
            // Disable interrupts so m_state or the semaphore count is not updated
            // from the ISR if an interrupt happens here.
            portENTER_CRITICAL(&spinlock);
            UBaseType_t interruptsLeft{uxSemaphoreGetCount(m_interruptCountingSepmaphore)};
            const bool interruptState{static_cast<bool>(m_state)};
            portEXIT_CRITICAL(&spinlock);
            // Discard the intermediate interrupts
            const UBaseType_t aux{interruptsLeft % 2};
            for (; interruptsLeft > aux; interruptsLeft--)
            {
                xSemaphoreTake(m_interruptCountingSepmaphore, portMAX_DELAY);
                // Take also the class semaphore
                xSemaphoreTake(s_interruptCountingSepmaphore, portMAX_DELAY);
            }
            // Depending on the semaphore counter (interrupts that remain to be processed)
            // and the current state of the interrupt, determine which state the past interrupts
            // correspond to (by doing an XOR operation).
            return (static_cast<bool>(aux) != interruptState) ? PinState::High : PinState::Low;
        }
        return PinState::Unknown;
    }

    /**
     * @brief Receive the last pending interrupt from the ISR using a semaphore.
     * This function blocks until an interrupt is received or the specified timeout elapses.
     * It then processes all remaining pending interrupts in the semaphore, effectively
     * resetting the semaphore count to 0, and returns the state of the last interrupt.
     *
     * @param xTicksToWait The maximum time to wait for an interrupt.
     * @return The state of the last received interrupt (PinState::Low or PinState::High)
     *         if any interrupt was received within the specified timeout,
     *         or PinState::Unknown if no interrupt was received.
     */
    PinState BasicInterrupt::receiveLastInterrupt(const TickType_t xTicksToWait) const
    {
        if (xSemaphoreTake(m_interruptCountingSepmaphore, xTicksToWait))
        {
            // Take also the class semaphore
            xSemaphoreTake(s_interruptCountingSepmaphore, portMAX_DELAY);
            while (uxSemaphoreGetCount(m_interruptCountingSepmaphore))
            {
                xSemaphoreTake(m_interruptCountingSepmaphore, portMAX_DELAY);
                // Take also the class semaphore
                xSemaphoreTake(s_interruptCountingSepmaphore, portMAX_DELAY);
            }
            return m_state;
        }
        return PinState::Unknown;
    }

} // namespace AT