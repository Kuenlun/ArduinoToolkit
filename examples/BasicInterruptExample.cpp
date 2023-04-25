#include <ArduinoToolkit/Interrupt/BasicInterrupt.h>

static constexpr uint8_t PIN_INT_DOOR{25};
static constexpr uint8_t PIN_INT_PIR{26};

/* * * * * *
 *  SETUP  *
 * * * * * */
void setup()
{
    static AT::BasicInterrupt doorInt(PIN_INT_DOOR, INPUT_PULLUP, true);
    static AT::BasicInterrupt pirInt(PIN_INT_PIR, INPUT_PULLDOWN, false);
    while (true)
    {
        AT::BasicInterrupt::waitUntilAnyInterrupt();
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
