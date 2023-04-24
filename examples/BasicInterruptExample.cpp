#include <ArduinoToolkit/Interrupt/BasicInterrupt.h>
#include <ArduinoToolkit/Interrupt/FilteredInterrupt.h>

static constexpr uint8_t PIN_INT_DOOR{25};
static constexpr uint8_t PIN_INT_PIR{26};

/* * * * * *
 *  SETUP  *
 * * * * * */
void setup()
{
    static AT::BasicInterrupt doorInt(PIN_INT_DOOR, INPUT_PULLUP, true);
    while (true)
    {
        switch (doorInt.receiveInterruptDiscardIntermediate())
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
    }
}

/* * * * * *
 *  LOOP   *
 * * * * * */
void loop()
{
    // Code written here won't run
}
