#pragma once

#include <Arduino.h>

namespace AT
{

    namespace NTPClientTask
    {

        void createDaemon(const TickType_t &updateDateTimePeriodTicks = pdMS_TO_TICKS(60 * 1000),
                          const TickType_t &retryUpdateDateTimePeriodTicks = pdMS_TO_TICKS(1000));

    } // namespace NTPClientTask

} // namespace AT
