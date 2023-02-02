#include <ArduinoToolkit.h>

static AT::LowpassInterrupt<1> intPir1(INPUT_PULLDOWN,
                                       pdMS_TO_TICKS(100),
                                       pdMS_TO_TICKS(1000),
                                       pdMS_TO_TICKS(10000));

static AT::LowpassInterrupt<42> intPir2(INPUT_PULLDOWN,
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
    // Blocks the task until the semaphore is released
    xSemaphoreTake(AT::semaphoreLowpassInterruptToRead, portMAX_DELAY);
    if (intPir1.getLowpassInterruptsWaiting())
        log_i("Pin %u got interrupt %d", intPir1.getPin(), intPir1.receiveLowpassInterrupts());
    // Blocks the task until the semaphore is released
    xSemaphoreTake(AT::semaphoreLowpassInterruptToRead, portMAX_DELAY);
    if (intPir2.getLowpassInterruptsWaiting())
        log_i("Pin %u got interrupt %d", intPir2.getPin(), intPir2.receiveLowpassInterrupts());
}
