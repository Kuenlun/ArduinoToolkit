#include <ArduinoToolkit.h>

#include "secrets.h"

/* * * * * *
 *  SETUP  *
 * * * * * */
void setup()
{
    // Create the WiFi daemon task
    AT::WiFiDaemon::createDaemon(WIFI_SSID, WIFI_PASS);
    // Create the NTPClient datetime daemon
    AT::NTPClientDaemon::createDaemon();
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
