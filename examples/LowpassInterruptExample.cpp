#include <ArduinoToolkit.h>

/*
static AT::LowpassInterrupt<1> pirInt1(INPUT_PULLDOWN,
                                       pdMS_TO_TICKS(100),
                                       pdMS_TO_TICKS(1000),
                                       pdMS_TO_TICKS(30000));
static AT::LowpassInterrupt<2> pirInt2(INPUT_PULLDOWN,
                                       pdMS_TO_TICKS(100),
                                       pdMS_TO_TICKS(1000),
                                       pdMS_TO_TICKS(30000));
*/
static AT::LowpassInterrupt<4> intBed(INPUT_PULLUP,
                                      pdMS_TO_TICKS(100),
                                      pdMS_TO_TICKS(1000),
                                      pdMS_TO_TICKS(30000));

/* * * * * *
 *  SETUP  *
 * * * * * */
void setup()
{
    // pirInt1.begin();
    // pirInt2.begin();
    intBed.begin();
}

/* * * * * *
 *  LOOP   *
 * * * * * */
void loop()
{
    // log_i("Pin %u got interrupt %d", 1, pirInt1.receiveLowpassInterrupts());
    // log_i("Pin %u got interrupt %d", 2, pirInt2.receiveLowpassInterrupts());
    log_i("Got bed interrupt %d", intBed.receiveLowpassInterrupts());
}
