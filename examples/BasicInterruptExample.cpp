#include <ArduinoToolkit/Interrupt/BasicInterrupt.h>

#define PIN_INT_DOOR 25
#define PIN_INT_PIR 26

/* * * * * *
 *  SETUP  *
 * * * * * */
void setup()
{
    static AT::BasicInterrupt doorInt(PIN_INT_DOOR, INPUT_PULLUP, true);
    static AT::BasicInterrupt pirInt(PIN_INT_PIR, INPUT_PULLDOWN);
    vTaskDelete(NULL);
}

/* * * * * *
 *  LOOP   *
 * * * * * */
void loop()
{
    // Setup and loop task has been deleted
    // Code written here won't run
}
