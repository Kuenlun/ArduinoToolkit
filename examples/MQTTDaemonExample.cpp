#include <ArduinoToolkit.h>

#include "secrets.h"

// AWS Certificates
static constexpr char AWS_CERT_CA[] = {
#include "AmazonRootCA1.pem"
};
static constexpr char AWS_PEM_CRT[] = {
#include "certificate.pem.crt"
};
static constexpr char AWS_PRIV_KEY[] = {
#include "private.pem.key"
};

/* * * * * *
 *  SETUP  *
 * * * * * */
void setup()
{
    // Create the WiFi daemon task
    AT::WiFiDaemon::createDaemon(WIFI_SSID, WIFI_PASS);
    // Create the MQTT daemon task
    AT::MQTTDaemon::createDaemon();
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
