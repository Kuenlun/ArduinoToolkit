#pragma once

#include <vector>

#include <Arduino.h>

namespace AT
{

    // IDs for the timers
    static constexpr uint8_t TIMER_LOW_TO_HIGH_ID = 0;
    static constexpr uint8_t TIMER_HIGH_TO_LOW_ID = 1;

    /**
     * Declaration of global variables shared among
     * all templates of LowpassInterrupt class
     */
    // FreeRTOS mutex to protect Arduino "attachInterrupt" function
    extern SemaphoreHandle_t mutexCreateInterrupt;
    // FreeRTOS queue to send the filtered interrupts (outputs)
    extern QueueHandle_t queueLowpassInterrupts;
    // Vector to save the pins used for LowpassInterrupt
    extern std::vector<uint8_t> interruptPinsUsed;

    template <uint8_t t_pin>
    class LowpassInterrupt
    {
    public:
        // Constructor
        LowpassInterrupt(const uint8_t mode,
                         const TickType_t lowToHighTimeoutTicks, // Must be >0 ticks
                         const TickType_t highToLowTimeoutTicks, // Must be >0 ticks
                         const TickType_t noEventTimeoutTicks)
            : m_mode(mode),
              m_lowToHighTimeoutTicks(lowToHighTimeoutTicks),
              m_highToLowTimeoutTicks(highToLowTimeoutTicks),
              m_noEventTimeoutTicks(noEventTimeoutTicks)
        {
            // Check the pin has already been used
            if (std::find(interruptPinsUsed.begin(), interruptPinsUsed.end(), t_pin) != interruptPinsUsed.end())
            {
                log_e("Pin %d has already been used");
                ESP_ERROR_CHECK(ESP_FAIL);
            }
            // Add the interrupt pin used to the vector
            interruptPinsUsed.push_back(t_pin);

            /*
            // Create the task only the first time
            if (interruptPinsUsed.empty())
            {
                xTaskCreatePinnedToCore(
                    LowpassInterruptTask,
                    "LowpassInterruptTask",
                    3 * 1024,
                    nullptr,
                    1,
                    nullptr,
                    ARDUINO_RUNNING_CORE);
            }
            */
            // Create the mutex if it is not initialized yet
            if (!mutexCreateInterrupt)
            {
                mutexCreateInterrupt = xSemaphoreCreateMutex();
                if (!mutexCreateInterrupt)
                    log_e("Could not create mutex");
            }

            // Create the queue to send the filtered interrupts (outputs)
            if (!queueLowpassInterrupts)
            {
                queueLowpassInterrupts = xQueueCreate(10, sizeof(bool));
                if (!queueLowpassInterrupts)
                    log_e("Could not create the queue");
            }

            // Create the timers
            s_timerLowToHigh = xTimerCreate(
                "s_timerLowToHigh",
                lowToHighTimeoutTicks,
                pdFALSE,
                (void *)TIMER_LOW_TO_HIGH_ID,
                timerFSMcallback);
            if (!s_timerLowToHigh)
                log_e("Could not create timer");

            s_timerHighToLow = xTimerCreate(
                "s_timerHighToLow",
                highToLowTimeoutTicks,
                pdFALSE,
                (void *)TIMER_HIGH_TO_LOW_ID,
                timerFSMcallback);
            if (!s_timerHighToLow)
                log_e("Could not create timer");
        }

        // Delete copy constructor
        LowpassInterrupt(LowpassInterrupt &other) = delete;

        // Delete assign operator
        void operator=(const LowpassInterrupt &) = delete;

        void begin() const
        {
            // Initialize the pin
            pinMode(t_pin, m_mode);
            // Configure the interrupt
            // Must be protected with a mutex
            xSemaphoreTake(mutexCreateInterrupt, portMAX_DELAY);
            attachInterrupt(t_pin, isrFunc, CHANGE);
            xSemaphoreGive(mutexCreateInterrupt);
        }

        inline static bool getState() { return s_FSMstate; }

    private:
        static void IRAM_ATTR isrFunc()
        {
            // Read the pin
            const uint8_t value = digitalRead(t_pin);

            BaseType_t xHigherPriorityTaskWoken = pdFALSE;
            if (s_FSMstate)
            {
                // Current FSM state is HIGH
                if (value)
                {
                    isr_log_v("LowpassInterrupt (Pin %u): State remained HIGH", t_pin);
                    xTimerStopFromISR(s_timerHighToLow, &xHigherPriorityTaskWoken);
                }
                else
                {
                    if (!xTimerIsTimerActive(s_timerHighToLow))
                    {
                        isr_log_v("LowpassInterrupt (Pin %u): State will change to LOW, starting timer", t_pin);
                        xTimerStartFromISR(s_timerHighToLow, &xHigherPriorityTaskWoken);
                    }
                }
            }
            else
            {
                // Current FSM state is LOW
                if (value)
                {
                    if (!xTimerIsTimerActive(s_timerLowToHigh))
                    {
                        isr_log_v("LowpassInterrupt (Pin %u): State will change to HIGH, starting timer", t_pin);
                        xTimerStartFromISR(s_timerLowToHigh, &xHigherPriorityTaskWoken);
                    }
                }
                else
                {
                    isr_log_v("LowpassInterrupt (Pin %u): State remained LOW", t_pin);
                    xTimerStopFromISR(s_timerLowToHigh, &xHigherPriorityTaskWoken);
                }
            }
            // Did this action unblock a higher priority task?
            if (xHigherPriorityTaskWoken)
                portYIELD_FROM_ISR();
        }

        static void IRAM_ATTR timerFSMcallback(const TimerHandle_t xTimer)
        {
            // Change the FSM state depending on which timer expired
            switch ((uint32_t)pvTimerGetTimerID(xTimer))
            {
            case TIMER_LOW_TO_HIGH_ID:
                s_FSMstate = HIGH;
                isr_log_d("LowpassInterrupt (Pin %u): State changed to HIGH", t_pin);
                break;
            case TIMER_HIGH_TO_LOW_ID:
                s_FSMstate = LOW;
                isr_log_d("LowpassInterrupt (Pin %u): State changed to LOW", t_pin);
                break;
            default:
                break;
            }

            BaseType_t xHigherPriorityTaskWoken = pdFALSE;
            // Send the filtered interrupt to the queue
            xQueueSendFromISR(queueLowpassInterrupts, &s_FSMstate, &xHigherPriorityTaskWoken);
            // Did this action unblock a higher priority task?
            if (xHigherPriorityTaskWoken)
                portYIELD_FROM_ISR();
        }

    private:
        const uint8_t m_mode;
        const TickType_t m_lowToHighTimeoutTicks;
        const TickType_t m_highToLowTimeoutTicks;
        const TickType_t m_noEventTimeoutTicks;

    private:
        // Current state of the finite state machine
        static bool s_FSMstate;
        // FSM timers
        static TimerHandle_t s_timerHighToLow;
        static TimerHandle_t s_timerLowToHigh;

    }; // class LowpassInterrupt

    /**
     * Definition of static variables from LowpassInterrupt class
     */
    // Current state of the finite state machine
    template <uint8_t t_pin>
    bool LowpassInterrupt<t_pin>::s_FSMstate = false;
    // FSM timers
    template <uint8_t t_pin>
    TimerHandle_t LowpassInterrupt<t_pin>::s_timerHighToLow = nullptr;
    template <uint8_t t_pin>
    TimerHandle_t LowpassInterrupt<t_pin>::s_timerLowToHigh = nullptr;

} // namespace AT
