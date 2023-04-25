#include "ArduinoToolkit/Interrupt/FilteredInterrupt.h"

namespace AT
{

    // Static class members
    UBaseType_t FilteredInterrupt::s_taskPriority{2};
    TaskHandle_t FilteredInterrupt::s_deferredInterruptTaskHandle{nullptr};
    std::vector<FilteredInterrupt *> FilteredInterrupt::s_instances;
    SemaphoreHandle_t FilteredInterrupt::s_interruptCountingSepmaphore{nullptr};

    void FilteredInterrupt::filteredStateChangeTimerCallback(const TimerHandle_t xTimer)
    {
        FilteredInterrupt *const &intPtr{static_cast<FilteredInterrupt *>(pvTimerGetTimerID(xTimer))};
        if (intPtr->m_state == PinState::Low)
        {
            intPtr->m_state = PinState::High;
            AT_LOG_D("Filtered state changed to HIGH");
        }
        else
        {
            intPtr->m_state = PinState::Low;
            AT_LOG_D("Filtered state changed to LOW");
        }
        // Increment the interrupt semaphore counter
        xSemaphoreGive(intPtr->m_interruptCountingSepmaphore);
        // Increment the class interrupt semaphore counter
        xSemaphoreGive(s_interruptCountingSepmaphore);
    }

    void FilteredInterrupt::processInterrupt(FilteredInterrupt *const intPtr)
    {
        const PinState basicInterruptState{intPtr->BasicInterrupt::receiveInterruptDiscardIntermediate(0)};
        // Check if the interrupt happened in this object
        if (basicInterruptState == PinState::Unknown)
            return;
        // Do things depending on the current state
        switch (intPtr->m_state)
        {
        case PinState::Low:
        {
            if (basicInterruptState == PinState::Low)
            {
                AT_LOG_V("Got same state -> Stop timer on pin %u", intPtr->getPin());
                xTimerStop(intPtr->m_changeFilteredStateTimer, portMAX_DELAY);
            }
            else
            {
                AT_LOG_V("Got different state -> Start timer on pin %u", intPtr->getPin());
                xTimerChangePeriod(intPtr->m_changeFilteredStateTimer,
                                   pdMS_TO_TICKS(intPtr->m_lowToHighTimeMs),
                                   portMAX_DELAY);
            }
        }
        break;
        case PinState::High:
        {
            if (basicInterruptState == PinState::Low)
            {
                AT_LOG_V("Got different state -> Start timer on pin %u", intPtr->getPin());
                xTimerChangePeriod(intPtr->m_changeFilteredStateTimer,
                                   pdMS_TO_TICKS(intPtr->m_highToLowTimeMs),
                                   portMAX_DELAY);
            }
            else
            {
                AT_LOG_V("Got same state -> Stop timer on pin %u", intPtr->getPin());
                xTimerStop(intPtr->m_changeFilteredStateTimer, portMAX_DELAY);
            }
        }
        break;
        default:
            // If the current filtered state is "PinState::Unknown" update the state directly
            intPtr->m_state = basicInterruptState;
            if (basicInterruptState == PinState::Low)
                AT_LOG_D("Filtered state changed to LOW on pin %u", intPtr->getPin());
            else
                AT_LOG_D("Filtered state changed to HIGH on pin %u", intPtr->getPin());
            // Increment the interrupt semaphore counter
            xSemaphoreGive(intPtr->m_interruptCountingSepmaphore);
            // Increment the class interrupt semaphore counter
            xSemaphoreGive(s_interruptCountingSepmaphore);
            break;
        }
    }

    // Deferred interrupt handler function
    void FilteredInterrupt::deferredInterruptTask(void *const parameters)
    {
        while (true)
        {
            BasicInterrupt::waitUntilAnyInterrupt();
            for (FilteredInterrupt *const intPtr : s_instances)
                processInterrupt(intPtr);
        }
    }

