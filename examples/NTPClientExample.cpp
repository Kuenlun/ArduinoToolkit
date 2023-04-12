#include "ArduinoToolkit/WiFi/NTPClientDaemon.h"

#include "secrets.h"

/* * * * * *
 *  SETUP  *
 * * * * * */
void setup()
{
    // Start the WiFi Daemon
    AT::WiFiDaemon::start(WIFI_SSID, WIFI_PASS, 2);
    // Wait for WiFi to connect
    AT::WiFiDaemon::blockUntilConnected();
    // Start the NTPClient datetime daemon
    AT::NTPClientDaemon::start();
    vTaskDelay(pdMS_TO_TICKS(30 * 1000));
    // Stop the NTPClient datetime daemon
    AT::NTPClientDaemon::stop();
    // Stop the WiFi Daemon
    AT::WiFiDaemon::stop();
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
