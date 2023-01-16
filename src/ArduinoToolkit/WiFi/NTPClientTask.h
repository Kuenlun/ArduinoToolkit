#pragma once

#include <Arduino.h>

namespace AT
{

    namespace NTPClientTask
    {

        TaskHandle_t createTask(const uint32_t& updateTimeClientPeriodMs = 60 * 1000);

    } // namespace NTPClientTask

} // namespace AT
