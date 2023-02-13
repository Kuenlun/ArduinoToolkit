#pragma once

#include <vector>

#include <Arduino.h>

#include "Interrupt.h"

#define DELAY_FISRST_STATE_CHANGE true

namespace AT
{

    static constexpr uint8_t LOWPASS_INTERRUPT_QUEUE_SIZE{10};

    /**
     * Declaration of global variables shared among
     * all templates of LowpassInterrupt class
     */
    // FreeRTOS mutex to protect Arduino "attachInterrupt" function
    extern SemaphoreHandle_t mutexCreateInterrupt;
    // Counting semaphore to show if any of the LowpassInterrupt template
    // classes have unread filtered interrupts
    extern SemaphoreHandle_t semaphoreLowpassInterruptToRead;
    // Vector to save the pins used for LowpassInterrupt
    extern std::vector<uint8_t> interruptPinsUsed;

    // Block until lowpass interrupt happens on any pin
    BaseType_t blockUntilLowpassInterrupt(const TickType_t xTicksToWait = portMAX_DELAY);

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
                log_e("[INT PIN %u] Pin %d has already been used", getPin());
                ESP_ERROR_CHECK(ESP_FAIL);
            }
            // Check that timeouts for changing states are greater than 0 ticks
            if (!(lowToHighTimeoutTicks && highToLowTimeoutTicks))
            {
                log_e("[INT PIN %u] Timeout for changing FSM states must be grater than 0 ticks", getPin());
                ESP_ERROR_CHECK(ESP_ERR_INVALID_ARG);
            }
            // Initialize static variables
            s_mode = mode;
            s_lowToHighTimeoutTicks = lowToHighTimeoutTicks;
            s_highToLowTimeoutTicks = highToLowTimeoutTicks;
            s_noActivityTimeoutTicks = noEventTimeoutTicks;
            // Add the interrupt pin used to the vector
            interruptPinsUsed.push_back(t_pin);

            // Create the queue to send filtered interrupts (outputs)
            s_queueLowpassInterrupts = xQueueCreate(LOWPASS_INTERRUPT_QUEUE_SIZE, sizeof(Interrupt));
            if (!s_queueLowpassInterrupts)
                log_e("[INT PIN %u] Could not create the queue", getPin());
            // Create the binary semaphore to protect timers
            s_binarySemaphoreProtectTimerActive = xSemaphoreCreateBinary();
            if (!s_binarySemaphoreProtectTimerActive)
                log_e("[INT PIN %u] Could not create the binary semaphore", getPin());
            xSemaphoreGive(s_binarySemaphoreProtectTimerActive);

            // Create the timer to change states
            s_timerStateChanger = xTimerCreate(
                "s_timerStateChanger",
                portMAX_DELAY,
                pdFALSE,
                (void *)0,
                timerStateChangerCallback);
            if (!s_timerStateChanger)
                log_e("[INT PIN %u] Could not create timer", getPin());
            // Create the no activity timer
            s_timerNoActivity = xTimerCreate(
                "s_timerNoActivity",
                s_noActivityTimeoutTicks,
                pdTRUE,
                (void *)0,
                timerNoActivityCallback);
            if (!s_timerNoActivity)
                log_e("[INT PIN %u] Could not create timer", getPin());
            xTimerStart(s_timerNoActivity, portMAX_DELAY);

            // Create the counting semaphore if it is not initialized yet
            if (!semaphoreLowpassInterruptToRead)
            {
                // Initialize the counting semaphore to 0 with uxMaxCount set to UBaseType_t max value
                semaphoreLowpassInterruptToRead = xSemaphoreCreateCounting(-1, 0);
                if (!semaphoreLowpassInterruptToRead)
                    log_e("[INT PIN %u] Could not create counting semaphore", getPin());
            }

            // Create the mutex if it is not initialized yet
            if (!mutexCreateInterrupt)
            {
                mutexCreateInterrupt = xSemaphoreCreateMutex();
                if (!mutexCreateInterrupt)
                    log_e("[INT PIN %u] Could not create mutex", getPin());
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

        static Interrupt receiveLowpassInterrupts(const TickType_t blockTimeTicks = portMAX_DELAY)
        {
            Interrupt interrupt;
            xSemaphoreTake(semaphoreLowpassInterruptToRead, blockTimeTicks);
            xQueueReceive(s_queueLowpassInterrupts, &interrupt, blockTimeTicks);
            return interrupt;
        }

        static UBaseType_t getLowpassInterruptsWaiting()
        {
            return uxQueueMessagesWaiting(s_queueLowpassInterrupts);
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

            if (s_FSMstate == LogicState::low && rawInterrupt == Interrupt::rising ||
                s_FSMstate == LogicState::high && rawInterrupt == Interrupt::falling)
            {
                // When "rawInterrupt" tries to change state
                if (uxSemaphoreGetCount(s_binarySemaphoreProtectTimerActive))
                {
                    xSemaphoreTakeFromISR(s_binarySemaphoreProtectTimerActive, &xHigherPriorityTaskWoken);
                    xTimerChangePeriodFromISR(s_timerStateChanger, s_lowToHighTimeoutTicks, &xHigherPriorityTaskWoken);
                    xTimerStartFromISR(s_timerStateChanger, &xHigherPriorityTaskWoken);
                    isr_log_v("[INT PIN %u] Got %s when state is %s -> Timer started",
                              getPin(), InterruptToStr(rawInterrupt), LogicStateToStr(s_FSMstate));
                }
                else
                {
                    isr_log_e("[INT PIN %u] (IMPOSSIBLE) Got %s when state is %s -> Timer already started",
                              getPin(), InterruptToStr(rawInterrupt), LogicStateToStr(s_FSMstate));
                }
            }
            else if (s_FSMstate == LogicState::low && rawInterrupt == Interrupt::falling ||
                     s_FSMstate == LogicState::high && rawInterrupt == Interrupt::rising)
            {
                // When "rawInterrupt" tries to reset state change
                if (!uxSemaphoreGetCount(s_binarySemaphoreProtectTimerActive))
                {
                    xTimerStopFromISR(s_timerStateChanger, &xHigherPriorityTaskWoken);
                    xSemaphoreGiveFromISR(s_binarySemaphoreProtectTimerActive, &xHigherPriorityTaskWoken);
                    isr_log_v("[INT PIN %u] Got %s when state is %s -> Timer stopped",
                              getPin(), InterruptToStr(rawInterrupt), LogicStateToStr(s_FSMstate));
                }
                else
                {
                    isr_log_e("[INT PIN %u] (IMPOSSIBLE) Got %s when state is %s -> Timer was not started",
                              getPin(), InterruptToStr(rawInterrupt), LogicStateToStr(s_FSMstate));
                }
            }
            else
            {
                // When "s_FSMstate" is LogicState::undefined
#if DELAY_FISRST_STATE_CHANGE == true
                if (uxSemaphoreGetCount(s_binarySemaphoreProtectTimerActive))
                {
                    xSemaphoreTakeFromISR(s_binarySemaphoreProtectTimerActive, &xHigherPriorityTaskWoken);
                    switch (rawInterrupt)
                    {
                    case Interrupt::falling:
                        xTimerChangePeriodFromISR(s_timerStateChanger, s_highToLowTimeoutTicks, &xHigherPriorityTaskWoken);
                        break;
                    case Interrupt::rising:
                        xTimerChangePeriodFromISR(s_timerStateChanger, s_lowToHighTimeoutTicks, &xHigherPriorityTaskWoken);
                        break;
                    default:
                        break;
                    }
                    xTimerStartFromISR(s_timerStateChanger, &xHigherPriorityTaskWoken);
                    s_auxFirstInterrupt = rawInterrupt;
                    isr_log_v("[INT PIN %u] Got %s when state is %s -> Timer started",
                              getPin(), InterruptToStr(rawInterrupt), LogicStateToStr(s_FSMstate));
                }
                else
                {
                    if (s_auxFirstInterrupt != rawInterrupt)
                    {
                        xTimerStopFromISR(s_timerStateChanger, &xHigherPriorityTaskWoken);
                        xSemaphoreGiveFromISR(s_binarySemaphoreProtectTimerActive, &xHigherPriorityTaskWoken);
                        isr_log_v("[INT PIN %u] Got %s when state is %s -> Timer stopped",
                                  getPin(), InterruptToStr(rawInterrupt), LogicStateToStr(s_FSMstate));
                    }
                    else
                    {
                        isr_log_e("[INT PIN %u] (IMPOSSIBLE) Got %s when state is %s -> Timer already started",
                                  getPin(), InterruptToStr(rawInterrupt), LogicStateToStr(s_FSMstate));
                    }
                }
#else
                s_auxFirstInterrupt = rawInterrupt;
                xSemaphoreTakeFromISR(s_binarySemaphoreProtectTimerActive, &xHigherPriorityTaskWoken);
                timerStateChangerCallback(s_timerStateChanger);
#endif
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
                isr_log_v("[INT PIN %u] (DEBOUNCER) Discarting same interrupt", getPin());
            // Did this action unblock a higher priority task?
            if (xHigherPriorityTaskWoken)
                portYIELD_FROM_ISR();
        }

        static void IRAM_ATTR timerNoActivityCallback(const TimerHandle_t xTimer)
        {
            // Call the ISR
            isr_log_d("[INT PIN %u] No activity for %d seconds. Calling ISR...",
                      getPin(), pdTICKS_TO_MS(s_noActivityTimeoutTicks) / 1000);
            isrFunc();
        }

        static void IRAM_ATTR timerStateChangerCallback(const TimerHandle_t xTimer)
        {
            // Update the state
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
                switch (s_auxFirstInterrupt)
                {
                case Interrupt::falling:
                    s_FSMstate = LogicState::low;
                    break;
                case Interrupt::rising:
                    s_FSMstate = LogicState::high;
                    break;
                }
                break;
            }
            }

            BaseType_t xHigherPriorityTaskWoken{pdFALSE};
            // Notify that the timer is available
            xSemaphoreGiveFromISR(s_binarySemaphoreProtectTimerActive, &xHigherPriorityTaskWoken);
            // Send the filtered interrupt to the queue
            const Interrupt interrupt{s_FSMstate == LogicState::low ? Interrupt::falling : Interrupt::rising};
            log_i("[INT PIN %u] Got interrupt %s", getPin(), InterruptToStr(interrupt));
            xQueueSendFromISR(s_queueLowpassInterrupts, &interrupt, &xHigherPriorityTaskWoken);
            // Enable binary semaphore to report that there are unprocessed filtered interrupts in any of the templated classes
            xSemaphoreGiveFromISR(semaphoreLowpassInterruptToRead, &xHigherPriorityTaskWoken);
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
        static TickType_t s_noActivityTimeoutTicks;
        // FSM timers
        static TimerHandle_t s_timerStateChanger;
        static TimerHandle_t s_timerNoActivity;
        // Variable to determine the first state change
        static Interrupt s_auxFirstInterrupt;
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
    LogicState LowpassInterrupt<t_pin>::s_FSMstate{LogicState::undefined};
    template <uint8_t t_pin>
    uint8_t LowpassInterrupt<t_pin>::s_mode;
    template <uint8_t t_pin>
    TickType_t LowpassInterrupt<t_pin>::s_lowToHighTimeoutTicks;
    template <uint8_t t_pin>
    TickType_t LowpassInterrupt<t_pin>::s_highToLowTimeoutTicks;
    // Timeout to wait when no events happen
    template <uint8_t t_pin>
    TickType_t LowpassInterrupt<t_pin>::s_noActivityTimeoutTicks;
    // FSM timers
    template <uint8_t t_pin>
    TimerHandle_t LowpassInterrupt<t_pin>::s_timerStateChanger{nullptr};
    template <uint8_t t_pin>
    TimerHandle_t LowpassInterrupt<t_pin>::s_timerNoActivity{nullptr};
    // Variable to determine the first state change
    template <uint8_t t_pin>
    Interrupt LowpassInterrupt<t_pin>::s_auxFirstInterrupt{Interrupt::noInterrupt};
    // Queues
    template <uint8_t t_pin>
    QueueHandle_t LowpassInterrupt<t_pin>::s_queueLowpassInterrupts{nullptr};
    template <uint8_t t_pin>
    QueueHandle_t LowpassInterrupt<t_pin>::s_binarySemaphoreProtectTimerActive{nullptr};

} // namespace AT
