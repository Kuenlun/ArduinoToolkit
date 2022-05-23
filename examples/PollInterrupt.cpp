#include "PollInterrupt.h"


static Tools::PollInterrupt<13> pirInt(INPUT_PULLDOWN);
static Tools::LowPassPollInterrupt<14> ledInt(INPUT_PULLDOWN, 1000, 1000);


/* * * * * *
 *  SETUP  *
 * * * * * */
void setup()
{
    Serial.begin(115200);
}

/* * * * * *
 *  LOOP   *
 * * * * * */
void loop()
{
    switch (pirInt.poll())
    {
    case Tools::Interrupt::rising:
        Serial.println("PIR Rising");
        break;
    case Tools::Interrupt::falling:
        Serial.println("PIR Falling");
        break;
    default:
        break;
    }

    switch (ledInt.poll())
    {
    case Tools::Interrupt::rising:
        Serial.println("LED Rising");
        break;
    case Tools::Interrupt::falling:
        Serial.println("LED Falling");
        break;
    default:
        break;
    }

    static unsigned long initTime = millis();
    if (millis() - initTime > 1000) {
        initTime = millis();

        Serial.printf("%d\n", (int8_t)pirInt.getCurrentState());

        Serial.printf("%d\n", (int8_t)ledInt.getCurrentState());
        Serial.printf("%d\n", (int8_t)ledInt.getFilteredState());
        Serial.println();
    }
}