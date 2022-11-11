#include <ArduinoToolkit.h>

static AT::LowpassInterrupt<1> pirInt1(INPUT_PULLDOWN,
                                       pdMS_TO_TICKS(100),
                                       pdMS_TO_TICKS(1000),
                                       pdMS_TO_TICKS(30000));

static AT::LowpassInterrupt<2> pirInt2(INPUT_PULLDOWN,
                                       1,
                                       pdMS_TO_TICKS(2000),
                                       pdMS_TO_TICKS(5000));

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
    //log_i("Pin %u got interrupt %d", pirInt1.getPin(), pirInt1.receiveLowpassInterrupts());

    // Method "getState" gets the current state of the FSM
    log_i("Pin %u state is %d", pirInt1.getPin(), pirInt1.getState());
    log_i("Pin %u state is %d", pirInt2.getPin(), pirInt2.getState());
    vTaskDelay(pdMS_TO_TICKS(5000));
}
