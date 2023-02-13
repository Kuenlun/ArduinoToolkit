#include <ArduinoToolkit.h>

/* * * * * *
 *  SETUP  *
 * * * * * */
void setup()
{
    AT::LowpassInterrupt<1> intPir(INPUT_PULLDOWN,
                                   pdMS_TO_TICKS(100),
                                   pdMS_TO_TICKS(1000),
                                   pdMS_TO_TICKS(10000));

    AT::LowpassInterrupt<42> intBed(INPUT_PULLUP,
                                    pdMS_TO_TICKS(100),
                                    pdMS_TO_TICKS(1000),
                                    pdMS_TO_TICKS(10000));

    while (true)
    {
        // Blocks the task until any lowpass interrupt arrives
        AT::blockUntilLowpassInterrupt();
        if (intPir.getLowpassInterruptsWaiting())
        {
            const AT::Interrupt interrupt{intPir.receiveLowpassInterrupts()};
            log_i("Pin %u got interrupt %s. New state is %s",
                  intPir.getPin(), AT::InterruptToStr(interrupt), AT::LogicStateToStr(intPir.getState()));
        }
        if (intBed.getLowpassInterruptsWaiting())
        {
            const AT::Interrupt interrupt{intBed.receiveLowpassInterrupts()};
            log_i("Pin %u got interrupt %s. New state is %s",
                  intBed.getPin(), AT::InterruptToStr(interrupt), AT::LogicStateToStr(intBed.getState()));
        }
    }
}

/* * * * * *
 *  LOOP   *
 * * * * * */
void loop()
{
}
