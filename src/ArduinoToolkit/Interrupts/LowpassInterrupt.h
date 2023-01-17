#pragma once

#include <vector>

#include <Arduino.h>

#include "Interrupt.h"

namespace AT
{

    static constexpr uint8_t LOWPASS_INTERRUPT_QUEUE_SIZE{10};

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
    private:
        // IDs for the timers
        enum struct TimerID : uint8_t
        {
            LowToHigh = 0,
            HighToLow = 1
        };

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

        // Copy constructor
        LowpassInterrupt(const LowpassInterrupt &) = delete;
        // Move constructor
        LowpassInterrupt(LowpassInterrupt &&) = delete;
        // Copy assignment operator
        LowpassInterrupt &operator=(const LowpassInterrupt &) = delete;
        // Move assignment operator
        LowpassInterrupt &operator=(LowpassInterrupt &&) = delete;
        // Destructor
        ~LowpassInterrupt() = default;

        static LogicState receiveLowpassInterrupts(const TickType_t blockTimeTicks = portMAX_DELAY)
        {
            LogicState state;
            xQueueReceive(s_queueLowpassInterrupts, &state, blockTimeTicks);
            return state;
        }

        inline static uint8_t getPin() { return t_pin; }

        inline static LogicState getState() { return s_FSMstate; }

    private:
        static void IRAM_ATTR isrFunc()
        {
            // Read the pin
            const LogicState state{(LogicState)digitalRead(t_pin)};

            BaseType_t xHigherPriorityTaskWoken{pdFALSE};
            xQueueOverwriteFromISR(s_queueRawInterrupts, &state, &xHigherPriorityTaskWoken);
            // Did this action unblock a higher priority task?
            if (xHigherPriorityTaskWoken)
                portYIELD_FROM_ISR();
        }

        static void IRAM_ATTR timerFSMcallback(const TimerHandle_t xTimer)
        {
            // Change the FSM state depending on which timer expired
            switch ((TimerID)(uint32_t)pvTimerGetTimerID(xTimer))
            {
            case TimerID::LowToHigh:
                s_FSMstate = LogicState::high;
                isr_log_i("LowpassInterrupt (Pin %u): LogicState changed to HIGH", t_pin);
                break;
            case TimerID::HighToLow:
                s_FSMstate = LogicState::low;
                isr_log_i("LowpassInterrupt (Pin %u): LogicState changed to LOW", t_pin);
                break;
            }

            BaseType_t xHigherPriorityTaskWoken{pdFALSE};
            // Allow the timers to start
            xSemaphoreGiveFromISR(s_binarySemaphoreProtectTimerActive, &xHigherPriorityTaskWoken);
            // Send the filtered interrupt to the queue
            xQueueSendFromISR(s_queueLowpassInterrupts, &s_FSMstate, &xHigherPriorityTaskWoken);
            // Did this action unblock a higher priority task?
            if (xHigherPriorityTaskWoken)
                portYIELD_FROM_ISR();
        }

        static Interrupt debouncer(const LogicState state)
        {
            static LogicState preState{LogicState::undefined};
            if (state != preState)
            {
                preState = state;
                switch (state)
                {
                case LogicState::low:
                    return Interrupt::falling;
                case LogicState::high:
                    return Interrupt::rising;
                }
            }
            return Interrupt::noInterrupt;
        }

        static void processInterrupt(const Interrupt interrupt)
        {
            switch (s_FSMstate)
            {
            case LogicState::high:
            {
                switch (interrupt)
                {
                case Interrupt::rising:
                    xTimerStop(s_timerHighToLow, portMAX_DELAY);
                    xSemaphoreGive(s_binarySemaphoreProtectTimerActive);
                    log_d("LowpassInterrupt (Pin %u): LogicState remained HIGH", t_pin);
                    break;
                case Interrupt::falling:
                    if (uxSemaphoreGetCount(s_binarySemaphoreProtectTimerActive))
                    {
                        xSemaphoreTake(s_binarySemaphoreProtectTimerActive, portMAX_DELAY);
                        xTimerStart(s_timerHighToLow, portMAX_DELAY);
                        log_d("LowpassInterrupt (Pin %u): LogicState will change to LOW, starting timer", t_pin);
                    }
                    break;
                }
                break;
            }
            case LogicState::low:
            {
                switch (interrupt)
                {
                case Interrupt::rising:
                    if (uxSemaphoreGetCount(s_binarySemaphoreProtectTimerActive))
                    {
                        xSemaphoreTake(s_binarySemaphoreProtectTimerActive, portMAX_DELAY);
                        xTimerStart(s_timerLowToHigh, portMAX_DELAY);
                        log_d("LowpassInterrupt (Pin %u): LogicState will change to HIGH, starting timer", t_pin);
                    }
                    break;
                case Interrupt::falling:
                    xTimerStop(s_timerLowToHigh, portMAX_DELAY);
                    xSemaphoreGive(s_binarySemaphoreProtectTimerActive);
                    log_d("LowpassInterrupt (Pin %u): LogicState remained LOW", t_pin);
                    break;
                }
            }
            }
        }

