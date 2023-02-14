#include <ArduinoToolkit.h>

#include "secrets.h"

/* * * * * *
 *  SETUP  *
 * * * * * */
void setup()
{
    {
        // Create the WiFi daemon task
        AT::WiFiDaemon wifiDaemon(WIFI_SSID, WIFI_PASS, 2);
        vTaskDelay(pdMS_TO_TICKS(10000));
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
