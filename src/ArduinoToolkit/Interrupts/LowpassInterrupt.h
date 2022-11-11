#pragma once

#include <vector>

#include <Arduino.h>

namespace AT
{

    // IDs for the timers
    static constexpr uint8_t TIMER_LOW_TO_HIGH_ID = 0;
    static constexpr uint8_t TIMER_HIGH_TO_LOW_ID = 1;
    static constexpr uint8_t LOWPASS_INTERRUPT_QUEUE_SIZE = 10;

    /**
     * Declaration of global variables shared among
     * all templates of LowpassInterrupt class
     */
    // FreeRTOS mutex to protect Arduino "attachInterrupt" function
    extern SemaphoreHandle_t mutexCreateInterrupt;
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
                         const TickType_t noEventTimeoutTicks = portMAX_DELAY)
        {

            // Check if the pin has already been used
            if (std::find(interruptPinsUsed.begin(), interruptPinsUsed.end(), t_pin) != interruptPinsUsed.end())
            {
                log_e("Pin %d has already been used");
                ESP_ERROR_CHECK(ESP_FAIL);
            }

            // Check that timeouts for changing states are greater than 0 ticks
            if (!lowToHighTimeoutTicks || !highToLowTimeoutTicks)
            {
                log_e("Timeout for changing FSM states must be grater than 0 ticks");
                ESP_ERROR_CHECK(ESP_ERR_INVALID_ARG);
            }

            s_mode = mode;
            s_lowToHighTimeoutTicks = lowToHighTimeoutTicks;
            s_highToLowTimeoutTicks = highToLowTimeoutTicks;
            s_noEventTimeoutTicks = noEventTimeoutTicks;

            // Create the task only the first time
            xTaskCreatePinnedToCore(
                LowpassInterruptTask,
                "LowpassInterruptTask",
                3 * 1024,
                nullptr,
                1,
                &s_LowpassInterruptTaskHandle,
                ARDUINO_RUNNING_CORE);

            // Add the interrupt pin used to the vector
            interruptPinsUsed.push_back(t_pin);
        }

        // Delete copy constructor
        LowpassInterrupt(LowpassInterrupt &other) = delete;

        // Delete assign operator
        void operator=(const LowpassInterrupt &) = delete;

        static bool receiveLowpassInterrupts(const TickType_t blockTimeTicks = portMAX_DELAY)
        {
            bool state;
            xQueueReceive(s_queueLowpassInterrupts, &state, blockTimeTicks);
            return state;
        }

        inline static uint8_t getPin() { return t_pin; }

        inline static bool getState() { return s_FSMstate; }

    private:
        static void IRAM_ATTR isrFunc()
        {
            // Read the pin
            const bool value = digitalRead(t_pin);

            BaseType_t xHigherPriorityTaskWoken = pdFALSE;
            xQueueOverwriteFromISR(s_queueRawInterrupts, &value, &xHigherPriorityTaskWoken);
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
            xQueueSendFromISR(s_queueLowpassInterrupts, &s_FSMstate, &xHigherPriorityTaskWoken);
            // Did this action unblock a higher priority task?
            if (xHigherPriorityTaskWoken)
                portYIELD_FROM_ISR();
        }

        static void LowpassInterruptTask(void *const parameters)
        {
            // Create the queue to send the filtered interrupts (outputs)
            s_queueRawInterrupts = xQueueCreate(1, sizeof(bool));
            if (!s_queueRawInterrupts)
                log_e("Could not create the queue");
            // Create the queue to send the filtered interrupts (outputs)
            s_queueLowpassInterrupts = xQueueCreate(LOWPASS_INTERRUPT_QUEUE_SIZE, sizeof(bool));
            if (!s_queueLowpassInterrupts)
                log_e("Could not create the queue");

            // Create the timers
            s_timerLowToHigh = xTimerCreate(
                "s_timerLowToHigh",
                s_lowToHighTimeoutTicks,
                pdFALSE,
                (void *)TIMER_LOW_TO_HIGH_ID,
                timerFSMcallback);
            if (!s_timerLowToHigh)
                log_e("Could not create timer");

            s_timerHighToLow = xTimerCreate(
                "s_timerHighToLow",
                s_highToLowTimeoutTicks,
                pdFALSE,
                (void *)TIMER_HIGH_TO_LOW_ID,
                timerFSMcallback);
            if (!s_timerHighToLow)
                log_e("Could not create timer");

            // Create the mutex if it is not initialized yet
            if (!mutexCreateInterrupt)
            {
                mutexCreateInterrupt = xSemaphoreCreateMutex();
                if (!mutexCreateInterrupt)
                    log_e("Could not create mutex");
            }

            // Initialize the pin
            pinMode(t_pin, s_mode);
            // Configure the interrupt
            // Must be protected with a mutex
            xSemaphoreTake(mutexCreateInterrupt, portMAX_DELAY);
            attachInterrupt(t_pin, isrFunc, CHANGE);
            xSemaphoreGive(mutexCreateInterrupt);

            while (true)
            {
                static bool value;
                if (xQueueReceive(s_queueRawInterrupts, &value, s_noEventTimeoutTicks))
                {
                    if (s_FSMstate)
                    {
                        // Current FSM state is HIGH
                        if (value)
                        {
                            log_v("LowpassInterrupt (Pin %u): State remained HIGH", t_pin);
                            xTimerStop(s_timerHighToLow, portMAX_DELAY);
                        }
                        else
                        {
                            if (!xTimerIsTimerActive(s_timerHighToLow))
                            {
                                log_v("LowpassInterrupt (Pin %u): State will change to LOW, starting timer", t_pin);
                                xTimerStart(s_timerHighToLow, portMAX_DELAY);
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
                                log_v("LowpassInterrupt (Pin %u): State will change to HIGH, starting timer", t_pin);
                                xTimerStart(s_timerLowToHigh, portMAX_DELAY);
                            }
                        }
                        else
                        {
                            log_v("LowpassInterrupt (Pin %u): State remained LOW", t_pin);
                            xTimerStop(s_timerLowToHigh, portMAX_DELAY);
                        }
                    }
                }
                else
                {
                    log_v("LowpassInterrupt (Pin %u): No event timeout, calling ISR...", t_pin);
                    isrFunc();
                }
            }
        }

    private:
        // Current state of the finite state machine
        static bool s_FSMstate;
        static uint8_t s_mode;
        static TickType_t s_lowToHighTimeoutTicks;
        static TickType_t s_highToLowTimeoutTicks;
        // Timeout to wait when no events happen
        static TickType_t s_noEventTimeoutTicks;
        // FreeRTOS task handle
        static TaskHandle_t s_LowpassInterruptTaskHandle;
        // FSM timers
        static TimerHandle_t s_timerHighToLow;
        static TimerHandle_t s_timerLowToHigh;
        // FreeRTOS queue to send the raw interrupts from the ISR to the task
        static QueueHandle_t s_queueRawInterrupts;
        // FreeRTOS queue to send the filtered interrupts (outputs)
        static QueueHandle_t s_queueLowpassInterrupts;

    }; // class LowpassInterrupt

    /**
     * Definition of static variables from LowpassInterrupt class
     */
    // Current state of the finite state machine
    template <uint8_t t_pin>
    bool LowpassInterrupt<t_pin>::s_FSMstate = false;
    template <uint8_t t_pin>
    uint8_t LowpassInterrupt<t_pin>::s_mode;
    template <uint8_t t_pin>
    TickType_t LowpassInterrupt<t_pin>::s_lowToHighTimeoutTicks;
    template <uint8_t t_pin>
    TickType_t LowpassInterrupt<t_pin>::s_highToLowTimeoutTicks;
    // Timeout to wait when no events happen
    template <uint8_t t_pin>
    TickType_t LowpassInterrupt<t_pin>::s_noEventTimeoutTicks;
    // FreeRTOS task handle
    template <uint8_t t_pin>
    TaskHandle_t LowpassInterrupt<t_pin>::s_LowpassInterruptTaskHandle = nullptr;
    // FSM timers
    template <uint8_t t_pin>
    TimerHandle_t LowpassInterrupt<t_pin>::s_timerHighToLow = nullptr;
    template <uint8_t t_pin>
    TimerHandle_t LowpassInterrupt<t_pin>::s_timerLowToHigh = nullptr;
    // Queues
    template <uint8_t t_pin>
    QueueHandle_t LowpassInterrupt<t_pin>::s_queueRawInterrupts = nullptr;
    template <uint8_t t_pin>
    QueueHandle_t LowpassInterrupt<t_pin>::s_queueLowpassInterrupts = nullptr;

} // namespace AT