    FilteredInterrupt::FilteredInterrupt(const uint8_t pin,
                                         const uint8_t mode,
                                         const uint32_t lowToHighTimeMs,
                                         const uint32_t highToLowTimeMs,
                                         const bool reverseLogic,
                                         const uint32_t periodicCallToISRms)
        : BasicInterrupt(pin, mode, reverseLogic, periodicCallToISRms),
          m_lowToHighTimeMs(lowToHighTimeMs),
          m_highToLowTimeMs(highToLowTimeMs)
    {
        // Create a semaphore to count the number of interrupts that happens
        m_interruptCountingSepmaphore = xSemaphoreCreateCounting(-1, 0);
        ASSERT(m_interruptCountingSepmaphore);
        // Create a timer to change the state of the filtered interrupt
        m_changeFilteredStateTimer = xTimerCreate("FilteredStateChangeTimer",
                                                  1, // Will be changed later (any unsigned > 0 is OK)
                                                  pdFALSE,
                                                  static_cast<void *>(this),
                                                  filteredStateChangeTimerCallback);
        ASSERT(m_changeFilteredStateTimer);
        // Check if the "deferredInterruptTask" needs to be created
        if (s_instances.empty())
        {
            // Create a semaphore to count the number of interrupts that happens for all objects
            s_interruptCountingSepmaphore = xSemaphoreCreateCounting(-1, 0);
            ASSERT(s_interruptCountingSepmaphore);
            // Set up the deferredInterruptTask
            const BaseType_t ret{xTaskCreatePinnedToCore(deferredInterruptTask,
                                                         "deferredInterruptTask",
                                                         3 * 1024,
                                                         nullptr,
                                                         s_taskPriority,
                                                         &s_deferredInterruptTaskHandle,
                                                         ARDUINO_RUNNING_CORE)};
            ASSERT(ret);
            AT_LOG_V("FilteredInterrupt deferred task created");
        }
        // Log some info from the task
        PRINT_TASK_INFO(s_deferredInterruptTaskHandle);
        // Add this object to the list of instances
        s_instances.push_back(this);
        AT_LOG_D("FilteredInterrupt constructed");
    }

    FilteredInterrupt::~FilteredInterrupt()
    {
        // Remove this object from the list of instances
        s_instances.erase(std::remove(s_instances.begin(), s_instances.end(), this), s_instances.end());
        // Delete the task if there are no objects left
        if (s_instances.empty())
        {
            vTaskDelete(s_deferredInterruptTaskHandle);
            AT_LOG_V("FilteredInterrupt deferred task deleted");
            vSemaphoreDelete(s_interruptCountingSepmaphore);
        }
        AT_LOG_D("FilteredInterrupt destructed");
    }

    PinState FilteredInterrupt::receiveInterrupt(const TickType_t xTicksToWait) const
    {
        // Block until an interrupt arrives from the ISR
        if (xSemaphoreTake(m_interruptCountingSepmaphore, xTicksToWait))
        {
            // Take also the class semaphore
            xSemaphoreTake(s_interruptCountingSepmaphore, portMAX_DELAY);

            const UBaseType_t interruptsLeft{uxSemaphoreGetCount(m_interruptCountingSepmaphore)};
            const bool interruptState{static_cast<bool>(m_state)};
            // Depending on the semaphore counter (interrupts that remain to be processed)
            // and the current state of the interrupt, determine which state the past interrupts
            // correspond to (by doing an XOR operation).
            return (!(interruptsLeft % 2) != interruptState) ? PinState::Low : PinState::High;
        }
        return PinState::Unknown;
    }

    PinState FilteredInterrupt::receiveInterruptDiscardIntermediate(const TickType_t xTicksToWait) const
    {
        if (xSemaphoreTake(m_interruptCountingSepmaphore, xTicksToWait))
        {
            // Take also the class semaphore
            xSemaphoreTake(s_interruptCountingSepmaphore, portMAX_DELAY);

            UBaseType_t interruptsLeft{uxSemaphoreGetCount(m_interruptCountingSepmaphore)};
            const bool interruptState{static_cast<bool>(m_state)};
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
    PinState FilteredInterrupt::receiveLastInterrupt(const TickType_t xTicksToWait) const
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

    bool FilteredInterrupt::waitUntilAnyInterrupt(const TickType_t xTicksToWait)
    {
        return xQueuePeek(s_interruptCountingSepmaphore, nullptr, xTicksToWait);
    }

} // namespace AT
