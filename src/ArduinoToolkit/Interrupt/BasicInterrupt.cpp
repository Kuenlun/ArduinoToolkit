#include "ArduinoToolkit/Interrupt/BasicInterrupt.h"

namespace AT
{

    // Static class members
    UBaseType_t BasicInterrupt::s_taskPriority{2};
    TaskHandle_t BasicInterrupt::s_deferredInterruptTaskHandle{nullptr};
    size_t BasicInterrupt::s_numInterruptsUsed{0};
    QueueHandle_t BasicInterrupt::s_interruptQueue{nullptr};

    // Deferred interrupt handler function
    void BasicInterrupt::deferredInterruptTask(void *const parameters)
    {
        uint8_t rawBuffer[sizeof(BasicInterrupt)];
        BasicInterrupt *const &intPtr{(BasicInterrupt *)rawBuffer};

        while (true)
        {
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

    // Interrupt service routine (ISR) function
    void IRAM_ATTR BasicInterrupt::intISR(void *const voidPtrInt)
    {
        BasicInterrupt *const &intPtr{(BasicInterrupt *)voidPtrInt};
        // Read the current state of the sensor pin
        const PinState newState = digitalRead(intPtr->m_pin) ? PinState::High : PinState::Low;
        // Check if the sensor state has changed
        BaseType_t xHigherPriorityTaskWoken{pdFALSE};
        if (newState != intPtr->m_state)
        {
            intPtr->m_state = newState;
            // Send the interrupt to the deferred interrupt task
            xQueueSendToBackFromISR(s_interruptQueue, intPtr, &xHigherPriorityTaskWoken);
        }
        // Did this action unblock a higher priority task?
        if (xHigherPriorityTaskWoken)
            portYIELD_FROM_ISR();
    }

    BasicInterrupt::BasicInterrupt(const uint8_t pin, const uint8_t mode)
        : m_pin(pin), m_mode(mode)
    {
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
        // Set up the pin mode and attach the interrupt
        pinMode(m_pin, m_mode);
        attachInterruptArg(m_pin, intISR, (void *)this, CHANGE);
        // Increase the interrupts used counter
        s_numInterruptsUsed++;
        AT_LOG_I("BasicInterrupt attached on pin %u", m_pin);
    }

    BasicInterrupt::~BasicInterrupt()
    {
        // Dettach the interrupt from the pin
        detachInterrupt(m_pin);
        // Decrease the interrupts used counter
        s_numInterruptsUsed--;
        // Delete the "deferredInterruptTask" if the counter is 0
        if (!s_numInterruptsUsed)
        {
            vTaskDelete(s_deferredInterruptTaskHandle);
            AT_LOG_V("BasicInterrupt deferred interrupt task deleted");
            vQueueDelete(s_interruptQueue);
            AT_LOG_V("BasicInterrupt interrupt queue deleted");
        }
        AT_LOG_I("BasicInterrupt detached from pin %u", m_pin);
    }

} // namespace AT