#include "PollInterrupt.h"


// Basic pollInterrupt on pin 13
static Tools::PollInterrupt<13> normalInt(INPUT_PULLDOWN);
// Filtered pollInterrupt on pin 14
static Tools::LowPassPollInterrupt<14> filteredInt(INPUT_PULLDOWN, 1000, 1000);


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
    // Poll method updates the state and returns an interrupt
    // As it is a poll interrupt should be called continuously
    switch (normalInt.poll())
    {
    case Tools::Interrupt::rising:
        Serial.println("Normal Interrupt Rising");
        break;
    case Tools::Interrupt::falling:
        Serial.println("Normal Interrupt Falling");
        break;
    default:
        break;
    }

    // Poll method updates the state and returns an interrupt
    // The interrupt is returned based on the change of states of the FSM
    // As it is a poll interrupt should be called continuously
    switch (filteredInt.poll())
    {
    case Tools::Interrupt::rising:
        Serial.println("Filtered Interrupt Rising");
        break;
    case Tools::Interrupt::falling:
        Serial.println("Filtered Interrupt Falling");
        break;
    default:
        break;
    }

    // Get every second the state of the interrupts
    static unsigned long initTime = millis();
    if (millis() - initTime > 1000) {
        initTime = millis();

        // Get the current state of the interrupt (instantaneously)
        Serial.printf("%d\n", (int8_t)normalInt.getCurrentState());

        // Get the current state of the interrupt (instantaneously)
        Serial.printf("%d\n", (int8_t)filteredInt.getCurrentState());
        // Get the current state of the interrupt (state of the FSM, filtered)
        Serial.printf("%d\n", (int8_t)filteredInt.getFilteredState());
        Serial.println();
    }
}