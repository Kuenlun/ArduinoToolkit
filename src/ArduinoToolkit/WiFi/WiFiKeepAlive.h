#pragma once

#include <Arduino.h>

namespace AT
{

    namespace WiFiKeepAlive
    {

        TaskHandle_t createTask(const char *const ssid, const char *const passphrase);
        BaseType_t blockUntilWiFiConnected(const TickType_t xTicksToWait=portMAX_DELAY);
        bool isWiFiConnected();
        void addWiFiDependentTask(const TaskHandle_t taskHandle);

    } // namespace WiFiKeepAlive

} // namespace AT
