/**
 * NOTE
 * AT::OTA::executeOTA(OTA_URL) does not work when the current
 * partition scheme is "huge_app.csv"
 * 
 * The OTA_URL should be like the following for AWS S3:
 * "bucket-name.s3.eu-central-1.amazonaws.com/sketchname.bin"
 */

#include <ArduinoToolkit.h>

#include "secrets.h"

/* * * * * *
 *  SETUP  *
 * * * * * */
void setup()
{
    // Create the WiFi daemon task
    AT::WiFiDaemon::createDaemon(WIFI_SSID, WIFI_PASS);
    // Wait for WiFi to connect
    AT::WiFiDaemon::blockUntilConnected();
    // Execute OTA
    AT::OTA::executeOTA(OTA_URL);
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
