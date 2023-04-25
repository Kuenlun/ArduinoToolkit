#include <ArduinoToolkit/Interrupt/BasicInterrupt.h>
#include <ArduinoToolkit/Interrupt/FilteredInterrupt.h>

static constexpr uint8_t PIN_INT_DOOR{25};
static constexpr uint8_t PIN_INT_PIR{26};

/* * * * * *
 *  SETUP  *
 * * * * * */
void setup()
{
    static AT::FilteredInterrupt doorInt(PIN_INT_DOOR, INPUT_PULLUP, 500, 1000, true);
    static AT::FilteredInterrupt pirInt(PIN_INT_PIR, INPUT_PULLDOWN, 100, 1000, false);
    while (true)
    {
        AT::FilteredInterrupt::waitUntilAnyInterrupt();
        switch (doorInt.receiveInterruptDiscardIntermediate(0))
        {
        case AT::PinState::High:
            LOG_I("Pin %u: HIGH", doorInt.getPin());
            break;
        case AT::PinState::Low:
            LOG_I("Pin %u: LOW", doorInt.getPin());
            break;
        default:
            break;
        }
        switch (pirInt.receiveInterruptDiscardIntermediate(0))
        {
        case AT::PinState::High:
            LOG_I("Pin %u: HIGH", pirInt.getPin());
            break;
        case AT::PinState::Low:
            LOG_I("Pin %u: LOW", pirInt.getPin());
            break;
        default:
            break;
        }
    }
}

/* * * * * *
 *  LOOP   *
 * * * * * */
void loop()
{
    // Code written here won't run
}
