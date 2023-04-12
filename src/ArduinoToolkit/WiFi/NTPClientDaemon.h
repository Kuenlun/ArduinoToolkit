#pragma once

#include "ArduinoToolkit/WiFi/WiFiDaemon.h"

namespace AT
{

    namespace NTPClientDaemon
    {

        void start(const TickType_t updateDateTimePeriodTicks = pdMS_TO_TICKS(60 * 1000),
                   const TickType_t retryUpdateDateTimePeriodTicks = pdMS_TO_TICKS(1000));
        void stop();

    } // namespace NTPClientDaemon

} // namespace AT
