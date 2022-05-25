#include "Tools/TimeConversions.h"
#include "Tools/Logger.h"


/* * * * * *
 *  SETUP  *
 * * * * * */
void setup()
{
    Serial.begin(115200);
    Tools::logger.config("TIME");

    delay(Tools::secsToMillis(5));

    LOG_TRACE(Tools::secsToMillis(1));
    LOG_TRACE(Tools::minsToMillis(1));
    LOG_TRACE(Tools::hoursToMillis(1));
    LOG_TRACE(Tools::daysToMillis(1));

    LOG_TRACE(Tools::millisToSecs(1000));
    LOG_TRACE(Tools::millisToMins(60000));
    LOG_TRACE(Tools::millisToHours(3600000));
    LOG_TRACE(Tools::millisToDays(86400000));
}

/* * * * * *
 *  LOOP   *
 * * * * * */
void loop()
{
}
