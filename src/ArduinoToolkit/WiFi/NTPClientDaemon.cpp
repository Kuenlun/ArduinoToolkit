#include <WiFiUdp.h>

#include <NTPClient.h>

#include "ArduinoToolkit/WiFi/NTPClientDaemon.h"

namespace AT
{

    namespace NTPClientDaemon
    {

        // Static variables
        static WiFiUDP ntpUDP;
        static NTPClient timeClient(ntpUDP);
        static uint32_t updateDateTimePeriodTicks;
        static uint32_t retryUpdateDateTimePeriodTicks;
        static TimerHandle_t timerUpdateDateTime;

        // Static functions
        static void changeTimerPeriodToNormal(const TimerHandle_t &xTimer,
                                              BaseType_t *const pxHigherPriorityTaskWoken)
        {
            if (xTimerGetPeriod(xTimer) != updateDateTimePeriodTicks)
            {
                xTimerChangePeriodFromISR(xTimer, updateDateTimePeriodTicks, pxHigherPriorityTaskWoken);
                AT_LOG_V("Update DateTime timer period changed to %ums", pdTICKS_TO_MS(updateDateTimePeriodTicks));
            }
        }

        static void changeTimerPeriodToRetry(const TimerHandle_t &xTimer,
                                             BaseType_t *const pxHigherPriorityTaskWoken)
        {
            if (xTimerGetPeriod(xTimer) != retryUpdateDateTimePeriodTicks)
            {
                xTimerChangePeriodFromISR(xTimer, retryUpdateDateTimePeriodTicks, pxHigherPriorityTaskWoken);
                AT_LOG_V("Update DateTime timer period changed to %ums", pdTICKS_TO_MS(retryUpdateDateTimePeriodTicks));
            }
        }

        static void timerUpdateDateTimeCB(const TimerHandle_t xTimer)
        {
            BaseType_t xHigherPriorityTaskWoken{pdFALSE};
            if (WiFiDaemon::isConnected())
            {
                if (timeClient.forceUpdate())
                {
                    AT_LOG_D("Current time is: %s", timeClient.getFormattedTime());
                    changeTimerPeriodToNormal(xTimer, &xHigherPriorityTaskWoken);
                }
                else
                {
                    AT_LOG_W("Could not update timeClient");
                    changeTimerPeriodToRetry(xTimer, &xHigherPriorityTaskWoken);
                }
            }
            else
            {
                AT_LOG_W("Could not update timeClient because WiFi is not connected");
                changeTimerPeriodToRetry(xTimer, &xHigherPriorityTaskWoken);
            }
            // Did this action unblock a higher priority task?
            if (xHigherPriorityTaskWoken)
                portYIELD_FROM_ISR();
        }

        // Public functions
        void start(const TickType_t _updateDateTimePeriodTicks,
                   const TickType_t _retryUpdateDateTimePeriodTicks)
        {
            // Initialize static variables
            updateDateTimePeriodTicks = _updateDateTimePeriodTicks;
            retryUpdateDateTimePeriodTicks = _retryUpdateDateTimePeriodTicks;

            // Initialize a NTPClient to get time
            timeClient.begin();
            // Set offset time in seconds to adjust for your timezone, for example:
            // GMT +1 = 3600
            timeClient.setTimeOffset(3600);

            // Create the update datetime timer
            timerUpdateDateTime = xTimerCreate(
                "timerUpdateDatetime",
                retryUpdateDateTimePeriodTicks,
                pdTRUE,
                (void *)0,
                timerUpdateDateTimeCB);
            if (!timerUpdateDateTime)
                AT_LOG_E("Could not create timer");
            // Start the timer
            xTimerStart(timerUpdateDateTime, portMAX_DELAY);
        }

        void stop()
        {
            xTimerStop(timerUpdateDateTime, portMAX_DELAY);
            timeClient.end();
            AT_LOG_I("NTPClientDaemon Deleted");
        }

    } // namespace NTPClientDaemon

} // namespace AT
