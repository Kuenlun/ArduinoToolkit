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
        xTimerStart(m_periodicCallToISRtimer, portMAX_DELAY);
        AT_LOG_I("BasicInterrupt enabled on pin %u", m_pin);
    }

    BasicInterrupt::~BasicInterrupt()
    {
        // Dettach the interrupt from the pin
        detachInterrupt(m_pin);
        // Delete the interrupt counting semaphore
        vSemaphoreDelete(m_interruptCountingSepmaphore);
        AT_LOG_I("BasicInterrupt disabled on pin %u", m_pin);
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

    /* HEADER
    private:
        static void deferredInterruptTask(void *const parameters);

    private:
        static UBaseType_t s_taskPriority;
        static TaskHandle_t s_deferredInterruptTaskHandle;
        static size_t s_numInterruptsUsed;
        static constexpr UBaseType_t s_INTERRUPT_QUEUE_LENGTH{100};
    */
    /* STATIC CLASS MEMBERS
    // Static class members
    UBaseType_t BasicInterrupt::s_taskPriority{2};
    TaskHandle_t BasicInterrupt::s_deferredInterruptTaskHandle{nullptr};
    size_t BasicInterrupt::s_numInterruptsUsed{0};
    */
    /* CONSTRUCTOR
        // Check if the "deferredInterruptTask" needs to be created
        if (!s_numInterruptsUsed)
        {
            // Create the queue to send the interrupts (common to all interrupt pins)
            s_interruptQueue = xQueueCreate(s_INTERRUPT_QUEUE_LENGTH, sizeof(BasicInterrupt));
            ASSERT(s_interruptQueue);
            // Set up the deferredInterruptTask
            const BaseType_t ret{xTaskCreatePinnedToCore(deferredInterruptTask,
                                                         "deferredInterruptTask",
                                                         3 * 1024,
                                                         nullptr,
                                                         s_taskPriority,
                                                         &s_deferredInterruptTaskHandle,
                                                         ARDUINO_RUNNING_CORE)};
            ASSERT(ret);
            AT_LOG_V("BasicInterrupt deferred interrupt task created");
        }
        // Log some info from the task
        PRINT_TASK_INFO(s_deferredInterruptTaskHandle);
        // Increase the interrupts used counter
        s_numInterruptsUsed++;
    */
    /* DESTRUCTOR
         // Decrease the interrupts used counter
         s_numInterruptsUsed--;
         if (!s_numInterruptsUsed)
         {
             vTaskDelete(s_deferredInterruptTaskHandle);
             AT_LOG_V("BasicInterrupt deferred interrupt task deleted");
         }
    */
    /* TASK
    // Deferred interrupt handler function
    void BasicInterrupt::deferredInterruptTask(void *const parameters)
    {
        uint8_t rawBuffer[sizeof(BasicInterrupt)];
        BasicInterrupt *const &intPtr{(BasicInterrupt *)rawBuffer};

        while (true)
        {
            // Wait for an interrupt to happen
            xQueueReceive(s_interruptQueue, intPtr, portMAX_DELAY);
            // Log the current state of the sensor pin
            switch (intPtr->m_state)
            {
            case PinState::High:
                AT_LOG_D("Pin %u: Separado", intPtr->m_pin);
                break;
            case PinState::Low:
                AT_LOG_D("Pin %u: Junto", intPtr->m_pin);
                break;
            default:
                break;
            }
        }
    }
    */

} // namespace AT