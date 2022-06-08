#include "Tools/Logger.h"
#include "Tools/WiFiUtils.h"


/**
 * WiFi credentials
 */
constexpr char WIFI_SSID[] = "REPLACE_WITH_YOUR_SSID";
constexpr char WIFI_PASS[] = "REPLACE_WITH_YOUR_PASSWORD";


/* * * * * *
 *  SETUP  *
 * * * * * */
void setup()
{
    Serial.begin(115200);
    Tools::logger.config("WiFi-Test");
    Tools::connectWiFi(WIFI_SSID, WIFI_PASS);
}

/* * * * * *
 *  LOOP   *
 * * * * * */
void loop()
{
}
