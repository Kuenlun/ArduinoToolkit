#include <vector>

#include <Arduino.h>

#include "LowpassInterrupt.h"

namespace AT
{

    /**
     * Definition of global variables shared among
     * all templates of LowpassInterrupt class
     */
    // FreeRTOS mutex to protect Arduino "attachInterrupt" function
    SemaphoreHandle_t mutexCreateInterrupt = nullptr;
    // FreeRTOS queue to send unfiltered interrupts from the ISR to the interrupt task
    QueueHandle_t queueInterrupts = nullptr;
    // Vector to save the pins used for LowpassInterrupt
    std::vector<Interrupt *> interruptPinsUsed;
    SemaphoreHandle_t mutexProtectVector = nullptr;

    // FreeRTOS task to handle the interrupts
    void LowpassInterruptTask(void *const parameters)
    {
        while (true)
        {
            static isrToIntTaskQueueElement queueElement;
            if (xQueueReceive(queueInterrupts, &queueElement, portMAX_DELAY))
            {
                xSemaphoreTake(mutexProtectVector, portMAX_DELAY);
                log_w("%d", interruptPinsUsed.size());
                // Get the object that is related to the interrupt
                Interrupt *interrupt = nullptr;
                for (auto i : interruptPinsUsed)
                {
                    log_w("%d", i->getPin());
                    if (i->getPin() == queueElement.pin)
                    {
                        interrupt = i;
                        break;
                    }
                }
                xSemaphoreGive(mutexProtectVector);
                if (!interrupt)
                    ESP_ERROR_CHECK(ESP_ERR_INVALID_STATE);

                log_w("Pin %d got Interrupt %d", interrupt->getPin(), queueElement.state);

                if (interrupt->getState())
                {
                    // Current FSM state is HIGH
                    if (queueElement.state)
                    {
                        log_v("LowpassInterrupt (Pin %u): State remained HIGH", interrupt->getPin());
                        xTimerStop(interrupt->m_timerHighToLow, portMAX_DELAY);
                    }
                    else
                    {
                        if (!xTimerIsTimerActive(interrupt->m_timerHighToLow))
                        {
                            log_v("LowpassInterrupt (Pin %u): State will change to LOW, starting timer", interrupt->getPin());
                            xTimerStart(interrupt->m_timerHighToLow, portMAX_DELAY);
                        }
                    }
                }
                else
                {
                    // Current FSM state is LOW
                    if (queueElement.state)
                    {
                        if (!xTimerIsTimerActive(interrupt->m_timerLowToHigh))
                        {
                            log_v("LowpassInterrupt (Pin %u): State will change to HIGH, starting timer", interrupt->getPin());
                            xTimerStart(interrupt->m_timerLowToHigh, portMAX_DELAY);
                        }
                    }
                    else
                    {
                        log_v("LowpassInterrupt (Pin %u): State remained LOW", interrupt->getPin());
                        xTimerStop(interrupt->m_timerLowToHigh, portMAX_DELAY);
                    }
                }
            }
        }
    }

} // namespace AT
