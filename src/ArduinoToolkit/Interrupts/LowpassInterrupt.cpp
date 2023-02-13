#include <Arduino.h>

#include "LowpassInterrupt.h"

namespace AT
{

    /**
     * Definition of global variables shared among
     * all templates of LowpassInterrupt class
     */
    // FreeRTOS mutex to protect Arduino "attachInterrupt" function
    SemaphoreHandle_t mutexCreateInterrupt{nullptr};
    // Counting semaphore to show if any of the LowpassInterrupt template
    // classes have unread filtered interrupts
    SemaphoreHandle_t semaphoreLowpassInterruptToRead{nullptr};
    // Vector to save the pins used for LowpassInterrupt
    std::vector<uint8_t> interruptPinsUsed;

    // Block until lowpass interrupt happens on any pin
    BaseType_t blockUntilLowpassInterrupt(const TickType_t xTicksToWait)
    {
        return xQueuePeek(semaphoreLowpassInterruptToRead, (void *)nullptr, xTicksToWait);
    }

} // namespace AT
