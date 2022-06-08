#include <WiFi.h>
#include "Tools/Logger.h"
#include "Tools/TimedExecution.h"


void printMACAddress() { LOG_INFO(WiFi.macAddress()); }


/* * * * * *
 *  SETUP  *
 * * * * * */
void setup()
{
    // Configure logger
    Serial.begin(115200);
    Tools::logger.config("GetMACAddress");

    // Configure WiFi
    WiFi.mode(WIFI_MODE_STA);
}


/* * * * * *
 *  LOOP   *
 * * * * * */
void loop()
{
    // Print the MAC address every 5 seconds
    TIMED_EXECUTION(Tools::secsToMillis(5), true, printMACAddress);
}
