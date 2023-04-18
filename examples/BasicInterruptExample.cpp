#include <ArduinoToolkit/Interrupt/BasicInterrupt.h>
#include <ArduinoToolkit/Interrupt/FilteredInterrupt.h>

static constexpr uint8_t PIN_INT_DOOR{25};
static constexpr uint8_t PIN_INT_PIR{26};

/* * * * * *
 *  SETUP  *
 * * * * * */
void setup()
{
    // static AT::BasicInterrupt pirInt(PIN_INT_PIR, INPUT_PULLDOWN);
    static AT::FilteredInterrupt doorInt(PIN_INT_DOOR, INPUT_PULLUP, true, 100, 200);
    while (true)
    {
        AT::PinState state{doorInt.receiveInterrupt()};
        // Log the current state of the sensor pin
        switch (state)
        {
        case AT::PinState::High:
            AT_LOG_D("Pin %u: HIGH", doorInt.getPin());
            break;
        case AT::PinState::Low:
            AT_LOG_D("Pin %u: LOW", doorInt.getPin());
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
