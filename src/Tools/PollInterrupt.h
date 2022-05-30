#pragma once

#include <Arduino.h>


namespace Tools
{

    struct Interrupt
    {
    public:
        enum Value : int8_t
        {
            noInterrupt = 0, rising = 1, falling = -1
        };

        Interrupt() = default;
        constexpr Interrupt(Value value) : m_value(value) { }

        // Allow switch and comparisons
        constexpr operator Value() const { return m_value; }
        // Allow cast to bool
        constexpr explicit operator bool() const { return (bool)m_value; }
    
    private:
        Value m_value;
    };


    struct State
    {
    public:
        enum Value : int8_t
        {
            undefined = -1, low = 0, high = 1
        };

        State() = default;
        constexpr State(const Value value) : m_value(value) { }
        constexpr State(const bool value) : m_value((Value)value) { }

        // Allow switch and comparisons
        constexpr operator Value() const { return m_value; }

        /**
         * @brief Prevent "if(state)" usage, as "undefined" value
         * can not be mapped to bool. Use "toBool" function instead.
         */
        explicit operator bool() const = delete;  

        /**
         * @brief Cast state to bool
         * @param undefinedDefault Whether undefinded state will be
         * mapped to true or falase.
         */
        bool toBool(const bool undefinedDefault=false) const
        {
            switch (m_value)
            {
            case low:
                return false;
            case high:
                return true;
            default:
                return undefinedDefault ? true : false;
            }
        }

    private:
        Value m_value;
    };


    /**
     * @brief
     * 1. Constructor configures the GPIO and attaches it to the interrupt
     * 2. ISR calls the "interrupt" function
     * 3. The poll function catches the interrupt flag
     * 4. Depending on the previous and curent state of the GPIO we can get the interrupt
     * 
     * @tparam t_intPin (uint8_t) Interrupt GPIO pin
     * @tparam t_invertLogic (bool) Invert the logic or not
     */
    template <uint8_t t_intPin, bool t_invertLogic=false>
    class PollInterrupt
    {
    public:
        /**
         * @param mode  PinMode setting: INPUT, INPUT_PULLDOWN or INPUT_PULLUP
         */
        PollInterrupt(const uint8_t mode=INPUT)
        {
            pinMode(t_intPin, mode);
            attachInterrupt(t_intPin, interrupt, CHANGE);
        }

        /**
         * @brief Updates the state and returns an interrupt
         * 
         * @return Interrupt 
         */
        static Interrupt poll()
        {
            if (s_intFlag) {
                // Reset the interrupt flag
                s_intFlag = false;
                // Read the GPIO value, invert it depending on "t_invertLogic" and cast it to State
                const State newState = State(t_invertLogic ^ digitalRead(t_intPin));
                // Remark: "newState" can't be "undefined"
                if (newState == s_state) {
                    return Interrupt::noInterrupt;
                } else if (newState == State::low ) {
                    s_state = newState;
                    return Interrupt::falling;
                } else {
                    s_state = newState;
                    return Interrupt::rising;
                }
            }
            return Interrupt::noInterrupt;
        }

        /**
         * @brief Get the current state of the interrupt
         * 
         * @return State 
         */
        static inline State getState() { return s_state; }

    private:
        static bool s_intFlag;
        static State s_state;

    private:
        IRAM_ATTR static void interrupt() { s_intFlag = true; }
    };

    template <uint8_t t_intPin, bool t_invertLogic>
    bool PollInterrupt<t_intPin, t_invertLogic>::s_intFlag = false;

    template <uint8_t t_intPin, bool t_invertLogic>
    State PollInterrupt<t_intPin, t_invertLogic>::s_state = State::undefined;



    /**
     * @brief Same functionality as the parent class "PollInterrupt" with
     * the addition of a low-pass filter implemented through a finite state machine.
     * 
     * @tparam t_intPin (uint8_t) Interrupt GPIO pin
     * @tparam t_invertLogic (bool) Invert the logic or not
     */
    template <uint8_t t_intPin, bool t_invertLogic=false>
    class LowPassPollInterrupt : public PollInterrupt<t_intPin, t_invertLogic>
    {
    private:
        enum class FSMState : int8_t
        {
            undefined = -1, low = 0, high = 1, waitToHigh = 2, waitToLow = 3
        };

    public:
        /**
         * @param mode  PinMode setting: INPUT, INPUT_PULLDOWN or INPUT_PULLUP
         * @param lowToHighWaitTime Time to wait whithout bounces for rising interrupt
         * @param highToLowWaitTime Time to wait whithout bounces for falling interrupt
         */
        LowPassPollInterrupt(const uint8_t mode=INPUT,
                             const unsigned long lowToHighWaitTime=0,
                             const unsigned long highToLowWaitTime=0)
            : PollInterrupt<t_intPin, t_invertLogic>(mode),
              m_FSMState(FSMState::undefined),
              m_lowToHighWaitTime(lowToHighWaitTime),
              m_highToLowWaitTime(highToLowWaitTime)
        {
        }

        /**
         * @brief Updates the state of the interrupt calling the parent poll method
         * and also updates the state of the FSM
         * Returns an interrupt based on the changes of states of the FSM
         * 
         * @return Interrupt 
         */
        Interrupt poll()
        {
            // Call parent poll function
            const Interrupt interrupt = PollInterrupt<t_intPin, t_invertLogic>::poll();
            // Feed the FSM
            updateFSMstate(interrupt);
            // Output interrupt depending on timing
            return updateFSMtiming();
        }

        /**
         * @brief Get the current state of the interrupt (same as parent method)
         * 
         * @return State 
         */
        State getRawState() { return PollInterrupt<t_intPin, t_invertLogic>::getState(); }

        /**
         * @brief Get the current state of the FSM (filtered state of the interrupt)
         * 
         * @return State
         */
        State getState() {
            switch (m_FSMState)
            {
            case FSMState::low: return State::low;
            case FSMState::high: return State::high;
            case FSMState::waitToHigh: return State::low;
            case FSMState::waitToLow: return State::high;
            default: return State::undefined;
            }
        }

    private:
        void updateFSMstate(const Interrupt interrupt)
        {
            if (interrupt == Interrupt::rising) {
                if (m_FSMState == FSMState::undefined || m_FSMState == FSMState::low) {
                    m_timer = millis();
                    m_FSMState = FSMState::waitToHigh;
                } else if (m_FSMState == FSMState::waitToLow) {
                    m_FSMState = FSMState::high;
                }
            } else if (interrupt == Interrupt::falling) {
                if (m_FSMState == FSMState::undefined || m_FSMState == FSMState::high) {
                    m_timer = millis();
                    m_FSMState = FSMState::waitToLow;
                } else if (m_FSMState == FSMState::waitToHigh) {
                    m_FSMState = FSMState::low;
                }
            }
        }

        Interrupt updateFSMtiming()
        {
            if (m_FSMState == FSMState::waitToHigh) {
                if (millis() - m_timer > m_lowToHighWaitTime) {
                    m_FSMState = FSMState::high;
                    return Interrupt::rising;
                }
            } else if (m_FSMState == FSMState::waitToLow) {
                if (millis() - m_timer > m_highToLowWaitTime) {
                    m_FSMState = FSMState::low;
                    return Interrupt::falling;
                }
            }
            return Interrupt::noInterrupt;
        }

    private:
        FSMState m_FSMState;
        unsigned long m_lowToHighWaitTime, m_highToLowWaitTime, m_timer;
    };

}
