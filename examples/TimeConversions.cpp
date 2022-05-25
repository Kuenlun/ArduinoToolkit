#include "TimeConversions.h"


/* * * * * *
 *  SETUP  *
 * * * * * */
void setup()
{
    Serial.begin(115200);
    delay(Tools::secsToMillis(5));

    Serial.println(Tools::secsToMillis(1));
    Serial.println(Tools::minsToMillis(1));
    Serial.println(Tools::hoursToMillis(1));
    Serial.println(Tools::daysToMillis(1));

    Serial.println(Tools::millisToSecs(1000));
    Serial.println(Tools::millisToMins(60000));
    Serial.println(Tools::millisToHours(3600000));
    Serial.println(Tools::millisToDays(86400000));
}

/* * * * * *
 *  LOOP   *
 * * * * * */
void loop()
{
}