#include "ArduinoToolkit/Interrupt/FilteredInterrupt.h"

namespace AT
{

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
        AT_LOG_D("FilteredInterrupt constructed");
    }

    FilteredInterrupt::~FilteredInterrupt()
    {
        AT_LOG_D("FilteredInterrupt destructed");
    }

} // namespace AT

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