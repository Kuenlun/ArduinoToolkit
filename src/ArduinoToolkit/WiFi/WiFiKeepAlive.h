#pragma once

#include <Arduino.h>

namespace AT
{

    namespace WiFiKeepAlive
    {

        TaskHandle_t createDaemon(const char *const ssid, const char *const passphrase);
        BaseType_t blockUntilConnected(const TickType_t &xTicksToWait = portMAX_DELAY);
        bool isConnected();
        void addDependentTask(const TaskHandle_t &taskHandle);

    } // namespace WiFiKeepAlive

} // namespace AT
