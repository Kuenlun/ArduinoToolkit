#pragma once

#include <Arduino.h>


namespace Tools
{

    enum class Interrupt : int8_t
    {
        noInterrupt = 0, rising = 1, falling = -1
    };

    enum class State : int8_t
    {
        undefined = -1, low = 0, high = 1
    };


    /**
     * @brief
     * 1. Constructor configures the GPIO and attaches it to the interrupt
     * 2. ISR calls the "interrupt" function
     * 3. The poll function catches the interrupt flag
     * 4. Depending on the previous and curent state of the GPIO we can get the interrupt
     * 
     * @tparam t_intPin Interrupt GPIO pin
     */
    template <uint8_t t_intPin>
    class PollInterrupt
    {
    public:
        PollInterrupt(const uint8_t mode=INPUT)
        {
            pinMode(t_intPin, mode);    // See if it is needed
            attachInterrupt(t_intPin, interrupt, CHANGE);
        }

        static Interrupt poll()
        {
            if (s_intFlag) {
                // Reset the interrupt flag
                s_intFlag = false;

                const State newState = (State)digitalRead(t_intPin);
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

        static inline State getCurrentState() { return s_state; }

    private:
        static bool s_intFlag;
        static State s_state;

    private:
        IRAM_ATTR static void interrupt() { s_intFlag = true; }
    };

    template <uint8_t t_intPin>
    bool PollInterrupt<t_intPin>::s_intFlag = false;

    template <uint8_t t_intPin>
    State PollInterrupt<t_intPin>::s_state = State::undefined;


    /**
     * @brief Same functionality as the parent class "PollInterrupt" with
     * the addition of a low-pass filter implemented through a finite state machine.
     * 
     * @tparam t_intPin Interrupt GPIO pin
     */
    template <uint8_t t_intPin>
    class LowPassPollInterrupt : public PollInterrupt<t_intPin>
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
            : PollInterrupt<t_intPin>(mode),
              m_FSMState(FSMState::undefined),
              m_lowToHighWaitTime(lowToHighWaitTime),
              m_highToLowWaitTime(highToLowWaitTime)
        {
        }

        Interrupt poll()
        {
            // Call parent poll function
            const Interrupt interrupt = PollInterrupt<t_intPin>::poll();
            // Feed the FSM
            updateFSMstate(interrupt);
            // Output interrupt depending on timing
            return updateFSMtiming();
        }

        State getFilteredState() {
            switch (m_FSMState)
            {
            case FSMState::low:
                return State::low;
            case FSMState::high:
                return State::high;
            case FSMState::waitToHigh:
                return State::low;
            case FSMState::waitToLow:
                return State::high;
            default:
                return State::undefined;
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