        static void LowpassInterruptTask(void *const parameters)
        {
            // Create the queue to send the filtered interrupts (outputs)
            s_queueRawInterrupts = xQueueCreate(1, sizeof(LogicState));
            if (!s_queueRawInterrupts)
                log_e("Could not create the queue");
            // Create the queue to send the filtered interrupts (outputs)
            s_queueLowpassInterrupts = xQueueCreate(LOWPASS_INTERRUPT_QUEUE_SIZE, sizeof(LogicState));
            if (!s_queueLowpassInterrupts)
                log_e("Could not create the queue");
            // Create the binary semaphore to protect timers
            s_binarySemaphoreProtectTimerActive = xSemaphoreCreateBinary();
            if (!s_binarySemaphoreProtectTimerActive)
                log_e("Could not create the binary semaphore");
            xSemaphoreGive(s_binarySemaphoreProtectTimerActive);

            // Create the timers
            s_timerLowToHigh = xTimerCreate(
                "s_timerLowToHigh",
                s_lowToHighTimeoutTicks,
                pdFALSE,
                (void *)TimerID::LowToHigh,
                timerFSMcallback);
            if (!s_timerLowToHigh)
                log_e("Could not create timer");

            s_timerHighToLow = xTimerCreate(
                "s_timerHighToLow",
                s_highToLowTimeoutTicks,
                pdFALSE,
                (void *)TimerID::HighToLow,
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
                static LogicState state;
                if (xQueueReceive(s_queueRawInterrupts, &state, s_noEventTimeoutTicks))
                {
                    const Interrupt interrupt{debouncer(state)};
                    if (interrupt != Interrupt::noInterrupt)
                        processInterrupt(interrupt);
                    else
                        log_v("Debouncer (Pin %u): Got same interrupt. Discarting...", t_pin);
                }
                else
                {
                    log_d("LowpassInterrupt (Pin %u): No event timeout, reading pin value", t_pin);
                    isrFunc();
                }
            }
        }

    private:
        // Current state of the finite state machine
        static LogicState s_FSMstate;
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
        // FreeRTOS binary semaphore to protect "xTimerIsTimerActive"
        // Sometimes when a fast interrupt happens, this function tells that
        // a timer is still active when in fact it has been stopped.
        static SemaphoreHandle_t s_binarySemaphoreProtectTimerActive;

    }; // class LowpassInterrupt

    /**
     * Definition of static variables from LowpassInterrupt class
     */
    // Current state of the finite state machine
    template <uint8_t t_pin>
    LogicState LowpassInterrupt<t_pin>::s_FSMstate{LogicState::low};
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
    TaskHandle_t LowpassInterrupt<t_pin>::s_LowpassInterruptTaskHandle{nullptr};
    // FSM timers
    template <uint8_t t_pin>
    TimerHandle_t LowpassInterrupt<t_pin>::s_timerHighToLow{nullptr};
    template <uint8_t t_pin>
    TimerHandle_t LowpassInterrupt<t_pin>::s_timerLowToHigh{nullptr};
    // Queues
    template <uint8_t t_pin>
    QueueHandle_t LowpassInterrupt<t_pin>::s_queueRawInterrupts{nullptr};
    template <uint8_t t_pin>
    QueueHandle_t LowpassInterrupt<t_pin>::s_queueLowpassInterrupts{nullptr};
    template <uint8_t t_pin>
    QueueHandle_t LowpassInterrupt<t_pin>::s_binarySemaphoreProtectTimerActive{nullptr};

} // namespace AT
