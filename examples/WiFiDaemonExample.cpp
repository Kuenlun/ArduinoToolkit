#include <ArduinoToolkit/WiFi/WiFiDaemon.h>

#include "secrets.h"

/* * * * * *
 *  SETUP  *
 * * * * * */
void setup()
{
    // Start the WiFi Daemon
    AT::WiFiDaemon::start(WIFI_SSID, WIFI_PASS, 2);
    vTaskDelay(pdMS_TO_TICKS(10 * 1000));
    // WiFiDaemon is destroyed here as it goes out of scope
    AT::WiFiDaemon::stop();
    // Delete setup and loop task
    LOG_I("Deleting setup and loop task");
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
