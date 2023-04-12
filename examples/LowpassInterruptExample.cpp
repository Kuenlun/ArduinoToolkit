#include "ArduinoToolkit/Interrupt/LowpassInterrupt.h"

/* * * * * *
 *  SETUP  *
 * * * * * */
void setup()
{
    AT::LowpassInterrupt<1> intPin(INPUT_PULLUP,
                                   pdMS_TO_TICKS(100),
                                   pdMS_TO_TICKS(1000),
                                   pdMS_TO_TICKS(10000));

    while (true)
    {
        // Blocks the task until any lowpass interrupt arrives
        AT::blockUntilLowpassInterrupt();
        if (intPin.getLowpassInterruptsWaiting())
        {
            const AT::Interrupt interrupt{intPin.receiveLowpassInterrupts()};
            LOG_I("Pin %u got interrupt %s. Current state is %s",
                  intPin.getPin(), AT::InterruptToStr(interrupt), AT::LogicStateToStr(intPin.getState()));
        }
    }
}

/* * * * * *
 *  LOOP   *
 * * * * * */
void loop()
{
    // Code here won't run
}
