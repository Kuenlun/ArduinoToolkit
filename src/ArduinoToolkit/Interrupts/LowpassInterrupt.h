#pragma once

#include <vector>

#include <Arduino.h>

#include "Interrupt.h"

#define DELAY_FISRST_STATE_CHANGE

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
            if (!(lowToHighTimeoutTicks && highToLowTimeoutTicks))
            {
                log_e("Timeout for changing FSM states must be grater than 0 ticks");
                ESP_ERROR_CHECK(ESP_ERR_INVALID_ARG);
            }
            // Initialize static variables
            s_mode = mode;
            s_lowToHighTimeoutTicks = lowToHighTimeoutTicks;
            s_highToLowTimeoutTicks = highToLowTimeoutTicks;
            s_noEventTimeoutTicks = noEventTimeoutTicks;
            // Add the interrupt pin used to the vector
            interruptPinsUsed.push_back(t_pin);

            // Create the queue to send filtered interrupts (outputs)
            s_queueLowpassInterrupts = xQueueCreate(LOWPASS_INTERRUPT_QUEUE_SIZE, sizeof(LogicState));
            if (!s_queueLowpassInterrupts)
                log_e("Could not create the queue");
            // Create the binary semaphore to protect timers
            s_binarySemaphoreProtectTimerActive = xSemaphoreCreateBinary();
            if (!s_binarySemaphoreProtectTimerActive)
                log_e("Could not create the binary semaphore");
            xSemaphoreGive(s_binarySemaphoreProtectTimerActive);

            // Create the timer to change states
            s_timerStateChanger = xTimerCreate(
                "s_timerStateChanger",
                portMAX_DELAY,
                pdFALSE,
                (void *)0,
                timerStateChangerCallback);
            if (!s_timerStateChanger)
                log_e("Could not create timer");
            // Create the no activity timer
            s_timerNoActivity = xTimerCreate(
                "s_timerNoActivity",
                s_noEventTimeoutTicks,
                pdTRUE,
                (void *)0,
                timerNoActivityCallback);
            if (!s_timerNoActivity)
                log_e("Could not create timer");
            xTimerStart(s_timerNoActivity, portMAX_DELAY);

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
        static IRAM_ATTR Interrupt debouncer(const LogicState newState)
        {
            static LogicState preState{LogicState::undefined};
            if (newState != preState)
            {
                preState = newState;
                switch (newState)
                {
                case LogicState::low:
                    return Interrupt::falling;
                case LogicState::high:
                    return Interrupt::rising;
                }
            }
            return Interrupt::noInterrupt;
        }

        static IRAM_ATTR BaseType_t processInterrupt(const Interrupt rawInterrupt)
        {
            BaseType_t xHigherPriorityTaskWoken{pdFALSE};

            switch (s_FSMstate)
            {
            case LogicState::low:
            {
                switch (rawInterrupt)
                {
                case Interrupt::falling:
                {
                    if (!uxSemaphoreGetCount(s_binarySemaphoreProtectTimerActive))
                    {
                        xTimerStopFromISR(s_timerStateChanger, &xHigherPriorityTaskWoken);
                        xSemaphoreGiveFromISR(s_binarySemaphoreProtectTimerActive, &xHigherPriorityTaskWoken);
                        isr_log_v("Got FALLING when state is LOW -> Timer stopped");
                    }
                    else
                    {
                        isr_log_e("(IMPOSSIBLE) Got FALLING when state is LOW -> Timer was not started");
                    }
                    break;
                }
                case Interrupt::rising:
                {
                    if (uxSemaphoreGetCount(s_binarySemaphoreProtectTimerActive))
                    {
                        xSemaphoreTakeFromISR(s_binarySemaphoreProtectTimerActive, &xHigherPriorityTaskWoken);
                        xTimerChangePeriodFromISR(s_timerStateChanger, s_lowToHighTimeoutTicks, &xHigherPriorityTaskWoken);
                        xTimerStartFromISR(s_timerStateChanger, &xHigherPriorityTaskWoken);
                        isr_log_v("Got RISING when state is LOW -> Timer started");
                    }
                    else
                    {
                        isr_log_e("(IMPOSSIBLE) Got RISING when state is LOW -> Timer already started");
                    }
                    break;
                }
                }
                break;
            }
            case LogicState::high:
            {
                switch (rawInterrupt)
                {
                case Interrupt::falling:
                {
                    if (uxSemaphoreGetCount(s_binarySemaphoreProtectTimerActive))
                    {
                        xSemaphoreTakeFromISR(s_binarySemaphoreProtectTimerActive, &xHigherPriorityTaskWoken);
                        xTimerChangePeriodFromISR(s_timerStateChanger, s_highToLowTimeoutTicks, &xHigherPriorityTaskWoken);
                        xTimerStartFromISR(s_timerStateChanger, &xHigherPriorityTaskWoken);
                        isr_log_v("Got FALLING when state is HIGH -> Timer started");
                    }
                    else
                    {
                        isr_log_e("(IMPOSSIBLE) Got FALLING when state is HIGH -> Timer already started");
                    }
                    break;
                }
                case Interrupt::rising:
                {
                    if (!uxSemaphoreGetCount(s_binarySemaphoreProtectTimerActive))
                    {
                        xTimerStopFromISR(s_timerStateChanger, &xHigherPriorityTaskWoken);
                        xSemaphoreGiveFromISR(s_binarySemaphoreProtectTimerActive, &xHigherPriorityTaskWoken);
                        isr_log_v("Got RISING when state is HIGH -> Timer stopped");
                    }
                    else
                    {
                        isr_log_e("(IMPOSSIBLE) Got RISING when state is HIGH -> Timer was not started");
                    }
                    break;
                }
                }
                break;
            }
            default:
            {
                switch (rawInterrupt)
                {
                case Interrupt::falling:
                {
#ifdef DELAY_FISRST_STATE_CHANGE
                    if (uxSemaphoreGetCount(s_binarySemaphoreProtectTimerActive))
                    {
                        xSemaphoreTakeFromISR(s_binarySemaphoreProtectTimerActive, &xHigherPriorityTaskWoken);
                        xTimerChangePeriodFromISR(s_timerStateChanger, s_highToLowTimeoutTicks, &xHigherPriorityTaskWoken);
                        xTimerStartFromISR(s_timerStateChanger, &xHigherPriorityTaskWoken);
                        s_auxFirstState = LOW;
                        isr_log_v("Got FALLING for the fist time -> Timer started");
                    }
                    else
                    {
                        isr_log_e("(IMPOSSIBLE) Got FALLING for the fist time -> Timer already started");
                    }
#else
                    s_auxFirstState = LOW;
                    timerStateChangerCallback(s_timerStateChanger);
#endif
                    break;
                }
                case Interrupt::rising:
                {
#ifdef DELAY_FISRST_STATE_CHANGE
                    if (uxSemaphoreGetCount(s_binarySemaphoreProtectTimerActive))
                    {
                        xSemaphoreTakeFromISR(s_binarySemaphoreProtectTimerActive, &xHigherPriorityTaskWoken);
                        xTimerChangePeriodFromISR(s_timerStateChanger, s_lowToHighTimeoutTicks, &xHigherPriorityTaskWoken);
                        xTimerStartFromISR(s_timerStateChanger, &xHigherPriorityTaskWoken);
                        s_auxFirstState = HIGH;
                        isr_log_v("Got RISING for the fist time -> Timer started");
                    }
                    else
                    {
                        isr_log_e("(IMPOSSIBLE) Got RISING for the fist time -> Timer already started");
                    }
#else
                    s_auxFirstState = HIGH;
                    timerStateChangerCallback(s_timerStateChanger);
#endif
                    break;
                }
                }
                break;
            }
            }
            return xHigherPriorityTaskWoken;
        }

        static IRAM_ATTR void isrFunc()
        {
            // Reset no activity timer
            BaseType_t xHigherPriorityTaskWoken{pdFALSE};
            xTimerResetFromISR(s_timerNoActivity, &xHigherPriorityTaskWoken);
            // Read the kind of interrupt
            const Interrupt rawInterrupt{debouncer((LogicState)digitalRead(t_pin))};
            // Process the rawInterrupt
            if (rawInterrupt != Interrupt::noInterrupt)
                xHigherPriorityTaskWoken = processInterrupt(rawInterrupt);
            else
                isr_log_w("(BOUNCE) Got same state");
            // Did this action unblock a higher priority task?
            if (xHigherPriorityTaskWoken)
                portYIELD_FROM_ISR();
        }

        static void IRAM_ATTR timerNoActivityCallback(const TimerHandle_t xTimer)
        {
            // Call the ISR
            isrFunc();
        }

        static void IRAM_ATTR timerStateChangerCallback(const TimerHandle_t xTimer)
        {

            switch (s_FSMstate)
            {
            case LogicState::high:
                s_FSMstate = LogicState::low;
                break;
            case LogicState::low:
                s_FSMstate = LogicState::high;
                break;
            default:
            {
                switch (s_auxFirstState)
                {
                case LOW:
                    s_FSMstate = LogicState::low;
                    break;
                case HIGH:
                    s_FSMstate = LogicState::high;
                    break;
                }
            }
            break;
            }

            BaseType_t xHigherPriorityTaskWoken{pdFALSE};
            // Notify that the timer is available
            xSemaphoreGiveFromISR(s_binarySemaphoreProtectTimerActive, &xHigherPriorityTaskWoken);
            // Send the filtered interrupt to the queue
            xQueueSendFromISR(s_queueLowpassInterrupts, &s_FSMstate, &xHigherPriorityTaskWoken);
            // Did this action unblock a higher priority task?
            if (xHigherPriorityTaskWoken)
                portYIELD_FROM_ISR();
        }

    private:
        // Current state of the finite state machine
        static LogicState s_FSMstate;
        static uint8_t s_mode;
        static TickType_t s_lowToHighTimeoutTicks;
        static TickType_t s_highToLowTimeoutTicks;
        // Timeout to wait when no events happen
        static TickType_t s_noEventTimeoutTicks;
        // FSM timers
        static TimerHandle_t s_timerStateChanger;
        static TimerHandle_t s_timerNoActivity;
        // Variable to determine the first state change
        static bool s_auxFirstState;
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
    // FSM timers
    template <uint8_t t_pin>
    TimerHandle_t LowpassInterrupt<t_pin>::s_timerStateChanger{nullptr};
    template <uint8_t t_pin>
    TimerHandle_t LowpassInterrupt<t_pin>::s_timerNoActivity{nullptr};
    // Variable to determine the first state change
    template <uint8_t t_pin>
    bool LowpassInterrupt<t_pin>::s_auxFirstState{false};
    // Queues
    template <uint8_t t_pin>
    QueueHandle_t LowpassInterrupt<t_pin>::s_queueLowpassInterrupts{nullptr};
    template <uint8_t t_pin>
    QueueHandle_t LowpassInterrupt<t_pin>::s_binarySemaphoreProtectTimerActive{nullptr};

} // namespace AT
