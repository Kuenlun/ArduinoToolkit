#include <Arduino.h>
#include <WiFi.h>
#include "Logger.h"
#include "TimedExecution.h"
#include "WiFiUtils.h"


namespace Tools {

    inline static void printLoadingDots() { LOG_DEBUG_RAW('.'); }

    bool connectWiFi(const char* const ssid,
                     const char* const pass,
                     const bool timeoutRestart,
                     const unsigned long waitTime)
    {
        LOG_INFO("WiFi -> Connecting to %s", ssid);
        WiFi.begin(ssid, pass);
        while (WiFi.status() != WL_CONNECTED)
        {
            static unsigned long startTime = millis();
            // Check for timeout
            if (millis() - startTime >= waitTime) {
                LOG_DEBUG_NL();
                if (timeoutRestart) {
                    LOG_FATAL("WiFi connection timeout");
                    ESP.restart();
                } else {
                    LOG_ERROR("WiFi connection timeout");
                    return false;
                }
            }
            TIMED_EXECUTION(200, true, printLoadingDots);
        }
        LOG_DEBUG_NL();
        LOG_INFO("WiFi -> Connected to %s with IP %s in channgel %d",
                 ssid, WiFi.localIP().toString().c_str(), WiFi.channel());
        return true;
    }

}