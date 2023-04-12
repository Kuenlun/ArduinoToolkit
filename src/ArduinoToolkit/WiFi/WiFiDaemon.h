#pragma once

#include <Arduino.h>
#include <WiFi.h>

#include "../Core/Assert.h"

namespace AT
{

    namespace WiFiDaemon
    {
        void start(const char *const ssid,
                   const char *const passphrase,
                   const UBaseType_t uxPriority);
        void stop();

        BaseType_t blockUntilConnected(const TickType_t xTicksToWait = portMAX_DELAY);
        bool isConnected();
        void addDependentTask(TaskHandle_t task = nullptr);

    } // namespace WiFiDaemon

} // namespace AT
