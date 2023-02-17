#include <ArduinoToolkit.h>

#include "secrets.h"

/* * * * * *
 *  SETUP  *
 * * * * * */
void setup()
{
    {
        // Create the WiFi Daemon
        auto wifiDaemon{AT::WiFiDaemon::instance(WIFI_SSID, WIFI_PASS, 2)};
        vTaskDelay(pdMS_TO_TICKS(10000));
        // WiFiDaemon is destroyed here as it goes out of scope
    }
    // Delete setup and loop task
    log_i("Deleting setup and loop task");
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
