#pragma once

#include <vector>

#include <Arduino.h>

namespace AT
{

    // IDs for the timers
    static constexpr uint8_t TIMER_LOW_TO_HIGH_ID = 0;
    static constexpr uint8_t TIMER_HIGH_TO_LOW_ID = 1;
    static constexpr uint8_t ISR_TO_INTERRUPT_TASK_QUEUE_SIZE = 10;
    static constexpr uint8_t LOWPASS_INTERRUPT_QUEUE_SIZE = 10;

    class Interrupt
    {
    protected:
        Interrupt(const uint8_t mode,
                  const TickType_t lowToHighTimeoutTicks, // Must be >0 ticks
                  const TickType_t highToLowTimeoutTicks, // Must be >0 ticks
                  const TickType_t noEventTimeoutTicks = portMAX_DELAY)
            : m_mode(mode),
              m_lowToHighTimeoutTicks(lowToHighTimeoutTicks),
              m_highToLowTimeoutTicks(highToLowTimeoutTicks),
              m_noEventTimeoutTicks(noEventTimeoutTicks),
              m_timerHighToLow(nullptr),
              m_timerLowToHigh(nullptr)
        {
        }

    public:
        virtual bool getState() = 0;

        virtual bool getPin() = 0;

        const uint8_t m_mode;
        const TickType_t m_lowToHighTimeoutTicks;
        const TickType_t m_highToLowTimeoutTicks;
        const TickType_t m_noEventTimeoutTicks;
        // FSM timers
        TimerHandle_t m_timerHighToLow;
        TimerHandle_t m_timerLowToHigh;
    };

    /**
     * Declaration of global variables shared among
     * all templates of LowpassInterrupt class
     */
    // FreeRTOS mutex to protect Arduino "attachInterrupt" function
    extern SemaphoreHandle_t mutexCreateInterrupt;
    // FreeRTOS queue to send unfiltered interrupts from the ISR to the interrupt task
    extern QueueHandle_t queueInterrupts;
    // Vector to save the pins used for LowpassInterrupt
    extern std::vector<Interrupt *> interruptPinsUsed;
    extern SemaphoreHandle_t mutexProtectVector;
    // FreeRTOS task to handle the interrupts
    extern void LowpassInterruptTask(void *const parameters);

    struct isrToIntTaskQueueElement
    {
        uint8_t pin;
        bool state;
    };

    template <uint8_t t_pin>
    class LowpassInterrupt : public Interrupt
    {
    public:
        // Constructor
        LowpassInterrupt(const uint8_t mode,
                         const TickType_t lowToHighTimeoutTicks, // Must be >0 ticks
                         const TickType_t highToLowTimeoutTicks, // Must be >0 ticks
                         const TickType_t noEventTimeoutTicks = portMAX_DELAY)
            : Interrupt(mode, lowToHighTimeoutTicks, highToLowTimeoutTicks, noEventTimeoutTicks)
        {
            if (!mutexProtectVector)
            {
                mutexProtectVector = xSemaphoreCreateMutex();
                if (!mutexProtectVector)
                    log_e("Could not create mutex");
            }
            xSemaphoreTake(mutexProtectVector, portMAX_DELAY);
            // Create the task only the first time
            if (interruptPinsUsed.empty())
            {
                // Add the interrupt pin used to the vector
                interruptPinsUsed.push_back(this);

                log_w("Se ha metido un elemento en el vector: %d", interruptPinsUsed.size());

                // Create the mutex to protect the attachment of interrupts if it is not initialized yet
                mutexCreateInterrupt = xSemaphoreCreateMutex();
                if (!mutexCreateInterrupt)
                    log_e("Could not create mutex");

                // Create the queue to send unfiltered interrupts from the ISR to the interrupt task
                queueInterrupts = xQueueCreate(ISR_TO_INTERRUPT_TASK_QUEUE_SIZE, sizeof(isrToIntTaskQueueElement));
                if (!queueInterrupts)
                    log_e("Could not create the queue");

                xTaskCreatePinnedToCore(
                    LowpassInterruptTask,
                    "LowpassInterruptTask",
                    3 * 1024,
                    nullptr,
                    1,
                    nullptr,
                    ARDUINO_RUNNING_CORE);
            }
            else
            {
                // Check if the pin has already been used
                for (Interrupt *obj : interruptPinsUsed)
                {
                    if (t_pin == obj->getPin())
                    {
                        log_e("Pin %d has already been used", t_pin);
                        ESP_ERROR_CHECK(ESP_FAIL);
                    }
                }

                // Add the interrupt pin used to the vector
                interruptPinsUsed.push_back(this);
            }
            xSemaphoreGive(mutexProtectVector);

            // Check that timeouts for changing states are greater than 0 ticks
            if (!lowToHighTimeoutTicks || !highToLowTimeoutTicks)
            {
                log_e("Timeout for changing FSM states must be grater than 0 ticks");
                ESP_ERROR_CHECK(ESP_ERR_INVALID_ARG);
            }

            // Create the queue to send the filtered interrupts (outputs)
            s_queueLowpassInterrupts = xQueueCreate(LOWPASS_INTERRUPT_QUEUE_SIZE, sizeof(bool));
            if (!s_queueLowpassInterrupts)
                log_e("Could not create the queue");

            // Create the timers
            m_timerLowToHigh = xTimerCreate(
                "m_timerLowToHigh",
                lowToHighTimeoutTicks,
                pdFALSE,
                (void *)TIMER_LOW_TO_HIGH_ID,
                timerFSMcallback);
            if (!m_timerLowToHigh)
                log_e("Could not create timer");

            m_timerHighToLow = xTimerCreate(
                "m_timerHighToLow",
                highToLowTimeoutTicks,
                pdFALSE,
                (void *)TIMER_HIGH_TO_LOW_ID,
                timerFSMcallback);
            if (!m_timerHighToLow)
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

        static bool receiveLowpassInterrupts(const TickType_t blockTimeTicks = portMAX_DELAY)
        {
            bool state;
            xQueueReceive(s_queueLowpassInterrupts, &state, blockTimeTicks);
            return state;
        }

        bool getState() override { return s_FSMstate; }

        bool getPin() override { return t_pin; }

    private:
        static void IRAM_ATTR isrFunc()
        {
            // Read the pin
            const isrToIntTaskQueueElement queueElement = {t_pin, digitalRead(t_pin)};
            BaseType_t xHigherPriorityTaskWoken = pdFALSE;
            xQueueSendFromISR(queueInterrupts, &queueElement, &xHigherPriorityTaskWoken);
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

    private:
        // Current state of the finite state machine
        static bool s_FSMstate;
        // FreeRTOS queue to send the filtered interrupts (outputs)
        static QueueHandle_t s_queueLowpassInterrupts;

    }; // class LowpassInterrupt

    /**
     * Definition of static variables from LowpassInterrupt class
     */
    // Current state of the finite state machine
    template <uint8_t t_pin>
    bool LowpassInterrupt<t_pin>::s_FSMstate = false;
    // Queues
    template <uint8_t t_pin>
    QueueHandle_t LowpassInterrupt<t_pin>::s_queueLowpassInterrupts = nullptr;

} // namespace AT
