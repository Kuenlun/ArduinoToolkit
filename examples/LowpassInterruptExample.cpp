#include <ArduinoToolkit.h>

static AT::LowpassInterrupt<1> pirInt1(INPUT_PULLDOWN,
                                       pdMS_TO_TICKS(100),
                                       pdMS_TO_TICKS(1000),
                                       pdMS_TO_TICKS(30000));
static AT::LowpassInterrupt<2> pirInt2(INPUT_PULLDOWN,
                                       pdMS_TO_TICKS(100),
                                       pdMS_TO_TICKS(1000),
                                       pdMS_TO_TICKS(30000));

/* * * * * *
 *  SETUP  *
 * * * * * */
void setup()
{
    pirInt1.begin();
    pirInt2.begin();

    // Delete setup and loop task
    vTaskDelete(NULL);
}

/* * * * * *
 *  LOOP   *
 * * * * * */
void loop()
{
    // This task has been deleted
    // Code here won't run
}
