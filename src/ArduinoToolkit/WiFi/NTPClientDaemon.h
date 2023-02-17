#pragma once

#include <Arduino.h>
#include <NTPClient.h>

#include "Daemon.h"

namespace AT
{

    class NTPClientDaemon : public Daemon<NTPClientDaemon>
    {
    protected:
        NTPClientDaemon(const TickType_t updateDateTimePeriodTicks = pdMS_TO_TICKS(60 * 1000),
                        const TickType_t retryUpdateDateTimePeriodTicks = pdMS_TO_TICKS(1000));
        ~NTPClientDaemon();

    private:
        static void timerUpdateDateTimeCB(const TimerHandle_t xTimer);
        static void changeTimerPeriodToNormal(const TimerHandle_t &xTimer,
                                              BaseType_t *const pxHigherPriorityTaskWoken);
        static void changeTimerPeriodToRetry(const TimerHandle_t &xTimer,
                                             BaseType_t *const pxHigherPriorityTaskWoken);

    private:
        TimerHandle_t m_timerUpdateDateTime;

    private:
        static NTPClient s_timeClient;
        static uint32_t s_updateDateTimePeriodTicks;
        static uint32_t s_retryUpdateDateTimePeriodTicks;
    }; // class NTPClientDaemon

} // namespace AT
