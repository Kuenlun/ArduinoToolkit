#pragma once

#include "TimeConversions.h"


namespace Tools {

    /**
     * @brief Connect to WiFi
     * 
     * @param ssid              Network SSID
     * @param pass              Network password
     * @param timeoutRestart    Restart on timeout (default true)
     * @param waitTime          Max time to wait for the connection to stablish (default 30 seconds)
     * @return true             Success
     * @return false            Failed to connect to WiFi
     */
    bool connectWiFi(const char* const ssid,
                     const char* const pass=nullptr,
                     const bool timeoutRestart=true,
                     const unsigned long waitTime=secsToMillis(30));
    
}
