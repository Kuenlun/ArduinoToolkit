#include "Tools/PollInterrupt.h"
#include "Tools/Logger.h"


// Basic pollInterrupt on pin 13
static Tools::PollInterrupt<13> normalInt(INPUT_PULLDOWN);
// Filtered pollInterrupt on pin 14 with inverted logic
static Tools::LowPassPollInterrupt<14, true> filteredInt(INPUT_PULLDOWN, 1000, 1000);


/* * * * * *
 *  SETUP  *
 * * * * * */
void setup()
{
    Serial.begin(115200);
    Tools::logger.config("PollInt");
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
        LOG_INFO("Normal Interrupt Rising");
        break;
    case Tools::Interrupt::falling:
        LOG_INFO("Normal Interrupt Falling");
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
        LOG_INFO("Filtered Interrupt Rising");
        break;
    case Tools::Interrupt::falling:
        LOG_INFO("Filtered Interrupt Falling");
        break;
    default:
        break;
    }

    // Get every second the state of the interrupts
    static unsigned long initTime = millis();
    if (millis() - initTime > Tools::secsToMillis(1)) {
        initTime = millis();

        // Get the current state of the interrupt (instantaneously)
        LOG_TRACE("PollInterrupt: %d", normalInt.getState());

        // Get the current state of the interrupt (state of the FSM, filtered)
        LOG_TRACE("LowPassPollInterrupt: %d", filteredInt.getState());
        // Get the current state of the interrupt (instantaneously)
        LOG_TRACE("LowPassPollInterrupt (raw): %d", filteredInt.getRawState());
        LOG_NL();
    }
}
