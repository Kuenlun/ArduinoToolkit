#include <ArduinoToolkit.h>

static AT::LowpassInterrupt<1> intPir(INPUT_PULLDOWN,
                                      pdMS_TO_TICKS(100),
                                      pdMS_TO_TICKS(1000),
                                      pdMS_TO_TICKS(10000));

static AT::LowpassInterrupt<42> intBed(INPUT_PULLUP,
                                       pdMS_TO_TICKS(100),
                                       pdMS_TO_TICKS(1000),
                                       pdMS_TO_TICKS(10000));

/* * * * * *
 *  SETUP  *
 * * * * * */
void setup()
{
}

/* * * * * *
 *  LOOP   *
 * * * * * */
void loop()
{
    // Method "receiveLowpassInterrupts" blocks the task until a LowpassInterrupt happens
    // log_i("Pin %u got interrupt %d", intPir.getPin(), intPir.receiveLowpassInterrupts());

    // Method "getState" gets the current state of the FSM
    log_i("Pin %u state is %d", intPir.getPin(), intPir.getState());
    vTaskDelay(pdMS_TO_TICKS(5000));
}
