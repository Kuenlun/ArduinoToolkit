#include <ArduinoToolkit.h>

#include "secrets.h"

/* * * * * *
 *  SETUP  *
 * * * * * */
void setup()
{
    // Create the WiFi Keep Alive task
    AT::WiFiKeepAlive::createTask(WIFI_SSID, WIFI_PASS);

    AT::NTPClientTask::createTask();

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
